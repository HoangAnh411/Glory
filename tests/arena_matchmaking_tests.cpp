#include "glory/game/arena.hpp"
#include "glory/game/combat.hpp"
#include "glory/game/prediction.hpp"
#include "glory/game/world_history.hpp"
#include "glory/network/network.hpp"
#include "glory/server/server_simulation.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <glm/geometric.hpp>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::filesystem::path asset(const char* file) {
    return std::filesystem::path(GLORY_ASSET_DIR) / "data" / file;
}

void testQueueModesAndLifecycle() {
    using namespace glory::game;
    ArenaManager duel;
    require(duel.request(1, QueueMode::Duel1v1, QueueAction::Join), "first duel queue request rejected");
    require(duel.state() == MatchState::Waiting && duel.queuedCount(QueueMode::Duel1v1) == 1,
        "duel started before two clients queued");
    require(duel.request(2, QueueMode::Duel1v1, QueueAction::Join), "second duel queue request rejected");
    require(duel.state() == MatchState::Countdown && duel.players().size() == 2 &&
        duel.countdownRemaining() == 3.0, "1v1 did not enter the three-second countdown");
    const auto red = duel.player(1);
    const auto blue = duel.player(2);
    require(red && blue && red->team == TeamId::Red && blue->team == TeamId::Blue &&
        red->spawnPosition.x == -5.0F && blue->spawnPosition.x == 5.0F,
        "duel teams were not assigned distinct side spawns");
    require(duel.locksPlayers(), "countdown did not lock movement and combat");
    duel.tick(2.99);
    require(duel.state() == MatchState::Countdown, "countdown ended too early");
    duel.tick(0.01);
    require(duel.inRound() && !duel.locksPlayers(), "round did not unlock at countdown end");
    require(!duel.canDamage(1, 1) && duel.canDamage(1, 2),
        "friendly-fire filter did not protect teammates while allowing enemies");
    duel.setPlayerHealth(2, 0.0F);
    require(duel.state() == MatchState::MatchFinished && duel.winningTeam() == TeamId::Red,
        "server did not declare the surviving team as winner");

    ArenaManager teams;
    for (std::uint32_t id = 10; id < 16; ++id)
        require(teams.request(id, QueueMode::Team3v3, QueueAction::Join), "3v3 queue request rejected");
    require(teams.state() == MatchState::Countdown && teams.players().size() == 6,
        "3v3 did not start at six queued clients");
    std::size_t redCount = 0;
    std::size_t blueCount = 0;
    for (const auto& [id, player] : teams.players()) {
        (void)id;
        redCount += player.team == TeamId::Red;
        blueCount += player.team == TeamId::Blue;
    }
    require(redCount == 3 && blueCount == 3 && teams.player(10)->spawnPosition.z !=
        teams.player(11)->spawnPosition.z, "3v3 teams or formation spawn points are incorrect");
}

void testServerCountdownAndTeamSnapshot() {
    using namespace glory;
    server::ServerSimulation simulation(asset("world_spawns.json"), asset("skills.json"));
    require(simulation.connectPlayer(101) && simulation.connectPlayer(102), "server failed to create arena players");
    require(simulation.requestMatchmaking(101, game::QueueMode::Duel1v1, game::QueueAction::Join) &&
        simulation.requestMatchmaking(102, game::QueueMode::Duel1v1, game::QueueAction::Join),
        "server rejected valid duel queue requests");
    const auto before = simulation.playerState(101);
    require(before && simulation.arena().locksPlayers(), "server did not enter locked countdown");
    input::InputFrame movement;
    movement.held.push_back(input::Control::KeyW);
    movement.pressed.push_back(input::Control::MouseLeft);
    simulation.setPlayerInput(101, movement, -90.0F, 1, simulation.clock().tickCount());
    for (int tick = 0; tick < 10; ++tick) simulation.tick();
    const auto during = simulation.playerState(101);
    const auto opponentDuring = simulation.playerState(102);
    require(during && opponentDuring && glm::length(during->position - before->position) < 0.001F &&
        opponentDuring->currentHp == 100.0F,
        "countdown did not lock movement and combat on the server");
    for (int tick = 0; tick < 170; ++tick) simulation.tick();
    require(simulation.arena().inRound(), "server countdown did not complete after 180 fixed ticks");
    const auto packetState = simulation.multiplayerSnapshot();
    require(packetState.players.size() == 2 && packetState.players[0].team != game::TeamId::None &&
        packetState.matchState == game::MatchState::InRound,
        "server authoritative snapshot omitted arena state or team ids");
}

void testProtocolAndHundredMillisecondLoopback() {
    using namespace glory::network;
    const auto requestBytes = encodeMatchmakingRequest({MatchmakingMode::Team3v3, MatchmakingAction::Join});
    MatchmakingRequestPacket request;
    require(decodeMatchmakingRequest(requestBytes, request) && request.mode == MatchmakingMode::Team3v3,
        "matchmaking request codec failed round-trip");
    const std::array<std::uint8_t, 2> malformed{99, 1};
    require(!decodeMatchmakingRequest(malformed, request), "invalid matchmaking mode was accepted");

    ServerSnapshotPacket state;
    state.serverTick = 42;
    state.matchState = ArenaMatchState::Countdown;
    state.matchMode = MatchmakingMode::Duel1v1;
    state.countdownRemaining = 1.5F;
    state.players.push_back({1, {}, {}, 0.0F, 100.0F, 100.0F, "Idle", false, false,
        "", 0, true, 0.0F, 0.0F, {}, {}, 0.0F, ArenaTeam::Red});
    const auto encoded = encodeServerSnapshot(state);
    ServerSnapshotPacket decoded;
    require(decodeServerSnapshot(encoded, decoded) && decoded.matchState == ArenaMatchState::Countdown &&
        decoded.countdownRemaining == 1.5F && decoded.players.front().team == ArenaTeam::Red,
        "arena snapshot fields failed binary round-trip");

    NetworkContext context;
    NetworkServer server(context, 0, 2);
    NetworkClient client(context);
    NetworkClient opponent(context);
    require(client.connect("127.0.0.1", server.port()) &&
        opponent.connect("127.0.0.1", server.port()), "loopback clients failed to initiate connection");
    unsigned acceptedCount = 0;
    const auto connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (acceptedCount < 2 && std::chrono::steady_clock::now() < connectDeadline) {
        for (const auto& event : server.poll())
            acceptedCount += event.type == EventType::Connected;
        client.poll();
        opponent.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(acceptedCount == 2 && client.state() == ConnectionState::Connected &&
        opponent.state() == ConnectionState::Connected, "two-client loopback ENet handshake failed");

    const auto duelRequest = encodeMatchmakingRequest({MatchmakingMode::Duel1v1, MatchmakingAction::Join});
    require(client.send(PacketType::MatchmakingRequest, duelRequest, true) &&
        opponent.send(PacketType::MatchmakingRequest, duelRequest, true),
        "loopback clients could not send matchmaking requests");
    client.poll();
    opponent.poll();
    unsigned receivedQueueRequests = 0;
    const auto queueDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (receivedQueueRequests < 2 && std::chrono::steady_clock::now() < queueDeadline) {
        for (const auto& event : server.poll())
            receivedQueueRequests += event.type == EventType::PacketReceived &&
                event.packetType == PacketType::MatchmakingRequest;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(receivedQueueRequests == 2, "server did not receive matchmaking requests from both clients");

    const auto pingStart = std::chrono::steady_clock::now();
    client.sendPing();
    client.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto serverServiceDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
    while (std::chrono::steady_clock::now() < serverServiceDeadline) {
        server.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    bool gotPong = false;
    const auto clientServiceDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while (!gotPong && std::chrono::steady_clock::now() < clientServiceDeadline) {
        for (const auto& event : client.poll()) gotPong |= event.packetType == PacketType::Pong;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - pingStart).count();
    if (!gotPong || elapsed < 100 || elapsed >= 500)
        throw std::runtime_error("injected 100 ms RTT loopback failed: pong=" +
            std::to_string(gotPong) + " elapsed=" + std::to_string(elapsed) +
            " measured=" + std::to_string(client.rttMilliseconds()));

    glory::game::CharacterController controller;
    glory::game::ClientPrediction prediction(controller);
    glory::input::InputFrame forward;
    forward.held.push_back(glory::input::Control::KeyW);
    prediction.predict(forward, 0.0F, 1.0 / 60.0);
    require(controller.state().position.x > 0.0F,
        "local prediction did not move immediately while awaiting the delayed server response");

    glory::game::SkillDefinition attack;
    attack.id = "combo_1";
    attack.name = "Latency Test Slash";
    attack.damage = 25.0F;
    attack.animationState = "Attack";
    attack.hitboxRadius = 0.2F;
    attack.hitboxOffset = {2.0F, 0.85F, 0.0F};
    attack.activeStart = 0.0F;
    attack.activeEnd = 1.0F;
    attack.comboWindow = 0.3F;
    attack.cancelWindowStart = 1.1F;
    glory::game::LagCompensator lagCompensator;
    for (std::uint64_t tick = 0; tick <= 24; ++tick) {
        const float x = static_cast<float>(tick) / 6.0F;
        lagCompensator.record(tick, {{2, {{x, 0.0F, 0.0F}, 0.4F, 1.8F}, 100.0F, 100.0F}});
    }
    const auto historicalTarget = lagCompensator.rewindHurtbox(2, 24, 12.0);
    glory::game::CombatTarget currentTarget{2, {{4.0F, 0.0F, 0.0F}, 0.4F, 1.8F}, 100.0F, 100.0F, 0};
    glory::game::CombatSystem combat(glory::game::SkillRegistry::fromSkills({attack}), currentTarget);
    require(historicalTarget.has_value(), "100 ms attack had no historical target sample");
    combat.update(1.0F / 60.0F, {0.0F, 0.0F, 0.0F}, 0.0F, true, "Attack", false, false,
        1.0F, false, &historicalTarget->entity.hurtbox);
    require(combat.hitEvents().size() == 1 && combat.dummy().currentHp == 75.0F &&
        combat.dummy().hurtbox.base.x == 4.0F,
        "100 ms delayed attack did not hit historical target while preserving current position");
    client.disconnect();
    opponent.disconnect();
    std::cout << "Arena passed: duel/3v3 lifecycle, authoritative team snapshot, delayed loopback RTT="
        << elapsed << " ms, local prediction and historical hit\n";
}

} // namespace

int main() {
    try {
        testQueueModesAndLifecycle();
        testServerCountdownAndTeamSnapshot();
        testProtocolAndHundredMillisecondLoopback();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

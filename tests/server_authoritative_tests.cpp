#include "glory/network/network.hpp"
#include "glory/server/server_simulation.hpp"

#include <glm/geometric.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::filesystem::path asset(const char* file) {
    return std::filesystem::path(GLORY_ASSET_DIR) / "data" / file;
}

glory::network::ServerSnapshotPacket makePacket(const glory::server::MultiplayerSnapshot& world) {
    using namespace glory;
    network::ServerSnapshotPacket packet;
    packet.serverTick = world.serverTick;
    for (const auto& player : world.players) {
        packet.players.push_back({player.clientId,
            {player.position.x, player.position.y, player.position.z},
            {player.velocity.x, player.velocity.y, player.velocity.z}, player.yaw, player.currentHp,
            player.maxHp, player.animationState, player.isDodging, player.isInvulnerable,
            player.activeSkillId, player.lastProcessedInputSequence, player.isGrounded,
            player.dodgeElapsed, player.dodgeCooldownRemaining,
            {player.dodgeDirection.x, player.dodgeDirection.y, player.dodgeDirection.z},
            {player.externalKnockbackVelocity.x, player.externalKnockbackVelocity.y,
                player.externalKnockbackVelocity.z}, player.knockbackRemaining});
    }
    packet.boss = {{world.bossPosition.x, world.bossPosition.y, world.bossPosition.z},
        world.bossCurrentHp, world.bossMaxHp, world.bossPhase};
    return packet;
}

glory::input::InputFrame decodeInput(const glory::network::ClientInputPacket& packet) {
    glory::input::InputFrame frame;
    for (unsigned bit = 1; bit <= static_cast<unsigned>(glory::input::Control::MouseRight); ++bit) {
        const auto control = static_cast<glory::input::Control>(bit);
        if ((packet.pressedButtons & (1U << bit)) != 0) frame.pressed.push_back(control);
        if ((packet.heldButtons & (1U << bit)) != 0) frame.held.push_back(control);
    }
    return frame;
}

} // namespace

int main() {
    using namespace glory;
    try {
        network::NetworkContext context;
        network::NetworkServer transport(context, 0, 2);
        network::NetworkClient first(context);
        network::NetworkClient second(context);
        server::ServerSimulation simulation(asset("world_spawns.json"), asset("skills.json"));
        require(first.connect("127.0.0.1", transport.port()), "first client connect start failed");

        std::uint32_t firstId = 0;
        std::uint32_t secondId = 0;
        const auto connectOneDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (first.state() != network::ConnectionState::Connected &&
            std::chrono::steady_clock::now() < connectOneDeadline) {
            for (const auto& event : transport.poll())
                if (event.type == network::EventType::Connected) {
                    firstId = event.clientId;
                    require(simulation.connectPlayer(firstId), "server failed to create first player");
                }
            first.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(first.state() == network::ConnectionState::Connected && firstId != 0,
            "first handshake failed");
        require(second.connect("127.0.0.1", transport.port()), "second client connect start failed");
        const auto connectTwoDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (second.state() != network::ConnectionState::Connected &&
            std::chrono::steady_clock::now() < connectTwoDeadline) {
            for (const auto& event : transport.poll())
                if (event.type == network::EventType::Connected) {
                    secondId = event.clientId;
                    require(simulation.connectPlayer(secondId), "server failed to create second player");
                }
            first.poll();
            second.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(second.state() == network::ConnectionState::Connected && secondId != 0 &&
            firstId != secondId && simulation.playerCount() == 2, "two independent player sessions were not created");

        std::uint32_t firstInputSequence = 0;
        std::uint32_t secondInputSequence = 0;
        const auto tickWithInputs = [&](const network::ClientInputPacket& firstInput,
            const network::ClientInputPacket& secondInput) {
            auto sequencedFirstInput = firstInput;
            auto sequencedSecondInput = secondInput;
            sequencedFirstInput.inputSequence = ++firstInputSequence;
            sequencedSecondInput.inputSequence = ++secondInputSequence;
            sequencedFirstInput.targetTick = simulation.clock().tickCount();
            sequencedSecondInput.targetTick = simulation.clock().tickCount();
            const auto firstBytes = network::encodeClientInput(sequencedFirstInput);
            const auto secondBytes = network::encodeClientInput(sequencedSecondInput);
            first.send(network::PacketType::ClientInput, firstBytes, false);
            second.send(network::PacketType::ClientInput, secondBytes, false);
            first.poll();
            second.poll();
            for (const auto& event : transport.poll()) {
                if (event.type != network::EventType::PacketReceived ||
                    event.packetType != network::PacketType::ClientInput) continue;
                network::ClientInputPacket decoded;
                if (!network::decodeClientInput(event.payload, decoded)) continue;
                simulation.setPlayerInput(event.clientId, decodeInput(decoded), decoded.cameraYawDegrees,
                    decoded.inputSequence, decoded.targetTick);
            }
            simulation.tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        };

        const auto firstBefore = simulation.playerState(firstId);
        const auto secondBefore = simulation.playerState(secondId);
        require(firstBefore && secondBefore, "initial player snapshot missing");
        const network::ClientInputPacket moving{0, 1U << static_cast<unsigned>(input::Control::KeyW), -90.0F};
        const network::ClientInputPacket neutral{0, 0, -90.0F};
        for (int tick = 0; tick < 12; ++tick) tickWithInputs(moving, neutral);
        const auto firstMoved = simulation.playerState(firstId);
        const auto secondStayed = simulation.playerState(secondId);
        require(firstMoved && secondStayed &&
            glm::length(firstMoved->position - firstBefore->position) > 0.2F,
            "server did not authoritatively move the input client");
        require(firstMoved->lastProcessedInputSequence > 0 &&
            firstMoved->lastProcessedInputSequence <= firstInputSequence,
            "server snapshot state did not retain the last processed input sequence");
        require(glm::length(secondStayed->position - secondBefore->position) < 0.001F,
            "idle client position changed due to another client's input");

        for (int tick = 0; tick < 8; ++tick) tickWithInputs(neutral, neutral);
        const network::ClientInputPacket attack{1U << static_cast<unsigned>(input::Control::MouseLeft),
            1U << static_cast<unsigned>(input::Control::MouseLeft), -90.0F};
        tickWithInputs(attack, neutral);
        for (int tick = 0; tick < 18; ++tick) tickWithInputs(neutral, neutral);
        const auto attacked = simulation.playerState(secondId);
        require(attacked && attacked->currentHp < 100.0F,
            "server-side hitbox combat did not reduce the opponent's HP");

        const auto encoded = network::encodeServerSnapshot(makePacket(simulation.multiplayerSnapshot()));
        transport.broadcast(network::PacketType::ServerSnapshot, encoded, false);
        bool receivedDamageSnapshot = false;
        bool receivedInputAcknowledgement = false;
        const auto snapshotDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!receivedDamageSnapshot && std::chrono::steady_clock::now() < snapshotDeadline) {
            for (const auto& event : first.poll()) {
                network::ServerSnapshotPacket decoded;
                if (event.packetType == network::PacketType::ServerSnapshot &&
                    network::decodeServerSnapshot(event.payload, decoded)) {
                    for (const auto& player : decoded.players)
                        if (player.clientId == secondId && player.currentHp < 100.0F)
                            receivedDamageSnapshot = true;
                        else if (player.clientId == firstId && player.lastProcessedInputSequence > 0)
                            receivedInputAcknowledgement = true;
                }
            }
            second.poll();
            transport.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(receivedDamageSnapshot, "client did not receive authoritative HP in world snapshot");
        require(receivedInputAcknowledgement,
            "server snapshot did not acknowledge a processed client input sequence");

        second.disconnect();
        bool sawDisconnect = false;
        const auto disconnectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!sawDisconnect && std::chrono::steady_clock::now() < disconnectDeadline) {
            first.poll();
            second.poll();
            for (const auto& event : transport.poll())
                if (event.type == network::EventType::Disconnected && event.clientId == secondId) {
                    simulation.disconnectPlayer(event.clientId);
                    sawDisconnect = true;
                }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(sawDisconnect && simulation.playerCount() == 1 && !simulation.playerState(secondId),
            "disconnect did not clean up the associated authoritative entity");

        std::cout << "Authoritative loopback passed: isolated movement, PvP damage, snapshot, disconnect\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

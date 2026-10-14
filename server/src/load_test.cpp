#include "glory/server/load_test.hpp"

#include "glory/game/simulation.hpp"
#include "glory/network/network.hpp"
#include "glory/server/server_simulation.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef GLORY_ASSET_DIR
#define GLORY_ASSET_DIR "assets"
#endif

namespace glory::server {
namespace {

using Clock = std::chrono::steady_clock;
using namespace glory;

struct BotClient {
    explicit BotClient(network::NetworkContext& context, std::size_t index)
        : client(std::make_unique<network::NetworkClient>(context)),
          username("loadbot" + std::to_string(index)), token("bot-session-" + std::to_string(index)),
          characterId(static_cast<std::int64_t>(index + 1)), randomState(static_cast<std::uint32_t>(index + 1)) {}

    std::unique_ptr<network::NetworkClient> client;
    BotState state = BotState::Disconnected;
    std::string username;
    std::string token;
    std::int64_t characterId = 0;
    std::uint32_t inputSequence = 0;
    std::uint32_t randomState = 1;
    std::size_t snapshots = 0;

    std::uint32_t nextRandom() noexcept {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return randomState;
    }

    bool send(network::PacketType type, const std::vector<std::uint8_t>& bytes,
        LoadTestMetrics& metrics, bool reliable = true) {
        if (!client->send(type, bytes, reliable)) return false;
        ++metrics.packetsSent;
        metrics.bytesTransferred += bytes.size() + 1;
        return true;
    }

    template<std::size_t Size>
    bool send(network::PacketType type, const std::array<std::uint8_t, Size>& bytes,
        LoadTestMetrics& metrics, bool reliable = true) {
        if (!client->send(type, bytes, reliable)) return false;
        ++metrics.packetsSent;
        metrics.bytesTransferred += bytes.size() + 1;
        return true;
    }
};

std::filesystem::path assetPath(const char* file) {
    return std::filesystem::path(GLORY_ASSET_DIR) / "data" / file;
}

network::ServerSnapshotPacket makePacket(const MultiplayerSnapshot& world) {
    network::ServerSnapshotPacket packet;
    packet.serverTick = world.serverTick;
    packet.players.reserve(world.players.size());
    for (const auto& player : world.players) {
        packet.players.push_back({player.clientId,
            {player.position.x, player.position.y, player.position.z},
            {player.velocity.x, player.velocity.y, player.velocity.z}, player.yaw,
            player.currentHp, player.maxHp, player.animationState, player.isDodging,
            player.isInvulnerable, player.activeSkillId, player.lastProcessedInputSequence,
            player.isGrounded, player.dodgeElapsed, player.dodgeCooldownRemaining,
            {player.dodgeDirection.x, player.dodgeDirection.y, player.dodgeDirection.z},
            {player.externalKnockbackVelocity.x, player.externalKnockbackVelocity.y,
                player.externalKnockbackVelocity.z}, player.knockbackRemaining,
            static_cast<network::ArenaTeam>(player.team)});
    }
    packet.boss = {{world.bossPosition.x, world.bossPosition.y, world.bossPosition.z},
        world.bossCurrentHp, world.bossMaxHp, world.bossPhase};
    return packet;
}

input::InputFrame decodeInput(const network::ClientInputPacket& packet) {
    input::InputFrame frame;
    for (unsigned bit = 1; bit <= static_cast<unsigned>(input::Control::MouseRight); ++bit) {
        const auto control = static_cast<input::Control>(bit);
        if ((packet.pressedButtons & (1U << bit)) != 0) frame.pressed.push_back(control);
        if ((packet.heldButtons & (1U << bit)) != 0) frame.held.push_back(control);
    }
    return frame;
}

bool sendServerPacket(network::NetworkServer& transport, std::uint32_t clientId,
    network::PacketType type, const std::vector<std::uint8_t>& payload, LoadTestMetrics& metrics) {
    if (!transport.send(clientId, type, payload, true)) return false;
    ++metrics.packetsSent;
    metrics.bytesTransferred += payload.size() + 1;
    return true;
}

void accountBotEvents(BotClient& bot, const std::vector<network::NetworkEvent>& events,
    LoadTestMetrics& metrics) {
    for (const auto& event : events) {
        if (event.type == network::EventType::Connected) {
            bot.state = BotState::Authenticating;
            const auto payload = network::encodeLoginRequest({bot.username, "load-test-password"});
            if (!bot.send(network::PacketType::LoginRequest, payload, metrics))
                throw std::runtime_error("bot could not send login request");
        } else if (event.type == network::EventType::PacketReceived) {
            ++metrics.packetsReceived;
            metrics.bytesTransferred += event.payload.size() + 1;
            if (event.packetType == network::PacketType::LoginResponse) {
                network::LoginResponsePacket response;
                if (!network::decodeLoginResponse(event.payload, response) ||
                    response.result != network::AuthResult::Success)
                    throw std::runtime_error("bot login exchange failed");
                const auto payload = network::encodeCharacterListRequest({bot.token});
                if (!bot.send(network::PacketType::CharacterListRequest, payload, metrics))
                    throw std::runtime_error("bot could not request characters");
            } else if (event.packetType == network::PacketType::CharacterListResponse) {
                network::CharacterListResponsePacket response;
                if (!network::decodeCharacterListResponse(event.payload, response) ||
                    response.result != network::AuthResult::Success)
                    throw std::runtime_error("bot character-list exchange failed");
                const auto payload = network::encodeCreateCharacterRequest(
                    {bot.token, bot.username, "Vanguard"});
                if (!bot.send(network::PacketType::CreateCharacterRequest, payload, metrics))
                    throw std::runtime_error("bot could not create a character");
            } else if (event.packetType == network::PacketType::CreateCharacterResponse) {
                network::CreateCharacterResponsePacket response;
                if (!network::decodeCreateCharacterResponse(event.payload, response) ||
                    response.result != network::AuthResult::Success)
                    throw std::runtime_error("bot character creation exchange failed");
                bot.characterId = response.character.characterId;
                bot.state = BotState::EnteringWorld;
                const auto payload = network::encodeEnterWorldRequest({bot.token, bot.characterId});
                if (!bot.send(network::PacketType::EnterWorldRequest, payload, metrics))
                    throw std::runtime_error("bot could not enter the world");
            } else if (event.packetType == network::PacketType::EnterWorldResponse) {
                network::EnterWorldResponsePacket response;
                if (!network::decodeEnterWorldResponse(event.payload, response) ||
                    response.result != network::AuthResult::Success)
                    throw std::runtime_error("bot world-entry exchange failed");
                bot.state = BotState::SimulatingLoop;
            } else if (event.packetType == network::PacketType::ServerSnapshot) {
                network::ServerSnapshotPacket snapshot;
                if (!network::decodeServerSnapshot(event.payload, snapshot))
                    throw std::runtime_error("bot received an invalid server snapshot");
                ++bot.snapshots;
                ++metrics.snapshotsReceived;
            }
        } else if (event.type == network::EventType::Disconnected) {
            bot.state = BotState::Disconnected;
        }
    }
}

} // namespace

double LoadTestMetrics::connectedBotsRate() const noexcept {
    return requestedBots == 0 ? 0.0 : static_cast<double>(connectedBots) /
        static_cast<double>(requestedBots);
}

LoadTestCoordinator::LoadTestCoordinator(LoadTestConfig config) : config_(config) {
    if (config_.botCount == 0 || config_.botCount > 8)
        throw std::invalid_argument("bot count must be between 1 and the current server limit (8)");
    if (config_.tickCount == 0) throw std::invalid_argument("tick count must be greater than zero");
}

LoadTestMetrics LoadTestCoordinator::run() {
    LoadTestMetrics metrics;
    metrics.requestedBots = config_.botCount;
    network::NetworkContext context;
    network::NetworkServer transport(context, 0, config_.botCount);
    ServerSimulation simulation(assetPath("world_spawns.json"), assetPath("skills.json"));
    std::vector<std::unique_ptr<BotClient>> bots;
    bots.reserve(config_.botCount);
    for (std::size_t index = 0; index < config_.botCount; ++index) {
        auto bot = std::make_unique<BotClient>(context, index);
        bot->state = BotState::Connecting;
        if (!bot->client->connect("127.0.0.1", transport.port()))
            throw std::runtime_error("failed to start bot connection");
        bots.push_back(std::move(bot));
    }

    std::vector<std::uint32_t> clientIds;
    clientIds.reserve(config_.botCount);
    const auto service = [&](std::uint32_t tick) {
        for (auto& bot : bots) accountBotEvents(*bot, bot->client->poll(), metrics);
        for (const auto& event : transport.poll()) {
            if (event.type == network::EventType::Connected) {
                if (!simulation.connectPlayer(event.clientId))
                    throw std::runtime_error("server simulation rejected a load-test client");
                clientIds.push_back(event.clientId);
            } else if (event.type == network::EventType::Disconnected) {
                simulation.disconnectPlayer(event.clientId);
            } else if (event.type == network::EventType::PacketReceived) {
                ++metrics.packetsReceived;
                metrics.bytesTransferred += event.payload.size() + 1;
                if (event.packetType == network::PacketType::LoginRequest) {
                    network::LoginRequestPacket request;
                    if (!network::decodeLoginRequest(event.payload, request))
                        throw std::runtime_error("malformed bot login request");
                    const auto payload = network::encodeLoginResponse(
                        {network::AuthResult::Success, "load-test-session"});
                    (void)sendServerPacket(transport, event.clientId,
                        network::PacketType::LoginResponse, payload, metrics);
                } else if (event.packetType == network::PacketType::CharacterListRequest) {
                    network::CharacterListRequestPacket request;
                    if (!network::decodeCharacterListRequest(event.payload, request))
                        throw std::runtime_error("malformed character-list request");
                    const auto payload = network::encodeCharacterListResponse(
                        {network::AuthResult::Success, {}});
                    (void)sendServerPacket(transport, event.clientId,
                        network::PacketType::CharacterListResponse, payload, metrics);
                } else if (event.packetType == network::PacketType::CreateCharacterRequest) {
                    network::CreateCharacterRequestPacket request;
                    if (!network::decodeCreateCharacterRequest(event.payload, request))
                        throw std::runtime_error("malformed create-character request");
                    network::CharacterInfoPacket character{static_cast<std::int64_t>(event.clientId),
                        request.name, request.className, 1, 0};
                    const auto payload = network::encodeCreateCharacterResponse(
                        {network::AuthResult::Success, character});
                    (void)sendServerPacket(transport, event.clientId,
                        network::PacketType::CreateCharacterResponse, payload, metrics);
                } else if (event.packetType == network::PacketType::EnterWorldRequest) {
                    network::EnterWorldRequestPacket request;
                    if (!network::decodeEnterWorldRequest(event.payload, request))
                        throw std::runtime_error("malformed enter-world request");
                    const auto payload = network::encodeEnterWorldResponse(
                        {network::AuthResult::Success, {request.characterId, "Bot", "Vanguard", 1, 0}});
                    (void)sendServerPacket(transport, event.clientId,
                        network::PacketType::EnterWorldResponse, payload, metrics);
                } else if (event.packetType == network::PacketType::ClientInput) {
                    network::ClientInputPacket packet;
                    if (!network::decodeClientInput(event.payload, packet))
                        throw std::runtime_error("malformed client input");
                    simulation.setPlayerInput(event.clientId, decodeInput(packet),
                        packet.cameraYawDegrees, packet.inputSequence, packet.targetTick);
                    ++metrics.inputPacketsProcessed;
                }
            }
        }

        if (tick == 0) return;
        for (auto& bot : bots) {
            if (bot->state != BotState::SimulatingLoop) continue;
            const auto behavior = bot->nextRandom();
            const auto phase = behavior % 4;
            const auto direction = phase == 0 ? input::Control::KeyW : phase == 1 ?
                input::Control::KeyA : phase == 2 ? input::Control::KeyS : input::Control::KeyD;
            const std::uint32_t move = 1U << static_cast<unsigned>(direction);
            const std::uint32_t attack = behavior % 12 == 0 ?
                1U << static_cast<unsigned>(input::Control::MouseLeft) : 0U;
            network::ClientInputPacket inputPacket{attack, move | attack,
                static_cast<float>(behavior % 360),
                ++bot->inputSequence, simulation.clock().tickCount()};
            (void)bot->send(network::PacketType::ClientInput,
                network::encodeClientInput(inputPacket), metrics, true);
        }
        for (auto& bot : bots) accountBotEvents(*bot, bot->client->poll(), metrics);
        for (const auto& event : transport.poll()) {
            if (event.type != network::EventType::PacketReceived ||
                event.packetType != network::PacketType::ClientInput) continue;
            ++metrics.packetsReceived;
            metrics.bytesTransferred += event.payload.size() + 1;
            network::ClientInputPacket packet;
            if (!network::decodeClientInput(event.payload, packet))
                throw std::runtime_error("malformed client input");
            simulation.setPlayerInput(event.clientId, decodeInput(packet),
                packet.cameraYawDegrees, packet.inputSequence, packet.targetTick);
            ++metrics.inputPacketsProcessed;
        }
    };

    const auto connectDeadline = Clock::now() + std::chrono::seconds(5);
    while (Clock::now() < connectDeadline) {
        service(0);
        const bool allInWorld = std::all_of(bots.begin(), bots.end(), [](const auto& bot) {
            return bot->state == BotState::SimulatingLoop;
        });
        if (allInWorld) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!std::all_of(bots.begin(), bots.end(), [](const auto& bot) {
            return bot->state == BotState::SimulatingLoop;
        })) throw std::runtime_error("timed out while connecting and authenticating the bot swarm");
    metrics.connectedBots = bots.size();

    double totalTickMs = 0.0;
    for (std::uint64_t tick = 0; tick < config_.tickCount; ++tick) {
        service(static_cast<std::uint32_t>(tick + 1));
        const auto start = Clock::now();
        simulation.tick();
        const double durationMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        totalTickMs += durationMs;
        metrics.maxTickDurationMs = std::max(metrics.maxTickDurationMs, durationMs);
        ++metrics.ticksCompleted;

        const auto encoded = network::encodeServerSnapshot(makePacket(simulation.multiplayerSnapshot()));
        for (const auto clientId : clientIds) {
            if (transport.send(clientId, network::PacketType::ServerSnapshot, encoded, false)) {
                ++metrics.packetsSent;
                metrics.bytesTransferred += encoded.size() + 1;
            }
        }
        (void)transport.poll();
        for (auto& bot : bots) accountBotEvents(*bot, bot->client->poll(), metrics);
    }
    metrics.avgTickDurationMs = totalTickMs / static_cast<double>(metrics.ticksCompleted);

    for (auto& bot : bots) {
        bot->state = BotState::Disconnecting;
        bot->client->disconnect();
    }
    const auto disconnectDeadline = Clock::now() + std::chrono::seconds(3);
    while (simulation.playerCount() != 0 && Clock::now() < disconnectDeadline) {
        for (auto& bot : bots) (void)bot->client->poll();
        for (const auto& event : transport.poll())
            if (event.type == network::EventType::Disconnected)
                simulation.disconnectPlayer(event.clientId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    metrics.remainingServerPlayers = simulation.playerCount();
    for (auto& bot : bots) bot->state = BotState::Disconnected;
    if (metrics.remainingServerPlayers != 0)
        throw std::runtime_error("server did not clean up all bot sessions after disconnect");
    return metrics;
}

void printLoadTestSummary(const LoadTestConfig& config, const LoadTestMetrics& metrics,
    std::ostream& output) {
    output << "Server Load Test Summary\n"
        << "Bots: " << metrics.connectedBots << '/' << config.botCount << " connected ("
        << metrics.connectedBotsRate() * 100.0 << "%)\n"
        << "Ticks: " << metrics.ticksCompleted << '/' << config.tickCount << " (60 Hz simulated)\n"
        << "Tick duration: avg=" << metrics.avgTickDurationMs << " ms, max="
        << metrics.maxTickDurationMs << " ms (target avg < 16.6 ms)\n"
        << "Network application traffic: sent=" << metrics.packetsSent << " packets, received="
        << metrics.packetsReceived << " packets, bytes=" << metrics.bytesTransferred
        << " (both directions; excludes ENet framing and connection handshake)\n"
        << "Bot traffic: processed inputs=" << metrics.inputPacketsProcessed
        << ", snapshots received=" << metrics.snapshotsReceived << '\n'
        << "Cleanup: remaining server players=" << metrics.remainingServerPlayers << '\n'
        << "Auth boundary: deterministic in-process login/character/world responses; real ENet and server simulation\n";
}

} // namespace glory::server

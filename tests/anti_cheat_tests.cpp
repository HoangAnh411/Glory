#include "glory/server/server_anti_cheat.hpp"
#include "glory/server/server_simulation.hpp"
#include "glory/network/network.hpp"

#include <glm/vec3.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <thread>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    try {
        using glory::server::AntiCheatViolation;
        using glory::server::ServerAntiCheat;

        ServerAntiCheat speedCheck;
        require(!speedCheck.validateMovement(glm::vec3(0.0F), {0.2F, 0.0F, 0.0F}, 1.0F / 60.0F, 5.5F),
            "movement above the server speed limit must be rejected for snapback");
        require(speedCheck.telemetry().speedViolations == 1,
            "speed violation counter was not incremented");

        ServerAntiCheat verticalSpeedCheck;
        require(!verticalSpeedCheck.validateMovement(glm::vec3(0.0F), {0.0F, 1.0F, 0.0F},
            1.0F / 60.0F, 5.5F), "abnormal vertical displacement must be rejected as a speed hack");
        require(verticalSpeedCheck.telemetry().speedViolations == 1,
            "vertical speed violation counter was not incremented");

        ServerAntiCheat teleportCheck;
        require(!teleportCheck.validateMovement(glm::vec3(0.0F), {50.0F, 0.0F, 0.0F}, 1.0F / 60.0F, 5.5F),
            "large one-tick movement must be rejected for snapback");
        require(teleportCheck.telemetry().teleportViolations == 1,
            "teleport violation counter was not incremented");

        ServerAntiCheat collisionCheck;
        require(!collisionCheck.validateMovement({19.0F, 0.0F, 0.0F}, {21.0F, 0.0F, 0.0F},
            1.0F, 5.5F), "movement claim through a CollisionWorld obstacle must be rejected");
        require(collisionCheck.telemetry().collisionViolations == 1,
            "collision violation counter was not incremented");

        ServerAntiCheat cooldownCheck;
        require(!cooldownCheck.validateCooldown(0.5F), "skill with remaining cooldown must be rejected");
        require(cooldownCheck.telemetry().cooldownViolations == 1,
            "cooldown violation counter was not incremented");
        require(cooldownCheck.validateCooldown(0.0F), "ready skill should be accepted");

        ServerAntiCheat kickCheck;
        for (std::uint32_t index = 0; index < ServerAntiCheat::maximumViolationStreak; ++index)
            kickCheck.recordViolation(AntiCheatViolation::Speed);
        require(kickCheck.telemetry().kickRequested,
            "repeated violations must trigger the server kick policy");

        glory::network::NetworkContext networkContext;
        glory::network::NetworkServer networkServer(networkContext, 0);
        glory::network::NetworkClient networkClient(networkContext);
        require(networkClient.connect("127.0.0.1", networkServer.port()),
            "anti-cheat kick test client failed to connect");
        bool connected = false;
        std::uint32_t clientId = 0;
        const auto connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!connected && std::chrono::steady_clock::now() < connectDeadline) {
            for (const auto& event : networkServer.poll())
                if (event.type == glory::network::EventType::Connected) clientId = event.clientId;
            for (const auto& event : networkClient.poll())
                connected |= event.type == glory::network::EventType::Connected;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(connected && clientId != 0, "anti-cheat kick test handshake failed");

        const std::filesystem::path dataPath = std::filesystem::path(GLORY_ASSET_DIR) / "data";
        glory::server::ServerSimulation simulation(dataPath / "world_spawns.json", dataPath / "skills.json");
        require(simulation.connectPlayer(clientId), "anti-cheat test session failed to connect");
        const auto initial = simulation.playerState(clientId);
        require(initial.has_value(), "initial authoritative player state is missing");
        simulation.reportClientPositionClaim(clientId, initial->position + glm::vec3(0.5F, 0.0F, 0.0F));
        auto afterSpeedClaim = simulation.playerState(clientId);
        require(afterSpeedClaim && afterSpeedClaim->position == initial->position &&
            simulation.antiCheatState(clientId)->speedViolations == 1,
            "server must reject a speed claim and preserve the authoritative position");
        simulation.reportClientPositionClaim(clientId, initial->position + glm::vec3(50.0F, 0.0F, 0.0F));
        require(simulation.playerState(clientId)->position == initial->position &&
            simulation.antiCheatState(clientId)->teleportViolations == 1,
            "server must snap a teleport claim back to the authoritative position");

        require(simulation.requestPlayerSkill(clientId, "heavy_strike"),
            "first skill request should be accepted");
        simulation.tick();
        const float hpBeforeRejectedSkill = simulation.boss().currentHp();
        require(!simulation.requestPlayerSkill(clientId, "heavy_strike"),
            "skill request during cooldown must be rejected");
        require(simulation.boss().currentHp() == hpBeforeRejectedSkill &&
            simulation.antiCheatState(clientId)->cooldownViolations == 1,
            "cooldown rejection must not deal damage");
        for (std::uint32_t index = 1; index < ServerAntiCheat::maximumViolationStreak; ++index)
            (void)simulation.requestPlayerSkill(clientId, "heavy_strike");
        const auto kickTargets = simulation.clientsToKick();
        require(std::find(kickTargets.begin(), kickTargets.end(), clientId) != kickTargets.end(),
            "server session must become eligible for kick after repeated violations");
        const glory::network::AntiCheatWarningPacket kickWarning{
            glory::network::AntiCheatViolationCode::Kick, 1, 1, 0, 8, 30.0F, 6.325F, true};
        const auto kickWarningBytes = glory::network::encodeAntiCheatWarning(kickWarning);
        require(networkServer.send(clientId, glory::network::PacketType::AntiCheatWarning,
            kickWarningBytes, true), "server failed to queue the kick warning");
        networkServer.disconnect(clientId);
        bool disconnected = false;
        bool receivedKickWarning = false;
        const auto disconnectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!disconnected && std::chrono::steady_clock::now() < disconnectDeadline) {
            (void)networkServer.poll();
            for (const auto& event : networkClient.poll()) {
                receivedKickWarning |= event.type == glory::network::EventType::PacketReceived &&
                    event.packetType == glory::network::PacketType::AntiCheatWarning;
                disconnected |= event.type == glory::network::EventType::Disconnected;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(receivedKickWarning && disconnected,
            "kick warning must reach the client before the kick closes its connection");

        using namespace glory::network;
        const AntiCheatWarningPacket warning{AntiCheatViolationCode::Speed, 3, 2, 1, 4,
            12.0F, 6.325F, false};
        AntiCheatWarningPacket decodedWarning;
        const auto encodedWarning = encodeAntiCheatWarning(warning);
        require(decodeAntiCheatWarning(encodedWarning, decodedWarning) &&
            decodedWarning.speedViolations == warning.speedViolations &&
            decodedWarning.maximumSpeed == warning.maximumSpeed,
            "anti-cheat warning packet must round-trip telemetry");

        std::cout << "Anti-cheat server movement, cooldown, kick, and protocol checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "glory/network/network.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

int main() {
    using namespace glory::network;
    try {
        ClientInputPacket decodedInput;
        const auto encodedInput = encodeClientInput({3, 7, 123.5F, 0x12345678U,
            0x0102030405060708ULL});
        if (!decodeClientInput(encodedInput, decodedInput) || decodedInput.pressedButtons != 3 ||
            decodedInput.heldButtons != 7 || decodedInput.inputSequence != 0x12345678U ||
            decodedInput.targetTick != 0x0102030405060708ULL ||
            std::abs(decodedInput.cameraYawDegrees - 123.5F) > 0.001F) {
            std::cerr << "Client input wire encoding failed\n";
            return 1;
        }
        const AntiCheatWarningPacket warning{AntiCheatViolationCode::Cooldown, 2, 3, 4, 5,
            7.5F, 6.325F, false};
        AntiCheatWarningPacket decodedWarning;
        const auto warningBytes = encodeAntiCheatWarning(warning);
        if (!decodeAntiCheatWarning(warningBytes, decodedWarning) ||
            decodedWarning.cooldownViolations != warning.cooldownViolations ||
            decodedWarning.currentSpeed != warning.currentSpeed) {
            std::cerr << "Anti-cheat warning wire encoding failed\n";
            return 1;
        }
        const auto probeBytes = encodeAntiCheatProbe({AntiCheatProbeKind::PositionClaim, 1.0F, 2.0F, 3.0F});
        AntiCheatProbePacket decodedProbe;
        if (!decodeAntiCheatProbe(probeBytes, decodedProbe) || decodedProbe.z != 3.0F) {
            std::cerr << "Anti-cheat probe wire encoding failed\n";
            return 1;
        }
        const auto skillBytes = encodeSkillRequest("heavy_strike");
        std::string decodedSkill;
        if (!decodeSkillRequest(skillBytes, decodedSkill) || decodedSkill != "heavy_strike") {
            std::cerr << "Skill request wire encoding failed\n";
            return 1;
        }
        ServerSnapshotPacket snapshot;
        snapshot.serverTick = 987;
        snapshot.players.push_back({4, {1.0F, 2.0F, 3.0F}, {4.0F, 5.0F, 6.0F}, 45.0F,
            75.0F, 100.0F, "Run", true, false, "slash", 0x12345678U, false, 0.2F, 0.4F,
            {1.0F, 0.0F, 0.0F}, {3.0F, 0.0F, 0.0F}, 0.25F});
        snapshot.boss = {{-2.0F, 0.0F, 3.0F}, 200.0F, 300.0F, "Phase1"};
        const auto snapshotBytes = encodeServerSnapshot(snapshot);
        ServerSnapshotPacket decodedSnapshot;
        if (!decodeServerSnapshot(snapshotBytes, decodedSnapshot) || decodedSnapshot.serverTick != 987 ||
            decodedSnapshot.players.size() != 1 || decodedSnapshot.players[0].clientId != 4 ||
            decodedSnapshot.players[0].currentHp != 75.0F || !decodedSnapshot.players[0].isDodging ||
            decodedSnapshot.players[0].activeSkillId != "slash" ||
            decodedSnapshot.players[0].lastProcessedInputSequence != 0x12345678U ||
            decodedSnapshot.players[0].isGrounded || decodedSnapshot.players[0].dodgeElapsed != 0.2F ||
            decodedSnapshot.players[0].dodgeDirection.x != 1.0F ||
            decodedSnapshot.players[0].externalKnockbackVelocity.x != 3.0F ||
            decodedSnapshot.players[0].knockbackRemaining != 0.25F ||
            decodedSnapshot.boss.phase != "Phase1") {
            std::cerr << "Snapshot round-trip failed\n";
            return 1;
        }
        auto malformedSnapshot = snapshotBytes;
        malformedSnapshot.push_back(0);
        if (decodeServerSnapshot(malformedSnapshot, decodedSnapshot)) {
            std::cerr << "Snapshot decoder accepted trailing bytes\n";
            return 1;
        }
        NetworkContext context;
        NetworkServer server(context, 0);
        NetworkClient client(context);
        if (server.port() == 0 || !client.connect("127.0.0.1", server.port())) return 1;

        bool connected = false;
        std::uint32_t clientId = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!connected && std::chrono::steady_clock::now() < deadline) {
            server.poll();
            for (const auto& event : client.poll()) {
                if (event.type == EventType::Connected) {
                    connected = true;
                    clientId = event.clientId;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (!connected || clientId == 0 || client.state() != ConnectionState::Connected) {
            std::cerr << "Handshake failed\n";
            return 1;
        }

        constexpr std::array<std::uint8_t, 3> reliableData{7, 8, 9};
        constexpr std::array<std::uint8_t, 2> unreliableData{4, 5};
        client.send(PacketType::ClientInput, reliableData, true);
        client.send(PacketType::ClientInput, unreliableData, false);
        client.sendPing();
        bool gotReliable = false;
        bool gotUnreliable = false;
        bool gotPong = false;
        bool gotResponse = false;
        const std::array<std::uint8_t, 1> response{42};
        const auto trafficDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while ((!gotReliable || !gotUnreliable || !gotPong || !gotResponse) &&
            std::chrono::steady_clock::now() < trafficDeadline) {
            for (const auto& event : server.poll()) {
                if (event.type != EventType::PacketReceived) continue;
                if (event.packetType == PacketType::ClientInput && event.payload.size() == reliableData.size() &&
                    event.payload[0] == reliableData[0]) gotReliable = true;
                if (event.packetType == PacketType::ClientInput && event.payload.size() == unreliableData.size() &&
                    event.payload[0] == unreliableData[0]) gotUnreliable = true;
                if (event.packetType == PacketType::ClientInput)
                    server.send(event.clientId, PacketType::ServerSnapshot, response, true);
            }
            for (const auto& event : client.poll()) {
                gotPong |= event.type == EventType::PacketReceived && event.packetType == PacketType::Pong;
                gotResponse |= event.type == EventType::PacketReceived &&
                    event.packetType == PacketType::ServerSnapshot && event.payload ==
                        std::vector<std::uint8_t>(response.begin(), response.end());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (!gotReliable || !gotUnreliable || !gotPong || !gotResponse) {
            std::cerr << "Packet exchange failed: reliable=" << gotReliable << " unreliable=" << gotUnreliable
                << " pong=" << gotPong << " response=" << gotResponse << '\n';
            return 1;
        }
        client.disconnect();
        bool disconnected = false;
        const auto disconnectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!disconnected && std::chrono::steady_clock::now() < disconnectDeadline) {
            client.poll();
            for (const auto& event : server.poll()) disconnected |= event.type == EventType::Disconnected;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (!disconnected) {
            std::cerr << "Disconnect event was not observed\n";
            return 1;
        }
        std::cout << "ENet loopback passed: handshake, reliable/unreliable, ping/pong, response, disconnect\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

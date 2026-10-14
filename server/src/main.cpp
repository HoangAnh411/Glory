#include "glory/server/server_simulation.hpp"
#include "glory/server/account_database.hpp"
#include "glory/server/database_backup.hpp"
#include "glory/server/structured_logger.hpp"
#include "glory/network/network.hpp"
#include "glory/game/inventory.hpp"
#include "glory/game/classes.hpp"
#include "glory/game/spatial_grid.hpp"
#include "glory/game/social.hpp"
#include "glory/game/dungeon.hpp"

#include <glm/geometric.hpp>
#include <charconv>
#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <csignal>
#include <filesystem>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#ifndef GLORY_ASSET_DIR
#define GLORY_ASSET_DIR "assets"
#endif

namespace {

volatile std::sig_atomic_t running = 1;

void handleSignal(int) {
    running = 0;
}

bool parseTickCount(std::string_view value, std::uint64_t& tickCount) {
    if (value.empty()) return false;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), tickCount);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size();
}

bool parsePort(std::string_view value, std::uint16_t& port) {
    unsigned parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed == 0 || parsed > 65535)
        return false;
    port = static_cast<std::uint16_t>(parsed);
    return true;
}

void printSnapshot(const glory::server::ServerSnapshot& state) {
    std::cout << "Server tick: " << state.tick << " (" << state.elapsedSeconds << " s simulated)\n"
        << "Player: position=" << state.playerPosition.x << ',' << state.playerPosition.y << ','
        << state.playerPosition.z << " hp=" << state.playerHealth << '\n'
        << "Boss: position=" << state.bossPosition.x << ',' << state.bossPosition.y << ','
        << state.bossPosition.z << " hp=" << state.bossHealth << " hits=" << state.bossHitsReceived
        << " encounter=" << (state.bossEncounterActive ? "active" : "inactive") << '\n'
        << "Active triggers: " << state.activeTriggers.size() << '\n';
}

glory::network::ServerSnapshotPacket makeNetworkSnapshot(const glory::server::MultiplayerSnapshot& state) {
    using glory::network::SnapshotVector3;
    glory::network::ServerSnapshotPacket packet;
    packet.serverTick = state.serverTick;
    packet.matchState = static_cast<glory::network::ArenaMatchState>(state.matchState);
    packet.matchMode = static_cast<glory::network::MatchmakingMode>(state.matchMode);
    packet.countdownRemaining = state.countdownRemaining;
    packet.winningTeam = static_cast<glory::network::ArenaTeam>(state.winningTeam);
    packet.players.reserve(state.players.size());
    for (const auto& player : state.players) {
        packet.players.push_back({player.clientId,
            SnapshotVector3{player.position.x, player.position.y, player.position.z},
            SnapshotVector3{player.velocity.x, player.velocity.y, player.velocity.z}, player.yaw,
            player.currentHp, player.maxHp, player.animationState, player.isDodging,
            player.isInvulnerable, player.activeSkillId, player.lastProcessedInputSequence,
            player.isGrounded, player.dodgeElapsed, player.dodgeCooldownRemaining,
            SnapshotVector3{player.dodgeDirection.x, player.dodgeDirection.y, player.dodgeDirection.z},
                SnapshotVector3{player.externalKnockbackVelocity.x, player.externalKnockbackVelocity.y,
                    player.externalKnockbackVelocity.z}, player.knockbackRemaining,
            static_cast<glory::network::ArenaTeam>(player.team)});
    }
    packet.boss = {{state.bossPosition.x, state.bossPosition.y, state.bossPosition.z},
        state.bossCurrentHp, state.bossMaxHp, state.bossPhase};
    return packet;
}

std::pair<glm::vec3, float> zoneSpawn(glory::game::ZoneId zone) {
    switch (zone) {
    case glory::game::ZoneId::Zone1Plains: return {{0.0F, 0.0F, 0.0F}, -90.0F};
    case glory::game::ZoneId::Zone2Arena: return {{256.0F, 0.0F, 256.0F}, -90.0F};
    case glory::game::ZoneId::Zone3Forest: return {{-256.0F, 0.0F, -256.0F}, -90.0F};
    }
    return {{0.0F, 0.0F, 0.0F}, -90.0F};
}

struct CharacterRuntime {
    std::int64_t characterId = 0;
    glory::game::Inventory inventory;
    glory::game::EquipmentManager equipment;
};

glory::network::InventorySyncPacket makeInventorySync(std::int64_t characterId,
    const glory::game::Inventory& inventory) {
    glory::network::InventorySyncPacket packet;
    packet.characterId = characterId;
    for (std::size_t index = 0; index < inventory.slots().size(); ++index) {
        const auto& slot = inventory.slots()[index];
        if (!slot.empty()) packet.items.push_back({static_cast<std::uint8_t>(index), slot.itemId, slot.quantity});
    }
    return packet;
}

glory::network::EquipmentSyncPacket makeEquipmentSync(std::int64_t characterId,
    const glory::game::EquipmentManager& equipment, const glory::game::ItemCatalog& catalog) {
    glory::network::EquipmentSyncPacket packet;
    packet.characterId = characterId;
    for (std::size_t index = 0; index < equipment.items().size(); ++index) {
        const auto& item = equipment.items()[index];
        if (item) packet.items.push_back({static_cast<glory::network::EquipmentSlotPacket>(index), *item});
    }
    const auto total = equipment.stats(catalog);
    packet.bonusAttack = total.attack - 10;
    packet.bonusDefense = total.defense - 5;
    packet.bonusMaxHp = total.maxHp - 100;
    return packet;
}

std::vector<glory::server::PersistedInventorySlot> persistInventory(const glory::game::Inventory& inventory) {
    std::vector<glory::server::PersistedInventorySlot> records;
    for (std::size_t index = 0; index < inventory.slots().size(); ++index) {
        const auto& slot = inventory.slots()[index];
        if (!slot.empty()) records.push_back({static_cast<int>(index), slot.itemId, slot.quantity});
    }
    return records;
}

std::vector<glory::server::PersistedEquipmentItem> persistEquipment(const glory::game::EquipmentManager& equipment) {
    std::vector<glory::server::PersistedEquipmentItem> records;
    for (std::size_t index = 0; index < equipment.items().size(); ++index)
        if (equipment.items()[index]) records.push_back({static_cast<int>(index), *equipment.items()[index]});
    return records;
}

} // namespace

int main(int argc, char* argv[]) {
    bool smokeTest = false;
    bool finiteRun = false;
    std::uint64_t requestedTicks = 0;
    std::uint64_t backupIntervalSeconds = 0;
    std::uint16_t port = 7777;
    std::filesystem::path databasePath = "data/accounts.sqlite3";
    std::filesystem::path logPath = "data/logs/server.log";
    std::filesystem::path backupDirectory = "data/backups";
    std::filesystem::path feedbackExportPath;
    glory::server::LogLevel minimumLogLevel = glory::server::LogLevel::Info;
    bool backupNow = false;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--smoke-test") {
            smokeTest = true;
            finiteRun = true;
            requestedTicks = 120;
        } else if (argument == "--ticks" && index + 1 < argc) {
            if (!parseTickCount(argv[++index], requestedTicks)) {
                std::cerr << "Invalid tick count\n";
                return 2;
            }
            finiteRun = true;
        } else if (argument == "--port" && index + 1 < argc) {
            if (!parsePort(argv[++index], port)) {
                std::cerr << "Invalid UDP port\n";
                return 2;
            }
        } else if (argument == "--database" && index + 1 < argc) {
            databasePath = argv[++index];
            if (databasePath.empty()) {
                std::cerr << "Invalid database path\n";
                return 2;
            }
        } else if (argument == "--log-level" && index + 1 < argc) {
            std::string level(argv[++index]);
            std::transform(level.begin(), level.end(), level.begin(), [](unsigned char character) {
                return static_cast<char>(std::toupper(character));
            });
            if (!glory::server::parseLogLevel(level, minimumLogLevel)) {
                std::cerr << "Invalid log level (expected DEBUG, INFO, WARN, ERROR, or CRITICAL)\n";
                return 2;
            }
        } else if (argument == "--log-file" && index + 1 < argc) {
            logPath = argv[++index];
            if (logPath.empty()) {
                std::cerr << "Invalid log file path\n";
                return 2;
            }
        } else if (argument == "--backup-dir" && index + 1 < argc) {
            backupDirectory = argv[++index];
            if (backupDirectory.empty()) {
                std::cerr << "Invalid backup directory\n";
                return 2;
            }
        } else if (argument == "--backup-interval" && index + 1 < argc) {
            if (!parseTickCount(argv[++index], backupIntervalSeconds) || backupIntervalSeconds >
                static_cast<std::uint64_t>(std::numeric_limits<std::chrono::seconds::rep>::max())) {
                std::cerr << "Invalid backup interval\n";
                return 2;
            }
        } else if (argument == "--backup-now") {
            backupNow = true;
        } else if (argument == "--export-feedback" && index + 1 < argc) {
            feedbackExportPath = argv[++index];
            if (feedbackExportPath.empty()) {
                std::cerr << "Invalid feedback export path\n";
                return 2;
            }
        } else {
            std::cerr << "Usage: glory_server [--smoke-test] [--ticks N] [--port N] [--database PATH] "
                "[--log-level DEBUG|INFO|WARN|ERROR|CRITICAL] [--log-file PATH] "
                "[--backup-dir DIR] [--backup-interval SECONDS] [--backup-now] [--export-feedback PATH]\n";
            return 2;
        }
    }

    try {
        glory::server::StructuredLogger logger(logPath, minimumLogLevel);
        logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::System,
            "server process starting");
        if (!feedbackExportPath.empty()) {
            glory::server::AccountDatabase exportDatabase(databasePath.string());
            if (!exportDatabase.exportFeedbackToJson(feedbackExportPath)) {
                std::cerr << "Feedback export failed: " << feedbackExportPath.string() << '\n';
                return 1;
            }
            std::cout << "Feedback exported: " << feedbackExportPath.string() << '\n';
            return 0;
        }
        glory::server::DatabaseBackupManager backupManager(databasePath);
        if (backupNow) {
            const auto backupPath = backupManager.createTimestampedBackup(backupDirectory);
            std::string integrityResult;
            if (!backupManager.verifyBackupIntegrity(backupPath, &integrityResult)) {
                logger.log(glory::server::LogLevel::Error, glory::server::LogCategory::Database,
                    "backup integrity check failed: " + integrityResult);
                std::cerr << "Backup integrity check failed: " << integrityResult << '\n';
                return 1;
            }
            const auto removed = backupManager.rotateBackups(backupDirectory, 5);
            logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Database,
                "verified backup created: " + backupPath.string());
            std::cout << "Backup created and verified: " << backupPath.string()
                << " (integrity_check=" << integrityResult << ", removed=" << removed << ")\n";
            return 0;
        }

        const std::filesystem::path dataDirectory = std::filesystem::path(GLORY_ASSET_DIR) / "data";
        glory::server::ServerSimulation simulation(dataDirectory / "world_spawns.json",
            dataDirectory / "skills.json");
        const glory::game::ItemCatalog itemCatalog = glory::game::ItemCatalog::loadFromFile(dataDirectory / "items.json");
        const auto allClassSkills = glory::game::SkillRegistry::loadFromFile(dataDirectory / "skills.json");
        const auto classCatalog = glory::game::ClassCatalog::loadFromFile(dataDirectory / "classes.json", allClassSkills);
        glory::network::NetworkContext networkContext;
        glory::server::AccountDatabase accounts(databasePath.string());
        logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Database,
            "account database opened: " + databasePath.string());
        glory::network::NetworkServer networkServer(networkContext, port);
        std::cout << "ENet server listening on UDP port " << networkServer.port() << '\n';
        logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Network,
            "ENet server listening on UDP port " + std::to_string(networkServer.port()));
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        constexpr auto tickDuration = std::chrono::nanoseconds(16'666'667);
        auto nextTick = std::chrono::steady_clock::now();
        auto nextBackup = std::chrono::steady_clock::now() + std::chrono::seconds(backupIntervalSeconds);
        std::future<void> backupTask;
        std::uint64_t executedTicks = 0;
        std::map<std::uint32_t, std::uint32_t> previousButtons;
        std::map<std::uint32_t, std::int64_t> activeCharacters;
        std::unordered_map<std::uint32_t, std::string> activeSessionTokens;
        std::unordered_map<std::int64_t, glory::network::CharacterInfoPacket> onlineCharacterInfo;
        std::unordered_map<std::uint32_t, CharacterRuntime> characterRuntimes;
        std::unordered_map<std::uint32_t, glory::game::ZoneId> clientZones;
        glory::game::PartyManager parties;
        glory::game::DungeonManager dungeons;
        std::unordered_map<std::uint32_t, std::uint64_t> lastDungeonAttackTick;
        std::unordered_map<std::int64_t, glory::network::GuildSyncPacket> guildInvitations;
        glory::game::LootDropSystem lootSystem;
        std::unordered_map<std::uint64_t, glory::game::ZoneId> lootZones;
        float previousBossHealth = simulation.snapshot().bossHealth;
        const auto sendCharacterSync = [&](std::uint32_t clientId) {
            const auto found = characterRuntimes.find(clientId);
            if (found == characterRuntimes.end()) return;
            const auto inventoryBytes = glory::network::encodeInventorySync(
                makeInventorySync(found->second.characterId, found->second.inventory));
            const auto equipmentBytes = glory::network::encodeEquipmentSync(
                makeEquipmentSync(found->second.characterId, found->second.equipment, itemCatalog));
            networkServer.send(clientId, glory::network::PacketType::InventorySync, inventoryBytes, true);
            networkServer.send(clientId, glory::network::PacketType::EquipmentSync, equipmentBytes, true);
        };
        const auto sendWorldLootToClient = [&](std::uint32_t clientId, const glm::vec3& position,
            glory::game::ZoneId zone) {
            glory::network::WorldLootSyncPacket packet;
            for (const auto& drop : lootSystem.drops()) {
                const auto zoneIt = lootZones.find(drop.lootId);
                const auto dropZone = zoneIt == lootZones.end()
                    ? glory::game::ZoneId::Zone1Plains : zoneIt->second;
                const float dx = position.x - drop.position.x;
                const float dz = position.z - drop.position.z;
                if (dropZone != zone || dx * dx + dz * dz > 40.0F * 40.0F) continue;
                packet.drops.push_back({drop.lootId, drop.position.x, drop.position.y, drop.position.z,
                    drop.itemId, drop.quantity});
            }
            const auto bytes = glory::network::encodeWorldLootSync(packet);
            networkServer.send(clientId, glory::network::PacketType::WorldLootSync, bytes, true);
        };
        const auto sendLootToAllClients = [&]() {
            const auto state = simulation.multiplayerSnapshot();
            for (const auto& player : state.players) {
                const auto zoneIt = clientZones.find(player.clientId);
                const auto zone = zoneIt == clientZones.end()
                    ? glory::game::ZoneId::Zone1Plains : zoneIt->second;
                sendWorldLootToClient(player.clientId, player.position, zone);
            }
        };
        const auto saveCharacterRuntime = [&](const std::string& token, const CharacterRuntime& runtime) {
            return accounts.saveCharacterState(token, runtime.characterId,
                persistInventory(runtime.inventory), persistEquipment(runtime.equipment));
        };
        const auto sendActionResult = [&](std::uint32_t clientId, glory::network::InventoryActionResult result) {
            const auto bytes = glory::network::encodeInventoryActionResponse({result});
            networkServer.send(clientId, glory::network::PacketType::InventoryActionResponse, bytes, true);
        };
        const auto sendGoldSync = [&](std::uint32_t clientId, std::int64_t characterId) {
            const auto bytes = glory::network::encodeCharacterGoldSync(
                {characterId, accounts.characterGold(characterId)});
            networkServer.send(clientId, glory::network::PacketType::CharacterGoldSync, bytes, true);
        };
        const auto clientForCharacter = [&](std::int64_t characterId) -> std::uint32_t {
            for (const auto& [clientId, activeId] : activeCharacters)
                if (activeId == characterId) return clientId;
            return 0;
        };
        const auto characterByName = [&](const std::string& name) -> std::int64_t {
            for (const auto& [characterId, info] : onlineCharacterInfo)
                if (info.name == name) return characterId;
            return 0;
        };
        const auto partySyncFor = [&](std::int64_t characterId, glory::network::SocialResult result =
            glory::network::SocialResult::Success) {
            using namespace glory::network;
            PartySyncPacket packet;
            packet.result = result;
            packet.partyId = parties.partyFor(characterId);
            packet.invitationPending = parties.hasInvitation(characterId);
            for (const auto& member : parties.members(characterId))
                packet.members.push_back({member.characterId, member.name, member.className, member.level,
                    member.currentHp, member.maxHp, member.leader});
            const auto bytes = encodePartySync(packet);
            const auto recipient = clientForCharacter(characterId);
            if (recipient) networkServer.send(recipient, PacketType::PartySync, bytes, true);
        };
        const auto broadcastPartySync = [&](std::int64_t characterId) {
            for (const auto& member : parties.members(characterId)) partySyncFor(member.characterId);
        };
        const auto guildSyncFor = [&](std::int64_t characterId, glory::network::SocialResult result =
            glory::network::SocialResult::Success) {
            auto packet = accounts.guildForCharacter(characterId);
            const auto invitation = guildInvitations.find(characterId);
            if (invitation != guildInvitations.end() && packet.guildId == 0) {
                packet = invitation->second;
                packet.memberRank = glory::network::GuildRank::Member;
            }
            if (result != glory::network::SocialResult::Success) packet.result = result;
            packet.invitationPending = invitation != guildInvitations.end();
            const auto bytes = glory::network::encodeGuildSync(packet);
            const auto recipient = clientForCharacter(characterId);
            if (recipient) networkServer.send(recipient, glory::network::PacketType::GuildSync, bytes, true);
        };
        const auto sendDungeonSync = [&](std::uint32_t clientId, const glory::game::DungeonInstance& instance,
            glory::network::DungeonResult result = glory::network::DungeonResult::Success) {
            using namespace glory::network;
            DungeonStateSyncPacket packet;
            packet.result = result;
            packet.dungeonId = instance.definition.dungeonId;
            packet.instanceId = instance.instanceId;
            packet.timerRemaining = instance.timerRemaining;
            packet.isCompleted = instance.state == glory::game::DungeonState::Completed;
            packet.isFailed = instance.state == glory::game::DungeonState::Failed;
            packet.bossHealth = instance.bossHealth;
            packet.bossMaxHealth = instance.definition.bossMaxHp;
            networkServer.send(clientId, PacketType::DungeonStateSync, encodeDungeonStateSync(packet), true);
        };
        const auto returnDungeonMembers = [&](std::uint64_t instanceId, bool grantRewards) {
            const auto* current = dungeons.find(instanceId);
            if (!current) return;
            const auto members = current->members;
            bool spawnedRewardLoot = false;
            for (const auto& member : members) {
                const auto clientId = clientForCharacter(member.characterId);
                if (clientId == 0) continue;
                if (grantRewards) {
                    const auto runtime = characterRuntimes.find(clientId);
                    const auto token = activeSessionTokens.find(clientId);
                    if (runtime != characterRuntimes.end() && token != activeSessionTokens.end()) {
                        CharacterRuntime rewarded = runtime->second;
                        const bool itemAdded = rewarded.inventory.addItem(current->definition.rewardItemId, 1, itemCatalog);
                        const bool inventorySaved = itemAdded && saveCharacterRuntime(token->second, rewarded);
                        const bool expGranted = accounts.grantCharacterExperience(token->second, member.characterId,
                            current->definition.rewardExp);
                        if (inventorySaved) runtime->second = std::move(rewarded);
                        else {
                            const auto lootId = lootSystem.spawnLoot(current->definition.rewardItemId, 1,
                                member.originPosition, itemCatalog);
                            if (lootId) {
                                lootZones[lootId] = static_cast<glory::game::ZoneId>(member.originZone);
                                spawnedRewardLoot = true;
                            }
                        }
                        auto info = onlineCharacterInfo.find(member.characterId);
                        if (expGranted && info != onlineCharacterInfo.end())
                            info->second.exp += static_cast<std::int32_t>(current->definition.rewardExp);
                    }
                }
                const auto* stillCurrent = dungeons.find(instanceId);
                if (stillCurrent) sendDungeonSync(clientId, *stillCurrent);
                (void)simulation.teleportPlayer(clientId, member.originPosition);
                clientZones[clientId] = static_cast<glory::game::ZoneId>(member.originZone);
                (void)dungeons.removeMember(instanceId, member.characterId);
                if (grantRewards) sendCharacterSync(clientId);
            }
            (void)dungeons.closeInstance(instanceId);
            if (spawnedRewardLoot) sendLootToAllClients();
        };
        while (running != 0 && (!finiteRun || executedTicks < requestedTicks)) {
            if (backupIntervalSeconds > 0 && std::chrono::steady_clock::now() >= nextBackup) {
                if (backupTask.valid() && backupTask.wait_for(std::chrono::milliseconds(0)) ==
                    std::future_status::ready) {
                    try { backupTask.get(); }
                    catch (const std::exception& backupError) {
                        logger.log(glory::server::LogLevel::Error, glory::server::LogCategory::Database,
                            "periodic backup worker failed: " + std::string(backupError.what()));
                    }
                }
                if (!backupTask.valid()) {
                    backupTask = std::async(std::launch::async, [&] {
                        const auto backupStart = std::chrono::steady_clock::now();
                        try {
                            const auto backupPath = backupManager.createTimestampedBackup(backupDirectory);
                            std::string integrityResult;
                            if (!backupManager.verifyBackupIntegrity(backupPath, &integrityResult))
                                throw std::runtime_error("backup integrity check failed: " + integrityResult);
                            const auto removed = backupManager.rotateBackups(backupDirectory, 5);
                            const auto durationMs = std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - backupStart).count();
                            logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Database,
                                "periodic backup verified: " + backupPath.string() + ", duration_ms=" +
                                std::to_string(durationMs) + ", rotated=" + std::to_string(removed));
                        } catch (const std::exception& backupError) {
                            logger.log(glory::server::LogLevel::Error, glory::server::LogCategory::Database,
                                "periodic backup failed: " + std::string(backupError.what()));
                        }
                    });
                } else {
                    logger.log(glory::server::LogLevel::Warn, glory::server::LogCategory::Database,
                        "skipping backup interval because the previous backup is still running");
                }
                nextBackup = std::chrono::steady_clock::now() + std::chrono::seconds(backupIntervalSeconds);
            }
            for (const auto& event : networkServer.poll()) {
                if (event.type == glory::network::EventType::Connected) {
                    std::cout << "Client connected, peer id=" << event.clientId << '\n';
                    logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Network,
                        "client connected, peer_id=" + std::to_string(event.clientId));
                    clientZones[event.clientId] = glory::game::ZoneId::Zone1Plains;
                    if (!simulation.connectPlayer(event.clientId)) {
                        std::cerr << "Rejected client: server player capacity reached\n";
                        networkServer.disconnect(event.clientId);
                    }
                } else if (event.type == glory::network::EventType::Disconnected) {
                    std::cout << "Client disconnected, id=" << event.clientId << '\n';
                    logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::Network,
                        "client disconnected, peer_id=" + std::to_string(event.clientId));
                    std::int64_t disconnectedCharacterId = 0;
                    if (const auto active = activeCharacters.find(event.clientId); active != activeCharacters.end()) {
                        const auto characterId = active->second;
                        disconnectedCharacterId = characterId;
                        if (const auto* dungeon = dungeons.activeInstanceForCharacter(characterId)) {
                            const auto instanceId = dungeon->instanceId;
                            (void)dungeons.removeMember(instanceId, characterId);
                            if (const auto* remaining = dungeons.find(instanceId)) {
                                for (const auto& member : remaining->members) {
                                    const auto client = clientForCharacter(member.characterId);
                                    if (client && client != event.clientId) sendDungeonSync(client, *remaining);
                                }
                                if (remaining->state == glory::game::DungeonState::Closed)
                                    (void)dungeons.closeInstance(instanceId);
                            }
                        }
                        const auto partyId = parties.partyFor(characterId);
                        (void)parties.leave(characterId);
                        if (partyId) {
                            for (const auto& [memberId, info] : onlineCharacterInfo) {
                                (void)info;
                                if (parties.partyFor(memberId) == partyId) partySyncFor(memberId);
                            }
                        }
                        onlineCharacterInfo.erase(characterId);
                    }
                    if (disconnectedCharacterId > 0) parties.cancelInvitation(disconnectedCharacterId);
                    simulation.disconnectPlayer(event.clientId);
                    previousButtons.erase(event.clientId);
                    activeCharacters.erase(event.clientId);
                    activeSessionTokens.erase(event.clientId);
                    lastDungeonAttackTick.erase(event.clientId);
                    characterRuntimes.erase(event.clientId);
                    clientZones.erase(event.clientId);
                } else if (event.packetType == glory::network::PacketType::ClientInput) {
                    glory::network::ClientInputPacket packet;
                    if (glory::network::decodeClientInput(event.payload, packet)) {
                        glory::input::InputFrame inputFrame;
                        for (unsigned bit = 1; bit <= static_cast<unsigned>(glory::input::Control::MouseRight); ++bit) {
                            const auto control = static_cast<glory::input::Control>(bit);
                            if ((packet.pressedButtons & (1U << bit)) != 0) inputFrame.pressed.push_back(control);
                            if ((packet.heldButtons & (1U << bit)) != 0) inputFrame.held.push_back(control);
                            if ((previousButtons[event.clientId] & (1U << bit)) != 0 &&
                                (packet.heldButtons & (1U << bit)) == 0)
                                inputFrame.released.push_back(control);
                        }
                        previousButtons[event.clientId] = packet.heldButtons;
                        simulation.setPlayerInput(event.clientId, inputFrame, packet.cameraYawDegrees,
                            packet.inputSequence, packet.targetTick);
                        if ((packet.pressedButtons & (1U << static_cast<unsigned>(glory::input::Control::MouseLeft))) != 0 &&
                            (!lastDungeonAttackTick.contains(event.clientId) ||
                             executedTicks >= lastDungeonAttackTick[event.clientId] + 30U)) {
                            const auto character = activeCharacters.find(event.clientId);
                            if (character != activeCharacters.end()) {
                                const auto* dungeon = dungeons.activeInstanceForCharacter(character->second);
                                const auto player = simulation.playerState(event.clientId);
                                if (dungeon && player && glm::distance(player->position, dungeon->bossSpawn) <= 5.0F &&
                                    dungeons.applyBossDamage(dungeon->instanceId, character->second, 35.0F))
                                    lastDungeonAttackTick[event.clientId] = executedTicks;
                            }
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::AntiCheatProbe) {
                    glory::network::AntiCheatProbePacket probe;
                    if (glory::network::decodeAntiCheatProbe(event.payload, probe)) {
                        simulation.reportClientPositionClaim(event.clientId, {probe.x, probe.y, probe.z});
                    }
                } else if (event.packetType == glory::network::PacketType::SkillRequest) {
                    std::string skillId;
                    if (glory::network::decodeSkillRequest(event.payload, skillId) &&
                        !simulation.requestPlayerSkill(event.clientId, skillId))
                        std::cout << "Rejected unavailable or cooling skill '" << skillId
                            << "' for client " << event.clientId << '\n';
                } else if (event.packetType == glory::network::PacketType::FeedbackSubmitRequest) {
                    using glory::network::FeedbackSubmitStatus;
                    FeedbackSubmitStatus status = FeedbackSubmitStatus::InvalidLength;
                    glory::network::FeedbackSubmitRequestPacket feedback;
                    if (glory::network::decodeFeedbackSubmitRequest(event.payload, feedback)) {
                        const auto token = activeSessionTokens.find(event.clientId);
                        const auto character = activeCharacters.find(event.clientId);
                        if (token != activeSessionTokens.end() && character != activeCharacters.end()) {
                            const auto result = accounts.submitFeedback(token->second, character->second, feedback);
                            if (result == glory::server::FeedbackSubmissionResult::Success) {
                                status = FeedbackSubmitStatus::Success;
                                logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::System,
                                    "closed beta feedback stored");
                            } else if (result == glory::server::FeedbackSubmissionResult::RateLimited) {
                                status = FeedbackSubmitStatus::RateLimited;
                            }
                        }
                    }
                    const auto response = glory::network::encodeFeedbackSubmitResponse({status});
                    networkServer.send(event.clientId, glory::network::PacketType::FeedbackSubmitResponse,
                        response, true);
                } else if (event.packetType == glory::network::PacketType::MatchmakingRequest) {
                    glory::network::MatchmakingRequestPacket request;
                    if (glory::network::decodeMatchmakingRequest(event.payload, request)) {
                        const bool accepted = simulation.requestMatchmaking(event.clientId,
                            static_cast<glory::game::QueueMode>(request.mode),
                            static_cast<glory::game::QueueAction>(request.action));
                        std::cout << "Matchmaking request from client " << event.clientId << ": "
                            << (accepted ? "accepted" : "rejected") << '\n';
                    }
                } else if (event.packetType == glory::network::PacketType::RegisterRequest) {
                    glory::network::RegisterRequestPacket request;
                    if (glory::network::decodeRegisterRequest(event.payload, request)) {
                        const auto result = accounts.registerAccount(request.username, request.password);
                        logger.log(result == glory::network::AuthResult::Success
                                ? glory::server::LogLevel::Info : glory::server::LogLevel::Warn,
                            glory::server::LogCategory::Auth, "account registration completed");
                        const auto response = glory::network::encodeRegisterResponse({result});
                        networkServer.send(event.clientId, glory::network::PacketType::RegisterResponse, response, true);
                    }
                } else if (event.packetType == glory::network::PacketType::LoginRequest) {
                    glory::network::LoginRequestPacket request;
                    if (glory::network::decodeLoginRequest(event.payload, request)) {
                        const auto login = accounts.authenticateAccount(request.username, request.password);
                        logger.log(login.result == glory::network::AuthResult::Success
                                ? glory::server::LogLevel::Info : glory::server::LogLevel::Warn,
                            glory::server::LogCategory::Auth, "account login attempt completed");
                        const auto response = glory::network::encodeLoginResponse(login);
                        networkServer.send(event.clientId, glory::network::PacketType::LoginResponse, response, true);
                    }
                } else if (event.packetType == glory::network::PacketType::CharacterListRequest) {
                    glory::network::CharacterListRequestPacket request;
                    if (glory::network::decodeCharacterListRequest(event.payload, request)) {
                        const auto response = glory::network::encodeCharacterListResponse(
                            accounts.listCharacters(request.sessionToken));
                        networkServer.send(event.clientId, glory::network::PacketType::CharacterListResponse, response, true);
                    }
                } else if (event.packetType == glory::network::PacketType::CreateCharacterRequest) {
                    glory::network::CreateCharacterRequestPacket request;
                    if (glory::network::decodeCreateCharacterRequest(event.payload, request)) {
                        auto characterResponse = classCatalog.find(request.className)
                            ? accounts.createCharacter(request.sessionToken, request.name, request.className)
                            : glory::network::CreateCharacterResponsePacket{glory::network::AuthResult::InvalidClass, {}};
                        const auto response = glory::network::encodeCreateCharacterResponse(characterResponse);
                        networkServer.send(event.clientId, glory::network::PacketType::CreateCharacterResponse,
                            response, true);
                    }
                } else if (event.packetType == glory::network::PacketType::EnterWorldRequest) {
                    glory::network::EnterWorldRequestPacket request;
                    if (glory::network::decodeEnterWorldRequest(event.payload, request)) {
                        auto response = accounts.selectCharacter(request.sessionToken, request.characterId);
                        if (response.result == glory::network::AuthResult::Success) {
                            const auto* classDefinition = classCatalog.find(response.character.className);
                            if (!classDefinition) classDefinition = classCatalog.find("Warrior");
                            if (!classDefinition || !simulation.applyCharacterClass(event.clientId,
                                *classDefinition, classCatalog.skillsForClass(classDefinition->name, allClassSkills)))
                                response.result = glory::network::AuthResult::ServerError;
                        }
                        if (response.result == glory::network::AuthResult::Success) {
                            std::vector<glory::server::PersistedInventorySlot> inventoryRecords;
                            std::vector<glory::server::PersistedEquipmentItem> equipmentRecords;
                            CharacterRuntime runtime;
                            runtime.characterId = response.character.characterId;
                            std::vector<std::pair<std::size_t, glory::game::InventorySlot>> inventorySlots;
                            std::vector<glory::game::EquipmentItem> equipmentItems;
                            const bool loaded = accounts.loadCharacterState(runtime.characterId,
                                inventoryRecords, equipmentRecords);
                            for (const auto& item : inventoryRecords) {
                                if (item.slotIndex < 0 || item.slotIndex >= 24) continue;
                                inventorySlots.push_back({static_cast<std::size_t>(item.slotIndex),
                                    {item.itemId, item.quantity}});
                            }
                            for (const auto& item : equipmentRecords) {
                                if (item.slotType < 0 || item.slotType >= 3) continue;
                                equipmentItems.push_back({static_cast<glory::game::EquipmentSlot>(item.slotType),
                                    item.itemId});
                            }
                            if (!loaded || !runtime.inventory.restore(inventorySlots, itemCatalog) ||
                                !runtime.equipment.restore(equipmentItems, itemCatalog)) {
                                response.result = glory::network::AuthResult::ServerError;
                            } else {
                                activeCharacters[event.clientId] = response.character.characterId;
                                onlineCharacterInfo[response.character.characterId] = response.character;
                                characterRuntimes[event.clientId] = std::move(runtime);
                                std::cout << "Client " << event.clientId << " entered world as "
                                    << response.character.name << '\n';
                            }
                        }
                        const auto payload = glory::network::encodeEnterWorldResponse(response);
                        networkServer.send(event.clientId, glory::network::PacketType::EnterWorldResponse,
                            payload, true);
                        if (response.result == glory::network::AuthResult::Success) {
                            sendCharacterSync(event.clientId);
                            sendGoldSync(event.clientId, response.character.characterId);
                            activeSessionTokens[event.clientId] = request.sessionToken;
                            partySyncFor(response.character.characterId);
                            guildSyncFor(response.character.characterId);
                            const auto player = simulation.playerState(event.clientId);
                            if (player) sendWorldLootToClient(event.clientId, player->position,
                                clientZones[event.clientId]);
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::EnterDungeonRequest) {
                    using namespace glory::network;
                    EnterDungeonRequestPacket request;
                    if (!decodeEnterDungeonRequest(event.payload, request)) continue;
                    EnterDungeonResponsePacket response;
                    response.dungeonId = request.dungeonId;
                    if (!accounts.ownsCharacter(request.sessionToken, request.characterId)) {
                        response.result = DungeonResult::InvalidSession;
                    } else if (!activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId) {
                        response.result = DungeonResult::CharacterNotSelected;
                    } else if (dungeons.activeInstanceForCharacter(request.characterId)) {
                        response.result = DungeonResult::AlreadyInDungeon;
                    } else {
                        std::vector<std::uint32_t> partyClients{event.clientId};
                        if (request.enterParty) {
                            const auto party = parties.members(request.characterId);
                            if (party.empty()) response.result = DungeonResult::NotInParty;
                            else {
                                partyClients.clear();
                                const auto originZone = clientZones[event.clientId];
                                for (const auto& member : party) {
                                    const auto client = clientForCharacter(member.characterId);
                                    if (!client || !activeCharacters.contains(client) ||
                                        clientZones[client] != originZone || dungeons.activeInstanceForCharacter(member.characterId)) {
                                        response.result = DungeonResult::PartyUnavailable;
                                        break;
                                    }
                                    partyClients.push_back(client);
                                }
                            }
                        }
                        if (response.result == DungeonResult::ServerError) response.result = DungeonResult::Success;
                        if (response.result == DungeonResult::Success) {
                            std::vector<glory::game::DungeonMember> members;
                            for (const auto client : partyClients) {
                                const auto characterId = activeCharacters.at(client);
                                const auto player = simulation.playerState(client);
                                if (!player) { response.result = DungeonResult::PartyUnavailable; break; }
                                members.push_back({characterId, static_cast<std::uint8_t>(clientZones[client]),
                                    player->position, player->currentHp, player->maxHp});
                            }
                            if (response.result == DungeonResult::Success) {
                                const auto instanceId = dungeons.createInstance(request.dungeonId, members);
                                if (!instanceId) response.result = DungeonResult::UnknownDungeon;
                                else {
                                    response.instanceId = *instanceId;
                                    const auto* instance = dungeons.find(*instanceId);
                                    for (std::size_t index = 0; index < partyClients.size(); ++index) {
                                        const auto client = partyClients[index];
                                        const glm::vec3 spawn = instance->playerSpawn + glm::vec3(
                                            static_cast<float>(index) * 1.5F, 0.0F, 0.0F);
                                        (void)simulation.teleportPlayer(client, spawn);
                                        networkServer.send(client, PacketType::EnterDungeonResponse,
                                            encodeEnterDungeonResponse({DungeonResult::Success,
                                                request.dungeonId, *instanceId}), true);
                                        sendDungeonSync(client, *instance);
                                    }
                                    continue;
                                }
                            }
                        }
                    }
                    networkServer.send(event.clientId, PacketType::EnterDungeonResponse,
                        encodeEnterDungeonResponse(response), true);
                } else if (event.packetType == glory::network::PacketType::LeaveDungeonRequest) {
                    using namespace glory::network;
                    LeaveDungeonRequestPacket request;
                    if (!decodeLeaveDungeonRequest(event.payload, request) ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        !activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId) continue;
                    const auto* instance = dungeons.activeInstanceForCharacter(request.characterId);
                    if (!instance) continue;
                    const auto instanceId = instance->instanceId;
                    const auto removed = dungeons.removeMember(instanceId, request.characterId);
                    if (!removed) continue;
                    (void)simulation.teleportPlayer(event.clientId, removed->originPosition);
                    clientZones[event.clientId] = static_cast<glory::game::ZoneId>(removed->originZone);
                    DungeonStateSyncPacket leftState;
                    leftState.dungeonId = instance->definition.dungeonId;
                    networkServer.send(event.clientId, PacketType::DungeonStateSync,
                        encodeDungeonStateSync(leftState), true);
                    if (const auto* remaining = dungeons.find(instanceId)) {
                        for (const auto& member : remaining->members) {
                            const auto client = clientForCharacter(member.characterId);
                            if (client) sendDungeonSync(client, *remaining);
                        }
                        if (remaining->state == glory::game::DungeonState::Closed)
                            (void)dungeons.closeInstance(instanceId);
                    }
                } else if (event.packetType == glory::network::PacketType::ChatMessage) {
                    using namespace glory::network;
                    ChatMessagePacket request;
                    if (!decodeChatMessage(event.payload, request) || request.channel == ChatChannel::System ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        !activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId ||
                        !glory::game::isValidChatText(request.text)) continue;
                    auto sender = onlineCharacterInfo.find(request.characterId);
                    if (sender == onlineCharacterInfo.end()) continue;
                    std::int64_t targetId = request.channel == ChatChannel::Whisper
                        ? characterByName(request.targetName) : 0;
                    if (request.channel == ChatChannel::Whisper && targetId == 0) continue;
                    std::vector<glory::game::ChatIdentity> identities;
                    identities.reserve(activeCharacters.size());
                    for (const auto& [clientId, characterId] : activeCharacters) {
                        (void)clientId;
                        const auto info = onlineCharacterInfo.find(characterId);
                        if (info == onlineCharacterInfo.end()) continue;
                        const auto guild = accounts.guildForCharacter(characterId);
                        identities.push_back({characterId, info->second.name,
                            static_cast<std::uint8_t>(clientZones.contains(clientForCharacter(characterId))
                                ? clientZones.at(clientForCharacter(characterId)) : glory::game::ZoneId::Zone1Plains),
                            parties.partyFor(characterId), guild.guildId, true});
                    }
                    const auto recipients = glory::game::routeChat(
                        static_cast<glory::game::ChatChannel>(request.channel), request.characterId, targetId, identities);
                    const ChatMessagePacket outgoing{{}, request.characterId, request.channel,
                        sender->second.name, request.channel == ChatChannel::Whisper ? request.targetName : std::string{},
                        request.text};
                    const auto bytes = encodeChatMessage(outgoing);
                    for (const auto recipientId : recipients) {
                        const auto recipient = clientForCharacter(recipientId);
                        if (recipient) networkServer.send(recipient, PacketType::ChatMessage, bytes, true);
                    }
                } else if (event.packetType == glory::network::PacketType::PartyRequest) {
                    using namespace glory::network;
                    PartyRequestPacket request;
                    if (!decodePartyRequest(event.payload, request) ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        !activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId) continue;
                    const auto characterId = request.characterId;
                    SocialResult result = SocialResult::Success;
                    switch (request.action) {
                    case PartyAction::Create: {
                        const auto info = onlineCharacterInfo.find(characterId);
                        const auto player = simulation.playerState(event.clientId);
                        if (info == onlineCharacterInfo.end()) { result = SocialResult::CharacterNotSelected; break; }
                        glory::game::PartyMember leader{characterId, info->second.name, info->second.className,
                            info->second.level, player ? player->currentHp : 100.0F, 100.0F, true};
                        if (!parties.createParty(leader)) result = SocialResult::AlreadyMember;
                        break;
                    }
                    case PartyAction::Invite: {
                        const auto target = characterByName(request.targetName);
                        if (!target) { result = SocialResult::InvalidTarget; break; }
                        if (!parties.invite(characterId, target)) result = SocialResult::NotLeader;
                        else partySyncFor(target);
                        break;
                    }
                    case PartyAction::Accept: {
                        const auto info = onlineCharacterInfo.find(characterId);
                        const auto player = simulation.playerState(event.clientId);
                        if (info == onlineCharacterInfo.end() || !parties.accept(characterId,
                            {characterId, info->second.name, info->second.className, info->second.level,
                                player ? player->currentHp : 100.0F, 100.0F, false})) result = SocialResult::NoInvitation;
                        break;
                    }
                    case PartyAction::Leave:
                        if (!parties.leave(characterId)) result = SocialResult::NotFound;
                        break;
                    case PartyAction::Kick: {
                        std::int64_t target = 0;
                        const auto parsed = std::from_chars(request.targetName.data(),
                            request.targetName.data() + request.targetName.size(), target);
                        if (parsed.ec != std::errc{} || parsed.ptr != request.targetName.data() + request.targetName.size())
                            target = characterByName(request.targetName);
                        if (!parties.kick(characterId, target)) result = SocialResult::NotLeader;
                        else partySyncFor(target);
                        break;
                    }
                    case PartyAction::Refresh: break;
                    }
                    partySyncFor(characterId, result);
                    broadcastPartySync(characterId);
                } else if (event.packetType == glory::network::PacketType::GuildRequest) {
                    using namespace glory::network;
                    GuildRequestPacket request;
                    if (!decodeGuildRequest(event.payload, request) ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        !activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId) continue;
                    const auto characterId = request.characterId;
                    const auto previousGuild = accounts.guildForCharacter(characterId);
                    SocialResult result = SocialResult::Success;
                    switch (request.action) {
                    case GuildAction::Create:
                        result = accounts.createGuild(request.sessionToken, characterId, request.guildName);
                        break;
                    case GuildAction::Invite: {
                        const auto target = characterByName(request.targetName);
                        result = target ? accounts.inviteGuildMember(request.sessionToken, characterId, target)
                                        : SocialResult::InvalidTarget;
                        if (result == SocialResult::Success) {
                            const auto guild = accounts.guildForCharacter(characterId);
                            guildInvitations[target] = guild;
                            guildSyncFor(target);
                        }
                        break;
                    }
                    case GuildAction::AcceptInvite: {
                        const auto invitation = guildInvitations.find(characterId);
                        if (invitation == guildInvitations.end()) result = SocialResult::NotFound;
                        else {
                            result = accounts.acceptGuildInvite(request.sessionToken, characterId,
                                static_cast<std::int64_t>(invitation->second.guildId));
                            if (result == SocialResult::Success) guildInvitations.erase(invitation);
                        }
                        break;
                    }
                    case GuildAction::Leave:
                        result = accounts.leaveGuild(request.sessionToken, characterId);
                        break;
                    case GuildAction::SetRank: {
                        std::int64_t target = 0;
                        const auto parsed = std::from_chars(request.targetName.data(),
                            request.targetName.data() + request.targetName.size(), target);
                        if (parsed.ec != std::errc{} || parsed.ptr != request.targetName.data() + request.targetName.size())
                            target = characterByName(request.targetName);
                        result = accounts.setGuildRank(request.sessionToken, characterId, target, request.rank);
                        break;
                    }
                    case GuildAction::Refresh: break;
                    }
                    auto guild = accounts.guildForCharacter(characterId);
                    guild.result = result == SocialResult::Success && guild.result == SocialResult::NotInGuild
                        ? SocialResult::Success : result;
                    guild.invitationPending = guildInvitations.contains(characterId);
                    const auto responseBytes = encodeGuildSync(guild);
                    networkServer.send(event.clientId, PacketType::GuildSync, responseBytes, true);
                    for (const auto& member : previousGuild.members) guildSyncFor(member.characterId);
                    for (const auto& member : guild.members) guildSyncFor(member.characterId);
                } else if (event.packetType == glory::network::PacketType::ZoneChangeRequest) {
                    glory::network::ZoneChangeRequestPacket request;
                    if (!glory::network::decodeZoneChangeRequest(event.payload, request)) continue;
                    const auto runtime = characterRuntimes.find(event.clientId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        dungeons.activeInstanceForCharacter(request.characterId)) continue;
                    const auto targetZone = static_cast<glory::game::ZoneId>(request.targetZone);
                    const auto [spawn, yaw] = zoneSpawn(targetZone);
                    if (!simulation.teleportPlayer(event.clientId, spawn, yaw)) continue;
                    clientZones[event.clientId] = targetZone;
                    const auto notification = glory::network::encodeZoneChangeNotification({
                        request.targetZone, spawn.x, spawn.y, spawn.z});
                    networkServer.send(event.clientId, glory::network::PacketType::ZoneChangeNotification,
                        notification, true);
                    sendWorldLootToClient(event.clientId, spawn, targetZone);
                } else if (event.packetType == glory::network::PacketType::EquipItemRequest) {
                    glory::network::EquipItemRequestPacket request;
                    if (!glory::network::decodeEquipItemRequest(event.payload, request)) continue;
                    auto runtime = characterRuntimes.find(event.clientId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::CharacterNotSelected); continue;
                    }
                    if (!accounts.ownsCharacter(request.sessionToken, request.characterId)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidSession); continue;
                    }
                    CharacterRuntime updated = runtime->second;
                    if (request.inventorySlot >= updated.inventory.slots().size()) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidSlot); continue;
                    }
                    const auto& item = updated.inventory.slots()[request.inventorySlot];
                    const auto* definition = itemCatalog.find(item.itemId);
                    const auto equipmentSlot = definition ? glory::game::equipmentSlotFor(definition->type) : std::nullopt;
                    if (!equipmentSlot || static_cast<std::uint8_t>(*equipmentSlot) !=
                        static_cast<std::uint8_t>(request.slot) || !updated.equipment.equip(updated.inventory,
                        request.inventorySlot, *equipmentSlot, itemCatalog)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidItem); continue;
                    }
                    if (!saveCharacterRuntime(request.sessionToken, updated)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::ServerError); continue;
                    }
                    runtime->second = std::move(updated);
                    sendActionResult(event.clientId, glory::network::InventoryActionResult::Success);
                    sendCharacterSync(event.clientId);
                } else if (event.packetType == glory::network::PacketType::UnequipItemRequest) {
                    glory::network::UnequipItemRequestPacket request;
                    if (!glory::network::decodeUnequipItemRequest(event.payload, request)) continue;
                    auto runtime = characterRuntimes.find(event.clientId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::CharacterNotSelected); continue;
                    }
                    if (!accounts.ownsCharacter(request.sessionToken, request.characterId)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidSession); continue;
                    }
                    CharacterRuntime updated = runtime->second;
                    if (!updated.equipment.unequip(updated.inventory,
                        static_cast<glory::game::EquipmentSlot>(request.slot), itemCatalog)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InventoryFull); continue;
                    }
                    if (!saveCharacterRuntime(request.sessionToken, updated)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::ServerError); continue;
                    }
                    runtime->second = std::move(updated);
                    sendActionResult(event.clientId, glory::network::InventoryActionResult::Success);
                    sendCharacterSync(event.clientId);
                } else if (event.packetType == glory::network::PacketType::PickupLootRequest) {
                    glory::network::PickupLootRequestPacket request;
                    if (!glory::network::decodePickupLootRequest(event.payload, request)) continue;
                    auto runtime = characterRuntimes.find(event.clientId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::CharacterNotSelected); continue;
                    }
                    if (!accounts.ownsCharacter(request.sessionToken, request.characterId)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidSession); continue;
                    }
                    const auto player = simulation.playerState(event.clientId);
                    const auto drop = std::find_if(lootSystem.drops().begin(), lootSystem.drops().end(),
                        [&](const glory::game::LootDrop& value) { return value.lootId == request.lootId; });
                    if (drop == lootSystem.drops().end()) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::LootNotFound); continue;
                    }
                    const auto dropZone = lootZones.find(request.lootId);
                    if (dropZone != lootZones.end() && dropZone->second != clientZones[event.clientId]) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::LootNotFound); continue;
                    }
                    if (!player || glm::distance(player->position, drop->position) > 2.0F) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::TooFar); continue;
                    }
                    auto updatedInventory = runtime->second.inventory;
                    auto updatedLoot = lootSystem;
                    if (!updatedLoot.pickupLoot(request.lootId, player->position, updatedInventory, itemCatalog)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InventoryFull); continue;
                    }
                    CharacterRuntime updated = runtime->second;
                    updated.inventory = std::move(updatedInventory);
                    if (!saveCharacterRuntime(request.sessionToken, updated)) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::ServerError); continue;
                    }
                    runtime->second = std::move(updated);
                    lootSystem = std::move(updatedLoot);
                    lootZones.erase(request.lootId);
                    sendActionResult(event.clientId, glory::network::InventoryActionResult::Success);
                    sendCharacterSync(event.clientId);
                    sendLootToAllClients();
                } else if (event.packetType == glory::network::PacketType::SimulateBossLootDropRequest) {
                    glory::network::SimulateBossLootDropRequestPacket request;
                    if (!glory::network::decodeSimulateBossLootDropRequest(event.payload, request)) continue;
                    const auto runtime = characterRuntimes.find(event.clientId);
                    const auto player = simulation.playerState(event.clientId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId ||
                        !accounts.ownsCharacter(request.sessionToken, request.characterId) || !player) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::InvalidSession); continue;
                    }
                    const auto lootId = lootSystem.spawnLoot("training_blade", 1, player->position, itemCatalog);
                    if (lootId == 0) {
                        sendActionResult(event.clientId, glory::network::InventoryActionResult::ServerError); continue;
                    }
                    lootZones[lootId] = clientZones[event.clientId];
                    sendActionResult(event.clientId, glory::network::InventoryActionResult::Success);
                    sendLootToAllClients();
                } else if (event.packetType == glory::network::PacketType::MarketBrowseRequest) {
                    using namespace glory::network;
                    MarketBrowseRequestPacket request;
                    if (!decodeMarketBrowseRequest(event.payload, request)) continue;
                    MarketBrowseResponsePacket response;
                    if (!accounts.ownsCharacter(request.sessionToken, request.characterId) ||
                        !activeCharacters.contains(event.clientId) || activeCharacters[event.clientId] != request.characterId) {
                        response.result = MarketResultPacket::InvalidSession;
                    } else {
                        const auto values = request.itemType == "Mine"
                            ? accounts.listingsForCharacter(request.characterId)
                            : accounts.browseMarket(request.itemType, request.search,
                                static_cast<int>(request.page), static_cast<int>(request.pageSize));
                        for (const auto& item : values)
                            response.listings.push_back({item.listingId, item.sellerCharacterId,
                                item.sellerName, item.itemId, item.itemName, item.itemType,
                                item.quantity, item.unitPrice});
                    }
                    networkServer.send(event.clientId, PacketType::MarketBrowseResponse,
                        encodeMarketBrowseResponse(response), true);
                } else if (event.packetType == glory::network::PacketType::MarketListRequest) {
                    using namespace glory::network;
                    using glory::server::MarketResult;
                    MarketListRequestPacket request;
                    if (!decodeMarketListRequest(event.payload, request)) continue;
                    MarketResult result = MarketResult::InvalidSession;
                    auto runtime = characterRuntimes.find(event.clientId);
                    const auto* item = itemCatalog.find(request.itemId);
                    if (runtime == characterRuntimes.end() || runtime->second.characterId != request.characterId) {
                        result = MarketResult::CharacterNotSelected;
                    } else if (!accounts.ownsCharacter(request.sessionToken, request.characterId)) {
                        result = MarketResult::InvalidSession;
                    } else if (!item) {
                        result = MarketResult::InvalidItem;
                    } else {
                        const char* type = item->type == glory::game::ItemType::Weapon ? "Weapon" :
                            item->type == glory::game::ItemType::Armor ? "Armor" :
                            item->type == glory::game::ItemType::Accessory ? "Accessory" : "Consumable";
                        result = accounts.listMarketItem(request.sessionToken, request.characterId,
                            request.itemId, item->name, type, request.quantity, item->maxStack, request.unitPrice);
                    }
                    if (result == MarketResult::Success && runtime != characterRuntimes.end()) {
                        std::vector<glory::server::PersistedInventorySlot> inventory;
                        std::vector<glory::server::PersistedEquipmentItem> equipment;
                        if (accounts.loadCharacterState(request.characterId, inventory, equipment)) {
                            auto& updated = runtime->second;
                            std::vector<std::pair<std::size_t, glory::game::InventorySlot>> slots;
                            for (const auto& value : inventory) if (value.slotIndex >= 0 && value.slotIndex < 24)
                                slots.push_back({static_cast<std::size_t>(value.slotIndex), {value.itemId, value.quantity}});
                            (void)updated.inventory.restore(slots, itemCatalog);
                            sendCharacterSync(event.clientId);
                        }
                    }
                    const auto response = encodeMarketActionResponse({static_cast<MarketResultPacket>(result),
                        accounts.characterGold(request.characterId)});
                    networkServer.send(event.clientId, PacketType::MarketListResponse, response, true);
                } else if (event.packetType == glory::network::PacketType::MarketBuyRequest ||
                    event.packetType == glory::network::PacketType::MarketCancelRequest) {
                    using namespace glory::network;
                    using glory::server::MarketResult;
                    MarketActionRequestPacket request;
                    if (!decodeMarketActionRequest(event.payload, request)) continue;
                    const bool buying = event.packetType == PacketType::MarketBuyRequest;
                    const auto sellerCharacterId = buying
                        ? accounts.marketListingSeller(request.listingId) : 0;
                    auto result = !activeCharacters.contains(event.clientId) ||
                        activeCharacters[event.clientId] != request.characterId
                        ? MarketResult::CharacterNotSelected
                        : buying
                            ? accounts.buyMarketListing(request.sessionToken, request.characterId, request.listingId)
                            : accounts.cancelMarketListing(request.sessionToken, request.characterId, request.listingId);
                    const auto runtime = characterRuntimes.find(event.clientId);
                    if (result == MarketResult::Success && runtime != characterRuntimes.end() &&
                        runtime->second.characterId == request.characterId) {
                        std::vector<glory::server::PersistedInventorySlot> inventory;
                        std::vector<glory::server::PersistedEquipmentItem> equipment;
                        if (accounts.loadCharacterState(request.characterId, inventory, equipment)) {
                            std::vector<std::pair<std::size_t, glory::game::InventorySlot>> slots;
                            for (const auto& value : inventory) if (value.slotIndex >= 0 && value.slotIndex < 24)
                                slots.push_back({static_cast<std::size_t>(value.slotIndex), {value.itemId, value.quantity}});
                            (void)runtime->second.inventory.restore(slots, itemCatalog);
                            sendCharacterSync(event.clientId);
                        }
                    }
                    const auto bytes = encodeMarketActionResponse({static_cast<MarketResultPacket>(result),
                        accounts.characterGold(request.characterId)});
                    networkServer.send(event.clientId,
                        buying ? PacketType::MarketBuyResponse : PacketType::MarketCancelResponse, bytes, true);
                    if (result == MarketResult::Success) sendGoldSync(event.clientId, request.characterId);
                    if (result == MarketResult::Success && buying && sellerCharacterId > 0) {
                        const auto sellerClient = clientForCharacter(sellerCharacterId);
                        if (sellerClient) sendGoldSync(sellerClient, sellerCharacterId);
                    }
                }
            }
            simulation.tick();
            for (const auto& antiCheatEvent : simulation.drainAntiCheatEvents()) {
                glory::network::AntiCheatWarningPacket warning;
                warning.violation = antiCheatEvent.kick
                    ? glory::network::AntiCheatViolationCode::Kick
                    : static_cast<glory::network::AntiCheatViolationCode>(antiCheatEvent.violation);
                warning.speedViolations = antiCheatEvent.telemetry.speedViolations;
                warning.teleportViolations = antiCheatEvent.telemetry.teleportViolations;
                warning.collisionViolations = antiCheatEvent.telemetry.collisionViolations;
                warning.cooldownViolations = antiCheatEvent.telemetry.cooldownViolations;
                warning.currentSpeed = antiCheatEvent.telemetry.currentSpeed;
                warning.maximumSpeed = antiCheatEvent.telemetry.maximumSpeed;
                warning.kick = antiCheatEvent.kick;
                const auto bytes = glory::network::encodeAntiCheatWarning(warning);
                networkServer.send(antiCheatEvent.clientId, glory::network::PacketType::AntiCheatWarning,
                    bytes, true);
                if (antiCheatEvent.kick) {
                    logger.log(glory::server::LogLevel::Critical, glory::server::LogCategory::AntiCheat,
                        "kicking client for repeated anti-cheat violations, peer_id=" +
                        std::to_string(antiCheatEvent.clientId));
                    std::cerr << "Kicking client " << antiCheatEvent.clientId
                        << ": Kicked for anti-cheat violation\n";
                    networkServer.disconnect(antiCheatEvent.clientId);
                    simulation.disconnectPlayer(antiCheatEvent.clientId);
                }
            }
            if ((executedTicks % 6U) == 0U) {
                for (const auto& player : simulation.multiplayerSnapshot().players) {
                    glory::network::AntiCheatWarningPacket telemetry;
                    telemetry.violation = glory::network::AntiCheatViolationCode::Telemetry;
                    telemetry.speedViolations = player.antiCheat.speedViolations;
                    telemetry.teleportViolations = player.antiCheat.teleportViolations;
                    telemetry.collisionViolations = player.antiCheat.collisionViolations;
                    telemetry.cooldownViolations = player.antiCheat.cooldownViolations;
                    telemetry.currentSpeed = player.antiCheat.currentSpeed;
                    telemetry.maximumSpeed = player.antiCheat.maximumSpeed;
                    telemetry.kick = player.antiCheat.kickRequested;
                    const auto bytes = glory::network::encodeAntiCheatWarning(telemetry);
                    networkServer.send(player.clientId, glory::network::PacketType::AntiCheatWarning,
                        bytes, true);
                }
            }
            for (const auto& [instanceId, instance] : dungeons.instances()) {
                if (instance.state != glory::game::DungeonState::Active) continue;
                for (const auto& member : instance.members) {
                    const auto client = clientForCharacter(member.characterId);
                    const auto player = client ? simulation.playerState(client) : std::nullopt;
                    if (player) (void)dungeons.updateMember(instanceId, member.characterId,
                        player->currentHp, player->position);
                }
            }
            dungeons.tick(static_cast<float>(tickDuration.count()) / 1'000'000'000.0F);
            std::vector<std::pair<std::uint64_t, bool>> terminalDungeons;
            for (const auto& [instanceId, instance] : dungeons.instances()) {
                if (instance.state == glory::game::DungeonState::Completed)
                    terminalDungeons.push_back({instanceId, true});
                else if (instance.state == glory::game::DungeonState::Failed)
                    terminalDungeons.push_back({instanceId, false});
            }
            for (const auto& [instanceId, completed] : terminalDungeons)
                returnDungeonMembers(instanceId, completed);
            for (const auto& [clientId, characterId] : activeCharacters) {
                const auto player = simulation.playerState(clientId);
                if (player) (void)parties.updateVitals(characterId, player->currentHp, player->maxHp);
            }
            if ((executedTicks % 15U) == 0U) {
                std::vector<std::int64_t> partyLeaders;
                for (const auto& [clientId, characterId] : activeCharacters) {
                    (void)clientId;
                    const auto members = parties.members(characterId);
                    if (!members.empty() && members.front().characterId == characterId)
                        partyLeaders.push_back(characterId);
                }
                for (const auto leaderId : partyLeaders) broadcastPartySync(leaderId);
            }
            const auto state = simulation.snapshot();
            if (previousBossHealth > 0.0F && state.bossHealth <= 0.0F) {
                const auto lootId = lootSystem.spawnLoot("training_blade", 1, state.bossPosition, itemCatalog);
                if (lootId != 0) lootZones[lootId] = glory::game::ZoneId::Zone1Plains;
                sendLootToAllClients();
            }
            previousBossHealth = state.bossHealth;
            const auto multiplayerState = simulation.multiplayerSnapshot();
            glory::game::SpatialGrid spatialGrid;
            for (const auto& player : multiplayerState.players) {
                const auto character = activeCharacters.find(player.clientId);
                if (character != activeCharacters.end() && dungeons.activeInstanceForCharacter(character->second))
                    continue;
                (void)spatialGrid.upsert({player.clientId,
                    clientZones.contains(player.clientId) ? clientZones.at(player.clientId) :
                        glory::game::ZoneId::Zone1Plains,
                    glory::game::SpatialEntityType::Player, player.position});
            }
            constexpr std::uint64_t bossSpatialId = UINT64_MAX;
            (void)spatialGrid.upsert({bossSpatialId, glory::game::ZoneId::Zone1Plains,
                glory::game::SpatialEntityType::Monster, multiplayerState.bossPosition});
            for (const auto& drop : lootSystem.drops()) {
                const auto zoneIt = lootZones.find(drop.lootId);
                const auto dropZone = zoneIt == lootZones.end()
                    ? glory::game::ZoneId::Zone1Plains : zoneIt->second;
                (void)spatialGrid.upsert({(std::uint64_t{1} << 63U) | drop.lootId, dropZone,
                    glory::game::SpatialEntityType::Loot, drop.position});
            }
            for (const auto& recipient : multiplayerState.players) {
                const auto zoneIt = clientZones.find(recipient.clientId);
                const auto zone = zoneIt == clientZones.end()
                    ? glory::game::ZoneId::Zone1Plains : zoneIt->second;
                const auto character = activeCharacters.find(recipient.clientId);
                const auto* dungeon = character == activeCharacters.end()
                    ? nullptr : dungeons.activeInstanceForCharacter(character->second);
                auto packet = makeNetworkSnapshot(multiplayerState);
                packet.currentZone = static_cast<glory::network::ZoneId>(zone);
                if (dungeon) {
                    std::unordered_map<std::uint32_t, bool> dungeonClients;
                    for (const auto& member : dungeon->members) {
                        const auto client = clientForCharacter(member.characterId);
                        if (client) dungeonClients.emplace(client, true);
                    }
                    packet.players.erase(std::remove_if(packet.players.begin(), packet.players.end(),
                        [&](const glory::network::SnapshotPlayer& player) {
                            return !dungeonClients.contains(player.clientId);
                        }), packet.players.end());
                    packet.boss = {{dungeon->bossSpawn.x, dungeon->bossSpawn.y, dungeon->bossSpawn.z},
                        dungeon->bossHealth, dungeon->definition.bossMaxHp, "DungeonBoss"};
                    packet.totalEntityCount = static_cast<std::uint16_t>(dungeonClients.size() + 1);
                    packet.visibleEntityCount = packet.totalEntityCount;
                    const auto snapshotBytes = glory::network::encodeServerSnapshot(packet);
                    networkServer.send(recipient.clientId, glory::network::PacketType::ServerSnapshot,
                        snapshotBytes, false);
                    if ((executedTicks % 15U) == 0U) sendDungeonSync(recipient.clientId, *dungeon);
                    continue;
                }
                const auto visible = spatialGrid.query(zone, recipient.position, 40.0F);
                std::unordered_map<std::uint64_t, bool> visibleIds;
                for (const auto& entity : visible) visibleIds.emplace(entity.id, true);
                packet.totalEntityCount = static_cast<std::uint16_t>(spatialGrid.entityCount());
                packet.visibleEntityCount = static_cast<std::uint16_t>(visible.size());
                packet.players.erase(std::remove_if(packet.players.begin(), packet.players.end(),
                    [&](const glory::network::SnapshotPlayer& player) {
                        return !visibleIds.contains(player.clientId);
                    }), packet.players.end());
                if (!visibleIds.contains(bossSpatialId)) packet.boss = {};
                const auto snapshotBytes = glory::network::encodeServerSnapshot(packet);
                networkServer.send(recipient.clientId, glory::network::PacketType::ServerSnapshot,
                    snapshotBytes, false);
            }
            if ((executedTicks % 15U) == 0U) sendLootToAllClients();
            ++executedTicks;
            nextTick += tickDuration;
            std::this_thread::sleep_until(nextTick);
        }
        if (backupTask.valid()) {
            try { backupTask.get(); }
            catch (const std::exception& backupError) {
                logger.log(glory::server::LogLevel::Error, glory::server::LogCategory::Database,
                    "periodic backup worker failed during shutdown: " + std::string(backupError.what()));
            }
        }
        printSnapshot(simulation.snapshot());
        if (smokeTest && executedTicks != 120) {
            std::cerr << "Server smoke test expected 120 ticks, got " << executedTicks << '\n';
            return 1;
        }
        if (smokeTest) std::cout << "Headless server smoke test passed: 120 fixed ticks at 60 Hz\n";
        logger.log(glory::server::LogLevel::Info, glory::server::LogCategory::System,
            "server process stopped cleanly");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Server startup failed: " << error.what() << '\n';
        return 1;
    }
}

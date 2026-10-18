#include "glory/engine/application.hpp"
#include "glory/game/action_map.hpp"
#include "glory/game/boss.hpp"
#include "glory/game/character_controller.hpp"
#include "glory/game/classes.hpp"
#include "glory/game/combat.hpp"
#include "glory/game/prediction.hpp"
#include "glory/game/simulation.hpp"
#include "glory/game/status_effect.hpp"
#include "glory/game/world_encounter.hpp"
#include "glory/game/inventory.hpp"
#include "glory/game/targeting.hpp"
#include "glory/network/network.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <stb_image_write.h>
#include <nlohmann/json.hpp>

namespace {

using glory::engine::SkillEditorEffect;
using glory::engine::SkillEditorEvent;
using glory::engine::SkillEditorSkill;
using glory::engine::WorldEditorSpawnPoint;
using glory::engine::WorldEditorTriggerZone;
using glory::game::SkillDefinition;
using glory::game::StatusEffectKind;

const char* effectType(StatusEffectKind kind) {
    switch (kind) {
    case StatusEffectKind::AttackBoost: return "AttackBoost";
    case StatusEffectKind::SpeedBoost: return "SpeedBoost";
    case StatusEffectKind::Regen: return "Regen";
    case StatusEffectKind::Bleed: return "Bleed";
    case StatusEffectKind::Burn: return "Burn";
    case StatusEffectKind::Slow: return "Slow";
    case StatusEffectKind::Stun: return "Stun";
    case StatusEffectKind::ArmorBreak: return "ArmorBreak";
    }
    return "AttackBoost";
}

StatusEffectKind effectKind(const std::string& type) {
    if (type == "AttackBoost") return StatusEffectKind::AttackBoost;
    if (type == "SpeedBoost") return StatusEffectKind::SpeedBoost;
    if (type == "Regen") return StatusEffectKind::Regen;
    if (type == "Bleed") return StatusEffectKind::Bleed;
    if (type == "Burn") return StatusEffectKind::Burn;
    if (type == "Slow") return StatusEffectKind::Slow;
    if (type == "Stun") return StatusEffectKind::Stun;
    if (type == "ArmorBreak") return StatusEffectKind::ArmorBreak;
    throw std::runtime_error("Unknown status effect type: " + type);
}

std::filesystem::path locateAssetRoot(const char* executablePath) {
    const auto executable = std::filesystem::absolute(executablePath).parent_path();
    const std::array<std::filesystem::path, 3> candidates{
        executable / "assets", executable.parent_path() / "assets", std::filesystem::path(GLORY_ASSET_DIR)};
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "data" / "world_map.json") &&
            std::filesystem::exists(candidate / "data" / "world_colliders.json"))
            return std::filesystem::weakly_canonical(candidate);
    }
    throw std::runtime_error("[COLLISION FATAL] Could not locate assets/data/world_map.json and "
        "assets/data/world_colliders.json beside the executable or in the configured asset directory.");
}

std::vector<SkillEditorSkill> toEditorSkills(const glory::game::SkillRegistry& registry) {
    std::vector<SkillEditorSkill> result;
    for (const SkillDefinition& source : registry.skills()) {
        SkillEditorSkill skill;
        skill.id = source.id;
        skill.name = source.name;
        skill.damage = source.damage;
        skill.cooldown = source.cooldown;
        skill.castTime = source.castTime;
        skill.animationState = source.animationState;
        skill.comboNext = source.comboNext;
        skill.comboWindow = source.comboWindow;
        skill.inputBuffer = source.inputBuffer;
        skill.cancelWindowStart = source.cancelWindowStart;
        skill.hitStopDuration = source.hitStopDuration;
        skill.knockbackSpeed = source.knockbackSpeed;
        skill.knockbackDuration = source.knockbackDuration;
        skill.hitboxRadius = source.hitboxRadius;
        skill.hitboxOffset = source.hitboxOffset;
        skill.activeStart = source.activeStart;
        skill.activeEnd = source.activeEnd;
        for (const auto& effect : source.onHitEffects)
            skill.onHitEffects.push_back({effect.id, effect.name, effectType(effect.kind), effect.duration,
                effect.magnitude, effect.tickInterval, effect.maxStacks});
        for (const auto& event : source.animationEvents)
            skill.animationEvents.push_back({event.time, event.type, event.name});
        result.push_back(std::move(skill));
    }
    return result;
}

glory::game::SkillRegistry fromEditorSkills(const std::vector<SkillEditorSkill>& sourceSkills) {
    std::vector<SkillDefinition> skills;
    for (const SkillEditorSkill& source : sourceSkills) {
        SkillDefinition skill;
        skill.id = source.id;
        skill.name = source.name;
        skill.damage = source.damage;
        skill.cooldown = source.cooldown;
        skill.castTime = source.castTime;
        skill.animationState = source.animationState;
        skill.comboNext = source.comboNext;
        skill.comboWindow = source.comboWindow;
        skill.inputBuffer = source.inputBuffer;
        skill.cancelWindowStart = source.cancelWindowStart;
        skill.hitStopDuration = source.hitStopDuration;
        skill.knockbackSpeed = source.knockbackSpeed;
        skill.knockbackDuration = source.knockbackDuration;
        skill.hitboxRadius = source.hitboxRadius;
        skill.hitboxOffset = source.hitboxOffset;
        skill.activeStart = source.activeStart;
        skill.activeEnd = source.activeEnd;
        for (const SkillEditorEffect& effect : source.onHitEffects) {
            glory::game::StatusEffect converted;
            converted.id = effect.id;
            converted.name = effect.name;
            converted.kind = effectKind(effect.type);
            converted.duration = effect.duration;
            converted.magnitude = effect.magnitude;
            converted.tickInterval = effect.tickInterval;
            converted.maxStacks = effect.maxStacks;
            skill.onHitEffects.push_back(std::move(converted));
        }
        for (const SkillEditorEvent& event : source.animationEvents)
            skill.animationEvents.push_back({event.time, event.type, event.name});
        skills.push_back(std::move(skill));
    }
    return glory::game::SkillRegistry::fromSkills(std::move(skills));
}

const char* spawnEntityType(glory::game::SpawnEntityType type) {
    switch (type) {
    case glory::game::SpawnEntityType::Player: return "Player";
    case glory::game::SpawnEntityType::PracticeDummy: return "PracticeDummy";
    case glory::game::SpawnEntityType::Boss: return "Boss";
    }
    return "Player";
}

glory::game::SpawnEntityType spawnEntityType(const std::string& type) {
    if (type == "Player") return glory::game::SpawnEntityType::Player;
    if (type == "PracticeDummy") return glory::game::SpawnEntityType::PracticeDummy;
    if (type == "Boss") return glory::game::SpawnEntityType::Boss;
    throw std::runtime_error("Unknown spawn entity type: " + type);
}

const char* triggerShape(glory::game::TriggerShape shape) {
    return shape == glory::game::TriggerShape::Box ? "Box" : "Sphere";
}

glory::game::TriggerShape triggerShape(const std::string& shape) {
    if (shape == "Box") return glory::game::TriggerShape::Box;
    if (shape == "Sphere") return glory::game::TriggerShape::Sphere;
    throw std::runtime_error("Unknown trigger shape: " + shape);
}

const char* triggerType(glory::game::TriggerType type) {
    switch (type) {
    case glory::game::TriggerType::EncounterBoundary: return "EncounterBoundary";
    case glory::game::TriggerType::BossAggro: return "BossAggro";
    case glory::game::TriggerType::HealArea: return "HealArea";
    case glory::game::TriggerType::HazardArea: return "HazardArea";
    }
    return "EncounterBoundary";
}

glory::game::TriggerType triggerType(const std::string& type) {
    if (type == "EncounterBoundary") return glory::game::TriggerType::EncounterBoundary;
    if (type == "BossAggro") return glory::game::TriggerType::BossAggro;
    if (type == "HealArea") return glory::game::TriggerType::HealArea;
    if (type == "HazardArea") return glory::game::TriggerType::HazardArea;
    throw std::runtime_error("Unknown trigger type: " + type);
}

void toWorldEditorData(const glory::game::WorldEncounter& world,
    std::vector<WorldEditorSpawnPoint>& spawns, std::vector<WorldEditorTriggerZone>& triggers) {
    spawns.clear();
    triggers.clear();
    for (const auto& spawn : world.spawnPoints())
        spawns.push_back({spawn.id, spawn.name, spawnEntityType(spawn.entityType), spawn.position, spawn.yaw});
    for (const auto& zone : world.triggerZones())
        triggers.push_back({zone.id, zone.name, triggerShape(zone.shape), zone.center,
            zone.extents, zone.radius, triggerType(zone.triggerType)});
}

glory::game::WorldEncounter fromWorldEditorData(const std::vector<WorldEditorSpawnPoint>& sourceSpawns,
    const std::vector<WorldEditorTriggerZone>& sourceTriggers) {
    std::vector<glory::game::SpawnPoint> spawns;
    std::vector<glory::game::TriggerZone> triggers;
    for (const auto& source : sourceSpawns)
        spawns.push_back({source.id, source.name, spawnEntityType(source.entityType), source.position, source.yaw});
    for (const auto& source : sourceTriggers) {
        glory::game::TriggerZone zone;
        zone.id = source.id;
        zone.name = source.name;
        zone.shape = triggerShape(source.shape);
        zone.center = source.center;
        zone.extents = source.extents;
        zone.radius = source.radius;
        zone.triggerType = triggerType(source.triggerType);
        triggers.push_back(std::move(zone));
    }
    return glory::game::WorldEncounter::fromData(std::move(spawns), std::move(triggers));
}

} // namespace

int main(int argc, char* argv[]) {
    glory::engine::ApplicationConfig config;
    std::filesystem::path assetRoot;
    try {
        assetRoot = locateAssetRoot(argv[0]);
        std::cout << "[collision] working directory=" << std::filesystem::current_path().string()
                  << "\n[collision] asset root=" << assetRoot.string() << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    const auto warriorModelPath = assetRoot / "models" / "hero_warrior.gltf";
    config.modelPath = std::filesystem::exists(warriorModelPath)
        ? warriorModelPath
        : assetRoot / "models" / "starter_crate.gltf";
    const auto bossModelPath = assetRoot / "models" / "boss_gorgon.gltf";
    config.bossModelPath = std::filesystem::exists(bossModelPath)
        ? bossModelPath
        : assetRoot / "models" / "static_crate.gltf";
    config.audioDirectory = assetRoot / "audio";
    config.skillsPath = assetRoot / "data" / "skills.json";
    config.worldSpawnsPath = assetRoot / "data" / "world_spawns.json";
    glory::engine::AccountPanelState accountPanel;
    glory::engine::InventoryPanelState inventoryPanel;
    glory::engine::ZonePanelState zonePanel;
    glory::engine::SocialPanelState socialPanel;
    glory::engine::DungeonPanelState dungeonPanel;
    glory::engine::MarketPanelState marketPanel;
    glory::engine::FeedbackPanelState feedbackPanel;
    config.accountPanel = &accountPanel;
    config.inventoryPanel = &inventoryPanel;
    config.zonePanel = &zonePanel;
    config.socialPanel = &socialPanel;
    config.dungeonPanel = &dungeonPanel;
    config.marketPanel = &marketPanel;
    config.feedbackPanel = &feedbackPanel;
    std::string connectHost;
    std::uint16_t connectPort = 7777;
    bool walkTestMode = false;
    bool moveTestMode = false;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--smoke-test") == 0) {
            config.smokeTestTickLimit = 120;
        } else if (std::strcmp(argv[index], "--smoke-test-1080p") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.smokeTestTickLimit = 120;
        } else if (std::strcmp(argv[index], "--capture") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.captureCollisionDebug = true;
        } else if (std::strcmp(argv[index], "--capture-no-hdr") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.captureHdrEnabled = false;
            config.captureCollisionDebug = true;
        } else if (std::strcmp(argv[index], "--capture-720p") == 0) {
            config.windowWidth = 1280;
            config.windowHeight = 720;
            config.captureMode = true;
            config.captureSuffix = "_1280x720";
            config.captureCollisionDebug = true;
        } else if (std::strcmp(argv[index], "--capture-high-dpi") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.highPixelDensity = true;
            config.captureSuffix = "_high_dpi";
            config.captureCollisionDebug = true;
        } else if (std::strcmp(argv[index], "--capture-colliders") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.captureCollisionDebug = true;
            config.captureSuffix = "_colliders";
        } else if (std::strcmp(argv[index], "--walk-test") == 0) {
            walkTestMode = true;
        } else if (std::strcmp(argv[index], "--move-test") == 0) {
            moveTestMode = true;
        } else if (std::strcmp(argv[index], "--camera-test") == 0) {
            config.cameraTestMode = true;
            config.captureMode = true;
            config.captureCollisionDebug = true;
            config.captureSuffix = "_camera-test";
            config.windowWidth = 640;
            config.windowHeight = 360;
        } else if (std::strcmp(argv[index], "--combat-test") == 0) {
            config.combatTestMode = true;
            config.captureMode = true;
            config.captureCollisionDebug = true;
            config.captureSuffix = "_combat-test";
            config.windowWidth = 640;
            config.windowHeight = 360;
        } else if (std::strcmp(argv[index], "--connect") == 0 && index + 1 < argc) {
            const std::string endpoint(argv[++index]);
            const std::size_t separator = endpoint.rfind(':');
            unsigned parsedPort = 0;
            if (separator == std::string::npos || separator == 0 || separator + 1 == endpoint.size()) {
                std::cerr << "Expected --connect IPv4:port\n";
                return 2;
            }
            const std::string portText = endpoint.substr(separator + 1);
            const auto parsed = std::from_chars(portText.data(), portText.data() + portText.size(), parsedPort);
            if (parsed.ec != std::errc{} || parsed.ptr != portText.data() + portText.size() ||
                parsedPort == 0 || parsedPort > 65535) {
                std::cerr << "Invalid server port\n";
                return 2;
            }
            connectHost = endpoint.substr(0, separator);
            connectPort = static_cast<std::uint16_t>(parsedPort);
        } else {
            std::cerr << "Usage: glory_client [--smoke-test|--smoke-test-1080p|--capture|--capture-no-hdr|--capture-720p|--capture-high-dpi|--capture-colliders|--walk-test|--move-test|--camera-test|--combat-test] [--connect IPv4:port]\n";
            return 2;
        }
    }

    if (connectHost.empty()) {
        accountPanel.authenticated = true;
    }

    std::unique_ptr<glory::network::NetworkContext> networkContext;
    std::unique_ptr<glory::network::NetworkClient> networkClient;
    std::optional<glory::network::ServerSnapshotPacket> authoritativeSnapshot;
    glory::network::SnapshotInterpolator snapshotInterpolator;
    if (!connectHost.empty()) {
        try {
            networkContext = std::make_unique<glory::network::NetworkContext>();
            networkClient = std::make_unique<glory::network::NetworkClient>(*networkContext);
            if (!networkClient->connect(connectHost, connectPort)) {
                std::cerr << "Unable to start connection to " << connectHost << ':' << connectPort << '\n';
                return 1;
            }
            std::cout << "Connecting to " << connectHost << ':' << connectPort << " via ENet UDP\n";
        } catch (const std::exception& error) {
            std::cerr << "Network initialization failed: " << error.what() << '\n';
            return 1;
        }
    }
    const glory::game::ItemCatalog itemCatalog = glory::game::ItemCatalog::loadFromFile(
        assetRoot / "data" / "items.json");
    const auto allClassSkills = glory::game::SkillRegistry::loadFromFile(config.skillsPath);
    const auto classCatalog = glory::game::ClassCatalog::loadFromFile(
        assetRoot / "data" / "classes.json", allClassSkills);
    for (const auto& definition : classCatalog.classes()) {
        glory::engine::ClassChoiceView choice;
        choice.name = definition.name;
        choice.description = definition.description;
        choice.hp = definition.baseHp;
        choice.attack = definition.baseAttack;
        choice.defense = definition.baseDefense;
        choice.speed = definition.moveSpeed;
        choice.dodgeCooldown = definition.dodgeCooldown;
        for (const auto& skillId : definition.skillIds) {
            const auto* skill = allClassSkills.find(skillId);
            choice.skills.push_back(skill ? skill->name : skillId);
        }
        accountPanel.classChoices.push_back(std::move(choice));
    }
    config.matchmaking = [&networkClient](unsigned int mode, bool join) {
        if (!networkClient || networkClient->state() != glory::network::ConnectionState::Connected ||
            (mode != 1 && mode != 2)) return false;
        const glory::network::MatchmakingRequestPacket request{
            static_cast<glory::network::MatchmakingMode>(mode),
            join ? glory::network::MatchmakingAction::Join : glory::network::MatchmakingAction::Leave};
        const auto payload = glory::network::encodeMatchmakingRequest(request);
        return networkClient->send(glory::network::PacketType::MatchmakingRequest, payload, true);
    };
    config.accountAction = [&networkClient, &classCatalog](glory::engine::AccountAction action,
        const glory::engine::AccountPanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected) return false;
        switch (action) {
        case glory::engine::AccountAction::Register: {
            const auto payload = encodeRegisterRequest({state.username, state.password});
            return networkClient->send(PacketType::RegisterRequest, payload, true);
        }
        case glory::engine::AccountAction::Login: {
            const auto payload = encodeLoginRequest({state.username, state.password});
            return networkClient->send(PacketType::LoginRequest, payload, true);
        }
        case glory::engine::AccountAction::RefreshCharacters: {
            const auto payload = encodeCharacterListRequest({state.sessionToken});
            return networkClient->send(PacketType::CharacterListRequest, payload, true);
        }
        case glory::engine::AccountAction::CreateCharacter: {
            if (!classCatalog.find(state.className)) return false;
            const auto payload = encodeCreateCharacterRequest({state.sessionToken, state.characterName, state.className});
            return networkClient->send(PacketType::CreateCharacterRequest, payload, true);
        }
        case glory::engine::AccountAction::EnterWorld: {
            if (state.selectedCharacter < 0 ||
                static_cast<std::size_t>(state.selectedCharacter) >= state.characters.size()) return false;
            const auto payload = encodeEnterWorldRequest({state.sessionToken,
                state.characters[static_cast<std::size_t>(state.selectedCharacter)].characterId});
            return networkClient->send(PacketType::EnterWorldRequest, payload, true);
        }
        }
        return false;
    };
    config.feedbackSubmit = [&networkClient, &accountPanel, &zonePanel](
        const glory::engine::FeedbackPanelState& state, const std::string& diagnostics) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected ||
            !accountPanel.authenticated || zonePanel.characterId <= 0) return false;
        FeedbackSubmitRequestPacket request;
        request.category = static_cast<FeedbackCategory>(std::clamp(state.category, 0, 3));
        request.rating = static_cast<std::uint8_t>(std::clamp(state.rating, 1, 5));
        request.subject = state.subject;
        request.description = state.description;
        request.diagnosticsJson = diagnostics;
        try {
            const auto payload = encodeFeedbackSubmitRequest(request);
            return networkClient->send(PacketType::FeedbackSubmitRequest, payload, true);
        } catch (const std::length_error&) {
            return false;
        }
    };
    config.inventoryAction = [&networkClient](glory::engine::InventoryAction action,
        const glory::engine::InventoryPanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected || state.characterId <= 0) return false;
        switch (action) {
        case glory::engine::InventoryAction::Equip: {
            if (state.selectedInventorySlot < 0 || state.selectedInventorySlot >= 24 ||
                state.selectedEquipmentSlot < 0 || state.selectedEquipmentSlot > 2) return false;
            const auto payload = encodeEquipItemRequest({
                state.sessionToken, state.characterId, static_cast<std::uint8_t>(state.selectedInventorySlot),
                static_cast<EquipmentSlotPacket>(state.selectedEquipmentSlot)});
            return networkClient->send(PacketType::EquipItemRequest, payload, true);
        }
        case glory::engine::InventoryAction::Unequip: {
            if (state.selectedEquipmentSlot < 0 || state.selectedEquipmentSlot > 2) return false;
            const auto payload = encodeUnequipItemRequest({state.sessionToken, state.characterId,
                static_cast<EquipmentSlotPacket>(state.selectedEquipmentSlot)});
            return networkClient->send(PacketType::UnequipItemRequest, payload, true);
        }
        case glory::engine::InventoryAction::SimulateBossLoot: {
            const auto payload = encodeSimulateBossLootDropRequest({state.sessionToken, state.characterId});
            return networkClient->send(PacketType::SimulateBossLootDropRequest, payload, true);
        }
        case glory::engine::InventoryAction::PickupLoot: {
            if (state.selectedLootId == 0) return false;
            const auto payload = encodePickupLootRequest({state.sessionToken, state.characterId, state.selectedLootId});
            return networkClient->send(PacketType::PickupLootRequest, payload, true);
        }
        }
        return false;
    };
    config.zoneChange = [&networkClient](std::uint8_t targetZone,
        const glory::engine::ZonePanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected ||
            state.characterId <= 0 || targetZone > static_cast<std::uint8_t>(ZoneId::Zone2Arena)) return false;
        const auto payload = encodeZoneChangeRequest({state.sessionToken, state.characterId,
            static_cast<ZoneId>(targetZone)});
        return networkClient->send(PacketType::ZoneChangeRequest, payload, true);
    };
    config.socialAction = [&networkClient](glory::engine::SocialAction action,
        const glory::engine::SocialPanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected || state.characterId <= 0)
            return false;
        if (action == glory::engine::SocialAction::SendChat) {
            const ChatMessagePacket request{state.sessionToken, state.characterId,
                static_cast<ChatChannel>(state.selectedChannel), {}, state.targetName, state.chatInput};
            return networkClient->send(PacketType::ChatMessage, encodeChatMessage(request), true);
        }
        const auto partyAction = [&]() -> std::optional<PartyAction> {
            switch (action) {
            case glory::engine::SocialAction::PartyCreate: return PartyAction::Create;
            case glory::engine::SocialAction::PartyInvite: return PartyAction::Invite;
            case glory::engine::SocialAction::PartyAccept: return PartyAction::Accept;
            case glory::engine::SocialAction::PartyLeave: return PartyAction::Leave;
            case glory::engine::SocialAction::PartyKick: return PartyAction::Kick;
            default: return std::nullopt;
            }
        }();
        if (partyAction) {
            const PartyRequestPacket request{state.sessionToken, state.characterId, *partyAction,
                action == glory::engine::SocialAction::PartyKick ? std::to_string(state.selectedMemberId) : state.targetName};
            return networkClient->send(PacketType::PartyRequest, encodePartyRequest(request), true);
        }
        GuildAction guildAction = GuildAction::Refresh;
        switch (action) {
        case glory::engine::SocialAction::GuildCreate: guildAction = GuildAction::Create; break;
        case glory::engine::SocialAction::GuildInvite: guildAction = GuildAction::Invite; break;
        case glory::engine::SocialAction::GuildAccept: guildAction = GuildAction::AcceptInvite; break;
        case glory::engine::SocialAction::GuildLeave: guildAction = GuildAction::Leave; break;
        case glory::engine::SocialAction::GuildSetRank: guildAction = GuildAction::SetRank; break;
        case glory::engine::SocialAction::GuildRefresh: guildAction = GuildAction::Refresh; break;
        default: return false;
        }
        GuildRequestPacket request;
        request.sessionToken = state.sessionToken;
        request.characterId = state.characterId;
        request.action = guildAction;
        request.guildId = state.guildId;
        request.guildName = state.guildNameInput;
        request.targetName = action == glory::engine::SocialAction::GuildSetRank
            ? std::to_string(state.selectedMemberId) : state.targetName;
        request.rank = static_cast<GuildRank>(state.memberRank);
        return networkClient->send(PacketType::GuildRequest, encodeGuildRequest(request), true);
    };
    config.dungeonAction = [&networkClient](glory::engine::DungeonAction action,
        const glory::engine::DungeonPanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected || state.characterId <= 0)
            return false;
        if (action == glory::engine::DungeonAction::Leave)
            return networkClient->send(PacketType::LeaveDungeonRequest,
                encodeLeaveDungeonRequest({state.sessionToken, state.characterId}), true);
        const EnterDungeonRequestPacket request{state.sessionToken, state.characterId,
            state.selectedDungeonId, action == glory::engine::DungeonAction::EnterParty};
        return networkClient->send(PacketType::EnterDungeonRequest, encodeEnterDungeonRequest(request), true);
    };
    config.marketAction = [&networkClient, &inventoryPanel](glory::engine::MarketAction action,
        const glory::engine::MarketPanelState& state) {
        using namespace glory::network;
        if (!networkClient || networkClient->state() != ConnectionState::Connected || state.characterId <= 0)
            return false;
        if (action == glory::engine::MarketAction::Browse) {
            MarketBrowseRequestPacket request{state.sessionToken, state.characterId, state.itemType,
                state.search, state.page, 20};
            return networkClient->send(PacketType::MarketBrowseRequest,
                encodeMarketBrowseRequest(request), true);
        }
        if (action == glory::engine::MarketAction::List) {
            if (state.selectedSlot < 0 || static_cast<std::size_t>(state.selectedSlot) >= inventoryPanel.slots.size())
                return false;
            const auto& item = inventoryPanel.slots[static_cast<std::size_t>(state.selectedSlot)];
            if (item.itemId.empty() || state.quantity <= 0 || state.quantity > item.quantity || state.unitPrice <= 0)
                return false;
            MarketListRequestPacket request{state.sessionToken, state.characterId, item.itemId,
                state.quantity, state.unitPrice};
            return networkClient->send(PacketType::MarketListRequest, encodeMarketListRequest(request), true);
        }
        if (state.selectedListing <= 0) return false;
        const MarketActionRequestPacket request{state.sessionToken, state.characterId, state.selectedListing};
        return networkClient->send(action == glory::engine::MarketAction::Buy
                ? PacketType::MarketBuyRequest : PacketType::MarketCancelRequest,
            encodeMarketActionRequest(request), true);
    };

    glory::game::Simulation simulation;
    glory::game::ActionMap actionMap;
    glory::game::CharacterController controller;
    const auto worldMapPath = assetRoot / "data" / "world_map.json";
    const auto worldColliderPath = assetRoot / "data" / "world_colliders.json";
    glory::game::CollisionWorld collisionWorld;
    try {
        collisionWorld = glory::game::CollisionWorld::loadFromFiles(worldMapPath, worldColliderPath);
    } catch (const std::exception& error) {
        std::cerr << "[COLLISION FATAL] map=" << std::filesystem::absolute(worldMapPath).string()
                  << " colliders=" << std::filesystem::absolute(worldColliderPath).string()
                  << " error=" << error.what() << '\n';
        return 1;
    }
    if (collisionWorld.obstacles().empty() || collisionWorld.arenaRadius() <= 0.0F) {
        std::cerr << "[COLLISION FATAL] Loaded zero world colliders or invalid arena boundary from "
                  << std::filesystem::absolute(worldColliderPath).string() << '\n';
        return 1;
    }
    std::cout << "[collision] map=" << std::filesystem::absolute(worldMapPath).string()
              << "\n[collision] colliders=" << std::filesystem::absolute(worldColliderPath).string()
              << " count=" << collisionWorld.obstacles().size()
              << " arenaCenter=(" << collisionWorld.arenaCenter().x << ',' << collisionWorld.arenaCenter().y
              << ") radius=" << collisionWorld.arenaRadius() << " noclip=off\n";
    controller.setCollisionWorld(collisionWorld);
    config.arenaCenter = collisionWorld.arenaCenter();
    config.arenaRadius = collisionWorld.arenaRadius();
    for (const auto& box : collisionWorld.obstacles())
        config.collisionDebugBoxes.push_back({box.minimum, box.maximum, box.name, box.blocksMovement});
    glory::game::WorldEncounter worldEncounter = glory::game::WorldEncounter::loadFromFile(config.worldSpawnsPath);
    toWorldEditorData(worldEncounter, config.worldSpawns, config.worldTriggers);
    if (const auto* playerSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Player))
        controller.teleport(playerSpawn->position, playerSpawn->yaw);
    glory::game::ClientPrediction prediction(controller);
    glm::vec3 practiceDummyPosition{0.0F};
    float practiceDummyYaw = 0.0F;
    if (const auto* dummySpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::PracticeDummy)) {
        practiceDummyPosition = dummySpawn->position;
        practiceDummyYaw = dummySpawn->yaw;
    }
    glory::game::StatusEffectManager playerEffects;
    float playerHealth = 150.0F;
    const glory::game::CharacterClassDefinition* activeClass = classCatalog.find("Warrior");
    if (activeClass)
        (void)controller.setClassMovement(activeClass->moveSpeed, activeClass->dodgeCooldown);
    glory::game::BossConfig bossConfig;
    if (const auto* bossSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Boss)) {
        bossConfig.spawnPosition = bossSpawn->position;
        bossConfig.spawnYaw = bossSpawn->yaw;
    }
    glory::game::Boss boss(bossConfig);
    boss.setCollisionWorld(collisionWorld);
    glory::game::AttackAimMode attackAimMode = glory::game::AttackAimMode::Assist;
    bool targetLockActive = false;
    glm::vec3 targetLockPosition{0.0F};
    bool bossEncounterActive = false;
    float bossActivationDelayRemaining = 2.0F;
    glory::game::SkillRegistry skillRegistry = glory::game::SkillRegistry::loadFromFile(config.skillsPath);
    config.editorSkills = toEditorSkills(skillRegistry);
    glory::game::CombatSystem combat(std::move(skillRegistry),
        glory::game::CombatTarget{8, {boss.position(), 0.55F, 2.2F}, 300.0F, 300.0F, 0}, collisionWorld);
    const auto applyWorldSpawns = [&]() {
        if (const auto* playerSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Player))
            controller.teleport(playerSpawn->position, playerSpawn->yaw);
        if (const auto* dummySpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::PracticeDummy)) {
            practiceDummyPosition = dummySpawn->position;
            practiceDummyYaw = dummySpawn->yaw;
        }
        if (const auto* bossSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Boss))
            boss.setSpawnTransform(bossSpawn->position, bossSpawn->yaw);
        combat.dummy().hurtbox.base = boss.position();
        bossEncounterActive = false;
        bossActivationDelayRemaining = 2.0F;
    };
    applyWorldSpawns();

    config.saveSkills = [&](const std::vector<SkillEditorSkill>& skills, std::string& error) {
        try {
            auto updated = fromEditorSkills(skills);
            updated.saveToFile(config.skillsPath);
            combat.replaceRegistry(std::move(updated));
            error.clear();
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    };
    config.reloadSkills = [&](std::vector<SkillEditorSkill>& skills, std::string& error) {
        try {
            auto updated = glory::game::SkillRegistry::loadFromFile(config.skillsPath);
            std::vector<SkillEditorSkill> editorCopy = toEditorSkills(updated);
            combat.replaceRegistry(std::move(updated));
            skills = std::move(editorCopy);
            error.clear();
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    };
    config.saveWorld = [&](const std::vector<WorldEditorSpawnPoint>& spawns,
        const std::vector<WorldEditorTriggerZone>& triggers, std::string& error) {
        try {
            auto updated = fromWorldEditorData(spawns, triggers);
            updated.saveToFile(config.worldSpawnsPath);
            worldEncounter = std::move(updated);
            applyWorldSpawns();
            error.clear();
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    };
    config.reloadWorld = [&](std::vector<WorldEditorSpawnPoint>& spawns,
        std::vector<WorldEditorTriggerZone>& triggers, std::string& error) {
        try {
            auto updated = glory::game::WorldEncounter::loadFromFile(config.worldSpawnsPath);
            std::vector<WorldEditorSpawnPoint> editorSpawns;
            std::vector<WorldEditorTriggerZone> editorTriggers;
            toWorldEditorData(updated, editorSpawns, editorTriggers);
            worldEncounter = std::move(updated);
            applyWorldSpawns();
            spawns = std::move(editorSpawns);
            triggers = std::move(editorTriggers);
            error.clear();
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    };
    std::uint64_t networkTick = 0;
    glory::network::AntiCheatWarningPacket latestAntiCheatWarning;
    std::string latestAntiCheatMessage;
    const auto fixedUpdate = [&](
        double fixedDeltaSeconds,
        const glory::input::InputFrame& inputFrame,
        float cameraYawDegrees,
        bool resetBoss,
        const std::string& requestedSkillId,
        const std::string& statusCommand,
        bool noclipEnabled) {
        controller.setCollisionEnabled(!noclipEnabled);
        using namespace glory::network;
        if (networkClient) {
            for (const auto& event : networkClient->poll()) {
                if (event.type == glory::network::EventType::Connected)
                    std::cout << "Connected to server; ClientId=" << event.clientId << '\n';
                else if (event.type == glory::network::EventType::Disconnected) {
                    std::cout << "Disconnected from server\n";
                    authoritativeSnapshot.reset();
                    snapshotInterpolator.clear();
                    prediction.reset();
                    latestAntiCheatWarning = {};
                    latestAntiCheatMessage.clear();
                    accountPanel.authenticated = false;
                    accountPanel.sessionToken.clear();
                    accountPanel.characters.clear();
                    inventoryPanel.characterId = 0;
                    inventoryPanel.sessionToken.clear();
                    inventoryPanel.slots.clear();
                    inventoryPanel.equipment.clear();
                    inventoryPanel.drops.clear();
                    socialPanel.characterId = 0;
                    socialPanel.sessionToken.clear();
                    socialPanel.partyId = 0;
                    socialPanel.guildId = 0;
                    socialPanel.partyMembers.clear();
                    socialPanel.guildMembers.clear();
                    dungeonPanel.characterId = 0;
                    dungeonPanel.sessionToken.clear();
                    dungeonPanel.instanceId = 0;
                    dungeonPanel.inDungeon = false;
                    dungeonPanel.isCompleted = false;
                    dungeonPanel.isFailed = false;
                    accountPanel.message = "Disconnected from server.";
                    accountPanel.messageIsError = true;
                } else if (event.packetType == glory::network::PacketType::Pong) {
                    zonePanel.pingRttMs = networkClient->rttMilliseconds();
                } else if (event.packetType == glory::network::PacketType::FeedbackSubmitResponse) {
                    glory::network::FeedbackSubmitResponsePacket response;
                    if (glory::network::decodeFeedbackSubmitResponse(event.payload, response)) {
                        feedbackPanel.message = response.status == glory::network::FeedbackSubmitStatus::Success
                            ? "Cảm ơn bạn đã đóng góp ý kiến cho Closed Beta!"
                            : (response.status == glory::network::FeedbackSubmitStatus::RateLimited
                                ? "Please wait before submitting more feedback."
                                : "Feedback was invalid or could not be saved.");
                    }
                }
                else if (event.packetType == glory::network::PacketType::AntiCheatWarning) {
                    glory::network::AntiCheatWarningPacket warning;
                    if (glory::network::decodeAntiCheatWarning(event.payload, warning)) {
                        latestAntiCheatWarning = warning;
                        latestAntiCheatMessage = warning.kick
                            ? "Kicked for anti-cheat violation"
                            : (warning.violation == glory::network::AntiCheatViolationCode::Telemetry
                                ? "Monitoring server authoritative movement"
                                : (warning.violation == glory::network::AntiCheatViolationCode::Cooldown
                                    ? "Skill rejected: cooldown active"
                                    : "Movement rejected; authoritative position restored"));
                    }
                }
                else if (event.packetType == glory::network::PacketType::RegisterResponse) {
                    RegisterResponsePacket response;
                    if (decodeRegisterResponse(event.payload, response)) {
                        accountPanel.messageIsError = response.result != AuthResult::Success;
                        switch (response.result) {
                        case AuthResult::Success: accountPanel.message = "Account registered. You can log in now."; accountPanel.password.clear(); break;
                        case AuthResult::UsernameTaken: accountPanel.message = "That username is already taken."; break;
                        case AuthResult::InvalidLength: accountPanel.message = "Username or password length is invalid."; break;
                        default: accountPanel.message = "Account registration failed."; break;
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::LoginResponse) {
                    LoginResponsePacket response;
                    if (decodeLoginResponse(event.payload, response)) {
                        accountPanel.messageIsError = response.result != AuthResult::Success;
                        if (response.result == AuthResult::Success) {
                            accountPanel.authenticated = true;
                            accountPanel.sessionToken = response.sessionToken;
                            inventoryPanel.sessionToken = response.sessionToken;
                            accountPanel.password.clear();
                            accountPanel.message = "Login successful.";
                            const auto request = encodeCharacterListRequest({accountPanel.sessionToken});
                            networkClient->send(PacketType::CharacterListRequest, request, true);
                        } else if (response.result == AuthResult::AccountNotFound)
                            accountPanel.message = "Account not found.";
                        else if (response.result == AuthResult::BadCredentials)
                            accountPanel.message = "Incorrect password.";
                        else if (response.result == AuthResult::InvalidLength)
                            accountPanel.message = "Username or password length is invalid.";
                        else accountPanel.message = "Login failed.";
                    }
                } else if (event.packetType == glory::network::PacketType::CharacterListResponse) {
                    CharacterListResponsePacket response;
                    if (decodeCharacterListResponse(event.payload, response)) {
                        accountPanel.messageIsError = response.result != AuthResult::Success;
                        if (response.result == AuthResult::Success) {
                            accountPanel.characters.clear();
                            for (const auto& character : response.characters)
                                accountPanel.characters.push_back({character.characterId, character.name,
                                    character.className, character.level, character.exp});
                            accountPanel.message = "Character list updated.";
                        } else accountPanel.message = "Session expired. Please log in again.";
                    }
                } else if (event.packetType == glory::network::PacketType::CreateCharacterResponse) {
                    CreateCharacterResponsePacket response;
                    if (decodeCreateCharacterResponse(event.payload, response)) {
                        accountPanel.messageIsError = response.result != AuthResult::Success;
                        if (response.result == AuthResult::Success) {
                            accountPanel.message = "Character created.";
                            const auto request = encodeCharacterListRequest({accountPanel.sessionToken});
                            networkClient->send(PacketType::CharacterListRequest, request, true);
                        } else if (response.result == AuthResult::CharacterNameTaken)
                            accountPanel.message = "That character name is already taken.";
                        else if (response.result == AuthResult::CharacterLimitReached)
                            accountPanel.message = "This account already has the maximum of 16 characters.";
                        else if (response.result == AuthResult::InvalidLength)
                            accountPanel.message = "Character name or class length is invalid.";
                        else if (response.result == AuthResult::InvalidClass)
                            accountPanel.message = "Select a valid character class.";
                        else accountPanel.message = "Character creation failed.";
                    }
                } else if (event.packetType == glory::network::PacketType::EnterWorldResponse) {
                    EnterWorldResponsePacket response;
                    if (decodeEnterWorldResponse(event.payload, response)) {
                        accountPanel.messageIsError = response.result != AuthResult::Success;
                        if (response.result == AuthResult::Success) {
                            activeClass = classCatalog.find(response.character.className);
                            if (!activeClass) activeClass = classCatalog.find("Warrior");
                            if (activeClass) {
                                (void)controller.setClassMovement(activeClass->moveSpeed, activeClass->dodgeCooldown);
                                playerHealth = activeClass->baseHp;
                                combat.replaceRegistry(classCatalog.skillsForClass(activeClass->name, allClassSkills));
                            }
                            inventoryPanel.characterId = response.character.characterId;
                            inventoryPanel.sessionToken = accountPanel.sessionToken;
                            zonePanel.characterId = response.character.characterId;
                            zonePanel.sessionToken = accountPanel.sessionToken;
                            socialPanel.characterId = response.character.characterId;
                            socialPanel.sessionToken = accountPanel.sessionToken;
                            dungeonPanel.characterId = response.character.characterId;
                            dungeonPanel.sessionToken = accountPanel.sessionToken;
                            marketPanel.characterId = response.character.characterId;
                            marketPanel.sessionToken = accountPanel.sessionToken;
                            const auto partyRequest = encodePartyRequest({accountPanel.sessionToken,
                                response.character.characterId, PartyAction::Refresh, {}});
                            networkClient->send(PacketType::PartyRequest, partyRequest, true);
                            GuildRequestPacket guildRequest;
                            guildRequest.sessionToken = accountPanel.sessionToken;
                            guildRequest.characterId = response.character.characterId;
                            guildRequest.action = GuildAction::Refresh;
                            networkClient->send(PacketType::GuildRequest, encodeGuildRequest(guildRequest), true);
                            accountPanel.message = "Entered world as " + response.character.name + ".";
                        } else if (response.result == AuthResult::CharacterNotFound)
                            accountPanel.message = "That character does not belong to this account.";
                        else if (response.result == AuthResult::InvalidSession)
                            accountPanel.message = "Session expired. Please log in again.";
                        else accountPanel.message = "Unable to enter world.";
                    }
                } else if (event.packetType == glory::network::PacketType::InventorySync) {
                    InventorySyncPacket packet;
                    if (decodeInventorySync(event.payload, packet) &&
                        (inventoryPanel.characterId == 0 || inventoryPanel.characterId == packet.characterId)) {
                        inventoryPanel.characterId = packet.characterId;
                        inventoryPanel.slots.assign(24, {});
                        for (const auto& item : packet.items) {
                            const auto* definition = itemCatalog.find(item.itemId);
                            if (item.slotIndex < inventoryPanel.slots.size())
                                inventoryPanel.slots[item.slotIndex] = {item.itemId,
                                    definition ? definition->name : item.itemId, item.quantity};
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::CharacterGoldSync) {
                    CharacterGoldSyncPacket packet;
                    if (decodeCharacterGoldSync(event.payload, packet) &&
                        packet.characterId == marketPanel.characterId) marketPanel.gold = packet.gold;
                } else if (event.packetType == glory::network::PacketType::MarketBrowseResponse) {
                    MarketBrowseResponsePacket packet;
                    if (decodeMarketBrowseResponse(event.payload, packet)) {
                        marketPanel.listings.clear();
                        for (const auto& listing : packet.listings)
                            marketPanel.listings.push_back({listing.listingId, listing.sellerName, listing.itemId,
                                listing.itemName, listing.itemType, listing.quantity, listing.unitPrice});
                        marketPanel.selectedListing = 0;
                        marketPanel.message = packet.result == MarketResultPacket::Success
                            ? "Market listings refreshed." : "Unable to browse market.";
                    }
                } else if (event.packetType == glory::network::PacketType::MarketListResponse ||
                    event.packetType == glory::network::PacketType::MarketBuyResponse ||
                    event.packetType == glory::network::PacketType::MarketCancelResponse) {
                    MarketActionResponsePacket packet;
                    if (decodeMarketActionResponse(event.payload, packet)) {
                        marketPanel.gold = packet.gold;
                        marketPanel.message = packet.result == MarketResultPacket::Success
                            ? "Market action completed." : "Market action failed (" +
                                std::to_string(static_cast<int>(packet.result)) + ").";
                        if (packet.result == MarketResultPacket::Success) {
                            marketPanel.itemType = event.packetType == glory::network::PacketType::MarketCancelResponse
                                ? "Mine" : "";
                            const auto request = encodeMarketBrowseRequest({marketPanel.sessionToken,
                                marketPanel.characterId, marketPanel.itemType, marketPanel.search,
                                marketPanel.page, 20});
                            networkClient->send(PacketType::MarketBrowseRequest, request, true);
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::EquipmentSync) {
                    EquipmentSyncPacket packet;
                    if (decodeEquipmentSync(event.payload, packet) && packet.characterId == inventoryPanel.characterId) {
                        inventoryPanel.equipment.clear();
                        for (const auto& item : packet.items) {
                            const auto* definition = itemCatalog.find(item.itemId);
                            inventoryPanel.equipment.push_back({static_cast<int>(item.slot), item.itemId,
                                definition ? definition->name : item.itemId});
                        }
                        inventoryPanel.bonusAttack = packet.bonusAttack;
                        inventoryPanel.bonusDefense = packet.bonusDefense;
                        inventoryPanel.bonusMaxHp = packet.bonusMaxHp;
                    }
                } else if (event.packetType == glory::network::PacketType::WorldLootSync) {
                    WorldLootSyncPacket packet;
                    if (decodeWorldLootSync(event.payload, packet)) {
                        inventoryPanel.drops.clear();
                        for (const auto& drop : packet.drops) {
                            const auto* definition = itemCatalog.find(drop.itemId);
                            inventoryPanel.drops.push_back({drop.lootId, drop.itemId,
                                definition ? definition->name : drop.itemId, drop.quantity,
                                {drop.x, drop.y, drop.z}});
                        }
                        if (std::none_of(inventoryPanel.drops.begin(), inventoryPanel.drops.end(),
                            [&](const auto& drop) { return drop.lootId == inventoryPanel.selectedLootId; }))
                            inventoryPanel.selectedLootId = 0;
                    }
                } else if (event.packetType == glory::network::PacketType::InventoryActionResponse) {
                    InventoryActionResponsePacket response;
                    if (decodeInventoryActionResponse(event.payload, response)) {
                        inventoryPanel.messageIsError = response.result != InventoryActionResult::Success;
                        switch (response.result) {
                        case InventoryActionResult::Success: inventoryPanel.message = "Inventory action completed."; break;
                        case InventoryActionResult::InvalidSession: inventoryPanel.message = "Session is invalid."; break;
                        case InventoryActionResult::CharacterNotSelected: inventoryPanel.message = "Select a character first."; break;
                        case InventoryActionResult::InvalidItem: inventoryPanel.message = "This item cannot use that equipment slot."; break;
                        case InventoryActionResult::InvalidSlot: inventoryPanel.message = "Inventory or equipment slot is invalid."; break;
                        case InventoryActionResult::InventoryFull: inventoryPanel.message = "Inventory is full."; break;
                        case InventoryActionResult::TooFar: inventoryPanel.message = "Move closer to the loot."; break;
                        case InventoryActionResult::LootNotFound: inventoryPanel.message = "That loot is no longer available."; break;
                        default: inventoryPanel.message = "Inventory action failed."; break;
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::ChatMessage) {
                    ChatMessagePacket packet;
                    if (decodeChatMessage(event.payload, packet)) {
                        socialPanel.chatHistory.push_back({static_cast<unsigned int>(packet.channel),
                            packet.senderName, packet.text});
                        if (socialPanel.chatHistory.size() > 100) socialPanel.chatHistory.erase(socialPanel.chatHistory.begin());
                    }
                } else if (event.packetType == glory::network::PacketType::PartySync) {
                    PartySyncPacket packet;
                    if (decodePartySync(event.payload, packet)) {
                        socialPanel.partyId = packet.partyId;
                        socialPanel.partyInvitePending = packet.invitationPending;
                        socialPanel.partyMembers.clear();
                        for (const auto& member : packet.members)
                            socialPanel.partyMembers.push_back({member.characterId, member.name, member.className,
                                member.level, member.currentHp, member.maxHp, member.leader});
                        if (packet.result != SocialResult::Success)
                            socialPanel.message = "Party request failed (" + std::to_string(static_cast<int>(packet.result)) + ").";
                    }
                } else if (event.packetType == glory::network::PacketType::GuildSync) {
                    GuildSyncPacket packet;
                    if (decodeGuildSync(event.payload, packet)) {
                        socialPanel.guildId = packet.guildId;
                        socialPanel.guildName = packet.guildName;
                        socialPanel.memberRank = static_cast<unsigned int>(packet.memberRank);
                        socialPanel.guildInvitePending = packet.invitationPending;
                        socialPanel.guildMembers.clear();
                        for (const auto& member : packet.members)
                            socialPanel.guildMembers.push_back({member.characterId, member.name, member.className,
                                member.level, static_cast<unsigned int>(member.rank)});
                        if (packet.result != SocialResult::Success && packet.result != SocialResult::NotInGuild)
                            socialPanel.message = "Guild request failed (" + std::to_string(static_cast<int>(packet.result)) + ").";
                    }
                } else if (event.packetType == glory::network::PacketType::EnterDungeonResponse) {
                    EnterDungeonResponsePacket response;
                    if (decodeEnterDungeonResponse(event.payload, response)) {
                        if (response.result == DungeonResult::Success) {
                            dungeonPanel.selectedDungeonId = response.dungeonId;
                            dungeonPanel.instanceId = response.instanceId;
                            dungeonPanel.inDungeon = true;
                            dungeonPanel.isCompleted = false;
                            dungeonPanel.isFailed = false;
                            dungeonPanel.message.clear();
                        } else {
                            dungeonPanel.message = "Unable to enter dungeon (" +
                                std::to_string(static_cast<int>(response.result)) + ").";
                        }
                    }
                } else if (event.packetType == glory::network::PacketType::DungeonStateSync) {
                    DungeonStateSyncPacket packet;
                    if (decodeDungeonStateSync(event.payload, packet)) {
                        dungeonPanel.selectedDungeonId = packet.dungeonId ? packet.dungeonId : 1;
                        dungeonPanel.instanceId = packet.instanceId;
                        dungeonPanel.timerRemaining = packet.timerRemaining;
                        dungeonPanel.bossHealth = packet.bossHealth;
                        dungeonPanel.bossMaxHealth = packet.bossMaxHealth;
                        dungeonPanel.isCompleted = packet.isCompleted;
                        dungeonPanel.isFailed = packet.isFailed;
                        dungeonPanel.inDungeon = packet.instanceId != 0 && !packet.isCompleted && !packet.isFailed;
                    }
                } else if (event.packetType == glory::network::PacketType::ZoneChangeNotification) {
                    ZoneChangeNotificationPacket notification;
                    if (decodeZoneChangeNotification(event.payload, notification)) {
                        static const char* zoneNames[] = {"Zone1_Plains", "Zone2_Arena", "Zone3_Forest"};
                        zonePanel.currentZone = zoneNames[static_cast<std::size_t>(notification.currentZone)];
                        controller.teleport({notification.spawnX, notification.spawnY, notification.spawnZ}, -90.0F);
                        prediction.reset();
                        snapshotInterpolator.clear();
                        authoritativeSnapshot.reset();
                        inventoryPanel.drops.clear();
                        inventoryPanel.selectedLootId = 0;
                    }
                }
                else if (event.packetType == glory::network::PacketType::ServerSnapshot) {
                    glory::network::ServerSnapshotPacket decoded;
                    if (glory::network::decodeServerSnapshot(event.payload, decoded)) {
                        static const char* zoneNames[] = {"Zone1_Plains", "Zone2_Arena", "Zone3_Forest"};
                        zonePanel.currentZone = zoneNames[static_cast<std::size_t>(decoded.currentZone)];
                        zonePanel.totalEntityCount = decoded.totalEntityCount;
                        zonePanel.visibleEntityCount = decoded.visibleEntityCount;
                        (void)snapshotInterpolator.addSnapshot(decoded,
                            glory::network::SnapshotInterpolator::monotonicMilliseconds());
                        for (const auto& serverPlayer : decoded.players) {
                            if (serverPlayer.clientId != networkClient->clientId()) continue;
                            glory::game::CharacterControllerState serverState;
                            serverState.position = {serverPlayer.position.x, serverPlayer.position.y,
                                serverPlayer.position.z};
                            serverState.velocity = {serverPlayer.velocity.x, serverPlayer.velocity.y,
                                serverPlayer.velocity.z};
                            serverState.facingYaw = serverPlayer.yaw;
                            serverState.isGrounded = serverPlayer.isGrounded;
                            serverState.isDodging = serverPlayer.isDodging;
                            serverState.isInvulnerable = serverPlayer.isInvulnerable;
                            serverState.dodgeElapsed = serverPlayer.dodgeElapsed;
                            serverState.dodgeCooldownRemaining = serverPlayer.dodgeCooldownRemaining;
                            serverState.dodgeDirection = {serverPlayer.dodgeDirection.x,
                                serverPlayer.dodgeDirection.y, serverPlayer.dodgeDirection.z};
                            serverState.externalKnockbackVelocity = {
                                serverPlayer.externalKnockbackVelocity.x,
                                serverPlayer.externalKnockbackVelocity.y,
                                serverPlayer.externalKnockbackVelocity.z};
                            serverState.knockbackRemaining = serverPlayer.knockbackRemaining;
                            prediction.reconcile(serverPlayer.lastProcessedInputSequence, {serverState});
                            break;
                        }
                        if (!authoritativeSnapshot || decoded.serverTick >= authoritativeSnapshot->serverTick)
                            authoritativeSnapshot = std::move(decoded);
                    }
                }
            }
            if (networkClient->state() == glory::network::ConnectionState::Connected) {
                const bool arenaInputLocked = authoritativeSnapshot &&
                    (authoritativeSnapshot->matchState == glory::network::ArenaMatchState::Countdown ||
                     authoritativeSnapshot->matchState == glory::network::ArenaMatchState::MatchFinished);
                const glory::input::InputFrame noInput;
                const glory::input::InputFrame& effectiveInput = arenaInputLocked ? noInput : inputFrame;
                const auto toButtonMask = [](const std::vector<glory::input::Control>& controls) {
                    std::uint32_t mask = 0;
                    for (const auto control : controls) {
                        const auto bit = static_cast<unsigned>(control);
                        if (bit < 32) mask |= (1U << bit);
                    }
                    return mask;
                };
                const std::uint32_t sequence = prediction.predict(effectiveInput, cameraYawDegrees,
                    fixedDeltaSeconds);
                constexpr std::uint64_t interpolationDelayTicks = 5;
                const std::uint64_t targetTick = authoritativeSnapshot &&
                    authoritativeSnapshot->serverTick > interpolationDelayTicks
                    ? authoritativeSnapshot->serverTick - interpolationDelayTicks : 0;
                const auto packet = glory::network::encodeClientInput({toButtonMask(effectiveInput.pressed),
                    toButtonMask(effectiveInput.held), cameraYawDegrees, sequence, targetTick});
                networkClient->send(glory::network::PacketType::ClientInput, packet, false);
                if (statusCommand == "anti_cheat_speed_probe" ||
                    statusCommand == "anti_cheat_teleport_probe") {
                    glm::vec3 claimedPosition = controller.state().position;
                    if (statusCommand == "anti_cheat_speed_probe") claimedPosition.x += 0.5F;
                    else claimedPosition.x += 50.0F;
                    const auto probe = glory::network::encodeAntiCheatProbe({
                        glory::network::AntiCheatProbeKind::PositionClaim,
                        claimedPosition.x, claimedPosition.y, claimedPosition.z});
                    networkClient->send(glory::network::PacketType::AntiCheatProbe, probe, true);
                } else if (statusCommand == "anti_cheat_cooldown_probe") {
                    const auto request = glory::network::encodeSkillRequest("heavy_strike");
                    networkClient->send(glory::network::PacketType::SkillRequest, request, true);
                }
                if (!requestedSkillId.empty()) {
                    const auto request = glory::network::encodeSkillRequest(requestedSkillId);
                    networkClient->send(glory::network::PacketType::SkillRequest, request, true);
                }
                if ((networkTick % 60) == 0) networkClient->sendPing();
            }
            ++networkTick;
            if (networkClient->state() == glory::network::ConnectionState::Connected && accountPanel.authenticated) {
                glory::engine::PlayerFrameState networkFrame;
                networkFrame.networkConnected =
                    networkClient->state() == glory::network::ConnectionState::Connected;
                networkFrame.antiCheatCurrentSpeed = latestAntiCheatWarning.currentSpeed;
                networkFrame.antiCheatMaximumSpeed = latestAntiCheatWarning.maximumSpeed;
                networkFrame.speedViolations = latestAntiCheatWarning.speedViolations;
                networkFrame.teleportViolations = latestAntiCheatWarning.teleportViolations;
                networkFrame.collisionViolations = latestAntiCheatWarning.collisionViolations;
                networkFrame.cooldownViolations = latestAntiCheatWarning.cooldownViolations;
                networkFrame.antiCheatMessage = latestAntiCheatMessage;
                networkFrame.serverPlayerCount = authoritativeSnapshot
                    ? static_cast<unsigned int>(authoritativeSnapshot->players.size()) : 0;
                networkFrame.authoritativeClientId = networkClient->clientId();
                const auto& predictionStats = prediction.debugState();
                networkFrame.clientInputSequence = predictionStats.clientSequence;
                networkFrame.serverAckSequence = predictionStats.serverAckSequence;
                networkFrame.unacknowledgedInputCount = static_cast<unsigned int>(
                    predictionStats.unacknowledgedInputCount);
                networkFrame.reconciliationCount = predictionStats.reconciliationCount;
                networkFrame.mispredictionOffset = predictionStats.mispredictionOffset;
                const auto& predictedState = controller.state();
                networkFrame.position = predictedState.position;
                networkFrame.velocity = predictedState.velocity;
                networkFrame.facingYaw = predictedState.facingYaw;
                networkFrame.isDodging = predictedState.isDodging;
                networkFrame.isInvulnerable = predictedState.isInvulnerable;
                if (authoritativeSnapshot) {
                    networkFrame.serverTick = authoritativeSnapshot->serverTick;
                    networkFrame.matchState = static_cast<unsigned int>(authoritativeSnapshot->matchState);
                    networkFrame.matchMode = static_cast<unsigned int>(authoritativeSnapshot->matchMode);
                    networkFrame.winningTeam = static_cast<unsigned int>(authoritativeSnapshot->winningTeam);
                    networkFrame.countdownRemaining = authoritativeSnapshot->countdownRemaining;
                    for (const auto& remote : authoritativeSnapshot->players) {
                        if (remote.clientId != networkFrame.authoritativeClientId) continue;
                        networkFrame.localTeam = static_cast<unsigned int>(remote.team);
                        networkFrame.authoritativePosition = {remote.position.x, remote.position.y, remote.position.z};
                        networkFrame.authoritativeHp = remote.currentHp;
                        networkFrame.authoritativeMaxHp = remote.maxHp;
                        networkFrame.authoritativeAnimation = remote.animationState;
                        networkFrame.authoritativeSkill = remote.activeSkillId;
                        networkFrame.playerHp = remote.currentHp;
                        networkFrame.playerMaxHp = remote.maxHp;
                        networkFrame.isDodging = remote.isDodging;
                        networkFrame.isInvulnerable = remote.isInvulnerable;
                    }
                    const auto remotePlayers = snapshotInterpolator.sample(networkFrame.authoritativeClientId,
                        {networkFrame.position.x, networkFrame.position.y, networkFrame.position.z},
                        glory::network::SnapshotInterpolator::monotonicMilliseconds());
                    networkFrame.remotePlayers.reserve(remotePlayers.size());
                    for (const auto& remote : remotePlayers) {
                        networkFrame.remotePlayers.push_back({remote.state.clientId,
                            {remote.state.position.x, remote.state.position.y, remote.state.position.z},
                            remote.state.yaw, remote.state.currentHp, remote.state.maxHp,
                            remote.distanceFromLocal, remote.state.animationState, remote.state.activeSkillId,
                            static_cast<unsigned int>(remote.state.team)});
                    }
                    networkFrame.bossPosition = {authoritativeSnapshot->boss.position.x,
                        authoritativeSnapshot->boss.position.y, authoritativeSnapshot->boss.position.z};
                    networkFrame.bossHp = authoritativeSnapshot->boss.currentHp;
                    networkFrame.bossMaxHp = authoritativeSnapshot->boss.maxHp;
                    networkFrame.bossPhase = authoritativeSnapshot->boss.phase;
                }
                return networkFrame;
            }
        }
        actionMap.update(inputFrame);
        if (statusCommand == "aim_mode_manual") attackAimMode = glory::game::AttackAimMode::Manual;
        else if (statusCommand == "aim_mode_assist") attackAimMode = glory::game::AttackAimMode::Assist;
        else if (statusCommand == "aim_mode_auto") attackAimMode = glory::game::AttackAimMode::Auto;
        using glory::game::StatusEffect;
        using glory::game::StatusEffectKind;
        if (statusCommand == "player_speed")
            playerEffects.apply({"debug_speed", "Speed Boost", StatusEffectKind::SpeedBoost, 8.0F, 8.0F,
                0.0F, 0.0F, 0.3F, 1, 1});
        else if (statusCommand == "player_attack")
            playerEffects.apply({"debug_attack", "Attack Boost", StatusEffectKind::AttackBoost, 8.0F, 8.0F,
                0.0F, 0.0F, 0.3F, 1, 1});
        else if (statusCommand == "dummy_burn")
            combat.dummy().statusEffects.apply({"debug_burn", "Burn", StatusEffectKind::Burn, 3.0F, 3.0F,
                0.5F, 0.5F, 5.0F, 1, 1});
        else if (statusCommand == "dummy_stun")
            combat.dummy().statusEffects.apply({"debug_stun", "Stun", StatusEffectKind::Stun, 1.0F, 1.0F,
                0.0F, 0.0F, 1.0F, 1, 1});
        else if (statusCommand == "clear_effects") {
            playerEffects.clear();
            combat.dummy().statusEffects.clear();
        }
        if (resetBoss) {
            boss.reset();
            applyWorldSpawns();
            combat.resetEncounter();
            controller.reset();
            combat.dummy().maxHp = boss.maxHp();
            combat.dummy().currentHp = boss.currentHp();
            combat.dummy().hurtbox = {boss.position(), 0.55F, 2.2F};
            playerEffects.clear();
            playerHealth = activeClass ? activeClass->baseHp : 100.0F;
            targetLockActive = false;
        }
        if (config.combatTestMode && statusCommand.starts_with("combat_setup_")) {
            controller.teleport({0.0F, 0.0F, 0.0F}, 0.0F);
            const bool farTarget = statusCommand == "combat_setup_far";
            boss.setSpawnTransform(farTarget ? glm::vec3{35.0F, 0.0F, 0.0F} : glm::vec3{3.0F, 0.0F, 0.0F}, 0.0F);
            combat.dummy().hurtbox.base = boss.position();
            combat.dummy().currentHp = boss.currentHp();
            if (statusCommand == "combat_setup_manual") attackAimMode = glory::game::AttackAimMode::Manual;
            else if (statusCommand == "combat_setup_assist") attackAimMode = glory::game::AttackAimMode::Assist;
            else if (statusCommand == "combat_setup_auto") attackAimMode = glory::game::AttackAimMode::Auto;
        }
        if (actionMap.state(glory::game::Action::TargetLock).pressed) {
            if (targetLockActive) {
                targetLockActive = false;
            } else {
                const auto selected = glory::game::chooseLockTarget(collisionWorld, controller.state().position,
                    cameraYawDegrees, boss.position(), !boss.isDead(), 25.0F, 30.0F);
                targetLockActive = selected.valid;
                if (targetLockActive) targetLockPosition = selected.position;
            }
        }
        if (targetLockActive) {
            if (!glory::game::shouldMaintainLock(controller.state().position, boss.position(),
                    !boss.isDead(), 30.0F)) {
                targetLockActive = false;
            } else {
                targetLockPosition = boss.position();
            }
        }
        const float activeMaxHp = activeClass ? activeClass->baseHp : 100.0F;
        playerEffects.update(static_cast<float>(fixedDeltaSeconds), playerHealth, activeMaxHp);
        if (playerHealth > 0.0F && !combat.isHitStopped())
            controller.update(actionMap, cameraYawDegrees, fixedDeltaSeconds, combat.dodgeAllowed(),
                playerEffects.calculateSpeedMultiplier(), playerEffects.isStunned(), targetLockActive, targetLockPosition);
        simulation.fixedUpdate(fixedDeltaSeconds);
        const auto& state = controller.state();
        const bool respawnPressed = std::find(inputFrame.pressed.begin(), inputFrame.pressed.end(),
            glory::input::Control::KeyR) != inputFrame.pressed.end();
        if (playerHealth <= 0.0F && !respawnPressed) {
            glory::engine::PlayerFrameState deadFrame;
            deadFrame.position = state.position;
            deadFrame.facingYaw = state.facingYaw;
            deadFrame.isGrounded = state.isGrounded;
            deadFrame.playerHp = 0.0F;
            deadFrame.playerMaxHp = activeClass ? activeClass->baseHp : 100.0F;
            deadFrame.playerDead = true;
            deadFrame.className = activeClass ? activeClass->name : std::string{};
            deadFrame.bossHp = boss.currentHp();
            deadFrame.bossMaxHp = boss.maxHp();
            deadFrame.bossName = boss.name();
            deadFrame.bossPosition = boss.position();
            deadFrame.bossYaw = boss.yawDegrees();
            deadFrame.bossDefeated = boss.isDead();
            deadFrame.bossPhase = boss.isDead() ? "Phase 2: DEFEATED" : "Phase 1";
            return deadFrame;
        }
        if (playerHealth <= 0.0F && respawnPressed) {
            if (const auto* playerSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Player))
                controller.teleport(playerSpawn->position, playerSpawn->yaw);
            playerHealth = activeClass ? activeClass->baseHp : 100.0F;
            playerEffects.clear();
            const auto& respawned = controller.state();
            (void)worldEncounter.updateTriggers(respawned.position);
        }
        const auto triggerEvents = worldEncounter.updateTriggers(state.position);
        for (const auto& triggerEvent : triggerEvents) {
            if (triggerEvent.transition != glory::game::TriggerTransition::Enter) continue;
            if (triggerEvent.type == glory::game::TriggerType::BossAggro && !bossEncounterActive) {
                bossEncounterActive = true;
                bossActivationDelayRemaining = 2.0F;
            }
            if (triggerEvent.type == glory::game::TriggerType::HealArea)
                playerHealth = activeClass ? activeClass->baseHp : 100.0F;
            if (triggerEvent.type == glory::game::TriggerType::HazardArea)
                playerHealth = std::max(0.0F, playerHealth - 10.0F);
        }
        if (boss.currentHp() < boss.maxHp() && !bossEncounterActive) {
            bossEncounterActive = true;
            bossActivationDelayRemaining = 2.0F;
        }
        if (bossEncounterActive && bossActivationDelayRemaining > 0.0F)
            bossActivationDelayRemaining = std::max(0.0F,
                bossActivationDelayRemaining - static_cast<float>(fixedDeltaSeconds));
        if (state.position.z >= 16.0F) {
            zonePanel.currentZone = "Thành Trấn Khởi Nguyên (Sanctuary Citadel)";
        } else if (state.position.z >= -25.0F) {
            zonePanel.currentZone = "Vùng Dã Ngoại (Outer Wilds)";
        } else {
            zonePanel.currentZone = "Phế Tích Cấm Địa (Ruins of Desolation)";
        }
        std::string effectiveSkillId = requestedSkillId;
        const auto& availableSkills = combat.skillStatuses();
        if (actionMap.state(glory::game::Action::Skill1).pressed && !availableSkills.empty())
            effectiveSkillId = availableSkills[0].id;
        else if (actionMap.state(glory::game::Action::Skill2).pressed && availableSkills.size() > 1)
            effectiveSkillId = availableSkills[1].id;
        else if (actionMap.state(glory::game::Action::Skill3).pressed && availableSkills.size() > 2)
            effectiveSkillId = availableSkills[2].id;
        else if (actionMap.state(glory::game::Action::Skill4).pressed && availableSkills.size() > 3)
            effectiveSkillId = availableSkills[3].id;
        if (!effectiveSkillId.empty()) combat.requestSkill(effectiveSkillId);
        combat.dummy().hurtbox.base = boss.position();
        combat.dummy().currentHp = boss.currentHp();
        const bool moving = glm::length(glm::vec2(state.velocity.x, state.velocity.z)) > 1.0e-4F;
        const bool wantsAttackAnimation = state.attackRequested || !effectiveSkillId.empty() ||
            combat.animationState() == "Attack";
        const std::string requestedAnimation = wantsAttackAnimation ? "Attack" : (moving ? "Run" : "Idle");
        const bool attackPressed = actionMap.state(glory::game::Action::PrimaryAttack).pressed ||
            state.attackRequested || !effectiveSkillId.empty();
        const auto attackTarget = targetLockActive
            ? glory::game::chooseAttackTarget(collisionWorld, state.position, cameraYawDegrees,
                targetLockPosition, !boss.isDead(), glory::game::AttackAimMode::Auto, combat.maximumAttackRange())
            : glory::game::chooseAttackTarget(collisionWorld, state.position, cameraYawDegrees,
                boss.position(), !boss.isDead(), attackAimMode, combat.maximumAttackRange());
        if (attackPressed)
            controller.orientForAttack(attackTarget.valid ? attackTarget.yawDegrees : cameraYawDegrees);
        combat.update(static_cast<float>(fixedDeltaSeconds), state.position, state.facingYaw,
            attackPressed, requestedAnimation,
            actionMap.state(glory::game::Action::Dodge).pressed, state.isDodging,
            (activeClass ? activeClass->baseAttack / 25.0F : 1.0F) *
                playerEffects.calculateDamageDealtMultiplier(), playerEffects.isStunned());
        boss.setPosition(combat.dummy().hurtbox.base);
        boss.setHealth(combat.dummy().currentHp);
        if (!combat.hitEvents().empty()) boss.stagger(0.06F);
        if (combat.dummy().statusEffects.isStunned())
            boss.stagger(static_cast<float>(fixedDeltaSeconds));
        const glory::game::BossUpdateResult bossResult = bossEncounterActive && bossActivationDelayRemaining <= 0.0F
            ? boss.update(static_cast<float>(fixedDeltaSeconds), state.position, state.isInvulnerable)
            : glory::game::BossUpdateResult{};
        const auto separation = glory::game::separateCapsules(
            {state.position, 0.4F, 1.8F}, {boss.position(), boss.hurtbox().radius, boss.hurtbox().height});
        controller.resolveWorldDisplacement(separation.first);
        boss.resolveWorldDisplacement(separation.second);
        if (bossResult.playerDamage > 0.0F) {
            playerHealth = std::max(0.0F, playerHealth - bossResult.playerDamage);
            controller.applyKnockback(bossResult.playerKnockback);
        }
        combat.dummy().hurtbox.base = boss.position();
        combat.dummy().currentHp = boss.currentHp();
        glory::engine::PlayerFrameState frame{
            state.position,
            state.velocity,
            state.facingYaw,
            state.isGrounded,
            moving,
            combat.attackAnimationRequested(),
            state.isDodging,
            state.isInvulnerable,
            state.dodgeCooldownRemaining,
            state.collisionCount,
        };
        frame.dummyHp = combat.dummy().currentHp;
        frame.dummyMaxHp = combat.dummy().maxHp;
        frame.dummyHitCount = combat.dummy().hitCount;
        frame.comboStep = combat.comboStep();
        frame.comboWindowOpen = combat.comboWindowOpen();
        frame.cancelWindowAvailable = combat.cancelWindowAvailable();
        frame.hitStopActive = combat.isHitStopped();
        frame.attackCanceled = combat.attackCanceled();
        frame.hitStopRemaining = combat.hitStopRemaining();
        frame.dummyKnockbackVelocity = combat.dummy().knockbackVelocity;
        frame.playerStunned = playerEffects.isStunned();
        frame.playerHp = playerHealth;
        frame.playerMaxHp = activeMaxHp;
        frame.className = activeClass ? activeClass->name : std::string{};
        frame.bossHp = boss.currentHp();
        frame.bossMaxHp = boss.maxHp();
        frame.bossName = boss.name();
        frame.bossPhase = boss.isDead() ? "Phase 2: DEFEATED" :
            (boss.phase() == glory::game::BossPhase::Phase2 ? "Phase 2: ENRAGED" : "Phase 1");
        frame.bossBehaviorNode = boss.activeNode();
        frame.bossAttackName = bossResult.attackName;
        frame.bossEnrageTriggered = bossResult.enrageTriggered;
        frame.bossAttackHit = bossResult.playerHit;
        frame.bossDefeated = boss.isDead();
        frame.bossPosition = boss.position();
        frame.bossYaw = boss.yawDegrees();
        frame.targetLocked = targetLockActive;
        frame.targetLockPosition = targetLockPosition;
        frame.attackAimMode = static_cast<int>(attackAimMode);
        frame.attackAimTargetValid = attackTarget.valid;
        frame.attackAimTargetPosition = attackTarget.position;
        frame.activeSkillDamage = combat.activeSkillDamage();
        frame.activeSkillCooldown = combat.activeSkillCooldown();
        frame.activeSkillHitboxRadius = combat.activeSkillHitboxRadius();
        frame.practiceDummyPosition = practiceDummyPosition;
        frame.practiceDummyYaw = practiceDummyYaw;
        for (const std::string& activeId : worldEncounter.activeTriggerIds()) {
            const auto zone = std::find_if(worldEncounter.triggerZones().begin(), worldEncounter.triggerZones().end(),
                [&](const glory::game::TriggerZone& candidate) { return candidate.id == activeId; });
            if (zone != worldEncounter.triggerZones().end())
                frame.activeTriggerStates.push_back(zone->name + " [Inside]");
        }
        for (const auto& effect : playerEffects.effects())
            frame.playerEffects.push_back({effect.name, effect.remainingTime, effect.stackCount});
        for (const auto& effect : combat.dummy().statusEffects.effects())
            frame.dummyEffects.push_back({effect.name, effect.remainingTime, effect.stackCount});
        for (const auto& effect : boss.statusEffects().effects())
            frame.dummyEffects.push_back({effect.name, effect.remainingTime, effect.stackCount});
        for (const auto& skill : combat.skillStatuses())
            frame.skills.push_back({skill.id, skill.name, skill.cooldownRemaining, skill.cooldownDuration});
        for (const auto& event : combat.hitEvents())
            frame.hitEvents.push_back({event.position, event.damage});
        if (bossResult.playerDamage > 0.0F)
            frame.hitEvents.push_back({state.position + glm::vec3(0.0F, 1.0F, 0.0F), -bossResult.playerDamage});
        return frame;
    };

    int result = 0;
    if (moveTestMode) {
        if (networkClient) {
            std::cerr << "--move-test requires offline mode so it can exercise this process's live update callback.\n";
            return 2;
        }
        const auto* playerSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Player);
        if (!playerSpawn) {
            std::cerr << "--move-test requires a player spawn in world_spawns.json.\n";
            return 1;
        }

        struct MoveTrace {
            std::string name;
            float cameraYaw = 0.0F;
            std::vector<glm::vec3> positions;
            float firstSpeed = -1.0F;
            float firstReleaseSpeed = -1.0F;
            float minimumDirectionDot = 1.0F;
            float finalFacingYaw = 0.0F;
            float maximumTurnRate = 0.0F;
        };
        struct MoveScenario {
            const char* name;
            std::vector<glory::input::Control> controls;
        };
        const std::array<MoveScenario, 5> scenarios{{
            {"W", {glory::input::Control::KeyW}},
            {"A", {glory::input::Control::KeyA}},
            {"S", {glory::input::Control::KeyS}},
            {"D", {glory::input::Control::KeyD}},
            {"WA", {glory::input::Control::KeyW, glory::input::Control::KeyA}},
        }};
        const std::array<float, 5> cameraYaws{{0.0F, 90.0F, 180.0F, 270.0F, 37.5F}};
        constexpr double fixedDelta = 1.0 / 60.0;
        constexpr double heldSeconds = 1.0;
        constexpr double totalSeconds = 1.4;
        const float targetSpeed = activeClass ? activeClass->moveSpeed : 5.5F;
        bool failed = false;
        std::string firstFailure;
        std::array<float, 3> cardinalDistances{};
        std::filesystem::create_directories("captures/move-test");
        glory::game::CollisionWorld movementWorld;

        const auto resetMovementScenario = [&](const glory::game::CollisionWorld& world) {
            controller.setCollisionWorld(world);
            controller.teleport(playerSpawn->position, -90.0F);
            playerHealth = activeClass ? activeClass->baseHp : 150.0F;
            boss.reset();
            boss.setCollisionWorld(collisionWorld);
            boss.setHealth(0.0F);
            combat.resetEncounter();
            combat.dummy().hurtbox = {boss.position(), boss.hurtbox().radius, boss.hurtbox().height};
            combat.dummy().currentHp = boss.currentHp();
            bossEncounterActive = false;
            bossActivationDelayRemaining = 2.0F;
        };

        const auto runScenario = [&](int renderFps, float cameraYaw, const MoveScenario& scenario,
            const glory::game::CollisionWorld& world, double scenarioHeldSeconds = heldSeconds) {
            resetMovementScenario(world);
            MoveTrace trace;
            trace.name = scenario.name;
            trace.cameraYaw = cameraYaw;
            trace.positions.push_back(controller.state().position);
            double accumulator = 0.0;
            double fixedElapsed = 0.0;
            float previousYaw = controller.state().facingYaw;
            bool sawRelease = false;
            const int renderFrames = static_cast<int>(std::ceil(totalSeconds * renderFps));
            for (int frameIndex = 0; frameIndex < renderFrames; ++frameIndex) {
                const double frameDelta = 1.0 / static_cast<double>(renderFps);
                accumulator += frameDelta;
                while (accumulator + 1.0e-9 >= fixedDelta) {
                    glory::input::InputFrame scriptedInput;
                    if (fixedElapsed < scenarioHeldSeconds)
                        scriptedInput.held = scenario.controls;
                    const glm::vec3 previousPosition = controller.state().position;
                    const auto frame = fixedUpdate(fixedDelta, scriptedInput, cameraYaw, false, {}, {}, false);
                    (void)frame;
                    const auto& state = controller.state();
                    const glm::vec3 displacement = state.position - previousPosition;
                    const float speed = glm::length(glm::vec2(displacement.x, displacement.z)) /
                        static_cast<float>(fixedDelta);
                    const glm::vec2 actualVelocity{displacement.x, displacement.z};
                    const float actualLength = glm::length(actualVelocity);
                    glm::vec2 expectedDirection{0.0F};
                    const float yawRadians = cameraYaw * 0.01745329251994329577F;
                    const glm::vec2 forward{std::cos(yawRadians), std::sin(yawRadians)};
                    const glm::vec2 right{-std::sin(yawRadians), std::cos(yawRadians)};
                    if (std::find(scenario.controls.begin(), scenario.controls.end(),
                            glory::input::Control::KeyW) != scenario.controls.end()) expectedDirection += forward;
                    if (std::find(scenario.controls.begin(), scenario.controls.end(),
                            glory::input::Control::KeyS) != scenario.controls.end()) expectedDirection -= forward;
                    if (std::find(scenario.controls.begin(), scenario.controls.end(),
                            glory::input::Control::KeyD) != scenario.controls.end()) expectedDirection += right;
                    if (std::find(scenario.controls.begin(), scenario.controls.end(),
                            glory::input::Control::KeyA) != scenario.controls.end()) expectedDirection -= right;
                    if (glm::length(expectedDirection) > 1.0e-5F) expectedDirection = glm::normalize(expectedDirection);
                    if (fixedElapsed < scenarioHeldSeconds && fixedElapsed >= 0.35 && actualLength > 1.0e-4F)
                        trace.minimumDirectionDot = std::min(trace.minimumDirectionDot,
                            glm::dot(actualVelocity / actualLength, expectedDirection));
                    if (trace.firstSpeed < 0.0F && fixedElapsed < scenarioHeldSeconds)
                        trace.firstSpeed = speed;
                    if (!sawRelease && fixedElapsed >= scenarioHeldSeconds) {
                        trace.firstReleaseSpeed = speed;
                        sawRelease = true;
                    }
                    const float turnRate = std::abs(std::remainder(state.facingYaw - previousYaw, 360.0F)) /
                        static_cast<float>(fixedDelta);
                    trace.maximumTurnRate = std::max(trace.maximumTurnRate, turnRate);
                    previousYaw = state.facingYaw;
                    trace.positions.push_back(state.position);
                    fixedElapsed += fixedDelta;
                    accumulator -= fixedDelta;
                }
            }
            trace.finalFacingYaw = controller.state().facingYaw;
            return trace;
        };

        std::array<std::vector<MoveTrace>, 3> tracesByFps;
        for (std::size_t fpsIndex = 0; fpsIndex < tracesByFps.size(); ++fpsIndex) {
            const int renderFps = fpsIndex == 0 ? 30 : (fpsIndex == 1 ? 60 : 144);
            auto& traces = tracesByFps[fpsIndex];
            for (const float cameraYaw : cameraYaws) {
                for (const auto& scenario : scenarios) {
                    MoveTrace trace = runScenario(renderFps, cameraYaw, scenario, movementWorld);
                    const float yawRadians = cameraYaw * 0.01745329251994329577F;
                    const glm::vec2 forward{std::cos(yawRadians), std::sin(yawRadians)};
                    const glm::vec2 right{-std::sin(yawRadians), std::cos(yawRadians)};
                    glm::vec2 expectedDirection{0.0F};
                    if (scenario.name[0] == 'W') expectedDirection += forward;
                    if (scenario.name[0] == 'S') expectedDirection -= forward;
                    if (scenario.name[0] == 'A') expectedDirection -= right;
                    if (scenario.name[0] == 'D') expectedDirection += right;
                    if (std::string(scenario.name) == "WA") expectedDirection = glm::normalize(forward - right);
                    const glm::vec3 finalPosition = trace.positions.back();
                    const float distance = glm::length(glm::vec2(finalPosition.x - playerSpawn->position.x,
                        finalPosition.z - playerSpawn->position.z));
                    if (std::abs(cameraYaw) < 1.0e-4F && std::string(scenario.name) == "W")
                        cardinalDistances[fpsIndex] = distance;
                    const float desiredYaw = std::atan2(expectedDirection.y, expectedDirection.x) *
                        57.29577951308232F;
                    const float facingError = std::abs(std::remainder(trace.finalFacingYaw - desiredYaw, 360.0F));
                    if (trace.firstSpeed >= targetSpeed * 0.95F || trace.firstReleaseSpeed <= 0.0F ||
                        trace.firstReleaseSpeed >= targetSpeed + 0.05F || trace.minimumDirectionDot < 0.985F ||
                        trace.maximumTurnRate > 720.05F || (scenario.name[0] == 'S' && facingError > 5.0F)) {
                        if (!failed) {
                            failed = true;
                            firstFailure = "fps=" + std::to_string(renderFps) + " yaw=" +
                                std::to_string(cameraYaw) + " scenario=" + scenario.name +
                                " firstSpeed=" + std::to_string(trace.firstSpeed) +
                                " releaseSpeed=" + std::to_string(trace.firstReleaseSpeed) +
                                " directionDot=" + std::to_string(trace.minimumDirectionDot) +
                                " turnRate=" + std::to_string(trace.maximumTurnRate) +
                                " facingError=" + std::to_string(facingError);
                        }
                    }
                    traces.push_back(std::move(trace));
                }
            }
        }

        const float distanceTolerance = 0.02F;
        if (std::abs(cardinalDistances[0] - cardinalDistances[1]) > distanceTolerance ||
            std::abs(cardinalDistances[1] - cardinalDistances[2]) > distanceTolerance) {
            failed = true;
            if (firstFailure.empty()) firstFailure = "render FPS changed fixed-step distance";
        }

        glory::game::CollisionWorld wallWorld;
        const float wallMinimumX = playerSpawn->position.x + 1.0F;
        wallWorld.addObstacle({{wallMinimumX, -1.0F, playerSpawn->position.z - 20.0F},
            {wallMinimumX + 0.5F, 3.0F, playerSpawn->position.z + 20.0F}, false, "move_test_wall", true});
        const MoveScenario diagonalWallScenario{"WA", {glory::input::Control::KeyW, glory::input::Control::KeyA}};
        const MoveTrace wallTrace = runScenario(60, 0.0F, diagonalWallScenario, wallWorld, 1.0);
        const glm::vec3 wallEnd = wallTrace.positions.back();
        const float wallStopX = wallMinimumX - 0.4F;
        if (wallEnd.x > wallStopX + 0.08F || wallEnd.z >= playerSpawn->position.z - 0.25F) {
            failed = true;
            if (firstFailure.empty()) firstFailure = "diagonal wall movement did not slide: end=(" +
                std::to_string(wallEnd.x) + "," + std::to_string(wallEnd.z) + ")";
        }
        controller.setCollisionWorld(collisionWorld);

        const auto worldPixel = [](const glm::vec3& position) {
            constexpr int imageSize = 1024;
            constexpr float scale = 42.0F;
            return glm::ivec2{imageSize / 2 + static_cast<int>(position.x * scale),
                imageSize / 2 + static_cast<int>(position.z * scale)};
        };
        const auto drawLine = [](std::vector<unsigned char>& image, glm::ivec2 start, glm::ivec2 end,
            std::array<unsigned char, 3> color) {
            constexpr int imageSize = 1024;
            const auto pixel = [&](glm::ivec2 point) {
                if (point.x < 0 || point.x >= imageSize || point.y < 0 || point.y >= imageSize) return;
                const std::size_t offset = (static_cast<std::size_t>(point.y) * imageSize + point.x) * 3;
                image[offset] = color[0]; image[offset + 1] = color[1]; image[offset + 2] = color[2];
            };
            const int dx = std::abs(end.x - start.x), sx = start.x < end.x ? 1 : -1;
            const int dy = -std::abs(end.y - start.y), sy = start.y < end.y ? 1 : -1;
            int error = dx + dy;
            for (;;) {
                pixel(start);
                if (start == end) break;
                const int twice = 2 * error;
                if (twice >= dy) { error += dy; start.x += sx; }
                if (twice <= dx) { error += dx; start.y += sy; }
            }
        };
        const std::array<std::array<unsigned char, 3>, 5> traceColors{{
            {40, 220, 255}, {255, 220, 40}, {255, 100, 80}, {110, 255, 120}, {230, 70, 230}}};
        for (std::size_t fpsIndex = 0; fpsIndex < tracesByFps.size(); ++fpsIndex) {
            const int renderFps = fpsIndex == 0 ? 30 : (fpsIndex == 1 ? 60 : 144);
            constexpr int imageSize = 1024;
            std::vector<unsigned char> image(imageSize * imageSize * 3, 18);
            for (const auto& box : collisionWorld.obstacles()) {
                const glm::ivec2 minimum = worldPixel({box.minimum.x, 0.0F, box.minimum.z});
                const glm::ivec2 maximum = worldPixel({box.maximum.x, 0.0F, box.maximum.z});
                const std::array<unsigned char, 3> color = box.blocksMovement
                    ? std::array<unsigned char, 3>{70, 180, 90} : std::array<unsigned char, 3>{60, 100, 180};
                drawLine(image, {minimum.x, minimum.y}, {maximum.x, minimum.y}, color);
                drawLine(image, {maximum.x, minimum.y}, {maximum.x, maximum.y}, color);
                drawLine(image, {maximum.x, maximum.y}, {minimum.x, maximum.y}, color);
                drawLine(image, {minimum.x, maximum.y}, {minimum.x, minimum.y}, color);
            }
            for (std::size_t traceIndex = 0; traceIndex < tracesByFps[fpsIndex].size(); ++traceIndex) {
                const auto& trace = tracesByFps[fpsIndex][traceIndex];
                const auto color = traceColors[traceIndex % traceColors.size()];
                for (std::size_t point = 1; point < trace.positions.size(); ++point)
                    drawLine(image, worldPixel(trace.positions[point - 1]), worldPixel(trace.positions[point]), color);
            }
            for (std::size_t point = 1; point < wallTrace.positions.size(); ++point)
                drawLine(image, worldPixel(wallTrace.positions[point - 1]), worldPixel(wallTrace.positions[point]),
                    {255, 40, 255});
            const auto pngPath = std::filesystem::path("captures/move-test") /
                ("move-test-" + std::to_string(renderFps) + ".png");
            stbi_write_png(pngPath.string().c_str(), imageSize, imageSize, 3, image.data(), imageSize * 3);
            nlohmann::json report{{"render_fps", renderFps}, {"passed", !failed},
                {"cardinal_distance", cardinalDistances[fpsIndex]}, {"traces", nlohmann::json::array()},
                {"wall_end", {wallEnd.x, wallEnd.y, wallEnd.z}}};
            for (const auto& trace : tracesByFps[fpsIndex]) {
                report["traces"].push_back({{"name", trace.name}, {"camera_yaw", trace.cameraYaw},
                    {"first_speed", trace.firstSpeed}, {"first_release_speed", trace.firstReleaseSpeed},
                    {"minimum_direction_dot", trace.minimumDirectionDot},
                    {"maximum_turn_rate", trace.maximumTurnRate}, {"final_facing_yaw", trace.finalFacingYaw},
                    {"positions", nlohmann::json::array()}});
                auto& positions = report["traces"].back()["positions"];
                for (const auto& position : trace.positions) positions.push_back({position.x, position.y, position.z});
            }
            const auto jsonPath = std::filesystem::path("captures/move-test") /
                ("move-test-" + std::to_string(renderFps) + ".json");
            std::ofstream jsonFile(jsonPath);
            jsonFile << report.dump(2) << '\n';
            std::cout << "--move-test " << renderFps << " FPS: " << (failed ? "FAIL " + firstFailure : "PASS")
                      << ", image=" << pngPath.string() << ", trace=" << jsonPath.string() << '\n';
        }
        if (failed) return 1;
    } else if (walkTestMode) {
        if (networkClient) {
            std::cerr << "--walk-test requires offline mode so it can exercise this process's live update callback.\n";
            return 2;
        }
        const auto* playerSpawn = worldEncounter.findSpawn(glory::game::SpawnEntityType::Player);
        if (!playerSpawn) {
            std::cerr << "--walk-test requires a player spawn in world_spawns.json.\n";
            return 1;
        }
        bool failed = false;
        std::string firstFailure;
        std::filesystem::create_directories("captures");
        for (const int renderFps : {30, 60, 144}) {
            std::vector<glm::vec3> playerTrail;
            std::vector<glm::vec3> bossTrail;
            std::uint64_t fixedStep = 0;
            const double fixedDelta = 1.0 / 60.0;
            for (int directionIndex = 0; directionIndex < 16; ++directionIndex) {
                controller.teleport(playerSpawn->position, playerSpawn->yaw);
                playerHealth = activeClass ? activeClass->baseHp : 150.0F;
                boss.reset();
                boss.setCollisionWorld(collisionWorld);
                boss.setHealth(149.0F);
                combat.resetEncounter();
                combat.dummy().hurtbox = {boss.position(), boss.hurtbox().radius, boss.hurtbox().height};
                combat.dummy().currentHp = boss.currentHp();
                bossEncounterActive = true;
                bossActivationDelayRemaining = 0.0F;

                double accumulator = 0.0;
                double elapsed = 0.0;
                const int renderFrames = renderFps * 20;
                for (int renderFrame = 0; renderFrame < renderFrames; ++renderFrame) {
                    const double frameDelta = 1.0 / static_cast<double>(renderFps);
                    accumulator += frameDelta;
                    const double frameTime = elapsed + frameDelta;
                    constexpr float sectorRadians = 0.39269908169872414F;
                    const float angle = static_cast<float>(directionIndex) * sectorRadians;
                    const glm::vec2 worldDirection{std::cos(angle), std::sin(angle)};
                    glory::input::InputFrame scriptedInput;
                    if (frameTime < 6.25) {
                        const float forward = -worldDirection.y;
                        const float right = worldDirection.x;
                        if (forward > 0.35F) scriptedInput.held.push_back(glory::input::Control::KeyW);
                        if (forward < -0.35F) scriptedInput.held.push_back(glory::input::Control::KeyS);
                        if (right > 0.35F) scriptedInput.held.push_back(glory::input::Control::KeyD);
                        if (right < -0.35F) scriptedInput.held.push_back(glory::input::Control::KeyA);
                    }
                    if (frameTime >= 5.9 && frameTime < 5.9 + frameDelta)
                        scriptedInput.pressed.push_back(glory::input::Control::KeyLeftShift);
                    while (accumulator + 1.0e-9 >= fixedDelta) {
                        const auto frame = fixedUpdate(fixedDelta, scriptedInput, -90.0F,
                            false, {}, {}, false);
                        const auto& player = controller.state();
                        const auto& bossPosition = boss.position();
                        playerTrail.push_back(player.position);
                        bossTrail.push_back(bossPosition);
                        ++fixedStep;
                        const auto validateActor = [&](const char* actorName, const glm::vec3& position,
                            const glory::game::Capsule& capsule) {
                            const glm::vec2 offset{position.x - collisionWorld.arenaCenter().x,
                                position.z - collisionWorld.arenaCenter().y};
                            const float radial = glm::length(offset);
                            const auto corrected = collisionWorld.resolve(capsule, position, {});
                            if (radial > collisionWorld.arenaRadius() + 0.5F + 0.02F ||
                                glm::distance(corrected.position, position) > 0.02F) {
                                std::string blocker = "arena boundary";
                                for (const auto& box : collisionWorld.obstacles()) {
                                    const float x = std::clamp(position.x, box.minimum.x, box.maximum.x);
                                    const float z = std::clamp(position.z, box.minimum.z, box.maximum.z);
                                    if (box.blocksMovement &&
                                        glm::length(glm::vec2{position.x - x, position.z - z}) < capsule.radius + 0.05F &&
                                        position.y < box.maximum.y && position.y + capsule.height > box.minimum.y) {
                                        blocker = box.name;
                                        break;
                                    }
                                }
                                if (!failed) {
                                    failed = true;
                                    firstFailure = "fps=" + std::to_string(renderFps) + " step=" +
                                        std::to_string(fixedStep) + " actor=" + actorName + " position=(" +
                                        std::to_string(position.x) + "," + std::to_string(position.y) + "," +
                                        std::to_string(position.z) + ") blocker=" + blocker;
                                }
                            }
                        };
                        validateActor("player", player.position, {player.position, 0.4F, 1.8F});
                        validateActor("boss", bossPosition,
                            {bossPosition, boss.hurtbox().radius, boss.hurtbox().height});
                        accumulator -= fixedDelta;
                    }
                    elapsed += frameDelta;
                }
            }

            std::array<std::vector<glm::vec3>, 4> collisionRouteTrails;
            std::array<std::string, 4> collisionRouteContacts;
            const auto runCollisionRoute = [&](std::size_t routeIndex, const glm::vec3& start, float cameraYaw,
                glory::input::Control moveControl, float seconds, const glm::vec3& bossStart) {
                auto& routeTrail = collisionRouteTrails[routeIndex];
                routeTrail.clear();
                routeTrail.push_back(start);
                controller.teleport(start, cameraYaw);
                playerHealth = activeClass ? activeClass->baseHp : 150.0F;
                boss.reset();
                boss.setCollisionWorld(collisionWorld);
                boss.setSpawnTransform(bossStart, 0.0F);
                boss.setHealth(0.0F);
                combat.resetEncounter();
                combat.dummy().hurtbox = {boss.position(), boss.hurtbox().radius, boss.hurtbox().height};
                combat.dummy().currentHp = boss.currentHp();
                bossEncounterActive = false;
                bossActivationDelayRemaining = 2.0F;

                double routeAccumulator = 0.0;
                double routeElapsed = 0.0;
                const int routeFrames = static_cast<int>(std::ceil(seconds * renderFps));
                for (int frameIndex = 0; frameIndex < routeFrames; ++frameIndex) {
                    const double frameDelta = 1.0 / static_cast<double>(renderFps);
                    routeAccumulator += frameDelta;
                    routeElapsed += frameDelta;
                    glory::input::InputFrame routeInput;
                    if (routeElapsed <= seconds) {
                        // The south spawn road has a blocking lamp; offset briefly before heading to the gate.
                        if (routeIndex == 1 && routeElapsed < 0.25)
                            routeInput.held.push_back(glory::input::Control::KeyD);
                        else
                            routeInput.held.push_back(moveControl);
                    }
                    while (routeAccumulator + 1.0e-9 >= fixedDelta) {
                        const auto frame = fixedUpdate(fixedDelta, routeInput, cameraYaw, false, {}, {}, false);
                        const glm::vec3 position = controller.state().position;
                        routeTrail.push_back(position);
                        if (frame.collisionCount > 0 && collisionRouteContacts[routeIndex].empty()) {
                            std::string blocker = "unknown";
                            for (const auto& box : collisionWorld.obstacles()) {
                                if (!box.blocksMovement || position.y >= box.maximum.y ||
                                    position.y + 1.8F <= box.minimum.y) continue;
                                const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
                                const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
                                if (glm::length(glm::vec2{position.x - closestX, position.z - closestZ}) <= 0.45F) {
                                    blocker = box.name;
                                    break;
                                }
                            }
                            collisionRouteContacts[routeIndex] = blocker + " at (" + std::to_string(position.x) +
                                "," + std::to_string(position.z) + ")";
                        }
                        routeAccumulator -= fixedDelta;
                    }
                }
                return controller.state().position;
            };
            const glm::vec3 northGateResult = runCollisionRoute(0, {0.0F, 0.0F, 0.0F}, 90.0F,
                glory::input::Control::KeyW, 10.0F, {0.0F, 0.0F, -30.0F});
            const glm::vec3 southGateResult = runCollisionRoute(1, {0.0F, 0.0F, 0.0F}, -90.0F,
                glory::input::Control::KeyW, 10.0F, {0.0F, 0.0F, 30.0F});
            const glm::vec3 westSpawnWallResult = runCollisionRoute(2, {0.0F, 0.0F, 0.0F}, 180.0F,
                glory::input::Control::KeyW, 5.0F, {32.0F, 0.0F, 0.0F});
            const glm::vec3 eastSpawnWallResult = runCollisionRoute(3, {0.0F, 0.0F, 0.0F}, 0.0F,
                glory::input::Control::KeyW, 5.0F, {-32.0F, 0.0F, 0.0F});
            const bool westHitHill = collisionRouteContacts[2].find("terrain_hill_green#") != std::string::npos;
            const bool eastHitHill = collisionRouteContacts[3].find("terrain_hill_green#") != std::string::npos;
            if (northGateResult.z < 36.0F || southGateResult.z > -36.0F ||
                westSpawnWallResult.x > -7.8F || westSpawnWallResult.x < -9.5F ||
                eastSpawnWallResult.x < 7.8F || eastSpawnWallResult.x > 9.5F || !westHitHill || !eastHitHill) {
                failed = true;
                firstFailure = "collision routes failed: northGate=(" + std::to_string(northGateResult.x) + "," +
                    std::to_string(northGateResult.z) + ") southGate=(" + std::to_string(southGateResult.x) + "," +
                    std::to_string(southGateResult.z) + ")" +
                    " westSpawnWallX=" + std::to_string(westSpawnWallResult.x) +
                    " eastSpawnWallX=" + std::to_string(eastSpawnWallResult.x) +
                    " contacts=[" + collisionRouteContacts[0] + ";" + collisionRouteContacts[1] + ";" +
                    collisionRouteContacts[2] + ";" + collisionRouteContacts[3] + "]";
            }

            constexpr int imageSize = 1024;
            constexpr float worldScale = 13.0F;
            std::vector<unsigned char> image(imageSize * imageSize * 3, 22);
            const auto pixel = [&](int x, int y, std::array<unsigned char, 3> color) {
                if (x < 0 || x >= imageSize || y < 0 || y >= imageSize) return;
                const std::size_t offset = (static_cast<std::size_t>(y) * imageSize + x) * 3;
                image[offset] = color[0]; image[offset + 1] = color[1]; image[offset + 2] = color[2];
            };
            const auto worldPixel = [](const glm::vec3& position) {
                return glm::ivec2{imageSize / 2 + static_cast<int>(position.x * worldScale),
                    imageSize / 2 + static_cast<int>(position.z * worldScale)};
            };
            const auto line = [&](glm::ivec2 a, glm::ivec2 b, std::array<unsigned char, 3> color) {
                const int dx = std::abs(b.x - a.x), sx = a.x < b.x ? 1 : -1;
                const int dy = -std::abs(b.y - a.y), sy = a.y < b.y ? 1 : -1;
                int error = dx + dy;
                for (;;) {
                    pixel(a.x, a.y, color);
                    if (a.x == b.x && a.y == b.y) break;
                    const int twice = 2 * error;
                    if (twice >= dy) { error += dy; a.x += sx; }
                    if (twice <= dx) { error += dx; a.y += sy; }
                }
            };
            for (int angle = 0; angle < 360; ++angle) {
                constexpr float degreesToRadians = 0.01745329251994329577F;
                const float radians = static_cast<float>(angle) * degreesToRadians;
                const auto a = worldPixel({collisionWorld.arenaCenter().x + collisionWorld.arenaRadius() *
                    std::cos(radians), 0.0F, collisionWorld.arenaCenter().y + collisionWorld.arenaRadius() *
                    std::sin(radians)});
                const float next = static_cast<float>(angle + 1) * degreesToRadians;
                const auto b = worldPixel({collisionWorld.arenaCenter().x + collisionWorld.arenaRadius() *
                    std::cos(next), 0.0F, collisionWorld.arenaCenter().y + collisionWorld.arenaRadius() *
                    std::sin(next)});
                line(a, b, {230, 230, 230});
            }
            for (const auto& box : collisionWorld.obstacles()) {
                const auto a = worldPixel({box.minimum.x, 0.0F, box.minimum.z});
                const auto b = worldPixel({box.maximum.x, 0.0F, box.maximum.z});
                const std::array<std::uint8_t, 3> color = box.blocksMovement
                    ? std::array<std::uint8_t, 3>{80, 190, 100}
                    : std::array<std::uint8_t, 3>{70, 130, 240};
                line({a.x, a.y}, {b.x, a.y}, color);
                line({b.x, a.y}, {b.x, b.y}, color);
                line({b.x, b.y}, {a.x, b.y}, color);
                line({a.x, b.y}, {a.x, a.y}, color);
            }
            constexpr std::array<std::array<std::uint8_t, 3>, 4> routeColors{{
                {40, 220, 255}, {255, 220, 40}, {255, 150, 40}, {230, 70, 230}}};
            for (std::size_t routeIndex = 0; routeIndex < collisionRouteTrails.size(); ++routeIndex)
                for (std::size_t index = 1; index < collisionRouteTrails[routeIndex].size(); ++index)
                    line(worldPixel(collisionRouteTrails[routeIndex][index - 1]),
                        worldPixel(collisionRouteTrails[routeIndex][index]), routeColors[routeIndex]);
            for (std::size_t i = 1; i < playerTrail.size(); ++i)
                line(worldPixel(playerTrail[i - 1]), worldPixel(playerTrail[i]), {40, 190, 255});
            for (std::size_t i = 1; i < bossTrail.size(); ++i)
                line(worldPixel(bossTrail[i - 1]), worldPixel(bossTrail[i]), {255, 85, 65});
            const std::string suffix = std::to_string(renderFps);
            const auto pngPath = std::filesystem::path("captures") / ("walk-test-" + suffix + ".png");
            const auto jsonPath = std::filesystem::path("captures") / ("walk-test-" + suffix + ".json");
            stbi_write_png(pngPath.string().c_str(), imageSize, imageSize, 3, image.data(), imageSize * 3);
            nlohmann::json trace{{"render_fps", renderFps}, {"fixed_steps", fixedStep},
                {"arena_center", {collisionWorld.arenaCenter().x, collisionWorld.arenaCenter().y}},
                {"arena_radius", collisionWorld.arenaRadius()}, {"player_positions", nlohmann::json::array()},
                {"boss_positions", nlohmann::json::array()},
                {"collision_routes", {{"north_gate", {northGateResult.x, northGateResult.z}},
                    {"south_gate", {southGateResult.x, southGateResult.z}},
                    {"west_spawn_wall_x", westSpawnWallResult.x},
                    {"east_spawn_wall_x", eastSpawnWallResult.x},
                    {"west_blocker", collisionRouteContacts[2]},
                    {"east_blocker", collisionRouteContacts[3]}}}};
            for (const auto& position : playerTrail)
                trace["player_positions"].push_back({position.x, position.y, position.z});
            for (const auto& position : bossTrail)
                trace["boss_positions"].push_back({position.x, position.y, position.z});
            trace["passed"] = !failed;
            trace["failure"] = failed ? firstFailure : "";
            std::ofstream jsonFile(jsonPath);
            jsonFile << trace.dump(2) << '\n';
            std::cout << "[walk-test routes] north gate z=" << northGateResult.z
                      << " south gate z=" << southGateResult.z
                      << " west wall x=" << westSpawnWallResult.x << " blocker=" << collisionRouteContacts[2]
                      << " east wall x=" << eastSpawnWallResult.x << " blocker=" << collisionRouteContacts[3] << '\n';
            std::cout << "--walk-test " << renderFps << " FPS: " << (failed ? "FAIL " + firstFailure : "PASS")
                      << ", fixed steps=" << fixedStep << ", trace=" << pngPath.string() << '\n';
        }
        if (failed) return 1;
    } else {
        result = glory::engine::runApplication(config, fixedUpdate);
    }

    if (result == 0 && config.smokeTestTickLimit != 0) {
        if (simulation.tickCount() != config.smokeTestTickLimit) {
            std::cerr << "Smoke test stopped at tick " << simulation.tickCount() << '\n';
            return 1;
        }
        std::cout << "SDL3/OpenGL 4.5 context and 120 fixed ticks passed\n";
    }

    return result;
}

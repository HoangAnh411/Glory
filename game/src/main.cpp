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
    const auto warriorModelPath = std::filesystem::path(GLORY_ASSET_DIR) / "models" / "hero_warrior.gltf";
    config.modelPath = std::filesystem::exists(warriorModelPath)
        ? warriorModelPath
        : std::filesystem::path(GLORY_ASSET_DIR) / "models" / "starter_crate.gltf";
    const auto bossModelPath = std::filesystem::path(GLORY_ASSET_DIR) / "models" / "boss_gorgon.gltf";
    config.bossModelPath = std::filesystem::exists(bossModelPath)
        ? bossModelPath
        : std::filesystem::path(GLORY_ASSET_DIR) / "models" / "static_crate.gltf";
    config.audioDirectory = std::filesystem::path(GLORY_ASSET_DIR) / "audio";
    config.skillsPath = std::filesystem::path(GLORY_ASSET_DIR) / "data" / "skills.json";
    config.worldSpawnsPath = std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_spawns.json";
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
        } else if (std::strcmp(argv[index], "--capture-no-hdr") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.captureHdrEnabled = false;
        } else if (std::strcmp(argv[index], "--capture-720p") == 0) {
            config.windowWidth = 1280;
            config.windowHeight = 720;
            config.captureMode = true;
            config.captureSuffix = "_1280x720";
        } else if (std::strcmp(argv[index], "--capture-high-dpi") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.highPixelDensity = true;
            config.captureSuffix = "_high_dpi";
        } else if (std::strcmp(argv[index], "--capture-colliders") == 0) {
            config.windowWidth = 1920;
            config.windowHeight = 1080;
            config.captureMode = true;
            config.captureCollisionDebug = true;
            config.captureSuffix = "_colliders";
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
            std::cerr << "Usage: glory_client [--smoke-test|--smoke-test-1080p|--capture|--capture-no-hdr|--capture-720p|--capture-high-dpi|--capture-colliders] [--connect IPv4:port]\n";
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
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "items.json");
    const auto allClassSkills = glory::game::SkillRegistry::loadFromFile(config.skillsPath);
    const auto classCatalog = glory::game::ClassCatalog::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "classes.json", allClassSkills);
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
    const auto collisionWorld = glory::game::CollisionWorld::loadFromFiles(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_map.json",
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_colliders.json");
    controller.setCollisionWorld(collisionWorld);
    config.arenaCenter = collisionWorld.arenaCenter();
    config.arenaRadius = collisionWorld.arenaRadius();
    for (const auto& box : collisionWorld.obstacles())
        config.collisionDebugBoxes.push_back({box.minimum, box.maximum});
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
    bool bossEncounterActive = false;
    float bossActivationDelayRemaining = 2.0F;
    glory::game::SkillRegistry skillRegistry = glory::game::SkillRegistry::loadFromFile(config.skillsPath);
    config.editorSkills = toEditorSkills(skillRegistry);
    glory::game::CombatSystem combat(std::move(skillRegistry),
        glory::game::CombatTarget{8, {boss.position(), 0.55F, 2.2F}, 300.0F, 300.0F, 0});
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

    const auto worldMapPath = std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_map.json";
    if (std::filesystem::exists(worldMapPath)) {
        try {
            std::ifstream file(worldMapPath);
            nlohmann::json root;
            file >> root;
            if (root.contains("props") && root["props"].is_array()) {
                for (const auto& item : root["props"]) {
                    glm::vec3 pos{0.0F};
                    if (item.contains("position") && item["position"].is_array() && item["position"].size() >= 3) {
                        pos = {item["position"][0].get<float>(), item["position"][1].get<float>(), item["position"][2].get<float>()};
                    }
                    if (item.contains("collision") && item["collision"].is_array() && item["collision"].size() >= 6) {
                        const auto& c = item["collision"];
                        glory::game::Aabb aabb;
                        aabb.minimum = pos + glm::vec3{c[0].get<float>(), c[1].get<float>(), c[2].get<float>()};
                        aabb.maximum = pos + glm::vec3{c[3].get<float>(), c[4].get<float>(), c[5].get<float>()};
                        controller.addObstacle(aabb);
                    }
                    if (item.contains("multi_collision") && item["multi_collision"].is_array()) {
                        for (const auto& c : item["multi_collision"]) {
                            if (c.is_array() && c.size() >= 6) {
                                glory::game::Aabb aabb;
                                aabb.minimum = pos + glm::vec3{c[0].get<float>(), c[1].get<float>(), c[2].get<float>()};
                                aabb.maximum = pos + glm::vec3{c[3].get<float>(), c[4].get<float>(), c[5].get<float>()};
                                controller.addObstacle(aabb);
                            }
                        }
                    }
                }
            }
        } catch (...) {
        }
    }
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
    const int result = glory::engine::runApplication(config, [&](
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
        }
        const float activeMaxHp = activeClass ? activeClass->baseHp : 100.0F;
        playerEffects.update(static_cast<float>(fixedDeltaSeconds), playerHealth, activeMaxHp);
        if (playerHealth > 0.0F && !combat.isHitStopped())
            controller.update(actionMap, cameraYawDegrees, fixedDeltaSeconds, combat.dodgeAllowed(),
                playerEffects.calculateSpeedMultiplier(), playerEffects.isStunned());
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
            worldEncounter.updateTriggers(respawned.position);
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
    });

    if (result == 0 && config.smokeTestTickLimit != 0) {
        if (simulation.tickCount() != config.smokeTestTickLimit) {
            std::cerr << "Smoke test stopped at tick " << simulation.tickCount() << '\n';
            return 1;
        }
        std::cout << "SDL3/OpenGL 4.5 context and 120 fixed ticks passed\n";
    }

    return result;
}

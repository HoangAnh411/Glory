#pragma once

#include "glory/input/input_frame.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace glory::engine {

struct SkillDebugState {
    std::string id;
    std::string name;
    float cooldownRemaining = 0.0F;
    float cooldownDuration = 0.0F;
};

struct CombatHitVisualEvent {
    glm::vec3 position{0.0F};
    float damage = 0.0F;
};

struct StatusEffectDebugState {
    std::string name;
    float remainingTime = 0.0F;
    unsigned int stackCount = 1;
};

struct SkillEditorEffect {
    std::string id;
    std::string name;
    std::string type = "Bleed";
    float duration = 1.0F;
    float magnitude = 1.0F;
    float tickInterval = 0.0F;
    unsigned int maxStacks = 1;
};

struct SkillEditorEvent {
    float time = 0.0F;
    std::string type = "sound";
    std::string name;
};

struct SkillEditorSkill {
    std::string id;
    std::string name;
    float damage = 1.0F;
    float cooldown = 0.0F;
    float castTime = 0.0F;
    std::string animationState;
    std::string comboNext;
    float comboWindow = 0.3F;
    float inputBuffer = 0.15F;
    float cancelWindowStart = 0.3F;
    float hitStopDuration = 0.06F;
    float knockbackSpeed = 0.0F;
    float knockbackDuration = 0.0F;
    float hitboxRadius = 0.5F;
    glm::vec3 hitboxOffset{0.0F};
    float activeStart = 0.0F;
    float activeEnd = 0.1F;
    std::vector<SkillEditorEffect> onHitEffects;
    std::vector<SkillEditorEvent> animationEvents;
};

using SaveSkillsCallback = std::function<bool(const std::vector<SkillEditorSkill>&, std::string&)>;
using ReloadSkillsCallback = std::function<bool(std::vector<SkillEditorSkill>&, std::string&)>;

struct WorldEditorSpawnPoint {
    std::string id;
    std::string name;
    std::string entityType = "Player";
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
};

struct WorldEditorTriggerZone {
    std::string id;
    std::string name;
    std::string shape = "Box";
    glm::vec3 center{0.0F};
    glm::vec3 extents{1.0F};
    float radius = 1.0F;
    std::string triggerType = "EncounterBoundary";
};

using SaveWorldCallback = std::function<bool(const std::vector<WorldEditorSpawnPoint>&,
    const std::vector<WorldEditorTriggerZone>&, std::string&)>;
using ReloadWorldCallback = std::function<bool(std::vector<WorldEditorSpawnPoint>&,
    std::vector<WorldEditorTriggerZone>&, std::string&)>;
using MatchmakingCallback = std::function<bool(unsigned int, bool)>;

enum class AccountAction { Register, Login, RefreshCharacters, CreateCharacter, EnterWorld };
struct AccountCharacterView {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    int level = 1;
    int exp = 0;
};
struct ClassChoiceView {
    std::string name;
    std::string description;
    float hp = 100.0F;
    float attack = 10.0F;
    float defense = 0.0F;
    float speed = 5.5F;
    float dodgeCooldown = 0.8F;
    std::vector<std::string> skills;
};
struct AccountPanelState {
    std::string username;
    std::string password;
    std::string characterName;
    std::string className = "Warrior";
    std::string sessionToken;
    std::string message;
    std::vector<AccountCharacterView> characters;
    std::vector<ClassChoiceView> classChoices;
    int selectedCharacter = -1;
    bool authenticated = false;
    bool messageIsError = false;
};
using AccountActionCallback = std::function<bool(AccountAction, const AccountPanelState&)>;

struct FeedbackPanelState {
    int category = 0;
    int rating = 5;
    std::string subject;
    std::string description;
    std::string message;
    bool open = false;
};
using FeedbackSubmitCallback = std::function<bool(const FeedbackPanelState&, const std::string&)>;

enum class InventoryAction { Equip, Unequip, SimulateBossLoot, PickupLoot };
struct InventoryPanelSlot { std::string itemId; std::string name; int quantity = 0; };
struct EquipmentPanelItem { int slotType = 0; std::string itemId; std::string name; };
struct WorldLootPanelItem { std::uint64_t lootId = 0; std::string itemId; std::string name; int quantity = 0; glm::vec3 position{0.0F}; };
struct InventoryPanelState {
    std::int64_t characterId = 0;
    std::string sessionToken;
    std::vector<InventoryPanelSlot> slots;
    std::vector<EquipmentPanelItem> equipment;
    std::vector<WorldLootPanelItem> drops;
    int selectedInventorySlot = -1;
    int selectedEquipmentSlot = 0;
    std::uint64_t selectedLootId = 0;
    int baseAttack = 10;
    int baseDefense = 5;
    int baseMaxHp = 100;
    int bonusAttack = 0;
    int bonusDefense = 0;
    int bonusMaxHp = 0;
    std::string message;
    bool messageIsError = false;
};
using InventoryActionCallback = std::function<bool(InventoryAction, const InventoryPanelState&)>;

struct ZonePanelState {
    std::int64_t characterId = 0;
    std::string sessionToken;
    std::string currentZone = "Zone1_Plains";
    unsigned int totalEntityCount = 0;
    unsigned int visibleEntityCount = 0;
    unsigned int pingRttMs = 0;
};
using ZoneChangeCallback = std::function<bool(std::uint8_t, const ZonePanelState&)>;

enum class SocialAction {
    SendChat, PartyCreate, PartyInvite, PartyAccept, PartyLeave, PartyKick,
    GuildCreate, GuildInvite, GuildAccept, GuildLeave, GuildSetRank, GuildRefresh
};
struct SocialChatLine { unsigned int channel = 0; std::string senderName; std::string text; };
struct SocialPartyMember {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    int level = 1;
    float currentHp = 0.0F;
    float maxHp = 100.0F;
    bool leader = false;
};
struct SocialGuildMember {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    int level = 1;
    unsigned int rank = 2;
};
struct SocialPanelState {
    std::int64_t characterId = 0;
    std::string sessionToken;
    int selectedChannel = 0;
    std::string targetName;
    std::string chatInput;
    std::string guildNameInput;
    std::string message;
    std::vector<SocialChatLine> chatHistory;
    std::vector<SocialPartyMember> partyMembers;
    std::vector<SocialGuildMember> guildMembers;
    std::uint64_t partyId = 0;
    std::uint64_t guildId = 0;
    std::string guildName;
    std::int64_t selectedMemberId = 0;
    unsigned int memberRank = 2;
    bool partyInvitePending = false;
    bool guildInvitePending = false;
};
using SocialActionCallback = std::function<bool(SocialAction, const SocialPanelState&)>;

enum class DungeonAction { EnterSolo, EnterParty, Leave };
struct DungeonPanelState {
    std::int64_t characterId = 0;
    std::string sessionToken;
    std::uint32_t selectedDungeonId = 1;
    std::uint64_t instanceId = 0;
    std::string dungeonName = "The Shadow Crypt";
    float timerRemaining = 0.0F;
    float bossHealth = 0.0F;
    float bossMaxHealth = 0.0F;
    bool inDungeon = false;
    bool isCompleted = false;
    bool isFailed = false;
    std::string message;
};
using DungeonActionCallback = std::function<bool(DungeonAction, const DungeonPanelState&)>;

enum class MarketAction { Browse, List, Buy, Cancel };
struct MarketPanelListing {
    std::int64_t listingId = 0;
    std::string sellerName;
    std::string itemId;
    std::string itemName;
    std::string itemType;
    int quantity = 0;
    std::int64_t unitPrice = 0;
};
struct MarketPanelState {
    std::int64_t characterId = 0;
    std::string sessionToken;
    std::int64_t gold = 0;
    std::string itemType;
    std::string search;
    std::string message;
    int selectedSlot = -1;
    int quantity = 1;
    int unitPrice = 1;
    std::uint32_t page = 0;
    std::int64_t selectedListing = 0;
    std::vector<MarketPanelListing> listings;
};
using MarketActionCallback = std::function<bool(MarketAction, const MarketPanelState&)>;

struct PlayerFrameState {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    float facingYaw = -90.0F;
    bool isGrounded = true;
    bool isMoving = false;
    bool attackRequested = false;
    bool isDodging = false;
    bool isInvulnerable = false;
    float dodgeCooldownRemaining = 0.0F;
    unsigned int collisionCount = 0;
    float dummyHp = 100.0F;
    float dummyMaxHp = 100.0F;
    unsigned int dummyHitCount = 0;
    unsigned int comboStep = 1;
    bool comboWindowOpen = false;
    bool cancelWindowAvailable = false;
    bool hitStopActive = false;
    bool attackCanceled = false;
    float hitStopRemaining = 0.0F;
    glm::vec3 dummyKnockbackVelocity{0.0F};
    bool playerStunned = false;
    std::vector<StatusEffectDebugState> playerEffects;
    std::vector<StatusEffectDebugState> dummyEffects;
    float playerHp = 100.0F;
    float playerMaxHp = 100.0F;
    bool playerDead = false;
    float bossHp = 300.0F;
    float bossMaxHp = 300.0F;
    std::string bossName;
    std::string bossPhase;
    std::string bossBehaviorNode;
    std::string bossAttackName;
    bool bossEnrageTriggered = false;
    bool bossAttackHit = false;
    bool bossDefeated = false;
    glm::vec3 bossPosition{0.0F};
    float bossYaw = 0.0F;
    std::vector<SkillDebugState> skills;
    std::string className;
    std::vector<CombatHitVisualEvent> hitEvents;
    bool targetLocked = false;
    glm::vec3 targetLockPosition{0.0F};
    std::vector<std::string> activeTriggerStates;
    glm::vec3 practiceDummyPosition{0.0F};
    float practiceDummyYaw = 0.0F;
    bool networkConnected = false;
    std::uint64_t serverTick = 0;
    unsigned int serverPlayerCount = 0;
    std::uint32_t authoritativeClientId = 0;
    glm::vec3 authoritativePosition{0.0F};
    float authoritativeHp = 0.0F;
    float authoritativeMaxHp = 0.0F;
    std::string authoritativeAnimation;
    std::string authoritativeSkill;
    std::uint32_t clientInputSequence = 0;
    std::uint32_t serverAckSequence = 0;
    unsigned int unacknowledgedInputCount = 0;
    std::uint64_t reconciliationCount = 0;
    float mispredictionOffset = 0.0F;
    struct RemotePlayer {
        std::uint32_t clientId = 0;
        glm::vec3 position{0.0F};
        float yaw = 0.0F;
        float currentHp = 0.0F;
        float maxHp = 0.0F;
        float distance = 0.0F;
        std::string animationState;
        std::string activeSkillId;
        unsigned int team = 0;
    };
    std::vector<RemotePlayer> remotePlayers;
    unsigned int matchState = 0;
    unsigned int matchMode = 1;
    unsigned int localTeam = 0;
    unsigned int winningTeam = 0;
    float countdownRemaining = 0.0F;
    float antiCheatCurrentSpeed = 0.0F;
    float antiCheatMaximumSpeed = 0.0F;
    unsigned int speedViolations = 0;
    unsigned int teleportViolations = 0;
    unsigned int collisionViolations = 0;
    unsigned int cooldownViolations = 0;
    std::string antiCheatMessage;
};

struct ApplicationConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    bool hiddenWindow = false;
    std::uint64_t smokeTestTickLimit = 0;
    bool captureMode = false;
    std::filesystem::path modelPath;
    std::filesystem::path bossModelPath;
    std::filesystem::path audioDirectory;
    std::filesystem::path skillsPath;
    std::vector<SkillEditorSkill> editorSkills;
    SaveSkillsCallback saveSkills;
    ReloadSkillsCallback reloadSkills;
    std::filesystem::path worldSpawnsPath;
    std::vector<WorldEditorSpawnPoint> worldSpawns;
    std::vector<WorldEditorTriggerZone> worldTriggers;
    SaveWorldCallback saveWorld;
    ReloadWorldCallback reloadWorld;
    MatchmakingCallback matchmaking;
    AccountPanelState* accountPanel = nullptr;
    AccountActionCallback accountAction;
    InventoryPanelState* inventoryPanel = nullptr;
    InventoryActionCallback inventoryAction;
    ZonePanelState* zonePanel = nullptr;
    ZoneChangeCallback zoneChange;
    SocialPanelState* socialPanel = nullptr;
    SocialActionCallback socialAction;
    DungeonPanelState* dungeonPanel = nullptr;
    DungeonActionCallback dungeonAction;
    MarketPanelState* marketPanel = nullptr;
    MarketActionCallback marketAction;
    FeedbackPanelState* feedbackPanel = nullptr;
    FeedbackSubmitCallback feedbackSubmit;
};

using FixedUpdateCallback = std::function<PlayerFrameState(double, const input::InputFrame&, float, bool,
    const std::string&, const std::string&)>;

int runApplication(const ApplicationConfig& config, const FixedUpdateCallback& fixedUpdate);

} // namespace glory::engine

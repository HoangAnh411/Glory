#pragma once

#include <cstdint>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

struct _ENetHost;
struct _ENetPeer;

namespace glory::network {

enum class PacketType : std::uint8_t {
    ConnectRequest = 1,
    ConnectAccepted,
    Ping,
    Pong,
    ClientInput,
    ServerHeartbeat,
    ServerSnapshot,
    MatchmakingRequest,
    RegisterRequest,
    RegisterResponse,
    LoginRequest,
    LoginResponse,
    CharacterListRequest,
    CharacterListResponse,
    CreateCharacterRequest,
    CreateCharacterResponse,
    EnterWorldRequest,
    EnterWorldResponse,
    InventorySync,
    EquipmentSync,
    EquipItemRequest,
    UnequipItemRequest,
    WorldLootSync,
    PickupLootRequest,
    SimulateBossLootDropRequest,
    InventoryActionResponse,
    ZoneChangeRequest,
    ZoneChangeNotification,
    ChatMessage,
    PartyRequest,
    PartySync,
    GuildRequest,
    GuildSync,
    EnterDungeonRequest,
    EnterDungeonResponse,
    LeaveDungeonRequest,
    DungeonStateSync,
    MarketBrowseRequest,
    MarketBrowseResponse,
    MarketListRequest,
    MarketListResponse,
    MarketBuyRequest,
    MarketBuyResponse,
    MarketCancelRequest,
    MarketCancelResponse,
    CharacterGoldSync,
    AntiCheatWarning,
    AntiCheatProbe,
    SkillRequest,
    FeedbackSubmitRequest,
    FeedbackSubmitResponse
};

enum class FeedbackCategory : std::uint8_t { Bug, Balance, Suggestion, Praise };
enum class FeedbackSubmitStatus : std::uint8_t { Success, InvalidLength, RateLimited };
struct FeedbackSubmitRequestPacket {
    FeedbackCategory category = FeedbackCategory::Bug;
    std::uint8_t rating = 0;
    std::string subject;
    std::string description;
    std::string diagnosticsJson;
};
struct FeedbackSubmitResponsePacket { FeedbackSubmitStatus status = FeedbackSubmitStatus::InvalidLength; };

enum class AntiCheatViolationCode : std::uint8_t { Speed, Teleport, Collision, Cooldown, Kick, Telemetry };
struct AntiCheatWarningPacket {
    AntiCheatViolationCode violation = AntiCheatViolationCode::Speed;
    std::uint32_t speedViolations = 0;
    std::uint32_t teleportViolations = 0;
    std::uint32_t collisionViolations = 0;
    std::uint32_t cooldownViolations = 0;
    float currentSpeed = 0.0F;
    float maximumSpeed = 0.0F;
    bool kick = false;
};
enum class AntiCheatProbeKind : std::uint8_t { PositionClaim };
struct AntiCheatProbePacket {
    AntiCheatProbeKind kind = AntiCheatProbeKind::PositionClaim;
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

enum class AuthResult : std::uint8_t {
    Success, UsernameTaken, InvalidLength, BadCredentials, AccountNotFound,
    InvalidSession, CharacterNameTaken, CharacterNotFound, CharacterLimitReached, InvalidClass, ServerError
};

struct RegisterRequestPacket { std::string username; std::string password; };
struct RegisterResponsePacket { AuthResult result = AuthResult::ServerError; };
struct LoginRequestPacket { std::string username; std::string password; };
struct LoginResponsePacket { AuthResult result = AuthResult::ServerError; std::string sessionToken; };
struct CharacterInfoPacket {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    std::int32_t level = 1;
    std::int32_t exp = 0;
};
struct CharacterListRequestPacket { std::string sessionToken; };
struct CharacterListResponsePacket { AuthResult result = AuthResult::ServerError; std::vector<CharacterInfoPacket> characters; };
struct CreateCharacterRequestPacket { std::string sessionToken; std::string name; std::string className; };
struct CreateCharacterResponsePacket { AuthResult result = AuthResult::ServerError; CharacterInfoPacket character; };
struct EnterWorldRequestPacket { std::string sessionToken; std::int64_t characterId = 0; };
struct EnterWorldResponsePacket { AuthResult result = AuthResult::ServerError; CharacterInfoPacket character; };

enum class EquipmentSlotPacket : std::uint8_t { Weapon, Armor, Accessory };
enum class InventoryActionResult : std::uint8_t {
    Success, InvalidSession, CharacterNotSelected, InvalidItem, InvalidSlot,
    InventoryFull, TooFar, LootNotFound, ServerError
};
struct InventorySlotPacket { std::uint8_t slotIndex = 0; std::string itemId; std::int32_t quantity = 0; };
struct InventorySyncPacket { std::int64_t characterId = 0; std::vector<InventorySlotPacket> items; };
struct EquipmentItemPacket { EquipmentSlotPacket slot = EquipmentSlotPacket::Weapon; std::string itemId; };
struct EquipmentSyncPacket {
    std::int64_t characterId = 0;
    std::vector<EquipmentItemPacket> items;
    std::int32_t bonusAttack = 0;
    std::int32_t bonusDefense = 0;
    std::int32_t bonusMaxHp = 0;
};
struct EquipItemRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    std::uint8_t inventorySlot = 0;
    EquipmentSlotPacket slot = EquipmentSlotPacket::Weapon;
};
struct UnequipItemRequestPacket { std::string sessionToken; std::int64_t characterId = 0; EquipmentSlotPacket slot = EquipmentSlotPacket::Weapon; };
struct WorldLootEntryPacket {
    std::uint64_t lootId = 0;
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    std::string itemId;
    std::int32_t quantity = 0;
};
struct WorldLootSyncPacket { std::vector<WorldLootEntryPacket> drops; };
struct PickupLootRequestPacket { std::string sessionToken; std::int64_t characterId = 0; std::uint64_t lootId = 0; };
struct SimulateBossLootDropRequestPacket { std::string sessionToken; std::int64_t characterId = 0; };
struct InventoryActionResponsePacket { InventoryActionResult result = InventoryActionResult::ServerError; };

enum class MatchmakingMode : std::uint8_t { Duel1v1 = 1, Team3v3 = 2 };
enum class MatchmakingAction : std::uint8_t { Join = 1, Leave = 2 };
enum class ArenaMatchState : std::uint8_t { Waiting = 0, Countdown = 1, InRound = 2, MatchFinished = 3 };
enum class ArenaTeam : std::uint8_t { None = 0, Red = 1, Blue = 2 };
enum class ZoneId : std::uint8_t { Zone1Plains, Zone2Arena, Zone3Forest };
struct ZoneChangeRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    ZoneId targetZone = ZoneId::Zone1Plains;
};
struct ZoneChangeNotificationPacket {
    ZoneId currentZone = ZoneId::Zone1Plains;
    float spawnX = 0.0F;
    float spawnY = 0.0F;
    float spawnZ = 0.0F;
};

enum class ChatChannel : std::uint8_t { World, Zone, Party, Guild, Whisper, System };
struct ChatMessagePacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    ChatChannel channel = ChatChannel::World;
    std::string senderName;
    std::string targetName;
    std::string text;
};

enum class PartyAction : std::uint8_t { Create, Invite, Accept, Leave, Kick, Refresh };
enum class SocialResult : std::uint8_t {
    Success, InvalidSession, CharacterNotSelected, InvalidTarget, NotLeader,
    AlreadyMember, NoInvitation, PartyFull, GuildExists, AlreadyInGuild,
    NotInGuild, InvalidRank, NotFound, ServerError
};
struct PartyRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    PartyAction action = PartyAction::Create;
    std::string targetName;
};
struct PartyMemberPacket {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    std::int32_t level = 1;
    float currentHp = 100.0F;
    float maxHp = 100.0F;
    bool leader = false;
};
struct PartySyncPacket {
    SocialResult result = SocialResult::Success;
    std::uint64_t partyId = 0;
    bool invitationPending = false;
    std::vector<PartyMemberPacket> members;
};

enum class GuildAction : std::uint8_t { Create, Invite, AcceptInvite, Leave, SetRank, Refresh };
enum class GuildRank : std::uint8_t { Leader, Officer, Member };
struct GuildRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    GuildAction action = GuildAction::Refresh;
    std::uint64_t guildId = 0;
    std::string guildName;
    std::string targetName;
    GuildRank rank = GuildRank::Member;
};
struct GuildMemberPacket {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    std::int32_t level = 1;
    GuildRank rank = GuildRank::Member;
};
struct GuildSyncPacket {
    SocialResult result = SocialResult::Success;
    std::uint64_t guildId = 0;
    std::string guildName;
    GuildRank memberRank = GuildRank::Member;
    bool invitationPending = false;
    std::vector<GuildMemberPacket> members;
};

enum class DungeonResult : std::uint8_t {
    Success, InvalidSession, CharacterNotSelected, UnknownDungeon, AlreadyInDungeon,
    NotInParty, PartyUnavailable, InvalidState, ServerError
};
struct EnterDungeonRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    std::uint32_t dungeonId = 1;
    bool enterParty = false;
};
struct EnterDungeonResponsePacket {
    DungeonResult result = DungeonResult::ServerError;
    std::uint32_t dungeonId = 0;
    std::uint64_t instanceId = 0;
};
struct LeaveDungeonRequestPacket { std::string sessionToken; std::int64_t characterId = 0; };
struct DungeonStateSyncPacket {
    DungeonResult result = DungeonResult::Success;
    std::uint32_t dungeonId = 0;
    std::uint64_t instanceId = 0;
    float timerRemaining = 0.0F;
    bool isCompleted = false;
    bool isFailed = false;
    float bossHealth = 0.0F;
    float bossMaxHealth = 0.0F;
};

enum class MarketResultPacket : std::uint8_t {
    Success, InvalidSession, CharacterNotSelected, InvalidItem, InvalidQuantity,
    InvalidPrice, InsufficientFunds, InventoryFull, ListingNotFound, NotSeller, ServerError
};
struct MarketListingPacket {
    std::int64_t listingId = 0;
    std::int64_t sellerCharacterId = 0;
    std::string sellerName;
    std::string itemId;
    std::string itemName;
    std::string itemType;
    std::int32_t quantity = 0;
    std::int64_t unitPrice = 0;
};
struct MarketBrowseRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    std::string itemType;
    std::string search;
    std::uint32_t page = 0;
    std::uint32_t pageSize = 20;
};
struct MarketBrowseResponsePacket {
    MarketResultPacket result = MarketResultPacket::Success;
    std::vector<MarketListingPacket> listings;
};
struct MarketListRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    std::string itemId;
    std::int32_t quantity = 0;
    std::int64_t unitPrice = 0;
};
struct MarketActionRequestPacket {
    std::string sessionToken;
    std::int64_t characterId = 0;
    std::int64_t listingId = 0;
};
struct MarketActionResponsePacket {
    MarketResultPacket result = MarketResultPacket::ServerError;
    std::int64_t gold = 0;
};
struct CharacterGoldSyncPacket { std::int64_t characterId = 0; std::int64_t gold = 0; };

struct MatchmakingRequestPacket {
    MatchmakingMode mode = MatchmakingMode::Duel1v1;
    MatchmakingAction action = MatchmakingAction::Join;
};

enum class ConnectionState { Disconnected, Connecting, Connected };
enum class EventType { Connected, PacketReceived, Disconnected };

struct ClientInputPacket {
    std::uint32_t pressedButtons = 0;
    std::uint32_t heldButtons = 0;
    float cameraYawDegrees = 0.0F;
    std::uint32_t inputSequence = 0;
    std::uint64_t targetTick = 0;
};

struct SnapshotVector3 { float x = 0.0F; float y = 0.0F; float z = 0.0F; };

struct SnapshotPlayer {
    std::uint32_t clientId = 0;
    SnapshotVector3 position;
    SnapshotVector3 velocity;
    float yaw = 0.0F;
    float currentHp = 100.0F;
    float maxHp = 100.0F;
    std::string animationState = "Idle";
    bool isDodging = false;
    bool isInvulnerable = false;
    std::string activeSkillId;
    std::uint32_t lastProcessedInputSequence = 0;
    bool isGrounded = true;
    float dodgeElapsed = 0.0F;
    float dodgeCooldownRemaining = 0.0F;
    SnapshotVector3 dodgeDirection;
    SnapshotVector3 externalKnockbackVelocity;
    float knockbackRemaining = 0.0F;
    ArenaTeam team = ArenaTeam::None;
};

struct SnapshotBoss {
    SnapshotVector3 position;
    float currentHp = 0.0F;
    float maxHp = 0.0F;
    std::string phase;
};

struct ServerSnapshotPacket {
    std::uint64_t serverTick = 0;
    ZoneId currentZone = ZoneId::Zone1Plains;
    std::uint16_t totalEntityCount = 0;
    std::uint16_t visibleEntityCount = 0;
    ArenaMatchState matchState = ArenaMatchState::Waiting;
    MatchmakingMode matchMode = MatchmakingMode::Duel1v1;
    float countdownRemaining = 0.0F;
    ArenaTeam winningTeam = ArenaTeam::None;
    std::vector<SnapshotPlayer> players;
    SnapshotBoss boss;
};

struct InterpolatedRemotePlayer {
    SnapshotPlayer state;
    float distanceFromLocal = 0.0F;
};

class SnapshotInterpolator {
public:
    explicit SnapshotInterpolator(std::size_t capacity = 32, double interpolationDelayMs = 75.0,
        double maxExtrapolationMs = 100.0);

    bool addSnapshot(const ServerSnapshotPacket& snapshot, double receivedAtMilliseconds);
    [[nodiscard]] std::vector<InterpolatedRemotePlayer> sample(std::uint32_t localClientId,
        const SnapshotVector3& localPosition, double nowMilliseconds) const;
    void clear() noexcept;
    [[nodiscard]] std::size_t bufferedSnapshotCount() const noexcept { return snapshots_.size(); }
    [[nodiscard]] static double monotonicMilliseconds() noexcept;

private:
    struct TimedSnapshot {
        ServerSnapshotPacket packet;
        double receivedAtMilliseconds = 0.0;
    };

    std::vector<TimedSnapshot> snapshots_;
    std::size_t capacity_ = 32;
    double interpolationDelayMs_ = 75.0;
    double maxExtrapolationMs_ = 100.0;
};

[[nodiscard]] std::array<std::uint8_t, 24> encodeClientInput(const ClientInputPacket& input);
[[nodiscard]] bool decodeClientInput(std::span<const std::uint8_t> bytes, ClientInputPacket& input);
[[nodiscard]] std::array<std::uint8_t, 26> encodeAntiCheatWarning(const AntiCheatWarningPacket& packet);
[[nodiscard]] bool decodeAntiCheatWarning(std::span<const std::uint8_t> bytes, AntiCheatWarningPacket& packet);
[[nodiscard]] std::array<std::uint8_t, 13> encodeAntiCheatProbe(const AntiCheatProbePacket& packet);
[[nodiscard]] bool decodeAntiCheatProbe(std::span<const std::uint8_t> bytes, AntiCheatProbePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeSkillRequest(const std::string& skillId);
[[nodiscard]] bool decodeSkillRequest(std::span<const std::uint8_t> bytes, std::string& skillId);
[[nodiscard]] std::array<std::uint8_t, 2> encodeMatchmakingRequest(const MatchmakingRequestPacket& request);
[[nodiscard]] bool decodeMatchmakingRequest(std::span<const std::uint8_t> bytes,
    MatchmakingRequestPacket& request) noexcept;
[[nodiscard]] std::vector<std::uint8_t> encodeRegisterRequest(const RegisterRequestPacket& packet);
[[nodiscard]] bool decodeRegisterRequest(std::span<const std::uint8_t> bytes, RegisterRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeRegisterResponse(const RegisterResponsePacket& packet);
[[nodiscard]] bool decodeRegisterResponse(std::span<const std::uint8_t> bytes, RegisterResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeLoginRequest(const LoginRequestPacket& packet);
[[nodiscard]] bool decodeLoginRequest(std::span<const std::uint8_t> bytes, LoginRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeLoginResponse(const LoginResponsePacket& packet);
[[nodiscard]] bool decodeLoginResponse(std::span<const std::uint8_t> bytes, LoginResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeCharacterListRequest(const CharacterListRequestPacket& packet);
[[nodiscard]] bool decodeCharacterListRequest(std::span<const std::uint8_t> bytes, CharacterListRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeCharacterListResponse(const CharacterListResponsePacket& packet);
[[nodiscard]] bool decodeCharacterListResponse(std::span<const std::uint8_t> bytes, CharacterListResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeCreateCharacterRequest(const CreateCharacterRequestPacket& packet);
[[nodiscard]] bool decodeCreateCharacterRequest(std::span<const std::uint8_t> bytes, CreateCharacterRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeCreateCharacterResponse(const CreateCharacterResponsePacket& packet);
[[nodiscard]] bool decodeCreateCharacterResponse(std::span<const std::uint8_t> bytes, CreateCharacterResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEnterWorldRequest(const EnterWorldRequestPacket& packet);
[[nodiscard]] bool decodeEnterWorldRequest(std::span<const std::uint8_t> bytes, EnterWorldRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEnterWorldResponse(const EnterWorldResponsePacket& packet);
[[nodiscard]] bool decodeEnterWorldResponse(std::span<const std::uint8_t> bytes, EnterWorldResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeInventorySync(const InventorySyncPacket& packet);
[[nodiscard]] bool decodeInventorySync(std::span<const std::uint8_t> bytes, InventorySyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEquipmentSync(const EquipmentSyncPacket& packet);
[[nodiscard]] bool decodeEquipmentSync(std::span<const std::uint8_t> bytes, EquipmentSyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEquipItemRequest(const EquipItemRequestPacket& packet);
[[nodiscard]] bool decodeEquipItemRequest(std::span<const std::uint8_t> bytes, EquipItemRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeUnequipItemRequest(const UnequipItemRequestPacket& packet);
[[nodiscard]] bool decodeUnequipItemRequest(std::span<const std::uint8_t> bytes, UnequipItemRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeWorldLootSync(const WorldLootSyncPacket& packet);
[[nodiscard]] bool decodeWorldLootSync(std::span<const std::uint8_t> bytes, WorldLootSyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodePickupLootRequest(const PickupLootRequestPacket& packet);
[[nodiscard]] bool decodePickupLootRequest(std::span<const std::uint8_t> bytes, PickupLootRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeSimulateBossLootDropRequest(const SimulateBossLootDropRequestPacket& packet);
[[nodiscard]] bool decodeSimulateBossLootDropRequest(std::span<const std::uint8_t> bytes, SimulateBossLootDropRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeInventoryActionResponse(const InventoryActionResponsePacket& packet);
[[nodiscard]] bool decodeInventoryActionResponse(std::span<const std::uint8_t> bytes, InventoryActionResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeServerSnapshot(const ServerSnapshotPacket& snapshot);
[[nodiscard]] bool decodeServerSnapshot(std::span<const std::uint8_t> bytes,
    ServerSnapshotPacket& snapshot);
[[nodiscard]] std::vector<std::uint8_t> encodeZoneChangeRequest(const ZoneChangeRequestPacket& packet);
[[nodiscard]] bool decodeZoneChangeRequest(std::span<const std::uint8_t> bytes,
    ZoneChangeRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeZoneChangeNotification(
    const ZoneChangeNotificationPacket& packet);
[[nodiscard]] bool decodeZoneChangeNotification(std::span<const std::uint8_t> bytes,
    ZoneChangeNotificationPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeChatMessage(const ChatMessagePacket& packet);
[[nodiscard]] bool decodeChatMessage(std::span<const std::uint8_t> bytes, ChatMessagePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodePartyRequest(const PartyRequestPacket& packet);
[[nodiscard]] bool decodePartyRequest(std::span<const std::uint8_t> bytes, PartyRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodePartySync(const PartySyncPacket& packet);
[[nodiscard]] bool decodePartySync(std::span<const std::uint8_t> bytes, PartySyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeGuildRequest(const GuildRequestPacket& packet);
[[nodiscard]] bool decodeGuildRequest(std::span<const std::uint8_t> bytes, GuildRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeGuildSync(const GuildSyncPacket& packet);
[[nodiscard]] bool decodeGuildSync(std::span<const std::uint8_t> bytes, GuildSyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEnterDungeonRequest(const EnterDungeonRequestPacket& packet);
[[nodiscard]] bool decodeEnterDungeonRequest(std::span<const std::uint8_t> bytes,
    EnterDungeonRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeEnterDungeonResponse(const EnterDungeonResponsePacket& packet);
[[nodiscard]] bool decodeEnterDungeonResponse(std::span<const std::uint8_t> bytes,
    EnterDungeonResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeLeaveDungeonRequest(const LeaveDungeonRequestPacket& packet);
[[nodiscard]] bool decodeLeaveDungeonRequest(std::span<const std::uint8_t> bytes,
    LeaveDungeonRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeDungeonStateSync(const DungeonStateSyncPacket& packet);
[[nodiscard]] bool decodeDungeonStateSync(std::span<const std::uint8_t> bytes,
    DungeonStateSyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeMarketBrowseRequest(const MarketBrowseRequestPacket& packet);
[[nodiscard]] bool decodeMarketBrowseRequest(std::span<const std::uint8_t> bytes, MarketBrowseRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeMarketBrowseResponse(const MarketBrowseResponsePacket& packet);
[[nodiscard]] bool decodeMarketBrowseResponse(std::span<const std::uint8_t> bytes, MarketBrowseResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeMarketListRequest(const MarketListRequestPacket& packet);
[[nodiscard]] bool decodeMarketListRequest(std::span<const std::uint8_t> bytes, MarketListRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeMarketActionRequest(const MarketActionRequestPacket& packet);
[[nodiscard]] bool decodeMarketActionRequest(std::span<const std::uint8_t> bytes, MarketActionRequestPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeMarketActionResponse(const MarketActionResponsePacket& packet);
[[nodiscard]] bool decodeMarketActionResponse(std::span<const std::uint8_t> bytes, MarketActionResponsePacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeCharacterGoldSync(const CharacterGoldSyncPacket& packet);
[[nodiscard]] bool decodeCharacterGoldSync(std::span<const std::uint8_t> bytes, CharacterGoldSyncPacket& packet);
[[nodiscard]] std::vector<std::uint8_t> encodeFeedbackSubmitRequest(const FeedbackSubmitRequestPacket& packet);
[[nodiscard]] bool decodeFeedbackSubmitRequest(std::span<const std::uint8_t> bytes,
    FeedbackSubmitRequestPacket& packet);
[[nodiscard]] std::array<std::uint8_t, 1> encodeFeedbackSubmitResponse(const FeedbackSubmitResponsePacket& packet);
[[nodiscard]] bool decodeFeedbackSubmitResponse(std::span<const std::uint8_t> bytes,
    FeedbackSubmitResponsePacket& packet) noexcept;

struct NetworkEvent {
    EventType type = EventType::PacketReceived;
    std::uint32_t clientId = 0;
    PacketType packetType = PacketType::ServerHeartbeat;
    std::vector<std::uint8_t> payload;
};

class NetworkContext {
public:
    NetworkContext();
    ~NetworkContext();
    NetworkContext(const NetworkContext&) = delete;
    NetworkContext& operator=(const NetworkContext&) = delete;
};

class NetworkServer {
public:
    explicit NetworkServer(NetworkContext& context, std::uint16_t port = 7777,
        std::size_t maxClients = 32);
    ~NetworkServer();
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;

    std::vector<NetworkEvent> poll(std::uint32_t timeoutMs = 0);
    bool send(std::uint32_t clientId, PacketType type, std::span<const std::uint8_t> payload,
        bool reliable = true);
    void broadcast(PacketType type, std::span<const std::uint8_t> payload, bool reliable = false);
    void disconnect(std::uint32_t clientId);
    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

private:
    _ENetHost* host_ = nullptr;
    std::uint16_t port_ = 7777;
    std::uint32_t nextClientId_ = 1;
    struct PeerEntry { _ENetPeer* peer; std::uint32_t id; bool accepted; };
    std::vector<PeerEntry> peers_;
};

class NetworkClient {
public:
    explicit NetworkClient(NetworkContext& context);
    ~NetworkClient();
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    bool connect(const std::string& host, std::uint16_t port);
    std::vector<NetworkEvent> poll(std::uint32_t timeoutMs = 0);
    bool send(PacketType type, std::span<const std::uint8_t> payload, bool reliable = true);
    void sendPing();
    void disconnect();
    [[nodiscard]] ConnectionState state() const noexcept { return state_; }
    [[nodiscard]] std::uint32_t clientId() const noexcept { return clientId_; }
    [[nodiscard]] std::uint32_t rttMilliseconds() const noexcept { return rttMs_; }

private:
    _ENetHost* host_ = nullptr;
    _ENetPeer* peer_ = nullptr;
    ConnectionState state_ = ConnectionState::Disconnected;
    std::uint32_t clientId_ = 0;
    std::uint32_t rttMs_ = 0;
};

} // namespace glory::network

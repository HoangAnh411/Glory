#include "glory/network/network.hpp"

#include <enet/enet.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_set>

namespace glory::network {
namespace {

constexpr std::uint8_t reliableChannel = 0;
constexpr std::uint8_t sequencedChannel = 1;
constexpr std::size_t headerSize = 1;
constexpr std::size_t maxPacketSize = 1200;
constexpr std::size_t maxSnapshotPlayers = 8;
constexpr std::size_t maxSnapshotStringLength = 32;
constexpr std::size_t maxAuthStringLength = 256;
constexpr std::size_t maxSocialPacketSize = maxPacketSize - headerSize;
std::mutex contextMutex;
std::size_t contextCount = 0;

std::uint64_t nowMilliseconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

ENetPacket* makePacket(PacketType type, std::span<const std::uint8_t> payload, bool reliable) {
    if (payload.size() + headerSize > maxPacketSize)
        throw std::length_error("Network packet exceeds 1200 bytes");
    std::vector<std::uint8_t> bytes(payload.size() + headerSize);
    bytes[0] = static_cast<std::uint8_t>(type);
    std::copy(payload.begin(), payload.end(), bytes.begin() + 1);
    return enet_packet_create(bytes.data(), bytes.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
}

bool parsePacket(const ENetPacket* packet, PacketType& type, std::vector<std::uint8_t>& payload) {
    if (!packet || packet->dataLength < headerSize || packet->dataLength > maxPacketSize) return false;
    const auto value = packet->data[0];
    if (value < static_cast<std::uint8_t>(PacketType::ConnectRequest) ||
        value > static_cast<std::uint8_t>(PacketType::FeedbackSubmitResponse)) return false;
    type = static_cast<PacketType>(value);
    payload.assign(packet->data + headerSize, packet->data + packet->dataLength);
    return true;
}

class AuthWriter {
public:
    void byte(std::uint8_t value) { bytes_.push_back(value); }
    void u32(std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) byte(static_cast<std::uint8_t>(value >> (i * 8)));
    }
    void u64(std::uint64_t value) {
        for (unsigned i = 0; i < 8; ++i) byte(static_cast<std::uint8_t>(value >> (i * 8)));
    }
    void f32(float value) { u32(std::bit_cast<std::uint32_t>(value)); }
    void string(const std::string& value) {
        if (value.size() > 1024) throw std::length_error("Auth string exceeds wire limit");
        bytes_.push_back(static_cast<std::uint8_t>(value.size()));
        bytes_.push_back(static_cast<std::uint8_t>(value.size() >> 8));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    std::vector<std::uint8_t> finish() && { return std::move(bytes_); }
private:
    std::vector<std::uint8_t> bytes_;
};

class AuthReader {
public:
    explicit AuthReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    bool byte(std::uint8_t& value) {
        if (offset_ >= bytes_.size()) return false;
        value = bytes_[offset_++]; return true;
    }
    bool u32(std::uint32_t& value) {
        value = 0;
        for (unsigned i = 0; i < 4; ++i) { std::uint8_t part; if (!byte(part)) return false; value |= static_cast<std::uint32_t>(part) << (i * 8); }
        return true;
    }
    bool u64(std::uint64_t& value) {
        value = 0;
        for (unsigned i = 0; i < 8; ++i) { std::uint8_t part; if (!byte(part)) return false; value |= static_cast<std::uint64_t>(part) << (i * 8); }
        return true;
    }
    bool f32(float& value) { std::uint32_t bits; if (!u32(bits)) return false; value = std::bit_cast<float>(bits); return std::isfinite(value); }
    bool string(std::string& value) {
        std::uint8_t low, high;
        if (!byte(low) || !byte(high)) return false;
        const std::size_t length = low | (static_cast<std::size_t>(high) << 8);
        if (length > 1024 || length > bytes_.size() - offset_) return false;
        value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), length);
        offset_ += length; return true;
    }
    bool result(AuthResult& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(AuthResult::ServerError)) return false;
        value = static_cast<AuthResult>(raw); return true;
    }
    bool actionResult(InventoryActionResult& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(InventoryActionResult::ServerError)) return false;
        value = static_cast<InventoryActionResult>(raw); return true;
    }
    bool equipmentSlot(EquipmentSlotPacket& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(EquipmentSlotPacket::Accessory)) return false;
        value = static_cast<EquipmentSlotPacket>(raw); return true;
    }
    bool socialResult(SocialResult& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(SocialResult::ServerError)) return false;
        value = static_cast<SocialResult>(raw); return true;
    }
    bool chatChannel(ChatChannel& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(ChatChannel::System)) return false;
        value = static_cast<ChatChannel>(raw); return true;
    }
    bool partyAction(PartyAction& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(PartyAction::Refresh)) return false;
        value = static_cast<PartyAction>(raw); return true;
    }
    bool guildAction(GuildAction& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(GuildAction::Refresh)) return false;
        value = static_cast<GuildAction>(raw); return true;
    }
    bool guildRank(GuildRank& value) {
        std::uint8_t raw;
        if (!byte(raw) || raw > static_cast<std::uint8_t>(GuildRank::Member)) return false;
        value = static_cast<GuildRank>(raw); return true;
    }
    bool done() const { return offset_ == bytes_.size(); }
private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

void writeCharacter(AuthWriter& writer, const CharacterInfoPacket& character) {
    writer.u64(static_cast<std::uint64_t>(character.characterId));
    writer.string(character.name); writer.string(character.className);
    writer.u32(static_cast<std::uint32_t>(character.level));
    writer.u32(static_cast<std::uint32_t>(character.exp));
}

bool readCharacter(AuthReader& reader, CharacterInfoPacket& character) {
    std::uint64_t id; std::uint32_t level, exp;
    if (!reader.u64(id) || !reader.string(character.name) || !reader.string(character.className) ||
        !reader.u32(level) || !reader.u32(exp)) return false;
    character.characterId = static_cast<std::int64_t>(id);
    character.level = static_cast<std::int32_t>(level); character.exp = static_cast<std::int32_t>(exp);
    return true;
}

void validateSocialPacketSize(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() > maxSocialPacketSize) throw std::length_error("Social packet exceeds ENet limit");
}

} // namespace

std::vector<std::uint8_t> encodeFeedbackSubmitRequest(const FeedbackSubmitRequestPacket& packet) {
    if (static_cast<std::uint8_t>(packet.category) > static_cast<std::uint8_t>(FeedbackCategory::Praise) ||
        packet.rating < 1 || packet.rating > 5 || packet.subject.empty() || packet.subject.size() > 120 ||
        packet.description.empty() || packet.description.size() > 600 || packet.diagnosticsJson.size() > 450)
        throw std::length_error("Feedback field is outside the allowed range");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.category));
    writer.byte(packet.rating);
    writer.string(packet.subject);
    writer.string(packet.description);
    writer.string(packet.diagnosticsJson);
    auto bytes = std::move(writer).finish();
    if (bytes.size() + headerSize > maxPacketSize) throw std::length_error("Feedback packet exceeds ENet limit");
    return bytes;
}

bool decodeFeedbackSubmitRequest(std::span<const std::uint8_t> bytes, FeedbackSubmitRequestPacket& packet) {
    if (bytes.size() + headerSize > maxPacketSize) return false;
    AuthReader reader(bytes);
    std::uint8_t category = 0;
    if (!reader.byte(category) || category > static_cast<std::uint8_t>(FeedbackCategory::Praise) ||
        !reader.byte(packet.rating) || packet.rating < 1 || packet.rating > 5 ||
        !reader.string(packet.subject) || !reader.string(packet.description) ||
        !reader.string(packet.diagnosticsJson) || !reader.done()) return false;
    if (packet.subject.empty() || packet.subject.size() > 120 || packet.description.empty() ||
        packet.description.size() > 600 || packet.diagnosticsJson.size() > 450) return false;
    packet.category = static_cast<FeedbackCategory>(category);
    return true;
}

std::array<std::uint8_t, 1> encodeFeedbackSubmitResponse(const FeedbackSubmitResponsePacket& packet) {
    return {static_cast<std::uint8_t>(packet.status)};
}

bool decodeFeedbackSubmitResponse(std::span<const std::uint8_t> bytes,
    FeedbackSubmitResponsePacket& packet) noexcept {
    if (bytes.size() != 1 || bytes[0] > static_cast<std::uint8_t>(FeedbackSubmitStatus::RateLimited)) return false;
    packet.status = static_cast<FeedbackSubmitStatus>(bytes[0]);
    return true;
}

NetworkContext::NetworkContext() {
    std::lock_guard lock(contextMutex);
    if (contextCount == 0 && enet_initialize() != 0)
        throw std::runtime_error("ENet initialization failed");
    ++contextCount;
}

std::array<std::uint8_t, 24> encodeClientInput(const ClientInputPacket& input) {
    std::array<std::uint8_t, 24> bytes{};
    const std::uint32_t yawBits = std::bit_cast<std::uint32_t>(input.cameraYawDegrees);
    for (unsigned index = 0; index < 4; ++index) {
        bytes[index] = static_cast<std::uint8_t>(input.pressedButtons >> (index * 8));
        bytes[index + 4] = static_cast<std::uint8_t>(input.heldButtons >> (index * 8));
        bytes[index + 8] = static_cast<std::uint8_t>(yawBits >> (index * 8));
        bytes[index + 12] = static_cast<std::uint8_t>(input.inputSequence >> (index * 8));
        bytes[index + 16] = static_cast<std::uint8_t>(input.targetTick >> (index * 8));
        bytes[index + 20] = static_cast<std::uint8_t>(input.targetTick >> ((index + 4) * 8));
    }
    return bytes;
}

bool decodeClientInput(std::span<const std::uint8_t> bytes, ClientInputPacket& input) {
    if (bytes.size() != 24) return false;
    std::uint32_t yawBits = 0;
    input.pressedButtons = 0;
    input.heldButtons = 0;
    input.inputSequence = 0;
    input.targetTick = 0;
    for (unsigned index = 0; index < 4; ++index) {
        input.pressedButtons |= static_cast<std::uint32_t>(bytes[index]) << (index * 8);
        input.heldButtons |= static_cast<std::uint32_t>(bytes[index + 4]) << (index * 8);
        yawBits |= static_cast<std::uint32_t>(bytes[index + 8]) << (index * 8);
        input.inputSequence |= static_cast<std::uint32_t>(bytes[index + 12]) << (index * 8);
        input.targetTick |= static_cast<std::uint64_t>(bytes[index + 16]) << (index * 8);
        input.targetTick |= static_cast<std::uint64_t>(bytes[index + 20]) << ((index + 4) * 8);
    }
    input.cameraYawDegrees = std::bit_cast<float>(yawBits);
    return std::isfinite(input.cameraYawDegrees);
}

std::array<std::uint8_t, 26> encodeAntiCheatWarning(const AntiCheatWarningPacket& packet) {
    if (packet.violation > AntiCheatViolationCode::Telemetry || !std::isfinite(packet.currentSpeed) ||
        !std::isfinite(packet.maximumSpeed) || packet.currentSpeed < 0.0F || packet.maximumSpeed < 0.0F)
        throw std::invalid_argument("Invalid anti-cheat warning packet");
    std::array<std::uint8_t, 26> bytes{};
    bytes[0] = static_cast<std::uint8_t>(packet.violation);
    const std::array<std::uint32_t, 4> counters{{packet.speedViolations, packet.teleportViolations,
        packet.collisionViolations, packet.cooldownViolations}};
    for (std::size_t counter = 0; counter < counters.size(); ++counter)
        for (unsigned byte = 0; byte < 4; ++byte)
            bytes[1 + counter * 4 + byte] = static_cast<std::uint8_t>(counters[counter] >> (byte * 8));
    const std::array<std::uint32_t, 2> speeds{{std::bit_cast<std::uint32_t>(packet.currentSpeed),
        std::bit_cast<std::uint32_t>(packet.maximumSpeed)}};
    for (std::size_t value = 0; value < speeds.size(); ++value)
        for (unsigned byte = 0; byte < 4; ++byte)
            bytes[17 + value * 4 + byte] = static_cast<std::uint8_t>(speeds[value] >> (byte * 8));
    bytes[25] = packet.kick ? 1 : 0;
    return bytes;
}

bool decodeAntiCheatWarning(std::span<const std::uint8_t> bytes, AntiCheatWarningPacket& packet) {
    if (bytes.size() != 26 || bytes[0] > static_cast<std::uint8_t>(AntiCheatViolationCode::Telemetry) ||
        bytes[25] > 1) return false;
    packet = {};
    packet.violation = static_cast<AntiCheatViolationCode>(bytes[0]);
    std::array<std::uint32_t*, 4> counters{{&packet.speedViolations, &packet.teleportViolations,
        &packet.collisionViolations, &packet.cooldownViolations}};
    for (std::size_t counter = 0; counter < counters.size(); ++counter)
        for (unsigned byte = 0; byte < 4; ++byte)
            *counters[counter] |= static_cast<std::uint32_t>(bytes[1 + counter * 4 + byte]) << (byte * 8);
    std::array<std::uint32_t, 2> speeds{};
    for (std::size_t value = 0; value < speeds.size(); ++value)
        for (unsigned byte = 0; byte < 4; ++byte)
            speeds[value] |= static_cast<std::uint32_t>(bytes[17 + value * 4 + byte]) << (byte * 8);
    packet.currentSpeed = std::bit_cast<float>(speeds[0]);
    packet.maximumSpeed = std::bit_cast<float>(speeds[1]);
    packet.kick = bytes[25] != 0;
    return std::isfinite(packet.currentSpeed) && std::isfinite(packet.maximumSpeed) &&
        packet.currentSpeed >= 0.0F && packet.maximumSpeed >= 0.0F;
}

std::array<std::uint8_t, 13> encodeAntiCheatProbe(const AntiCheatProbePacket& packet) {
    if (packet.kind != AntiCheatProbeKind::PositionClaim || !std::isfinite(packet.x) ||
        !std::isfinite(packet.y) || !std::isfinite(packet.z))
        throw std::invalid_argument("Invalid anti-cheat probe packet");
    std::array<std::uint8_t, 13> bytes{};
    bytes[0] = static_cast<std::uint8_t>(packet.kind);
    const std::array<std::uint32_t, 3> values{{std::bit_cast<std::uint32_t>(packet.x),
        std::bit_cast<std::uint32_t>(packet.y), std::bit_cast<std::uint32_t>(packet.z)}};
    for (std::size_t value = 0; value < values.size(); ++value)
        for (unsigned byte = 0; byte < 4; ++byte)
            bytes[1 + value * 4 + byte] = static_cast<std::uint8_t>(values[value] >> (byte * 8));
    return bytes;
}

bool decodeAntiCheatProbe(std::span<const std::uint8_t> bytes, AntiCheatProbePacket& packet) {
    if (bytes.size() != 13 || bytes[0] != static_cast<std::uint8_t>(AntiCheatProbeKind::PositionClaim))
        return false;
    packet = {};
    packet.kind = static_cast<AntiCheatProbeKind>(bytes[0]);
    std::array<float*, 3> values{{&packet.x, &packet.y, &packet.z}};
    for (std::size_t value = 0; value < values.size(); ++value) {
        std::uint32_t bits = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            bits |= static_cast<std::uint32_t>(bytes[1 + value * 4 + byte]) << (byte * 8);
        *values[value] = std::bit_cast<float>(bits);
    }
    return std::isfinite(packet.x) && std::isfinite(packet.y) && std::isfinite(packet.z);
}

std::vector<std::uint8_t> encodeSkillRequest(const std::string& skillId) {
    if (skillId.empty() || skillId.size() > 48) throw std::invalid_argument("Invalid skill request");
    std::vector<std::uint8_t> bytes;
    bytes.reserve(skillId.size() + 1);
    bytes.push_back(static_cast<std::uint8_t>(skillId.size()));
    bytes.insert(bytes.end(), skillId.begin(), skillId.end());
    return bytes;
}

bool decodeSkillRequest(std::span<const std::uint8_t> bytes, std::string& skillId) {
    if (bytes.empty() || bytes[0] == 0 || bytes[0] > 48 || bytes.size() != bytes[0] + 1) return false;
    skillId.assign(bytes.begin() + 1, bytes.end());
    return std::all_of(skillId.begin(), skillId.end(), [](unsigned char value) {
        return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
            (value >= '0' && value <= '9') || value == '_' || value == '-';
    });
}

std::array<std::uint8_t, 2> encodeMatchmakingRequest(const MatchmakingRequestPacket& request) {
    if ((request.mode != MatchmakingMode::Duel1v1 && request.mode != MatchmakingMode::Team3v3) ||
        (request.action != MatchmakingAction::Join && request.action != MatchmakingAction::Leave))
        throw std::invalid_argument("Invalid matchmaking request");
    return {static_cast<std::uint8_t>(request.mode), static_cast<std::uint8_t>(request.action)};
}

bool decodeMatchmakingRequest(std::span<const std::uint8_t> bytes,
    MatchmakingRequestPacket& request) noexcept {
    if (bytes.size() != 2 ||
        (bytes[0] != static_cast<std::uint8_t>(MatchmakingMode::Duel1v1) &&
            bytes[0] != static_cast<std::uint8_t>(MatchmakingMode::Team3v3)) ||
        (bytes[1] != static_cast<std::uint8_t>(MatchmakingAction::Join) &&
            bytes[1] != static_cast<std::uint8_t>(MatchmakingAction::Leave))) return false;
    request.mode = static_cast<MatchmakingMode>(bytes[0]);
    request.action = static_cast<MatchmakingAction>(bytes[1]);
    return true;
}

std::vector<std::uint8_t> encodeRegisterRequest(const RegisterRequestPacket& packet) {
    AuthWriter writer; writer.string(packet.username); writer.string(packet.password); return std::move(writer).finish();
}
bool decodeRegisterRequest(std::span<const std::uint8_t> bytes, RegisterRequestPacket& packet) {
    AuthReader reader(bytes); return reader.string(packet.username) && reader.string(packet.password) && reader.done();
}
std::vector<std::uint8_t> encodeRegisterResponse(const RegisterResponsePacket& packet) {
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result)); return std::move(writer).finish();
}
bool decodeRegisterResponse(std::span<const std::uint8_t> bytes, RegisterResponsePacket& packet) {
    AuthReader reader(bytes); return reader.result(packet.result) && reader.done();
}
std::vector<std::uint8_t> encodeLoginRequest(const LoginRequestPacket& packet) {
    AuthWriter writer; writer.string(packet.username); writer.string(packet.password); return std::move(writer).finish();
}
bool decodeLoginRequest(std::span<const std::uint8_t> bytes, LoginRequestPacket& packet) {
    AuthReader reader(bytes); return reader.string(packet.username) && reader.string(packet.password) && reader.done();
}
std::vector<std::uint8_t> encodeLoginResponse(const LoginResponsePacket& packet) {
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result)); writer.string(packet.sessionToken);
    return std::move(writer).finish();
}
bool decodeLoginResponse(std::span<const std::uint8_t> bytes, LoginResponsePacket& packet) {
    AuthReader reader(bytes); return reader.result(packet.result) && reader.string(packet.sessionToken) && reader.done();
}
std::vector<std::uint8_t> encodeCharacterListRequest(const CharacterListRequestPacket& packet) {
    AuthWriter writer; writer.string(packet.sessionToken); return std::move(writer).finish();
}
bool decodeCharacterListRequest(std::span<const std::uint8_t> bytes, CharacterListRequestPacket& packet) {
    AuthReader reader(bytes); return reader.string(packet.sessionToken) && reader.done();
}
std::vector<std::uint8_t> encodeCharacterListResponse(const CharacterListResponsePacket& packet) {
    if (packet.characters.size() > 16) throw std::length_error("Character list exceeds limit");
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result));
    writer.byte(static_cast<std::uint8_t>(packet.characters.size()));
    for (const auto& character : packet.characters) writeCharacter(writer, character);
    return std::move(writer).finish();
}
bool decodeCharacterListResponse(std::span<const std::uint8_t> bytes, CharacterListResponsePacket& packet) {
    AuthReader reader(bytes); std::uint8_t count;
    if (!reader.result(packet.result) || !reader.byte(count) || count > 16) return false;
    packet.characters.clear(); packet.characters.resize(count);
    for (auto& character : packet.characters) if (!readCharacter(reader, character)) return false;
    return reader.done();
}
std::vector<std::uint8_t> encodeCreateCharacterRequest(const CreateCharacterRequestPacket& packet) {
    AuthWriter writer; writer.string(packet.sessionToken); writer.string(packet.name); writer.string(packet.className);
    return std::move(writer).finish();
}
bool decodeCreateCharacterRequest(std::span<const std::uint8_t> bytes, CreateCharacterRequestPacket& packet) {
    AuthReader reader(bytes); return reader.string(packet.sessionToken) && reader.string(packet.name) &&
        reader.string(packet.className) && reader.done();
}
std::vector<std::uint8_t> encodeCreateCharacterResponse(const CreateCharacterResponsePacket& packet) {
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result));
    writeCharacter(writer, packet.character); return std::move(writer).finish();
}
bool decodeCreateCharacterResponse(std::span<const std::uint8_t> bytes, CreateCharacterResponsePacket& packet) {
    AuthReader reader(bytes); return reader.result(packet.result) && readCharacter(reader, packet.character) && reader.done();
}
std::vector<std::uint8_t> encodeEnterWorldRequest(const EnterWorldRequestPacket& packet) {
    AuthWriter writer; writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    return std::move(writer).finish();
}
bool decodeEnterWorldRequest(std::span<const std::uint8_t> bytes, EnterWorldRequestPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId;
    if (!reader.string(packet.sessionToken) || !reader.u64(characterId) || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}
std::vector<std::uint8_t> encodeEnterWorldResponse(const EnterWorldResponsePacket& packet) {
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result)); writeCharacter(writer, packet.character);
    return std::move(writer).finish();
}
bool decodeEnterWorldResponse(std::span<const std::uint8_t> bytes, EnterWorldResponsePacket& packet) {
    AuthReader reader(bytes); return reader.result(packet.result) && readCharacter(reader, packet.character) && reader.done();
}

std::vector<std::uint8_t> encodeInventorySync(const InventorySyncPacket& packet) {
    if (packet.characterId <= 0 || packet.items.size() > 24) throw std::length_error("Invalid inventory sync");
    AuthWriter writer; writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.items.size()));
    std::array<bool, 24> seen{};
    for (const auto& item : packet.items) {
        if (item.slotIndex >= seen.size() || seen[item.slotIndex] || item.itemId.empty() ||
            item.itemId.size() > 32 || item.quantity <= 0) throw std::invalid_argument("Invalid inventory slot");
        seen[item.slotIndex] = true;
        writer.byte(item.slotIndex); writer.string(item.itemId); writer.u32(static_cast<std::uint32_t>(item.quantity));
    }
    return std::move(writer).finish();
}
bool decodeInventorySync(std::span<const std::uint8_t> bytes, InventorySyncPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId; std::uint8_t count;
    if (!reader.u64(characterId) || characterId == 0 || !reader.byte(count) || count > 24) return false;
    std::vector<InventorySlotPacket> items; std::array<bool, 24> seen{};
    for (std::uint8_t index = 0; index < count; ++index) {
        InventorySlotPacket item; std::uint32_t quantity;
        if (!reader.byte(item.slotIndex) || item.slotIndex >= seen.size() || seen[item.slotIndex] ||
            !reader.string(item.itemId) || item.itemId.empty() || item.itemId.size() > 32 ||
            !reader.u32(quantity) || quantity == 0 || quantity > 999) return false;
        seen[item.slotIndex] = true; item.quantity = static_cast<std::int32_t>(quantity); items.push_back(std::move(item));
    }
    if (!reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); packet.items = std::move(items); return true;
}

std::vector<std::uint8_t> encodeEquipmentSync(const EquipmentSyncPacket& packet) {
    if (packet.characterId <= 0 || packet.items.size() > 3 || packet.bonusAttack < 0 ||
        packet.bonusDefense < 0 || packet.bonusMaxHp < 0) throw std::invalid_argument("Invalid equipment sync");
    AuthWriter writer; writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.items.size())); std::array<bool, 3> seen{};
    for (const auto& item : packet.items) {
        const auto slot = static_cast<std::uint8_t>(item.slot);
        if (slot >= seen.size() || seen[slot] || item.itemId.empty() || item.itemId.size() > 32)
            throw std::invalid_argument("Invalid equipment item");
        seen[slot] = true; writer.byte(slot); writer.string(item.itemId);
    }
    writer.u32(static_cast<std::uint32_t>(packet.bonusAttack));
    writer.u32(static_cast<std::uint32_t>(packet.bonusDefense));
    writer.u32(static_cast<std::uint32_t>(packet.bonusMaxHp));
    return std::move(writer).finish();
}
bool decodeEquipmentSync(std::span<const std::uint8_t> bytes, EquipmentSyncPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId; std::uint8_t count;
    if (!reader.u64(characterId) || characterId == 0 || !reader.byte(count) || count > 3) return false;
    std::vector<EquipmentItemPacket> items; std::array<bool, 3> seen{};
    for (std::uint8_t index = 0; index < count; ++index) {
        EquipmentItemPacket item; std::uint32_t slot;
        if (!reader.equipmentSlot(item.slot)) return false;
        slot = static_cast<std::uint8_t>(item.slot);
        if (seen[slot] || !reader.string(item.itemId) || item.itemId.empty() || item.itemId.size() > 32) return false;
        seen[slot] = true; items.push_back(std::move(item));
    }
    std::uint32_t attack, defense, hp;
    if (!reader.u32(attack) || !reader.u32(defense) || !reader.u32(hp) || attack > 100000 || defense > 100000 ||
        hp > 100000 || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); packet.items = std::move(items);
    packet.bonusAttack = static_cast<std::int32_t>(attack); packet.bonusDefense = static_cast<std::int32_t>(defense);
    packet.bonusMaxHp = static_cast<std::int32_t>(hp); return true;
}

std::vector<std::uint8_t> encodeEquipItemRequest(const EquipItemRequestPacket& packet) {
    if (packet.characterId <= 0 || packet.inventorySlot >= 24 ||
        static_cast<std::uint8_t>(packet.slot) > static_cast<std::uint8_t>(EquipmentSlotPacket::Accessory))
        throw std::invalid_argument("Invalid equip request");
    AuthWriter writer; writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(packet.inventorySlot); writer.byte(static_cast<std::uint8_t>(packet.slot));
    return std::move(writer).finish();
}
bool decodeEquipItemRequest(std::span<const std::uint8_t> bytes, EquipItemRequestPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || !reader.u64(characterId) ||
        characterId == 0 || !reader.byte(packet.inventorySlot) || packet.inventorySlot >= 24 ||
        !reader.equipmentSlot(packet.slot) || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}

std::vector<std::uint8_t> encodeUnequipItemRequest(const UnequipItemRequestPacket& packet) {
    if (packet.characterId <= 0) throw std::invalid_argument("Invalid unequip request");
    AuthWriter writer; writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.slot)); return std::move(writer).finish();
}
bool decodeUnequipItemRequest(std::span<const std::uint8_t> bytes, UnequipItemRequestPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || !reader.u64(characterId) ||
        characterId == 0 || !reader.equipmentSlot(packet.slot) || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}

std::vector<std::uint8_t> encodeWorldLootSync(const WorldLootSyncPacket& packet) {
    if (packet.drops.size() > 16) throw std::length_error("World loot sync exceeds limit");
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.drops.size()));
    for (const auto& drop : packet.drops) {
        if (drop.lootId == 0 || !std::isfinite(drop.x) || !std::isfinite(drop.y) || !std::isfinite(drop.z) ||
            drop.itemId.empty() || drop.itemId.size() > 32 || drop.quantity <= 0 || drop.quantity > 999)
            throw std::invalid_argument("Invalid world loot entry");
        writer.u64(drop.lootId); writer.f32(drop.x); writer.f32(drop.y); writer.f32(drop.z);
        writer.string(drop.itemId); writer.u32(static_cast<std::uint32_t>(drop.quantity));
    }
    return std::move(writer).finish();
}
bool decodeWorldLootSync(std::span<const std::uint8_t> bytes, WorldLootSyncPacket& packet) {
    AuthReader reader(bytes); std::uint8_t count;
    if (!reader.byte(count) || count > 16) return false;
    std::vector<WorldLootEntryPacket> drops; std::unordered_set<std::uint64_t> seen;
    for (std::uint8_t index = 0; index < count; ++index) {
        WorldLootEntryPacket drop; std::uint32_t quantity;
        if (!reader.u64(drop.lootId) || drop.lootId == 0 || !seen.insert(drop.lootId).second ||
            !reader.f32(drop.x) || !reader.f32(drop.y) || !reader.f32(drop.z) ||
            !reader.string(drop.itemId) || drop.itemId.empty() || drop.itemId.size() > 32 ||
            !reader.u32(quantity) || quantity == 0 || quantity > 999) return false;
        drop.quantity = static_cast<std::int32_t>(quantity); drops.push_back(std::move(drop));
    }
    if (!reader.done()) return false; packet.drops = std::move(drops); return true;
}

std::vector<std::uint8_t> encodePickupLootRequest(const PickupLootRequestPacket& packet) {
    if (packet.characterId <= 0 || packet.lootId == 0) throw std::invalid_argument("Invalid pickup request");
    AuthWriter writer; writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.u64(packet.lootId); return std::move(writer).finish();
}
bool decodePickupLootRequest(std::span<const std::uint8_t> bytes, PickupLootRequestPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || !reader.u64(characterId) ||
        characterId == 0 || !reader.u64(packet.lootId) || packet.lootId == 0 || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}

std::vector<std::uint8_t> encodeSimulateBossLootDropRequest(const SimulateBossLootDropRequestPacket& packet) {
    if (packet.characterId <= 0) throw std::invalid_argument("Invalid debug loot request");
    AuthWriter writer; writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    return std::move(writer).finish();
}
bool decodeSimulateBossLootDropRequest(std::span<const std::uint8_t> bytes, SimulateBossLootDropRequestPacket& packet) {
    AuthReader reader(bytes); std::uint64_t characterId;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || !reader.u64(characterId) ||
        characterId == 0 || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}

std::vector<std::uint8_t> encodeInventoryActionResponse(const InventoryActionResponsePacket& packet) {
    AuthWriter writer; writer.byte(static_cast<std::uint8_t>(packet.result)); return std::move(writer).finish();
}
bool decodeInventoryActionResponse(std::span<const std::uint8_t> bytes, InventoryActionResponsePacket& packet) {
    AuthReader reader(bytes); return reader.actionResult(packet.result) && reader.done();
}

std::vector<std::uint8_t> encodeServerSnapshot(const ServerSnapshotPacket& snapshot) {
    if (snapshot.players.size() > maxSnapshotPlayers || snapshot.currentZone > ZoneId::Zone3Forest ||
        snapshot.visibleEntityCount > snapshot.totalEntityCount)
        throw std::length_error("Invalid snapshot zone or entity counts");
    std::vector<std::uint8_t> bytes;
    bytes.reserve(64 + snapshot.players.size() * 96);
    const auto writeInteger = [&bytes](std::uint64_t value, unsigned count) {
        for (unsigned index = 0; index < count; ++index)
            bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
    };
    const auto writeFloat = [&writeInteger](float value) {
        if (!std::isfinite(value)) throw std::invalid_argument("Snapshot contains non-finite number");
        writeInteger(std::bit_cast<std::uint32_t>(value), 4);
    };
    const auto writeVector = [&writeFloat](const SnapshotVector3& value) {
        writeFloat(value.x); writeFloat(value.y); writeFloat(value.z);
    };
    const auto writeString = [&bytes](const std::string& value) {
        if (value.size() > maxSnapshotStringLength) throw std::length_error("Snapshot string is too long");
        bytes.push_back(static_cast<std::uint8_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    };
    if (snapshot.matchState > ArenaMatchState::MatchFinished ||
        (snapshot.matchMode != MatchmakingMode::Duel1v1 && snapshot.matchMode != MatchmakingMode::Team3v3) ||
        snapshot.winningTeam > ArenaTeam::Blue || !std::isfinite(snapshot.countdownRemaining) ||
        snapshot.countdownRemaining < 0.0F)
        throw std::invalid_argument("Invalid arena snapshot state");
    bytes.push_back(4); // Schema version.
    writeInteger(snapshot.serverTick, 8);
    bytes.push_back(static_cast<std::uint8_t>(snapshot.currentZone));
    writeInteger(snapshot.totalEntityCount, 2);
    writeInteger(snapshot.visibleEntityCount, 2);
    bytes.push_back(static_cast<std::uint8_t>(snapshot.matchState));
    bytes.push_back(static_cast<std::uint8_t>(snapshot.matchMode));
    writeFloat(snapshot.countdownRemaining);
    bytes.push_back(static_cast<std::uint8_t>(snapshot.winningTeam));
    bytes.push_back(static_cast<std::uint8_t>(snapshot.players.size()));
    for (const SnapshotPlayer& player : snapshot.players) {
        if (player.clientId == 0 || player.currentHp < 0.0F || player.maxHp <= 0.0F ||
            player.currentHp > player.maxHp) throw std::invalid_argument("Invalid snapshot player state");
        writeInteger(player.clientId, 4);
        if (player.team > ArenaTeam::Blue) throw std::invalid_argument("Invalid snapshot team");
        bytes.push_back(static_cast<std::uint8_t>(player.team));
        writeVector(player.position); writeVector(player.velocity);
        writeFloat(player.yaw); writeFloat(player.currentHp); writeFloat(player.maxHp);
        bytes.push_back(static_cast<std::uint8_t>((player.isDodging ? 1 : 0) |
            (player.isInvulnerable ? 2 : 0) | (player.isGrounded ? 4 : 0)));
        writeString(player.animationState); writeString(player.activeSkillId);
        writeInteger(player.lastProcessedInputSequence, 4);
        writeFloat(player.dodgeElapsed); writeFloat(player.dodgeCooldownRemaining);
        writeVector(player.dodgeDirection); writeVector(player.externalKnockbackVelocity);
        writeFloat(player.knockbackRemaining);
    }
    writeVector(snapshot.boss.position);
    if (snapshot.boss.currentHp < 0.0F || snapshot.boss.maxHp < 0.0F ||
        snapshot.boss.currentHp > snapshot.boss.maxHp) throw std::invalid_argument("Invalid snapshot boss state");
    writeFloat(snapshot.boss.currentHp); writeFloat(snapshot.boss.maxHp);
    writeString(snapshot.boss.phase);
    if (bytes.size() > maxPacketSize - headerSize) throw std::length_error("Snapshot exceeds ENet packet limit");
    return bytes;
}

bool decodeServerSnapshot(std::span<const std::uint8_t> bytes, ServerSnapshotPacket& snapshot) {
    if (bytes.empty() || bytes.size() > maxPacketSize - headerSize || bytes[0] != 4) return false;
    std::size_t cursor = 1;
    const auto readInteger = [&bytes, &cursor](unsigned count, std::uint64_t& value) {
        if (cursor + count > bytes.size()) return false;
        value = 0;
        for (unsigned index = 0; index < count; ++index)
            value |= static_cast<std::uint64_t>(bytes[cursor++]) << (index * 8);
        return true;
    };
    const auto readFloat = [&readInteger](float& value) {
        std::uint64_t bits = 0;
        if (!readInteger(4, bits)) return false;
        value = std::bit_cast<float>(static_cast<std::uint32_t>(bits));
        return std::isfinite(value);
    };
    const auto readVector = [&readFloat](SnapshotVector3& value) {
        return readFloat(value.x) && readFloat(value.y) && readFloat(value.z);
    };
    const auto readString = [&bytes, &cursor](std::string& value) {
        if (cursor >= bytes.size()) return false;
        const std::size_t length = bytes[cursor++];
        if (length > maxSnapshotStringLength || cursor + length > bytes.size()) return false;
        value.assign(reinterpret_cast<const char*>(bytes.data() + cursor), length);
        cursor += length;
        return true;
    };
    ServerSnapshotPacket decoded;
    std::uint64_t integer = 0;
    if (!readInteger(8, decoded.serverTick) || cursor >= bytes.size()) return false;
    decoded.currentZone = static_cast<ZoneId>(bytes[cursor++]);
    if (decoded.currentZone > ZoneId::Zone3Forest || !readInteger(2, integer)) return false;
    decoded.totalEntityCount = static_cast<std::uint16_t>(integer);
    if (!readInteger(2, integer)) return false;
    decoded.visibleEntityCount = static_cast<std::uint16_t>(integer);
    if (decoded.visibleEntityCount > decoded.totalEntityCount || cursor + 2 > bytes.size()) return false;
    decoded.matchState = static_cast<ArenaMatchState>(bytes[cursor++]);
    decoded.matchMode = static_cast<MatchmakingMode>(bytes[cursor++]);
    if (!readFloat(decoded.countdownRemaining) || cursor >= bytes.size()) return false;
    decoded.winningTeam = static_cast<ArenaTeam>(bytes[cursor++]);
    if (decoded.matchState > ArenaMatchState::MatchFinished ||
        (decoded.matchMode != MatchmakingMode::Duel1v1 && decoded.matchMode != MatchmakingMode::Team3v3) ||
        decoded.winningTeam > ArenaTeam::Blue || decoded.countdownRemaining < 0.0F) return false;
    if (cursor >= bytes.size()) return false;
    const std::size_t playerCount = bytes[cursor++];
    if (playerCount > maxSnapshotPlayers) return false;
    decoded.players.reserve(playerCount);
    std::unordered_set<std::uint32_t> clientIds;
    for (std::size_t index = 0; index < playerCount; ++index) {
        SnapshotPlayer player;
        if (!readInteger(4, integer) || cursor >= bytes.size()) return false;
        player.clientId = static_cast<std::uint32_t>(integer);
        player.team = static_cast<ArenaTeam>(bytes[cursor++]);
        std::uint64_t flags = 0;
        if (player.clientId == 0 || player.team > ArenaTeam::Blue ||
            !clientIds.insert(player.clientId).second ||
            !readVector(player.position) || !readVector(player.velocity) ||
            !readFloat(player.yaw) || !readFloat(player.currentHp) || !readFloat(player.maxHp) ||
            !readInteger(1, flags) || flags > 7 || !readString(player.animationState) ||
            !readString(player.activeSkillId) || !readInteger(4, integer) ||
            !readFloat(player.dodgeElapsed) || !readFloat(player.dodgeCooldownRemaining) ||
            !readVector(player.dodgeDirection) || !readVector(player.externalKnockbackVelocity) ||
            !readFloat(player.knockbackRemaining) ||
            player.maxHp <= 0.0F || player.currentHp < 0.0F ||
            player.dodgeElapsed < 0.0F || player.dodgeCooldownRemaining < 0.0F ||
            player.knockbackRemaining < 0.0F ||
            player.currentHp > player.maxHp) return false;
        player.isDodging = (flags & 1) != 0;
        player.isInvulnerable = (flags & 2) != 0;
        player.isGrounded = (flags & 4) != 0;
        player.lastProcessedInputSequence = static_cast<std::uint32_t>(integer);
        decoded.players.push_back(std::move(player));
    }
    if (!readVector(decoded.boss.position) || !readFloat(decoded.boss.currentHp) ||
        !readFloat(decoded.boss.maxHp) || !readString(decoded.boss.phase) ||
        decoded.boss.currentHp < 0.0F || decoded.boss.maxHp < 0.0F ||
        decoded.boss.currentHp > decoded.boss.maxHp ||
        cursor != bytes.size()) return false;
    snapshot = std::move(decoded);
    return true;
}

std::vector<std::uint8_t> encodeZoneChangeRequest(const ZoneChangeRequestPacket& packet) {
    if (packet.characterId <= 0 || packet.targetZone > ZoneId::Zone3Forest || packet.sessionToken.empty())
        throw std::invalid_argument("Invalid zone change request");
    AuthWriter writer;
    writer.string(packet.sessionToken);
    writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.targetZone));
    return std::move(writer).finish();
}

bool decodeZoneChangeRequest(std::span<const std::uint8_t> bytes, ZoneChangeRequestPacket& packet) {
    AuthReader reader(bytes);
    std::uint64_t characterId = 0;
    std::uint8_t zone = 0;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || !reader.u64(characterId) ||
        characterId == 0 || !reader.byte(zone) || zone > static_cast<std::uint8_t>(ZoneId::Zone3Forest) ||
        !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId);
    packet.targetZone = static_cast<ZoneId>(zone);
    return true;
}

std::vector<std::uint8_t> encodeZoneChangeNotification(const ZoneChangeNotificationPacket& packet) {
    if (packet.currentZone > ZoneId::Zone3Forest || !std::isfinite(packet.spawnX) ||
        !std::isfinite(packet.spawnY) || !std::isfinite(packet.spawnZ))
        throw std::invalid_argument("Invalid zone change notification");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.currentZone));
    writer.f32(packet.spawnX); writer.f32(packet.spawnY); writer.f32(packet.spawnZ);
    return std::move(writer).finish();
}

bool decodeZoneChangeNotification(std::span<const std::uint8_t> bytes,
    ZoneChangeNotificationPacket& packet) {
    AuthReader reader(bytes);
    std::uint8_t zone = 0;
    if (!reader.byte(zone) || zone > static_cast<std::uint8_t>(ZoneId::Zone3Forest) ||
        !reader.f32(packet.spawnX) || !reader.f32(packet.spawnY) || !reader.f32(packet.spawnZ) ||
        !reader.done()) return false;
    packet.currentZone = static_cast<ZoneId>(zone);
    return true;
}

std::vector<std::uint8_t> encodeChatMessage(const ChatMessagePacket& packet) {
    if ((packet.characterId < 0 || (packet.characterId == 0 && packet.channel != ChatChannel::System)) ||
        packet.sessionToken.size() > 96 || packet.senderName.size() > 48 ||
        packet.targetName.size() > 48 || packet.text.empty() || packet.text.size() > 768 ||
        packet.channel > ChatChannel::System) throw std::invalid_argument("Invalid chat message");
    AuthWriter writer;
    writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.channel)); writer.string(packet.senderName);
    writer.string(packet.targetName); writer.string(packet.text);
    auto bytes = std::move(writer).finish();
    validateSocialPacketSize(bytes);
    return bytes;
}

bool decodeChatMessage(std::span<const std::uint8_t> bytes, ChatMessagePacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes);
    std::uint64_t characterId = 0;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.size() > 96 ||
        !reader.u64(characterId) || !reader.chatChannel(packet.channel) ||
        !reader.string(packet.senderName) || packet.senderName.size() > 48 ||
        !reader.string(packet.targetName) || packet.targetName.size() > 48 ||
        !reader.string(packet.text) || packet.text.empty() || packet.text.size() > 768 || !reader.done()) return false;
    if (characterId > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        (characterId == 0 && packet.channel != ChatChannel::System)) return false;
    packet.characterId = static_cast<std::int64_t>(characterId);
    return true;
}

std::vector<std::uint8_t> encodePartyRequest(const PartyRequestPacket& packet) {
    if (packet.characterId <= 0 || packet.sessionToken.empty() || packet.sessionToken.size() > 96 ||
        packet.targetName.size() > 48 || packet.action > PartyAction::Refresh)
        throw std::invalid_argument("Invalid party request");
    AuthWriter writer;
    writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.action)); writer.string(packet.targetName);
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodePartyRequest(std::span<const std::uint8_t> bytes, PartyRequestPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); std::uint64_t characterId = 0;
    if (!reader.string(packet.sessionToken) || packet.sessionToken.empty() || packet.sessionToken.size() > 96 ||
        !reader.u64(characterId) || characterId == 0 || !reader.partyAction(packet.action) ||
        !reader.string(packet.targetName) || packet.targetName.size() > 48 || !reader.done()) return false;
    packet.characterId = static_cast<std::int64_t>(characterId); return true;
}

std::vector<std::uint8_t> encodePartySync(const PartySyncPacket& packet) {
    if (packet.result > SocialResult::ServerError || packet.members.size() > 5 ||
        (packet.members.empty() != (packet.partyId == 0))) throw std::invalid_argument("Invalid party sync");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.result)); writer.u64(packet.partyId);
    writer.byte(packet.invitationPending ? 1 : 0); writer.byte(static_cast<std::uint8_t>(packet.members.size()));
    for (const auto& member : packet.members) {
        if (member.characterId <= 0 || member.name.empty() || member.name.size() > 48 ||
            member.className.size() > 32 || member.level <= 0 || !std::isfinite(member.currentHp) ||
            !std::isfinite(member.maxHp) || member.maxHp <= 0.0F || member.currentHp < 0.0F ||
            member.currentHp > member.maxHp) throw std::invalid_argument("Invalid party member");
        writer.u64(static_cast<std::uint64_t>(member.characterId)); writer.string(member.name);
        writer.string(member.className); writer.u32(static_cast<std::uint32_t>(member.level));
        writer.f32(member.currentHp); writer.f32(member.maxHp); writer.byte(member.leader ? 1 : 0);
    }
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodePartySync(std::span<const std::uint8_t> bytes, PartySyncPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); PartySyncPacket decoded; std::uint8_t pending = 0, count = 0;
    if (!reader.socialResult(decoded.result) || !reader.u64(decoded.partyId) ||
        !reader.byte(pending) || pending > 1 || !reader.byte(count) || count > 5) return false;
    decoded.invitationPending = pending != 0; decoded.members.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        PartyMemberPacket member; std::uint64_t id = 0; std::uint32_t level = 0; std::uint8_t leader = 0;
        if (!reader.u64(id) || id == 0 || !reader.string(member.name) || member.name.empty() ||
            member.name.size() > 48 || !reader.string(member.className) || member.className.size() > 32 ||
            !reader.u32(level) || level == 0 || level > 10000 || !reader.f32(member.currentHp) ||
            !reader.f32(member.maxHp) || !reader.byte(leader) || leader > 1 || member.maxHp <= 0.0F ||
            member.currentHp < 0.0F || member.currentHp > member.maxHp) return false;
        member.characterId = static_cast<std::int64_t>(id); member.level = static_cast<std::int32_t>(level);
        member.leader = leader != 0; decoded.members.push_back(std::move(member));
    }
    if (!reader.done() || (decoded.members.empty() != (decoded.partyId == 0))) return false;
    packet = std::move(decoded); return true;
}

std::vector<std::uint8_t> encodeGuildRequest(const GuildRequestPacket& packet) {
    if (packet.characterId <= 0 || packet.sessionToken.empty() || packet.sessionToken.size() > 96 ||
        packet.guildName.size() > 32 || packet.targetName.size() > 48 || packet.action > GuildAction::Refresh ||
        packet.rank > GuildRank::Member) throw std::invalid_argument("Invalid guild request");
    AuthWriter writer;
    writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.byte(static_cast<std::uint8_t>(packet.action)); writer.u64(packet.guildId);
    writer.string(packet.guildName); writer.string(packet.targetName); writer.byte(static_cast<std::uint8_t>(packet.rank));
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodeGuildRequest(std::span<const std::uint8_t> bytes, GuildRequestPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); GuildRequestPacket decoded; std::uint64_t characterId = 0;
    if (!reader.string(decoded.sessionToken) || decoded.sessionToken.empty() || decoded.sessionToken.size() > 96 ||
        !reader.u64(characterId) || characterId == 0 || !reader.guildAction(decoded.action) ||
        !reader.u64(decoded.guildId) || !reader.string(decoded.guildName) || decoded.guildName.size() > 32 ||
        !reader.string(decoded.targetName) || decoded.targetName.size() > 48 ||
        !reader.guildRank(decoded.rank) || !reader.done()) return false;
    decoded.characterId = static_cast<std::int64_t>(characterId); packet = std::move(decoded); return true;
}

std::vector<std::uint8_t> encodeGuildSync(const GuildSyncPacket& packet) {
    if (packet.result > SocialResult::ServerError || packet.members.size() > 8 ||
        packet.guildName.size() > 32 || packet.memberRank > GuildRank::Member ||
        (packet.guildId == 0 && !packet.members.empty())) throw std::invalid_argument("Invalid guild sync");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.result)); writer.u64(packet.guildId);
    writer.string(packet.guildName); writer.byte(static_cast<std::uint8_t>(packet.memberRank));
    writer.byte(packet.invitationPending ? 1 : 0); writer.byte(static_cast<std::uint8_t>(packet.members.size()));
    for (const auto& member : packet.members) {
        if (member.characterId <= 0 || member.name.empty() || member.name.size() > 48 ||
            member.className.size() > 32 || member.level <= 0 || member.rank > GuildRank::Member)
            throw std::invalid_argument("Invalid guild member");
        writer.u64(static_cast<std::uint64_t>(member.characterId)); writer.string(member.name);
        writer.string(member.className); writer.u32(static_cast<std::uint32_t>(member.level));
        writer.byte(static_cast<std::uint8_t>(member.rank));
    }
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodeGuildSync(std::span<const std::uint8_t> bytes, GuildSyncPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); GuildSyncPacket decoded; std::uint8_t count = 0, invitation = 0;
    if (!reader.socialResult(decoded.result) || !reader.u64(decoded.guildId) ||
        !reader.string(decoded.guildName) || decoded.guildName.size() > 32 ||
        !reader.guildRank(decoded.memberRank) || !reader.byte(invitation) || invitation > 1 ||
        !reader.byte(count) || count > 8 ||
        (decoded.guildId == 0 && count != 0)) return false;
    decoded.invitationPending = invitation != 0;
    decoded.members.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        GuildMemberPacket member; std::uint64_t id = 0; std::uint32_t level = 0;
        if (!reader.u64(id) || id == 0 || !reader.string(member.name) || member.name.empty() ||
            member.name.size() > 48 || !reader.string(member.className) || member.className.size() > 32 ||
            !reader.u32(level) || level == 0 || level > 10000 || !reader.guildRank(member.rank)) return false;
        member.characterId = static_cast<std::int64_t>(id); member.level = static_cast<std::int32_t>(level);
        decoded.members.push_back(std::move(member));
    }
    if (!reader.done()) return false;
    packet = std::move(decoded); return true;
}

std::vector<std::uint8_t> encodeEnterDungeonRequest(const EnterDungeonRequestPacket& packet) {
    if (packet.sessionToken.empty() || packet.sessionToken.size() > 96 || packet.characterId <= 0 ||
        packet.dungeonId == 0) throw std::invalid_argument("Invalid dungeon entry request");
    AuthWriter writer;
    writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    writer.u32(packet.dungeonId); writer.byte(packet.enterParty ? 1 : 0);
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodeEnterDungeonRequest(std::span<const std::uint8_t> bytes, EnterDungeonRequestPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); EnterDungeonRequestPacket decoded; std::uint64_t characterId = 0;
    std::uint8_t party = 0;
    if (!reader.string(decoded.sessionToken) || decoded.sessionToken.empty() || decoded.sessionToken.size() > 96 ||
        !reader.u64(characterId) || characterId == 0 ||
        characterId > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        !reader.u32(decoded.dungeonId) || decoded.dungeonId == 0 || !reader.byte(party) || party > 1 ||
        !reader.done()) return false;
    decoded.characterId = static_cast<std::int64_t>(characterId);
    decoded.enterParty = party != 0;
    packet = std::move(decoded);
    return true;
}

std::vector<std::uint8_t> encodeEnterDungeonResponse(const EnterDungeonResponsePacket& packet) {
    if (packet.result > DungeonResult::ServerError ||
        (packet.result == DungeonResult::Success && (packet.dungeonId == 0 || packet.instanceId == 0)))
        throw std::invalid_argument("Invalid dungeon entry response");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.result)); writer.u32(packet.dungeonId);
    writer.u64(packet.instanceId);
    return std::move(writer).finish();
}

bool decodeEnterDungeonResponse(std::span<const std::uint8_t> bytes, EnterDungeonResponsePacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); EnterDungeonResponsePacket decoded; std::uint8_t result = 0;
    if (!reader.byte(result) || result > static_cast<std::uint8_t>(DungeonResult::ServerError) ||
        !reader.u32(decoded.dungeonId) || !reader.u64(decoded.instanceId) || !reader.done()) return false;
    decoded.result = static_cast<DungeonResult>(result);
    if (decoded.result == DungeonResult::Success && (decoded.dungeonId == 0 || decoded.instanceId == 0)) return false;
    packet = decoded;
    return true;
}

std::vector<std::uint8_t> encodeLeaveDungeonRequest(const LeaveDungeonRequestPacket& packet) {
    if (packet.sessionToken.empty() || packet.sessionToken.size() > 96 || packet.characterId <= 0)
        throw std::invalid_argument("Invalid dungeon leave request");
    AuthWriter writer;
    writer.string(packet.sessionToken); writer.u64(static_cast<std::uint64_t>(packet.characterId));
    return std::move(writer).finish();
}

bool decodeLeaveDungeonRequest(std::span<const std::uint8_t> bytes, LeaveDungeonRequestPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); LeaveDungeonRequestPacket decoded; std::uint64_t characterId = 0;
    if (!reader.string(decoded.sessionToken) || decoded.sessionToken.empty() || decoded.sessionToken.size() > 96 ||
        !reader.u64(characterId) || characterId == 0 ||
        characterId > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) || !reader.done())
        return false;
    decoded.characterId = static_cast<std::int64_t>(characterId);
    packet = std::move(decoded);
    return true;
}

std::vector<std::uint8_t> encodeDungeonStateSync(const DungeonStateSyncPacket& packet) {
    if (packet.result > DungeonResult::ServerError || !std::isfinite(packet.timerRemaining) ||
        packet.timerRemaining < 0.0F || !std::isfinite(packet.bossHealth) || packet.bossHealth < 0.0F ||
        !std::isfinite(packet.bossMaxHealth) || packet.bossMaxHealth < 0.0F ||
        (packet.instanceId != 0 && (packet.dungeonId == 0 || packet.bossMaxHealth <= 0.0F ||
            packet.bossHealth > packet.bossMaxHealth)) ||
        (packet.isCompleted && packet.isFailed)) throw std::invalid_argument("Invalid dungeon state");
    AuthWriter writer;
    writer.byte(static_cast<std::uint8_t>(packet.result)); writer.u32(packet.dungeonId);
    writer.u64(packet.instanceId); writer.f32(packet.timerRemaining);
    writer.byte(packet.isCompleted ? 1 : 0); writer.byte(packet.isFailed ? 1 : 0);
    writer.f32(packet.bossHealth); writer.f32(packet.bossMaxHealth);
    auto bytes = std::move(writer).finish(); validateSocialPacketSize(bytes); return bytes;
}

bool decodeDungeonStateSync(std::span<const std::uint8_t> bytes, DungeonStateSyncPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader reader(bytes); DungeonStateSyncPacket decoded; std::uint8_t result = 0, completed = 0, failed = 0;
    if (!reader.byte(result) || result > static_cast<std::uint8_t>(DungeonResult::ServerError) ||
        !reader.u32(decoded.dungeonId) || !reader.u64(decoded.instanceId) ||
        !reader.f32(decoded.timerRemaining) || !reader.byte(completed) || completed > 1 ||
        !reader.byte(failed) || failed > 1 || !reader.f32(decoded.bossHealth) ||
        !reader.f32(decoded.bossMaxHealth) || !reader.done() || !std::isfinite(decoded.timerRemaining) ||
        decoded.timerRemaining < 0.0F || !std::isfinite(decoded.bossHealth) || decoded.bossHealth < 0.0F ||
        !std::isfinite(decoded.bossMaxHealth) || decoded.bossMaxHealth < 0.0F ||
        (decoded.instanceId != 0 && (decoded.dungeonId == 0 || decoded.bossMaxHealth <= 0.0F ||
            decoded.bossHealth > decoded.bossMaxHealth)) || (completed && failed)) return false;
    decoded.result = static_cast<DungeonResult>(result);
    decoded.isCompleted = completed != 0; decoded.isFailed = failed != 0;
    packet = decoded;
    return true;
}

std::vector<std::uint8_t> encodeMarketBrowseRequest(const MarketBrowseRequestPacket& packet) {
    if (packet.pageSize == 0 || packet.pageSize > 20 || packet.itemType.size() > 32 || packet.search.size() > 64)
        throw std::invalid_argument("Invalid market browse request");
    AuthWriter w; w.string(packet.sessionToken); w.u64(static_cast<std::uint64_t>(packet.characterId));
    w.string(packet.itemType); w.string(packet.search); w.u32(packet.page); w.u32(packet.pageSize);
    auto bytes = std::move(w).finish(); validateSocialPacketSize(bytes); return bytes;
}
bool decodeMarketBrowseRequest(std::span<const std::uint8_t> bytes, MarketBrowseRequestPacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader r(bytes); MarketBrowseRequestPacket p; std::uint64_t id = 0;
    if (!r.string(p.sessionToken) || !r.u64(id) || !r.string(p.itemType) || !r.string(p.search) ||
        !r.u32(p.page) || !r.u32(p.pageSize) || !r.done() || id == 0 || p.pageSize == 0 ||
        p.pageSize > 20 || p.itemType.size() > 32 || p.search.size() > 64) return false;
    p.characterId = static_cast<std::int64_t>(id); packet = std::move(p); return true;
}
std::vector<std::uint8_t> encodeMarketBrowseResponse(const MarketBrowseResponsePacket& packet) {
    if (packet.result > MarketResultPacket::ServerError || packet.listings.size() > 20)
        throw std::invalid_argument("Invalid market response");
    AuthWriter w; w.byte(static_cast<std::uint8_t>(packet.result)); w.byte(static_cast<std::uint8_t>(packet.listings.size()));
    for (const auto& item : packet.listings) {
        if (item.quantity <= 0 || item.unitPrice <= 0) throw std::invalid_argument("Invalid market listing");
        w.u64(static_cast<std::uint64_t>(item.listingId)); w.u64(static_cast<std::uint64_t>(item.sellerCharacterId));
        w.string(item.sellerName); w.string(item.itemId); w.string(item.itemName); w.string(item.itemType);
        w.u32(static_cast<std::uint32_t>(item.quantity)); w.u64(static_cast<std::uint64_t>(item.unitPrice));
    }
    auto bytes = std::move(w).finish(); validateSocialPacketSize(bytes); return bytes;
}
bool decodeMarketBrowseResponse(std::span<const std::uint8_t> bytes, MarketBrowseResponsePacket& packet) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader r(bytes); MarketBrowseResponsePacket p; std::uint8_t result = 0, count = 0;
    if (!r.byte(result) || result > static_cast<std::uint8_t>(MarketResultPacket::ServerError) ||
        !r.byte(count) || count > 20) return false;
    p.result = static_cast<MarketResultPacket>(result);
    for (std::uint8_t i = 0; i < count; ++i) {
        MarketListingPacket item; std::uint64_t listing = 0, seller = 0, price = 0; std::uint32_t quantity = 0;
        if (!r.u64(listing) || !r.u64(seller) || !r.string(item.sellerName) || !r.string(item.itemId) ||
            !r.string(item.itemName) || !r.string(item.itemType) || !r.u32(quantity) || !r.u64(price) ||
            listing == 0 || seller == 0 || quantity == 0 || price == 0) return false;
        item.listingId = static_cast<std::int64_t>(listing); item.sellerCharacterId = static_cast<std::int64_t>(seller);
        item.quantity = static_cast<std::int32_t>(quantity); item.unitPrice = static_cast<std::int64_t>(price);
        p.listings.push_back(std::move(item));
    }
    if (!r.done()) return false; packet = std::move(p); return true;
}
std::vector<std::uint8_t> encodeMarketListRequest(const MarketListRequestPacket& p) {
    if (p.characterId <= 0 || p.itemId.empty() || p.itemId.size() > 64 || p.quantity <= 0 || p.unitPrice <= 0)
        throw std::invalid_argument("Invalid market list request");
    AuthWriter w; w.string(p.sessionToken); w.u64(static_cast<std::uint64_t>(p.characterId));
    w.string(p.itemId); w.u32(static_cast<std::uint32_t>(p.quantity)); w.u64(static_cast<std::uint64_t>(p.unitPrice));
    auto bytes = std::move(w).finish(); validateSocialPacketSize(bytes); return bytes;
}
bool decodeMarketListRequest(std::span<const std::uint8_t> bytes, MarketListRequestPacket& p) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader r(bytes); MarketListRequestPacket d; std::uint64_t character = 0, price = 0; std::uint32_t quantity = 0;
    if (!r.string(d.sessionToken) || !r.u64(character) || !r.string(d.itemId) || !r.u32(quantity) ||
        !r.u64(price) || !r.done() || character == 0 || d.itemId.empty() || d.itemId.size() > 64 ||
        quantity == 0 || quantity > 100000 || price == 0 || price > INT64_MAX) return false;
    d.characterId = static_cast<std::int64_t>(character); d.quantity = static_cast<std::int32_t>(quantity);
    d.unitPrice = static_cast<std::int64_t>(price); p = std::move(d); return true;
}
std::vector<std::uint8_t> encodeMarketActionRequest(const MarketActionRequestPacket& p) {
    if (p.characterId <= 0 || p.listingId <= 0) throw std::invalid_argument("Invalid market action");
    AuthWriter w; w.string(p.sessionToken); w.u64(static_cast<std::uint64_t>(p.characterId));
    w.u64(static_cast<std::uint64_t>(p.listingId)); return std::move(w).finish();
}
bool decodeMarketActionRequest(std::span<const std::uint8_t> bytes, MarketActionRequestPacket& p) {
    if (bytes.size() > maxSocialPacketSize) return false;
    AuthReader r(bytes); MarketActionRequestPacket d; std::uint64_t character = 0, listing = 0;
    if (!r.string(d.sessionToken) || !r.u64(character) || !r.u64(listing) || !r.done() || !character || !listing)
        return false;
    d.characterId = static_cast<std::int64_t>(character); d.listingId = static_cast<std::int64_t>(listing);
    p = std::move(d); return true;
}
std::vector<std::uint8_t> encodeMarketActionResponse(const MarketActionResponsePacket& p) {
    if (p.result > MarketResultPacket::ServerError || p.gold < 0) throw std::invalid_argument("Invalid market result");
    AuthWriter w; w.byte(static_cast<std::uint8_t>(p.result)); w.u64(static_cast<std::uint64_t>(p.gold));
    return std::move(w).finish();
}
bool decodeMarketActionResponse(std::span<const std::uint8_t> bytes, MarketActionResponsePacket& p) {
    AuthReader r(bytes); MarketActionResponsePacket d; std::uint8_t result = 0; std::uint64_t gold = 0;
    if (!r.byte(result) || result > static_cast<std::uint8_t>(MarketResultPacket::ServerError) ||
        !r.u64(gold) || !r.done() || gold > INT64_MAX) return false;
    d.result = static_cast<MarketResultPacket>(result); d.gold = static_cast<std::int64_t>(gold); p = d; return true;
}
std::vector<std::uint8_t> encodeCharacterGoldSync(const CharacterGoldSyncPacket& p) {
    if (p.characterId <= 0 || p.gold < 0) throw std::invalid_argument("Invalid character gold");
    AuthWriter w; w.u64(static_cast<std::uint64_t>(p.characterId)); w.u64(static_cast<std::uint64_t>(p.gold));
    return std::move(w).finish();
}
bool decodeCharacterGoldSync(std::span<const std::uint8_t> bytes, CharacterGoldSyncPacket& p) {
    AuthReader r(bytes); std::uint64_t id = 0, gold = 0;
    if (!r.u64(id) || !r.u64(gold) || !r.done() || id == 0 || id > INT64_MAX || gold > INT64_MAX) return false;
    p.characterId = static_cast<std::int64_t>(id); p.gold = static_cast<std::int64_t>(gold); return true;
}

SnapshotInterpolator::SnapshotInterpolator(std::size_t capacity, double interpolationDelayMs,
    double maxExtrapolationMs)
    : capacity_(std::max<std::size_t>(2, capacity)),
      interpolationDelayMs_(std::isfinite(interpolationDelayMs)
          ? std::max(0.0, interpolationDelayMs) : 75.0),
      maxExtrapolationMs_(std::isfinite(maxExtrapolationMs)
          ? std::max(0.0, maxExtrapolationMs) : 100.0) {}

bool SnapshotInterpolator::addSnapshot(const ServerSnapshotPacket& snapshot, double receivedAtMilliseconds) {
    if (!std::isfinite(receivedAtMilliseconds)) return false;
    try {
        (void)encodeServerSnapshot(snapshot);
    } catch (const std::exception&) {
        return false;
    }
    const auto position = std::lower_bound(snapshots_.begin(), snapshots_.end(), snapshot.serverTick,
        [](const TimedSnapshot& entry, std::uint64_t tick) { return entry.packet.serverTick < tick; });
    if (position != snapshots_.end() && position->packet.serverTick == snapshot.serverTick) {
        *position = {snapshot, receivedAtMilliseconds};
    } else {
        snapshots_.insert(position, {snapshot, receivedAtMilliseconds});
    }
    while (snapshots_.size() > capacity_) snapshots_.erase(snapshots_.begin());
    return true;
}

std::vector<InterpolatedRemotePlayer> SnapshotInterpolator::sample(std::uint32_t localClientId,
    const SnapshotVector3& localPosition, double nowMilliseconds) const {
    std::vector<InterpolatedRemotePlayer> result;
    if (snapshots_.empty() || !std::isfinite(nowMilliseconds)) return result;
    const TimedSnapshot& latest = snapshots_.back();
    const double elapsedMs = std::max(0.0, nowMilliseconds - latest.receivedAtMilliseconds);
    const double renderTick = static_cast<double>(latest.packet.serverTick) + elapsedMs * 0.06 -
        interpolationDelayMs_ * 0.06;

    const auto findPlayer = [](const ServerSnapshotPacket& packet, std::uint32_t clientId)
        -> const SnapshotPlayer* {
        const auto found = std::find_if(packet.players.begin(), packet.players.end(),
            [clientId](const SnapshotPlayer& player) { return player.clientId == clientId; });
        return found == packet.players.end() ? nullptr : &*found;
    };
    const auto lerp = [](float first, float second, float alpha) {
        return first + (second - first) * alpha;
    };
    const auto lerpVector = [&lerp](const SnapshotVector3& first, const SnapshotVector3& second, float alpha) {
        return SnapshotVector3{lerp(first.x, second.x, alpha), lerp(first.y, second.y, alpha),
            lerp(first.z, second.z, alpha)};
    };
    const auto lerpYaw = [&lerp](float first, float second, float alpha) {
        const float shortestDelta = std::remainder(second - first, 360.0F);
        float angle = first + shortestDelta * alpha;
        angle = std::fmod(angle, 360.0F);
        if (angle < 0.0F) angle += 360.0F;
        return angle;
    };

    result.reserve(latest.packet.players.size());
    for (const SnapshotPlayer& newestPlayer : latest.packet.players) {
        if (newestPlayer.clientId == localClientId) continue;
        SnapshotPlayer interpolated = newestPlayer;
        if (renderTick < static_cast<double>(snapshots_.front().packet.serverTick)) {
            if (const SnapshotPlayer* earliest = findPlayer(snapshots_.front().packet, newestPlayer.clientId))
                interpolated = *earliest;
        } else if (renderTick > static_cast<double>(latest.packet.serverTick)) {
            const double extrapolationMs = std::min(maxExtrapolationMs_,
                (renderTick - static_cast<double>(latest.packet.serverTick)) * (1000.0 / 60.0));
            const float seconds = static_cast<float>(extrapolationMs / 1000.0);
            interpolated.position.x += newestPlayer.velocity.x * seconds;
            interpolated.position.y += newestPlayer.velocity.y * seconds;
            interpolated.position.z += newestPlayer.velocity.z * seconds;
        } else {
            const auto upper = std::lower_bound(snapshots_.begin(), snapshots_.end(), renderTick,
                [](const TimedSnapshot& entry, double tick) {
                    return static_cast<double>(entry.packet.serverTick) < tick;
                });
            if (upper != snapshots_.end() && upper != snapshots_.begin()) {
                const TimedSnapshot& firstSnapshot = *(upper - 1);
                const TimedSnapshot& secondSnapshot = *upper;
                const SnapshotPlayer* first = findPlayer(firstSnapshot.packet, newestPlayer.clientId);
                const SnapshotPlayer* second = findPlayer(secondSnapshot.packet, newestPlayer.clientId);
                if (first && second) {
                    const double tickSpan = static_cast<double>(secondSnapshot.packet.serverTick -
                        firstSnapshot.packet.serverTick);
                    const float alpha = tickSpan > 0.0 ? static_cast<float>(std::clamp(
                        (renderTick - static_cast<double>(firstSnapshot.packet.serverTick)) / tickSpan,
                        0.0, 1.0)) : 1.0F;
                    interpolated = alpha < 0.5F ? *first : *second;
                    interpolated.position = lerpVector(first->position, second->position, alpha);
                    interpolated.yaw = lerpYaw(first->yaw, second->yaw, alpha);
                    interpolated.velocity = lerpVector(first->velocity, second->velocity, alpha);
                } else if (first) interpolated = *first;
                else if (second) interpolated = *second;
            } else if (upper != snapshots_.end()) {
                if (const SnapshotPlayer* next = findPlayer(upper->packet, newestPlayer.clientId))
                    interpolated = *next;
            }
        }
        const float dx = interpolated.position.x - localPosition.x;
        const float dy = interpolated.position.y - localPosition.y;
        const float dz = interpolated.position.z - localPosition.z;
        result.push_back({std::move(interpolated), std::sqrt(dx * dx + dy * dy + dz * dz)});
    }
    return result;
}

void SnapshotInterpolator::clear() noexcept { snapshots_.clear(); }

double SnapshotInterpolator::monotonicMilliseconds() noexcept {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0;
}

NetworkContext::~NetworkContext() {
    std::lock_guard lock(contextMutex);
    if (contextCount > 0 && --contextCount == 0) enet_deinitialize();
}

NetworkServer::NetworkServer(NetworkContext&, std::uint16_t port, std::size_t maxClients) : port_(port) {
    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;
    host_ = enet_host_create(&address, maxClients, 2, 0, 0);
    if (!host_) throw std::runtime_error("Unable to create ENet server host");
    if (port_ == 0) port_ = host_->address.port;
}

NetworkServer::~NetworkServer() { if (host_) enet_host_destroy(host_); }

std::vector<NetworkEvent> NetworkServer::poll(std::uint32_t timeoutMs) {
    std::vector<NetworkEvent> events;
    ENetEvent event{};
    while (enet_host_service(host_, &event, timeoutMs) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            peers_.push_back({event.peer, nextClientId_++, false});
            events.push_back({EventType::Connected, peers_.back().id});
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            PacketType type{};
            std::vector<std::uint8_t> payload;
            auto found = std::find_if(peers_.begin(), peers_.end(), [&](const auto& entry) {
                return entry.peer == event.peer;
            });
            if (found != peers_.end() && parsePacket(event.packet, type, payload)) {
                if (type == PacketType::ConnectRequest && payload.empty() && !found->accepted) {
                    found->accepted = true;
                    const std::array<std::uint8_t, 4> idBytes{
                        static_cast<std::uint8_t>(found->id), static_cast<std::uint8_t>(found->id >> 8),
                        static_cast<std::uint8_t>(found->id >> 16), static_cast<std::uint8_t>(found->id >> 24)};
                    send(found->id, PacketType::ConnectAccepted, idBytes, true);
                } else if (type == PacketType::Ping && payload.size() == 8) {
                    send(found->id, PacketType::Pong, payload, true);
                } else if (found->accepted) {
                    events.push_back({EventType::PacketReceived, found->id, type, std::move(payload)});
                }
            }
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            auto found = std::find_if(peers_.begin(), peers_.end(), [&](const auto& entry) {
                return entry.peer == event.peer;
            });
            if (found != peers_.end()) {
                events.push_back({EventType::Disconnected, found->id});
                peers_.erase(found);
            }
        }
        timeoutMs = 0;
    }
    return events;
}

bool NetworkServer::send(std::uint32_t clientId, PacketType type,
    std::span<const std::uint8_t> payload, bool reliable) {
    auto found = std::find_if(peers_.begin(), peers_.end(), [clientId](const auto& entry) {
        return entry.id == clientId && entry.accepted;
    });
    if (found == peers_.end()) return false;
    ENetPacket* packet = makePacket(type, payload, reliable);
    if (enet_peer_send(found->peer, reliable ? reliableChannel : sequencedChannel, packet) != 0) {
        enet_packet_destroy(packet);
        return false;
    }
    return true;
}

void NetworkServer::broadcast(PacketType type, std::span<const std::uint8_t> payload, bool reliable) {
    for (const PeerEntry& peer : peers_)
        if (peer.accepted) (void)send(peer.id, type, payload, reliable);
}

void NetworkServer::disconnect(std::uint32_t clientId) {
    auto found = std::find_if(peers_.begin(), peers_.end(), [clientId](const auto& entry) {
        return entry.id == clientId;
    });
    if (found != peers_.end()) enet_peer_disconnect_later(found->peer, 0);
}

NetworkClient::NetworkClient(NetworkContext&) {
    host_ = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host_) throw std::runtime_error("Unable to create ENet client host");
}

NetworkClient::~NetworkClient() {
    if (peer_) enet_peer_reset(peer_);
    if (host_) enet_host_destroy(host_);
}

bool NetworkClient::connect(const std::string& host, std::uint16_t port) {
    if (state_ != ConnectionState::Disconnected) return false;
    ENetAddress address{};
    if (enet_address_set_host(&address, host.c_str()) != 0) return false;
    address.port = port;
    peer_ = enet_host_connect(host_, &address, 2, 0);
    if (!peer_) return false;
    state_ = ConnectionState::Connecting;
    return true;
}

std::vector<NetworkEvent> NetworkClient::poll(std::uint32_t timeoutMs) {
    std::vector<NetworkEvent> events;
    ENetEvent event{};
    while (enet_host_service(host_, &event, timeoutMs) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            const std::array<std::uint8_t, 0> empty{};
            send(PacketType::ConnectRequest, empty, true);
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            PacketType type{};
            std::vector<std::uint8_t> payload;
            if (parsePacket(event.packet, type, payload)) {
                if (type == PacketType::ConnectAccepted && payload.size() == 4) {
                    clientId_ = static_cast<std::uint32_t>(payload[0]) |
                        (static_cast<std::uint32_t>(payload[1]) << 8) |
                        (static_cast<std::uint32_t>(payload[2]) << 16) |
                        (static_cast<std::uint32_t>(payload[3]) << 24);
                    state_ = ConnectionState::Connected;
                    events.push_back({EventType::Connected, clientId_});
                } else if (type == PacketType::Pong && payload.size() == 8) {
                    std::uint64_t sentAt = 0;
                    for (unsigned i = 0; i < 8; ++i) sentAt |= static_cast<std::uint64_t>(payload[i]) << (i * 8);
                    rttMs_ = static_cast<std::uint32_t>(nowMilliseconds() - sentAt);
                    events.push_back({EventType::PacketReceived, clientId_, type, std::move(payload)});
                } else {
                    events.push_back({EventType::PacketReceived, clientId_, type, std::move(payload)});
                }
            }
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            state_ = ConnectionState::Disconnected;
            clientId_ = 0;
            peer_ = nullptr;
            events.push_back({EventType::Disconnected});
        }
        timeoutMs = 0;
    }
    return events;
}

bool NetworkClient::send(PacketType type, std::span<const std::uint8_t> payload, bool reliable) {
    if (!peer_ || state_ != ConnectionState::Connected || type == PacketType::ConnectRequest) {
        if (!peer_ || (type != PacketType::ConnectRequest && state_ != ConnectionState::Connected)) return false;
    }
    ENetPacket* packet = makePacket(type, payload, reliable);
    if (enet_peer_send(peer_, reliable ? reliableChannel : sequencedChannel, packet) != 0) {
        enet_packet_destroy(packet);
        return false;
    }
    return true;
}

void NetworkClient::sendPing() {
    const std::uint64_t stamp = nowMilliseconds();
    std::array<std::uint8_t, 8> payload{};
    for (unsigned i = 0; i < 8; ++i) payload[i] = static_cast<std::uint8_t>(stamp >> (i * 8));
    send(PacketType::Ping, payload, false);
}

void NetworkClient::disconnect() {
    if (peer_) enet_peer_disconnect(peer_, 0);
}

} // namespace glory::network

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace glory::game {

enum class ChatChannel : std::uint8_t { World, Zone, Party, Guild, Whisper, System };

struct ChatIdentity {
    std::int64_t characterId = 0;
    std::string name;
    std::uint8_t zone = 0;
    std::uint64_t partyId = 0;
    std::uint64_t guildId = 0;
    bool connected = false;
};

struct ChatMessage {
    ChatChannel channel = ChatChannel::World;
    std::string senderName;
    std::string targetName;
    std::string text;
};

[[nodiscard]] bool isValidChatText(const std::string& text) noexcept;
[[nodiscard]] std::vector<std::int64_t> routeChat(ChatChannel channel, std::int64_t senderId,
    std::int64_t targetId, std::span<const ChatIdentity> identities);

struct PartyMember {
    std::int64_t characterId = 0;
    std::string name;
    std::string className;
    int level = 1;
    float currentHp = 100.0F;
    float maxHp = 100.0F;
    bool leader = false;
};

class PartyManager {
public:
    static constexpr std::size_t maxMembers = 5;

    [[nodiscard]] std::optional<std::uint64_t> createParty(const PartyMember& leader);
    [[nodiscard]] bool invite(std::int64_t leaderId, std::int64_t targetId);
    [[nodiscard]] bool accept(std::int64_t characterId, const PartyMember& member);
    [[nodiscard]] bool leave(std::int64_t characterId);
    [[nodiscard]] bool kick(std::int64_t leaderId, std::int64_t targetId);
    void cancelInvitation(std::int64_t characterId) noexcept;
    [[nodiscard]] bool updateVitals(std::int64_t characterId, float currentHp, float maxHp);
    [[nodiscard]] std::uint64_t partyFor(std::int64_t characterId) const noexcept;
    [[nodiscard]] bool hasInvitation(std::int64_t characterId) const noexcept;
    [[nodiscard]] std::uint64_t invitationFor(std::int64_t characterId) const noexcept;
    [[nodiscard]] std::vector<PartyMember> members(std::int64_t characterId) const;
    [[nodiscard]] std::vector<PartyMember> membersForParty(std::uint64_t partyId) const;

private:
    struct Party { std::uint64_t id = 0; std::vector<PartyMember> members; };
    [[nodiscard]] bool removeMember(std::uint64_t partyId, std::int64_t characterId);

    std::uint64_t nextPartyId_ = 1;
    std::unordered_map<std::uint64_t, Party> parties_;
    std::unordered_map<std::int64_t, std::uint64_t> characterParties_;
    std::unordered_map<std::int64_t, std::uint64_t> invitations_;
};

[[nodiscard]] bool isValidChatChannel(ChatChannel channel) noexcept;

} // namespace glory::game

#include "glory/game/social.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace glory::game {
namespace {

bool validMember(const PartyMember& member) {
    return member.characterId > 0 && !member.name.empty() && member.name.size() <= 48 &&
        member.className.size() <= 32 && member.level > 0 && std::isfinite(member.currentHp) &&
        std::isfinite(member.maxHp) && member.maxHp > 0.0F && member.currentHp >= 0.0F &&
        member.currentHp <= member.maxHp;
}

} // namespace

bool isValidChatText(const std::string& text) noexcept {
    if (text.empty() || text.size() > 768) return false;
    std::size_t characters = 0;
    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<unsigned char>(text[index]);
        std::size_t length = 0;
        std::uint32_t codepoint = 0;
        if (first <= 0x7f) { length = 1; codepoint = first; }
        else if ((first & 0xe0) == 0xc0) { length = 2; codepoint = first & 0x1f; }
        else if ((first & 0xf0) == 0xe0) { length = 3; codepoint = first & 0x0f; }
        else if ((first & 0xf8) == 0xf0) { length = 4; codepoint = first & 0x07; }
        else return false;
        if (index + length > text.size()) return false;
        for (std::size_t part = 1; part < length; ++part) {
            const auto continuation = static_cast<unsigned char>(text[index + part]);
            if ((continuation & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (continuation & 0x3f);
        }
        if ((length == 2 && codepoint < 0x80) || (length == 3 && codepoint < 0x800) ||
            (length == 4 && codepoint < 0x10000) || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint == 0) return false;
        index += length;
        if (++characters > 256) return false;
    }
    return true;
}

bool isValidChatChannel(ChatChannel channel) noexcept {
    return static_cast<std::uint8_t>(channel) <= static_cast<std::uint8_t>(ChatChannel::System);
}

std::vector<std::int64_t> routeChat(ChatChannel channel, std::int64_t senderId,
    std::int64_t targetId, std::span<const ChatIdentity> identities) {
    std::vector<std::int64_t> recipients;
    if (!isValidChatChannel(channel) || (senderId <= 0 && channel != ChatChannel::System)) return recipients;
    const auto sender = std::find_if(identities.begin(), identities.end(), [senderId](const ChatIdentity& item) {
        return item.characterId == senderId && item.connected;
    });
    if (sender == identities.end() && channel != ChatChannel::System) return recipients;
    const auto target = std::find_if(identities.begin(), identities.end(), [targetId](const ChatIdentity& item) {
        return item.characterId == targetId && item.connected;
    });
    for (const auto& item : identities) {
        if (!item.connected || item.characterId <= 0) continue;
        bool include = false;
        switch (channel) {
        case ChatChannel::World:
        case ChatChannel::System: include = true; break;
        case ChatChannel::Zone: include = sender != identities.end() && item.zone == sender->zone; break;
        case ChatChannel::Party:
            include = sender != identities.end() && sender->partyId != 0 && item.partyId == sender->partyId;
            break;
        case ChatChannel::Guild:
            include = sender != identities.end() && sender->guildId != 0 && item.guildId == sender->guildId;
            break;
        case ChatChannel::Whisper:
            include = (item.characterId == senderId) || (target != identities.end() && item.characterId == targetId);
            break;
        }
        if (include) recipients.push_back(item.characterId);
    }
    std::sort(recipients.begin(), recipients.end());
    recipients.erase(std::unique(recipients.begin(), recipients.end()), recipients.end());
    return recipients;
}

std::optional<std::uint64_t> PartyManager::createParty(const PartyMember& leader) {
    if (!validMember(leader) || characterParties_.contains(leader.characterId) || nextPartyId_ == 0) return std::nullopt;
    const std::uint64_t id = nextPartyId_++;
    Party party{id, {leader}};
    party.members.front().leader = true;
    parties_.emplace(id, std::move(party));
    characterParties_[leader.characterId] = id;
    invitations_.erase(leader.characterId);
    return id;
}

bool PartyManager::invite(std::int64_t leaderId, std::int64_t targetId) {
    if (targetId <= 0 || targetId == leaderId || characterParties_.contains(targetId)) return false;
    const auto membership = characterParties_.find(leaderId);
    if (membership == characterParties_.end()) return false;
    const auto party = parties_.find(membership->second);
    if (party == parties_.end() || party->second.members.empty() ||
        party->second.members.front().characterId != leaderId ||
        party->second.members.size() >= maxMembers) return false;
    invitations_[targetId] = membership->second;
    return true;
}

bool PartyManager::accept(std::int64_t characterId, const PartyMember& member) {
    const auto invitation = invitations_.find(characterId);
    if (invitation == invitations_.end() || !validMember(member) || member.characterId != characterId ||
        characterParties_.contains(characterId)) return false;
    const auto party = parties_.find(invitation->second);
    if (party == parties_.end() || party->second.members.size() >= maxMembers) {
        invitations_.erase(invitation);
        return false;
    }
    PartyMember added = member;
    added.leader = false;
    party->second.members.push_back(std::move(added));
    characterParties_[characterId] = party->first;
    invitations_.erase(invitation);
    return true;
}

bool PartyManager::removeMember(std::uint64_t partyId, std::int64_t characterId) {
    auto party = parties_.find(partyId);
    if (party == parties_.end()) return false;
    auto& members = party->second.members;
    const auto member = std::find_if(members.begin(), members.end(), [characterId](const PartyMember& item) {
        return item.characterId == characterId;
    });
    if (member == members.end()) return false;
    const bool wasLeader = member->leader;
    members.erase(member);
    characterParties_.erase(characterId);
    if (members.empty()) {
        parties_.erase(party);
        for (auto it = invitations_.begin(); it != invitations_.end();) {
            if (it->second == partyId) it = invitations_.erase(it); else ++it;
        }
    } else if (wasLeader) {
        members.front().leader = true;
    }
    return true;
}

bool PartyManager::leave(std::int64_t characterId) {
    const auto membership = characterParties_.find(characterId);
    return membership != characterParties_.end() && removeMember(membership->second, characterId);
}

bool PartyManager::kick(std::int64_t leaderId, std::int64_t targetId) {
    const auto leaderParty = characterParties_.find(leaderId);
    const auto targetParty = characterParties_.find(targetId);
    if (leaderParty == characterParties_.end() || targetParty == characterParties_.end() ||
        leaderParty->second != targetParty->second || leaderId == targetId) return false;
    const auto party = parties_.find(leaderParty->second);
    if (party == parties_.end() || party->second.members.empty() ||
        party->second.members.front().characterId != leaderId) return false;
    invitations_.erase(targetId);
    return removeMember(leaderParty->second, targetId);
}

void PartyManager::cancelInvitation(std::int64_t characterId) noexcept {
    invitations_.erase(characterId);
}

bool PartyManager::updateVitals(std::int64_t characterId, float currentHp, float maxHp) {
    const auto membership = characterParties_.find(characterId);
    if (membership == characterParties_.end() || !std::isfinite(currentHp) || !std::isfinite(maxHp) ||
        maxHp <= 0.0F || currentHp < 0.0F || currentHp > maxHp) return false;
    auto& party = parties_.at(membership->second);
    const auto member = std::find_if(party.members.begin(), party.members.end(), [characterId](const PartyMember& item) {
        return item.characterId == characterId;
    });
    if (member == party.members.end()) return false;
    member->currentHp = currentHp;
    member->maxHp = maxHp;
    return true;
}

std::uint64_t PartyManager::partyFor(std::int64_t characterId) const noexcept {
    const auto found = characterParties_.find(characterId);
    return found == characterParties_.end() ? 0 : found->second;
}

bool PartyManager::hasInvitation(std::int64_t characterId) const noexcept {
    return invitations_.contains(characterId);
}

std::uint64_t PartyManager::invitationFor(std::int64_t characterId) const noexcept {
    const auto found = invitations_.find(characterId);
    return found == invitations_.end() ? 0 : found->second;
}

std::vector<PartyMember> PartyManager::members(std::int64_t characterId) const {
    const auto membership = characterParties_.find(characterId);
    if (membership == characterParties_.end()) return {};
    const auto party = parties_.find(membership->second);
    return party == parties_.end() ? std::vector<PartyMember>{} : party->second.members;
}

std::vector<PartyMember> PartyManager::membersForParty(std::uint64_t partyId) const {
    const auto found = parties_.find(partyId);
    return found == parties_.end() ? std::vector<PartyMember>{} : found->second.members;
}

} // namespace glory::game

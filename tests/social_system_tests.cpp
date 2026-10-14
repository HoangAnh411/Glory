#include "glory/game/social.hpp"
#include "glory/network/network.hpp"
#include "glory/server/account_database.hpp"

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testChatRouting() {
    using namespace glory::game;
    const std::vector<ChatIdentity> roster{
        {1, "A", 0, 10, 20, true}, {2, "B", 0, 10, 20, true},
        {3, "C", 1, 0, 20, true}, {4, "D", 1, 0, 0, true}, {5, "Offline", 0, 10, 20, false}};
    require(isValidChatText("hello") && isValidChatText("xin chào"), "valid UTF-8 chat text accepted");
    require(!isValidChatText("") && !isValidChatText(std::string(257, 'x')) &&
        !isValidChatText(std::string("\xc0\xaf", 2)), "empty, long and malformed chat text rejected");
    require(routeChat(ChatChannel::World, 1, 0, roster) == std::vector<std::int64_t>({1, 2, 3, 4}),
        "world chat routes to all connected players");
    require(routeChat(ChatChannel::Zone, 1, 0, roster) == std::vector<std::int64_t>({1, 2}),
        "zone chat routes only to same zone");
    require(routeChat(ChatChannel::Party, 1, 0, roster) == std::vector<std::int64_t>({1, 2}),
        "party chat routes only to same party");
    require(routeChat(ChatChannel::Guild, 1, 0, roster) == std::vector<std::int64_t>({1, 2, 3}),
        "guild chat routes only to same guild");
    require(routeChat(ChatChannel::Whisper, 1, 4, roster) == std::vector<std::int64_t>({1, 4}),
        "whisper routes only to sender and recipient");
    require(routeChat(ChatChannel::System, 0, 0, roster) == std::vector<std::int64_t>({1, 2, 3, 4}),
        "server system messages route to all connected players");
}

void testPartyLifecycle() {
    using namespace glory::game;
    PartyManager parties;
    PartyMember leader{11, "Leader", "Vanguard", 8, 100.0F, 100.0F};
    PartyMember member{12, "Member", "Arcanist", 7, 70.0F, 100.0F};
    const auto partyId = parties.createParty(leader);
    require(partyId.has_value() && parties.partyFor(11) == *partyId, "party can be created");
    require(parties.invite(11, 12) && parties.hasInvitation(12) && parties.invitationFor(12) == *partyId,
        "leader can invite player");
    require(parties.accept(12, member) && parties.partyFor(12) == *partyId,
        "invitee can accept party invite");
    require(parties.updateVitals(12, 25.0F, 100.0F) && parties.members(11)[1].currentHp == 25.0F,
        "party sync state includes updated HP");
    require(!parties.kick(12, 11) && parties.kick(11, 12), "only leader can kick a member");
    require(parties.invite(11, 12) && parties.accept(12, member), "member can rejoin after kick");
    require(parties.leave(11) && parties.partyFor(11) == 0 && parties.members(12).front().leader,
        "party leader leaving promotes the next member");
    require(parties.leave(12) && parties.members(12).empty(), "last member leaving destroys party");
    const auto secondParty = parties.createParty(leader);
    require(secondParty && parties.invite(11, 12), "party invitation can be created again");
    parties.cancelInvitation(12);
    require(!parties.hasInvitation(12), "disconnect cleanup cancels an online-only party invitation");
}

void testGuildPersistence() {
    using namespace glory::network;
    const auto databasePath = std::filesystem::temp_directory_path() /
        ("glory_social_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite3");
    std::int64_t leaderId = 0, memberId = 0;
    {
        glory::server::AccountDatabase database(databasePath.string());
        require(database.registerAccount("social_owner", "long-password") == AuthResult::Success,
            "guild owner account registers");
        const auto ownerLogin = database.authenticateAccount("social_owner", "long-password");
        const auto leader = database.createCharacter(ownerLogin.sessionToken, "GuildLeader", "Vanguard");
        const auto member = database.createCharacter(ownerLogin.sessionToken, "GuildMember", "Arcanist");
        require(leader.result == AuthResult::Success && member.result == AuthResult::Success,
            "guild characters created");
        leaderId = leader.character.characterId;
        memberId = member.character.characterId;
        require(database.createGuild(ownerLogin.sessionToken, leaderId, "Starlight") == SocialResult::Success,
            "guild creation succeeds");
        require(database.createGuild(ownerLogin.sessionToken, memberId, "Starlight") == SocialResult::GuildExists,
            "duplicate guild name rejected");
        require(database.inviteGuildMember(ownerLogin.sessionToken, leaderId, memberId) == SocialResult::Success,
            "guild leader can invite a character");
        require(database.acceptGuildInvite(ownerLogin.sessionToken, memberId,
            database.guildForCharacter(leaderId).guildId) == SocialResult::Success,
            "invited character accepts membership");
        require(database.setGuildRank(ownerLogin.sessionToken, leaderId, memberId, GuildRank::Officer) ==
            SocialResult::Success, "guild leader promotes member");
        const auto guild = database.guildForCharacter(memberId);
        require(guild.guildName == "Starlight" && guild.members.size() == 2 &&
            guild.memberRank == GuildRank::Officer, "guild sync includes persisted rank and members");
        require(database.leaveGuild(ownerLogin.sessionToken, leaderId) == SocialResult::Success,
            "guild leader can leave");
        const auto promoted = database.guildForCharacter(memberId);
        require(promoted.memberRank == GuildRank::Leader && promoted.members.size() == 1,
            "guild leader transfers to remaining member");
    }
    {
        glory::server::AccountDatabase reopened(databasePath.string());
        require(reopened.registerAccount("social_owner2", "long-password") == AuthResult::Success,
            "second account can use reopened database");
        const auto sync = reopened.guildForCharacter(memberId);
        require(sync.guildName == "Starlight" && sync.memberRank == GuildRank::Leader &&
            sync.members.size() == 1 && sync.members[0].characterId == memberId,
            "guild state survives database reopen");
        require(sync.result == SocialResult::Success, "reopened guild sync reports success");
    }
    std::error_code ignored;
    std::filesystem::remove(databasePath, ignored);
}

void testSocialCodecs() {
    using namespace glory::network;
    ChatMessagePacket chat{"token", 4, ChatChannel::Whisper, "Alice", "Bob", "hello"}, decodedChat;
    require(decodeChatMessage(encodeChatMessage(chat), decodedChat) && decodedChat.text == "hello" &&
        decodedChat.channel == ChatChannel::Whisper, "chat packet round-trip");
    const ChatMessagePacket relayed{{}, 4, ChatChannel::World, "Alice", {}, "hello"};
    require(decodeChatMessage(encodeChatMessage(relayed), decodedChat) && decodedChat.sessionToken.empty(),
        "server relayed chat does not expose a session token");
    PartyRequestPacket partyRequest{"token", 4, PartyAction::Invite, "Bob"}, decodedPartyRequest;
    require(decodePartyRequest(encodePartyRequest(partyRequest), decodedPartyRequest) &&
        decodedPartyRequest.action == PartyAction::Invite, "party request round-trip");
    PartySyncPacket party{SocialResult::Success, 9, false,
        {{4, "Alice", "Vanguard", 10, 90.0F, 100.0F, true}}}, decodedParty;
    require(decodePartySync(encodePartySync(party), decodedParty) && decodedParty.members.size() == 1 &&
        decodedParty.members[0].currentHp == 90.0F, "party sync round-trip");
    GuildRequestPacket guildRequest{"token", 4, GuildAction::SetRank, 9, "Guild", "Bob", GuildRank::Officer};
    GuildRequestPacket decodedGuildRequest;
    require(decodeGuildRequest(encodeGuildRequest(guildRequest), decodedGuildRequest) &&
        decodedGuildRequest.rank == GuildRank::Officer, "guild request round-trip");
    GuildSyncPacket guild{SocialResult::Success, 9, "Guild", GuildRank::Leader, false,
        {{4, "Alice", "Vanguard", 10, GuildRank::Leader}}}, decodedGuild;
    require(decodeGuildSync(encodeGuildSync(guild), decodedGuild) && decodedGuild.members.size() == 1 &&
        decodedGuild.guildName == "Guild", "guild sync round-trip");
    const auto malformed = encodePartySync(party);
    require(!decodePartySync({malformed.data(), malformed.size() - 1}, decodedParty),
        "truncated social packet rejected");
}

} // namespace

int main() {
    try {
        testChatRouting();
        testPartyLifecycle();
        testGuildPersistence();
        testSocialCodecs();
        std::cout << "Chat, party, guild persistence and codec tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

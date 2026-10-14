#include "glory/server/account_database.hpp"
#include "glory/network/network.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testDatabaseAndAccounts() {
    using glory::network::AuthResult;
    glory::server::AccountDatabase database(":memory:");
    require(database.registerAccount("kaede_01", "correct-horse") == AuthResult::Success,
        "registration should succeed and create account schema");
    require(database.registerAccount("kaede_01", "correct-horse") == AuthResult::UsernameTaken,
        "duplicate username should be rejected");
    require(database.registerAccount("ab", "short") == AuthResult::InvalidLength,
        "short credentials should be rejected");

    const auto validLogin = database.authenticateAccount("kaede_01", "correct-horse");
    require(validLogin.result == AuthResult::Success && validLogin.sessionToken.size() == 64,
        "valid credentials should issue a 256-bit session token");
    require(database.authenticateAccount("kaede_01", "wrong-password").result == AuthResult::BadCredentials,
        "incorrect password should fail authentication");
    require(database.authenticateAccount("missing_01", "correct-horse").result == AuthResult::AccountNotFound,
        "missing account should be reported");

    require(database.createCharacter("invalid-token", "Aster", "Vanguard").result == AuthResult::InvalidSession,
        "character creation must require a valid session");
    const auto created = database.createCharacter(validLogin.sessionToken, "Aster", "Vanguard");
    require(created.result == AuthResult::Success && created.character.level == 1 && created.character.exp == 0,
        "character should be created with default progression");
    require(database.createCharacter(validLogin.sessionToken, "Aster", "Ranger").result == AuthResult::CharacterNameTaken,
        "duplicate character names should be rejected");
    const auto listed = database.listCharacters(validLogin.sessionToken);
    require(listed.result == AuthResult::Success && listed.characters.size() == 1 &&
        listed.characters.front().name == "Aster", "account should list only its own characters");
    const auto other = database.registerAccount("kaede_02", "another-pass");
    require(other == AuthResult::Success, "second account should register");
    const auto otherLogin = database.authenticateAccount("kaede_02", "another-pass");
    require(database.listCharacters(otherLogin.sessionToken).characters.empty(),
        "character list should be scoped to the authenticated account");
    require(database.selectCharacter(validLogin.sessionToken, created.character.characterId).result == AuthResult::Success,
        "account owner should be able to enter with its character");
    require(database.selectCharacter(otherLogin.sessionToken, created.character.characterId).result == AuthResult::CharacterNotFound,
        "another account must not enter with a character it does not own");
    require(database.grantCharacterExperience(validLogin.sessionToken, created.character.characterId, 250) &&
        database.listCharacters(validLogin.sessionToken).characters.front().exp == 250,
        "dungeon reward experience should persist for the owning character");
    require(!database.grantCharacterExperience(otherLogin.sessionToken, created.character.characterId, 250),
        "experience rewards must reject a session that does not own the character");
}

void testAuthPacketRoundTrips() {
    using namespace glory::network;
    RegisterRequestPacket registration{"player_01", "password-123"}, decodedRegistration;
    auto bytes = encodeRegisterRequest(registration);
    require(decodeRegisterRequest(bytes, decodedRegistration) &&
        decodedRegistration.username == registration.username && decodedRegistration.password == registration.password,
        "register request should round-trip");
    require(!decodeRegisterRequest(std::span<const std::uint8_t>(bytes.data(), bytes.size() - 1), decodedRegistration),
        "truncated register request should be rejected");

    RegisterResponsePacket registerResponse{AuthResult::UsernameTaken}, decodedRegisterResponse;
    require(decodeRegisterResponse(encodeRegisterResponse(registerResponse), decodedRegisterResponse) &&
        decodedRegisterResponse.result == registerResponse.result, "register response should round-trip");

    LoginRequestPacket login{"player_01", "password-123"}, decodedLogin;
    require(decodeLoginRequest(encodeLoginRequest(login), decodedLogin) && decodedLogin.password == login.password,
        "login request should round-trip");
    LoginResponsePacket loginResponse{AuthResult::Success, std::string(64, 'a')}, decodedLoginResponse;
    require(decodeLoginResponse(encodeLoginResponse(loginResponse), decodedLoginResponse) &&
        decodedLoginResponse.sessionToken == loginResponse.sessionToken, "login response should round-trip");

    CharacterListRequestPacket listRequest{"session-token"}, decodedListRequest;
    require(decodeCharacterListRequest(encodeCharacterListRequest(listRequest), decodedListRequest) &&
        decodedListRequest.sessionToken == listRequest.sessionToken, "character list request should round-trip");
    CharacterListResponsePacket listResponse{AuthResult::Success, {{42, "Aster", "Vanguard", 8, 1200}}};
    CharacterListResponsePacket decodedListResponse;
    require(decodeCharacterListResponse(encodeCharacterListResponse(listResponse), decodedListResponse) &&
        decodedListResponse.characters.size() == 1 && decodedListResponse.characters.front().exp == 1200,
        "character list response should round-trip");

    CreateCharacterRequestPacket createRequest{"session-token", "Aster", "Vanguard"}, decodedCreateRequest;
    require(decodeCreateCharacterRequest(encodeCreateCharacterRequest(createRequest), decodedCreateRequest) &&
        decodedCreateRequest.className == createRequest.className, "create character request should round-trip");
    CreateCharacterResponsePacket createResponse{AuthResult::Success, {42, "Aster", "Vanguard", 1, 0}};
    CreateCharacterResponsePacket decodedCreateResponse;
    require(decodeCreateCharacterResponse(encodeCreateCharacterResponse(createResponse), decodedCreateResponse) &&
        decodedCreateResponse.character.characterId == 42, "create character response should round-trip");

    EnterWorldRequestPacket enterRequest{"session-token", 42}, decodedEnterRequest;
    require(decodeEnterWorldRequest(encodeEnterWorldRequest(enterRequest), decodedEnterRequest) &&
        decodedEnterRequest.characterId == enterRequest.characterId, "enter world request should round-trip");
    EnterWorldResponsePacket enterResponse{AuthResult::Success, {42, "Aster", "Vanguard", 1, 0}};
    EnterWorldResponsePacket decodedEnterResponse;
    require(decodeEnterWorldResponse(encodeEnterWorldResponse(enterResponse), decodedEnterResponse) &&
        decodedEnterResponse.character.name == "Aster", "enter world response should round-trip");
}

} // namespace

int main() {
    try {
        testDatabaseAndAccounts();
        testAuthPacketRoundTrips();
        std::cout << "Account database and auth packet tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

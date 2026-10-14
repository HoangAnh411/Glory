#include "glory/server/account_database.hpp"
#include "glory/network/network.hpp"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testFeedbackStorageAndExport() {
    using namespace glory::network;
    glory::server::AccountDatabase database(":memory:");
    require(database.registerAccount("beta_player", "closed-beta-password") == AuthResult::Success,
        "feedback account setup");
    const auto login = database.authenticateAccount("beta_player", "closed-beta-password");
    const auto character = database.createCharacter(login.sessionToken, "BetaHero", "Vanguard");
    require(character.result == AuthResult::Success, "feedback character setup");
    FeedbackSubmitRequestPacket request{FeedbackCategory::Bug, 4, "Crash \"report\"",
        "A line\\backslash\nand details", "{\"fps\":60,\"zone\":\"plains\"}"};
    require(database.submitFeedback(login.sessionToken, character.character.characterId, request) ==
        glory::server::FeedbackSubmissionResult::Success,
        "valid feedback should persist");
    require(database.submitFeedback(login.sessionToken, character.character.characterId, request) ==
        glory::server::FeedbackSubmissionResult::RateLimited,
        "repeat submissions from the account should be rate limited");
    require(database.submitFeedback("invalid-session", character.character.characterId, request) ==
        glory::server::FeedbackSubmissionResult::Invalid,
        "feedback requires a valid session");
    require(database.submitFeedback(login.sessionToken, character.character.characterId + 100, request) ==
        glory::server::FeedbackSubmissionResult::Invalid,
        "feedback must belong to the authenticated account");
    request.rating = 6;
    require(database.submitFeedback(login.sessionToken, character.character.characterId, request) ==
        glory::server::FeedbackSubmissionResult::Invalid,
        "rating outside 1..5 must be rejected");
    request.rating = 4;
    request.diagnosticsJson = "{malformed";
    require(database.submitFeedback(login.sessionToken, character.character.characterId, request) ==
        glory::server::FeedbackSubmissionResult::Invalid,
        "malformed diagnostics JSON must be rejected");

    const auto path = std::filesystem::temp_directory_path() / "glory_closed_beta_feedback" / "feedback.json";
    require(database.exportFeedbackToJson(path), "feedback JSON export should succeed");
    std::ifstream input(path, std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    const auto records = nlohmann::json::parse(json);
    require(json.find("\"category\":\"Bug\"") != std::string::npos &&
        json.find("Crash \\\"report\\\"") != std::string::npos &&
        json.find("diagnostics_json") != std::string::npos &&
        json.find("BetaHero") != std::string::npos && records.is_array() && records.size() == 1 &&
        records[0]["subject"] == "Crash \"report\"" &&
        records[0]["diagnostics_json"] == nlohmann::json{{"fps", 60}, {"zone", "plains"}},
        "export must preserve feedback fields and produce valid escaped JSON");
    input.close();
    const auto databasePath = path.parent_path() / "accounts.sqlite3";
    {
        glory::server::AccountDatabase persistent(databasePath.string());
        require(!persistent.exportFeedbackToJson(databasePath),
            "feedback export must not overwrite its source database");
    }
    std::filesystem::remove_all(path.parent_path());
}

void testFeedbackCodec() {
    using namespace glory::network;
    FeedbackSubmitRequestPacket source{FeedbackCategory::Suggestion, 5, "Add party finder",
        "Make group setup easier", "{\"platform\":\"test\"}"}, decoded;
    const auto bytes = encodeFeedbackSubmitRequest(source);
    require(decodeFeedbackSubmitRequest(bytes, decoded) && decoded.category == source.category &&
        decoded.rating == source.rating && decoded.subject == source.subject &&
        decoded.description == source.description && decoded.diagnosticsJson == source.diagnosticsJson,
        "feedback packet must round-trip");
    require(!decodeFeedbackSubmitRequest(std::span<const std::uint8_t>(bytes.data(), bytes.size() - 1), decoded),
        "truncated feedback packet must fail");
    for (std::uint8_t rating = 1; rating <= 5; ++rating) {
        source.rating = rating;
        require(decodeFeedbackSubmitRequest(encodeFeedbackSubmitRequest(source), decoded) &&
            decoded.rating == rating, "all ratings from 1 through 5 must encode and decode");
    }
    source.rating = 0;
    bool rejected = false;
    try { (void)encodeFeedbackSubmitRequest(source); }
    catch (const std::length_error&) { rejected = true; }
    require(rejected, "encoder must reject invalid rating");
    source.rating = 5;
    source.subject.assign(121, 's');
    rejected = false;
    try { (void)encodeFeedbackSubmitRequest(source); }
    catch (const std::length_error&) { rejected = true; }
    require(rejected, "encoder must reject an overlength subject");
    source.subject = "Subject";
    source.description.assign(601, 'd');
    rejected = false;
    try { (void)encodeFeedbackSubmitRequest(source); }
    catch (const std::length_error&) { rejected = true; }
    require(rejected, "encoder must reject an overlength description");
    FeedbackSubmitResponsePacket response{FeedbackSubmitStatus::RateLimited}, decodedResponse;
    const auto responseBytes = encodeFeedbackSubmitResponse(response);
    require(decodeFeedbackSubmitResponse(responseBytes, decodedResponse) &&
        decodedResponse.status == FeedbackSubmitStatus::RateLimited, "feedback response must round-trip");
}
}

int main() {
    try {
        testFeedbackStorageAndExport();
        testFeedbackCodec();
        std::cout << "Closed beta feedback tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Closed beta feedback test failure: " << error.what() << '\n';
        return 1;
    }
}

#pragma once

#include "glory/network/network.hpp"

#include <cstdint>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct sqlite3;

namespace glory::server {

struct PersistedInventorySlot {
    int slotIndex = 0;
    std::string itemId;
    int quantity = 0;
};

struct PersistedEquipmentItem {
    int slotType = 0;
    std::string itemId;
};

struct MarketListing {
    std::int64_t listingId = 0;
    std::int64_t sellerCharacterId = 0;
    std::string sellerName;
    std::string itemId;
    int quantity = 0;
    std::int64_t unitPrice = 0;
    std::string itemType;
    std::string itemName;
};

enum class FeedbackSubmissionResult : std::uint8_t { Success, Invalid, RateLimited };

enum class MarketResult : std::uint8_t {
    Success, InvalidSession, CharacterNotSelected, InvalidItem, InvalidQuantity,
    InvalidPrice, InsufficientFunds, InventoryFull, ListingNotFound, NotSeller, ServerError
};

class AccountDatabase {
public:
    explicit AccountDatabase(const std::string& path);
    ~AccountDatabase();
    AccountDatabase(const AccountDatabase&) = delete;
    AccountDatabase& operator=(const AccountDatabase&) = delete;

    [[nodiscard]] glory::network::AuthResult registerAccount(const std::string& username,
        const std::string& password);
    [[nodiscard]] glory::network::LoginResponsePacket authenticateAccount(const std::string& username,
        const std::string& password);
    [[nodiscard]] glory::network::CharacterListResponsePacket listCharacters(const std::string& sessionToken);
    [[nodiscard]] glory::network::CreateCharacterResponsePacket createCharacter(const std::string& sessionToken,
        const std::string& name, const std::string& className);
    [[nodiscard]] glory::network::EnterWorldResponsePacket selectCharacter(const std::string& sessionToken,
        std::int64_t characterId);
    [[nodiscard]] bool ownsCharacter(const std::string& sessionToken, std::int64_t characterId) const;
    [[nodiscard]] bool loadCharacterState(std::int64_t characterId,
        std::vector<PersistedInventorySlot>& inventory,
        std::vector<PersistedEquipmentItem>& equipment) const;
    [[nodiscard]] bool saveCharacterState(const std::string& sessionToken, std::int64_t characterId,
        const std::vector<PersistedInventorySlot>& inventory,
        const std::vector<PersistedEquipmentItem>& equipment);
    [[nodiscard]] bool grantCharacterExperience(const std::string& sessionToken, std::int64_t characterId,
        std::uint32_t amount);
    [[nodiscard]] glory::network::SocialResult createGuild(const std::string& sessionToken,
        std::int64_t characterId, const std::string& guildName);
    [[nodiscard]] glory::network::SocialResult inviteGuildMember(const std::string& sessionToken,
        std::int64_t characterId, std::int64_t targetCharacterId);
    [[nodiscard]] glory::network::SocialResult acceptGuildInvite(const std::string& sessionToken,
        std::int64_t characterId, std::int64_t guildId);
    [[nodiscard]] glory::network::SocialResult leaveGuild(const std::string& sessionToken,
        std::int64_t characterId);
    [[nodiscard]] glory::network::SocialResult setGuildRank(const std::string& sessionToken,
        std::int64_t characterId, std::int64_t targetCharacterId, glory::network::GuildRank rank);
    [[nodiscard]] glory::network::GuildSyncPacket guildForCharacter(std::int64_t characterId) const;
    [[nodiscard]] MarketResult listMarketItem(const std::string& token, std::int64_t characterId,
        const std::string& itemId, const std::string& itemName, const std::string& itemType,
        int quantity, int maxStack, std::int64_t unitPrice);
    [[nodiscard]] std::vector<MarketListing> browseMarket(const std::string& itemType,
        const std::string& search, int page, int pageSize) const;
    [[nodiscard]] std::vector<MarketListing> listingsForCharacter(std::int64_t characterId) const;
    [[nodiscard]] MarketResult buyMarketListing(const std::string& token, std::int64_t characterId,
        std::int64_t listingId);
    [[nodiscard]] MarketResult cancelMarketListing(const std::string& token, std::int64_t characterId,
        std::int64_t listingId);
    [[nodiscard]] std::int64_t characterGold(std::int64_t characterId) const;
    [[nodiscard]] std::int64_t marketListingSeller(std::int64_t listingId) const;
    [[nodiscard]] FeedbackSubmissionResult submitFeedback(const std::string& sessionToken, std::int64_t characterId,
        const glory::network::FeedbackSubmitRequestPacket& feedback);
    [[nodiscard]] bool exportFeedbackToJson(const std::filesystem::path& outputPath) const;

private:
    [[nodiscard]] std::optional<std::int64_t> accountForToken(const std::string& token) const;

    sqlite3* database_ = nullptr;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::int64_t> sessions_;
    std::unordered_map<std::int64_t, std::chrono::steady_clock::time_point> lastFeedbackSubmission_;
};

} // namespace glory::server

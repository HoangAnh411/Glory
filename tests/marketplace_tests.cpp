#include "glory/server/account_database.hpp"
#include "glory/network/network.hpp"
#include <sqlite3.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Character {
    std::string token;
    std::int64_t id = 0;
};

Character createCharacter(glory::server::AccountDatabase& db, const std::string& account,
    const std::string& name) {
    using namespace glory::network;
    require(db.registerAccount(account, "SecurePass123") == AuthResult::Success, "register account");
    auto login = db.authenticateAccount(account, "SecurePass123");
    require(login.result == AuthResult::Success, "authenticate account");
    auto created = db.createCharacter(login.sessionToken, name, "Vanguard");
    require(created.result == AuthResult::Success, "create character");
    return {login.sessionToken, created.character.characterId};
}

void testMarketOperations() {
    const auto path = std::filesystem::temp_directory_path() / "glory_marketplace_tests.sqlite";
    std::filesystem::remove(path);
    std::int64_t savedListingId = 0;
    {
        glory::server::AccountDatabase db(path.string());
        auto seller = createCharacter(db, "seller_account", "SellerOne");
        auto buyer = createCharacter(db, "buyer_account", "BuyerOne");
        require(db.characterGold(seller.id) == 1000 && db.characterGold(buyer.id) == 1000,
            "characters should start with migrated gold");
        require(db.saveCharacterState(seller.token, seller.id, {{0, "healing_tonic", 5}}, {}),
            "persist seller inventory");
        require(db.listMarketItem(seller.token, seller.id, "healing_tonic", "Healing Tonic",
            "Consumable", 3, 20, 100) == glory::server::MarketResult::Success, "list item");
        std::vector<glory::server::PersistedInventorySlot> inventory;
        std::vector<glory::server::PersistedEquipmentItem> equipment;
        require(db.loadCharacterState(seller.id, inventory, equipment) && inventory.size() == 1 &&
            inventory[0].quantity == 2, "listing should deduct exactly the listed quantity");
        const auto listings = db.browseMarket("Consumable", "Healing", 0, 20);
        require(listings.size() == 1 && listings[0].quantity == 3, "search and category filter");
        savedListingId = listings[0].listingId;
        require(db.buyMarketListing(buyer.token, buyer.id, savedListingId) ==
            glory::server::MarketResult::Success, "buy listing");
        require(db.characterGold(buyer.id) == 700 && db.characterGold(seller.id) == 1300,
            "purchase should transfer gold");
        require(db.loadCharacterState(buyer.id, inventory, equipment) && inventory.size() == 1 &&
            inventory[0].itemId == "healing_tonic" && inventory[0].quantity == 3,
            "purchase should deliver item");

        require(db.listMarketItem(seller.token, seller.id, "healing_tonic", "Healing Tonic",
            "Consumable", 1, 20, 1) == glory::server::MarketResult::Success, "list rollback test item");
        auto active = db.browseMarket("", "", 0, 20);
        require(active.size() == 1, "rollback listing should be active");
        const auto rollbackListingId = active[0].listingId;
        std::vector<glory::server::PersistedInventorySlot> fullInventory;
        for (int slot = 0; slot < 24; ++slot)
            fullInventory.push_back({slot, "filler_" + std::to_string(slot), 1});
        require(db.saveCharacterState(buyer.token, buyer.id, fullInventory, {}), "fill buyer inventory");
        require(db.buyMarketListing(buyer.token, buyer.id, rollbackListingId) ==
            glory::server::MarketResult::InventoryFull, "full inventory should reject purchase");
        require(db.characterGold(buyer.id) == 700 &&
            db.browseMarket("", "", 0, 20).size() == 1,
            "full inventory failure should roll back gold and listing status");
        require(db.saveCharacterState(buyer.token, buyer.id, {}, {}), "empty buyer inventory");
        require(db.cancelMarketListing(buyer.token, buyer.id, rollbackListingId) ==
            glory::server::MarketResult::NotSeller, "only seller may cancel listing");
        require(db.cancelMarketListing(seller.token, seller.id, rollbackListingId) ==
            glory::server::MarketResult::Success, "seller should cancel rollback listing");

        require(db.listMarketItem(seller.token, seller.id, "healing_tonic", "Healing Tonic",
            "Consumable", 1, 20, 5000) == glory::server::MarketResult::Success, "create expensive listing");
        const auto expensive = db.browseMarket("", "", 0, 20);
        require(expensive.size() == 1 && db.buyMarketListing(buyer.token, buyer.id, expensive[0].listingId) ==
            glory::server::MarketResult::InsufficientFunds, "insufficient gold should reject purchase");
        require(db.characterGold(buyer.id) == 700, "failed purchase must not debit buyer");
        require(db.cancelMarketListing(seller.token, seller.id, expensive[0].listingId) ==
            glory::server::MarketResult::Success, "seller cancels own listing");
        require(db.cancelMarketListing(buyer.token, buyer.id, savedListingId) ==
            glory::server::MarketResult::ListingNotFound, "sold listing cannot be cancelled");
        require(db.listMarketItem(seller.token, seller.id, "healing_tonic", "Healing Tonic",
            "Consumable", 1, 20, 25) == glory::server::MarketResult::Success, "persist active listing");
    }
    {
        glory::server::AccountDatabase db(path.string());
        require(db.characterGold(1) == 1300 && db.characterGold(2) == 700,
            "gold should persist after reopening database");
        const auto restoredListings = db.browseMarket("", "", 0, 20);
        require(restoredListings.size() == 1 && restoredListings[0].unitPrice == 25,
            "active listings should persist after reopening database");
    }
    std::filesystem::remove(path);
}

void testMarketplaceCodec() {
    using namespace glory::network;
    MarketBrowseRequestPacket browse{"session", 7, "Armor", "plate", 2, 15}, decodedBrowse;
    require(decodeMarketBrowseRequest(encodeMarketBrowseRequest(browse), decodedBrowse) &&
        decodedBrowse.characterId == 7 && decodedBrowse.itemType == "Armor" &&
        decodedBrowse.page == 2, "browse request codec");
    MarketBrowseResponsePacket response;
    response.listings.push_back({9, 3, "Seller", "iron_helm", "Iron Helm", "Armor", 2, 55});
    MarketBrowseResponsePacket decodedResponse;
    require(decodeMarketBrowseResponse(encodeMarketBrowseResponse(response), decodedResponse) &&
        decodedResponse.listings.size() == 1 && decodedResponse.listings[0].unitPrice == 55,
        "browse response codec");
    MarketListRequestPacket list{"session", 7, "iron_helm", 2, 55}, decodedList;
    require(decodeMarketListRequest(encodeMarketListRequest(list), decodedList) &&
        decodedList.quantity == 2, "list request codec");
    MarketActionRequestPacket action{"session", 7, 9}, decodedAction;
    require(decodeMarketActionRequest(encodeMarketActionRequest(action), decodedAction) &&
        decodedAction.listingId == 9, "buy/cancel request codec");
    MarketActionResponsePacket result{MarketResultPacket::Success, 945}, decodedResult;
    require(decodeMarketActionResponse(encodeMarketActionResponse(result), decodedResult) &&
        decodedResult.gold == 945, "action response codec");
    CharacterGoldSyncPacket gold{7, 945}, decodedGold;
    require(decodeCharacterGoldSync(encodeCharacterGoldSync(gold), decodedGold) &&
        decodedGold.gold == 945, "gold sync codec");
}

void testLegacyDatabaseMigration() {
    const auto path = std::filesystem::temp_directory_path() / "glory_marketplace_legacy.sqlite";
    std::filesystem::remove(path);
    sqlite3* raw = nullptr;
    require(sqlite3_open(path.string().c_str(), &raw) == SQLITE_OK, "create legacy database");
    const char* schema =
        "CREATE TABLE accounts(account_id INTEGER PRIMARY KEY AUTOINCREMENT,username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,salt TEXT NOT NULL,created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE characters(character_id INTEGER PRIMARY KEY AUTOINCREMENT,account_id INTEGER NOT NULL,"
        "name TEXT UNIQUE NOT NULL,class_name TEXT NOT NULL,level INTEGER DEFAULT 1,exp INTEGER DEFAULT 0,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
        "INSERT INTO accounts(username,password_hash,salt) VALUES('legacy_user','hash','salt');"
        "INSERT INTO characters(account_id,name,class_name) VALUES(1,'LegacyHero','Vanguard');";
    const int result = sqlite3_exec(raw, schema, nullptr, nullptr, nullptr);
    sqlite3_close(raw);
    require(result == SQLITE_OK, "prepare legacy schema");
    {
        glory::server::AccountDatabase migrated(path.string());
        require(migrated.characterGold(1) == 1000, "legacy character should receive default starting gold");
    }
    std::filesystem::remove(path);
}
}

int main() {
    try {
        testMarketOperations();
        testMarketplaceCodec();
        testLegacyDatabaseMigration();
        std::cout << "Marketplace tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

#include "glory/server/account_database.hpp"

#include <sqlite3.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace glory::server {
namespace {

constexpr std::uint32_t passwordIterations = 210'000;
constexpr std::size_t saltSize = 16;
constexpr std::size_t hashSize = 32;

class Statement {
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(database));
    }
    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    sqlite3_stmt* get() const { return statement_; }
private:
    sqlite3_stmt* statement_ = nullptr;
};

void execute(sqlite3* database, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(database);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

bool hasColumn(sqlite3* database, const char* table, const char* column) {
    const std::string sql = std::string("PRAGMA table_info(") + table + ");";
    Statement statement(database, sql.c_str());
    while (sqlite3_step(statement.get()) == SQLITE_ROW) {
        const auto* name = sqlite3_column_text(statement.get(), 1);
        if (name && std::string_view(reinterpret_cast<const char*>(name)) == column) return true;
    }
    return false;
}

bool validName(std::string_view value) {
    if (value.size() < 3 || value.size() > 24) return false;
    for (const unsigned char character : value)
        if (!std::isalnum(character) && character != '_') return false;
    return true;
}

bool validPassword(std::string_view value) { return value.size() >= 8 && value.size() <= 128; }

bool secureRandom(std::span<std::uint8_t> output) {
#ifdef _WIN32
    return BCryptGenRandom(nullptr, output.data(), static_cast<ULONG>(output.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
#else
    return RAND_bytes(output.data(), static_cast<int>(output.size())) == 1;
#endif
}

bool derivePassword(std::string_view password, std::span<const std::uint8_t> salt,
    std::span<std::uint8_t> output) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
        BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) return false;
    const NTSTATUS result = BCryptDeriveKeyPBKDF2(algorithm,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt.data()), static_cast<ULONG>(salt.size()), passwordIterations,
        output.data(), static_cast<ULONG>(output.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return result >= 0;
#else
    return PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt.data(),
        static_cast<int>(salt.size()), static_cast<int>(passwordIterations), EVP_sha256(),
        static_cast<int>(output.size()), output.data()) == 1;
#endif
}

std::string toHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result(bytes.size() * 2, '0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        result[index * 2] = digits[bytes[index] >> 4];
        result[index * 2 + 1] = digits[bytes[index] & 0x0f];
    }
    return result;
}

bool fromHex(std::string_view text, std::span<std::uint8_t> output) {
    if (text.size() != output.size() * 2) return false;
    const auto nibble = [](char value) -> int {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    };
    for (std::size_t index = 0; index < output.size(); ++index) {
        const int high = nibble(text[index * 2]);
        const int low = nibble(text[index * 2 + 1]);
        if (high < 0 || low < 0) return false;
        output[index] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return true;
}

bool constantTimeEqual(std::span<const std::uint8_t> left, std::span<const std::uint8_t> right) {
    if (left.size() != right.size()) return false;
    std::uint8_t difference = 0;
    for (std::size_t index = 0; index < left.size(); ++index) difference |= left[index] ^ right[index];
    return difference == 0;
}

std::string columnText(sqlite3_stmt* statement, int column) {
    const auto* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}

std::string jsonEscape(std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string escaped;
    for (const unsigned char character : value) {
        switch (character) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (character < 0x20) {
                escaped += "\\u00";
                escaped += hex[character >> 4];
                escaped += hex[character & 0xf];
            } else escaped += static_cast<char>(character);
        }
    }
    return escaped;
}

const char* feedbackCategoryName(glory::network::FeedbackCategory category) {
    switch (category) {
    case glory::network::FeedbackCategory::Bug: return "Bug";
    case glory::network::FeedbackCategory::Balance: return "Balance";
    case glory::network::FeedbackCategory::Suggestion: return "Suggestion";
    case glory::network::FeedbackCategory::Praise: return "Praise";
    }
    return "Bug";
}

const char* guildRankName(glory::network::GuildRank rank) {
    switch (rank) {
    case glory::network::GuildRank::Leader: return "Leader";
    case glory::network::GuildRank::Officer: return "Officer";
    case glory::network::GuildRank::Member: return "Member";
    }
    return "Member";
}

glory::network::GuildRank parseGuildRank(const std::string& rank) {
    if (rank == "Leader") return glory::network::GuildRank::Leader;
    if (rank == "Officer") return glory::network::GuildRank::Officer;
    return glory::network::GuildRank::Member;
}

} // namespace

AccountDatabase::AccountDatabase(const std::string& path) {
    if (path != ":memory:") {
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
    }
    if (sqlite3_open_v2(path.c_str(), &database_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr) != SQLITE_OK) {
        const std::string message = database_ ? sqlite3_errmsg(database_) : "Unable to open SQLite database";
        if (database_) sqlite3_close(database_);
        database_ = nullptr;
        throw std::runtime_error(message);
    }
    sqlite3_busy_timeout(database_, 5000);
    execute(database_, "PRAGMA foreign_keys = ON;");
    execute(database_, "CREATE TABLE IF NOT EXISTS accounts ("
        "account_id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT NOT NULL UNIQUE, "
        "password_hash TEXT NOT NULL, salt TEXT NOT NULL, created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP);");
    execute(database_, "CREATE TABLE IF NOT EXISTS characters ("
        "character_id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, name TEXT NOT NULL UNIQUE, "
        "class_name TEXT NOT NULL, level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0, "
        "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(account_id) REFERENCES accounts(account_id) ON DELETE CASCADE);");
    if (!hasColumn(database_, "characters", "gold"))
        execute(database_, "ALTER TABLE characters ADD COLUMN gold INTEGER NOT NULL DEFAULT 1000 CHECK(gold >= 0);");
    execute(database_, "CREATE TABLE IF NOT EXISTS inventories ("
        "inventory_id INTEGER PRIMARY KEY AUTOINCREMENT, character_id INTEGER NOT NULL, slot_index INTEGER NOT NULL, "
        "item_id TEXT NOT NULL, quantity INTEGER NOT NULL CHECK(quantity > 0), "
        "UNIQUE(character_id,slot_index), FOREIGN KEY(character_id) REFERENCES characters(character_id) ON DELETE CASCADE);");
    execute(database_, "CREATE TABLE IF NOT EXISTS equipments ("
        "equipment_id INTEGER PRIMARY KEY AUTOINCREMENT, character_id INTEGER NOT NULL, slot_type INTEGER NOT NULL, "
        "item_id TEXT NOT NULL, UNIQUE(character_id,slot_type), "
        "FOREIGN KEY(character_id) REFERENCES characters(character_id) ON DELETE CASCADE);");
    execute(database_, "CREATE TABLE IF NOT EXISTS guilds ("
        "guild_id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE, leader_character_id INTEGER NOT NULL, "
        "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(leader_character_id) REFERENCES characters(character_id) ON DELETE CASCADE);");
    execute(database_, "CREATE TABLE IF NOT EXISTS guild_members ("
        "guild_id INTEGER NOT NULL, character_id INTEGER NOT NULL UNIQUE, "
        "rank TEXT NOT NULL CHECK(rank IN ('Leader','Officer','Member')), "
        "joined_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY(guild_id,character_id), "
        "FOREIGN KEY(guild_id) REFERENCES guilds(guild_id) ON DELETE CASCADE, "
        "FOREIGN KEY(character_id) REFERENCES characters(character_id) ON DELETE CASCADE);");
    execute(database_, "CREATE TABLE IF NOT EXISTS market_listings ("
        "listing_id INTEGER PRIMARY KEY AUTOINCREMENT, seller_character_id INTEGER NOT NULL, seller_name TEXT NOT NULL, "
        "item_id TEXT NOT NULL, quantity INTEGER NOT NULL CHECK(quantity > 0), unit_price INTEGER NOT NULL CHECK(unit_price > 0), "
        "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, status TEXT NOT NULL CHECK(status IN ('Active','Sold','Cancelled')), "
        "item_name TEXT NOT NULL DEFAULT '', item_type TEXT NOT NULL DEFAULT '', max_stack INTEGER NOT NULL DEFAULT 999 CHECK(max_stack > 0), "
        "FOREIGN KEY(seller_character_id) REFERENCES characters(character_id));");
    if (!hasColumn(database_, "market_listings", "max_stack"))
        execute(database_, "ALTER TABLE market_listings ADD COLUMN max_stack INTEGER NOT NULL DEFAULT 999 CHECK(max_stack > 0);");
    execute(database_, "CREATE TABLE IF NOT EXISTS feedback ("
        "feedback_id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, character_name TEXT NOT NULL, "
        "category TEXT NOT NULL CHECK(category IN ('Bug','Balance','Suggestion','Praise')), "
        "rating INTEGER NOT NULL CHECK(rating BETWEEN 1 AND 5), subject TEXT NOT NULL, description TEXT NOT NULL, "
        "diagnostics_json TEXT NOT NULL, created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(account_id) REFERENCES accounts(account_id) ON DELETE CASCADE);");
}

AccountDatabase::~AccountDatabase() { if (database_) sqlite3_close(database_); }

glory::network::AuthResult AccountDatabase::registerAccount(const std::string& username,
    const std::string& password) {
    using glory::network::AuthResult;
    if (!validName(username) || !validPassword(password)) return AuthResult::InvalidLength;
    std::array<std::uint8_t, saltSize> salt{};
    std::array<std::uint8_t, hashSize> hash{};
    if (!secureRandom(salt) || !derivePassword(password, salt, hash)) return AuthResult::ServerError;
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "INSERT INTO accounts(username,password_hash,salt) VALUES(?,?,?);");
        sqlite3_bind_text(statement.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        const std::string hashText = toHex(hash), saltText = toHex(salt);
        sqlite3_bind_text(statement.get(), 2, hashText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 3, saltText.c_str(), -1, SQLITE_TRANSIENT);
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) return AuthResult::Success;
        if (sqlite3_extended_errcode(database_) == SQLITE_CONSTRAINT_UNIQUE) return AuthResult::UsernameTaken;
    } catch (...) { return AuthResult::ServerError; }
    return AuthResult::ServerError;
}

glory::network::LoginResponsePacket AccountDatabase::authenticateAccount(const std::string& username,
    const std::string& password) {
    using glory::network::AuthResult;
    glory::network::LoginResponsePacket response;
    if (!validName(username) || !validPassword(password)) { response.result = AuthResult::InvalidLength; return response; }
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "SELECT account_id,password_hash,salt FROM accounts WHERE username=?;");
        sqlite3_bind_text(statement.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) { response.result = AuthResult::AccountNotFound; return response; }
        if (step != SQLITE_ROW) return response;
        const std::int64_t accountId = sqlite3_column_int64(statement.get(), 0);
        std::array<std::uint8_t, saltSize> salt{};
        std::array<std::uint8_t, hashSize> expected{}, actual{};
        if (!fromHex(columnText(statement.get(), 1), expected) ||
            !fromHex(columnText(statement.get(), 2), salt) || !derivePassword(password, salt, actual)) return response;
        if (!constantTimeEqual(expected, actual)) { response.result = AuthResult::BadCredentials; return response; }
        std::array<std::uint8_t, 32> tokenBytes{};
        if (!secureRandom(tokenBytes)) return response;
        response.sessionToken = toHex(tokenBytes);
        sessions_[response.sessionToken] = accountId;
        response.result = AuthResult::Success;
        return response;
    } catch (...) { return response; }
}

std::optional<std::int64_t> AccountDatabase::accountForToken(const std::string& token) const {
    const auto found = sessions_.find(token);
    if (found == sessions_.end()) return std::nullopt;
    return found->second;
}

glory::network::CharacterListResponsePacket AccountDatabase::listCharacters(const std::string& sessionToken) {
    using glory::network::AuthResult;
    glory::network::CharacterListResponsePacket response;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) { response.result = AuthResult::InvalidSession; return response; }
        Statement statement(database_, "SELECT character_id,name,class_name,level,exp FROM characters WHERE account_id=? ORDER BY character_id;");
        sqlite3_bind_int64(statement.get(), 1, *accountId);
        int step;
        while ((step = sqlite3_step(statement.get())) == SQLITE_ROW) {
            response.characters.push_back({sqlite3_column_int64(statement.get(), 0), columnText(statement.get(), 1),
                columnText(statement.get(), 2), sqlite3_column_int(statement.get(), 3), sqlite3_column_int(statement.get(), 4)});
        }
        if (step != SQLITE_DONE) return response;
        response.result = AuthResult::Success;
    } catch (...) { response.characters.clear(); return response; }
    return response;
}

glory::network::CreateCharacterResponsePacket AccountDatabase::createCharacter(const std::string& sessionToken,
    const std::string& name, const std::string& className) {
    using glory::network::AuthResult;
    glory::network::CreateCharacterResponsePacket response;
    if (!validName(name) || !validName(className)) { response.result = AuthResult::InvalidLength; return response; }
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) { response.result = AuthResult::InvalidSession; return response; }
        Statement count(database_, "SELECT COUNT(*) FROM characters WHERE account_id=?;");
        sqlite3_bind_int64(count.get(), 1, *accountId);
        if (sqlite3_step(count.get()) != SQLITE_ROW) return response;
        if (sqlite3_column_int(count.get(), 0) >= 16) {
            response.result = AuthResult::CharacterLimitReached;
            return response;
        }
        Statement statement(database_, "INSERT INTO characters(account_id,name,class_name) VALUES(?,?,?);");
        sqlite3_bind_int64(statement.get(), 1, *accountId);
        sqlite3_bind_text(statement.get(), 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 3, className.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            if (sqlite3_extended_errcode(database_) == SQLITE_CONSTRAINT_UNIQUE)
                response.result = AuthResult::CharacterNameTaken;
            return response;
        }
        response.result = AuthResult::Success;
        response.character = {sqlite3_last_insert_rowid(database_), name, className, 1, 0};
    } catch (...) { return response; }
    return response;
}

glory::network::EnterWorldResponsePacket AccountDatabase::selectCharacter(const std::string& sessionToken,
    std::int64_t characterId) {
    using glory::network::AuthResult;
    glory::network::EnterWorldResponsePacket response;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) { response.result = AuthResult::InvalidSession; return response; }
        Statement statement(database_, "SELECT character_id,name,class_name,level,exp FROM characters "
            "WHERE account_id=? AND character_id=?;");
        sqlite3_bind_int64(statement.get(), 1, *accountId);
        sqlite3_bind_int64(statement.get(), 2, characterId);
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) { response.result = AuthResult::CharacterNotFound; return response; }
        if (step != SQLITE_ROW) return response;
        response.result = AuthResult::Success;
        response.character = {sqlite3_column_int64(statement.get(), 0), columnText(statement.get(), 1),
            columnText(statement.get(), 2), sqlite3_column_int(statement.get(), 3),
            sqlite3_column_int(statement.get(), 4)};
    } catch (...) { return response; }
    return response;
}

bool AccountDatabase::ownsCharacter(const std::string& sessionToken, std::int64_t characterId) const {
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return false;
        Statement statement(database_, "SELECT 1 FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(statement.get(), 1, characterId);
        sqlite3_bind_int64(statement.get(), 2, *accountId);
        return sqlite3_step(statement.get()) == SQLITE_ROW;
    } catch (...) { return false; }
}

bool AccountDatabase::loadCharacterState(std::int64_t characterId,
    std::vector<PersistedInventorySlot>& inventory,
    std::vector<PersistedEquipmentItem>& equipment) const {
    try {
        std::lock_guard lock(mutex_);
        std::vector<PersistedInventorySlot> loadedInventory;
        std::vector<PersistedEquipmentItem> loadedEquipment;
        Statement inventoryQuery(database_,
            "SELECT slot_index,item_id,quantity FROM inventories WHERE character_id=? ORDER BY slot_index;");
        sqlite3_bind_int64(inventoryQuery.get(), 1, characterId);
        int step;
        while ((step = sqlite3_step(inventoryQuery.get())) == SQLITE_ROW) {
            loadedInventory.push_back({sqlite3_column_int(inventoryQuery.get(), 0),
                columnText(inventoryQuery.get(), 1), sqlite3_column_int(inventoryQuery.get(), 2)});
        }
        if (step != SQLITE_DONE) return false;
        Statement equipmentQuery(database_,
            "SELECT slot_type,item_id FROM equipments WHERE character_id=? ORDER BY slot_type;");
        sqlite3_bind_int64(equipmentQuery.get(), 1, characterId);
        while ((step = sqlite3_step(equipmentQuery.get())) == SQLITE_ROW) {
            loadedEquipment.push_back({sqlite3_column_int(equipmentQuery.get(), 0),
                columnText(equipmentQuery.get(), 1)});
        }
        if (step != SQLITE_DONE) return false;
        inventory = std::move(loadedInventory);
        equipment = std::move(loadedEquipment);
        return true;
    } catch (...) { return false; }
}

bool AccountDatabase::saveCharacterState(const std::string& sessionToken, std::int64_t characterId,
    const std::vector<PersistedInventorySlot>& inventory,
    const std::vector<PersistedEquipmentItem>& equipment) {
    if (inventory.size() > 24 || equipment.size() > 3) return false;
    std::array<bool, 24> inventorySlots{};
    std::array<bool, 3> equipmentSlots{};
    for (const auto& slot : inventory) {
        if (slot.slotIndex < 0 || slot.slotIndex >= static_cast<int>(inventorySlots.size()) ||
            inventorySlots[static_cast<std::size_t>(slot.slotIndex)] || slot.itemId.empty() ||
            slot.itemId.size() > 48 || slot.quantity <= 0) return false;
        inventorySlots[static_cast<std::size_t>(slot.slotIndex)] = true;
    }
    for (const auto& item : equipment) {
        if (item.slotType < 0 || item.slotType >= static_cast<int>(equipmentSlots.size()) ||
            equipmentSlots[static_cast<std::size_t>(item.slotType)] || item.itemId.empty() ||
            item.itemId.size() > 48) return false;
        equipmentSlots[static_cast<std::size_t>(item.slotType)] = true;
    }
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return false;
        Statement owner(database_, "SELECT 1 FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(owner.get(), 1, characterId);
        sqlite3_bind_int64(owner.get(), 2, *accountId);
        if (sqlite3_step(owner.get()) != SQLITE_ROW) return false;
        execute(database_, "BEGIN IMMEDIATE;");
        try {
            Statement deleteInventory(database_, "DELETE FROM inventories WHERE character_id=?;");
            sqlite3_bind_int64(deleteInventory.get(), 1, characterId);
            if (sqlite3_step(deleteInventory.get()) != SQLITE_DONE) throw std::runtime_error("Inventory delete failed");
            Statement deleteEquipment(database_, "DELETE FROM equipments WHERE character_id=?;");
            sqlite3_bind_int64(deleteEquipment.get(), 1, characterId);
            if (sqlite3_step(deleteEquipment.get()) != SQLITE_DONE) throw std::runtime_error("Equipment delete failed");
            Statement insertInventory(database_,
                "INSERT INTO inventories(character_id,slot_index,item_id,quantity) VALUES(?,?,?,?);");
            for (const auto& slot : inventory) {
                sqlite3_bind_int64(insertInventory.get(), 1, characterId);
                sqlite3_bind_int(insertInventory.get(), 2, slot.slotIndex);
                sqlite3_bind_text(insertInventory.get(), 3, slot.itemId.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(insertInventory.get(), 4, slot.quantity);
                if (sqlite3_step(insertInventory.get()) != SQLITE_DONE) throw std::runtime_error("Inventory insert failed");
                sqlite3_reset(insertInventory.get()); sqlite3_clear_bindings(insertInventory.get());
            }
            Statement insertEquipment(database_,
                "INSERT INTO equipments(character_id,slot_type,item_id) VALUES(?,?,?);");
            for (const auto& item : equipment) {
                sqlite3_bind_int64(insertEquipment.get(), 1, characterId);
                sqlite3_bind_int(insertEquipment.get(), 2, item.slotType);
                sqlite3_bind_text(insertEquipment.get(), 3, item.itemId.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(insertEquipment.get()) != SQLITE_DONE) throw std::runtime_error("Equipment insert failed");
                sqlite3_reset(insertEquipment.get()); sqlite3_clear_bindings(insertEquipment.get());
            }
            execute(database_, "COMMIT;");
        } catch (...) {
            try { execute(database_, "ROLLBACK;"); } catch (...) {}
            return false;
        }
        return true;
    } catch (...) { return false; }
}

bool AccountDatabase::grantCharacterExperience(const std::string& sessionToken, std::int64_t characterId,
    std::uint32_t amount) {
    if (characterId <= 0 || amount == 0 || amount > 1'000'000) return false;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return false;
        Statement update(database_, "UPDATE characters SET exp=exp+? WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(update.get(), 1, static_cast<sqlite3_int64>(amount));
        sqlite3_bind_int64(update.get(), 2, characterId);
        sqlite3_bind_int64(update.get(), 3, *accountId);
        return sqlite3_step(update.get()) == SQLITE_DONE && sqlite3_changes(database_) == 1;
    } catch (...) { return false; }
}

glory::network::SocialResult AccountDatabase::createGuild(const std::string& sessionToken,
    std::int64_t characterId, const std::string& guildName) {
    using glory::network::SocialResult;
    if (characterId <= 0 || !validName(guildName)) return SocialResult::InvalidTarget;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return SocialResult::InvalidSession;
        Statement owner(database_, "SELECT 1 FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(owner.get(), 1, characterId); sqlite3_bind_int64(owner.get(), 2, *accountId);
        if (sqlite3_step(owner.get()) != SQLITE_ROW) return SocialResult::InvalidSession;
        Statement existingMember(database_, "SELECT 1 FROM guild_members WHERE character_id=?;");
        sqlite3_bind_int64(existingMember.get(), 1, characterId);
        if (sqlite3_step(existingMember.get()) == SQLITE_ROW) return SocialResult::AlreadyInGuild;
        Statement existingName(database_, "SELECT 1 FROM guilds WHERE name=? COLLATE NOCASE;");
        sqlite3_bind_text(existingName.get(), 1, guildName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(existingName.get()) == SQLITE_ROW) return SocialResult::GuildExists;
        execute(database_, "BEGIN IMMEDIATE;");
        try {
            Statement insertGuild(database_, "INSERT INTO guilds(name,leader_character_id) VALUES(?,?);");
            sqlite3_bind_text(insertGuild.get(), 1, guildName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(insertGuild.get(), 2, characterId);
            if (sqlite3_step(insertGuild.get()) != SQLITE_DONE) throw std::runtime_error("Guild insert failed");
            const auto guildId = sqlite3_last_insert_rowid(database_);
            Statement insertLeader(database_,
                "INSERT INTO guild_members(guild_id,character_id,rank) VALUES(?,?,'Leader');");
            sqlite3_bind_int64(insertLeader.get(), 1, guildId); sqlite3_bind_int64(insertLeader.get(), 2, characterId);
            if (sqlite3_step(insertLeader.get()) != SQLITE_DONE) throw std::runtime_error("Guild leader insert failed");
            execute(database_, "COMMIT;");
            return SocialResult::Success;
        } catch (...) {
            try { execute(database_, "ROLLBACK;"); } catch (...) {}
            return SocialResult::ServerError;
        }
    } catch (...) { return SocialResult::ServerError; }
}

glory::network::SocialResult AccountDatabase::inviteGuildMember(const std::string& sessionToken,
    std::int64_t characterId, std::int64_t targetCharacterId) {
    using glory::network::SocialResult;
    if (characterId <= 0 || targetCharacterId <= 0 || targetCharacterId == characterId)
        return SocialResult::InvalidTarget;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return SocialResult::InvalidSession;
        Statement inviter(database_, "SELECT gm.guild_id,gm.rank FROM guild_members gm "
            "JOIN characters c ON c.character_id=gm.character_id WHERE gm.character_id=? AND c.account_id=?;");
        sqlite3_bind_int64(inviter.get(), 1, characterId); sqlite3_bind_int64(inviter.get(), 2, *accountId);
        if (sqlite3_step(inviter.get()) != SQLITE_ROW) return SocialResult::NotInGuild;
        const auto guildId = sqlite3_column_int64(inviter.get(), 0);
        const auto rank = columnText(inviter.get(), 1);
        if (rank != "Leader" && rank != "Officer") return SocialResult::NotLeader;
        Statement target(database_, "SELECT 1 FROM characters WHERE character_id=?;");
        sqlite3_bind_int64(target.get(), 1, targetCharacterId);
        if (sqlite3_step(target.get()) != SQLITE_ROW) return SocialResult::InvalidTarget;
        Statement member(database_, "SELECT 1 FROM guild_members WHERE character_id=?;");
        sqlite3_bind_int64(member.get(), 1, targetCharacterId);
        if (sqlite3_step(member.get()) == SQLITE_ROW) return SocialResult::AlreadyInGuild;
        Statement count(database_, "SELECT COUNT(*) FROM guild_members WHERE guild_id=?;");
        sqlite3_bind_int64(count.get(), 1, guildId);
        if (sqlite3_step(count.get()) != SQLITE_ROW) return SocialResult::ServerError;
        if (sqlite3_column_int(count.get(), 0) >= 8) return SocialResult::PartyFull;
        return SocialResult::Success;
    } catch (...) { return SocialResult::ServerError; }
}

glory::network::SocialResult AccountDatabase::acceptGuildInvite(const std::string& sessionToken,
    std::int64_t characterId, std::int64_t guildId) {
    using glory::network::SocialResult;
    if (characterId <= 0 || guildId <= 0) return SocialResult::InvalidTarget;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return SocialResult::InvalidSession;
        Statement owner(database_, "SELECT 1 FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(owner.get(), 1, characterId); sqlite3_bind_int64(owner.get(), 2, *accountId);
        if (sqlite3_step(owner.get()) != SQLITE_ROW) return SocialResult::InvalidSession;
        Statement member(database_, "SELECT 1 FROM guild_members WHERE character_id=?;");
        sqlite3_bind_int64(member.get(), 1, characterId);
        if (sqlite3_step(member.get()) == SQLITE_ROW) return SocialResult::AlreadyInGuild;
        Statement guild(database_, "SELECT COUNT(*) FROM guild_members WHERE guild_id=?;");
        sqlite3_bind_int64(guild.get(), 1, guildId);
        if (sqlite3_step(guild.get()) != SQLITE_ROW) return SocialResult::NotFound;
        const int count = sqlite3_column_int(guild.get(), 0);
        Statement exists(database_, "SELECT 1 FROM guilds WHERE guild_id=?;");
        sqlite3_bind_int64(exists.get(), 1, guildId);
        if (sqlite3_step(exists.get()) != SQLITE_ROW) return SocialResult::NotFound;
        if (count >= 8) return SocialResult::PartyFull;
        execute(database_, "BEGIN IMMEDIATE;");
        try {
            Statement insert(database_, "INSERT INTO guild_members(guild_id,character_id,rank) VALUES(?,?,'Member');");
            sqlite3_bind_int64(insert.get(), 1, guildId); sqlite3_bind_int64(insert.get(), 2, characterId);
            if (sqlite3_step(insert.get()) != SQLITE_DONE) throw std::runtime_error("Guild member insert failed");
            execute(database_, "COMMIT;");
            return SocialResult::Success;
        } catch (...) {
            try { execute(database_, "ROLLBACK;"); } catch (...) {}
            return SocialResult::ServerError;
        }
    } catch (...) { return SocialResult::ServerError; }
}

glory::network::SocialResult AccountDatabase::leaveGuild(const std::string& sessionToken,
    std::int64_t characterId) {
    using glory::network::SocialResult;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return SocialResult::InvalidSession;
        Statement member(database_, "SELECT gm.guild_id,gm.rank FROM guild_members gm "
            "JOIN characters c ON c.character_id=gm.character_id WHERE gm.character_id=? AND c.account_id=?;");
        sqlite3_bind_int64(member.get(), 1, characterId); sqlite3_bind_int64(member.get(), 2, *accountId);
        if (sqlite3_step(member.get()) != SQLITE_ROW) return SocialResult::NotInGuild;
        const auto guildId = sqlite3_column_int64(member.get(), 0);
        const bool leader = columnText(member.get(), 1) == "Leader";
        execute(database_, "BEGIN IMMEDIATE;");
        try {
            std::int64_t successorId = 0;
            if (leader) {
                Statement successor(database_, "SELECT character_id FROM guild_members WHERE guild_id=? "
                    "AND character_id<>? ORDER BY joined_at,character_id LIMIT 1;");
                sqlite3_bind_int64(successor.get(), 1, guildId); sqlite3_bind_int64(successor.get(), 2, characterId);
                if (sqlite3_step(successor.get()) == SQLITE_ROW)
                    successorId = sqlite3_column_int64(successor.get(), 0);
            }
            if (leader && successorId == 0) {
                Statement removeGuild(database_, "DELETE FROM guilds WHERE guild_id=?;");
                sqlite3_bind_int64(removeGuild.get(), 1, guildId);
                if (sqlite3_step(removeGuild.get()) != SQLITE_DONE) throw std::runtime_error("Guild delete failed");
            } else {
                if (leader) {
                    Statement updateGuild(database_, "UPDATE guilds SET leader_character_id=? WHERE guild_id=?;");
                    sqlite3_bind_int64(updateGuild.get(), 1, successorId); sqlite3_bind_int64(updateGuild.get(), 2, guildId);
                    if (sqlite3_step(updateGuild.get()) != SQLITE_DONE) throw std::runtime_error("Leader transfer failed");
                    Statement promote(database_, "UPDATE guild_members SET rank='Leader' WHERE guild_id=? AND character_id=?;");
                    sqlite3_bind_int64(promote.get(), 1, guildId); sqlite3_bind_int64(promote.get(), 2, successorId);
                    if (sqlite3_step(promote.get()) != SQLITE_DONE) throw std::runtime_error("Leader promotion failed");
                }
                Statement removeMember(database_, "DELETE FROM guild_members WHERE guild_id=? AND character_id=?;");
                sqlite3_bind_int64(removeMember.get(), 1, guildId); sqlite3_bind_int64(removeMember.get(), 2, characterId);
                if (sqlite3_step(removeMember.get()) != SQLITE_DONE) throw std::runtime_error("Guild leave failed");
            }
            execute(database_, "COMMIT;");
            return SocialResult::Success;
        } catch (...) {
            try { execute(database_, "ROLLBACK;"); } catch (...) {}
            return SocialResult::ServerError;
        }
    } catch (...) { return SocialResult::ServerError; }
}

glory::network::SocialResult AccountDatabase::setGuildRank(const std::string& sessionToken,
    std::int64_t characterId, std::int64_t targetCharacterId, glory::network::GuildRank rank) {
    using glory::network::GuildRank;
    using glory::network::SocialResult;
    if (rank == GuildRank::Leader || rank > GuildRank::Member || characterId == targetCharacterId)
        return SocialResult::InvalidRank;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return SocialResult::InvalidSession;
        Statement caller(database_, "SELECT gm.guild_id,gm.rank FROM guild_members gm "
            "JOIN characters c ON c.character_id=gm.character_id WHERE gm.character_id=? AND c.account_id=?;");
        sqlite3_bind_int64(caller.get(), 1, characterId); sqlite3_bind_int64(caller.get(), 2, *accountId);
        if (sqlite3_step(caller.get()) != SQLITE_ROW) return SocialResult::NotInGuild;
        const auto guildId = sqlite3_column_int64(caller.get(), 0);
        if (columnText(caller.get(), 1) != "Leader") return SocialResult::NotLeader;
        Statement target(database_, "SELECT 1 FROM guild_members WHERE guild_id=? AND character_id=?;");
        sqlite3_bind_int64(target.get(), 1, guildId); sqlite3_bind_int64(target.get(), 2, targetCharacterId);
        if (sqlite3_step(target.get()) != SQLITE_ROW) return SocialResult::InvalidTarget;
        Statement update(database_, "UPDATE guild_members SET rank=? WHERE guild_id=? AND character_id=?;");
        sqlite3_bind_text(update.get(), 1, guildRankName(rank), -1, SQLITE_STATIC);
        sqlite3_bind_int64(update.get(), 2, guildId); sqlite3_bind_int64(update.get(), 3, targetCharacterId);
        return sqlite3_step(update.get()) == SQLITE_DONE ? SocialResult::Success : SocialResult::ServerError;
    } catch (...) { return SocialResult::ServerError; }
}

glory::network::GuildSyncPacket AccountDatabase::guildForCharacter(std::int64_t characterId) const {
    using namespace glory::network;
    GuildSyncPacket packet;
    try {
        std::lock_guard lock(mutex_);
        Statement member(database_, "SELECT g.guild_id,g.name,gm.rank FROM guild_members gm "
            "JOIN guilds g ON g.guild_id=gm.guild_id WHERE gm.character_id=?;");
        sqlite3_bind_int64(member.get(), 1, characterId);
        if (sqlite3_step(member.get()) != SQLITE_ROW) { packet.result = SocialResult::NotInGuild; return packet; }
        packet.guildId = static_cast<std::uint64_t>(sqlite3_column_int64(member.get(), 0));
        packet.guildName = columnText(member.get(), 1);
        packet.memberRank = parseGuildRank(columnText(member.get(), 2));
        Statement members(database_, "SELECT c.character_id,c.name,c.class_name,c.level,gm.rank "
            "FROM guild_members gm JOIN characters c ON c.character_id=gm.character_id "
            "WHERE gm.guild_id=? ORDER BY CASE gm.rank WHEN 'Leader' THEN 0 WHEN 'Officer' THEN 1 ELSE 2 END,gm.joined_at;");
        sqlite3_bind_int64(members.get(), 1, static_cast<sqlite3_int64>(packet.guildId));
        int step = 0;
        while ((step = sqlite3_step(members.get())) == SQLITE_ROW) {
            packet.members.push_back({sqlite3_column_int64(members.get(), 0), columnText(members.get(), 1),
                columnText(members.get(), 2), sqlite3_column_int(members.get(), 3),
                parseGuildRank(columnText(members.get(), 4))});
        }
        if (step != SQLITE_DONE) packet.result = SocialResult::ServerError;
        return packet;
    } catch (...) { packet.result = SocialResult::ServerError; return packet; }
}

MarketResult AccountDatabase::listMarketItem(const std::string& token, std::int64_t characterId,
    const std::string& itemId, const std::string& itemName, const std::string& itemType,
    int quantity, int maxStack, std::int64_t unitPrice) {
    if (itemId.empty()) return MarketResult::InvalidItem;
    if (quantity <= 0 || maxStack <= 0) return MarketResult::InvalidQuantity;
    if (unitPrice <= 0 || quantity > std::numeric_limits<std::int64_t>::max() / unitPrice)
        return MarketResult::InvalidPrice;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(token);
        if (!accountId) return MarketResult::InvalidSession;
        execute(database_, "BEGIN IMMEDIATE;");
        const auto rollback = [&] { execute(database_, "ROLLBACK;"); };
        Statement owner(database_, "SELECT name FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(owner.get(), 1, characterId); sqlite3_bind_int64(owner.get(), 2, *accountId);
        if (sqlite3_step(owner.get()) != SQLITE_ROW) { rollback(); return MarketResult::CharacterNotSelected; }
        const std::string sellerName = columnText(owner.get(), 0);
        Statement total(database_, "SELECT COALESCE(SUM(quantity),0) FROM inventories WHERE character_id=? AND item_id=?;");
        sqlite3_bind_int64(total.get(), 1, characterId); sqlite3_bind_text(total.get(), 2, itemId.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(total.get()) != SQLITE_ROW || sqlite3_column_int64(total.get(), 0) < quantity) {
            rollback(); return MarketResult::InvalidQuantity;
        }
        int remaining = quantity;
        Statement slots(database_, "SELECT slot_index,quantity FROM inventories WHERE character_id=? AND item_id=? ORDER BY slot_index;");
        sqlite3_bind_int64(slots.get(), 1, characterId); sqlite3_bind_text(slots.get(), 2, itemId.c_str(), -1, SQLITE_TRANSIENT);
        std::vector<std::pair<int, int>> deductions;
        while (remaining > 0 && sqlite3_step(slots.get()) == SQLITE_ROW) {
            const int available = sqlite3_column_int(slots.get(), 1);
            const int take = std::min(available, remaining);
            deductions.emplace_back(sqlite3_column_int(slots.get(), 0), take);
            remaining -= take;
        }
        for (const auto [slot, take] : deductions) {
            Statement update(database_, "UPDATE inventories SET quantity=quantity-? WHERE character_id=? AND slot_index=?;");
            sqlite3_bind_int(update.get(), 1, take); sqlite3_bind_int64(update.get(), 2, characterId);
            sqlite3_bind_int(update.get(), 3, slot);
            if (sqlite3_step(update.get()) != SQLITE_DONE) { rollback(); return MarketResult::ServerError; }
        }
        Statement clean(database_, "DELETE FROM inventories WHERE character_id=? AND quantity<=0;");
        sqlite3_bind_int64(clean.get(), 1, characterId);
        if (sqlite3_step(clean.get()) != SQLITE_DONE) { rollback(); return MarketResult::ServerError; }
        Statement listing(database_, "INSERT INTO market_listings(seller_character_id,seller_name,item_id,quantity,unit_price,status,item_name,item_type,max_stack) VALUES(?,?,?,?,?,'Active',?,?,?);");
        sqlite3_bind_int64(listing.get(), 1, characterId);
        sqlite3_bind_text(listing.get(), 2, sellerName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(listing.get(), 3, itemId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(listing.get(), 4, quantity); sqlite3_bind_int64(listing.get(), 5, unitPrice);
        sqlite3_bind_text(listing.get(), 6, itemName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(listing.get(), 7, itemType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(listing.get(), 8, maxStack);
        if (sqlite3_step(listing.get()) != SQLITE_DONE) { rollback(); return MarketResult::ServerError; }
        execute(database_, "COMMIT;");
        return MarketResult::Success;
    } catch (...) { try { execute(database_, "ROLLBACK;"); } catch (...) {} return MarketResult::ServerError; }
}

std::vector<MarketListing> AccountDatabase::browseMarket(const std::string& itemType,
    const std::string& search, int page, int pageSize) const {
    std::vector<MarketListing> result;
    if (page < 0 || pageSize < 1 || pageSize > 50) return result;
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "SELECT listing_id,seller_character_id,seller_name,item_id,item_name,item_type,quantity,unit_price "
            "FROM market_listings WHERE status='Active' AND (?='' OR item_type=?) AND (?='' OR item_name LIKE ?) "
            "ORDER BY created_at DESC,listing_id DESC LIMIT ? OFFSET ?;");
        const std::string pattern = "%" + search + "%";
        sqlite3_bind_text(statement.get(), 1, itemType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 2, itemType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 3, search.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 4, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement.get(), 5, pageSize);
        sqlite3_bind_int64(statement.get(), 6, static_cast<sqlite3_int64>(page) * pageSize);
        while (sqlite3_step(statement.get()) == SQLITE_ROW)
            result.push_back({sqlite3_column_int64(statement.get(), 0), sqlite3_column_int64(statement.get(), 1),
                columnText(statement.get(), 2), columnText(statement.get(), 3), sqlite3_column_int(statement.get(), 6),
                sqlite3_column_int64(statement.get(), 7), columnText(statement.get(), 5), columnText(statement.get(), 4)});
    } catch (...) { result.clear(); }
    return result;
}

std::vector<MarketListing> AccountDatabase::listingsForCharacter(std::int64_t characterId) const {
    std::vector<MarketListing> result;
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "SELECT listing_id,seller_character_id,seller_name,item_id,item_name,item_type,quantity,unit_price "
            "FROM market_listings WHERE seller_character_id=? AND status='Active' ORDER BY listing_id DESC;");
        sqlite3_bind_int64(statement.get(), 1, characterId);
        while (sqlite3_step(statement.get()) == SQLITE_ROW)
            result.push_back({sqlite3_column_int64(statement.get(), 0), sqlite3_column_int64(statement.get(), 1),
                columnText(statement.get(), 2), columnText(statement.get(), 3), sqlite3_column_int(statement.get(), 6),
                sqlite3_column_int64(statement.get(), 7), columnText(statement.get(), 5), columnText(statement.get(), 4)});
    } catch (...) { result.clear(); }
    return result;
}

MarketResult AccountDatabase::buyMarketListing(const std::string& token, std::int64_t characterId,
    std::int64_t listingId) {
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(token);
        if (!accountId) return MarketResult::InvalidSession;
        execute(database_, "BEGIN IMMEDIATE;");
        const auto rollback = [&] { execute(database_, "ROLLBACK;"); };
        Statement buyer(database_, "SELECT gold FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(buyer.get(), 1, characterId); sqlite3_bind_int64(buyer.get(), 2, *accountId);
        if (sqlite3_step(buyer.get()) != SQLITE_ROW) { rollback(); return MarketResult::CharacterNotSelected; }
        const auto gold = sqlite3_column_int64(buyer.get(), 0);
        Statement listing(database_, "SELECT seller_character_id,item_id,quantity,unit_price,max_stack FROM market_listings WHERE listing_id=? AND status='Active';");
        sqlite3_bind_int64(listing.get(), 1, listingId);
        if (sqlite3_step(listing.get()) != SQLITE_ROW) { rollback(); return MarketResult::ListingNotFound; }
        const auto sellerId = sqlite3_column_int64(listing.get(), 0);
        const std::string itemId = columnText(listing.get(), 1);
        const int quantity = sqlite3_column_int(listing.get(), 2);
        const auto price = sqlite3_column_int64(listing.get(), 3);
        const int maxStack = sqlite3_column_int(listing.get(), 4);
        if (sellerId == characterId) { rollback(); return MarketResult::NotSeller; }
        if (quantity <= 0 || price <= 0 || quantity > std::numeric_limits<std::int64_t>::max() / price) {
            rollback(); return MarketResult::ServerError;
        }
        const auto totalPrice = price * quantity;
        if (gold < totalPrice) { rollback(); return MarketResult::InsufficientFunds; }
        std::array<bool, 24> occupied{};
        Statement occupiedRows(database_, "SELECT slot_index FROM inventories WHERE character_id=?;");
        sqlite3_bind_int64(occupiedRows.get(), 1, characterId);
        while (sqlite3_step(occupiedRows.get()) == SQLITE_ROW) {
            const int slot = sqlite3_column_int(occupiedRows.get(), 0);
            if (slot >= 0 && slot < static_cast<int>(occupied.size())) occupied[static_cast<std::size_t>(slot)] = true;
        }
        struct SlotAddition { int slot; int amount; bool exists; };
        std::vector<SlotAddition> additions;
        int remaining = quantity;
        Statement stacks(database_, "SELECT slot_index,quantity FROM inventories WHERE character_id=? AND item_id=? ORDER BY slot_index;");
        sqlite3_bind_int64(stacks.get(), 1, characterId); sqlite3_bind_text(stacks.get(), 2, itemId.c_str(), -1, SQLITE_TRANSIENT);
        while (remaining > 0 && sqlite3_step(stacks.get()) == SQLITE_ROW) {
            const int slot = sqlite3_column_int(stacks.get(), 0);
            const int room = std::max(0, maxStack - sqlite3_column_int(stacks.get(), 1));
            const int amount = std::min(room, remaining);
            if (amount > 0) { additions.push_back({slot, amount, true}); remaining -= amount; }
        }
        for (std::size_t slot = 0; slot < occupied.size() && remaining > 0; ++slot) {
            if (occupied[slot]) continue;
            const int amount = std::min(maxStack, remaining);
            additions.push_back({static_cast<int>(slot), amount, false});
            remaining -= amount;
        }
        if (remaining > 0) { rollback(); return MarketResult::InventoryFull; }
        for (const auto& addition : additions) {
            Statement add(database_, addition.exists
                ? "UPDATE inventories SET quantity=quantity+? WHERE character_id=? AND slot_index=?;"
                : "INSERT INTO inventories(character_id,slot_index,item_id,quantity) VALUES(?,?,?,?);");
            if (addition.exists) {
                sqlite3_bind_int(add.get(), 1, addition.amount); sqlite3_bind_int64(add.get(), 2, characterId);
                sqlite3_bind_int(add.get(), 3, addition.slot);
            } else {
                sqlite3_bind_int64(add.get(), 1, characterId); sqlite3_bind_int(add.get(), 2, addition.slot);
                sqlite3_bind_text(add.get(), 3, itemId.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(add.get(), 4, addition.amount);
            }
            if (sqlite3_step(add.get()) != SQLITE_DONE) { rollback(); return MarketResult::ServerError; }
        }
        Statement debit(database_, "UPDATE characters SET gold=gold-? WHERE character_id=? AND gold>=?;");
        sqlite3_bind_int64(debit.get(), 1, totalPrice); sqlite3_bind_int64(debit.get(), 2, characterId); sqlite3_bind_int64(debit.get(), 3, totalPrice);
        if (sqlite3_step(debit.get()) != SQLITE_DONE || sqlite3_changes(database_) != 1) { rollback(); return MarketResult::InsufficientFunds; }
        Statement credit(database_, "UPDATE characters SET gold=gold+? WHERE character_id=? AND gold<=9223372036854775807-?;");
        sqlite3_bind_int64(credit.get(), 1, totalPrice); sqlite3_bind_int64(credit.get(), 2, sellerId); sqlite3_bind_int64(credit.get(), 3, totalPrice);
        if (sqlite3_step(credit.get()) != SQLITE_DONE || sqlite3_changes(database_) != 1) { rollback(); return MarketResult::ServerError; }
        Statement sold(database_, "UPDATE market_listings SET status='Sold' WHERE listing_id=? AND status='Active';");
        sqlite3_bind_int64(sold.get(), 1, listingId);
        if (sqlite3_step(sold.get()) != SQLITE_DONE || sqlite3_changes(database_) != 1) { rollback(); return MarketResult::ListingNotFound; }
        execute(database_, "COMMIT;");
        return MarketResult::Success;
    } catch (...) { try { execute(database_, "ROLLBACK;"); } catch (...) {} return MarketResult::ServerError; }
}

MarketResult AccountDatabase::cancelMarketListing(const std::string& token, std::int64_t characterId,
    std::int64_t listingId) {
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(token);
        if (!accountId) return MarketResult::InvalidSession;
        execute(database_, "BEGIN IMMEDIATE;");
        const auto rollback = [&] { execute(database_, "ROLLBACK;"); };
        Statement owner(database_, "SELECT 1 FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(owner.get(), 1, characterId); sqlite3_bind_int64(owner.get(), 2, *accountId);
        if (sqlite3_step(owner.get()) != SQLITE_ROW) { rollback(); return MarketResult::CharacterNotSelected; }
        Statement listing(database_, "SELECT seller_character_id,item_id,quantity,max_stack FROM market_listings WHERE listing_id=? AND status='Active';");
        sqlite3_bind_int64(listing.get(), 1, listingId);
        if (sqlite3_step(listing.get()) != SQLITE_ROW) { rollback(); return MarketResult::ListingNotFound; }
        if (sqlite3_column_int64(listing.get(), 0) != characterId) { rollback(); return MarketResult::NotSeller; }
        const std::string itemId = columnText(listing.get(), 1);
        const int quantity = sqlite3_column_int(listing.get(), 2);
        const int maxStack = sqlite3_column_int(listing.get(), 3);
        std::array<bool, 24> occupied{};
        Statement occupiedRows(database_, "SELECT slot_index FROM inventories WHERE character_id=?;");
        sqlite3_bind_int64(occupiedRows.get(), 1, characterId);
        while (sqlite3_step(occupiedRows.get()) == SQLITE_ROW) {
            const int slot = sqlite3_column_int(occupiedRows.get(), 0);
            if (slot >= 0 && slot < static_cast<int>(occupied.size())) occupied[static_cast<std::size_t>(slot)] = true;
        }
        struct SlotAddition { int slot; int amount; bool exists; };
        std::vector<SlotAddition> additions;
        int remaining = quantity;
        Statement stacks(database_, "SELECT slot_index,quantity FROM inventories WHERE character_id=? AND item_id=? ORDER BY slot_index;");
        sqlite3_bind_int64(stacks.get(), 1, characterId); sqlite3_bind_text(stacks.get(), 2, itemId.c_str(), -1, SQLITE_TRANSIENT);
        while (remaining > 0 && sqlite3_step(stacks.get()) == SQLITE_ROW) {
            const int slot = sqlite3_column_int(stacks.get(), 0);
            const int room = std::max(0, maxStack - sqlite3_column_int(stacks.get(), 1));
            const int amount = std::min(room, remaining);
            if (amount > 0) { additions.push_back({slot, amount, true}); remaining -= amount; }
        }
        for (std::size_t slot = 0; slot < occupied.size() && remaining > 0; ++slot) {
            if (occupied[slot]) continue;
            const int amount = std::min(maxStack, remaining);
            additions.push_back({static_cast<int>(slot), amount, false});
            remaining -= amount;
        }
        if (remaining > 0) { rollback(); return MarketResult::InventoryFull; }
        for (const auto& addition : additions) {
            Statement add(database_, addition.exists
                ? "UPDATE inventories SET quantity=quantity+? WHERE character_id=? AND slot_index=?;"
                : "INSERT INTO inventories(character_id,slot_index,item_id,quantity) VALUES(?,?,?,?);");
            if (addition.exists) {
                sqlite3_bind_int(add.get(), 1, addition.amount); sqlite3_bind_int64(add.get(), 2, characterId);
                sqlite3_bind_int(add.get(), 3, addition.slot);
            } else {
                sqlite3_bind_int64(add.get(), 1, characterId); sqlite3_bind_int(add.get(), 2, addition.slot);
                sqlite3_bind_text(add.get(), 3, itemId.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(add.get(), 4, addition.amount);
            }
            if (sqlite3_step(add.get()) != SQLITE_DONE) { rollback(); return MarketResult::ServerError; }
        }
        Statement cancel(database_, "UPDATE market_listings SET status='Cancelled' WHERE listing_id=? AND status='Active';");
        sqlite3_bind_int64(cancel.get(), 1, listingId);
        if (sqlite3_step(cancel.get()) != SQLITE_DONE || sqlite3_changes(database_) != 1) { rollback(); return MarketResult::ListingNotFound; }
        execute(database_, "COMMIT;");
        return MarketResult::Success;
    } catch (...) { try { execute(database_, "ROLLBACK;"); } catch (...) {} return MarketResult::ServerError; }
}

std::int64_t AccountDatabase::characterGold(std::int64_t characterId) const {
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "SELECT gold FROM characters WHERE character_id=?;");
        sqlite3_bind_int64(statement.get(), 1, characterId);
        return sqlite3_step(statement.get()) == SQLITE_ROW ? sqlite3_column_int64(statement.get(), 0) : 0;
    } catch (...) { return 0; }
}

std::int64_t AccountDatabase::marketListingSeller(std::int64_t listingId) const {
    try {
        std::lock_guard lock(mutex_);
        Statement statement(database_, "SELECT seller_character_id FROM market_listings WHERE listing_id=?;");
        sqlite3_bind_int64(statement.get(), 1, listingId);
        return sqlite3_step(statement.get()) == SQLITE_ROW ? sqlite3_column_int64(statement.get(), 0) : 0;
    } catch (...) { return 0; }
}

FeedbackSubmissionResult AccountDatabase::submitFeedback(const std::string& sessionToken, std::int64_t characterId,
    const glory::network::FeedbackSubmitRequestPacket& feedback) {
    if (feedback.rating < 1 || feedback.rating > 5 || feedback.subject.empty() ||
        feedback.subject.size() > 120 || feedback.description.empty() || feedback.description.size() > 600 ||
        feedback.diagnosticsJson.size() > 450 ||
        static_cast<std::uint8_t>(feedback.category) >
            static_cast<std::uint8_t>(glory::network::FeedbackCategory::Praise)) return FeedbackSubmissionResult::Invalid;
    const auto diagnostics = nlohmann::json::parse(feedback.diagnosticsJson, nullptr, false);
    if (diagnostics.is_discarded() || !diagnostics.is_object()) return FeedbackSubmissionResult::Invalid;
    try {
        std::lock_guard lock(mutex_);
        const auto accountId = accountForToken(sessionToken);
        if (!accountId) return FeedbackSubmissionResult::Invalid;
        const auto now = std::chrono::steady_clock::now();
        Statement character(database_, "SELECT name FROM characters WHERE character_id=? AND account_id=?;");
        sqlite3_bind_int64(character.get(), 1, characterId);
        sqlite3_bind_int64(character.get(), 2, *accountId);
        if (sqlite3_step(character.get()) != SQLITE_ROW) return FeedbackSubmissionResult::Invalid;
        const auto previous = lastFeedbackSubmission_.find(*accountId);
        if (previous != lastFeedbackSubmission_.end() && now - previous->second < std::chrono::seconds(60))
            return FeedbackSubmissionResult::RateLimited;
        const std::string characterName = columnText(character.get(), 0);
        Statement insert(database_, "INSERT INTO feedback(account_id,character_name,category,rating,subject,description,diagnostics_json) "
            "VALUES(?,?,?,?,?,?,?);");
        sqlite3_bind_int64(insert.get(), 1, *accountId);
        sqlite3_bind_text(insert.get(), 2, characterName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 3, feedbackCategoryName(feedback.category), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert.get(), 4, feedback.rating);
        sqlite3_bind_text(insert.get(), 5, feedback.subject.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 6, feedback.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 7, feedback.diagnosticsJson.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insert.get()) != SQLITE_DONE) return FeedbackSubmissionResult::Invalid;
        lastFeedbackSubmission_[*accountId] = now;
        return FeedbackSubmissionResult::Success;
    } catch (...) { return FeedbackSubmissionResult::Invalid; }
}

bool AccountDatabase::exportFeedbackToJson(const std::filesystem::path& outputPath) const {
    try {
        const char* databaseFilename = sqlite3_db_filename(database_, "main");
        if (databaseFilename && *databaseFilename) {
            const auto outputAbsolute = std::filesystem::weakly_canonical(std::filesystem::absolute(outputPath));
            const auto databaseAbsolute = std::filesystem::weakly_canonical(std::filesystem::absolute(databaseFilename));
            if (outputAbsolute == databaseAbsolute) return false;
            std::error_code equivalentError;
            if (std::filesystem::exists(outputAbsolute) && std::filesystem::exists(databaseAbsolute) &&
                std::filesystem::equivalent(outputAbsolute, databaseAbsolute, equivalentError) && !equivalentError)
                return false;
        }
        const auto parent = outputPath.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::lock_guard lock(mutex_);
        Statement rows(database_, "SELECT feedback_id,account_id,character_name,category,rating,subject,description,diagnostics_json,created_at "
            "FROM feedback ORDER BY feedback_id;");
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output << "[";
        bool first = true;
        int stepResult = SQLITE_OK;
        while ((stepResult = sqlite3_step(rows.get())) == SQLITE_ROW) {
            if (!first) output << ',';
            first = false;
            output << "{\"feedback_id\":" << sqlite3_column_int64(rows.get(), 0)
                << ",\"account_id\":" << sqlite3_column_int64(rows.get(), 1)
                << ",\"character_name\":\"" << jsonEscape(columnText(rows.get(), 2))
                << "\",\"category\":\"" << jsonEscape(columnText(rows.get(), 3))
                << "\",\"rating\":" << sqlite3_column_int(rows.get(), 4)
                << ",\"subject\":\"" << jsonEscape(columnText(rows.get(), 5))
                << "\",\"description\":\"" << jsonEscape(columnText(rows.get(), 6))
                << "\",\"diagnostics_json\":" << columnText(rows.get(), 7)
                << ",\"created_at\":\"" << jsonEscape(columnText(rows.get(), 8)) << "\"}";
        }
        if (stepResult != SQLITE_DONE) return false;
        output << "]\n";
        return output.good();
    } catch (...) { return false; }
}

} // namespace glory::server

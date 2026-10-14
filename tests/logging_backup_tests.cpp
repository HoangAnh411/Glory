#include "glory/server/account_database.hpp"
#include "glory/server/database_backup.hpp"
#include "glory/server/structured_logger.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::filesystem::path temporaryDirectory() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("glory_logging_backup_" + std::to_string(stamp));
}

void setGold(const std::filesystem::path& databasePath, std::int64_t characterId, std::int64_t gold) {
    sqlite3* database = nullptr;
    require(sqlite3_open(databasePath.string().c_str(), &database) == SQLITE_OK,
        "could not open database to seed character gold");
    sqlite3_stmt* statement = nullptr;
    const int prepareResult = sqlite3_prepare_v2(database,
        "UPDATE characters SET gold=? WHERE character_id=?;", -1, &statement, nullptr);
    if (prepareResult != SQLITE_OK) {
        sqlite3_close(database);
        throw std::runtime_error("could not prepare gold update");
    }
    sqlite3_bind_int64(statement, 1, gold);
    sqlite3_bind_int64(statement, 2, characterId);
    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);
    sqlite3_close(database);
    require(stepResult == SQLITE_DONE, "could not persist character gold");
}

void testLogger(const std::filesystem::path& directory) {
    using namespace glory::server;
    const auto logPath = directory / "nested" / "server.log";
    {
        StructuredLogger logger(logPath, LogLevel::Info);
        logger.log(LogLevel::Debug, LogCategory::System, "filtered debug message");
        logger.log(LogLevel::Info, LogCategory::Database, "included info message");
        logger.setMinimumLevel(LogLevel::Error);
        logger.log(LogLevel::Warn, LogCategory::Network, "filtered warning message");
        logger.log(LogLevel::Error, LogCategory::Auth, "included error message");
        logger.log(LogLevel::Critical, LogCategory::System, "included critical message");
        logger.setMinimumLevel(LogLevel::Debug);
        logger.log(LogLevel::Debug, LogCategory::Combat, "debug enabled dynamically");
        logger.log(LogLevel::Warn, LogCategory::Network, "warning enabled dynamically");

        std::array<std::thread, 4> writers;
        for (std::size_t worker = 0; worker < writers.size(); ++worker) {
            writers[worker] = std::thread([&logger, worker] {
                for (int entry = 0; entry < 5; ++entry) {
                    const auto category = (worker % 2 == 0)
                        ? LogCategory::Economy : LogCategory::AntiCheat;
                    logger.log(LogLevel::Info, category,
                        "concurrent entry " + std::to_string(worker) + ":" + std::to_string(entry));
                }
            });
        }
        for (auto& writer : writers) writer.join();
        logger.flush();
    }

    require(std::filesystem::exists(logPath), "logger did not create nested parent directories and file");
    std::ifstream input(logPath);
    require(static_cast<bool>(input), "could not read structured log output");
    const std::regex format(R"(^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\] \[(DEBUG|INFO|WARN|ERROR|CRITICAL)\] \[(Network|Auth|Combat|Economy|AntiCheat|Database|System)\] .+$)");
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(input, line)) {
        require(std::regex_match(line, format), "logger line did not match the required timestamp/category format");
        lines.push_back(line);
    }
    require(lines.size() == 25, "logger filtering or concurrent line count was incorrect");
    const auto contains = [&](const std::string& fragment) {
        return std::any_of(lines.begin(), lines.end(), [&](const auto& entry) {
            return entry.find(fragment) != std::string::npos;
        });
    };
    require(!contains("filtered debug") && !contains("filtered warning"),
        "logger did not filter messages below the configured level");
    require(contains("[Database] included info") && contains("[Auth] included error") &&
        contains("[System] included critical") && contains("debug enabled dynamically") &&
        contains("[WARN] [Network] warning enabled dynamically"),
        "logger did not preserve emitted levels and categories");

    LogLevel parsed = LogLevel::Debug;
    require(parseLogLevel("WARN", parsed) && parsed == LogLevel::Warn &&
        parseLogLevel("CRITICAL", parsed) && parsed == LogLevel::Critical &&
        !parseLogLevel("TRACE", parsed), "log-level parser accepted or rejected the wrong values");
}

void testOnlineBackupAndRotation(const std::filesystem::path& directory) {
    using namespace glory;
    const auto sourcePath = directory / "accounts.sqlite3";
    const auto backupPath = directory / "manual-backup.sqlite3";
    std::int64_t characterId = 0;
    {
        server::AccountDatabase source(sourcePath.string());
        require(source.registerAccount("backup_user", "strong-password") == network::AuthResult::Success,
            "backup source account registration failed");
        const auto login = source.authenticateAccount("backup_user", "strong-password");
        require(login.result == network::AuthResult::Success, "backup source account login failed");
        const auto character = source.createCharacter(login.sessionToken, "BackupHero", "Vanguard");
        require(character.result == network::AuthResult::Success, "backup source character creation failed");
        characterId = character.character.characterId;
        const std::vector<server::PersistedInventorySlot> inventory{{0, "iron_sword", 2},
            {5, "health_potion", 7}};
        const std::vector<server::PersistedEquipmentItem> equipment{{0, "iron_sword"},
            {2, "travel_charm"}};
        require(source.saveCharacterState(login.sessionToken, characterId, inventory, equipment),
            "backup source inventory/equipment save failed");
        setGold(sourcePath, characterId, 4321);

        server::DatabaseBackupManager backup(sourcePath);
        require(backup.createBackup(backupPath) == backupPath, "online backup did not create its destination");
        std::string integrity;
        require(backup.verifyBackupIntegrity(backupPath, &integrity) && integrity == "ok",
            "online backup failed SQLite integrity_check");

        server::AccountDatabase restored(backupPath.string());
        const auto restoredLogin = restored.authenticateAccount("backup_user", "strong-password");
        require(restoredLogin.result == network::AuthResult::Success,
            "restored backup did not retain account credentials");
        const auto restoredCharacters = restored.listCharacters(restoredLogin.sessionToken);
        require(restoredCharacters.result == network::AuthResult::Success &&
            restoredCharacters.characters.size() == 1 &&
            restoredCharacters.characters.front().characterId == characterId &&
            restoredCharacters.characters.front().name == "BackupHero",
            "restored backup did not retain the character record");
        require(restored.characterGold(characterId) == 4321,
            "restored backup did not retain character gold");
        std::vector<server::PersistedInventorySlot> restoredInventory;
        std::vector<server::PersistedEquipmentItem> restoredEquipment;
        require(restored.loadCharacterState(characterId, restoredInventory, restoredEquipment) &&
            restoredInventory.size() == inventory.size() && restoredEquipment.size() == equipment.size() &&
            restoredInventory[0].slotIndex == inventory[0].slotIndex &&
            restoredInventory[0].itemId == inventory[0].itemId &&
            restoredInventory[0].quantity == inventory[0].quantity &&
            restoredInventory[1].slotIndex == inventory[1].slotIndex &&
            restoredInventory[1].itemId == inventory[1].itemId &&
            restoredInventory[1].quantity == inventory[1].quantity &&
            restoredEquipment[0].slotType == equipment[0].slotType &&
            restoredEquipment[0].itemId == equipment[0].itemId &&
            restoredEquipment[1].slotType == equipment[1].slotType &&
            restoredEquipment[1].itemId == equipment[1].itemId,
            "restored backup did not retain inventory and equipment");

        const auto invalidPath = directory / "invalid.sqlite3";
        {
            std::ofstream invalid(invalidPath);
            invalid << "not a sqlite database";
        }
        require(!backup.verifyBackupIntegrity(invalidPath),
            "integrity check incorrectly accepted a non-SQLite file");

        const auto backupDirectory = directory / "rotating";
        std::filesystem::create_directories(backupDirectory);
        std::vector<std::filesystem::path> createdBackups;
        for (int index = 0; index < 4; ++index)
            createdBackups.push_back(backup.createTimestampedBackup(backupDirectory));
        std::set<std::string> uniqueNames;
        for (const auto& path : createdBackups) uniqueNames.insert(path.filename().string());
        require(uniqueNames.size() == createdBackups.size(), "timestamped backup names were not unique");
        std::sort(createdBackups.begin(), createdBackups.end(), [](const auto& left, const auto& right) {
            return left.filename().string() > right.filename().string();
        });
        const auto unrelated = backupDirectory / "unrelated.sqlite3";
        std::ofstream(unrelated) << "keep this file";
        require(backup.rotateBackups(backupDirectory, 2) == 2,
            "backup rotation did not remove the excess timestamped backups");
        std::set<std::string> retainedNames;
        for (const auto& entry : std::filesystem::directory_iterator(backupDirectory)) {
            if (entry.path().filename().string().starts_with("glory-backup-") &&
                entry.path().extension() == ".sqlite3") retainedNames.insert(entry.path().filename().string());
        }
        require(retainedNames.size() == 2 && std::filesystem::exists(unrelated) &&
            retainedNames.contains(createdBackups[0].filename().string()) &&
            retainedNames.contains(createdBackups[1].filename().string()),
            "backup rotation did not keep the two newest backups or removed an unrelated file");
    }
}

} // namespace

int main() {
    const auto directory = temporaryDirectory();
    try {
        std::filesystem::create_directories(directory);
        testLogger(directory);
        testOnlineBackupAndRotation(directory);
        std::filesystem::remove_all(directory);
        std::cout << "Structured logger and SQLite online backup passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

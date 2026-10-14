#include "glory/server/database_backup.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace glory::server {
namespace {

struct DatabaseCloser {
    void operator()(sqlite3* database) const noexcept {
        if (database) sqlite3_close(database);
    }
};
using DatabaseHandle = std::unique_ptr<sqlite3, DatabaseCloser>;

std::string sqliteError(sqlite3* database, const char* operation) {
    return std::string(operation) + ": " + (database ? sqlite3_errmsg(database) : "SQLite operation failed");
}

std::string timestampName() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto millisecondsPart = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t time = system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream name;
    name << "glory-backup-" << std::put_time(&local, "%Y%m%d-%H%M%S-")
        << std::setfill('0') << std::setw(3) << millisecondsPart.count();
    return name.str();
}

bool ownedBackup(const std::filesystem::directory_entry& entry) {
    if (!entry.is_regular_file()) return false;
    const auto filename = entry.path().filename().string();
    return filename.starts_with("glory-backup-") && entry.path().extension() == ".sqlite3";
}

} // namespace

DatabaseBackupManager::DatabaseBackupManager(std::filesystem::path sourceDatabase)
    : sourceDatabase_(std::move(sourceDatabase)) {
    if (sourceDatabase_.empty() || sourceDatabase_ == ":memory:")
        throw std::invalid_argument("A file-backed SQLite source database is required");
}

std::filesystem::path DatabaseBackupManager::createBackup(
    const std::filesystem::path& destinationPath) const {
    if (destinationPath.empty()) throw std::invalid_argument("Backup destination path is empty");
    const auto sourceAbsolute = std::filesystem::weakly_canonical(sourceDatabase_);
    const auto destinationAbsolute = std::filesystem::absolute(destinationPath).lexically_normal();
    if (sourceAbsolute == destinationAbsolute)
        throw std::invalid_argument("Backup destination must not be the source database");
    if (std::filesystem::exists(destinationPath))
        throw std::runtime_error("Backup destination already exists: " + destinationPath.string());
    const auto parent = destinationPath.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);

    sqlite3* sourceRaw = nullptr;
    if (sqlite3_open_v2(sourceDatabase_.string().c_str(), &sourceRaw,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        const std::string message = sqliteError(sourceRaw, "Unable to open source database for backup");
        if (sourceRaw) sqlite3_close(sourceRaw);
        throw std::runtime_error(message);
    }
    DatabaseHandle source(sourceRaw);
    sqlite3_busy_timeout(source.get(), 5000);

    sqlite3* destinationRaw = nullptr;
    if (sqlite3_open_v2(destinationPath.string().c_str(), &destinationRaw,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        const std::string message = sqliteError(destinationRaw, "Unable to create backup database");
        if (destinationRaw) sqlite3_close(destinationRaw);
        throw std::runtime_error(message);
    }
    DatabaseHandle destination(destinationRaw);
    sqlite3_busy_timeout(destination.get(), 5000);

    sqlite3_backup* backup = sqlite3_backup_init(destination.get(), "main", source.get(), "main");
    if (!backup) {
        const std::string message = sqliteError(destination.get(), "Unable to initialize SQLite online backup");
        destination.reset();
        source.reset();
        std::filesystem::remove(destinationPath);
        throw std::runtime_error(message);
    }

    int stepResult = SQLITE_OK;
    do {
        stepResult = sqlite3_backup_step(backup, 128);
        if (stepResult == SQLITE_BUSY || stepResult == SQLITE_LOCKED)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (stepResult == SQLITE_OK || stepResult == SQLITE_BUSY || stepResult == SQLITE_LOCKED);
    const int finishResult = sqlite3_backup_finish(backup);
    if (stepResult != SQLITE_DONE || finishResult != SQLITE_OK) {
        const std::string message = sqliteError(destination.get(), "SQLite online backup failed");
        destination.reset();
        source.reset();
        std::filesystem::remove(destinationPath);
        throw std::runtime_error(message);
    }
    destination.reset();
    source.reset();
    return destinationPath;
}

std::filesystem::path DatabaseBackupManager::createTimestampedBackup(
    const std::filesystem::path& backupDirectory) const {
    std::filesystem::create_directories(backupDirectory);
    const std::string base = timestampName();
    auto destination = backupDirectory / (base + ".sqlite3");
    for (std::size_t suffix = 1; std::filesystem::exists(destination); ++suffix)
        destination = backupDirectory / (base + "-" + std::to_string(suffix) + ".sqlite3");
    return createBackup(destination);
}

bool DatabaseBackupManager::verifyBackupIntegrity(const std::filesystem::path& backupPath,
    std::string* resultMessage) const {
    if (resultMessage) resultMessage->clear();
    sqlite3* databaseRaw = nullptr;
    if (sqlite3_open_v2(backupPath.string().c_str(), &databaseRaw,
        SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        if (resultMessage) *resultMessage = sqliteError(databaseRaw, "Unable to open backup");
        if (databaseRaw) sqlite3_close(databaseRaw);
        return false;
    }
    DatabaseHandle database(databaseRaw);
    sqlite3_stmt* statementRaw = nullptr;
    if (sqlite3_prepare_v2(database.get(), "PRAGMA integrity_check;", -1, &statementRaw, nullptr) != SQLITE_OK) {
        if (resultMessage) *resultMessage = sqliteError(database.get(), "Unable to prepare integrity check");
        return false;
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(statementRaw, sqlite3_finalize);
    bool sawResult = false;
    bool valid = true;
    std::ostringstream details;
    int stepResult = SQLITE_OK;
    while ((stepResult = sqlite3_step(statement.get())) == SQLITE_ROW) {
        sawResult = true;
        const auto* value = sqlite3_column_text(statement.get(), 0);
        const std::string row = value ? reinterpret_cast<const char*>(value) : "";
        if (row != "ok") valid = false;
        if (details.tellp() > 0) details << "; ";
        details << row;
    }
    if (stepResult != SQLITE_DONE || !sawResult) {
        valid = false;
        if (stepResult != SQLITE_DONE) details << sqlite3_errmsg(database.get());
    }
    if (resultMessage) *resultMessage = details.str();
    return valid;
}

std::size_t DatabaseBackupManager::rotateBackups(const std::filesystem::path& backupDirectory,
    std::size_t maxRetained) const {
    if (!std::filesystem::exists(backupDirectory)) return 0;
    std::vector<std::filesystem::path> backups;
    for (const auto& entry : std::filesystem::directory_iterator(backupDirectory))
        if (ownedBackup(entry)) backups.push_back(entry.path());
    std::sort(backups.begin(), backups.end(), [](const auto& left, const auto& right) {
        return left.filename().string() > right.filename().string();
    });
    std::size_t removed = 0;
    for (std::size_t index = maxRetained; index < backups.size(); ++index) {
        if (std::filesystem::remove(backups[index])) ++removed;
    }
    return removed;
}

} // namespace glory::server

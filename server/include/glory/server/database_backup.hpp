#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace glory::server {

class DatabaseBackupManager {
public:
    explicit DatabaseBackupManager(std::filesystem::path sourceDatabase);

    [[nodiscard]] std::filesystem::path createBackup(const std::filesystem::path& destinationPath) const;
    [[nodiscard]] std::filesystem::path createTimestampedBackup(
        const std::filesystem::path& backupDirectory) const;
    [[nodiscard]] bool verifyBackupIntegrity(const std::filesystem::path& backupPath,
        std::string* resultMessage = nullptr) const;
    [[nodiscard]] std::size_t rotateBackups(const std::filesystem::path& backupDirectory,
        std::size_t maxRetained) const;

private:
    std::filesystem::path sourceDatabase_;
};

} // namespace glory::server

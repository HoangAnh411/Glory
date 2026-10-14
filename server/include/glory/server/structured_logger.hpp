#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace glory::server {

enum class LogLevel { Debug, Info, Warn, Error, Critical };
enum class LogCategory { Network, Auth, Combat, Economy, AntiCheat, Database, System };

[[nodiscard]] const char* logLevelName(LogLevel level) noexcept;
[[nodiscard]] const char* logCategoryName(LogCategory category) noexcept;
[[nodiscard]] bool parseLogLevel(std::string_view value, LogLevel& level) noexcept;

class StructuredLogger {
public:
    explicit StructuredLogger(std::filesystem::path filePath = {},
        LogLevel minimumLevel = LogLevel::Info);
    StructuredLogger(const StructuredLogger&) = delete;
    StructuredLogger& operator=(const StructuredLogger&) = delete;

    void setMinimumLevel(LogLevel level);
    [[nodiscard]] LogLevel minimumLevel() const;
    void log(LogLevel level, LogCategory category, std::string_view message);
    void flush();

private:
    mutable std::mutex mutex_;
    LogLevel minimumLevel_;
    std::filesystem::path filePath_;
    std::ofstream file_;
};

} // namespace glory::server

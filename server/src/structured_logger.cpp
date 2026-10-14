#include "glory/server/structured_logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace glory::server {
namespace {

std::string timestampNow() {
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
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.'
        << std::setfill('0') << std::setw(3) << millisecondsPart.count();
    return output.str();
}

std::string singleLine(std::string_view message) {
    std::string result(message);
    for (char& character : result)
        if (character == '\r' || character == '\n') character = ' ';
    return result;
}

} // namespace

const char* logLevelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

const char* logCategoryName(LogCategory category) noexcept {
    switch (category) {
    case LogCategory::Network: return "Network";
    case LogCategory::Auth: return "Auth";
    case LogCategory::Combat: return "Combat";
    case LogCategory::Economy: return "Economy";
    case LogCategory::AntiCheat: return "AntiCheat";
    case LogCategory::Database: return "Database";
    case LogCategory::System: return "System";
    }
    return "Unknown";
}

bool parseLogLevel(std::string_view value, LogLevel& level) noexcept {
    if (value == "DEBUG") level = LogLevel::Debug;
    else if (value == "INFO") level = LogLevel::Info;
    else if (value == "WARN") level = LogLevel::Warn;
    else if (value == "ERROR") level = LogLevel::Error;
    else if (value == "CRITICAL") level = LogLevel::Critical;
    else return false;
    return true;
}

StructuredLogger::StructuredLogger(std::filesystem::path filePath, LogLevel minimumLevel)
    : minimumLevel_(minimumLevel), filePath_(std::move(filePath)) {
    if (filePath_.empty()) return;
    const auto parent = filePath_.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    file_.open(filePath_, std::ios::out | std::ios::app);
    if (!file_) throw std::runtime_error("Unable to open log file: " + filePath_.string());
}

void StructuredLogger::setMinimumLevel(LogLevel level) {
    std::lock_guard lock(mutex_);
    minimumLevel_ = level;
}

LogLevel StructuredLogger::minimumLevel() const {
    std::lock_guard lock(mutex_);
    return minimumLevel_;
}

void StructuredLogger::log(LogLevel level, LogCategory category, std::string_view message) {
    std::lock_guard lock(mutex_);
    if (level < minimumLevel_) return;
    const std::string line = "[" + timestampNow() + "] [" + logLevelName(level) + "] [" +
        logCategoryName(category) + "] " + singleLine(message) + '\n';
    std::clog << line;
    std::clog.flush();
    if (file_) {
        file_ << line;
        file_.flush();
        if (!file_) throw std::runtime_error("Failed writing log file: " + filePath_.string());
    }
}

void StructuredLogger::flush() {
    std::lock_guard lock(mutex_);
    std::clog.flush();
    if (file_) file_.flush();
}

} // namespace glory::server

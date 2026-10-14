#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace glory::server {

enum class BotState {
    Disconnected,
    Connecting,
    Authenticating,
    EnteringWorld,
    SimulatingLoop,
    Disconnecting
};

struct LoadTestConfig {
    std::size_t botCount = 8;
    std::uint64_t tickCount = 120;
};

struct LoadTestMetrics {
    std::uint64_t ticksCompleted = 0;
    double avgTickDurationMs = 0.0;
    double maxTickDurationMs = 0.0;
    std::uint64_t packetsSent = 0;
    std::uint64_t packetsReceived = 0;
    std::uint64_t bytesTransferred = 0;
    std::size_t connectedBots = 0;
    std::size_t requestedBots = 0;
    std::size_t snapshotsReceived = 0;
    std::size_t inputPacketsProcessed = 0;
    std::size_t remainingServerPlayers = 0;

    [[nodiscard]] double connectedBotsRate() const noexcept;
};

class LoadTestCoordinator {
public:
    explicit LoadTestCoordinator(LoadTestConfig config);
    [[nodiscard]] LoadTestMetrics run();

private:
    LoadTestConfig config_;
};

void printLoadTestSummary(const LoadTestConfig& config, const LoadTestMetrics& metrics,
    std::ostream& output);

} // namespace glory::server

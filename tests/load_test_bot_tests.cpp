#include "glory/server/load_test.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    try {
        const glory::server::LoadTestConfig config{8, 120};
        const auto metrics = glory::server::LoadTestCoordinator(config).run();
        require(metrics.ticksCompleted == 120, "load test did not complete 120 fixed ticks");
        require(metrics.connectedBots == 8 && metrics.connectedBotsRate() == 1.0,
            "all eight loopback bots did not connect and enter the world");
        require(metrics.inputPacketsProcessed >= 8 * 120,
            "server did not process movement and attack input from every bot on every tick");
        require(metrics.snapshotsReceived >= 8 * 120,
            "bots did not process authoritative snapshots throughout the test");
        require(metrics.packetsSent > 0 && metrics.packetsReceived > 0 && metrics.bytesTransferred > 0,
            "network traffic metrics were not collected");
        require(metrics.avgTickDurationMs < 16.6, "average server simulation tick exceeded 16.6 ms");
        require(metrics.remainingServerPlayers == 0, "server retained players after swarm disconnect");
        std::cout << "8-bot ENet loopback passed: 120 ticks, avg tick "
            << metrics.avgTickDurationMs << " ms\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

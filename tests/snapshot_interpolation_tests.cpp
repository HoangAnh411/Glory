#include "glory/network/network.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

glory::network::ServerSnapshotPacket snapshot(std::uint64_t tick, float remoteX, float yaw,
    bool includeRemote = true, float remoteVelocityX = 0.0F) {
    using namespace glory::network;
    ServerSnapshotPacket packet;
    packet.serverTick = tick;
    packet.players.push_back({1, {0.0F, 0.0F, 0.0F}, {}, 0.0F, 100.0F, 100.0F, "Idle"});
    if (includeRemote)
        packet.players.push_back({2, {remoteX, 0.0F, 0.0F}, {remoteVelocityX, 0.0F, 0.0F},
            yaw, 80.0F, 100.0F, "Run", false, false, "slash"});
    return packet;
}

float sampleX(double delayMs) {
    glory::network::SnapshotInterpolator interpolator(8, delayMs, 100.0);
    require(interpolator.addSnapshot(snapshot(10, 0.0F, 350.0F), 1000.0), "first snapshot rejected");
    require(interpolator.addSnapshot(snapshot(16, 6.0F, 10.0F), 1100.0), "second snapshot rejected");
    const auto players = interpolator.sample(1, {}, 1100.0);
    require(players.size() == 1, "remote player missing while sampling");
    return players.front().state.position.x;
}

} // namespace

int main() {
    try {
        require(std::abs(sampleX(100.0) - 0.0F) < 0.001F, "alpha 0 position interpolation failed");
        require(std::abs(sampleX(50.0) - 3.0F) < 0.001F, "alpha 0.5 position interpolation failed");
        require(std::abs(sampleX(0.0) - 6.0F) < 0.001F, "alpha 1 position interpolation failed");

        glory::network::SnapshotInterpolator yawInterpolator(8, 50.0, 100.0);
        yawInterpolator.addSnapshot(snapshot(10, 0.0F, 350.0F), 1000.0);
        yawInterpolator.addSnapshot(snapshot(16, 6.0F, 10.0F), 1100.0);
        auto remotes = yawInterpolator.sample(1, {}, 1100.0);
        require(remotes.size() == 1 &&
            (std::abs(remotes.front().state.yaw) < 0.01F || std::abs(remotes.front().state.yaw - 360.0F) < 0.01F),
            "shortest-angle interpolation did not cross 350 to 10 through zero");

        glory::network::SnapshotInterpolator reordered(8, 100.0, 100.0);
        require(reordered.addSnapshot(snapshot(16, 16.0F, 0.0F), 1100.0), "newer snapshot rejected");
        require(reordered.addSnapshot(snapshot(10, 10.0F, 0.0F), 1000.0), "out-of-order snapshot rejected");
        require(reordered.bufferedSnapshotCount() == 2, "out-of-order snapshots not buffered");
        remotes = reordered.sample(1, {}, 1100.0);
        require(remotes.size() == 1 && std::abs(remotes.front().state.position.x - 10.0F) < 0.001F,
            "out-of-order snapshot buffer was not sorted by server tick");

        glory::network::SnapshotInterpolator extrapolator(8, 0.0, 100.0);
        extrapolator.addSnapshot(snapshot(20, 4.0F, 0.0F, true, 10.0F), 1000.0);
        remotes = extrapolator.sample(1, {}, 1200.0);
        require(remotes.size() == 1 && std::abs(remotes.front().state.position.x - 5.0F) < 0.001F,
            "dead reckoning exceeded or ignored the 100ms extrapolation cap");

        require(extrapolator.addSnapshot(snapshot(21, 0.0F, 0.0F, false), 1216.0),
            "disconnect snapshot rejected");
        remotes = extrapolator.sample(1, {}, 1216.0);
        require(remotes.empty(), "remote player remained after disappearing from server snapshot");

        std::cout << "Snapshot interpolation passed: endpoints, angle wrap, ordering, extrapolation, cleanup\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

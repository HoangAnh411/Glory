#include "glory/engine/fixed_timestep.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testFixedUpdatesAndRemainder() {
    glory::engine::FixedTimestep timestep(0.1, 0.25, 8);
    std::uint32_t updates = 0;
    double updateDelta = 0.0;

    expect(timestep.advance(0.05, [&](double delta) {
        ++updates;
        updateDelta = delta;
    }) == 0, "partial step does not update");
    expect(timestep.advance(0.05, [&](double delta) {
        ++updates;
        updateDelta = delta;
    }) == 1, "accumulated full step updates once");
    expect(updates == 1, "one fixed update was invoked");
    expect(std::abs(updateDelta - 0.1) < 1e-9, "callback receives fixed delta");
    expect(timestep.remainderSeconds() < 1e-9, "full step leaves no remainder");
}

void testCatchUpLimitAndFrameClamp() {
    glory::engine::FixedTimestep timestep(0.1, 0.5, 2);
    std::uint32_t updates = 0;

    expect(timestep.advance(0.5, [&](double) { ++updates; }) == 2,
           "catch-up work is capped at configured step count");
    expect(updates == 2, "only capped updates are invoked");
    expect(timestep.remainderSeconds() < 0.1, "overdue whole steps are discarded");

    updates = 0;
    expect(timestep.advance(2.0, [&](double) { ++updates; }) == 2,
           "large frame delta is clamped before catch-up");
    expect(updates == 2, "clamped frame cannot exceed catch-up cap");
}

void testInvalidDeltaAndConfiguration() {
    glory::engine::FixedTimestep timestep(0.1, 0.25, 8);
    std::uint32_t updates = 0;
    timestep.advance(-1.0, [&](double) { ++updates; });
    timestep.advance(std::nan(""), [&](double) { ++updates; });
    expect(updates == 0, "negative and non-finite frame deltas are ignored");

    bool rejectedConfiguration = false;
    try {
        glory::engine::FixedTimestep invalid(0.0, 0.25, 8);
    } catch (const std::invalid_argument&) {
        rejectedConfiguration = true;
    }
    expect(rejectedConfiguration, "zero fixed step is rejected");
}

} // namespace

int main() {
    testFixedUpdatesAndRemainder();
    testCatchUpLimitAndFrameClamp();
    testInvalidDeltaAndConfiguration();

    if (failures != 0) {
        std::cerr << failures << " fixed timestep test(s) failed\n";
        return 1;
    }

    std::cout << "All fixed timestep tests passed\n";
    return 0;
}

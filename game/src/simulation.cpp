#include "glory/game/simulation.hpp"

namespace glory::game {

void Simulation::fixedUpdate(double fixedDeltaSeconds) noexcept {
    ++tickCount_;
    elapsedSeconds_ += fixedDeltaSeconds;
}

std::uint64_t Simulation::tickCount() const noexcept {
    return tickCount_;
}

double Simulation::elapsedSeconds() const noexcept {
    return elapsedSeconds_;
}

} // namespace glory::game

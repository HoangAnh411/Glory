#pragma once

#include <cstdint>

namespace glory::game {

class Simulation {
public:
    void fixedUpdate(double fixedDeltaSeconds) noexcept;

    [[nodiscard]] std::uint64_t tickCount() const noexcept;
    [[nodiscard]] double elapsedSeconds() const noexcept;

private:
    std::uint64_t tickCount_ = 0;
    double elapsedSeconds_ = 0.0;
};

} // namespace glory::game

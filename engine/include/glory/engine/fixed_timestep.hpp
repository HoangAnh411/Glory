#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace glory::engine {

class FixedTimestep {
public:
    explicit FixedTimestep(
        double stepSeconds = 1.0 / 60.0,
        double maxFrameSeconds = 0.25,
        std::uint32_t maxCatchUpSteps = 8)
        : stepSeconds_(stepSeconds),
          maxFrameSeconds_(maxFrameSeconds),
          maxCatchUpSteps_(maxCatchUpSteps) {
        if (!std::isfinite(stepSeconds_) || stepSeconds_ <= 0.0 ||
            !std::isfinite(maxFrameSeconds_) || maxFrameSeconds_ <= 0.0 ||
            maxCatchUpSteps_ == 0) {
            throw std::invalid_argument("Fixed timestep settings must be positive and finite");
        }
    }

    template <typename UpdateFunction>
    std::uint32_t advance(double frameSeconds, UpdateFunction&& update) {
        if (!std::isfinite(frameSeconds) || frameSeconds < 0.0) {
            frameSeconds = 0.0;
        }
        if (frameSeconds > maxFrameSeconds_) {
            frameSeconds = maxFrameSeconds_;
        }

        accumulatorSeconds_ += frameSeconds;
        std::uint32_t steps = 0;
        while (accumulatorSeconds_ >= stepSeconds_ && steps < maxCatchUpSteps_) {
            update(stepSeconds_);
            accumulatorSeconds_ -= stepSeconds_;
            ++steps;
        }

        // Drop whole overdue steps while preserving the fractional remainder.
        if (steps == maxCatchUpSteps_ && accumulatorSeconds_ >= stepSeconds_) {
            accumulatorSeconds_ = std::fmod(accumulatorSeconds_, stepSeconds_);
        }

        return steps;
    }

    [[nodiscard]] double stepSeconds() const noexcept {
        return stepSeconds_;
    }

    [[nodiscard]] double remainderSeconds() const noexcept {
        return accumulatorSeconds_;
    }

private:
    double stepSeconds_;
    double maxFrameSeconds_;
    std::uint32_t maxCatchUpSteps_;
    double accumulatorSeconds_ = 0.0;
};

} // namespace glory::engine

#pragma once

#include "glory/game/combat.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <utility>
#include <vector>

namespace glory::game {

struct HistoricalEntityState {
    std::uint32_t entityId = 0;
    Hurtbox hurtbox;
    float currentHp = 0.0F;
    float maxHp = 0.0F;
};

class WorldHistoryBuffer {
public:
    explicit WorldHistoryBuffer(std::size_t capacity = 61);

    void record(std::uint64_t tick, std::vector<HistoricalEntityState> entities);
    [[nodiscard]] std::optional<HistoricalEntityState> sample(double tick,
        std::uint32_t entityId) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return frames_.size(); }
    [[nodiscard]] std::uint64_t oldestTick() const noexcept;
    [[nodiscard]] std::uint64_t newestTick() const noexcept;
    void clear() noexcept { frames_.clear(); }

private:
    struct Frame {
        std::uint64_t tick = 0;
        std::vector<HistoricalEntityState> entities;
    };

    std::deque<Frame> frames_;
    std::size_t capacity_ = 61;
};

struct RewoundHurtbox {
    HistoricalEntityState entity;
    double sampledTick = 0.0;
};

class LagCompensator {
public:
    static constexpr double ticksPerSecond = 60.0;
    static constexpr double maxCompensationSeconds = 0.35;
    static constexpr std::uint64_t maxRewindTicks = 21;

    explicit LagCompensator(std::size_t historyCapacity = 61) : history_(historyCapacity) {}

    void record(std::uint64_t tick, std::vector<HistoricalEntityState> entities) {
        history_.record(tick, std::move(entities));
    }
    [[nodiscard]] std::optional<RewoundHurtbox> rewindHurtbox(std::uint32_t entityId,
        std::uint64_t currentTick, double requestedTick) const noexcept;
    [[nodiscard]] const WorldHistoryBuffer& history() const noexcept { return history_; }

private:
    WorldHistoryBuffer history_;
};

} // namespace glory::game

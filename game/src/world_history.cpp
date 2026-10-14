#include "glory/game/world_history.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace glory::game {
namespace {

const HistoricalEntityState* findEntity(const std::vector<HistoricalEntityState>& entities,
    std::uint32_t entityId) noexcept {
    const auto found = std::find_if(entities.begin(), entities.end(), [entityId](const auto& entity) {
        return entity.entityId == entityId;
    });
    return found == entities.end() ? nullptr : &*found;
}

} // namespace

WorldHistoryBuffer::WorldHistoryBuffer(std::size_t capacity)
    : capacity_(std::max<std::size_t>(2, capacity)) {}

void WorldHistoryBuffer::record(std::uint64_t tick, std::vector<HistoricalEntityState> entities) {
    for (const HistoricalEntityState& entity : entities) {
        if (entity.entityId == 0 || !std::isfinite(entity.hurtbox.base.x) ||
            !std::isfinite(entity.hurtbox.base.y) || !std::isfinite(entity.hurtbox.base.z) ||
            !std::isfinite(entity.hurtbox.radius) || entity.hurtbox.radius <= 0.0F ||
            !std::isfinite(entity.hurtbox.height) || entity.hurtbox.height < entity.hurtbox.radius * 2.0F ||
            !std::isfinite(entity.currentHp) || !std::isfinite(entity.maxHp) ||
            entity.currentHp < 0.0F || entity.maxHp < 0.0F || entity.currentHp > entity.maxHp)
            throw std::invalid_argument("Invalid entity state in world history");
    }
    const auto position = std::lower_bound(frames_.begin(), frames_.end(), tick,
        [](const Frame& frame, std::uint64_t value) { return frame.tick < value; });
    if (position != frames_.end() && position->tick == tick) {
        *position = {tick, std::move(entities)};
    } else {
        frames_.insert(position, {tick, std::move(entities)});
    }
    while (frames_.size() > capacity_) frames_.pop_front();
}

std::optional<HistoricalEntityState> WorldHistoryBuffer::sample(double tick,
    std::uint32_t entityId) const noexcept {
    if (frames_.empty() || entityId == 0 || !std::isfinite(tick) ||
        tick < static_cast<double>(frames_.front().tick) ||
        tick > static_cast<double>(frames_.back().tick)) return std::nullopt;
    const auto upper = std::lower_bound(frames_.begin(), frames_.end(), tick,
        [](const Frame& frame, double value) { return static_cast<double>(frame.tick) < value; });
    if (upper == frames_.end()) return std::nullopt;
    const HistoricalEntityState* second = findEntity(upper->entities, entityId);
    if (!second) return std::nullopt;
    if (static_cast<double>(upper->tick) == tick || upper == frames_.begin()) return *second;

    const Frame& firstFrame = *(upper - 1);
    const HistoricalEntityState* first = findEntity(firstFrame.entities, entityId);
    if (!first) return *second;
    const double tickSpan = static_cast<double>(upper->tick - firstFrame.tick);
    const float alpha = tickSpan > 0.0 ? static_cast<float>(std::clamp(
        (tick - static_cast<double>(firstFrame.tick)) / tickSpan, 0.0, 1.0)) : 1.0F;
    HistoricalEntityState result = *second;
    result.hurtbox.base = first->hurtbox.base + (second->hurtbox.base - first->hurtbox.base) * alpha;
    result.hurtbox.radius = first->hurtbox.radius + (second->hurtbox.radius - first->hurtbox.radius) * alpha;
    result.hurtbox.height = first->hurtbox.height + (second->hurtbox.height - first->hurtbox.height) * alpha;
    result.currentHp = alpha < 1.0F ? first->currentHp : second->currentHp;
    result.maxHp = second->maxHp;
    return result;
}

std::uint64_t WorldHistoryBuffer::oldestTick() const noexcept {
    return frames_.empty() ? 0 : frames_.front().tick;
}

std::uint64_t WorldHistoryBuffer::newestTick() const noexcept {
    return frames_.empty() ? 0 : frames_.back().tick;
}

std::optional<RewoundHurtbox> LagCompensator::rewindHurtbox(std::uint32_t entityId,
    std::uint64_t currentTick, double requestedTick) const noexcept {
    if (!std::isfinite(requestedTick) || history_.size() == 0) return std::nullopt;
    const std::uint64_t lowerBound = currentTick > maxRewindTicks ? currentTick - maxRewindTicks : 0;
    const double boundedTick = std::clamp(requestedTick, static_cast<double>(lowerBound),
        static_cast<double>(currentTick));
    const double availableTick = std::max(boundedTick, static_cast<double>(history_.oldestTick()));
    const double sampleTick = std::min(availableTick, static_cast<double>(history_.newestTick()));
    if (sampleTick > static_cast<double>(currentTick)) return std::nullopt;
    const auto entity = history_.sample(sampleTick, entityId);
    if (!entity) return std::nullopt;
    return RewoundHurtbox{*entity, sampleTick};
}

} // namespace glory::game

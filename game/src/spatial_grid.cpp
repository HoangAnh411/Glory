#include "glory/game/spatial_grid.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace glory::game {

SpatialGrid::SpatialGrid(float cellSize) : cellSize_(cellSize) {
    if (!std::isfinite(cellSize_) || cellSize_ <= 0.0F || cellSize_ > 1'000'000.0F)
        throw std::invalid_argument("Spatial grid cell size must be finite and positive");
}

std::size_t SpatialGrid::CellKeyHash::operator()(const CellKey& key) const noexcept {
    const std::size_t zoneHash = std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(key.zone));
    const std::size_t xHash = std::hash<int>{}(key.x);
    const std::size_t zHash = std::hash<int>{}(key.z);
    return zoneHash ^ (xHash + 0x9e3779b9U + (zoneHash << 6U) + (zoneHash >> 2U)) ^
        (zHash + 0x9e3779b9U + (xHash << 6U) + (xHash >> 2U));
}

SpatialGrid::CellKey SpatialGrid::keyFor(ZoneId zone, const glm::vec3& position) const {
    return {zone, static_cast<int>(std::floor(position.x / cellSize_)),
        static_cast<int>(std::floor(position.z / cellSize_))};
}

SpatialCell SpatialGrid::cellFor(const glm::vec3& position) const {
    const auto key = keyFor(ZoneId::Zone1Plains, position);
    return {key.x, key.z};
}

void SpatialGrid::removeFromCell(std::uint64_t entityId, const CellKey& key) {
    const auto cell = cells_.find(key);
    if (cell == cells_.end()) return;
    auto& ids = cell->second;
    ids.erase(std::remove(ids.begin(), ids.end(), entityId), ids.end());
    if (ids.empty()) cells_.erase(cell);
}

bool SpatialGrid::upsert(const SpatialEntity& entity) {
    const float maxCoordinate = cellSize_ * static_cast<float>(std::numeric_limits<int>::max() / 2);
    if (entity.id == 0 || !isValidZone(entity.zone) ||
        !std::isfinite(entity.position.x) || !std::isfinite(entity.position.y) ||
        !std::isfinite(entity.position.z) || std::abs(entity.position.x) > maxCoordinate ||
        std::abs(entity.position.z) > maxCoordinate) return false;
    if (const auto found = entities_.find(entity.id); found != entities_.end())
        removeFromCell(entity.id, keyFor(found->second.zone, found->second.position));
    entities_[entity.id] = entity;
    cells_[keyFor(entity.zone, entity.position)].push_back(entity.id);
    return true;
}

bool SpatialGrid::remove(std::uint64_t entityId) {
    const auto found = entities_.find(entityId);
    if (found == entities_.end()) return false;
    removeFromCell(entityId, keyFor(found->second.zone, found->second.position));
    entities_.erase(found);
    return true;
}

std::vector<SpatialEntity> SpatialGrid::query(ZoneId zone, const glm::vec3& position, float radius) const {
    std::vector<SpatialEntity> result;
    const float maxCoordinate = cellSize_ * static_cast<float>(std::numeric_limits<int>::max() / 2);
    if (!isValidZone(zone) || !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z) || !std::isfinite(radius) || radius < 0.0F || radius > cellSize_ * 256.0F ||
        std::abs(position.x) + radius > maxCoordinate ||
        std::abs(position.z) + radius > maxCoordinate) return result;
    const int minX = static_cast<int>(std::floor((position.x - radius) / cellSize_));
    const int maxX = static_cast<int>(std::floor((position.x + radius) / cellSize_));
    const int minZ = static_cast<int>(std::floor((position.z - radius) / cellSize_));
    const int maxZ = static_cast<int>(std::floor((position.z + radius) / cellSize_));
    const float radiusSquared = radius * radius;
    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            const auto cell = cells_.find({zone, x, z});
            if (cell == cells_.end()) continue;
            for (const std::uint64_t id : cell->second) {
                const auto entity = entities_.find(id);
                if (entity != entities_.end()) {
                    const float dx = entity->second.position.x - position.x;
                    const float dz = entity->second.position.z - position.z;
                    if (dx * dx + dz * dz <= radiusSquared) result.push_back(entity->second);
                }
            }
        }
    }
    std::sort(result.begin(), result.end(), [](const SpatialEntity& left, const SpatialEntity& right) {
        return left.id < right.id;
    });
    return result;
}

const SpatialEntity* SpatialGrid::find(std::uint64_t entityId) const noexcept {
    const auto found = entities_.find(entityId);
    return found == entities_.end() ? nullptr : &found->second;
}

const char* zoneName(ZoneId zone) noexcept {
    switch (zone) {
    case ZoneId::Zone1Plains: return "Zone1_Plains";
    case ZoneId::Zone2Arena: return "Zone2_Arena";
    case ZoneId::Zone3Forest: return "Zone3_Forest";
    }
    return "Unknown";
}

bool isValidZone(ZoneId zone) noexcept {
    return static_cast<std::uint8_t>(zone) <= static_cast<std::uint8_t>(ZoneId::Zone3Forest);
}

} // namespace glory::game

#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace glory::game {

enum class ZoneId : std::uint8_t { Zone1Plains, Zone2Arena, Zone3Forest };
enum class SpatialEntityType : std::uint8_t { Player, Monster, Loot };

struct SpatialEntity {
    std::uint64_t id = 0;
    ZoneId zone = ZoneId::Zone1Plains;
    SpatialEntityType type = SpatialEntityType::Player;
    glm::vec3 position{0.0F};
};

struct SpatialCell {
    int x = 0;
    int z = 0;
};

class SpatialGrid {
public:
    explicit SpatialGrid(float cellSize = 32.0F);
    [[nodiscard]] bool upsert(const SpatialEntity& entity);
    [[nodiscard]] bool remove(std::uint64_t entityId);
    [[nodiscard]] std::vector<SpatialEntity> query(ZoneId zone, const glm::vec3& position,
        float radius) const;
    [[nodiscard]] SpatialCell cellFor(const glm::vec3& position) const;
    [[nodiscard]] std::size_t entityCount() const noexcept { return entities_.size(); }
    [[nodiscard]] const SpatialEntity* find(std::uint64_t entityId) const noexcept;

private:
    struct CellKey {
        ZoneId zone;
        int x;
        int z;
        bool operator==(const CellKey&) const noexcept = default;
    };
    struct CellKeyHash {
        [[nodiscard]] std::size_t operator()(const CellKey& key) const noexcept;
    };
    [[nodiscard]] CellKey keyFor(ZoneId zone, const glm::vec3& position) const;
    void removeFromCell(std::uint64_t entityId, const CellKey& key);

    float cellSize_;
    std::unordered_map<CellKey, std::vector<std::uint64_t>, CellKeyHash> cells_;
    std::unordered_map<std::uint64_t, SpatialEntity> entities_;
};

[[nodiscard]] const char* zoneName(ZoneId zone) noexcept;
[[nodiscard]] bool isValidZone(ZoneId zone) noexcept;

} // namespace glory::game

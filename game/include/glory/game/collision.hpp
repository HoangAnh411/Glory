#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include <vector>

namespace glory::game {

struct Capsule {
    glm::vec3 base{0.0F};
    float radius = 0.4F;
    float height = 1.8F;
};

struct Aabb {
    glm::vec3 minimum{0.0F};
    glm::vec3 maximum{0.0F};
    bool step = false;
};

struct Ramp {
    float minimumX = 0.0F;
    float maximumX = 1.0F;
    float minimumZ = -1.0F;
    float maximumZ = 1.0F;
    float startHeight = 0.0F;
    float endHeight = 0.5F;

    [[nodiscard]] float heightAt(float x) const noexcept;
    [[nodiscard]] glm::vec3 normal() const noexcept;
};

struct CollisionResult {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    bool isGrounded = false;
    unsigned int contactCount = 0;
};

class CollisionWorld {
public:
    void addObstacle(const Aabb& obstacle);
    void addStep(Aabb step);
    void addRamp(const Ramp& ramp);

    [[nodiscard]] const std::vector<Aabb>& obstacles() const noexcept { return obstacles_; }
    [[nodiscard]] const std::vector<Ramp>& ramps() const noexcept { return ramps_; }
    [[nodiscard]] float arenaRadius() const noexcept { return arenaRadius_; }
    [[nodiscard]] const glm::vec2& arenaCenter() const noexcept { return arenaCenter_; }
    void setArenaBoundary(glm::vec2 center, float radius);
    [[nodiscard]] static CollisionWorld loadFromFiles(const std::filesystem::path& worldMapPath,
        const std::filesystem::path& colliderDataPath);
    [[nodiscard]] CollisionResult resolve(const Capsule& capsule, const glm::vec3& desiredPosition,
        const glm::vec3& velocity, float maximumStepHeight = 0.25F) const noexcept;

    [[nodiscard]] static CollisionWorld makeSampleWorld();

private:
    std::vector<Aabb> obstacles_;
    std::vector<Ramp> ramps_;
    glm::vec2 arenaCenter_{0.0F};
    float arenaRadius_ = 0.0F;
};

} // namespace glory::game

#include "glory/game/collision.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace glory::game {
namespace {

constexpr float walkableNormalY = 0.70710678F;
constexpr float contactEpsilon = 1.0e-4F;

bool finite(const glm::vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool containsXZ(const Ramp& ramp, float x, float z, float radius = 0.0F) noexcept {
    return x + radius >= ramp.minimumX && x - radius <= ramp.maximumX &&
        z + radius >= ramp.minimumZ && z - radius <= ramp.maximumZ;
}

bool isWalkable(const Ramp& ramp) noexcept {
    const float width = ramp.maximumX - ramp.minimumX;
    return width > 0.0F && std::abs(ramp.endHeight - ramp.startHeight) < width &&
        ramp.normal().y > walkableNormalY;
}

bool overlapsVertically(const Capsule& capsule, const glm::vec3& base, const Aabb& box) noexcept {
    return base.y < box.maximum.y - contactEpsilon &&
        base.y + capsule.height > box.minimum.y + contactEpsilon;
}

bool overlapsCircleXZ(const glm::vec3& position, float radius, const Aabb& box) noexcept {
    const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
    const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
    const float dx = position.x - closestX;
    const float dz = position.z - closestZ;
    return dx * dx + dz * dz < radius * radius;
}

glm::vec2 separationNormal(const glm::vec3& position, const Aabb& box, float radius,
    glm::vec3& resolved) noexcept {
    const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
    const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
    glm::vec2 separation{position.x - closestX, position.z - closestZ};
    float distance = glm::length(separation);
    if (distance > contactEpsilon) {
        const glm::vec2 normal = separation / distance;
        resolved.x += normal.x * (radius - distance);
        resolved.z += normal.y * (radius - distance);
        return normal;
    }

    const float left = position.x - box.minimum.x;
    const float right = box.maximum.x - position.x;
    const float back = position.z - box.minimum.z;
    const float front = box.maximum.z - position.z;
    const float nearest = std::min({left, right, back, front});
    glm::vec2 normal{0.0F};
    float penetration = 0.0F;
    if (nearest == left) { normal = {-1.0F, 0.0F}; penetration = radius + left; }
    else if (nearest == right) { normal = {1.0F, 0.0F}; penetration = radius + right; }
    else if (nearest == back) { normal = {0.0F, -1.0F}; penetration = radius + back; }
    else { normal = {0.0F, 1.0F}; penetration = radius + front; }
    resolved.x += normal.x * penetration;
    resolved.z += normal.y * penetration;
    return normal;
}

} // namespace

float Ramp::heightAt(float x) const noexcept {
    const float width = maximumX - minimumX;
    if (width <= 0.0F) return startHeight;
    const float alpha = std::clamp((x - minimumX) / width, 0.0F, 1.0F);
    return startHeight + (endHeight - startHeight) * alpha;
}

glm::vec3 Ramp::normal() const noexcept {
    const float width = maximumX - minimumX;
    if (width <= 0.0F) return {0.0F, 1.0F, 0.0F};
    return glm::normalize(glm::vec3{-(endHeight - startHeight) / width, 1.0F, 0.0F});
}

void CollisionWorld::addObstacle(const Aabb& obstacle) {
    if (!finite(obstacle.minimum) || !finite(obstacle.maximum) ||
        obstacle.minimum.x >= obstacle.maximum.x || obstacle.minimum.y >= obstacle.maximum.y ||
        obstacle.minimum.z >= obstacle.maximum.z) {
        throw std::invalid_argument("Collision obstacle bounds must be finite and ordered");
    }
    obstacles_.push_back(obstacle);
}

void CollisionWorld::addStep(Aabb step) {
    if (!finite(step.minimum) || !finite(step.maximum) ||
        step.minimum.x >= step.maximum.x || step.minimum.y >= step.maximum.y ||
        step.minimum.z >= step.maximum.z) {
        throw std::invalid_argument("Collision step bounds must be finite and ordered");
    }
    step.step = true;
    obstacles_.push_back(step);
}

void CollisionWorld::addRamp(const Ramp& ramp) {
    if (!std::isfinite(ramp.minimumX) || !std::isfinite(ramp.maximumX) ||
        !std::isfinite(ramp.minimumZ) || !std::isfinite(ramp.maximumZ) ||
        !std::isfinite(ramp.startHeight) || !std::isfinite(ramp.endHeight) ||
        ramp.minimumX >= ramp.maximumX || ramp.minimumZ >= ramp.maximumZ) {
        throw std::invalid_argument("Collision ramp bounds must be finite and ordered");
    }
    ramps_.push_back(ramp);
}

void CollisionWorld::setArenaBoundary(glm::vec2 center, float radius) {
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(radius) || radius <= 0.0F)
        throw std::invalid_argument("Arena boundary must have finite center and positive radius");
    arenaCenter_ = center;
    arenaRadius_ = radius;
}

CollisionWorld CollisionWorld::loadFromFiles(const std::filesystem::path& worldMapPath,
    const std::filesystem::path& colliderDataPath) {
    using Json = nlohmann::json;
    std::ifstream colliderFile(colliderDataPath);
    if (!colliderFile) throw std::runtime_error("Unable to open world collider data: " + colliderDataPath.string());
    Json colliderData;
    colliderFile >> colliderData;
    CollisionWorld world;
    const auto& arena = colliderData.at("arena");
    world.setArenaBoundary({arena.at("center").at(0).get<float>(), arena.at("center").at(1).get<float>()},
        arena.at("radius").get<float>());

    std::ifstream worldFile(worldMapPath);
    if (!worldFile) throw std::runtime_error("Unable to open world map: " + worldMapPath.string());
    Json worldMap;
    worldFile >> worldMap;
    const float pi = std::acos(-1.0F);
    for (const auto& prop : worldMap.value("props", Json::array())) {
        const auto position = prop.value("position", Json::array());
        if (!position.is_array() || position.size() < 3) continue;
        const glm::vec3 origin{position[0].get<float>(), position[1].get<float>(), position[2].get<float>()};
        const float yaw = prop.value("yaw", 0.0F) * pi / 180.0F;
        const float scale = std::abs(prop.value("scale", 1.0F));
        const auto addLocalBounds = [&](const Json& bounds) {
            if (!bounds.is_array() || bounds.size() < 6) return;
            const glm::vec3 localMin{bounds[0].get<float>(), bounds[1].get<float>(), bounds[2].get<float>()};
            const glm::vec3 localMax{bounds[3].get<float>(), bounds[4].get<float>(), bounds[5].get<float>()};
            float minX = std::numeric_limits<float>::max();
            float minZ = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float maxZ = std::numeric_limits<float>::lowest();
            for (int corner = 0; corner < 4; ++corner) {
                const float x = ((corner & 1) ? localMax.x : localMin.x) * scale;
                const float z = ((corner & 2) ? localMax.z : localMin.z) * scale;
                const float worldX = origin.x + x * std::cos(yaw) + z * std::sin(yaw);
                const float worldZ = origin.z - x * std::sin(yaw) + z * std::cos(yaw);
                minX = std::min(minX, worldX); maxX = std::max(maxX, worldX);
                minZ = std::min(minZ, worldZ); maxZ = std::max(maxZ, worldZ);
            }
            world.addObstacle({{minX, origin.y + localMin.y * scale, minZ},
                {maxX, origin.y + localMax.y * scale, maxZ}, false});
        };
        if (prop.contains("collision")) addLocalBounds(prop["collision"]);
        if (prop.contains("multi_collision") && prop["multi_collision"].is_array())
            for (const auto& bounds : prop["multi_collision"]) addLocalBounds(bounds);
    }
    for (const auto& item : colliderData.value("obstacles", Json::array())) {
        const auto& minimum = item.at("minimum");
        const auto& maximum = item.at("maximum");
        world.addObstacle({{minimum[0].get<float>(), minimum[1].get<float>(), minimum[2].get<float>()},
            {maximum[0].get<float>(), maximum[1].get<float>(), maximum[2].get<float>()},
            item.value("step", false)});
    }
    return world;
}

CollisionResult CollisionWorld::resolve(const Capsule& capsule, const glm::vec3& desiredPosition,
    const glm::vec3& velocity, float maximumStepHeight) const noexcept {
    CollisionResult result{capsule.base, velocity, false, 0};
    if (!finite(capsule.base) || !finite(desiredPosition) || !finite(velocity) ||
        !std::isfinite(capsule.radius) || capsule.radius <= 0.0F ||
        !std::isfinite(capsule.height) || capsule.height < capsule.radius * 2.0F ||
        !std::isfinite(maximumStepHeight) || maximumStepHeight < 0.0F) return result;

    const glm::vec3 totalDelta = desiredPosition - capsule.base;
    const float movementLength = glm::length(totalDelta);
    const float substepDistance = std::max(capsule.radius * 0.5F, 0.05F);
    const float requestedSubsteps = std::ceil(movementLength / substepDistance);
    const int substepCount = static_cast<int>(std::clamp(requestedSubsteps, 1.0F, 256.0F));
    const glm::vec3 substep = totalDelta / static_cast<float>(substepCount);
    result.position = capsule.base;

    for (int stepIndex = 0; stepIndex < substepCount; ++stepIndex) {
        const glm::vec3 previous = result.position;
        glm::vec3 candidate = previous + substep;
        bool supportedOnObstacle = false;

        for (const Ramp& ramp : ramps_) {
            if (!containsXZ(ramp, candidate.x, candidate.z, capsule.radius)) continue;
            const glm::vec3 rampNormal = ramp.normal();
            if (!isWalkable(ramp)) {
                const glm::vec2 horizontalNormal = glm::normalize(glm::vec2{rampNormal.x, rampNormal.z});
                const float intoRamp = substep.x * horizontalNormal.x + substep.z * horizontalNormal.y;
                if (intoRamp < 0.0F) {
                    candidate.x = previous.x;
                    candidate.z = previous.z;
                    result.velocity.x -= intoRamp * horizontalNormal.x;
                    result.velocity.z -= intoRamp * horizontalNormal.y;
                    ++result.contactCount;
                }
                continue;
            }
            const float rampHeight = ramp.heightAt(candidate.x);
            if (result.velocity.y <= 0.0F && candidate.y <= rampHeight + maximumStepHeight) {
                candidate.y = rampHeight;
                result.velocity.y = 0.0F;
                result.isGrounded = true;
            }
        }

        for (const Aabb& obstacle : obstacles_) {
            if (!overlapsCircleXZ(candidate, capsule.radius, obstacle)) continue;

            if (std::abs(candidate.y - obstacle.maximum.y) <= contactEpsilon) {
                supportedOnObstacle = true;
                continue;
            }
            if (result.velocity.y < 0.0F && previous.y >= obstacle.maximum.y - contactEpsilon &&
                candidate.y < obstacle.maximum.y && candidate.y + capsule.height > obstacle.minimum.y) {
                candidate.y = obstacle.maximum.y;
                result.velocity.y = 0.0F;
                supportedOnObstacle = true;
                ++result.contactCount;
                continue;
            }
            if (result.velocity.y > 0.0F && previous.y + capsule.height <= obstacle.minimum.y + contactEpsilon &&
                candidate.y + capsule.height > obstacle.minimum.y) {
                candidate.y = obstacle.minimum.y - capsule.height;
                result.velocity.y = 0.0F;
                ++result.contactCount;
                continue;
            }
            if (!overlapsVertically(capsule, candidate, obstacle)) continue;

            if (obstacle.step && obstacle.maximum.y - previous.y <= maximumStepHeight + contactEpsilon &&
                obstacle.maximum.y >= previous.y - contactEpsilon) {
                glm::vec3 raised = candidate;
                raised.y = obstacle.maximum.y;
                if (!overlapsVertically(capsule, raised, obstacle)) {
                    candidate = raised;
                    result.velocity.y = 0.0F;
                    supportedOnObstacle = true;
                    ++result.contactCount;
                    continue;
                }
            }

            glm::vec3 separated = candidate;
            const glm::vec2 normal = separationNormal(candidate, obstacle, capsule.radius, separated);
            candidate = separated;
            const float intoWall = result.velocity.x * normal.x + result.velocity.z * normal.y;
            if (intoWall < 0.0F) {
                result.velocity.x -= intoWall * normal.x;
                result.velocity.z -= intoWall * normal.y;
            }
            ++result.contactCount;
        }

        if (candidate.y <= 0.0F) {
            candidate.y = 0.0F;
            if (result.velocity.y < 0.0F) result.velocity.y = 0.0F;
            result.isGrounded = true;
        } else {
            result.isGrounded = supportedOnObstacle;
        }

        for (const Ramp& ramp : ramps_) {
            if (!containsXZ(ramp, candidate.x, candidate.z, capsule.radius) || !isWalkable(ramp)) continue;
            const float rampHeight = ramp.heightAt(candidate.x);
            if (candidate.y <= rampHeight + contactEpsilon && result.velocity.y <= 0.0F) {
                candidate.y = rampHeight;
                result.velocity.y = 0.0F;
                result.isGrounded = true;
            }
        }

        result.position = candidate;
    }

    return result;
}

CollisionWorld CollisionWorld::makeSampleWorld() {
    CollisionWorld world;
    world.addObstacle({{20.0F, 0.0F, -1.5F}, {20.5F, 2.0F, 1.5F}, false});
    world.addRamp({24.0F, 28.0F, -2.0F, 2.0F, 0.0F, 1.5F});
    world.addRamp({30.0F, 32.0F, -2.0F, 2.0F, 0.0F, 2.0F});
    world.addStep({{34.0F, 0.0F, -1.0F}, {34.6F, 0.2F, 1.0F}, false});
    return world;
}

} // namespace glory::game

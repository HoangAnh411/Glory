#include "glory/engine/shadow_math.hpp"

#include <glm/geometric.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace glory::engine {
namespace {

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

glm::mat4 calculateLightSpaceMatrix(
    const glm::vec3& lightDirection,
    const glm::vec3& sceneCenter,
    const glm::vec2& orthographicHalfExtent,
    float nearPlane,
    float farPlane) {
    if (!isFinite(lightDirection) || glm::length(lightDirection) <= 1.0e-6F || !isFinite(sceneCenter) ||
        !std::isfinite(orthographicHalfExtent.x) || !std::isfinite(orthographicHalfExtent.y) ||
        orthographicHalfExtent.x <= 0.0F || orthographicHalfExtent.y <= 0.0F ||
        !std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane <= 0.0F || farPlane <= nearPlane) {
        throw std::invalid_argument("Invalid directional shadow camera parameters");
    }

    const glm::vec3 direction = glm::normalize(lightDirection);
    glm::vec3 up{0.0F, 1.0F, 0.0F};
    if (std::abs(glm::dot(direction, up)) > 0.98F) up = {0.0F, 0.0F, 1.0F};

    const float lightDistance = (nearPlane + farPlane) * 0.5F;
    const glm::mat4 lightView = glm::lookAt(sceneCenter - direction * lightDistance, sceneCenter, up);
    const glm::mat4 lightProjection = glm::ortho(
        -orthographicHalfExtent.x, orthographicHalfExtent.x,
        -orthographicHalfExtent.y, orthographicHalfExtent.y,
        nearPlane, farPlane);
    return lightProjection * lightView;
}

std::vector<float> calculateCascadeSplits(float nearPlane, float farPlane, int cascadeCount, float lambda) {
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) || !std::isfinite(lambda) ||
        nearPlane <= 0.0F || farPlane <= nearPlane || cascadeCount < 1 || cascadeCount > 4)
        throw std::invalid_argument("Invalid cascade split parameters");
    lambda = std::clamp(lambda, 0.0F, 1.0F);
    std::vector<float> splits;
    splits.reserve(static_cast<std::size_t>(cascadeCount));
    for (int index = 1; index <= cascadeCount; ++index) {
        const float fraction = static_cast<float>(index) / static_cast<float>(cascadeCount);
        const float logarithmic = nearPlane * std::pow(farPlane / nearPlane, fraction);
        const float uniform = nearPlane + (farPlane - nearPlane) * fraction;
        splits.push_back(lambda * logarithmic + (1.0F - lambda) * uniform);
    }
    splits.back() = farPlane;
    return splits;
}

glm::vec2 snapShadowCenterToTexel(const glm::vec2& center, float worldUnitsPerTexel) {
    if (!std::isfinite(center.x) || !std::isfinite(center.y) ||
        !std::isfinite(worldUnitsPerTexel) || worldUnitsPerTexel <= 0.0F)
        throw std::invalid_argument("Invalid shadow texel snapping parameters");
    return glm::round(center / worldUnitsPerTexel) * worldUnitsPerTexel;
}

float calculateHeightFogFactor(float distance, float worldHeight, float density) noexcept {
    if (!std::isfinite(distance) || !std::isfinite(worldHeight) || !std::isfinite(density) ||
        distance <= 0.0F || density <= 0.0F) return 0.0F;
    const float heightDensity = std::exp(-std::max(worldHeight, 0.0F) * 0.045F);
    const float opticalDepth = std::min(density * distance * heightDensity, 20.0F);
    return std::clamp(1.0F - std::exp(-(opticalDepth * opticalDepth)), 0.0F, 1.0F);
}

} // namespace glory::engine

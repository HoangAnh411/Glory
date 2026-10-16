#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <array>

namespace glory::engine {

[[nodiscard]] glm::mat4 calculateLightSpaceMatrix(
    const glm::vec3& lightDirection,
    const glm::vec3& sceneCenter,
    const glm::vec2& orthographicHalfExtent,
    float nearPlane,
    float farPlane);

[[nodiscard]] std::vector<float> calculateCascadeSplits(
    float nearPlane, float farPlane, int cascadeCount, float lambda);
[[nodiscard]] glm::vec2 snapShadowCenterToTexel(
    const glm::vec2& center, float worldUnitsPerTexel);
[[nodiscard]] float calculateHeightFogFactor(float distance, float worldHeight, float density) noexcept;
[[nodiscard]] std::array<glm::mat4, 4> calculateCascadeLightSpaceMatrices(
    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDirection,
    const std::vector<float>& splits, float cameraNear, int mapResolution);
[[nodiscard]] bool sphereIntersectsClip(const glm::mat4& viewProjection,
    const glm::vec3& center, float radius) noexcept;

} // namespace glory::engine

#include "glory/engine/shadow_math.hpp"

#include <glm/geometric.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

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

std::array<glm::mat4, 4> calculateCascadeLightSpaceMatrices(
    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightDirection,
    const std::vector<float>& splits, float cameraNear, int mapResolution) {
    if (splits.empty() || splits.size() > 4 || !isFinite(lightDirection) ||
        glm::length(lightDirection) < 1.0e-6F || !std::isfinite(cameraNear) || cameraNear <= 0.0F ||
        mapResolution <= 0) throw std::invalid_argument("Invalid cascade matrix inputs");
    const glm::mat4 inverseProjection = glm::inverse(projection);
    const glm::mat4 inverseView = glm::inverse(view);
    const float projectionDenominator = projection[2][2] + 1.0F;
    const float cameraFar = std::abs(projectionDenominator) > 1.0e-6F
        ? projection[3][2] / projectionDenominator : splits.back();
    const float safeCameraFar = std::isfinite(cameraFar) && cameraFar > cameraNear ? cameraFar : splits.back();
    std::array<glm::vec3, 4> nearCorners{};
    std::array<glm::vec3, 4> farCorners{};
    std::size_t index = 0;
    for (int y : {-1, 1}) for (int x : {-1, 1}) {
        glm::vec4 nearPoint = inverseProjection * glm::vec4(static_cast<float>(x), static_cast<float>(y), -1.0F, 1.0F);
        glm::vec4 farPoint = inverseProjection * glm::vec4(static_cast<float>(x), static_cast<float>(y), 1.0F, 1.0F);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        glm::vec4 worldNear = inverseView * nearPoint;
        glm::vec4 worldFar = inverseView * farPoint;
        nearCorners[index] = glm::vec3(worldNear) / worldNear.w;
        farCorners[index] = glm::vec3(worldFar) / worldFar.w;
        ++index;
    }
    std::array<glm::mat4, 4> result{};
    float previousSplit = cameraNear;
    const glm::vec3 direction = glm::normalize(lightDirection);
    for (std::size_t cascade = 0; cascade < splits.size(); ++cascade) {
        const float split = splits[cascade];
        if (!std::isfinite(split) || split <= previousSplit)
            throw std::invalid_argument("Cascade splits must be finite and increasing");
        const float denominator = std::max(1.0e-6F, safeCameraFar - cameraNear);
        const float nearFraction = std::clamp((previousSplit - cameraNear) / denominator, 0.0F, 1.0F);
        const float farFraction = std::clamp((split - cameraNear) / denominator, 0.0F, 1.0F);
        std::array<glm::vec3, 8> corners{};
        glm::vec3 center{0.0F};
        index = 0;
        for (std::size_t corner = 0; corner < 4; ++corner) {
            const glm::vec3 ray = farCorners[corner] - nearCorners[corner];
            corners[index++] = nearCorners[corner] + ray * nearFraction;
            corners[index++] = nearCorners[corner] + ray * farFraction;
        }
        for (const glm::vec3& corner : corners) center += corner / 8.0F;
        float radius = 0.0F;
        for (const glm::vec3& corner : corners) radius = std::max(radius, glm::length(corner - center));
        radius = std::ceil(radius * 16.0F) / 16.0F;
        glm::vec3 up{0.0F, 1.0F, 0.0F};
        if (std::abs(glm::dot(direction, up)) > 0.98F) up = {0.0F, 0.0F, 1.0F};
        const glm::mat4 lightView = glm::lookAt(center - direction * (radius * 2.0F), center, up);
        glm::mat4 lightProjection = glm::ortho(-radius, radius, -radius, radius, 0.1F, radius * 4.0F);
        const glm::vec4 clipCenter = lightProjection * lightView * glm::vec4(center, 1.0F);
        const glm::vec2 texelNdc = glm::vec2(2.0F / static_cast<float>(mapResolution));
        const glm::vec2 snapped = glm::round(glm::vec2(clipCenter) / clipCenter.w / texelNdc) * texelNdc;
        lightProjection[3][0] += snapped.x - clipCenter.x / clipCenter.w;
        lightProjection[3][1] += snapped.y - clipCenter.y / clipCenter.w;
        result[cascade] = lightProjection * lightView;
        previousSplit = split;
    }
    for (std::size_t cascade = splits.size(); cascade < result.size(); ++cascade)
        result[cascade] = result[splits.size() - 1];
    return result;
}

bool sphereIntersectsClip(const glm::mat4& viewProjection, const glm::vec3& center, float radius) noexcept {
    if (!isFinite(center) || !std::isfinite(radius) || radius < 0.0F) return false;
    const glm::vec4 clip = viewProjection * glm::vec4(center, 1.0F);
    if (!std::isfinite(clip.w) || std::abs(clip.w) < 1.0e-6F) return false;
    const float scale = radius * 2.0F / std::abs(clip.w);
    return clip.x >= -clip.w - scale && clip.x <= clip.w + scale &&
        clip.y >= -clip.w - scale && clip.y <= clip.w + scale &&
        clip.z >= -clip.w - scale && clip.z <= clip.w + scale;
}

} // namespace glory::engine

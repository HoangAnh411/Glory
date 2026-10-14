#include "glory/engine/shadow_math.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <stdexcept>

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

} // namespace glory::engine

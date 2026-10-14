#pragma once

#include <glm/vec3.hpp>

namespace glory::engine {

struct DirectionalLight {
    glm::vec3 direction{0.5F, -0.70710678F, -0.5F};
    glm::vec3 color{1.0F, 0.93F, 0.82F};
    float ambientStrength = 0.18F;
    float diffuseStrength = 0.85F;
    float specularStrength = 0.5F;
    float shininess = 32.0F;
};

[[nodiscard]] glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees);
void setDirectionFromAngles(DirectionalLight& light, float yawDegrees, float pitchDegrees);

} // namespace glory::engine

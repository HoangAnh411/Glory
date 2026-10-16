#pragma once

#include <glm/vec3.hpp>

namespace glory::engine {

struct DirectionalLight {
    glm::vec3 direction{-0.612F, -0.5F, -0.612F};
    glm::vec3 color{1.0F, 0.68F, 0.42F};
    float ambientStrength = 0.72F;
    float diffuseStrength = 1.35F;
    float specularStrength = 0.65F;
    float shininess = 32.0F;
};

[[nodiscard]] glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees);
void setDirectionFromAngles(DirectionalLight& light, float yawDegrees, float pitchDegrees);

} // namespace glory::engine

#pragma once

#include <glm/vec3.hpp>

namespace glory::engine {

struct DirectionalLight {
    glm::vec3 direction{-0.45F, -0.72F, -0.52F};
    glm::vec3 color{1.0F, 0.95F, 0.88F};
    float ambientStrength = 0.82F;
    float diffuseStrength = 0.9F;
    float specularStrength = 0.65F;
    float shininess = 32.0F;
};

[[nodiscard]] glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees);
void setDirectionFromAngles(DirectionalLight& light, float yawDegrees, float pitchDegrees);

} // namespace glory::engine

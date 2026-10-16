#pragma once

#include <glm/vec3.hpp>

namespace glory::engine {

struct DirectionalLight {
    glm::vec3 direction{0.25F, -0.85F, 0.45F};
    glm::vec3 color{1.0F, 0.96F, 0.90F};
    float ambientStrength = 0.55F;
    float diffuseStrength = 0.95F;
    float specularStrength = 0.65F;
    float shininess = 32.0F;
};

[[nodiscard]] glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees);
void setDirectionFromAngles(DirectionalLight& light, float yawDegrees, float pitchDegrees);

} // namespace glory::engine

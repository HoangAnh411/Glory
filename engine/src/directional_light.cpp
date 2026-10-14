#include "glory/engine/directional_light.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>
#include <stdexcept>

namespace glory::engine {

glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees) {
    if (!std::isfinite(yawDegrees) || !std::isfinite(pitchDegrees) ||
        pitchDegrees < -89.0F || pitchDegrees > 89.0F) {
        throw std::invalid_argument("Light yaw/pitch angles must be finite and pitch must be within [-89, 89]");
    }
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
    return glm::normalize(glm::vec3{
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw),
    });
}

void setDirectionFromAngles(DirectionalLight& light, float yawDegrees, float pitchDegrees) {
    light.direction = directionFromAngles(yawDegrees, pitchDegrees);
}

} // namespace glory::engine

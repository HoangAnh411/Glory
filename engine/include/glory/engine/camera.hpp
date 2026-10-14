#pragma once

#include "glory/input/input_frame.hpp"

#include <glm/glm.hpp>

namespace glory::engine {

struct CameraConfig {
    glm::vec3 position{0.0F, 0.0F, 3.0F};
    float yawDegrees = -90.0F;
    float pitchDegrees = 0.0F;
    float fieldOfViewDegrees = 45.0F;
    float aspectRatio = 16.0F / 9.0F;
    float nearPlane = 0.1F;
    float farPlane = 100.0F;
    float movementSpeed = 4.0F;
    float mouseSensitivity = 0.1F;
};

class FreeCamera {
public:
    explicit FreeCamera(const CameraConfig& config = {});

    void update(const input::InputFrame& inputFrame, double fixedDeltaSeconds) noexcept;
    void setAspectRatio(float aspectRatio);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix() const;
    [[nodiscard]] glm::mat4 viewProjectionMatrix() const;
    [[nodiscard]] glm::vec3 position() const noexcept;
    [[nodiscard]] float yawDegrees() const noexcept;
    [[nodiscard]] float pitchDegrees() const noexcept;
    [[nodiscard]] float aspectRatio() const noexcept;

private:
    [[nodiscard]] glm::vec3 frontDirection() const noexcept;

    glm::vec3 position_;
    float yawDegrees_;
    float pitchDegrees_;
    float fieldOfViewDegrees_;
    float aspectRatio_;
    float nearPlane_;
    float farPlane_;
    float movementSpeed_;
    float mouseSensitivity_;
};

} // namespace glory::engine

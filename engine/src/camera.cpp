#include "glory/engine/camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace glory::engine {
namespace {

constexpr float minimumPitchDegrees = -89.0F;
constexpr float maximumPitchDegrees = 89.0F;
constexpr glm::vec3 worldUp{0.0F, 1.0F, 0.0F};

bool contains(const std::vector<input::Control>& controls, input::Control control) noexcept {
    return std::find(controls.begin(), controls.end(), control) != controls.end();
}

bool isFinite(const glm::vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

FreeCamera::FreeCamera(const CameraConfig& config)
    : position_(config.position),
      yawDegrees_(config.yawDegrees),
      pitchDegrees_(std::clamp(config.pitchDegrees, minimumPitchDegrees, maximumPitchDegrees)),
      fieldOfViewDegrees_(config.fieldOfViewDegrees),
      aspectRatio_(config.aspectRatio),
      nearPlane_(config.nearPlane),
      farPlane_(config.farPlane),
      movementSpeed_(config.movementSpeed),
      mouseSensitivity_(config.mouseSensitivity) {
    if (!isFinite(position_) || !std::isfinite(yawDegrees_) ||
        !std::isfinite(config.pitchDegrees) || !std::isfinite(fieldOfViewDegrees_) ||
        fieldOfViewDegrees_ <= 0.0F || fieldOfViewDegrees_ >= 180.0F ||
        !std::isfinite(aspectRatio_) || aspectRatio_ <= 0.0F ||
        !std::isfinite(nearPlane_) || !std::isfinite(farPlane_) ||
        nearPlane_ <= 0.0F || farPlane_ <= nearPlane_ ||
        !std::isfinite(movementSpeed_) || movementSpeed_ < 0.0F ||
        !std::isfinite(mouseSensitivity_) || mouseSensitivity_ < 0.0F) {
        throw std::invalid_argument("Invalid free camera configuration");
    }
    yawDegrees_ = std::remainder(yawDegrees_, 360.0F);
}

void FreeCamera::update(const input::InputFrame& inputFrame, double fixedDeltaSeconds) noexcept {
    if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0) {
        return;
    }

    if (std::isfinite(inputFrame.mouseDeltaX) && std::isfinite(inputFrame.mouseDeltaY)) {
        yawDegrees_ = std::remainder(
            yawDegrees_ + inputFrame.mouseDeltaX * mouseSensitivity_,
            360.0F);
        pitchDegrees_ = std::clamp(
            pitchDegrees_ - inputFrame.mouseDeltaY * mouseSensitivity_,
            minimumPitchDegrees,
            maximumPitchDegrees);
    }

    const glm::vec3 front = frontDirection();
    const glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 movement{0.0F};
    if (contains(inputFrame.held, input::Control::KeyW)) movement += front;
    if (contains(inputFrame.held, input::Control::KeyS)) movement -= front;
    if (contains(inputFrame.held, input::Control::KeyD)) movement += right;
    if (contains(inputFrame.held, input::Control::KeyA)) movement -= right;
    if (contains(inputFrame.held, input::Control::KeySpace)) movement += worldUp;
    if (contains(inputFrame.held, input::Control::KeyLeftControl) ||
        contains(inputFrame.held, input::Control::KeyRightControl)) {
        movement -= worldUp;
    }

    const float movementLength = glm::length(movement);
    if (movementLength > 0.0F) {
        position_ += (movement / movementLength) * movementSpeed_ *
            static_cast<float>(fixedDeltaSeconds);
    }
}

void FreeCamera::setAspectRatio(float aspectRatio) {
    if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0F) {
        throw std::invalid_argument("Camera aspect ratio must be positive and finite");
    }
    aspectRatio_ = aspectRatio;
}

glm::mat4 FreeCamera::viewMatrix() const {
    return glm::lookAt(position_, position_ + frontDirection(), worldUp);
}

glm::mat4 FreeCamera::projectionMatrix() const {
    return glm::perspective(
        glm::radians(fieldOfViewDegrees_), aspectRatio_, nearPlane_, farPlane_);
}

glm::mat4 FreeCamera::viewProjectionMatrix() const {
    return projectionMatrix() * viewMatrix();
}

glm::vec3 FreeCamera::position() const noexcept {
    return position_;
}

float FreeCamera::yawDegrees() const noexcept {
    return yawDegrees_;
}

float FreeCamera::pitchDegrees() const noexcept {
    return pitchDegrees_;
}

float FreeCamera::aspectRatio() const noexcept {
    return aspectRatio_;
}

glm::vec3 FreeCamera::frontDirection() const noexcept {
    const float yawRadians = glm::radians(yawDegrees_);
    const float pitchRadians = glm::radians(pitchDegrees_);
    return glm::normalize(glm::vec3{
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians),
    });
}

} // namespace glory::engine

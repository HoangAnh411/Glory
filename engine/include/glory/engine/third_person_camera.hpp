#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace glory::engine {

struct ThirdPersonCameraConfig {
    float distance = 4.5F;
    glm::vec3 targetOffset{0.0F, 1.2F, 0.0F};
    float yawDegrees = -90.0F;
    float pitchDegrees = 12.0F;
    float fieldOfViewDegrees = 45.0F;
    float aspectRatio = 16.0F / 9.0F;
    float nearPlane = 0.1F;
    float farPlane = 100.0F;
    float mouseSensitivity = 0.1F;
};

class ThirdPersonCamera {
public:
    explicit ThirdPersonCamera(const ThirdPersonCameraConfig& config = {})
        : distance_(config.distance), targetOffset_(config.targetOffset),
          yawDegrees_(config.yawDegrees), pitchDegrees_(config.pitchDegrees),
          fieldOfViewDegrees_(config.fieldOfViewDegrees), aspectRatio_(config.aspectRatio),
          nearPlane_(config.nearPlane), farPlane_(config.farPlane),
          mouseSensitivity_(config.mouseSensitivity) {
        if (!finite(targetOffset_) || !std::isfinite(distance_) || distance_ <= 0.0F ||
            !std::isfinite(yawDegrees_) || !std::isfinite(pitchDegrees_) ||
            !std::isfinite(fieldOfViewDegrees_) || fieldOfViewDegrees_ <= 0.0F ||
            fieldOfViewDegrees_ >= 180.0F || !std::isfinite(aspectRatio_) || aspectRatio_ <= 0.0F ||
            !std::isfinite(nearPlane_) || !std::isfinite(farPlane_) || nearPlane_ <= 0.0F ||
            farPlane_ <= nearPlane_ || !std::isfinite(mouseSensitivity_) || mouseSensitivity_ < 0.0F) {
            throw std::invalid_argument("Invalid third-person camera configuration");
        }
        yawDegrees_ = std::remainder(yawDegrees_, 360.0F);
        pitchDegrees_ = std::clamp(pitchDegrees_, minimumPitchDegrees, maximumPitchDegrees);
    }

    void update(const glm::vec3& target, float mouseDeltaX, float mouseDeltaY) noexcept {
        if (finite(target)) target_ = target;
        if (std::isfinite(mouseDeltaX) && std::isfinite(mouseDeltaY)) {
            yawDegrees_ = std::remainder(yawDegrees_ + mouseDeltaX * mouseSensitivity_, 360.0F);
            pitchDegrees_ = std::clamp(pitchDegrees_ - mouseDeltaY * mouseSensitivity_,
                minimumPitchDegrees, maximumPitchDegrees);
        }
    }

    void setTarget(const glm::vec3& target) noexcept { if (finite(target)) target_ = target; }
    void setAspectRatio(float aspectRatio) {
        if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0F)
            throw std::invalid_argument("Camera aspect ratio must be positive and finite");
        aspectRatio_ = aspectRatio;
    }

    [[nodiscard]] glm::vec3 position() const noexcept {
        const float yaw = glm::radians(yawDegrees_);
        const float pitch = glm::radians(pitchDegrees_);
        const glm::vec3 forward{std::cos(yaw) * std::cos(pitch), std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)};
        return target_ + targetOffset_ - forward * distance_;
    }
    [[nodiscard]] glm::vec3 targetPosition() const noexcept { return target_ + targetOffset_; }
    [[nodiscard]] float yawDegrees() const noexcept { return yawDegrees_; }
    [[nodiscard]] float pitchDegrees() const noexcept { return pitchDegrees_; }
    [[nodiscard]] float distance() const noexcept { return distance_; }
    [[nodiscard]] float aspectRatio() const noexcept { return aspectRatio_; }
    [[nodiscard]] glm::mat4 viewMatrix() const { return glm::lookAt(position(), targetPosition(), worldUp); }
    [[nodiscard]] glm::mat4 projectionMatrix() const {
        return glm::perspective(glm::radians(fieldOfViewDegrees_), aspectRatio_, nearPlane_, farPlane_);
    }
    [[nodiscard]] glm::mat4 viewProjectionMatrix() const { return projectionMatrix() * viewMatrix(); }

private:
    static bool finite(const glm::vec3& value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    static constexpr float minimumPitchDegrees = -80.0F;
    static constexpr float maximumPitchDegrees = 80.0F;
    static constexpr glm::vec3 worldUp{0.0F, 1.0F, 0.0F};
    glm::vec3 target_{0.0F};
    float distance_;
    glm::vec3 targetOffset_;
    float yawDegrees_;
    float pitchDegrees_;
    float fieldOfViewDegrees_;
    float aspectRatio_;
    float nearPlane_;
    float farPlane_;
    float mouseSensitivity_;
};

} // namespace glory::engine

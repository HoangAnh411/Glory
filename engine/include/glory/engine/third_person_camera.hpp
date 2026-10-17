#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace glory::engine {

struct ThirdPersonCameraConfig {
    float distance = 5.0F;
    float shoulderOffset = 0.0F;
    float framingOffset = 0.0F;
    glm::vec3 targetOffset{0.0F, 1.4F, 0.0F};
    float yawDegrees = -90.0F;
    float pitchDegrees = 15.0F;
    float fieldOfViewDegrees = 55.0F;
    float aspectRatio = 16.0F / 9.0F;
    float nearPlane = 0.1F;
    float farPlane = 100.0F;
    float mouseSensitivity = 0.1F;
    bool invertMouseY = false;
    bool autoReturnEnabled = false;
};

class ThirdPersonCamera {
public:
    explicit ThirdPersonCamera(const ThirdPersonCameraConfig& config = {})
        : distance_(config.distance), smoothedDistance_(config.distance), effectiveDistance_(config.distance),
          shoulderOffset_(config.shoulderOffset), framingOffset_(config.framingOffset),
          targetOffset_(config.targetOffset),
          yawDegrees_(config.yawDegrees), pitchDegrees_(config.pitchDegrees),
          fieldOfViewDegrees_(config.fieldOfViewDegrees), aspectRatio_(config.aspectRatio),
          nearPlane_(config.nearPlane), farPlane_(config.farPlane),
          mouseSensitivity_(config.mouseSensitivity), invertMouseY_(config.invertMouseY),
          autoReturnEnabled_(config.autoReturnEnabled) {
        if (!finite(targetOffset_) || !std::isfinite(distance_) || distance_ <= 0.0F ||
            !std::isfinite(shoulderOffset_) || !std::isfinite(framingOffset_) ||
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

    void update(const glm::vec3& target, float mouseDeltaX, float mouseDeltaY,
        float deltaSeconds = 1.0F / 60.0F) noexcept {
        if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0F) {
            const float zoomBlend = 1.0F - std::exp(-deltaSeconds / zoomSmoothSeconds);
            smoothedDistance_ += (distance_ - smoothedDistance_) * zoomBlend;
        }
        if (finite(target) && std::isfinite(deltaSeconds) && deltaSeconds > 0.0F) {
            if (!targetInitialized_) {
                target_ = target;
                targetVelocity_ = glm::vec3(0.0F);
                targetInitialized_ = true;
            } else {
                constexpr float springOmega = 40.0F;
                const glm::vec3 displacement = target_ - target;
                const float decay = std::exp(-springOmega * deltaSeconds);
                const glm::vec3 temporary = (targetVelocity_ + springOmega * displacement) * deltaSeconds;
                target_ = target + (displacement + temporary) * decay;
                targetVelocity_ = (targetVelocity_ - springOmega * temporary) * decay;
            }
        }
        if (locked_ && finite(lockTarget_)) {
            const glm::vec3 diff = lockTarget_ - target;
            const float horizontalDist = std::sqrt(diff.x * diff.x + diff.z * diff.z);
            if (horizontalDist > 0.1F) {
                const float desiredYaw = glm::degrees(std::atan2(diff.z, diff.x));
                const float yawDiff = std::remainder(desiredYaw - yawDegrees_, 360.0F);
                yawDegrees_ = std::remainder(yawDegrees_ + yawDiff * 0.12F, 360.0F);

                const float desiredPitch = glm::clamp(glm::degrees(std::atan2(diff.y, horizontalDist)) + 8.0F, 5.0F, 35.0F);
                pitchDegrees_ += (desiredPitch - pitchDegrees_) * 0.12F;
            }
            if (std::isfinite(mouseDeltaY)) {
                pitchDegrees_ = std::clamp(pitchDegrees_ + mouseDeltaY * mouseSensitivity_ * (invertMouseY_ ? -1.0F : 1.0F),
                    minimumPitchDegrees, maximumPitchDegrees);
            }
        } else {
            if (std::isfinite(mouseDeltaX) && std::isfinite(mouseDeltaY)) {
                yawDegrees_ = std::remainder(yawDegrees_ + mouseDeltaX * mouseSensitivity_, 360.0F);
                pitchDegrees_ = std::clamp(pitchDegrees_ + mouseDeltaY * mouseSensitivity_ * (invertMouseY_ ? -1.0F : 1.0F),
                    minimumPitchDegrees, maximumPitchDegrees);
            }
        }
    }

    void setTarget(const glm::vec3& target) noexcept { if (finite(target)) target_ = target; }
    void setOrbitPose(float yawDegrees, float pitchDegrees) noexcept {
        if (std::isfinite(yawDegrees)) yawDegrees_ = std::remainder(yawDegrees, 360.0F);
        if (std::isfinite(pitchDegrees)) pitchDegrees_ = std::clamp(pitchDegrees, minimumPitchDegrees, maximumPitchDegrees);
    }
    void constrainArm(float maximumDistance, float deltaSeconds) noexcept {
        if (!std::isfinite(maximumDistance) || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0F) return;
        const float safeDistance = std::clamp(maximumDistance, 0.0F, distance_);
        if (safeDistance < effectiveDistance_) effectiveDistance_ = safeDistance;
        else effectiveDistance_ = std::min(safeDistance, effectiveDistance_ + deltaSeconds * armReturnSpeed);
    }
    void setLocked(bool locked, const glm::vec3& lockTarget = glm::vec3(0.0F)) noexcept {
        locked_ = locked;
        if (finite(lockTarget)) lockTarget_ = lockTarget;
    }
    [[nodiscard]] bool isLocked() const noexcept { return locked_; }
    [[nodiscard]] const glm::vec3& lockTarget() const noexcept { return lockTarget_; }
    void setAspectRatio(float aspectRatio) {
        if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0F)
            throw std::invalid_argument("Camera aspect ratio must be positive and finite");
        aspectRatio_ = aspectRatio;
    }

    [[nodiscard]] glm::vec3 position() const noexcept {
        const float yaw = glm::radians(yawDegrees_);
        const float pitch = glm::radians(pitchDegrees_);
        const glm::vec3 forward{std::cos(yaw) * std::cos(pitch), -std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)};
        glm::vec3 cameraPosition = targetPosition() - forward * effectiveDistance_;
        cameraPosition.y = std::max(cameraPosition.y, 0.4F);
        return cameraPosition;
    }
    [[nodiscard]] glm::vec3 targetPosition() const noexcept {
        const float yaw = glm::radians(yawDegrees_);
        const float pitch = glm::radians(pitchDegrees_);
        const glm::vec3 cameraRight{-std::sin(yaw), 0.0F, std::cos(yaw)};
        const glm::vec3 cameraUp{std::cos(yaw) * std::sin(pitch), std::cos(pitch),
            std::sin(yaw) * std::sin(pitch)};
        const float verticalComposition = effectiveDistance_ * std::tan(glm::radians(fieldOfViewDegrees_) * 0.5F) * 0.18F;
        return target_ + targetOffset_ + cameraRight * (shoulderOffset_ + framingOffset_) + cameraUp * verticalComposition;
    }
    [[nodiscard]] glm::vec3 desiredPosition() const noexcept {
        const float yaw = glm::radians(yawDegrees_);
        const float pitch = glm::radians(pitchDegrees_);
        const glm::vec3 forward{std::cos(yaw) * std::cos(pitch), -std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)};
        return targetPosition() - forward * smoothedDistance_;
    }
    [[nodiscard]] float yawDegrees() const noexcept { return yawDegrees_; }
    [[nodiscard]] float pitchDegrees() const noexcept { return pitchDegrees_; }
    [[nodiscard]] float distance() const noexcept { return distance_; }
    [[nodiscard]] float shoulderOffset() const noexcept { return shoulderOffset_; }
    [[nodiscard]] float framingOffset() const noexcept { return framingOffset_; }
    void setZoom(float distance) noexcept {
        if (std::isfinite(distance)) distance_ = std::clamp(distance, 3.0F, 8.0F);
    }
    void zoomBy(float wheelDelta) noexcept { setZoom(distance_ - wheelDelta * 0.5F); }
    void setMouseSensitivity(float value) noexcept {
        if (std::isfinite(value)) mouseSensitivity_ = std::clamp(value, 0.02F, 0.5F);
    }
    [[nodiscard]] float mouseSensitivity() const noexcept { return mouseSensitivity_; }
    void setInvertMouseY(bool value) noexcept { invertMouseY_ = value; }
    [[nodiscard]] bool invertMouseY() const noexcept { return invertMouseY_; }
    void setAutoReturnEnabled(bool value) noexcept { autoReturnEnabled_ = value; }
    [[nodiscard]] bool autoReturnEnabled() const noexcept { return autoReturnEnabled_; }
    void setShoulderOffset(float value) noexcept {
        if (std::isfinite(value)) shoulderOffset_ = std::clamp(value, 0.0F, 0.4F);
    }
    void setFieldOfView(float degrees) noexcept {
        if (std::isfinite(degrees)) fieldOfViewDegrees_ = std::clamp(degrees, 40.0F, 80.0F);
    }
    [[nodiscard]] float fieldOfViewDegrees() const noexcept { return fieldOfViewDegrees_; }
    [[nodiscard]] float effectiveDistance() const noexcept { return effectiveDistance_; }
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

    static constexpr float minimumPitchDegrees = -10.0F;
    static constexpr float maximumPitchDegrees = 70.0F;
    static constexpr float armReturnSpeed = 10.0F;
    static constexpr float zoomSmoothSeconds = 0.12F;
    static constexpr glm::vec3 worldUp{0.0F, 1.0F, 0.0F};
    glm::vec3 target_{0.0F};
    glm::vec3 targetVelocity_{0.0F};
    bool targetInitialized_ = false;
    float distance_;
    float smoothedDistance_;
    float effectiveDistance_;
    float shoulderOffset_;
    float framingOffset_;
    glm::vec3 targetOffset_;
    float yawDegrees_;
    float pitchDegrees_;
    float fieldOfViewDegrees_;
    float aspectRatio_;
    float nearPlane_;
    float farPlane_;
    float mouseSensitivity_;
    bool invertMouseY_ = false;
    bool autoReturnEnabled_ = false;
    bool locked_ = false;
    glm::vec3 lockTarget_{0.0F};
};

} // namespace glory::engine

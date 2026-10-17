#include "glory/engine/third_person_camera.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
bool nearlyEqual(float left, float right, float epsilon = 1.0e-3F) {
    return std::abs(left - right) <= epsilon;
}
void testFollowOrbitAndMatrices() {
    glory::engine::ThirdPersonCamera camera;
    camera.setTarget({2.0F, 0.0F, 3.0F});
    const glm::vec3 position = camera.position();
    const glm::vec3 target = camera.targetPosition();
    expect(nearlyEqual(glm::length(position - target), camera.distance()), "camera maintains configured follow distance");
    expect(nearlyEqual(target.x, 2.0F) && nearlyEqual(target.y, 1.852F, 5.0e-3F),
        "camera targets the chest pivot with a small vertical composition offset and no shoulder shift");
    expect(nearlyEqual(camera.distance(), 5.0F), "enhanced camera defaults to a five-meter arm");
    expect(nearlyEqual(camera.fieldOfViewDegrees(), 55.0F), "orbit camera default FOV is 55 degrees");
    const glm::vec4 playerClip = camera.viewProjectionMatrix() *
        glm::vec4(glm::vec3{2.0F, 1.4F, 3.0F}, 1.0F);
    const glm::vec2 playerNdc = glm::vec2(playerClip) / playerClip.w;
    expect(playerNdc.x >= -0.12F && playerNdc.x <= 0.12F &&
        playerNdc.y >= -0.35F && playerNdc.y <= -0.05F,
        "unobstructed orbit camera frames the chest pivot near center-lower screen");
    camera.constrainArm(2.0F, 0.016F);
    expect(nearlyEqual(camera.effectiveDistance(), 2.0F), "camera arm never retracts inside the minimum distance");
    camera.constrainArm(5.0F, 0.1F);
    expect(nearlyEqual(camera.effectiveDistance(), 3.0F), "camera arm extends back smoothly after obstruction clears");
    camera.update({2.0F, 0.0F, 3.0F}, 100.0F, -100.0F);
    expect(nearlyEqual(camera.yawDegrees(), -80.0F) && nearlyEqual(camera.pitchDegrees(), 5.0F),
        "relative mouse deltas orbit the camera");
    camera.update({2.0F, 0.0F, 3.0F}, 0.0F, -10000.0F);
    expect(nearlyEqual(camera.pitchDegrees(), -10.0F), "pitch is clamped to the safe lower angle");
    const glm::mat4 viewProjection = camera.viewProjectionMatrix();
    expect(std::isfinite(viewProjection[0][0]) && std::isfinite(viewProjection[3][3]),
        "view-projection matrix contains finite values");
}
void testInvalidConfiguration() {
    bool rejected = false;
    try {
        glory::engine::ThirdPersonCameraConfig config;
        config.distance = 0.0F;
        glory::engine::ThirdPersonCamera camera(config);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    expect(rejected, "zero follow distance is rejected");
}

void testOrbitPivotFramingAcrossYawPitchAndZoom() {
    glory::engine::ThirdPersonCamera camera;
    for (float yaw = -180.0F; yaw <= 180.0F; yaw += 45.0F) {
        for (float pitch : {-10.0F, 15.0F, 40.0F, 70.0F}) {
            for (float distance : {3.0F, 5.0F, 8.0F}) {
                camera.setOrbitPose(yaw, pitch);
                camera.setZoom(distance);
                camera.setTarget({0.0F, 0.0F, 0.0F});
                const glm::vec4 clip = camera.viewProjectionMatrix() * glm::vec4(0.0F, 1.4F, 0.0F, 1.0F);
                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                expect(ndc.x >= -0.12F && ndc.x <= 0.12F && ndc.y >= -0.35F && ndc.y <= -0.05F,
                    "player chest pivot remains centered-low for each orbit angle and zoom");
                expect(camera.position().y >= 0.4F, "orbit camera remains above the ground clearance");
                expect(glm::length(camera.position() - camera.targetPosition()) >= 2.0F,
                    "orbit arm respects the minimum camera distance");
            }
        }
    }
}

void testMouseDeltaDoesNotDependOnFrameTime() {
    glory::engine::ThirdPersonCamera first;
    glory::engine::ThirdPersonCamera second;
    first.update({0.0F, 0.0F, 0.0F}, 30.0F, -12.0F, 1.0F / 30.0F);
    second.update({0.0F, 0.0F, 0.0F}, 30.0F, -12.0F, 1.0F / 144.0F);
    expect(nearlyEqual(first.yawDegrees(), second.yawDegrees()) &&
        nearlyEqual(first.pitchDegrees(), second.pitchDegrees()),
        "mouse pixel deltas produce identical camera rotation across frame rates");
}

void testZoomEasesAndAutoReturnIsOptIn() {
    glory::engine::ThirdPersonCamera camera;
    camera.setZoom(8.0F);
    expect(nearlyEqual(camera.effectiveDistance(), 5.0F), "zoom target does not teleport the camera arm");
    camera.update({0.0F, 0.0F, 0.0F}, 0.0F, 0.0F, 0.1F);
    camera.constrainArm(8.0F, 0.1F);
    expect(camera.effectiveDistance() > 5.0F && camera.effectiveDistance() < 8.0F,
        "zoom distance eases over time");
    expect(!camera.autoReturnEnabled(), "camera auto-return is disabled by default");
}
} // namespace

int main() {
    testFollowOrbitAndMatrices();
    testInvalidConfiguration();
    testOrbitPivotFramingAcrossYawPitchAndZoom();
    testMouseDeltaDoesNotDependOnFrameTime();
    testZoomEasesAndAutoReturnIsOptIn();
    if (failures != 0) return 1;
    std::cout << "All third-person camera tests passed\n";
    return 0;
}

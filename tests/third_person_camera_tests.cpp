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
    expect(nearlyEqual(target.x, 2.5F) && nearlyEqual(target.y, 1.4F), "camera target includes chest height and right shoulder offset");
    expect(nearlyEqual(camera.distance(), 5.0F), "enhanced camera defaults to a five-meter arm");
    expect(nearlyEqual(camera.fieldOfViewDegrees(), 65.0F), "enhanced camera default FOV is 65 degrees");
    camera.constrainArm(2.0F, 0.016F);
    expect(nearlyEqual(camera.effectiveDistance(), 2.5F), "camera arm never retracts inside the minimum distance");
    camera.constrainArm(5.0F, 0.1F);
    expect(nearlyEqual(camera.effectiveDistance(), 3.3F), "camera arm extends back smoothly after obstruction clears");
    camera.update({2.0F, 0.0F, 3.0F}, 100.0F, -100.0F);
    expect(nearlyEqual(camera.yawDegrees(), -80.0F) && nearlyEqual(camera.pitchDegrees(), 20.0F),
        "relative mouse deltas orbit the camera");
    camera.update({2.0F, 0.0F, 3.0F}, 0.0F, -10000.0F);
    expect(nearlyEqual(camera.pitchDegrees(), 55.0F), "pitch is clamped to the safe upper angle");
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
} // namespace

int main() {
    testFollowOrbitAndMatrices();
    testInvalidConfiguration();
    if (failures != 0) return 1;
    std::cout << "All third-person camera tests passed\n";
    return 0;
}

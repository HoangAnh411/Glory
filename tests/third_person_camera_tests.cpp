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
    expect(nearlyEqual(target.x, 2.0F) && nearlyEqual(target.y, 1.2F), "camera target includes the character offset");
    camera.update({2.0F, 0.0F, 3.0F}, 100.0F, -100.0F);
    expect(nearlyEqual(camera.yawDegrees(), -80.0F) && nearlyEqual(camera.pitchDegrees(), 22.0F),
        "relative mouse deltas orbit the camera");
    camera.update({2.0F, 0.0F, 3.0F}, 0.0F, -10000.0F);
    expect(nearlyEqual(camera.pitchDegrees(), 80.0F), "pitch is clamped before camera inversion");
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

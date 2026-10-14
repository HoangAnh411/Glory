#include "glory/engine/camera.hpp"

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

bool nearlyEqual(float left, float right, float epsilon = 1.0e-4F) {
    return std::abs(left - right) <= epsilon;
}

void testViewAndProjectionMatrices() {
    glory::engine::FreeCamera camera;
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix();
    const glm::mat4 viewProjection = camera.viewProjectionMatrix();

    expect(nearlyEqual(view[3][2], -3.0F), "view matrix contains camera translation");
    expect(nearlyEqual(projection[0][0] / projection[1][1], 1.0F / camera.aspectRatio()),
           "projection matrix reflects the camera aspect ratio");
    expect(std::isfinite(viewProjection[0][0]) && std::isfinite(viewProjection[3][3]),
           "view-projection matrix contains finite values");

    camera.setAspectRatio(1.0F);
    expect(nearlyEqual(camera.aspectRatio(), 1.0F), "aspect ratio can be updated for resize");
    expect(nearlyEqual(
               camera.projectionMatrix()[0][0] / camera.projectionMatrix()[1][1], 1.0F),
           "updated aspect ratio changes the projection matrix");
}

void testMouseLookAndPitchClamping() {
    glory::engine::FreeCamera camera;
    glory::input::InputFrame input;
    input.mouseDeltaX = 100.0F;
    camera.update(input, 1.0 / 60.0);
    expect(nearlyEqual(camera.yawDegrees(), -80.0F), "horizontal mouse delta changes yaw");

    input.mouseDeltaX = 0.0F;
    input.mouseDeltaY = -10000.0F;
    camera.update(input, 1.0 / 60.0);
    expect(nearlyEqual(camera.pitchDegrees(), 89.0F), "pitch is clamped at the upper limit");

    input.mouseDeltaY = 20000.0F;
    camera.update(input, 1.0 / 60.0);
    expect(nearlyEqual(camera.pitchDegrees(), -89.0F), "pitch is clamped at the lower limit");
}

void testMovementUsesFixedDeltaAndNormalizesDiagonalInput() {
    glory::engine::FreeCamera camera;
    glory::input::InputFrame input;
    input.held = {glory::input::Control::KeyW};
    camera.update(input, 0.5);
    expect(nearlyEqual(camera.position().z, 1.0F), "forward movement scales with fixed delta");

    glory::engine::FreeCamera diagonalCamera;
    input.held = {glory::input::Control::KeyW, glory::input::Control::KeyD};
    diagonalCamera.update(input, 1.0);
    const float displacement = glm::length(diagonalCamera.position() - glm::vec3(0.0F, 0.0F, 3.0F));
    expect(nearlyEqual(displacement, 4.0F), "diagonal movement is normalized");

    glory::engine::FreeCamera verticalCamera;
    input.held = {glory::input::Control::KeySpace};
    verticalCamera.update(input, 0.25);
    expect(nearlyEqual(verticalCamera.position().y, 1.0F), "Space moves the camera upward");

    input.held = {glory::input::Control::KeyLeftControl};
    verticalCamera.update(input, 0.25);
    expect(nearlyEqual(verticalCamera.position().y, 0.0F), "Control moves the camera downward");
}

void testInvalidConfigurationAndDeltaAreHandled() {
    bool rejectedAspect = false;
    try {
        glory::engine::FreeCamera camera;
        camera.setAspectRatio(0.0F);
    } catch (const std::invalid_argument&) {
        rejectedAspect = true;
    }
    expect(rejectedAspect, "non-positive aspect ratio is rejected");

    bool rejectedPlanes = false;
    try {
        glory::engine::CameraConfig config;
        config.farPlane = config.nearPlane;
        glory::engine::FreeCamera camera(config);
    } catch (const std::invalid_argument&) {
        rejectedPlanes = true;
    }
    expect(rejectedPlanes, "far plane must be greater than near plane");

    glory::engine::FreeCamera camera;
    glory::input::InputFrame input;
    input.held = {glory::input::Control::KeyW};
    camera.update(input, -1.0);
    expect(nearlyEqual(camera.position().z, 3.0F), "negative delta does not move the camera");
}

} // namespace

int main() {
    testViewAndProjectionMatrices();
    testMouseLookAndPitchClamping();
    testMovementUsesFixedDeltaAndNormalizesDiagonalInput();
    testInvalidConfigurationAndDeltaAreHandled();

    if (failures != 0) {
        std::cerr << failures << " camera test(s) failed\n";
        return 1;
    }
    std::cout << "All camera tests passed\n";
    return 0;
}

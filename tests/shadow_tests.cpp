#include "glory/engine/shadow_math.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void requireNear(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 1.0e-4F) throw std::runtime_error(message);
}

glm::vec3 project(const glm::mat4& matrix, const glm::vec3& point) {
    const glm::vec4 clip = matrix * glm::vec4(point, 1.0F);
    return glm::vec3(clip) / clip.w;
}

} // namespace

int main() {
    try {
        using glory::engine::calculateLightSpaceMatrix;
        const glm::vec3 center{2.0F, 3.0F, 4.0F};
        const glm::mat4 matrix = calculateLightSpaceMatrix(
            {0.0F, 0.0F, -1.0F}, center, {5.0F, 7.0F}, 1.0F, 21.0F);

        const glm::vec3 centerNdc = project(matrix, center);
        requireNear(centerNdc.x, 0.0F, "scene center should project to the light-space center X");
        requireNear(centerNdc.y, 0.0F, "scene center should project to the light-space center Y");
        requireNear(centerNdc.z, 0.0F, "scene center should lie midway between light near/far planes");

        const glm::vec3 horizontalEdge = project(matrix, center + glm::vec3(5.0F, 0.0F, 0.0F));
        const glm::vec3 verticalEdge = project(matrix, center + glm::vec3(0.0F, 7.0F, 0.0F));
        requireNear(horizontalEdge.x, 1.0F, "orthographic horizontal bound should map to clip edge");
        requireNear(verticalEdge.y, 1.0F, "orthographic vertical bound should map to clip edge");

        const glm::mat4 downwardLight = calculateLightSpaceMatrix(
            {0.0F, -1.0F, 0.0F}, center, {5.0F, 5.0F}, 1.0F, 21.0F);
        const float centerDepth = project(downwardLight, center).z;
        const float sourceSideDepth = project(downwardLight, center + glm::vec3(0.0F, 1.0F, 0.0F)).z;
        if (!(sourceSideDepth < centerDepth)) throw std::runtime_error("vertical light direction was not oriented toward the scene");

        bool rejected = false;
        try { (void)calculateLightSpaceMatrix({0.0F, 0.0F, 0.0F}, center, {5.0F, 5.0F}, 1.0F, 21.0F); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("zero light direction was accepted");

        rejected = false;
        try { (void)calculateLightSpaceMatrix({0.0F, 0.0F, -1.0F}, center, {0.0F, 5.0F}, 1.0F, 21.0F); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("invalid orthographic extent was accepted");
    } catch (const std::exception& error) {
        std::cerr << "Shadow math test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "CPU shadow matrix tests passed\n";
    return 0;
}

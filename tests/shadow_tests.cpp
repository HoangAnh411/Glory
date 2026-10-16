#include "glory/engine/shadow_math.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
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
        using glory::engine::calculateCascadeSplits;
        using glory::engine::snapShadowCenterToTexel;
        using glory::engine::calculateHeightFogFactor;
        const auto splits = calculateCascadeSplits(0.1F, 100.0F, 4, 0.65F);
        if (splits.size() != 4 || !(splits[0] < splits[1] && splits[1] < splits[2] && splits[2] < splits[3]))
            throw std::runtime_error("cascade splits must be monotonic");
        requireNear(splits.back(), 100.0F, "last cascade must end at shadow far distance");
        const glm::vec2 snapped = snapShadowCenterToTexel({1.231F, -2.674F}, 0.1F);
        requireNear(snapped.x, 1.2F, "cascade center should snap to the texel grid");
        requireNear(snapped.y, -2.7F, "cascade center should snap to the texel grid");
        const float fogNear = calculateHeightFogFactor(20.0F, 0.0F, 0.008F);
        const float fogFar = calculateHeightFogFactor(120.0F, 0.0F, 0.008F);
        const float fogHigh = calculateHeightFogFactor(120.0F, 30.0F, 0.008F);
        if (!(std::isfinite(fogNear) && std::isfinite(fogFar) && std::isfinite(fogHigh) &&
            fogNear < fogFar && fogHigh < fogFar)) throw std::runtime_error("height fog must be finite and attenuate by distance/height");
        const glm::mat4 cameraProjection = glm::perspective(glm::radians(65.0F), 16.0F / 9.0F, 0.1F, 500.0F);
        const glm::mat4 cameraViewA = glm::lookAt(glm::vec3(0.0F, 3.0F, 8.0F), glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        const glm::mat4 cameraViewB = glm::lookAt(glm::vec3(0.00001F, 3.0F, 8.0F), glm::vec3(0.00001F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        const auto cascadeMatricesA = glory::engine::calculateCascadeLightSpaceMatrices(
            cameraViewA, cameraProjection, {-0.5F, -0.6F, -0.4F}, splits, 0.1F, 2048);
        const auto cascadeMatricesB = glory::engine::calculateCascadeLightSpaceMatrices(
            cameraViewB, cameraProjection, {-0.5F, -0.6F, -0.4F}, splits, 0.1F, 2048);
        for (std::size_t cascade = 0; cascade < splits.size(); ++cascade)
            for (int column = 0; column < 4; ++column)
                for (int row = 0; row < 4; ++row)
                    if (!std::isfinite(cascadeMatricesA[cascade][column][row]))
                        throw std::runtime_error("cascade matrix contains a non-finite value");
        requireNear(cascadeMatricesA[0][3][0], cascadeMatricesB[0][3][0], "sub-texel camera translation should not shift snapped cascade X");
        requireNear(cascadeMatricesA[0][3][1], cascadeMatricesB[0][3][1], "sub-texel camera translation should not shift snapped cascade Y");
        const auto threeSplits = calculateCascadeSplits(0.1F, 80.0F, 3, 0.5F);
        const auto threeCascadeMatrices = glory::engine::calculateCascadeLightSpaceMatrices(
            cameraViewA, cameraProjection, {-0.5F, -0.6F, -0.4F}, threeSplits, 0.1F, 1024);
        bool rejected = false;
        const glm::vec3 nearVisible = glm::vec3(0.0F, 3.0F, 8.0F) +
            glm::normalize(glm::vec3(0.0F, 0.0F, 0.0F) - glm::vec3(0.0F, 3.0F, 8.0F)) * 2.0F;
        if (!glory::engine::sphereIntersectsClip(threeCascadeMatrices[0], nearVisible, 1.0F))
            throw std::runtime_error("visible actor sphere was culled from the near cascade");
        rejected = false;
        try {
            (void)glory::engine::calculateCascadeLightSpaceMatrices(
                cameraViewA, cameraProjection, {-0.5F, -0.6F, -0.4F}, {4.0F, 3.0F}, 0.1F, 2048);
        } catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("unordered cascade splits were accepted");
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

        rejected = false;
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

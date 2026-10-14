#include "glory/engine/pbr_math.hpp"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>

namespace glory::engine::pbr {
namespace {
constexpr float dielectricF0 = 0.04F;
constexpr float minimumRoughness = 0.04F;
constexpr float pi = 3.14159265358979323846F;
}

glm::vec3 dielectricMetallicF0(const glm::vec3& albedo, float metallic) noexcept {
    const float blend = std::clamp(metallic, 0.0F, 1.0F);
    return glm::mix(glm::vec3(dielectricF0), albedo, blend);
}

glm::vec3 fresnelSchlick(float cosine, const glm::vec3& f0) noexcept {
    const float factor = std::pow(1.0F - std::clamp(cosine, 0.0F, 1.0F), 5.0F);
    return f0 + (glm::vec3(1.0F) - f0) * factor;
}

float distributionGgx(float normalHalfway, float roughness) noexcept {
    const float alpha = std::max(roughness, minimumRoughness);
    const float alphaSquared = alpha * alpha;
    const float denominatorTerm = std::max(normalHalfway * normalHalfway * (alphaSquared - 1.0F) + 1.0F, 1.0e-6F);
    return alphaSquared / (pi * denominatorTerm * denominatorTerm);
}

float geometrySchlickGgx(float normalDirection, float roughness) noexcept {
    const float k = std::pow(std::clamp(roughness, minimumRoughness, 1.0F) + 1.0F, 2.0F) / 8.0F;
    const float denominator = normalDirection * (1.0F - k) + k;
    return denominator > 0.0F ? normalDirection / denominator : 0.0F;
}

float geometrySmith(float normalView, float normalLight, float roughness) noexcept {
    return geometrySchlickGgx(normalView, roughness) * geometrySchlickGgx(normalLight, roughness);
}

} // namespace glory::engine::pbr

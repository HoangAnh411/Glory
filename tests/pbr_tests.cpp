#include "glory/engine/pbr_math.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <glm/geometric.hpp>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace glory::engine::pbr;
    const glm::vec3 albedo(0.8F, 0.3F, 0.1F);
    const glm::vec3 dielectric = dielectricMetallicF0(albedo, 0.0F);
    require(std::abs(dielectric.x - 0.04F) < 1.0e-6F &&
        std::abs(dielectric.y - 0.04F) < 1.0e-6F &&
        std::abs(dielectric.z - 0.04F) < 1.0e-6F, "Dielectric F0 must be 0.04");

    const glm::vec3 metal = dielectricMetallicF0(albedo, 1.0F);
    require(glm::length(metal - albedo) < 1.0e-6F, "Metal F0 must equal albedo");

    const float smooth = distributionGgx(1.0F, 0.1F);
    const float rough = distributionGgx(1.0F, 0.8F);
    require(std::isfinite(smooth) && std::isfinite(rough) && smooth > rough && rough > 0.0F,
        "GGX distribution must be finite and peak more sharply at low roughness");
    require(geometrySmith(0.7F, 0.6F, 0.5F) >= 0.0F && geometrySmith(0.7F, 0.6F, 0.5F) <= 1.0F,
        "Smith geometry must stay in the unit interval");
    return 0;
}

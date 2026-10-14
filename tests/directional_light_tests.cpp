#include "glory/engine/directional_light.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        const glm::vec3 direction = glory::engine::directionFromAngles(0.0F, 0.0F);
        if (glm::length(direction - glm::vec3(1.0F, 0.0F, 0.0F)) > 1.0e-5F) {
            throw std::runtime_error("zero light angles should point along positive X");
        }
        const glm::vec3 vertical = glory::engine::directionFromAngles(123.0F, -89.0F);
        if (std::abs(glm::length(vertical) - 1.0F) > 1.0e-5F || vertical.y >= 0.0F) {
            throw std::runtime_error("light direction should be normalized and honor pitch");
        }
        glory::engine::DirectionalLight light;
        glory::engine::setDirectionFromAngles(light, -90.0F, -30.0F);
        if (glm::length(light.direction - glory::engine::directionFromAngles(-90.0F, -30.0F)) > 1.0e-5F ||
            light.ambientStrength <= 0.0F || light.shininess <= 0.0F) {
            throw std::runtime_error("light setter or default material parameters are invalid");
        }
        bool rejected = false;
        try { (void)glory::engine::directionFromAngles(0.0F, 90.0F); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("out-of-range light pitch was accepted");
    } catch (const std::exception& error) {
        std::cerr << "Directional light test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Directional light math tests passed\n";
    return 0;
}

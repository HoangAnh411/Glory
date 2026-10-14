#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace glory::engine {

struct Particle {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    glm::vec3 acceleration{0.0F};
    glm::vec4 color{1.0F};
    float size = 0.1F;
    float lifetime = 0.0F;
    float maxLifetime = 1.0F;
    float initialAlpha = 1.0F;
};

struct ParticleConfig {
    glm::vec3 direction{0.0F, 1.0F, 0.0F};
    glm::vec3 gravity{0.0F, -3.0F, 0.0F};
    glm::vec4 color{1.0F, 0.55F, 0.12F, 1.0F};
    float coneAngleRadians = 0.8F;
    float minimumSpeed = 1.5F;
    float maximumSpeed = 4.0F;
    float minimumSize = 0.035F;
    float maximumSize = 0.12F;
    float lifetime = 0.65F;
    std::uint32_t randomSeed = 0xC0FFEEU;
};

class ParticleSystem {
public:
    [[nodiscard]] std::size_t spawnBurst(const glm::vec3& position, std::size_t count,
        const ParticleConfig& config = {});
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] const std::vector<Particle>& particles() const noexcept { return particles_; }
    [[nodiscard]] std::size_t activeCount() const noexcept { return particles_.size(); }
    void clear() noexcept { particles_.clear(); }

private:
    std::vector<Particle> particles_;
};

} // namespace glory::engine

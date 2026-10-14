#pragma once

#include "glory/engine/gl_api.hpp"
#include "glory/engine/particle_system.hpp"
#include "glory/engine/shader_program.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace glory::engine {
class ParticleRenderer {
public:
    explicit ParticleRenderer(GlApi& gl);
    ~ParticleRenderer();
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;
    void draw(const ParticleSystem& particles, const glm::mat4& viewProjection,
        const glm::vec3& cameraRight, const glm::vec3& cameraUp) const;
private:
    GlApi& gl_;
    ShaderProgram shader_;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLint cameraRightLocation_ = -1;
    GLint cameraUpLocation_ = -1;
};
}

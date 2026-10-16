#pragma once

#include "glory/engine/gl_api.hpp"
#include "glory/engine/shader_program.hpp"

namespace glory::engine {
class Skybox {
public:
    explicit Skybox(GlApi& gl);
    ~Skybox();
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    void draw(const glm::mat4& view, const glm::mat4& projection, bool encodeSrgb) const noexcept;
    void bindEnvironment(unsigned int unit) const noexcept;
private:
    GlApi& gl_;
    ShaderProgram shader_;
    GLuint cubemap_ = 0;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLint cubemapLocation_ = -1;
    GLint encodeSrgbLocation_ = -1;
};
}

#include "glory/engine/skybox.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

namespace glory::engine {
namespace {
constexpr std::string_view skyboxVertexSource = R"glsl(#version 450 core
layout(location=0) in vec3 aPos;
uniform mat4 u_ViewProjection;
out vec3 vDirection;
void main(){
    vDirection=aPos;
    vec4 pos=u_ViewProjection*vec4(aPos,1.0);
    gl_Position=pos.xyww;
}
)glsl";
constexpr std::string_view skyboxFragmentSource = R"glsl(#version 450 core
in vec3 vDirection;
uniform samplerCube u_EnvironmentMap;
out vec4 fragmentColor;
void main(){ fragmentColor=vec4(texture(u_EnvironmentMap,normalize(vDirection)).rgb,1.0); }
)glsl";
constexpr std::array<float, 108> cubeVertices{{
    -1,-1,-1, 1,-1,-1, 1,1,-1, 1,1,-1, -1,1,-1, -1,-1,-1,
    -1,-1,1, 1,-1,1, 1,1,1, 1,1,1, -1,1,1, -1,-1,1,
    -1,1,1, -1,1,-1, -1,-1,-1, -1,-1,-1, -1,-1,1, -1,1,1,
    1,1,1, 1,1,-1, 1,-1,-1, 1,-1,-1, 1,-1,1, 1,1,1,
    -1,-1,-1, 1,-1,-1, 1,-1,1, 1,-1,1, -1,-1,1, -1,-1,-1,
    -1,1,-1, 1,1,-1, 1,1,1, 1,1,1, -1,1,1, -1,1,-1
}};

glm::vec3 faceDirection(int face, float u, float v) {
    switch (face) {
    case 0: return {1.0F, -v, -u};
    case 1: return {-1.0F, -v, u};
    case 2: return {u, 1.0F, v};
    case 3: return {u, -1.0F, -v};
    case 4: return {u, -v, 1.0F};
    default: return {-u, -v, -1.0F};
    }
}

std::array<unsigned char, 4> skyColor(int face, int x, int y, int size) {
    const float u = 2.0F * (static_cast<float>(x) + 0.5F) / static_cast<float>(size) - 1.0F;
    const float v = 2.0F * (static_cast<float>(y) + 0.5F) / static_cast<float>(size) - 1.0F;
    const glm::vec3 direction = glm::normalize(faceDirection(face, u, v));
    const float horizon = glm::smoothstep(-0.18F, 0.32F, direction.y);
    const float cloud = std::clamp(std::sin(direction.x * 19.0F + direction.z * 13.0F) *
        std::sin(direction.z * 23.0F - direction.y * 9.0F), 0.0F, 1.0F);
    glm::vec3 color = glm::mix(glm::vec3(0.43F, 0.55F, 0.57F), glm::vec3(0.13F, 0.38F, 0.68F), horizon);
    color = glm::mix(color, glm::vec3(0.78F, 0.84F, 0.86F), cloud * horizon * 0.18F);
    return {static_cast<unsigned char>(color.r * 255.0F), static_cast<unsigned char>(color.g * 255.0F),
        static_cast<unsigned char>(color.b * 255.0F), 255};
}
}

Skybox::Skybox(GlApi& gl) : gl_(gl), shader_(gl_, skyboxVertexSource, skyboxFragmentSource) {
    constexpr int resolution = 128;
    gl_.genTextures(1, &cubemap_);
    if (cubemap_ == 0) throw std::runtime_error("Failed to allocate skybox cubemap");
    try {
        gl_.bindTexture(GL_TEXTURE_CUBE_MAP, cubemap_);
        gl_.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl_.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl_.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        for (int face = 0; face < 6; ++face) {
            std::vector<unsigned char> pixels(static_cast<std::size_t>(resolution * resolution * 4));
            for (int y = 0; y < resolution; ++y) {
                for (int x = 0; x < resolution; ++x) {
                    const auto color = skyColor(face, x, y, resolution);
                    const std::size_t index = static_cast<std::size_t>((y * resolution + x) * 4);
                    for (std::size_t channel = 0; channel < 4; ++channel) pixels[index + channel] = color[channel];
                }
            }
            gl_.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, resolution, resolution,
                0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        }
        gl_.genVertexArrays(1, &vertexArray_);
        gl_.genBuffers(1, &vertexBuffer_);
        if (vertexArray_ == 0 || vertexBuffer_ == 0) throw std::runtime_error("Failed to allocate skybox mesh");
        gl_.bindVertexArray(vertexArray_);
        gl_.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        gl_.bufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(cubeVertices)), cubeVertices.data(), GL_STATIC_DRAW);
        gl_.enableVertexAttribArray(0);
        gl_.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * static_cast<GLsizei>(sizeof(float)), nullptr);
        gl_.bindVertexArray(0);
        shader_.use();
        cubemapLocation_ = gl_.getUniformLocation(shader_.program(), "u_EnvironmentMap");
        if (cubemapLocation_ < 0) throw std::runtime_error("Skybox environment sampler was not found");
        gl_.uniform1i(cubemapLocation_, 2);
    } catch (...) {
        if (vertexBuffer_ != 0) gl_.deleteBuffers(1, &vertexBuffer_);
        if (vertexArray_ != 0) gl_.deleteVertexArrays(1, &vertexArray_);
        if (cubemap_ != 0) gl_.deleteTextures(1, &cubemap_);
        vertexBuffer_ = vertexArray_ = cubemap_ = 0;
        throw;
    }
}

Skybox::~Skybox() {
    if (vertexBuffer_ != 0) gl_.deleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0) gl_.deleteVertexArrays(1, &vertexArray_);
    if (cubemap_ != 0) gl_.deleteTextures(1, &cubemap_);
}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection) const noexcept {
    const glm::mat4 viewNoTranslation{glm::mat3(view)};
    shader_.use();
    shader_.setViewProjection(projection * viewNoTranslation);
    gl_.activeTexture(GL_TEXTURE2);
    gl_.bindTexture(GL_TEXTURE_CUBE_MAP, cubemap_);
    gl_.depthFunc(GL_LEQUAL);
    gl_.bindVertexArray(vertexArray_);
    gl_.drawArrays(GL_TRIANGLES, 0, 36);
    gl_.bindVertexArray(0);
    gl_.depthFunc(GL_LESS);
}

void Skybox::bindEnvironment(unsigned int unit) const noexcept {
    gl_.activeTexture(GL_TEXTURE0 + unit);
    gl_.bindTexture(GL_TEXTURE_CUBE_MAP, cubemap_);
}
}

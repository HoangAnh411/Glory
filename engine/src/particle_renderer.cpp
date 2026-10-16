#include "glory/engine/particle_renderer.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace glory::engine {
namespace {
constexpr std::string_view particleVertexSource = R"glsl(#version 450 core
layout(location=0) in vec3 aCenter;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aCorner;
uniform mat4 u_ViewProjection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
out vec4 vColor;
void main(){
    vec3 worldPosition=aCenter+(u_CameraRight*aCorner.x+u_CameraUp*aCorner.y);
    gl_Position=u_ViewProjection*vec4(worldPosition,1.0);
    vColor=aColor;
}
)glsl";
constexpr std::string_view particleFragmentSource = R"glsl(#version 450 core
in vec4 vColor;
uniform bool u_EncodeSrgb;
out vec4 fragmentColor;
vec3 srgbToLinear(vec3 value){
    vec3 low=value/12.92;
    vec3 high=pow((value+0.055)/1.055,vec3(2.4));
    return mix(high,low,lessThanEqual(value,vec3(0.04045)));
}
vec3 linearToSrgb(vec3 value){
    vec3 low=value*12.92;
    vec3 high=1.055*pow(max(value,vec3(0.0)),vec3(1.0/2.4))-0.055;
    return mix(high,low,lessThanEqual(value,vec3(0.0031308)));
}
void main(){
    vec3 color=srgbToLinear(vColor.rgb);
    if(u_EncodeSrgb) color=linearToSrgb(color);
    fragmentColor=vec4(color,vColor.a);
}
)glsl";
struct ParticleVertex { float center[3]; float color[4]; float corner[2]; };
constexpr std::array<std::array<float, 2>, 6> corners{{
    {{-1.0F, -1.0F}}, {{1.0F, -1.0F}}, {{1.0F, 1.0F}},
    {{-1.0F, -1.0F}}, {{1.0F, 1.0F}}, {{-1.0F, 1.0F}}
}};
}

ParticleRenderer::ParticleRenderer(GlApi& gl)
    : gl_(gl), shader_(gl_, particleVertexSource, particleFragmentSource) {
    gl_.genVertexArrays(1, &vertexArray_);
    gl_.genBuffers(1, &vertexBuffer_);
    if (vertexArray_ == 0 || vertexBuffer_ == 0) {
        if (vertexBuffer_ != 0) gl_.deleteBuffers(1, &vertexBuffer_);
        if (vertexArray_ != 0) gl_.deleteVertexArrays(1, &vertexArray_);
        vertexBuffer_ = vertexArray_ = 0;
        throw std::runtime_error("Failed to allocate particle render buffers");
    }
    gl_.bindVertexArray(vertexArray_);
    gl_.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    gl_.bufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(ParticleVertex));
    gl_.enableVertexAttribArray(0);
    gl_.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(ParticleVertex, center)));
    gl_.enableVertexAttribArray(1);
    gl_.vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(ParticleVertex, color)));
    gl_.enableVertexAttribArray(2);
    gl_.vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(ParticleVertex, corner)));
    gl_.bindVertexArray(0);
    cameraRightLocation_ = gl_.getUniformLocation(shader_.program(), "u_CameraRight");
    cameraUpLocation_ = gl_.getUniformLocation(shader_.program(), "u_CameraUp");
    encodeSrgbLocation_ = gl_.getUniformLocation(shader_.program(), "u_EncodeSrgb");
}

ParticleRenderer::~ParticleRenderer() {
    if (vertexBuffer_ != 0) gl_.deleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0) gl_.deleteVertexArrays(1, &vertexArray_);
}

void ParticleRenderer::draw(const ParticleSystem& particles, const glm::mat4& viewProjection,
    const glm::vec3& cameraRight, const glm::vec3& cameraUp, bool encodeSrgb) const {
    const auto& active = particles.particles();
    if (active.empty()) return;
    std::vector<ParticleVertex> vertices;
    vertices.reserve(active.size() * corners.size());
    for (const Particle& particle : active) {
        for (const auto& corner : corners) {
            vertices.push_back({
                {particle.position.x, particle.position.y, particle.position.z},
                {particle.color.r, particle.color.g, particle.color.b, particle.color.a},
                {corner[0] * particle.size, corner[1] * particle.size}
            });
        }
    }

    shader_.use();
    shader_.setViewProjection(viewProjection);
    gl_.uniform3f(cameraRightLocation_, cameraRight.x, cameraRight.y, cameraRight.z);
    gl_.uniform3f(cameraUpLocation_, cameraUp.x, cameraUp.y, cameraUp.z);
    gl_.uniform1i(encodeSrgbLocation_, encodeSrgb ? 1 : 0);
    gl_.bindVertexArray(vertexArray_);
    gl_.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    gl_.bufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(ParticleVertex)), vertices.data(), GL_STREAM_DRAW);
    gl_.enable(GL_BLEND);
    gl_.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_.depthMask(GL_FALSE);
    gl_.drawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    gl_.depthMask(GL_TRUE);
    gl_.disable(GL_BLEND);
    gl_.bindVertexArray(0);
}
}

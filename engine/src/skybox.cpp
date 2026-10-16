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
uniform bool u_EncodeSrgb;
uniform vec3 u_SunDirection;
uniform int u_SkyPreset;
out vec4 fragmentColor;
vec3 linearToSrgb(vec3 value){
    vec3 low=value*12.92;
    vec3 high=1.055*pow(max(value,vec3(0.0)),vec3(1.0/2.4))-0.055;
    return mix(high,low,lessThanEqual(value,vec3(0.0031308)));
}
vec3 srgbToLinear(vec3 value){
    vec3 low=value/12.92;
    vec3 high=pow((value+0.055)/1.055,vec3(2.4));
    return mix(high,low,lessThanEqual(value,vec3(0.04045)));
}
void main(){
    vec3 dir=normalize(vDirection);
    float horizon=1.0-smoothstep(-0.18,0.46,dir.y);
    float horizonBand=exp(-pow((dir.y-0.015)*7.0,2.0));
    vec3 zenith=vec3(0.12,0.16,0.38);
    vec3 horizonColor=vec3(0.94,0.36,0.20);
    vec3 midColor=vec3(0.88,0.29,0.32);
    if(u_SkyPreset==1){zenith=vec3(0.16,0.20,0.42);horizonColor=vec3(0.98,0.58,0.48);midColor=vec3(0.65,0.38,0.62);}
    else if(u_SkyPreset==2){zenith=vec3(0.12,0.36,0.68);horizonColor=vec3(0.78,0.84,0.91);midColor=vec3(0.34,0.58,0.82);}
    else if(u_SkyPreset==3){zenith=vec3(0.12,0.38,0.75);horizonColor=vec3(0.55,0.62,0.70);midColor=horizonColor;}
    vec3 color=mix(zenith,horizonColor,horizon);
    color=mix(color,midColor,exp(-pow((dir.y-0.25)*3.5,2.0))*0.52);
    float sunDot=max(dot(dir,normalize(u_SunDirection)),0.0);
    float glow=exp(-max(0.0,1.0-sunDot)*32.0)*(0.2+0.8*horizonBand);
    float disc=pow(clamp((sunDot-0.996)/0.004,0.0,1.0),0.45);
    color+=vec3(0.72,0.30,0.10)*glow;
    color=mix(color,vec3(1.0,0.82,0.55),disc);
    color=srgbToLinear(color);
    if(u_EncodeSrgb) color=linearToSrgb(color);
    fragmentColor=vec4(color,1.0);
}
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
    const float horizon = glm::smoothstep(-0.22F, 0.48F, direction.y);
    const float horizonBand = std::exp(-std::pow((direction.y - 0.025F) * 8.0F, 2.0F));
    const float warmBand = std::exp(-std::pow((direction.y - 0.22F) * 4.5F, 2.0F));
    const float cloud = std::clamp(std::sin(direction.x * 12.0F + direction.z * 9.0F) *
        std::sin(direction.z * 15.0F - direction.y * 7.0F), 0.0F, 1.0F);
    const glm::vec3 horizonColor = glm::mix(glm::vec3(0.96F, 0.50F, 0.24F),
        glm::vec3(0.98F, 0.76F, 0.40F), glm::clamp((direction.y + 0.18F) * 2.0F, 0.0F, 1.0F));
    const glm::vec3 zenithColor = glm::vec3(0.12F, 0.16F, 0.38F);
    glm::vec3 color = glm::mix(horizonColor, zenithColor, horizon);
    color = glm::mix(color, glm::vec3(0.94F, 0.37F, 0.27F), warmBand * 0.55F);
    const glm::vec3 sunDirection = glm::normalize(glm::vec3(0.612F, 0.5F, 0.612F));
    const float sunDot = glm::dot(direction, sunDirection);
    const float sunDisc = std::pow(glm::clamp((sunDot - 0.996F) / 0.004F, 0.0F, 1.0F), 0.45F);
    const float sunGlow = std::exp(-std::max(0.0F, 1.0F - sunDot) * 34.0F);
    color += glm::vec3(1.4F, 0.75F, 0.30F) * sunGlow * horizonBand;
    color = glm::mix(color, glm::vec3(1.0F, 0.88F, 0.62F), sunDisc);
    color = glm::mix(color, glm::vec3(0.96F, 0.66F, 0.47F), cloud * (0.12F + horizonBand * 0.22F));
    return {
        static_cast<unsigned char>(std::clamp(color.r * 255.0F, 0.0F, 255.0F)),
        static_cast<unsigned char>(std::clamp(color.g * 255.0F, 0.0F, 255.0F)),
        static_cast<unsigned char>(std::clamp(color.b * 255.0F, 0.0F, 255.0F)),
        255
    };
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
            gl_.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_SRGB8_ALPHA8, resolution, resolution,
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
        encodeSrgbLocation_ = gl_.getUniformLocation(shader_.program(), "u_EncodeSrgb");
        sunDirectionLocation_ = gl_.getUniformLocation(shader_.program(), "u_SunDirection");
        presetLocation_ = gl_.getUniformLocation(shader_.program(), "u_SkyPreset");
        if (encodeSrgbLocation_ < 0 || sunDirectionLocation_ < 0 || presetLocation_ < 0)
            throw std::runtime_error("Skybox uniforms were not found");
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

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection, bool encodeSrgb,
    const glm::vec3& lightDirection, int preset) const noexcept {
    const glm::mat4 viewNoTranslation{glm::mat3(view)};
    shader_.use();
    shader_.setViewProjection(projection * viewNoTranslation);
    gl_.uniform1i(encodeSrgbLocation_, encodeSrgb ? 1 : 0);
    gl_.uniform3f(sunDirectionLocation_, -lightDirection.x, -lightDirection.y, -lightDirection.z);
    gl_.uniform1i(presetLocation_, preset);
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

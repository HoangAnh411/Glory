#include "glory/engine/ground_plane.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace glory::engine {
namespace {

struct GroundVertex {
    float position[3];
    float normal[3];
    float texCoord[2];
};

constexpr float floorY = -0.72F;
constexpr std::array<GroundVertex, 6> vertices{{
    {{-10.0F, floorY, -10.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
    {{ 10.0F, floorY,  10.0F}, {0.0F, 1.0F, 0.0F}, {8.0F, 8.0F}},
    {{ 10.0F, floorY, -10.0F}, {0.0F, 1.0F, 0.0F}, {8.0F, 0.0F}},
    {{-10.0F, floorY, -10.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
    {{-10.0F, floorY,  10.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 8.0F}},
    {{ 10.0F, floorY,  10.0F}, {0.0F, 1.0F, 0.0F}, {8.0F, 8.0F}},
}};

} // namespace

GroundPlane::GroundPlane(GlApi& gl) : gl_(gl) {
    gl_.genVertexArrays(1, &vertexArray_);
    if (vertexArray_ == 0) throw std::runtime_error("Failed to create ground plane vertex array");
    gl_.genBuffers(1, &vertexBuffer_);
    if (vertexBuffer_ == 0) {
        gl_.deleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
        throw std::runtime_error("Failed to create ground plane vertex buffer");
    }

    gl_.bindVertexArray(vertexArray_);
    gl_.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    gl_.bufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vertices)), vertices.data(), GL_STATIC_DRAW);
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(GroundVertex));
    gl_.enableVertexAttribArray(0);
    gl_.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(GroundVertex, position)));
    gl_.enableVertexAttribArray(1);
    gl_.vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(GroundVertex, normal)));
    gl_.enableVertexAttribArray(2);
    gl_.vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(GroundVertex, texCoord)));
    gl_.bindVertexArray(0);
    gl_.bindBuffer(GL_ARRAY_BUFFER, 0);
}

GroundPlane::~GroundPlane() {
    if (vertexBuffer_ != 0) gl_.deleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0) gl_.deleteVertexArrays(1, &vertexArray_);
}

void GroundPlane::draw() const noexcept {
    gl_.bindVertexArray(vertexArray_);
    gl_.drawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    gl_.bindVertexArray(0);
}

} // namespace glory::engine

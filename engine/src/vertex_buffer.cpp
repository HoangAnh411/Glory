#include "glory/engine/vertex_buffer.hpp"

#include <cstddef>
#include <stdexcept>

namespace glory::engine {

VertexBuffer::VertexBuffer(GlApi& gl, const std::array<TriangleVertex, 3>& vertices)
    : gl_(gl) {
    gl_.genVertexArrays(1, &vertexArray_);
    if (vertexArray_ == 0) {
        throw std::runtime_error("Failed to create triangle vertex array");
    }

    gl_.genBuffers(1, &vertexBuffer_);
    if (vertexBuffer_ == 0) {
        gl_.deleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
        throw std::runtime_error("Failed to create triangle vertex buffer");
    }

    gl_.bindVertexArray(vertexArray_);
    gl_.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    gl_.bufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(vertices)),
        vertices.data(),
        GL_STATIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(TriangleVertex));
    gl_.enableVertexAttribArray(0);
    gl_.vertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(TriangleVertex, position)));
    gl_.enableVertexAttribArray(1);
    gl_.vertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(TriangleVertex, color)));
    gl_.bindVertexArray(0);
    gl_.bindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer() {
    if (vertexBuffer_ != 0) {
        gl_.deleteBuffers(1, &vertexBuffer_);
    }
    if (vertexArray_ != 0) {
        gl_.deleteVertexArrays(1, &vertexArray_);
    }
}

void VertexBuffer::draw() const noexcept {
    gl_.bindVertexArray(vertexArray_);
    gl_.drawArrays(GL_TRIANGLES, 0, 3);
    gl_.bindVertexArray(0);
}

} // namespace glory::engine

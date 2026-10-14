#pragma once

#include "glory/engine/gl_api.hpp"

#include <array>

namespace glory::engine {

struct TriangleVertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

class VertexBuffer {
public:
    VertexBuffer(GlApi& gl, const std::array<TriangleVertex, 3>& vertices);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    void draw() const noexcept;

private:
    GlApi& gl_;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
};

} // namespace glory::engine

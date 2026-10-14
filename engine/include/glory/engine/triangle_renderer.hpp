#pragma once

#include "glory/engine/gl_api.hpp"
#include "glory/engine/shader_program.hpp"
#include "glory/engine/vertex_buffer.hpp"

#include <glm/mat4x4.hpp>

namespace glory::engine {

class TriangleRenderer {
public:
    TriangleRenderer();

    TriangleRenderer(const TriangleRenderer&) = delete;
    TriangleRenderer& operator=(const TriangleRenderer&) = delete;

    void render(const glm::mat4& viewProjection, int drawableWidth, int drawableHeight) const noexcept;

private:
    GlApi gl_;
    ShaderProgram shader_;
    VertexBuffer vertexBuffer_;
};

} // namespace glory::engine

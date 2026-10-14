#include "glory/engine/triangle_renderer.hpp"

#include <array>
#include <string_view>

namespace glory::engine {
namespace {

constexpr std::string_view vertexShaderSource = R"glsl(#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 u_ViewProjection;

out vec3 vertexColor;

void main() {
    gl_Position = u_ViewProjection * vec4(position, 1.0);
    vertexColor = color;
}
)glsl";

constexpr std::string_view fragmentShaderSource = R"glsl(#version 450 core
in vec3 vertexColor;
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(vertexColor, 1.0);
}
)glsl";

constexpr std::array<TriangleVertex, 3> triangleVertices{{
    {{{0.0F, 0.65F, 0.0F}}, {{1.0F, 0.2F, 0.15F}}},
    {{{-0.6F, -0.45F, 0.0F}}, {{0.15F, 0.8F, 0.35F}}},
    {{{0.6F, -0.45F, 0.0F}}, {{0.2F, 0.4F, 1.0F}}},
}};

} // namespace

TriangleRenderer::TriangleRenderer()
    : gl_(),
      shader_(gl_, vertexShaderSource, fragmentShaderSource),
      vertexBuffer_(gl_, triangleVertices) {
    gl_.enable(GL_DEPTH_TEST);
}

void TriangleRenderer::render(
    const glm::mat4& viewProjection,
    int drawableWidth,
    int drawableHeight) const noexcept {
    if (drawableWidth <= 0 || drawableHeight <= 0) {
        return;
    }

    gl_.viewport(0, 0, drawableWidth, drawableHeight);
    gl_.clearColor(0.035F, 0.055F, 0.075F, 1.0F);
    gl_.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader_.use();
    shader_.setViewProjection(viewProjection);
    vertexBuffer_.draw();
}

} // namespace glory::engine

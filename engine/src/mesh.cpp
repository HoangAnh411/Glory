#include "glory/engine/mesh.hpp"
#include <cstddef>
#include <climits>
#include <stdexcept>
namespace glory::engine {
Mesh::Mesh(GlApi& gl, const GltfAsset& asset) : gl_(gl), indexCount_(static_cast<GLsizei>(asset.indices.size())) {
    if (asset.vertices.empty() || asset.indices.empty() || asset.indices.size() > static_cast<std::size_t>(INT_MAX)) throw std::runtime_error("Invalid or oversized mesh data");
    gl_.genVertexArrays(1, &vao_); gl_.genBuffers(1, &vbo_); gl_.genBuffers(1, &ebo_);
    if (!vao_ || !vbo_ || !ebo_) { if (ebo_) gl_.deleteBuffers(1, &ebo_); if (vbo_) gl_.deleteBuffers(1, &vbo_); if (vao_) gl_.deleteVertexArrays(1, &vao_); vao_ = vbo_ = ebo_ = 0; throw std::runtime_error("Failed to allocate mesh buffers"); }
    gl_.bindVertexArray(vao_);
    gl_.bindBuffer(GL_ARRAY_BUFFER, vbo_);
    gl_.bufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(asset.vertices.size() * sizeof(GltfVertex)), asset.vertices.data(), GL_STATIC_DRAW);
    gl_.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    gl_.bufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(asset.indices.size() * sizeof(std::uint32_t)), asset.indices.data(), GL_STATIC_DRAW);
    gl_.enableVertexAttribArray(0); gl_.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GltfVertex), reinterpret_cast<void*>(offsetof(GltfVertex, position)));
    gl_.enableVertexAttribArray(1); gl_.vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GltfVertex), reinterpret_cast<void*>(offsetof(GltfVertex, normal)));
    gl_.enableVertexAttribArray(2); gl_.vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GltfVertex), reinterpret_cast<void*>(offsetof(GltfVertex, texCoord)));
    gl_.enableVertexAttribArray(3); gl_.vertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(GltfVertex), reinterpret_cast<void*>(offsetof(GltfVertex, joints)));
    gl_.enableVertexAttribArray(4); gl_.vertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GltfVertex), reinterpret_cast<void*>(offsetof(GltfVertex, weights)));
    gl_.bindVertexArray(0);
}
Mesh::~Mesh() { if (ebo_) gl_.deleteBuffers(1, &ebo_); if (vbo_) gl_.deleteBuffers(1, &vbo_); if (vao_) gl_.deleteVertexArrays(1, &vao_); }
void Mesh::draw() const noexcept { gl_.bindVertexArray(vao_); gl_.drawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr); gl_.bindVertexArray(0); }
}

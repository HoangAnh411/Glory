#pragma once
#include "glory/engine/gl_api.hpp"
#include "glory/engine/gltf_asset.hpp"
namespace glory::engine {
class Mesh {
public:
    Mesh(GlApi& gl, const GltfAsset& asset);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    void draw() const noexcept;
private:
    GlApi& gl_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
};
}

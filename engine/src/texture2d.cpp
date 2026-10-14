#include "glory/engine/texture2d.hpp"
#include <stdexcept>
namespace glory::engine {
Texture2D::Texture2D(GlApi& gl, const ImageData& image) : gl_(gl) {
    if (image.width <= 0 || image.height <= 0 || image.rgba.size() != static_cast<std::size_t>(image.width) * image.height * 4U) throw std::runtime_error("Invalid RGBA image for texture upload");
    gl_.genTextures(1, &texture_);
    if (!texture_) throw std::runtime_error("Failed to allocate OpenGL texture");
    gl_.bindTexture(GL_TEXTURE_2D, texture_);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
    gl_.generateMipmap(GL_TEXTURE_2D);
}
Texture2D::~Texture2D() { if (texture_) gl_.deleteTextures(1, &texture_); }
void Texture2D::bind(unsigned int unit) const noexcept { gl_.activeTexture(GL_TEXTURE0 + unit); gl_.bindTexture(GL_TEXTURE_2D, texture_); }
}

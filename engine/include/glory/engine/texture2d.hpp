#pragma once
#include "glory/engine/gl_api.hpp"
#include "glory/engine/image_loader.hpp"
namespace glory::engine {
class Texture2D {
public:
    Texture2D(GlApi& gl, const ImageData& image, bool srgbColor = true);
    ~Texture2D();
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    void bind(unsigned int unit = 0) const noexcept;
private:
    GlApi& gl_;
    GLuint texture_ = 0;
};
}

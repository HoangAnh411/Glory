#pragma once

#include "glory/engine/gl_api.hpp"

namespace glory::engine {

class ShadowMap {
public:
    explicit ShadowMap(GlApi& gl, int resolution = 2048);
    ~ShadowMap();
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void bindForWriting() const noexcept;
    void bindForSampling(int textureUnit) const noexcept;
    static void bindDefaultFramebuffer(const GlApi& gl) noexcept;
    [[nodiscard]] int resolution() const noexcept { return resolution_; }

private:
    void release() noexcept;

    GlApi& gl_;
    GLuint framebuffer_ = 0;
    GLuint depthTexture_ = 0;
    int resolution_ = 0;
};

} // namespace glory::engine

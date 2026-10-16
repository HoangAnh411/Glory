#include "glory/engine/shadow_map.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace glory::engine {

ShadowMap::ShadowMap(GlApi& gl, int resolution) : gl_(gl), resolution_(resolution) {
    if (resolution_ <= 0) throw std::invalid_argument("Shadow map resolution must be positive");
    allocate();
}

void ShadowMap::allocate() {
    try {
        gl_.genTextures(1, &depthTexture_);
        if (depthTexture_ == 0) throw std::runtime_error("Failed to create shadow depth texture");
        gl_.bindTexture(GL_TEXTURE_2D, depthTexture_);
        gl_.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution_ * 2, resolution_ * 2, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        constexpr std::array<GLfloat, 4> borderColor{1.0F, 1.0F, 1.0F, 1.0F};
        gl_.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

        gl_.genFramebuffers(1, &framebuffer_);
        if (framebuffer_ == 0) throw std::runtime_error("Failed to create shadow framebuffer");
        gl_.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        gl_.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);
        gl_.drawBuffer(GL_NONE);
        gl_.readBuffer(GL_NONE);
        const GLenum status = gl_.checkFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Shadow atlas framebuffer is incomplete at " +
                std::to_string(resolution_ * 2) + "x" + std::to_string(resolution_ * 2) +
                " (status " + std::to_string(status) + ")");
        }
        gl_.bindFramebuffer(GL_FRAMEBUFFER, 0);
        gl_.bindTexture(GL_TEXTURE_2D, 0);
    } catch (...) {
        gl_.bindFramebuffer(GL_FRAMEBUFFER, 0);
        gl_.bindTexture(GL_TEXTURE_2D, 0);
        release();
        throw;
    }
}

void ShadowMap::setResolution(int resolution) {
    if (resolution != 1024 && resolution != 2048 && resolution != 4096)
        throw std::invalid_argument("Shadow tile resolution must be 1024, 2048, or 4096");
    if (resolution == resolution_) return;
    const int previousResolution = resolution_;
    release();
    resolution_ = resolution;
    try {
        allocate();
    } catch (...) {
        resolution_ = previousResolution;
        allocate();
        throw;
    }
}

ShadowMap::~ShadowMap() { release(); }

void ShadowMap::release() noexcept {
    if (framebuffer_ != 0) gl_.deleteFramebuffers(1, &framebuffer_);
    if (depthTexture_ != 0) gl_.deleteTextures(1, &depthTexture_);
    framebuffer_ = 0;
    depthTexture_ = 0;
}

void ShadowMap::bindForWriting(int cascade) const noexcept {
    gl_.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    const int tile = cascade % 4;
    gl_.viewport((tile & 1) * resolution_, (tile >> 1) * resolution_, resolution_, resolution_);
}

void ShadowMap::bindForSampling(int textureUnit) const noexcept {
    gl_.activeTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
    gl_.bindTexture(GL_TEXTURE_2D, depthTexture_);
}

void ShadowMap::bindDefaultFramebuffer(const GlApi& gl) noexcept {
    gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace glory::engine

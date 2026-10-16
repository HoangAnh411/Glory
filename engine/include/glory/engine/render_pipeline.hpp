#pragma once

#include "glory/engine/gl_api.hpp"
#include "glory/engine/shader_program.hpp"
#include "glory/engine/shadow_map.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace glory::engine {

enum class GpuPass : std::size_t {
    ShadowCascade0, ShadowCascade1, ShadowCascade2, ShadowCascade3, ActorShadow,
    Scene, Sky, ParticleOverlay, Tonemap, Total, Count
};

struct GpuTimingSample {
    std::array<double, static_cast<std::size_t>(GpuPass::Count)> milliseconds{};
};

class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    void beginFrame(int width, int height);
    void bindSceneTarget() noexcept;
    void beginPass(GpuPass pass) noexcept;
    void endPass() noexcept;
    void beginActorShadowTiming(int cascade) noexcept;
    void endActorShadowTiming(int cascade) noexcept;
    void finishFrame();
    void setHdrEnabled(bool enabled) noexcept { hdrEnabled_ = enabled; }
    [[nodiscard]] bool hdrEnabled() const noexcept { return hdrEnabled_; }
    void setExposure(float exposure) noexcept;
    [[nodiscard]] float exposure() const noexcept { return exposure_; }
    [[nodiscard]] bool timerQueriesAvailable() const noexcept { return gl_.timerQueriesAvailable; }
    [[nodiscard]] const std::string& timerStatus() const noexcept { return timerStatus_; }
    [[nodiscard]] const std::string& rendererInfo() const noexcept { return rendererInfo_; }
    [[nodiscard]] const std::deque<GpuTimingSample>& history() const noexcept { return history_; }
    [[nodiscard]] bool framebufferComplete() const noexcept { return framebufferComplete_; }
    [[nodiscard]] std::uint64_t resizeCount() const noexcept { return resizeCount_; }
    [[nodiscard]] ShadowMap& shadowMap() noexcept { return shadowMap_; }

private:
    static constexpr std::size_t passCount = static_cast<std::size_t>(GpuPass::Count);
    static constexpr std::size_t actorTimestampCount = 8;
    static constexpr std::size_t queryCount = passCount + 1 + actorTimestampCount;
    static constexpr std::size_t ringSize = 3;
    struct QuerySlot {
        std::array<GLuint, queryCount> queries{};
        std::array<bool, queryCount> issued{};
        bool inFlight = false;
    };

    void resize(int width, int height);
    void collectAvailableQueries() noexcept;
    [[nodiscard]] static std::size_t passIndex(GpuPass pass) noexcept;

    GlApi gl_;
    ShadowMap shadowMap_;
    ShaderProgram tonemapShader_;
    GLuint framebuffer_ = 0;
    GLuint colorTexture_ = 0;
    GLuint depthTexture_ = 0;
    GLuint fullscreenVao_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool framebufferComplete_ = false;
    bool hdrEnabled_ = true;
    float exposure_ = 1.0F;
    GLint exposureLocation_ = -1;
    GLint hdrTextureLocation_ = -1;
    std::array<QuerySlot, ringSize> querySlots_{};
    std::size_t nextSlot_ = 0;
    std::size_t activeSlot_ = ringSize;
    GpuPass activePass_ = GpuPass::Count;
    bool recordingQueries_ = false;
    std::deque<GpuTimingSample> history_;
    std::string timerStatus_;
    std::string rendererInfo_;
    std::uint64_t resizeCount_ = 0;
};

} // namespace glory::engine

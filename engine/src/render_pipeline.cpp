#include "glory/engine/render_pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace glory::engine {
namespace {

constexpr std::string_view fullscreenVertexSource = R"glsl(#version 450 core
uniform mat4 u_ViewProjection;
out vec2 vUv;
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUv = position * 0.5;
    gl_Position = u_ViewProjection * vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)glsl";

constexpr std::string_view tonemapFragmentSource = R"glsl(#version 450 core
in vec2 vUv;
uniform sampler2D u_HdrColor;
uniform float u_Exposure;
out vec4 fragmentColor;
vec3 linearToSrgb(vec3 value) {
    vec3 low = value * 12.92;
    vec3 high = 1.055 * pow(max(value, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(value, vec3(0.0031308)));
}
vec3 acesApproximation(vec3 value) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((value * (a * value + b)) / (value * (c * value + d) + e), 0.0, 1.0);
}
void main() {
    vec3 hdr = max(texture(u_HdrColor, vUv).rgb * u_Exposure, vec3(0.0));
    fragmentColor = vec4(linearToSrgb(acesApproximation(hdr)), 1.0);
}
)glsl";

} // namespace

RenderPipeline::RenderPipeline()
    : shadowMap_(gl_), tonemapShader_(gl_, fullscreenVertexSource, tonemapFragmentSource) {
    const auto* renderer = gl_.getString(GL_RENDERER);
    const auto* version = gl_.getString(GL_VERSION);
    rendererInfo_ = renderer ? reinterpret_cast<const char*>(renderer) : "Unknown OpenGL renderer";
    rendererInfo_ += " | ";
    rendererInfo_ += version ? reinterpret_cast<const char*>(version) : "Unknown OpenGL version";
    exposureLocation_ = gl_.getUniformLocation(tonemapShader_.program(), "u_Exposure");
    hdrTextureLocation_ = gl_.getUniformLocation(tonemapShader_.program(), "u_HdrColor");
    if (hdrTextureLocation_ < 0 || exposureLocation_ < 0)
        throw std::runtime_error("Tonemap shader uniforms were not found");
    gl_.genVertexArrays(1, &fullscreenVao_);
    if (fullscreenVao_ == 0) throw std::runtime_error("Failed to allocate tonemap fullscreen VAO");
    if (gl_.timerQueriesAvailable) {
        std::array<GLuint, ringSize * queryCount> queries{};
        gl_.genQueries(static_cast<GLsizei>(queries.size()), queries.data());
        bool allocated = std::all_of(queries.begin(), queries.end(), [](GLuint query) { return query != 0; });
        if (allocated) {
            for (std::size_t slot = 0; slot < ringSize; ++slot)
                std::copy_n(queries.begin() + static_cast<std::ptrdiff_t>(slot * queryCount),
                    queryCount, querySlots_[slot].queries.begin());
            timerStatus_ = "GL_TIME_ELAPSED + GL_TIMESTAMP ready (3-frame ring)";
        } else {
            gl_.deleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
            gl_.timerQueriesAvailable = false;
            timerStatus_ = "GPU timer query allocation failed; timings unavailable";
        }
    } else {
        timerStatus_ = "GPU timer queries unsupported by this OpenGL driver";
    }
}

RenderPipeline::~RenderPipeline() {
    if (gl_.timerQueriesAvailable) {
        for (const QuerySlot& slot : querySlots_)
            gl_.deleteQueries(static_cast<GLsizei>(slot.queries.size()), slot.queries.data());
    }
    if (fullscreenVao_ != 0) gl_.deleteVertexArrays(1, &fullscreenVao_);
    if (depthTexture_ != 0) gl_.deleteTextures(1, &depthTexture_);
    if (colorTexture_ != 0) gl_.deleteTextures(1, &colorTexture_);
    if (framebuffer_ != 0) gl_.deleteFramebuffers(1, &framebuffer_);
}

void RenderPipeline::setExposure(float exposure) noexcept {
    if (std::isfinite(exposure)) exposure_ = std::clamp(exposure, 0.05F, 8.0F);
}

void RenderPipeline::resize(int width, int height) {
    GLuint nextFramebuffer = 0;
    GLuint nextColor = 0;
    GLuint nextDepth = 0;
    gl_.genFramebuffers(1, &nextFramebuffer);
    gl_.genTextures(1, &nextColor);
    gl_.genTextures(1, &nextDepth);
    if (nextFramebuffer == 0 || nextColor == 0 || nextDepth == 0) {
        if (nextDepth) gl_.deleteTextures(1, &nextDepth);
        if (nextColor) gl_.deleteTextures(1, &nextColor);
        if (nextFramebuffer) gl_.deleteFramebuffers(1, &nextFramebuffer);
        throw std::runtime_error("HDR framebuffer allocation failed for " +
            std::to_string(width) + "x" + std::to_string(height));
    }

    gl_.bindTexture(GL_TEXTURE_2D, nextColor);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);

    gl_.bindTexture(GL_TEXTURE_2D, nextDepth);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

    gl_.bindFramebuffer(GL_FRAMEBUFFER, nextFramebuffer);
    gl_.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextColor, 0);
    gl_.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, nextDepth, 0);
    const GLenum status = gl_.checkFramebufferStatus(GL_FRAMEBUFFER);
    gl_.bindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::ostringstream message;
        message << "HDR framebuffer incomplete at " << width << 'x' << height
                << " (status 0x" << std::hex << status << ')';
        gl_.deleteTextures(1, &nextDepth);
        gl_.deleteTextures(1, &nextColor);
        gl_.deleteFramebuffers(1, &nextFramebuffer);
        framebufferComplete_ = false;
        throw std::runtime_error(message.str());
    }

    if (depthTexture_) gl_.deleteTextures(1, &depthTexture_);
    if (colorTexture_) gl_.deleteTextures(1, &colorTexture_);
    if (framebuffer_) gl_.deleteFramebuffers(1, &framebuffer_);
    framebuffer_ = nextFramebuffer;
    colorTexture_ = nextColor;
    depthTexture_ = nextDepth;
    width_ = width;
    height_ = height;
    framebufferComplete_ = true;
    ++resizeCount_;
}

void RenderPipeline::collectAvailableQueries() noexcept {
    if (!gl_.timerQueriesAvailable) return;
    for (QuerySlot& slot : querySlots_) {
        if (!slot.inFlight) continue;
        bool available = true;
        for (std::size_t queryIndex = 0; queryIndex < queryCount; ++queryIndex) {
            if (!slot.issued[queryIndex]) continue;
            GLint resultAvailable = GL_FALSE;
            gl_.getQueryObjectiv(slot.queries[queryIndex], GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
            available = available && resultAvailable == GL_TRUE;
        }
        if (!available) continue;

        std::array<GLuint64, queryCount> values{};
        for (std::size_t i = 0; i < queryCount; ++i) {
            if (slot.issued[i]) gl_.getQueryObjectui64v(slot.queries[i], GL_QUERY_RESULT, &values[i]);
        }
        GpuTimingSample sample;
    for (std::size_t i = 0; i < passCount - 1; ++i)
            sample.milliseconds[i] = static_cast<double>(values[i]) / 1.0e6;
        sample.milliseconds[static_cast<std::size_t>(GpuPass::ActorShadow)] = 0.0;
        for (std::size_t cascade = 0; cascade < 4; ++cascade) {
            const std::size_t startIndex = passCount + 1 + cascade * 2;
            const std::size_t endIndex = startIndex + 1;
            if (values[endIndex] >= values[startIndex])
                sample.milliseconds[static_cast<std::size_t>(GpuPass::ActorShadow)] +=
                    static_cast<double>(values[endIndex] - values[startIndex]) / 1.0e6;
        }
        sample.milliseconds[static_cast<std::size_t>(GpuPass::Total)] =
            values[passCount] >= values[passCount - 1]
                ? static_cast<double>(values[passCount] - values[passCount - 1]) / 1.0e6 : 0.0;
        history_.push_back(sample);
        if (history_.size() > 240) history_.pop_front();
        slot.inFlight = false;
        slot.issued.fill(false);
    }
}

void RenderPipeline::beginFrame(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width != width_ || height != height_) resize(width, height);
    collectAvailableQueries();
    recordingQueries_ = false;
    activeSlot_ = ringSize;
    if (gl_.timerQueriesAvailable) {
        for (std::size_t offset = 0; offset < ringSize; ++offset) {
            const std::size_t candidate = (nextSlot_ + offset) % ringSize;
            if (!querySlots_[candidate].inFlight) {
                activeSlot_ = candidate;
                nextSlot_ = (candidate + 1) % ringSize;
                recordingQueries_ = true;
                querySlots_[candidate].issued.fill(false);
                gl_.queryCounter(querySlots_[candidate].queries[passCount - 1], GL_TIMESTAMP);
                querySlots_[candidate].issued[passCount - 1] = true;
                break;
            }
        }
    }
    bindSceneTarget();
}

void RenderPipeline::bindSceneTarget() noexcept {
    gl_.bindFramebuffer(GL_FRAMEBUFFER, hdrEnabled_ ? framebuffer_ : 0);
    gl_.viewport(0, 0, width_, height_);
}

std::size_t RenderPipeline::passIndex(GpuPass pass) noexcept {
    return static_cast<std::size_t>(pass);
}

void RenderPipeline::beginPass(GpuPass pass) noexcept {
    if (!recordingQueries_ || pass >= GpuPass::Tonemap || activePass_ != GpuPass::Count) return;
    activePass_ = pass;
    gl_.beginQuery(GL_TIME_ELAPSED, querySlots_[activeSlot_].queries[passIndex(pass)]);
    querySlots_[activeSlot_].issued[passIndex(pass)] = true;
}

void RenderPipeline::endPass() noexcept {
    if (!recordingQueries_ || activePass_ == GpuPass::Count) return;
    gl_.endQuery(GL_TIME_ELAPSED);
    activePass_ = GpuPass::Count;
}

void RenderPipeline::beginActorShadowTiming(int cascade) noexcept {
    if (!recordingQueries_ || cascade < 0 || cascade >= 4) return;
    const std::size_t queryIndex = passCount + 1 + static_cast<std::size_t>(cascade) * 2;
    gl_.queryCounter(querySlots_[activeSlot_].queries[queryIndex], GL_TIMESTAMP);
    querySlots_[activeSlot_].issued[queryIndex] = true;
}

void RenderPipeline::endActorShadowTiming(int cascade) noexcept {
    if (!recordingQueries_ || cascade < 0 || cascade >= 4) return;
    const std::size_t queryIndex = passCount + 2 + static_cast<std::size_t>(cascade) * 2;
    gl_.queryCounter(querySlots_[activeSlot_].queries[queryIndex], GL_TIMESTAMP);
    querySlots_[activeSlot_].issued[queryIndex] = true;
}

void RenderPipeline::finishFrame() {
    if (hdrEnabled_) {
        gl_.bindFramebuffer(GL_FRAMEBUFFER, 0);
        gl_.viewport(0, 0, width_, height_);
        gl_.disable(GL_DEPTH_TEST);
        if (recordingQueries_) {
            gl_.beginQuery(GL_TIME_ELAPSED, querySlots_[activeSlot_].queries[passIndex(GpuPass::Tonemap)]);
            querySlots_[activeSlot_].issued[passIndex(GpuPass::Tonemap)] = true;
        }
        tonemapShader_.use();
        tonemapShader_.setViewProjection(glm::mat4(1.0F));
        gl_.uniform1f(exposureLocation_, exposure_);
        gl_.uniform1i(hdrTextureLocation_, 0);
        gl_.activeTexture(GL_TEXTURE0);
        gl_.bindTexture(GL_TEXTURE_2D, colorTexture_);
        gl_.bindVertexArray(fullscreenVao_);
        gl_.drawArrays(GL_TRIANGLES, 0, 3);
        gl_.bindVertexArray(0);
        if (recordingQueries_) gl_.endQuery(GL_TIME_ELAPSED);
        gl_.enable(GL_DEPTH_TEST);
    }
    if (recordingQueries_) {
        gl_.queryCounter(querySlots_[activeSlot_].queries[passCount], GL_TIMESTAMP);
        querySlots_[activeSlot_].issued[passCount] = true;
        querySlots_[activeSlot_].inFlight = true;
    }
}

} // namespace glory::engine

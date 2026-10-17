#pragma once
#include "glory/engine/gl_api.hpp"
#include "glory/engine/directional_light.hpp"
#include "glory/engine/model.hpp"
#include "glory/engine/shader_program.hpp"
#include "glory/engine/shadow_map.hpp"
#include "glory/engine/ground_plane.hpp"
#include "glory/engine/skybox.hpp"
#include "glory/engine/particle_renderer.hpp"
#include "glory/engine/particle_system.hpp"
#include "glory/engine/texture2d.hpp"
#include "glory/engine/render_pipeline.hpp"
#include <filesystem>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <functional>
namespace glory::engine {
class ModelRenderer;
struct RenderActor {
    ModelRenderer* renderer = nullptr;
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
    float scale = 1.0F;
    glm::vec3 tint{1.0F};
};
struct StaticWorldProp {
    std::string type;
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
    float scale = 1.0F;
};
class ModelRenderer {
public:
    explicit ModelRenderer(const std::filesystem::path& modelPath);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPosition,
        const DirectionalLight& light, int width, int height, RenderPipeline& pipeline,
        const std::vector<RenderActor>& actors = {}) const;
    void updateAnimation(float deltaSeconds);
    void setTransform(const glm::vec3& position, float yawDegrees) noexcept;
    void setDeadAppearance(bool dead) noexcept { deadAppearance_ = dead; }
    [[nodiscard]] float cameraObstructionDistance(const glm::vec3& origin,
        const glm::vec3& desiredPosition, float cameraRadius = 0.2F) const noexcept;
    [[nodiscard]] const std::string& cameraBlockerName() const noexcept { return cameraBlockerName_; }
    [[nodiscard]] const glm::vec3& cameraBlockerPosition() const noexcept { return cameraBlockerPosition_; }
    [[nodiscard]] float lastCameraObstructionDistance() const noexcept { return lastCameraObstructionDistance_; }
    [[nodiscard]] std::pair<glm::vec3, glm::vec3> localBounds() const { return model_.animatedBounds(); }
    [[nodiscard]] glm::vec3 rootJointWorldPosition() const {
        return glm::vec3(modelTransform_ * glm::vec4(model_.rootJointPosition(), 1.0F));
    }
    [[nodiscard]] std::size_t spawnParticleBurst(const glm::vec3& position, std::size_t count = 48);
    [[nodiscard]] std::size_t activeParticleCount() const noexcept { return particles_.activeCount(); }
    void setAnimationPlaying(bool playing) noexcept;
    void setAnimationTime(float time) noexcept;
    void selectAnimation(std::size_t index) noexcept;
    void setFadeDuration(float seconds) noexcept;
    [[nodiscard]] float fadeDuration() const noexcept;
    [[nodiscard]] float blendFactor() const noexcept;
    [[nodiscard]] const std::string& animationState() const noexcept;
    [[nodiscard]] bool triggerAnimationState(const std::string& state);
    void setAnimationEventCallback(AnimationEventCallback callback);
    void setShadowsEnabled(bool enabled) noexcept { shadowsEnabled_ = enabled; }
    [[nodiscard]] bool shadowsEnabled() const noexcept { return shadowsEnabled_; }
    void setCharacterLightingEnabled(bool enabled) noexcept { characterLightingEnabled_ = enabled; }
    [[nodiscard]] bool characterLightingEnabled() const noexcept { return characterLightingEnabled_; }
    void setFogEnabled(bool enabled) noexcept { fogEnabled_ = enabled; }
    [[nodiscard]] bool fogEnabled() const noexcept { return fogEnabled_; }
    void setFogDensity(float density) noexcept;
    [[nodiscard]] float fogDensity() const noexcept { return fogDensity_; }
    void setSkyPreset(int preset) noexcept { skyPreset_ = std::clamp(preset, 0, 4); }
    [[nodiscard]] int skyPreset() const noexcept { return skyPreset_; }
    void setHemisphereEnabled(bool enabled) noexcept { hemisphereEnabled_ = enabled; }
    [[nodiscard]] bool hemisphereEnabled() const noexcept { return hemisphereEnabled_; }
    void setShadowBias(float bias) noexcept;
    [[nodiscard]] float shadowBias() const noexcept { return shadowBias_; }
    void setNormalOffsetBias(float bias) noexcept;
    [[nodiscard]] float normalOffsetBias() const noexcept { return normalOffsetBias_; }
    void setCascadeCount(int count) noexcept { cascadeCount_ = std::clamp(count, 3, 4); }
    [[nodiscard]] int cascadeCount() const noexcept { return cascadeCount_; }
    void setCascadeLambda(float lambda) noexcept;
    [[nodiscard]] float cascadeLambda() const noexcept { return cascadeLambda_; }
    void setShadowFar(float distance) noexcept;
    [[nodiscard]] float shadowFar() const noexcept { return shadowFar_; }
    void setPcfSamples(int samples) noexcept { pcfSamples_ = samples <= 4 ? 4 : (samples <= 8 ? 8 : 16); }
    [[nodiscard]] int pcfSamples() const noexcept { return pcfSamples_; }
    void setCascadeDebug(bool enabled) noexcept { cascadeDebug_ = enabled; }
    [[nodiscard]] bool cascadeDebug() const noexcept { return cascadeDebug_; }
    void setLightingDebugView(int view) noexcept { lightingDebugView_ = std::clamp(view, 0, 8); }
    [[nodiscard]] int lightingDebugView() const noexcept { return lightingDebugView_; }
    void setPbrMaterial(float metallic, float roughness, float ao) noexcept;
    [[nodiscard]] float metallic() const noexcept { return metallic_; }
    [[nodiscard]] float roughness() const noexcept { return roughness_; }
    [[nodiscard]] float ambientOcclusion() const noexcept { return ambientOcclusion_; }
    void setSkyboxEnabled(bool enabled) noexcept { skyboxEnabled_ = enabled; }
    [[nodiscard]] bool skyboxEnabled() const noexcept { return skyboxEnabled_; }
    [[nodiscard]] std::size_t animationIndex() const noexcept;
    [[nodiscard]] std::size_t animationCount() const noexcept;
    [[nodiscard]] bool hasAnimation() const noexcept;
    [[nodiscard]] bool isAnimationPlaying() const noexcept;
    [[nodiscard]] float animationTime() const noexcept;
    [[nodiscard]] float animationDuration() const noexcept;
    [[nodiscard]] const std::string& animationName() const noexcept;
    [[nodiscard]] const std::string& animationName(std::size_t index) const noexcept;
private:
    GlApi gl_;
    ShaderProgram depthShader_;
    ShaderProgram shader_;
    Model model_;
    GroundPlane groundPlane_;
    Skybox skybox_;
    ParticleSystem particles_;
    ParticleRenderer particleRenderer_;
    bool shadowsEnabled_ = true;
    bool characterLightingEnabled_ = true;
    bool fogEnabled_ = true;
    bool hemisphereEnabled_ = true;
    float fogDensity_ = 0.004F;
    int skyPreset_ = 4;
    float shadowBias_ = 0.005F;
    float normalOffsetBias_ = 0.015F;
    int cascadeCount_ = 4;
    float cascadeLambda_ = 0.65F;
    float shadowFar_ = 100.0F;
    int pcfSamples_ = 16;
    bool cascadeDebug_ = false;
    int lightingDebugView_ = 0;
    float metallic_ = 0.0F;
    float roughness_ = 0.5F;
    float ambientOcclusion_ = 1.0F;
    bool skyboxEnabled_ = true;
    bool deadAppearance_ = false;
    glm::mat4 modelTransform_{1.0F};
    std::unique_ptr<Texture2D> arenaFloorTexture_;
    std::unordered_map<std::string, std::unique_ptr<Model>> propModelTemplates_;
    std::vector<StaticWorldProp> staticProps_;
    mutable std::string cameraBlockerName_ = "none";
    mutable glm::vec3 cameraBlockerPosition_{0.0F};
    mutable float lastCameraObstructionDistance_ = 0.0F;
};
}

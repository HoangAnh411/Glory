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
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <functional>
namespace glory::engine {
struct StaticWorldProp {
    std::string type;
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
    float scale = 1.0F;
};
class ModelRenderer {
public:
    explicit ModelRenderer(const std::filesystem::path& modelPath);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPosition, const DirectionalLight& light, int width, int height) const;
    void renderOverlay(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPosition,
        const DirectionalLight& light, const glm::vec3& position, float yawDegrees, float scale,
        const glm::vec3& tint = glm::vec3(1.0F)) const;
    void updateAnimation(float deltaSeconds);
    void setTransform(const glm::vec3& position, float yawDegrees) noexcept;
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
    void setShadowBias(float bias) noexcept;
    [[nodiscard]] float shadowBias() const noexcept { return shadowBias_; }
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
    ShadowMap shadowMap_;
    ShaderProgram depthShader_;
    ShaderProgram shader_;
    Model model_;
    GroundPlane groundPlane_;
    Skybox skybox_;
    ParticleSystem particles_;
    ParticleRenderer particleRenderer_;
    bool shadowsEnabled_ = true;
    float shadowBias_ = 0.005F;
    float metallic_ = 0.0F;
    float roughness_ = 0.5F;
    float ambientOcclusion_ = 1.0F;
    bool skyboxEnabled_ = true;
    glm::mat4 modelTransform_{1.0F};
    std::unique_ptr<Texture2D> arenaFloorTexture_;
    std::unordered_map<std::string, std::unique_ptr<Model>> propModelTemplates_;
    std::vector<StaticWorldProp> staticProps_;
};
}

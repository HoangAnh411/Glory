#pragma once
#include "glory/engine/mesh.hpp"
#include "glory/engine/texture2d.hpp"
#include "glory/engine/animation.hpp"
#include <filesystem>
#include <memory>
#include <glm/vec3.hpp>
#include <utility>
namespace glory::engine {
class Model {
public:
    Model(GlApi& gl, const std::filesystem::path& path);
    void draw() const noexcept;
    void bindTexture(unsigned int unit = 0) const noexcept;
    void updateAnimation(float deltaSeconds);
    void setAnimationPlaying(bool playing) noexcept;
    void setAnimationTime(float time) noexcept;
    void selectAnimation(std::size_t index) noexcept;
    void setFadeDuration(float seconds) noexcept;
    [[nodiscard]] float fadeDuration() const noexcept;
    [[nodiscard]] float blendFactor() const noexcept;
    [[nodiscard]] const std::string& animationState() const noexcept;
    [[nodiscard]] bool triggerAnimationState(const std::string& state);
    void setAnimationEventCallback(AnimationEventCallback callback);
    [[nodiscard]] std::size_t animationIndex() const noexcept;
    [[nodiscard]] std::size_t animationCount() const noexcept;
    [[nodiscard]] bool hasAnimation() const noexcept;
    [[nodiscard]] bool isAnimationPlaying() const noexcept;
    [[nodiscard]] float animationTime() const noexcept;
    [[nodiscard]] float animationDuration() const noexcept;
    [[nodiscard]] const std::string& animationName() const noexcept;
    [[nodiscard]] const std::string& animationName(std::size_t index) const noexcept;
    [[nodiscard]] std::vector<glm::mat4> skinPalette() const;
    [[nodiscard]] bool hasSkin() const noexcept;
    [[nodiscard]] glm::vec3 baseColorFactor() const noexcept;
    [[nodiscard]] glm::vec3 emissiveFactor() const noexcept;
    [[nodiscard]] float metallicFactor() const noexcept;
    [[nodiscard]] float roughnessFactor() const noexcept;
    [[nodiscard]] float ambientOcclusion() const noexcept;
    [[nodiscard]] std::pair<glm::vec3, glm::vec3> localBounds() const noexcept;
private:
    GltfAsset asset_;
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Texture2D> texture_;
    Animator animator_;
    AnimationStateMachine stateMachine_;
    AnimationEventCallback eventCallback_;
    bool stateMachineActive_ = false;
    std::size_t animationIndex_ = 0;
};
}

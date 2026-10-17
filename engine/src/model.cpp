#include "glory/engine/model.hpp"
#include "glory/engine/gltf_asset.hpp"
#include "glory/engine/image_loader.hpp"
#include <memory>
#include <limits>
#include <glm/vec4.hpp>
namespace glory::engine {
Model::Model(GlApi& gl, const std::filesystem::path& path) : asset_(loadGltfAsset(path)) {
    ImageData image = asset_.embeddedTexture.empty() ? loadImage(asset_.texturePath) : decodeImage(asset_.embeddedTexture);
    mesh_ = std::make_unique<Mesh>(gl, asset_);
    texture_ = std::make_unique<Texture2D>(gl, image);
}
void Model::draw() const noexcept { mesh_->draw(); }
void Model::bindTexture(unsigned int unit) const noexcept { texture_->bind(unit); }
void Model::updateAnimation(float deltaSeconds) {
    if (!hasAnimation()) return;
    if (stateMachineActive_) {
        if (!animator_.isPlaying()) return;
        stateMachine_.advance(deltaSeconds, asset_.skeleton, asset_.animations, eventCallback_);
        return;
    }
    const float previousTime = animator_.time();
    animator_.advance(deltaSeconds, animationDuration());
    if (animator_.isPlaying()) dispatchAnimationEvents(asset_.animations[animationIndex_], previousTime, deltaSeconds, eventCallback_);
}
void Model::setAnimationPlaying(bool playing) noexcept { animator_.setPlaying(playing); }
void Model::setAnimationTime(float time) noexcept {
    if (stateMachineActive_) stateMachine_.setTime(time, asset_.animations);
    else animator_.setTime(time, animationDuration());
}
bool Model::hasAnimation() const noexcept { return !asset_.animations.empty(); }
bool Model::isAnimationPlaying() const noexcept { return animator_.isPlaying(); }
float Model::animationTime() const noexcept { return stateMachineActive_ ? stateMachine_.time() : animator_.time(); }
void Model::selectAnimation(std::size_t index) noexcept {
    if (index >= asset_.animations.size()) return;
    stateMachineActive_ = false;
    animationIndex_ = index;
    animator_.setTime(0.0F, animationDuration());
}
void Model::setFadeDuration(float seconds) noexcept { stateMachine_.setFadeDuration(seconds); }
float Model::fadeDuration() const noexcept { return stateMachine_.fadeDuration(); }
float Model::blendFactor() const noexcept { return stateMachineActive_ ? stateMachine_.blendFactor() : 1.0F; }
const std::string& Model::animationState() const noexcept {
    return stateMachineActive_ ? stateMachine_.stateName() : animationName();
}
bool Model::triggerAnimationState(const std::string& state) {
    if (!hasAnimation()) return false;
    const AnimationPose currentPose = stateMachineActive_ ? AnimationPose{} :
        evaluatePose(asset_.skeleton, &asset_.animations[animationIndex_], animator_.time());
    if (!stateMachine_.trigger(state, asset_.skeleton, asset_.animations,
        stateMachineActive_ ? nullptr : &currentPose)) return false;
    stateMachineActive_ = true;
    return true;
}
void Model::setAnimationEventCallback(AnimationEventCallback callback) { eventCallback_ = std::move(callback); }
std::size_t Model::animationIndex() const noexcept { return stateMachineActive_ ? stateMachine_.clipIndex() : animationIndex_; }
std::size_t Model::animationCount() const noexcept { return asset_.animations.size(); }
float Model::animationDuration() const noexcept { return hasAnimation() ? asset_.animations[animationIndex()].duration : 0.0F; }
const std::string& Model::animationName() const noexcept { static const std::string noAnimation = "No animation"; return hasAnimation() ? asset_.animations[animationIndex()].name : noAnimation; }
const std::string& Model::animationName(std::size_t index) const noexcept { static const std::string emptyName; return index < asset_.animations.size() ? asset_.animations[index].name : emptyName; }
std::vector<glm::mat4> Model::skinPalette() const {
    if (!asset_.hasSkin) return {};
    if (stateMachineActive_) return evaluateSkinPalette(asset_.skeleton, stateMachine_.pose(asset_.skeleton, asset_.animations));
    const AnimationClip* clip = hasAnimation() ? &asset_.animations[animationIndex_] : nullptr;
    return evaluateSkinPalette(asset_.skeleton, clip, animator_.time());
}
bool Model::hasSkin() const noexcept { return asset_.hasSkin; }
glm::vec3 Model::baseColorFactor() const noexcept { return {asset_.baseColorFactor[0], asset_.baseColorFactor[1], asset_.baseColorFactor[2]}; }
glm::vec3 Model::emissiveFactor() const noexcept { return {asset_.emissiveFactor[0], asset_.emissiveFactor[1], asset_.emissiveFactor[2]}; }
float Model::metallicFactor() const noexcept { return asset_.metallicFactor; }
float Model::roughnessFactor() const noexcept { return asset_.roughnessFactor; }
float Model::ambientOcclusion() const noexcept { return asset_.ambientOcclusion; }
std::pair<glm::vec3, glm::vec3> Model::localBounds() const noexcept {
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    for (const GltfVertex& vertex : asset_.vertices) {
        const glm::vec3 position{vertex.position[0], vertex.position[1], vertex.position[2]};
        minimum = glm::min(minimum, position);
        maximum = glm::max(maximum, position);
    }
    return {minimum, maximum};
}
std::pair<glm::vec3, glm::vec3> Model::animatedBounds() const {
    if (!asset_.hasSkin) return localBounds();
    const std::vector<glm::mat4> palette = skinPalette();
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    for (const GltfVertex& vertex : asset_.vertices) {
        glm::mat4 skin(0.0F);
        for (std::size_t influence = 0; influence < 4; ++influence) {
            const std::size_t joint = vertex.joints[influence];
            if (joint < palette.size()) skin += palette[joint] * vertex.weights[influence];
        }
        const glm::vec3 point = glm::vec3(skin * glm::vec4(vertex.position[0], vertex.position[1], vertex.position[2], 1.0F));
        minimum = glm::min(minimum, point);
        maximum = glm::max(maximum, point);
    }
    return {minimum, maximum};
}
}

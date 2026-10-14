#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace glory::engine {

struct SkeletonNode {
    std::string name;
    std::int32_t parent = -1;
    glm::vec3 translation{0.0F};
    glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F};
};

struct Skeleton {
    std::vector<SkeletonNode> nodes;
    std::vector<std::uint32_t> jointNodeIndices;
    std::vector<glm::mat4> inverseBindMatrices;
};

struct VectorKeyframe { float time = 0.0F; glm::vec3 value{0.0F}; };
struct RotationKeyframe { float time = 0.0F; glm::quat value{1.0F, 0.0F, 0.0F, 0.0F}; };

struct AnimationEvent {
    float time = 0.0F;
    std::string name;
    std::unordered_map<std::string, std::string> parameters;
};

struct AnimationChannel {
    std::uint32_t nodeIndex = 0;
    bool translationStep = false;
    bool rotationStep = false;
    bool scaleStep = false;
    std::vector<VectorKeyframe> translations;
    std::vector<RotationKeyframe> rotations;
    std::vector<VectorKeyframe> scales;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0F;
    std::vector<AnimationChannel> channels;
    std::vector<AnimationEvent> events;
};

using AnimationPose = std::vector<SkeletonNode>;
using AnimationEventCallback = std::function<void(const AnimationEvent&)>;

class Animator {
public:
    void setPlaying(bool playing) noexcept { playing_ = playing; }
    [[nodiscard]] bool isPlaying() const noexcept { return playing_; }
    [[nodiscard]] float time() const noexcept { return time_; }
    void setTime(float time, float duration) noexcept;
    void advance(float deltaSeconds, float duration) noexcept;
private:
    float time_ = 0.0F;
    bool playing_ = true;
};

class AnimationStateMachine {
public:
    void setFadeDuration(float seconds) noexcept;
    [[nodiscard]] float fadeDuration() const noexcept { return fadeDuration_; }
    [[nodiscard]] float blendFactor() const noexcept { return blendFactor_; }
    [[nodiscard]] const std::string& stateName() const noexcept { return stateName_; }
    [[nodiscard]] float time() const noexcept { return targetTime_; }
    [[nodiscard]] bool trigger(const std::string& state, const Skeleton& skeleton,
        const std::vector<AnimationClip>& clips, const AnimationPose* initialPose = nullptr);
    void setTime(float time, const std::vector<AnimationClip>& clips) noexcept;
    void advance(float deltaSeconds, const Skeleton& skeleton,
        const std::vector<AnimationClip>& clips, const AnimationEventCallback& callback);
    [[nodiscard]] AnimationPose pose(const Skeleton& skeleton,
        const std::vector<AnimationClip>& clips) const;
    [[nodiscard]] std::size_t clipIndex() const noexcept { return targetClip_; }
private:
    std::size_t targetClip_ = 0;
    float targetTime_ = 0.0F;
    float fadeDuration_ = 0.25F;
    float fadeElapsed_ = 0.0F;
    float blendFactor_ = 1.0F;
    std::string stateName_;
    AnimationPose sourcePose_;
    bool hasState_ = false;
};

[[nodiscard]] glm::vec3 sampleVectorKeys(const std::vector<VectorKeyframe>& keys, float time, bool step);
[[nodiscard]] glm::quat sampleRotationKeys(const std::vector<RotationKeyframe>& keys, float time, bool step);
[[nodiscard]] AnimationPose evaluatePose(const Skeleton& skeleton, const AnimationClip* clip, float time);
[[nodiscard]] AnimationPose blendPoses(const AnimationPose& source, const AnimationPose& target, float alpha);
void dispatchAnimationEvents(const AnimationClip& clip, float previousTime, float deltaSeconds,
    const AnimationEventCallback& callback);
[[nodiscard]] std::vector<glm::mat4> evaluateSkinPalette(const Skeleton& skeleton, const AnimationPose& pose);
[[nodiscard]] std::vector<glm::mat4> evaluateSkinPalette(
    const Skeleton& skeleton, const AnimationClip* clip, float time);

} // namespace glory::engine

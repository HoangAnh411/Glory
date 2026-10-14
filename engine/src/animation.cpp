#include "glory/engine/animation.hpp"

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <stdexcept>

namespace glory::engine {
namespace {

template <typename Key>
std::size_t findNextKey(const std::vector<Key>& keys, float time) {
    const auto next = std::upper_bound(keys.begin(), keys.end(), time,
        [](float value, const Key& key) { return value < key.time; });
    return static_cast<std::size_t>(std::distance(keys.begin(), next));
}

glm::mat4 composeTransform(const SkeletonNode& node) {
    return glm::translate(glm::mat4(1.0F), node.translation) *
        glm::mat4_cast(glm::normalize(node.rotation)) *
        glm::scale(glm::mat4(1.0F), node.scale);
}

} // namespace

void Animator::setTime(float time, float duration) noexcept {
    if (!std::isfinite(time) || !std::isfinite(duration) || duration <= 0.0F) {
        time_ = 0.0F;
        return;
    }
    time_ = std::clamp(time, 0.0F, duration);
}

void Animator::advance(float deltaSeconds, float duration) noexcept {
    if (!playing_ || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F ||
        !std::isfinite(duration) || duration <= 0.0F) return;
    time_ = std::fmod(time_ + deltaSeconds, duration);
}

void AnimationStateMachine::setFadeDuration(float seconds) noexcept {
    if (std::isfinite(seconds)) fadeDuration_ = std::clamp(seconds, 0.0F, 10.0F);
}

bool AnimationStateMachine::trigger(const std::string& state, const Skeleton& skeleton,
    const std::vector<AnimationClip>& clips, const AnimationPose* initialPose) {
    const auto found = std::find_if(clips.begin(), clips.end(), [&](const AnimationClip& clip) {
        return clip.name == state;
    });
    if (found == clips.end()) return false;
    const std::size_t nextClip = static_cast<std::size_t>(std::distance(clips.begin(), found));
    if (hasState_ && nextClip == targetClip_) return true;
    sourcePose_ = hasState_ ? pose(skeleton, clips) :
        (initialPose != nullptr ? *initialPose : evaluatePose(skeleton, nullptr, 0.0F));
    targetClip_ = nextClip;
    targetTime_ = 0.0F;
    fadeElapsed_ = 0.0F;
    blendFactor_ = fadeDuration_ <= 0.0F ? 1.0F : 0.0F;
    stateName_ = state;
    hasState_ = true;
    return true;
}

void AnimationStateMachine::setTime(float time, const std::vector<AnimationClip>& clips) noexcept {
    if (!hasState_ || targetClip_ >= clips.size() || !std::isfinite(time)) return;
    const float duration = clips[targetClip_].duration;
    targetTime_ = std::isfinite(duration) && duration > 0.0F ? std::clamp(time, 0.0F, duration) : 0.0F;
    sourcePose_.clear();
    blendFactor_ = 1.0F;
}

void AnimationStateMachine::advance(float deltaSeconds, const Skeleton& skeleton,
    const std::vector<AnimationClip>& clips, const AnimationEventCallback& callback) {
    if (!hasState_ || targetClip_ >= clips.size() || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) return;
    const AnimationClip& clip = clips[targetClip_];
    dispatchAnimationEvents(clip, targetTime_, deltaSeconds, callback);
    if (std::isfinite(clip.duration) && clip.duration > 0.0F) {
        targetTime_ = std::fmod(targetTime_ + deltaSeconds, clip.duration);
    }
    if (blendFactor_ < 1.0F) {
        fadeElapsed_ += deltaSeconds;
        blendFactor_ = fadeDuration_ <= 0.0F ? 1.0F : std::clamp(fadeElapsed_ / fadeDuration_, 0.0F, 1.0F);
        if (blendFactor_ >= 1.0F) sourcePose_.clear();
    }
}

AnimationPose AnimationStateMachine::pose(const Skeleton& skeleton,
    const std::vector<AnimationClip>& clips) const {
    if (!hasState_ || targetClip_ >= clips.size()) return evaluatePose(skeleton, nullptr, 0.0F);
    const AnimationPose target = evaluatePose(skeleton, &clips[targetClip_], targetTime_);
    if (blendFactor_ >= 1.0F || sourcePose_.empty()) return target;
    return blendPoses(sourcePose_, target, blendFactor_);
}

glm::vec3 sampleVectorKeys(const std::vector<VectorKeyframe>& keys, float time, bool step) {
    if (keys.empty()) return glm::vec3(0.0F);
    if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const std::size_t nextIndex = findNextKey(keys, time);
    const auto& previous = keys[nextIndex - 1];
    const auto& next = keys[nextIndex];
    if (step) return previous.value;
    const float span = next.time - previous.time;
    const float amount = span > 0.0F ? (time - previous.time) / span : 0.0F;
    return glm::mix(previous.value, next.value, amount);
}

glm::quat sampleRotationKeys(const std::vector<RotationKeyframe>& keys, float time, bool step) {
    if (keys.empty()) return glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    if (keys.size() == 1 || time <= keys.front().time) return glm::normalize(keys.front().value);
    if (time >= keys.back().time) return glm::normalize(keys.back().value);
    const std::size_t nextIndex = findNextKey(keys, time);
    const auto& previous = keys[nextIndex - 1];
    const auto& next = keys[nextIndex];
    if (step) return glm::normalize(previous.value);
    const float span = next.time - previous.time;
    const float amount = span > 0.0F ? (time - previous.time) / span : 0.0F;
    glm::quat target = next.value;
    if (glm::dot(previous.value, target) < 0.0F) target = -target;
    return glm::normalize(glm::slerp(previous.value, target, amount));
}

AnimationPose evaluatePose(const Skeleton& skeleton, const AnimationClip* clip, float time) {
    AnimationPose pose = skeleton.nodes;
    if (clip == nullptr) return pose;
    for (const AnimationChannel& channel : clip->channels) {
        if (channel.nodeIndex >= pose.size()) throw std::invalid_argument("Animation channel references an unknown skeleton node");
        SkeletonNode& node = pose[channel.nodeIndex];
        if (!channel.translations.empty()) node.translation = sampleVectorKeys(channel.translations, time, channel.translationStep);
        if (!channel.rotations.empty()) node.rotation = sampleRotationKeys(channel.rotations, time, channel.rotationStep);
        if (!channel.scales.empty()) node.scale = sampleVectorKeys(channel.scales, time, channel.scaleStep);
    }
    return pose;
}

AnimationPose blendPoses(const AnimationPose& source, const AnimationPose& target, float alpha) {
    if (source.size() != target.size()) throw std::invalid_argument("Cannot blend poses with different node counts");
    const float amount = std::isfinite(alpha) ? std::clamp(alpha, 0.0F, 1.0F) : 0.0F;
    AnimationPose result = source;
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i].translation = glm::mix(source[i].translation, target[i].translation, amount);
        result[i].scale = glm::mix(source[i].scale, target[i].scale, amount);
        glm::quat targetRotation = target[i].rotation;
        if (glm::dot(source[i].rotation, targetRotation) < 0.0F) targetRotation = -targetRotation;
        result[i].rotation = glm::normalize(glm::slerp(source[i].rotation, targetRotation, amount));
    }
    return result;
}

void dispatchAnimationEvents(const AnimationClip& clip, float previousTime, float deltaSeconds,
    const AnimationEventCallback& callback) {
    if (!callback || !std::isfinite(previousTime) || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F ||
        !std::isfinite(clip.duration) || clip.duration <= 0.0F) return;
    const double start = std::max(0.0, static_cast<double>(previousTime));
    const double end = start + static_cast<double>(deltaSeconds);
    const double duration = clip.duration;
    if (end / duration >= static_cast<double>(std::numeric_limits<long long>::max())) return;
    std::vector<std::pair<double, const AnimationEvent*>> crossed;
    for (const AnimationEvent& event : clip.events) {
        if (!std::isfinite(event.time) || event.time < 0.0F || event.time > clip.duration) continue;
        const double eventTime = event.time;
        const auto firstCycle = static_cast<long long>(std::floor((start - eventTime) / duration)) + 1;
        const auto lastCycle = static_cast<long long>(std::floor((end - eventTime) / duration));
        for (long long cycle = firstCycle; cycle <= lastCycle; ++cycle) {
            crossed.emplace_back(eventTime + static_cast<double>(cycle) * duration, &event);
        }
    }
    std::stable_sort(crossed.begin(), crossed.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
    for (const auto& occurrence : crossed) callback(*occurrence.second);
}

std::vector<glm::mat4> evaluateSkinPalette(const Skeleton& skeleton, const AnimationPose& pose) {
    if (skeleton.jointNodeIndices.size() != skeleton.inverseBindMatrices.size()) {
        throw std::invalid_argument("Skeleton joint mapping and inverse-bind matrix counts differ");
    }
    if (pose.size() != skeleton.nodes.size()) throw std::invalid_argument("Pose node count does not match skeleton");
    std::vector<glm::mat4> global(pose.size(), glm::mat4(1.0F));
    std::vector<std::uint8_t> state(pose.size(), 0);
    std::function<void(std::size_t)> evaluateNode = [&](std::size_t index) {
        if (state[index] == 2) return;
        if (state[index] == 1) throw std::invalid_argument("Skeleton hierarchy contains a cycle");
        state[index] = 1;
        const std::int32_t parent = pose[index].parent;
        if (parent >= 0) {
            if (static_cast<std::size_t>(parent) >= pose.size()) throw std::invalid_argument("Skeleton parent index is invalid");
            evaluateNode(static_cast<std::size_t>(parent));
            global[index] = global[static_cast<std::size_t>(parent)] * composeTransform(pose[index]);
        } else {
            global[index] = composeTransform(pose[index]);
        }
        state[index] = 2;
    };
    for (std::size_t i = 0; i < pose.size(); ++i) evaluateNode(i);

    std::vector<glm::mat4> palette;
    palette.reserve(skeleton.jointNodeIndices.size());
    for (std::size_t i = 0; i < skeleton.jointNodeIndices.size(); ++i) {
        const std::uint32_t nodeIndex = skeleton.jointNodeIndices[i];
        if (nodeIndex >= global.size()) throw std::invalid_argument("Skin joint references an unknown skeleton node");
        palette.push_back(global[nodeIndex] * skeleton.inverseBindMatrices[i]);
    }
    return palette;
}

std::vector<glm::mat4> evaluateSkinPalette(const Skeleton& skeleton, const AnimationClip* clip, float time) {
    return evaluateSkinPalette(skeleton, evaluatePose(skeleton, clip, time));
}

} // namespace glory::engine

#include "glory/engine/animation.hpp"
#include "glory/engine/gltf_asset.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void requireNear(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 1.0e-4F) throw std::runtime_error(message);
}
}

int main() {
    try {
        using namespace glory::engine;
        const glm::vec3 midpoint = sampleVectorKeys({{0.0F, {0.0F, 0.0F, 0.0F}}, {2.0F, {4.0F, 2.0F, 0.0F}}}, 1.0F, false);
        requireNear(midpoint.x, 2.0F, "translation key interpolation failed");

        const glm::quat quarterTurn(0.70710678F, 0.0F, 0.0F, 0.70710678F);
        const glm::quat halfTurn = sampleRotationKeys({{0.0F, glm::quat(1, 0, 0, 0)}, {1.0F, quarterTurn}}, 0.5F, false);
        const glm::vec3 rotated = halfTurn * glm::vec3(1.0F, 0.0F, 0.0F);
        requireNear(rotated.x, 0.70710678F, "quaternion slerp X component failed");
        requireNear(rotated.y, 0.70710678F, "quaternion slerp Y component failed");
        requireNear(sampleVectorKeys({{0.0F, {1, 0, 0}}, {1.0F, {3, 0, 0}}}, 0.75F, true).x, 1.0F, "STEP interpolation failed");

        AnimationPose poseA{{"joint", -1, {0, 0, 0}, glm::quat(1, 0, 0, 0), {1, 1, 1}}};
        AnimationPose poseB{{"joint", -1, {4, 2, 0}, glm::quat(0.70710678F, 0, 0, 0.70710678F), {3, 3, 3}}};
        const auto atStart = blendPoses(poseA, poseB, 0.0F);
        const auto halfway = blendPoses(poseA, poseB, 0.5F);
        const auto atEnd = blendPoses(poseA, poseB, 1.0F);
        requireNear(atStart[0].translation.x, 0.0F, "pose blend alpha 0 did not return source");
        requireNear(halfway[0].translation.x, 2.0F, "pose blend midpoint translation is incorrect");
        requireNear(halfway[0].scale.x, 2.0F, "pose blend midpoint scale is incorrect");
        requireNear(atEnd[0].translation.x, 4.0F, "pose blend alpha 1 did not return target");
        requireNear(glm::length(halfway[0].rotation), 1.0F, "blended rotation is not normalized");
        poseB[0].rotation = -poseB[0].rotation;
        requireNear(glm::abs(glm::dot(blendPoses(poseA, poseB, 0.5F)[0].rotation, halfway[0].rotation)),
            1.0F, "quaternion sign correction did not preserve the shortest rotation");

        AnimationClip idle{"Idle", 1.0F};
        idle.channels.push_back({0, false, false, false, {{0.0F, {0, 0, 0}}, {1.0F, {0, 0, 0}}}, {}, {}});
        AnimationClip run{"Run", 1.0F};
        run.channels.push_back({0, false, false, false, {{0.0F, {2, 0, 0}}, {1.0F, {2, 0, 0}}}, {}, {}});
        AnimationClip attack{"Attack", 1.0F};
        attack.channels.push_back({0, false, false, false, {{0.0F, {4, 0, 0}}, {1.0F, {4, 0, 0}}}, {}, {}});
        attack.events = {{0.3F, "hit_frame", {{"damage", "10"}}}, {0.6F, "swing_sound", {}}};
        const std::vector<AnimationClip> stateClips{idle, run, attack};

        AnimationClip eventClip = attack;
        std::vector<std::string> fired;
        const auto listen = [&](const AnimationEvent& event) { fired.push_back(event.name); };
        dispatchAnimationEvents(eventClip, 0.0F, 0.2F, listen);
        if (!fired.empty()) throw std::runtime_error("animation event fired before its timestamp");
        dispatchAnimationEvents(eventClip, 0.2F, 0.2F, listen);
        if (fired.size() != 1 || fired.back() != "hit_frame") throw std::runtime_error("animation event was not fired on timestamp crossing");
        fired.clear();
        dispatchAnimationEvents(eventClip, 0.8F, 0.5F, listen);
        if (fired.size() != 1 || fired.front() != "hit_frame") throw std::runtime_error("loop wrap-around event was not fired");

        Skeleton skeleton;
        skeleton.nodes = {{"root", -1, {1, 0, 0}}, {"joint", 0, {0, 2, 0}}};
        skeleton.jointNodeIndices = {1};
        skeleton.inverseBindMatrices = {glm::mat4(1.0F)};
        AnimationClip clip;
        clip.name = "hierarchy";
        clip.channels.push_back({1, false, false, false, {{0.0F, {0, 2, 0}}, {1.0F, {0, 4, 0}}}, {}, {}});
        const auto palette = evaluateSkinPalette(skeleton, &clip, 0.5F);
        requireNear(palette[0][3].x, 1.0F, "parent transform was not accumulated");
        requireNear(palette[0][3].y, 3.0F, "animated child transform was not applied");

        Skeleton stateSkeleton;
        stateSkeleton.nodes = {{"root", -1, {0, 0, 0}}};
        stateSkeleton.jointNodeIndices = {0};
        stateSkeleton.inverseBindMatrices = {glm::mat4(1.0F)};
        AnimationStateMachine states;
        states.setFadeDuration(0.2F);
        if (!states.trigger("Idle", stateSkeleton, stateClips)) throw std::runtime_error("Idle state trigger failed");
        states.advance(0.1F, stateSkeleton, stateClips, {});
        if (!states.trigger("Run", stateSkeleton, stateClips)) throw std::runtime_error("Run state trigger failed");
        requireNear(states.blendFactor(), 0.0F, "state transition did not begin at source pose");
        states.advance(0.1F, stateSkeleton, stateClips, {});
        requireNear(states.blendFactor(), 0.5F, "state blend factor did not advance with fade time");
        requireNear(states.pose(stateSkeleton, stateClips)[0].translation.x, 1.0F,
            "state machine did not blend the source and target poses");
        requireNear(evaluateSkinPalette(stateSkeleton, states.pose(stateSkeleton, stateClips))[0][3].x, 1.0F,
            "skin palette did not use the blended pose");
        states.advance(0.1F, stateSkeleton, stateClips, {});
        requireNear(states.blendFactor(), 1.0F, "state transition did not finish at fade duration");
        if (states.stateName() != "Run") throw std::runtime_error("state machine did not retain target state");
        requireNear(states.time(), 0.2F, "state machine did not expose its active clip time");
        states.setTime(0.6F, stateClips);
        requireNear(states.time(), 0.6F, "state machine timeline scrub did not update active state time");

        Animator animator;
        animator.advance(2.25F, 1.0F);
        requireNear(animator.time(), 0.25F, "animation playback did not loop");
        animator.setPlaying(false);
        animator.advance(0.5F, 1.0F);
        requireNear(animator.time(), 0.25F, "paused animation advanced");

        const auto asset = loadGltfAsset(std::filesystem::path(GLORY_ASSET_DIR) / "models" / "starter_crate.gltf");
        if (!asset.hasSkin || asset.skeleton.jointNodeIndices.size() != 2 || asset.animations.size() != 3 ||
            asset.animations.front().channels.empty() || asset.animations.front().duration != 1.0F) {
            throw std::runtime_error("sample animated glTF was not parsed correctly");
        }
        if (asset.animations.front().name != "Idle" || asset.animations[1].name != "Run" ||
            asset.animations[2].name != "Attack" || asset.animations[2].events.size() != 2 ||
            asset.animations[2].events.front().name != "hit_frame" ||
            asset.animations[2].events.front().parameters.at("damage") != "10") {
            throw std::runtime_error("sample Idle/Run/Attack clips or attack events were not parsed");
        }
        const auto idlePose = evaluatePose(asset.skeleton, &asset.animations[0], 0.5F);
        const auto runPose = evaluatePose(asset.skeleton, &asset.animations[1], 0.5F);
        if (glm::abs(glm::dot(idlePose[2].rotation, runPose[2].rotation)) > 0.999F) {
            throw std::runtime_error("sample Idle and Run clips do not contain distinct poses");
        }
        const auto samplePalette = evaluateSkinPalette(asset.skeleton, &asset.animations.front(), 0.5F);
        if (samplePalette.size() != 2) throw std::runtime_error("sample skin palette size is incorrect");
    } catch (const std::exception& error) {
        std::cerr << "Skeletal animation test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "CPU skeletal animation tests passed\n";
    return 0;
}

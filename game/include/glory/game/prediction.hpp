#pragma once

#include "glory/game/character_controller.hpp"
#include "glory/input/input_frame.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>

namespace glory::game {

struct AuthoritativeCharacterState {
    CharacterControllerState controller;
};

struct PredictionDebugState {
    std::uint32_t clientSequence = 0;
    std::uint32_t serverAckSequence = 0;
    std::size_t unacknowledgedInputCount = 0;
    std::uint64_t reconciliationCount = 0;
    float mispredictionOffset = 0.0F;
};

class ClientPrediction {
public:
    explicit ClientPrediction(CharacterController& controller, float correctionThreshold = 0.05F);

    std::uint32_t predict(const input::InputFrame& inputFrame, float cameraYawDegrees,
        double fixedDeltaSeconds);
    bool reconcile(std::uint32_t acknowledgedSequence, const AuthoritativeCharacterState& authoritative);
    void reset() noexcept;

    [[nodiscard]] const PredictionDebugState& debugState() const noexcept { return debugState_; }
    [[nodiscard]] std::size_t unacknowledgedInputCount() const noexcept { return inputs_.size(); }

private:
    struct BufferedInput {
        std::uint32_t sequence = 0;
        input::InputFrame frame;
        float cameraYawDegrees = 0.0F;
        double fixedDeltaSeconds = 0.0;
        glm::vec3 predictedPosition{0.0F};
    };

    void apply(const input::InputFrame& inputFrame, float cameraYawDegrees, double fixedDeltaSeconds);
    [[nodiscard]] static bool sequenceAtOrBefore(std::uint32_t sequence, std::uint32_t acknowledgement) noexcept;
    [[nodiscard]] static bool sequenceAtOrAfter(std::uint32_t sequence, std::uint32_t acknowledgement) noexcept;

    CharacterController& controller_;
    std::deque<BufferedInput> inputs_;
    std::uint32_t nextSequence_ = 0;
    bool hasAcknowledgement_ = false;
    glm::vec3 lastAcknowledgedServerPosition_{0.0F};
    float correctionThreshold_ = 0.05F;
    PredictionDebugState debugState_;
};

} // namespace glory::game

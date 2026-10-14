#include "glory/game/prediction.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace glory::game {
namespace {

constexpr std::size_t maximumBufferedInputs = 256;

} // namespace

ClientPrediction::ClientPrediction(CharacterController& controller, float correctionThreshold)
    : controller_(controller), correctionThreshold_(correctionThreshold) {
    if (!std::isfinite(correctionThreshold_) || correctionThreshold_ < 0.0F)
        throw std::invalid_argument("Invalid prediction correction threshold");
}

std::uint32_t ClientPrediction::predict(const input::InputFrame& inputFrame, float cameraYawDegrees,
    double fixedDeltaSeconds) {
    if (!std::isfinite(cameraYawDegrees) || !std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0)
        return nextSequence_;
    ++nextSequence_;
    if (nextSequence_ == 0) ++nextSequence_;
    apply(inputFrame, cameraYawDegrees, fixedDeltaSeconds);
    inputs_.push_back({nextSequence_, inputFrame, cameraYawDegrees, fixedDeltaSeconds,
        controller_.state().position});
    while (inputs_.size() > maximumBufferedInputs) inputs_.pop_front();
    debugState_.clientSequence = nextSequence_;
    debugState_.unacknowledgedInputCount = inputs_.size();
    return nextSequence_;
}

bool ClientPrediction::reconcile(std::uint32_t acknowledgedSequence,
    const AuthoritativeCharacterState& authoritative) {
    if (hasAcknowledgement_ && !sequenceAtOrAfter(acknowledgedSequence, debugState_.serverAckSequence))
        return false;

    glm::vec3 predictedAtAcknowledgement = hasAcknowledgement_ &&
        acknowledgedSequence == debugState_.serverAckSequence
        ? lastAcknowledgedServerPosition_ : controller_.state().position;
    for (const BufferedInput& input : inputs_) {
        if (input.sequence == acknowledgedSequence) {
            predictedAtAcknowledgement = input.predictedPosition;
            break;
        }
    }
    const glm::vec3 difference = predictedAtAcknowledgement - authoritative.controller.position;
    debugState_.mispredictionOffset = glm::length(difference);
    std::erase_if(inputs_, [acknowledgedSequence](const BufferedInput& input) {
        return sequenceAtOrBefore(input.sequence, acknowledgedSequence);
    });

    if (debugState_.mispredictionOffset > correctionThreshold_) {
        controller_.restoreNetworkState(authoritative.controller);
        for (BufferedInput& input : inputs_) {
            apply(input.frame, input.cameraYawDegrees, input.fixedDeltaSeconds);
            input.predictedPosition = controller_.state().position;
        }
        ++debugState_.reconciliationCount;
    }

    hasAcknowledgement_ = true;
    debugState_.serverAckSequence = acknowledgedSequence;
    lastAcknowledgedServerPosition_ = authoritative.controller.position;
    debugState_.unacknowledgedInputCount = inputs_.size();
    return true;
}

void ClientPrediction::reset() noexcept {
    inputs_.clear();
    nextSequence_ = 0;
    hasAcknowledgement_ = false;
    lastAcknowledgedServerPosition_ = glm::vec3(0.0F);
    debugState_ = {};
}

void ClientPrediction::apply(const input::InputFrame& inputFrame, float cameraYawDegrees,
    double fixedDeltaSeconds) {
    ActionMap actions;
    actions.update(inputFrame);
    controller_.update(actions, cameraYawDegrees, fixedDeltaSeconds);
}

bool ClientPrediction::sequenceAtOrBefore(std::uint32_t sequence, std::uint32_t acknowledgement) noexcept {
    return sequence == acknowledgement || static_cast<std::int32_t>(acknowledgement - sequence) > 0;
}

bool ClientPrediction::sequenceAtOrAfter(std::uint32_t sequence, std::uint32_t acknowledgement) noexcept {
    return sequence == acknowledgement || static_cast<std::int32_t>(sequence - acknowledgement) > 0;
}

} // namespace glory::game

#include "glory/game/prediction.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

glory::input::InputFrame forwardInput() {
    glory::input::InputFrame frame;
    frame.held.push_back(glory::input::Control::KeyW);
    return frame;
}

} // namespace

int main() {
    using namespace glory::game;
    try {
        constexpr double fixedDelta = 1.0 / 60.0;
        CharacterController predictedController;
        ClientPrediction prediction(predictedController);
        const auto movement = forwardInput();

        const std::uint32_t firstSequence = prediction.predict(movement, 0.0F, fixedDelta);
        require(firstSequence == 1 && predictedController.state().position.x > 0.05F,
            "client movement prediction waited for a server snapshot");
        prediction.predict(movement, 0.0F, fixedDelta);

        AuthoritativeCharacterState matchingAck;
        matchingAck.controller.position = {5.5F * static_cast<float>(fixedDelta), 0.0F, 0.0F};
        matchingAck.controller.facingYaw = 0.0F;
        require(prediction.reconcile(firstSequence, matchingAck), "valid server acknowledgement was rejected");
        require(prediction.unacknowledgedInputCount() == 1 &&
            prediction.debugState().serverAckSequence == firstSequence,
            "acknowledgement did not remove only acknowledged input frames");
        require(prediction.reconcile(firstSequence, matchingAck) &&
            prediction.debugState().reconciliationCount == 0,
            "repeated acknowledgement incorrectly compared against the current predicted position");

        CharacterController reconcileController;
        ClientPrediction reconciliation(reconcileController);
        const auto sequenceOne = reconciliation.predict(movement, 0.0F, fixedDelta);
        reconciliation.predict(movement, 0.0F, fixedDelta);
        reconciliation.predict(movement, 0.0F, fixedDelta);
        AuthoritativeCharacterState correctedServerState;
        correctedServerState.controller.position = {0.0F, 0.0F, 0.0F};
        correctedServerState.controller.facingYaw = 0.0F;
        correctedServerState.controller.externalKnockbackVelocity = {3.0F, 0.0F, 0.0F};
        correctedServerState.controller.knockbackRemaining = 0.25F;
        CharacterController expectedReplay;
        expectedReplay.restoreNetworkState(correctedServerState.controller);
        ActionMap replayActions;
        replayActions.update(movement);
        replayActions.update(movement);
        expectedReplay.update(replayActions, 0.0F, fixedDelta);
        expectedReplay.update(replayActions, 0.0F, fixedDelta);
        require(reconciliation.reconcile(sequenceOne, correctedServerState),
            "authoritative correction was rejected");
        require(reconciliation.debugState().reconciliationCount == 1 &&
            reconciliation.debugState().mispredictionOffset > 0.05F,
            "large misprediction did not trigger a reconciliation");
        require(std::abs(reconcileController.state().position.x - expectedReplay.state().position.x) < 0.001F,
            "client did not rewind to server position and replay remaining inputs");
        require(reconciliation.unacknowledgedInputCount() == 2,
            "reconciliation discarded inputs that the server had not acknowledged");

        std::cout << "Prediction/reconciliation passed: immediate movement, ACK pruning, rewind and replay\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

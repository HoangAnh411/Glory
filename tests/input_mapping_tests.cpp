#include "glory/game/action_map.hpp"

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testDefaultBindingsAndActionTransitions() {
    glory::game::ActionMap actions;
    expect(actions.binding(glory::game::Action::MoveForward) == glory::input::Control::KeyW,
           "forward is bound to W by default");
    expect(actions.binding(glory::game::Action::PrimaryAttack) == glory::input::Control::MouseLeft,
           "primary attack is bound to left mouse by default");
    expect(actions.binding(glory::game::Action::Dodge) == glory::input::Control::KeyLeftShift,
           "dodge is bound to left shift by default");

    glory::input::InputFrame frame;
    frame.pressed = {glory::input::Control::KeyW};
    frame.held = {glory::input::Control::KeyW};
    actions.update(frame);
    auto state = actions.state(glory::game::Action::MoveForward);
    expect(state.pressed && state.held && !state.released,
           "press frame sets pressed and held states");

    frame.pressed.clear();
    actions.update(frame);
    state = actions.state(glory::game::Action::MoveForward);
    expect(!state.pressed && state.held && !state.released,
           "subsequent frame preserves held state only");

    frame.held.clear();
    frame.released = {glory::input::Control::KeyW};
    actions.update(frame);
    state = actions.state(glory::game::Action::MoveForward);
    expect(!state.pressed && !state.held && state.released,
           "release frame sets released state only");
}

void testRebindingAndValidation() {
    glory::game::ActionMap actions;
    expect(actions.bind(glory::game::Action::MoveForward, glory::input::Control::KeyUp),
           "valid action can be rebound");
    expect(actions.binding(glory::game::Action::MoveForward) == glory::input::Control::KeyUp,
           "rebinding replaces previous control");

    glory::input::InputFrame frame;
    frame.pressed = {glory::input::Control::KeyW};
    frame.held = {glory::input::Control::KeyW};
    actions.update(frame);
    expect(!actions.state(glory::game::Action::MoveForward).held,
           "old control no longer activates rebound action");

    frame.pressed = {glory::input::Control::KeyUp};
    frame.held = {glory::input::Control::KeyUp};
    actions.update(frame);
    expect(actions.state(glory::game::Action::MoveForward).held,
           "new control activates rebound action");

    expect(!actions.bind(glory::game::Action::MoveForward, glory::input::Control::Unknown),
           "unknown control is rejected");
    expect(!actions.bind(glory::game::Action::Count, glory::input::Control::KeyW),
           "invalid action is rejected");
    expect(actions.binding(glory::game::Action::MoveForward) == glory::input::Control::KeyUp,
           "rejected binding leaves previous mapping intact");
    expect(actions.state(glory::game::Action::Count).held == false,
           "invalid action query returns an empty state");
}

void testSharedControlCanTriggerMultipleActions() {
    glory::game::ActionMap actions;
    expect(actions.bind(glory::game::Action::Interact, glory::input::Control::MouseLeft),
           "an existing control may intentionally be shared");
    glory::input::InputFrame frame;
    frame.pressed = {glory::input::Control::MouseLeft};
    frame.held = {glory::input::Control::MouseLeft};
    actions.update(frame);
    expect(actions.state(glory::game::Action::PrimaryAttack).pressed &&
               actions.state(glory::game::Action::Interact).pressed,
           "shared input activates both mapped actions");
}

} // namespace

int main() {
    testDefaultBindingsAndActionTransitions();
    testRebindingAndValidation();
    testSharedControlCanTriggerMultipleActions();

    if (failures != 0) {
        std::cerr << failures << " input mapping test(s) failed\n";
        return 1;
    }
    std::cout << "All input mapping tests passed\n";
    return 0;
}

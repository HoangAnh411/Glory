#include "glory/game/action_map.hpp"

#include <algorithm>

namespace glory::game {
namespace {

constexpr bool isValidAction(Action action) noexcept {
    return static_cast<std::size_t>(action) < static_cast<std::size_t>(Action::Count);
}

bool contains(const std::vector<input::Control>& controls, input::Control control) noexcept {
    return std::find(controls.begin(), controls.end(), control) != controls.end();
}

} // namespace

ActionMap::ActionMap() noexcept {
    bindings_.fill(input::Control::Unknown);
    (void)bind(Action::MoveForward, input::Control::KeyW);
    (void)bind(Action::MoveBackward, input::Control::KeyS);
    (void)bind(Action::MoveLeft, input::Control::KeyA);
    (void)bind(Action::MoveRight, input::Control::KeyD);
    (void)bind(Action::Jump, input::Control::KeySpace);
    (void)bind(Action::Dodge, input::Control::KeyLeftShift);
    (void)bind(Action::PrimaryAttack, input::Control::MouseLeft);
    (void)bind(Action::SecondaryAttack, input::Control::MouseRight);
    (void)bind(Action::Interact, input::Control::KeyE);
}

bool ActionMap::bind(Action action, input::Control control) noexcept {
    if (!isValidAction(action) || control == input::Control::Unknown) {
        return false;
    }
    bindings_[static_cast<std::size_t>(action)] = control;
    return true;
}

input::Control ActionMap::binding(Action action) const noexcept {
    if (!isValidAction(action)) {
        return input::Control::Unknown;
    }
    return bindings_[static_cast<std::size_t>(action)];
}

ActionState ActionMap::state(Action action) const noexcept {
    if (!isValidAction(action)) {
        return {};
    }
    return states_[static_cast<std::size_t>(action)];
}

void ActionMap::update(const input::InputFrame& frame) noexcept {
    for (std::size_t index = 0; index < actionCount; ++index) {
        const input::Control control = bindings_[index];
        states_[index] = {
            contains(frame.pressed, control),
            control != input::Control::Unknown && contains(frame.held, control),
            contains(frame.released, control),
        };
    }
}

} // namespace glory::game

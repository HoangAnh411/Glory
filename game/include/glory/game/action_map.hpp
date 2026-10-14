#pragma once

#include "glory/input/input_frame.hpp"

#include <array>
#include <cstddef>

namespace glory::game {

enum class Action : std::size_t {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    Dodge,
    PrimaryAttack,
    SecondaryAttack,
    Interact,
    Count,
};

struct ActionState {
    bool pressed = false;
    bool held = false;
    bool released = false;
};

class ActionMap {
public:
    ActionMap() noexcept;

    [[nodiscard]] bool bind(Action action, input::Control control) noexcept;
    [[nodiscard]] input::Control binding(Action action) const noexcept;
    [[nodiscard]] ActionState state(Action action) const noexcept;
    void update(const input::InputFrame& frame) noexcept;

private:
    static constexpr std::size_t actionCount = static_cast<std::size_t>(Action::Count);
    std::array<input::Control, actionCount> bindings_{};
    std::array<ActionState, actionCount> states_{};
};

} // namespace glory::game

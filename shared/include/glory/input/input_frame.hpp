#pragma once

#include <vector>

namespace glory::input {

enum class Control {
    Unknown,
    KeyW,
    KeyA,
    KeyS,
    KeyD,
    KeyQ,
    KeyE,
    KeySpace,
    KeyLeftControl,
    KeyRightControl,
    KeyEscape,
    KeyF1,
    KeyLeftShift,
    KeyRightShift,
    KeyUp,
    KeyDown,
    KeyLeft,
    KeyRight,
    MouseLeft,
    MouseMiddle,
    MouseRight,
};

struct InputFrame {
    std::vector<Control> pressed;
    std::vector<Control> held;
    std::vector<Control> released;
    float mouseDeltaX = 0.0F;
    float mouseDeltaY = 0.0F;
};

} // namespace glory::input

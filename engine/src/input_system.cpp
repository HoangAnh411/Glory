#include "glory/engine/input_system.hpp"

#include <algorithm>
#include <utility>

namespace glory::engine {
namespace {

input::Control translateKey(SDL_Scancode key) noexcept {
    switch (key) {
    case SDL_SCANCODE_W: return input::Control::KeyW;
    case SDL_SCANCODE_A: return input::Control::KeyA;
    case SDL_SCANCODE_S: return input::Control::KeyS;
    case SDL_SCANCODE_D: return input::Control::KeyD;
    case SDL_SCANCODE_Q: return input::Control::KeyQ;
    case SDL_SCANCODE_E: return input::Control::KeyE;
    case SDL_SCANCODE_SPACE: return input::Control::KeySpace;
    case SDL_SCANCODE_LCTRL: return input::Control::KeyLeftControl;
    case SDL_SCANCODE_RCTRL: return input::Control::KeyRightControl;
    case SDL_SCANCODE_LSHIFT: return input::Control::KeyLeftShift;
    case SDL_SCANCODE_RSHIFT: return input::Control::KeyRightShift;
    case SDL_SCANCODE_ESCAPE: return input::Control::KeyEscape;
    case SDL_SCANCODE_F1: return input::Control::KeyF1;
    case SDL_SCANCODE_UP: return input::Control::KeyUp;
    case SDL_SCANCODE_DOWN: return input::Control::KeyDown;
    case SDL_SCANCODE_LEFT: return input::Control::KeyLeft;
    case SDL_SCANCODE_RIGHT: return input::Control::KeyRight;
    case SDL_SCANCODE_1: return input::Control::Key1;
    case SDL_SCANCODE_2: return input::Control::Key2;
    case SDL_SCANCODE_3: return input::Control::Key3;
    case SDL_SCANCODE_4: return input::Control::Key4;
    case SDL_SCANCODE_TAB: return input::Control::KeyTab;
    default: return input::Control::Unknown;
    }
}

input::Control translateMouseButton(std::uint8_t button) noexcept {
    switch (button) {
    case SDL_BUTTON_LEFT: return input::Control::MouseLeft;
    case SDL_BUTTON_MIDDLE: return input::Control::MouseMiddle;
    case SDL_BUTTON_RIGHT: return input::Control::MouseRight;
    default: return input::Control::Unknown;
    }
}

void addUnique(std::vector<input::Control>& controls, input::Control control) {
    if (control != input::Control::Unknown &&
        std::find(controls.begin(), controls.end(), control) == controls.end()) {
        controls.push_back(control);
    }
}

void press(input::InputFrame& frame, input::Control control, bool repeated) {
    if (control == input::Control::Unknown) {
        return;
    }
    if (std::find(frame.held.begin(), frame.held.end(), control) == frame.held.end()) {
        frame.held.push_back(control);
        if (!repeated) {
            addUnique(frame.pressed, control);
        }
    }
}

void release(input::InputFrame& frame, input::Control control) {
    const auto found = std::find(frame.held.begin(), frame.held.end(), control);
    if (found != frame.held.end()) {
        frame.held.erase(found);
        addUnique(frame.released, control);
    }
}

} // namespace

void InputSystem::processEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        press(pendingFrame_, translateKey(event.key.scancode), event.key.repeat);
        break;
    case SDL_EVENT_KEY_UP:
        release(pendingFrame_, translateKey(event.key.scancode));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        press(pendingFrame_, translateMouseButton(event.button.button), false);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        release(pendingFrame_, translateMouseButton(event.button.button));
        break;
    case SDL_EVENT_MOUSE_MOTION:
        pendingFrame_.mouseDeltaX += event.motion.xrel;
        pendingFrame_.mouseDeltaY += event.motion.yrel;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        for (const input::Control control : pendingFrame_.held) {
            addUnique(pendingFrame_.released, control);
        }
        pendingFrame_.held.clear();
        break;
    default:
        break;
    }
}

input::InputFrame InputSystem::consumeFrame() {
    input::InputFrame frame;
    frame.pressed = std::move(pendingFrame_.pressed);
    frame.held = pendingFrame_.held;
    frame.released = std::move(pendingFrame_.released);
    frame.mouseDeltaX = pendingFrame_.mouseDeltaX;
    frame.mouseDeltaY = pendingFrame_.mouseDeltaY;
    pendingFrame_.mouseDeltaX = 0.0F;
    pendingFrame_.mouseDeltaY = 0.0F;
    return frame;
}

} // namespace glory::engine

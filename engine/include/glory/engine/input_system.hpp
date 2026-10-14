#pragma once

#include "glory/input/input_frame.hpp"

#include <SDL3/SDL_events.h>

namespace glory::engine {

class InputSystem {
public:
    void processEvent(const SDL_Event& event);
    [[nodiscard]] input::InputFrame consumeFrame();

private:
    input::InputFrame pendingFrame_;
};

} // namespace glory::engine

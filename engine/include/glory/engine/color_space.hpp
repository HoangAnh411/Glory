#pragma once

#include <algorithm>
#include <cmath>

namespace glory::engine {

inline float srgbToLinear(float channel) noexcept {
    channel = std::clamp(channel, 0.0F, 1.0F);
    return channel <= 0.04045F ? channel / 12.92F
        : std::pow((channel + 0.055F) / 1.055F, 2.4F);
}

} // namespace glory::engine

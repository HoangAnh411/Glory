#pragma once

#include <cstdint>
#include <functional>

namespace glory::engine {

struct ApplicationConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    bool hiddenWindow = false;
    std::uint64_t smokeTestTickLimit = 0;
};

using FixedUpdateCallback = std::function<void(double)>;

int runApplication(const ApplicationConfig& config, const FixedUpdateCallback& fixedUpdate);

} // namespace glory::engine

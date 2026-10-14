#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace glory::engine {

struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

ImageData decodeImage(std::span<const std::uint8_t> encoded);
ImageData loadImage(const std::filesystem::path& path);

} // namespace glory::engine

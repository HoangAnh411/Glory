#include "glory/engine/image_loader.hpp"

#include <stb_image.h>

#include <limits>
#include <memory>
#include <stdexcept>

namespace glory::engine {

ImageData decodeImage(std::span<const std::uint8_t> encoded) {
    if (encoded.empty() || encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Image data is empty or too large");
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()),
        &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(std::string("Image decode failed: ") + (reason ? reason : "unknown error"));
    }
    if (width <= 0 || height <= 0) throw std::runtime_error("Decoded image has invalid dimensions");
    const auto byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    return {width, height, std::vector<std::uint8_t>(pixels.get(), pixels.get() + byteCount)};
}

ImageData loadImage(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    const std::string nativePath = path.string();
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load(nativePath.c_str(), &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Image load failed for " + nativePath + ": " + (reason ? reason : "unknown error"));
    }
    if (width <= 0 || height <= 0) throw std::runtime_error("Decoded image has invalid dimensions: " + nativePath);
    const auto byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    return {width, height, std::vector<std::uint8_t>(pixels.get(), pixels.get() + byteCount)};
}

} // namespace glory::engine

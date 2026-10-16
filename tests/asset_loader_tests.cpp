#include "glory/engine/gltf_asset.hpp"
#include "glory/engine/image_loader.hpp"
#include "glory/engine/color_space.hpp"

#include <filesystem>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    const std::filesystem::path root = GLORY_ASSET_DIR;
    try {
        if (std::abs(glory::engine::srgbToLinear(0.5F) - 0.21404114F) > 1.0e-5F ||
            std::abs(glory::engine::srgbToLinear(0.04045F) - 0.0031308F) > 1.0e-5F)
            throw std::runtime_error("sRGB-to-linear transfer function is incorrect");
        const auto image = glory::engine::loadImage(root / "textures" / "starter_crate.png");
        if (image.width != 8 || image.height != 8 || image.rgba.size() != 8U * 8U * 4U) throw std::runtime_error("sample PNG dimensions or decoded bytes are incorrect");
        const auto asset = glory::engine::loadGltfAsset(root / "models" / "starter_crate.gltf");
        if (asset.vertices.size() != 24 || asset.indices.size() != 36 || asset.texturePath.empty() ||
            !asset.hasSkin || asset.skeleton.jointNodeIndices.size() != 2 || asset.animations.size() != 3 ||
            asset.animations.front().name != "Idle" || asset.animations.back().name != "Attack" ||
            asset.animations.back().events.size() != 2 ||
            asset.animations.back().events.front().parameters.at("damage") != "10" ||
            asset.vertices.front().weights[0] + asset.vertices.front().weights[1] < 0.99F) {
            throw std::runtime_error("sample glTF mesh/texture/skin/animation extraction is incorrect");
        }
        const auto staticAsset = glory::engine::loadGltfAsset(root / "models" / "static_crate.gltf");
        if (staticAsset.hasSkin || !staticAsset.animations.empty() || staticAsset.vertices.front().weights[0] != 1.0F) {
            throw std::runtime_error("static glTF fallback data is incorrect");
        }
        bool rejectedImage = false;
        try { const std::uint8_t bad[] = {0, 1, 2, 3}; (void)glory::engine::decodeImage(bad); }
        catch (const std::runtime_error&) { rejectedImage = true; }
        if (!rejectedImage) throw std::runtime_error("invalid image input was accepted");
        bool rejectedGltf = false;
        try { (void)glory::engine::loadGltfAsset(root / "models" / "missing.gltf"); }
        catch (const std::runtime_error&) { rejectedGltf = true; }
        if (!rejectedGltf) throw std::runtime_error("missing glTF file was accepted");
    } catch (const std::exception& error) {
        std::cerr << "Asset loader test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "CPU image and glTF loader tests passed\n";
    return 0;
}

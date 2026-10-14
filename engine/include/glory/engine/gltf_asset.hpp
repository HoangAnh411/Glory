#pragma once

#include "glory/engine/animation.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace glory::engine {

struct GltfVertex {
    float position[3]{};
    float normal[3]{};
    float texCoord[2]{};
    std::uint32_t joints[4]{};
    float weights[4]{1.0F, 0.0F, 0.0F, 0.0F};
};

struct GltfAsset {
    std::vector<GltfVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::filesystem::path texturePath;
    std::vector<std::uint8_t> embeddedTexture;
    int embeddedTextureMime = 0; // 0 PNG, 1 JPEG
    float metallicFactor = 1.0F;
    float roughnessFactor = 1.0F;
    float ambientOcclusion = 1.0F;
    float baseColorFactor[4]{1.0F, 1.0F, 1.0F, 1.0F};
    bool hasSkin = false;
    Skeleton skeleton;
    std::vector<AnimationClip> animations;
};

GltfAsset loadGltfAsset(const std::filesystem::path& path);

} // namespace glory::engine

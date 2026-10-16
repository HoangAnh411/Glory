#include "glory/engine/gltf_asset.hpp"
#include "glory/engine/color_space.hpp"

#include <cgltf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace glory::engine {
namespace {

struct DataDeleter { void operator()(cgltf_data* data) const { cgltf_free(data); } };

glm::mat4 readMatrix(const cgltf_accessor* accessor, cgltf_size element) {
    float values[16]{};
    if (!cgltf_accessor_read_float(accessor, element, values, 16)) {
        throw std::runtime_error("Failed to read glTF inverse bind matrix");
    }
    glm::mat4 matrix(1.0F);
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            matrix[column][row] = values[column * 4 + row];
    return matrix;
}

SkeletonNode readNode(const cgltf_data* data, cgltf_size index) {
    const cgltf_node& source = data->nodes[index];
    SkeletonNode node;
    node.name = source.name != nullptr ? source.name : ("node_" + std::to_string(index));
    if (source.parent != nullptr) node.parent = static_cast<std::int32_t>(cgltf_node_index(data, source.parent));
    if (source.has_matrix) {
        glm::mat4 matrix(1.0F);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                matrix[column][row] = source.matrix[column * 4 + row];
        glm::vec3 skew;
        glm::vec4 perspective;
        if (!glm::decompose(matrix, node.scale, node.rotation, node.translation, skew, perspective)) {
            throw std::runtime_error("Unable to decompose glTF node matrix");
        }
        node.rotation = glm::normalize(node.rotation);
    } else {
        if (source.has_translation) node.translation = {source.translation[0], source.translation[1], source.translation[2]};
        if (source.has_rotation) node.rotation = glm::normalize(glm::quat(source.rotation[3], source.rotation[0], source.rotation[1], source.rotation[2]));
        if (source.has_scale) node.scale = {source.scale[0], source.scale[1], source.scale[2]};
    }
    return node;
}

AnimationChannel& channelFor(AnimationClip& clip, std::uint32_t nodeIndex) {
    const auto found = std::find_if(clip.channels.begin(), clip.channels.end(),
        [nodeIndex](const AnimationChannel& channel) { return channel.nodeIndex == nodeIndex; });
    if (found != clip.channels.end()) return *found;
    clip.channels.push_back(AnimationChannel{});
    clip.channels.back().nodeIndex = nodeIndex;
    return clip.channels.back();
}

void readAnimations(const cgltf_data* data, GltfAsset& asset) {
    asset.animations.reserve(data->animations_count);
    for (cgltf_size animationIndex = 0; animationIndex < data->animations_count; ++animationIndex) {
        const cgltf_animation& source = data->animations[animationIndex];
        AnimationClip clip;
        clip.name = source.name != nullptr ? source.name : ("animation_" + std::to_string(animationIndex));
        for (cgltf_size channelIndex = 0; channelIndex < source.channels_count; ++channelIndex) {
            const cgltf_animation_channel& sourceChannel = source.channels[channelIndex];
            if (sourceChannel.target_node == nullptr || sourceChannel.sampler == nullptr) {
                throw std::runtime_error("glTF animation channel has no target node or sampler");
            }
            const cgltf_size nodeIndex = cgltf_node_index(data, sourceChannel.target_node);
            if (nodeIndex >= data->nodes_count) throw std::runtime_error("glTF animation target node is invalid");
            const cgltf_accessor* input = sourceChannel.sampler->input;
            const cgltf_accessor* output = sourceChannel.sampler->output;
            if (input == nullptr || output == nullptr || input->type != cgltf_type_scalar || input->count == 0 || input->count != output->count) {
                throw std::runtime_error("glTF animation sampler has invalid input/output accessors");
            }
            if (sourceChannel.sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
                throw std::runtime_error("glTF CUBICSPLINE animation is not supported yet");
            }
            AnimationChannel& channel = channelFor(clip, static_cast<std::uint32_t>(nodeIndex));
            const bool step = sourceChannel.sampler->interpolation == cgltf_interpolation_type_step;
            float previousTime = -1.0F;
            for (cgltf_size keyIndex = 0; keyIndex < input->count; ++keyIndex) {
                float timestamp = 0.0F;
                if (!cgltf_accessor_read_float(input, keyIndex, &timestamp, 1) || !std::isfinite(timestamp) || timestamp < 0.0F || timestamp <= previousTime) {
                    throw std::runtime_error("glTF animation key timestamps must be finite, nonnegative and increasing");
                }
                previousTime = timestamp;
                clip.duration = std::max(clip.duration, timestamp);
                if (sourceChannel.target_path == cgltf_animation_path_type_translation || sourceChannel.target_path == cgltf_animation_path_type_scale) {
                    if (output->type != cgltf_type_vec3) throw std::runtime_error("glTF translation/scale animation output must be VEC3");
                    float value[3]{};
                    if (!cgltf_accessor_read_float(output, keyIndex, value, 3)) throw std::runtime_error("Unable to read glTF vector animation key");
                    const VectorKeyframe key{timestamp, {value[0], value[1], value[2]}};
                    if (sourceChannel.target_path == cgltf_animation_path_type_translation) {
                        channel.translationStep = step;
                        channel.translations.push_back(key);
                    } else {
                        channel.scaleStep = step;
                        channel.scales.push_back(key);
                    }
                } else if (sourceChannel.target_path == cgltf_animation_path_type_rotation) {
                    if (output->type != cgltf_type_vec4) throw std::runtime_error("glTF rotation animation output must be VEC4");
                    float value[4]{};
                    if (!cgltf_accessor_read_float(output, keyIndex, value, 4)) throw std::runtime_error("Unable to read glTF rotation animation key");
                    channel.rotationStep = step;
                    channel.rotations.push_back({timestamp, glm::normalize(glm::quat(value[3], value[0], value[1], value[2]))});
                } else {
                    throw std::runtime_error("glTF morph-weight animation is not supported yet");
                }
            }
        }
        if (source.extras.data != nullptr) {
            const std::string extras(source.extras.data);
            const std::regex eventPattern(R"json("time"\s*:\s*([0-9]+(?:\.[0-9]+)?)\s*,\s*"name"\s*:\s*"([^"]+)")json");
            for (std::sregex_iterator eventIt(extras.begin(), extras.end(), eventPattern), end; eventIt != end; ++eventIt) {
                const float eventTime = std::stof((*eventIt)[1].str());
                if (!std::isfinite(eventTime) || eventTime < 0.0F || eventTime > clip.duration) {
                    throw std::runtime_error("glTF animation event time is outside its clip duration");
                }
                std::unordered_map<std::string, std::string> parameters;
                const std::size_t nameEnd = static_cast<std::size_t>(eventIt->position(2) + eventIt->length(2));
                const std::size_t nextEvent = extras.find("\"time\"", nameEnd);
                const std::size_t parametersStart = extras.find("\"parameters\"", nameEnd);
                if (parametersStart != std::string::npos &&
                    (nextEvent == std::string::npos || parametersStart < nextEvent)) {
                    const std::size_t objectStart = extras.find('{', parametersStart);
                    const std::size_t objectEnd = extras.find('}', objectStart);
                    if (objectStart != std::string::npos && objectEnd != std::string::npos) {
                        const std::string parameterJson = extras.substr(objectStart + 1, objectEnd - objectStart - 1);
                        const std::regex parameterPattern(R"json("([^"]+)"\s*:\s*"([^"]*)")json");
                        for (std::sregex_iterator parameterIt(parameterJson.begin(), parameterJson.end(), parameterPattern);
                             parameterIt != end; ++parameterIt) {
                            parameters.emplace((*parameterIt)[1].str(), (*parameterIt)[2].str());
                        }
                    }
                }
                clip.events.push_back({eventTime, (*eventIt)[2].str(), std::move(parameters)});
            }
        }
        asset.animations.push_back(std::move(clip));
    }
}

} // namespace

GltfAsset loadGltfAsset(const std::filesystem::path& path) {
    cgltf_options options{};
    cgltf_data* raw = nullptr;
    const std::string filename = path.string();
    const cgltf_result parseResult = cgltf_parse_file(&options, filename.c_str(), &raw);
    if (parseResult != cgltf_result_success || raw == nullptr) throw std::runtime_error("cgltf parse failed for " + filename);
    std::unique_ptr<cgltf_data, DataDeleter> data(raw);
    if (cgltf_load_buffers(&options, data.get(), filename.c_str()) != cgltf_result_success ||
        cgltf_validate(data.get()) != cgltf_result_success) {
        throw std::runtime_error("cgltf buffer load or validation failed for " + filename);
    }
    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0) throw std::runtime_error("glTF has no mesh primitive");
    const cgltf_primitive& primitive = data->meshes[0].primitives[0];
    if (primitive.type != cgltf_primitive_type_triangles) throw std::runtime_error("Only triangle glTF primitives are supported");

    const cgltf_accessor* positions = nullptr;
    const cgltf_accessor* normals = nullptr;
    const cgltf_accessor* texcoords = nullptr;
    const cgltf_accessor* colors = nullptr;
    const cgltf_accessor* joints = nullptr;
    const cgltf_accessor* weights = nullptr;
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        const auto& attribute = primitive.attributes[i];
        if (attribute.index != 0) continue;
        if (attribute.type == cgltf_attribute_type_position) positions = attribute.data;
        if (attribute.type == cgltf_attribute_type_normal) normals = attribute.data;
        if (attribute.type == cgltf_attribute_type_texcoord) texcoords = attribute.data;
        if (attribute.type == cgltf_attribute_type_color) colors = attribute.data;
        if (attribute.type == cgltf_attribute_type_joints) joints = attribute.data;
        if (attribute.type == cgltf_attribute_type_weights) weights = attribute.data;
    }
    if (positions == nullptr || positions->type != cgltf_type_vec3 || positions->count == 0) throw std::runtime_error("glTF primitive is missing valid POSITION data");
    if ((normals != nullptr && (normals->type != cgltf_type_vec3 || normals->count != positions->count)) ||
        (texcoords != nullptr && (texcoords->type != cgltf_type_vec2 || texcoords->count != positions->count))) {
        throw std::runtime_error("glTF NORMAL or TEXCOORD_0 accessor does not match POSITION data");
    }
    if (colors != nullptr && ((colors->type != cgltf_type_vec3 && colors->type != cgltf_type_vec4) ||
        colors->count != positions->count)) throw std::runtime_error("glTF COLOR_0 accessor must be VEC3/VEC4 and match POSITION data");
    if ((joints == nullptr) != (weights == nullptr) ||
        (joints != nullptr && (joints->type != cgltf_type_vec4 || weights->type != cgltf_type_vec4 || joints->count != positions->count || weights->count != positions->count))) {
        throw std::runtime_error("glTF JOINTS_0 and WEIGHTS_0 must both be VEC4 with POSITION-matching counts");
    }

    GltfAsset asset;
    asset.vertices.resize(positions->count);
    for (cgltf_size i = 0; i < positions->count; ++i) {
        GltfVertex& vertex = asset.vertices[i];
        if (!cgltf_accessor_read_float(positions, i, vertex.position, 3)) throw std::runtime_error("Invalid POSITION accessor");
        if (normals != nullptr && !cgltf_accessor_read_float(normals, i, vertex.normal, 3)) throw std::runtime_error("Invalid NORMAL accessor");
        if (texcoords != nullptr && !cgltf_accessor_read_float(texcoords, i, vertex.texCoord, 2)) throw std::runtime_error("Invalid TEXCOORD_0 accessor");
        if (colors != nullptr) {
            if (!cgltf_accessor_read_float(colors, i, vertex.color, colors->type == cgltf_type_vec4 ? 4 : 3))
                throw std::runtime_error("Invalid COLOR_0 accessor");
            for (std::size_t channel = 0; channel < 3; ++channel)
                vertex.color[channel] = srgbToLinear(vertex.color[channel]);
        }
    }

    const cgltf_skin* skin = nullptr;
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
        if (data->nodes[i].mesh == &data->meshes[0] && data->nodes[i].skin != nullptr) { skin = data->nodes[i].skin; break; }
    }
    if (skin != nullptr) {
        asset.hasSkin = true;
        if (skin->joints_count == 0 || skin->joints_count > 64) throw std::runtime_error("glTF skin must contain between 1 and 64 joints");
        asset.skeleton.nodes.reserve(data->nodes_count);
        for (cgltf_size i = 0; i < data->nodes_count; ++i) asset.skeleton.nodes.push_back(readNode(data.get(), i));
        asset.skeleton.jointNodeIndices.reserve(skin->joints_count);
        asset.skeleton.inverseBindMatrices.assign(skin->joints_count, glm::mat4(1.0F));
        for (cgltf_size i = 0; i < skin->joints_count; ++i) {
            const cgltf_size nodeIndex = cgltf_node_index(data.get(), skin->joints[i]);
            if (nodeIndex >= data->nodes_count) throw std::runtime_error("glTF skin references an invalid joint node");
            asset.skeleton.jointNodeIndices.push_back(static_cast<std::uint32_t>(nodeIndex));
        }
        if (skin->inverse_bind_matrices != nullptr) {
            if (skin->inverse_bind_matrices->type != cgltf_type_mat4 || skin->inverse_bind_matrices->count != skin->joints_count) throw std::runtime_error("glTF inverse bind matrix accessor does not match skin joints");
            for (cgltf_size i = 0; i < skin->joints_count; ++i) asset.skeleton.inverseBindMatrices[i] = readMatrix(skin->inverse_bind_matrices, i);
        }
        if (joints == nullptr) throw std::runtime_error("Skinned glTF primitive is missing JOINTS_0/WEIGHTS_0");
        for (cgltf_size i = 0; i < positions->count; ++i) {
            cgltf_uint jointValues[4]{};
            float weightValues[4]{};
            if (!cgltf_accessor_read_uint(joints, i, jointValues, 4) || !cgltf_accessor_read_float(weights, i, weightValues, 4)) throw std::runtime_error("Unable to decode glTF vertex skin attributes");
            float totalWeight = 0.0F;
            for (int influence = 0; influence < 4; ++influence) {
                if (jointValues[influence] >= skin->joints_count || !std::isfinite(weightValues[influence]) || weightValues[influence] < 0.0F) throw std::runtime_error("Invalid glTF joint index or weight");
                asset.vertices[i].joints[influence] = jointValues[influence];
                asset.vertices[i].weights[influence] = weightValues[influence];
                totalWeight += weightValues[influence];
            }
            if (!std::isfinite(totalWeight) || totalWeight <= 0.0F) throw std::runtime_error("glTF vertex skin weights must have a positive finite sum");
            for (float& weight : asset.vertices[i].weights) weight /= totalWeight;
        }
    } else if (joints != nullptr) {
        throw std::runtime_error("glTF primitive has joint attributes but no node skin");
    }

    if (primitive.indices != nullptr) {
        asset.indices.resize(primitive.indices->count);
        for (cgltf_size i = 0; i < primitive.indices->count; ++i) {
            const cgltf_size index = cgltf_accessor_read_index(primitive.indices, i);
            if (index >= asset.vertices.size()) throw std::runtime_error("glTF index is outside POSITION data");
            asset.indices[i] = static_cast<std::uint32_t>(index);
        }
    } else {
        asset.indices.resize(asset.vertices.size());
        for (std::size_t i = 0; i < asset.indices.size(); ++i) asset.indices[i] = static_cast<std::uint32_t>(i);
    }

    if (primitive.material != nullptr) {
        const cgltf_material& material = *primitive.material;
        const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
        asset.metallicFactor = std::clamp(pbr.metallic_factor, 0.0F, 1.0F);
        asset.roughnessFactor = std::clamp(pbr.roughness_factor, 0.04F, 1.0F);
        asset.ambientOcclusion = material.occlusion_texture.texture != nullptr
            ? std::clamp(material.occlusion_texture.scale, 0.0F, 1.0F) : 1.0F;
        for (std::size_t channel = 0; channel < 4; ++channel) asset.baseColorFactor[channel] = pbr.base_color_factor[channel];
        for (std::size_t channel = 0; channel < 3; ++channel) {
            asset.baseColorFactor[channel] = srgbToLinear(asset.baseColorFactor[channel]);
            asset.emissiveFactor[channel] = srgbToLinear(material.emissive_factor[channel]);
        }
    }
    const cgltf_texture* texture = primitive.material != nullptr ? primitive.material->pbr_metallic_roughness.base_color_texture.texture : nullptr;
    if (texture != nullptr && texture->image != nullptr) {
        const cgltf_image& image = *texture->image;
        if (image.uri != nullptr) {
            if (std::string_view(image.uri).starts_with("data:")) throw std::runtime_error("Embedded data URI textures are not supported");
            asset.texturePath = path.parent_path() / image.uri;
        } else if (image.buffer_view != nullptr) {
            const auto* begin = cgltf_buffer_view_data(image.buffer_view);
            if (begin == nullptr || image.buffer_view->size == 0) throw std::runtime_error("glTF embedded texture buffer is unavailable");
            asset.embeddedTexture.assign(begin, begin + image.buffer_view->size);
            asset.embeddedTextureMime = image.mime_type != nullptr && std::string_view(image.mime_type) == "image/jpeg" ? 1 : 0;
        }
    }
    if (asset.texturePath.empty() && asset.embeddedTexture.empty()) throw std::runtime_error("glTF has no supported base-color texture");
    if (data->animations_count != 0) {
        if (!asset.hasSkin) throw std::runtime_error("glTF animation currently requires a skinned mesh");
        readAnimations(data.get(), asset);
    }
    return asset;
}

} // namespace glory::engine

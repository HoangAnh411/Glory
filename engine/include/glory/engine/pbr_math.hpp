#pragma once

#include <glm/vec3.hpp>

namespace glory::engine::pbr {

[[nodiscard]] glm::vec3 dielectricMetallicF0(const glm::vec3& albedo, float metallic) noexcept;
[[nodiscard]] glm::vec3 fresnelSchlick(float cosine, const glm::vec3& f0) noexcept;
[[nodiscard]] float distributionGgx(float normalHalfway, float roughness) noexcept;
[[nodiscard]] float geometrySchlickGgx(float normalDirection, float roughness) noexcept;
[[nodiscard]] float geometrySmith(float normalView, float normalLight, float roughness) noexcept;

} // namespace glory::engine::pbr

#pragma once

#include "glory/engine/gl_api.hpp"
#include "glory/engine/directional_light.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include <string_view>

namespace glory::engine {

class ShaderProgram {
public:
    ShaderProgram(GlApi& gl, std::string_view vertexSource, std::string_view fragmentSource);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    void use() const noexcept;
    void setViewProjection(const glm::mat4& matrix) const noexcept;
    void setModelMatrix(const glm::mat4& matrix) const noexcept;
    void setTextureUnit(int unit) const noexcept;
    void setShadowMapUnit(int unit) const noexcept;
    void setEnvironmentMapUnit(int unit) const noexcept;
    void setPbrMaterial(float metallic, float roughness, float ao) const noexcept;
    void setBaseColorFactor(const glm::vec3& color) const noexcept;
    void setEmissiveFactor(const glm::vec3& color) const noexcept;
    void setEncodeSrgb(bool enabled) const noexcept;
    void setLightSpaceMatrix(const glm::mat4& matrix) const noexcept;
    void setShadowSettings(bool enabled, float bias) const noexcept;
    void setBaseColor(const glm::vec3& color) const noexcept;
    void useTexture(bool enabled) const noexcept;
    void setLighting(const DirectionalLight& light, const glm::vec3& viewPosition) const noexcept;
    void setSkinPalette(const std::vector<glm::mat4>& matrices, bool hasSkin) const noexcept;
    [[nodiscard]] GLuint program() const noexcept { return program_; }

private:
    GlApi& gl_;
    GLuint program_ = 0;
    GLint viewProjectionLocation_ = -1;
    GLint modelMatrixLocation_ = -1;
    GLint textureLocation_ = -1;
    GLint lightDirectionLocation_ = -1;
    GLint lightColorLocation_ = -1;
    GLint ambientStrengthLocation_ = -1;
    GLint diffuseStrengthLocation_ = -1;
    GLint specularStrengthLocation_ = -1;
    GLint shininessLocation_ = -1;
    GLint viewPositionLocation_ = -1;
    GLint jointMatricesLocation_ = -1;
    GLint hasSkinLocation_ = -1;
    GLint shadowMapLocation_ = -1;
    GLint lightSpaceMatrixLocation_ = -1;
    GLint shadowEnabledLocation_ = -1;
    GLint shadowBiasLocation_ = -1;
    GLint useTextureLocation_ = -1;
    GLint baseColorLocation_ = -1;
    GLint environmentMapLocation_ = -1;
    GLint metallicLocation_ = -1;
    GLint roughnessLocation_ = -1;
    GLint aoLocation_ = -1;
    GLint baseColorFactorLocation_ = -1;
    GLint emissiveFactorLocation_ = -1;
    GLint encodeSrgbLocation_ = -1;
};

} // namespace glory::engine

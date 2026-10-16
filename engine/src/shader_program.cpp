#include "glory/engine/shader_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace glory::engine {
namespace {

class ShaderHandle {
public:
    ShaderHandle(GlApi& gl, GLenum type, std::string_view source, const char* stage)
        : gl_(gl), shader_(gl_.createShader(type)) {
        if (shader_ == 0) {
            throw std::runtime_error(std::string("Failed to create ") + stage + " shader");
        }

        try {
            const GLchar* sourceText = source.data();
            const GLint sourceLength = static_cast<GLint>(source.size());
            gl_.shaderSource(shader_, 1, &sourceText, &sourceLength);
            gl_.compileShader(shader_);

            GLint compiled = GL_FALSE;
            gl_.getShaderiv(shader_, GL_COMPILE_STATUS, &compiled);
            if (compiled != GL_TRUE) {
                throw std::runtime_error(std::string("Failed to compile ") + stage +
                                         " shader:\n" + shaderLog());
            }
        } catch (...) {
            gl_.deleteShader(shader_);
            shader_ = 0;
            throw;
        }
    }

    ~ShaderHandle() {
        if (shader_ != 0) {
            gl_.deleteShader(shader_);
        }
    }

    ShaderHandle(const ShaderHandle&) = delete;
    ShaderHandle& operator=(const ShaderHandle&) = delete;

    [[nodiscard]] GLuint get() const noexcept { return shader_; }

private:
    [[nodiscard]] std::string shaderLog() const {
        GLint logLength = 0;
        gl_.getShaderiv(shader_, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength <= 1) {
            return "<driver returned no shader log>";
        }

        std::vector<GLchar> log(static_cast<std::size_t>(logLength));
        GLsizei written = 0;
        gl_.getShaderInfoLog(shader_, logLength, &written, log.data());
        return std::string(log.data(), static_cast<std::size_t>(written));
    }

    GlApi& gl_;
    GLuint shader_ = 0;
};

std::string programLog(GlApi& gl, GLuint program) {
    GLint logLength = 0;
    gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 1) {
        return "<driver returned no program log>";
    }

    std::vector<GLchar> log(static_cast<std::size_t>(logLength));
    GLsizei written = 0;
    gl.getProgramInfoLog(program, logLength, &written, log.data());
    return std::string(log.data(), static_cast<std::size_t>(written));
}

} // namespace

ShaderProgram::ShaderProgram(
    GlApi& gl,
    std::string_view vertexSource,
    std::string_view fragmentSource)
    : gl_(gl) {
    ShaderHandle vertex(gl_, GL_VERTEX_SHADER, vertexSource, "vertex");
    ShaderHandle fragment(gl_, GL_FRAGMENT_SHADER, fragmentSource, "fragment");

    program_ = gl_.createProgram();
    if (program_ == 0) {
        throw std::runtime_error("Failed to create OpenGL shader program");
    }

    try {
        gl_.attachShader(program_, vertex.get());
        gl_.attachShader(program_, fragment.get());
        gl_.linkProgram(program_);

        GLint linked = GL_FALSE;
        gl_.getProgramiv(program_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            throw std::runtime_error("Failed to link OpenGL shader program:\n" +
                                     programLog(gl_, program_));
        }
        viewProjectionLocation_ = gl_.getUniformLocation(program_, "u_ViewProjection");
        if (viewProjectionLocation_ < 0) {
            throw std::runtime_error("Shader uniform u_ViewProjection was not found");
        }
        modelMatrixLocation_ = gl_.getUniformLocation(program_, "u_Model");
        textureLocation_ = gl_.getUniformLocation(program_, "u_Texture");
        lightDirectionLocation_ = gl_.getUniformLocation(program_, "u_LightDirection");
        lightColorLocation_ = gl_.getUniformLocation(program_, "u_LightColor");
        ambientStrengthLocation_ = gl_.getUniformLocation(program_, "u_AmbientStrength");
        diffuseStrengthLocation_ = gl_.getUniformLocation(program_, "u_DiffuseStrength");
        specularStrengthLocation_ = gl_.getUniformLocation(program_, "u_SpecularStrength");
        shininessLocation_ = gl_.getUniformLocation(program_, "u_Shininess");
        viewPositionLocation_ = gl_.getUniformLocation(program_, "u_ViewPosition");
        jointMatricesLocation_ = gl_.getUniformLocation(program_, "u_JointMatrices[0]");
        hasSkinLocation_ = gl_.getUniformLocation(program_, "u_HasSkin");
        shadowMapLocation_ = gl_.getUniformLocation(program_, "u_ShadowMap");
        lightSpaceMatrixLocation_ = gl_.getUniformLocation(program_, "u_LightSpaceMatrix");
        shadowEnabledLocation_ = gl_.getUniformLocation(program_, "u_ShadowEnabled");
        shadowBiasLocation_ = gl_.getUniformLocation(program_, "u_ShadowBias");
        useTextureLocation_ = gl_.getUniformLocation(program_, "u_UseTexture");
        baseColorLocation_ = gl_.getUniformLocation(program_, "u_BaseColor");
        environmentMapLocation_ = gl_.getUniformLocation(program_, "u_EnvironmentMap");
        metallicLocation_ = gl_.getUniformLocation(program_, "u_Metallic");
        roughnessLocation_ = gl_.getUniformLocation(program_, "u_Roughness");
        aoLocation_ = gl_.getUniformLocation(program_, "u_AO");
        baseColorFactorLocation_ = gl_.getUniformLocation(program_, "u_BaseColorFactor");
        emissiveFactorLocation_ = gl_.getUniformLocation(program_, "u_EmissiveFactor");
        encodeSrgbLocation_ = gl_.getUniformLocation(program_, "u_EncodeSrgb");
    } catch (...) {
        gl_.deleteProgram(program_);
        program_ = 0;
        throw;
    }
}

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) {
        gl_.deleteProgram(program_);
    }
}

void ShaderProgram::use() const noexcept {
    gl_.useProgram(program_);
}

void ShaderProgram::setViewProjection(const glm::mat4& matrix) const noexcept {
    gl_.uniformMatrix4fv(viewProjectionLocation_, 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderProgram::setModelMatrix(const glm::mat4& matrix) const noexcept {
    if (modelMatrixLocation_ >= 0) gl_.uniformMatrix4fv(modelMatrixLocation_, 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderProgram::setTextureUnit(int unit) const noexcept {
    if (textureLocation_ >= 0) gl_.uniform1i(textureLocation_, unit);
}

void ShaderProgram::setShadowMapUnit(int unit) const noexcept {
    if (shadowMapLocation_ >= 0) gl_.uniform1i(shadowMapLocation_, unit);
}

void ShaderProgram::setEnvironmentMapUnit(int unit) const noexcept {
    if (environmentMapLocation_ >= 0) gl_.uniform1i(environmentMapLocation_, unit);
}

void ShaderProgram::setPbrMaterial(float metallic, float roughness, float ao) const noexcept {
    if (metallicLocation_ >= 0) gl_.uniform1f(metallicLocation_, metallic);
    if (roughnessLocation_ >= 0) gl_.uniform1f(roughnessLocation_, roughness);
    if (aoLocation_ >= 0) gl_.uniform1f(aoLocation_, ao);
}

void ShaderProgram::setBaseColorFactor(const glm::vec3& color) const noexcept {
    if (baseColorFactorLocation_ >= 0) gl_.uniform3f(baseColorFactorLocation_, color.x, color.y, color.z);
}
void ShaderProgram::setEmissiveFactor(const glm::vec3& color) const noexcept {
    if (emissiveFactorLocation_ >= 0) gl_.uniform3f(emissiveFactorLocation_, color.x, color.y, color.z);
}
void ShaderProgram::setEncodeSrgb(bool enabled) const noexcept {
    if (encodeSrgbLocation_ >= 0) gl_.uniform1i(encodeSrgbLocation_, enabled ? 1 : 0);
}

void ShaderProgram::setLightSpaceMatrix(const glm::mat4& matrix) const noexcept {
    if (lightSpaceMatrixLocation_ >= 0) gl_.uniformMatrix4fv(lightSpaceMatrixLocation_, 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderProgram::setShadowSettings(bool enabled, float bias) const noexcept {
    if (shadowEnabledLocation_ >= 0) gl_.uniform1i(shadowEnabledLocation_, enabled ? GL_TRUE : GL_FALSE);
    if (shadowBiasLocation_ >= 0) gl_.uniform1f(shadowBiasLocation_, bias);
}

void ShaderProgram::setBaseColor(const glm::vec3& color) const noexcept {
    if (useTextureLocation_ >= 0) gl_.uniform1i(useTextureLocation_, GL_FALSE);
    if (baseColorLocation_ >= 0) gl_.uniform3f(baseColorLocation_, color.x, color.y, color.z);
}

void ShaderProgram::useTexture(bool enabled) const noexcept {
    if (useTextureLocation_ >= 0) gl_.uniform1i(useTextureLocation_, enabled ? GL_TRUE : GL_FALSE);
}

void ShaderProgram::setLighting(const DirectionalLight& light, const glm::vec3& viewPosition) const noexcept {
    gl_.uniform3f(lightDirectionLocation_, light.direction.x, light.direction.y, light.direction.z);
    gl_.uniform3f(lightColorLocation_, light.color.x, light.color.y, light.color.z);
    gl_.uniform1f(ambientStrengthLocation_, light.ambientStrength);
    gl_.uniform1f(diffuseStrengthLocation_, light.diffuseStrength);
    gl_.uniform1f(specularStrengthLocation_, light.specularStrength);
    gl_.uniform1f(shininessLocation_, light.shininess);
    gl_.uniform3f(viewPositionLocation_, viewPosition.x, viewPosition.y, viewPosition.z);
}

void ShaderProgram::setSkinPalette(const std::vector<glm::mat4>& matrices, bool hasSkin) const noexcept {
    gl_.uniform1i(hasSkinLocation_, hasSkin ? GL_TRUE : GL_FALSE);
    if (!hasSkin || matrices.empty()) return;
    const auto count = static_cast<GLsizei>(matrices.size());
    gl_.uniformMatrix4fv(jointMatricesLocation_, count, GL_FALSE, glm::value_ptr(matrices.front()));
}

} // namespace glory::engine

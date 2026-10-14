#include "glory/engine/model_renderer.hpp"

#include "glory/engine/shadow_math.hpp"
#include "glory/engine/pbr_math.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace glory::engine {
namespace {

constexpr std::string_view depthVertexSource = R"glsl(#version 450 core
layout(location=0) in vec3 aPos;
layout(location=3) in uvec4 aJoints;
layout(location=4) in vec4 aWeights;
uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_JointMatrices[64];
uniform bool u_HasSkin;
void main(){
    mat4 skinMatrix=mat4(1.0);
    if(u_HasSkin){
        skinMatrix=aWeights.x*u_JointMatrices[aJoints.x]+aWeights.y*u_JointMatrices[aJoints.y]+
            aWeights.z*u_JointMatrices[aJoints.z]+aWeights.w*u_JointMatrices[aJoints.w];
    }
    gl_Position=u_ViewProjection*u_Model*skinMatrix*vec4(aPos,1.0);
}
)glsl";

constexpr std::string_view depthFragmentSource = R"glsl(#version 450 core
void main(){}
)glsl";

constexpr std::string_view vertexSource = R"glsl(#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in uvec4 aJoints;
layout(location=4) in vec4 aWeights;
uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_JointMatrices[64];
uniform bool u_HasSkin;
out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vFragPosLightSpace;
void main(){
    mat4 skinMatrix=mat4(1.0);
    if(u_HasSkin){
        skinMatrix=aWeights.x*u_JointMatrices[aJoints.x]+aWeights.y*u_JointMatrices[aJoints.y]+
            aWeights.z*u_JointMatrices[aJoints.z]+aWeights.w*u_JointMatrices[aJoints.w];
    }
    vec4 skinnedPos=skinMatrix*vec4(aPos,1.0);
    vec3 skinnedNormal=mat3(skinMatrix)*aNormal;
    vec4 worldPos=u_Model*skinnedPos;
    vFragPos=worldPos.xyz;
    vNormal=normalize(mat3(u_Model)*skinnedNormal);
    vTexCoord=aTexCoord;
    vFragPosLightSpace=u_LightSpaceMatrix*worldPos;
    gl_Position=u_ViewProjection*worldPos;
}
)glsl";

constexpr std::string_view fragmentSource = R"glsl(#version 450 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vFragPosLightSpace;
uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;
uniform samplerCube u_EnvironmentMap;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPosition;
uniform vec3 u_BaseColor;
uniform vec3 u_BaseColorFactor;
uniform float u_AmbientStrength;
uniform float u_DiffuseStrength;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;
uniform float u_ShadowBias;
uniform bool u_ShadowEnabled;
uniform bool u_UseTexture;
out vec4 fragmentColor;
const float PI=3.14159265359;
float distributionGgx(float nDotH,float roughness){
    float a=roughness*roughness;
    float a2=a*a;
    float d=nDotH*nDotH*(a2-1.0)+1.0;
    return a2/max(PI*d*d,0.000001);
}
float geometrySchlickGgx(float nDotX,float roughness){
    float r=roughness+1.0;
    float k=(r*r)/8.0;
    return nDotX/(nDotX*(1.0-k)+k);
}
float geometrySmith(float nDotV,float nDotL,float roughness){
    return geometrySchlickGgx(nDotV,roughness)*geometrySchlickGgx(nDotL,roughness);
}
vec3 fresnelSchlick(float cosine,vec3 f0){
    return f0+(1.0-f0)*pow(clamp(1.0-cosine,0.0,1.0),5.0);
}
float calculateShadow(vec3 normal, vec3 lightDir){
    if(!u_ShadowEnabled) return 0.0;
    vec3 projected=vFragPosLightSpace.xyz/vFragPosLightSpace.w;
    projected=projected*0.5+0.5;
    if(projected.z>1.0 || projected.x<0.0 || projected.x>1.0 || projected.y<0.0 || projected.y>1.0) return 0.0;
    float bias=max(u_ShadowBias*(1.0-dot(normal,lightDir)),u_ShadowBias*0.2);
    vec2 texelSize=1.0/vec2(textureSize(u_ShadowMap,0));
    float shadow=0.0;
    for(int x=-1;x<=1;++x){
        for(int y=-1;y<=1;++y){
            float closestDepth=texture(u_ShadowMap,projected.xy+vec2(x,y)*texelSize).r;
            shadow+=(projected.z-bias)>closestDepth?1.0:0.0;
        }
    }
    return shadow/9.0;
}
void main(){
    vec4 baseSample=u_UseTexture?texture(u_Texture,vTexCoord):vec4(u_BaseColor,1.0);
    vec3 baseColor=baseSample.rgb*u_BaseColorFactor;
    vec3 normal=normalize(vNormal);
    vec3 lightDir=normalize(-u_LightDirection);
    vec3 viewDir=normalize(u_ViewPosition-vFragPos);
    vec3 halfwayDir=normalize(lightDir+viewDir);
    float nDotV=max(dot(normal,viewDir),0.0);
    float nDotL=max(dot(normal,lightDir),0.0);
    float roughness=clamp(u_Roughness,0.04,1.0);
    float metallic=clamp(u_Metallic,0.0,1.0);
    vec3 f0=mix(vec3(0.04),baseColor,metallic);
    vec3 fresnel=fresnelSchlick(max(dot(halfwayDir,viewDir),0.0),f0);
    float ndf=distributionGgx(max(dot(normal,halfwayDir),0.0),roughness);
    float geometry=geometrySmith(nDotV,nDotL,roughness);
    vec3 specular=ndf*geometry*fresnel/max(4.0*nDotV*nDotL,0.001);
    vec3 kD=(vec3(1.0)-fresnel)*(1.0-metallic);
    vec3 diffuse=kD*baseColor/PI;
    vec3 environmentDiffuse=texture(u_EnvironmentMap,normal).rgb*baseColor*kD;
    vec3 environmentSpecular=texture(u_EnvironmentMap,reflect(-viewDir,normal)).rgb*fresnel*(1.0-roughness);
    vec3 ambient=(environmentDiffuse+environmentSpecular)*u_AmbientStrength*u_AO;
    float shadow=calculateShadow(normal,lightDir);
    vec3 direct=(diffuse+specular)*u_LightColor*u_DiffuseStrength*nDotL;
    fragmentColor=vec4(ambient+(1.0-shadow)*direct,baseSample.a);
}
)glsl";

} // namespace

ModelRenderer::ModelRenderer(const std::filesystem::path& path)
    : gl_(), shadowMap_(gl_), depthShader_(gl_, depthVertexSource, depthFragmentSource),
      shader_(gl_, vertexSource, fragmentSource), model_(gl_, path), groundPlane_(gl_), skybox_(gl_),
      particleRenderer_(gl_) {
    gl_.enable(GL_DEPTH_TEST);
    metallic_ = model_.metallicFactor();
    roughness_ = model_.roughnessFactor();
    ambientOcclusion_ = model_.ambientOcclusion();
    shader_.use();
    shader_.setTextureUnit(0);
    shader_.setShadowMapUnit(1);
    shader_.setEnvironmentMapUnit(2);
}

void ModelRenderer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPosition,
    const DirectionalLight& light, int width, int height) const {
    if (width <= 0 || height <= 0) return;
    const glm::mat4 lightSpace = calculateLightSpaceMatrix(
        light.direction, glm::vec3(0.0F), glm::vec2(16.0F), 0.1F, 50.0F);
    const std::vector<glm::mat4> palette = model_.skinPalette();

    if (shadowsEnabled_) {
        shadowMap_.bindForWriting();
        gl_.viewport(0, 0, shadowMap_.resolution(), shadowMap_.resolution());
        gl_.clear(GL_DEPTH_BUFFER_BIT);
        depthShader_.use();
        depthShader_.setViewProjection(lightSpace);
        depthShader_.setSkinPalette(palette, model_.hasSkin());
        depthShader_.setModelMatrix(glm::mat4(1.0F));
        groundPlane_.draw();
        depthShader_.setModelMatrix(modelTransform_);
        model_.draw();
    }

    ShadowMap::bindDefaultFramebuffer(gl_);
    gl_.viewport(0, 0, width, height);
    gl_.clearColor(0.035F, 0.055F, 0.075F, 1.0F);
    gl_.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shadowMap_.bindForSampling(1);
    skybox_.bindEnvironment(2);
    gl_.activeTexture(GL_TEXTURE0);
    shader_.use();
    shader_.setViewProjection(projection * view);
    shader_.setModelMatrix(glm::mat4(1.0F));
    shader_.setLightSpaceMatrix(lightSpace);
    shader_.setLighting(light, viewPosition);
    shader_.setShadowSettings(shadowsEnabled_, shadowBias_);
    shader_.setPbrMaterial(metallic_, roughness_, ambientOcclusion_);
    shader_.setSkinPalette({}, false);
    shader_.setBaseColor({0.25F, 0.29F, 0.31F});
    shader_.setBaseColorFactor({1.0F, 1.0F, 1.0F});
    shader_.setPbrMaterial(0.0F, 0.9F, 1.0F);
    groundPlane_.draw();

    shader_.useTexture(true);
    shader_.setBaseColorFactor(model_.baseColorFactor());
    shader_.setPbrMaterial(metallic_, roughness_, ambientOcclusion_);
    shader_.setSkinPalette(palette, model_.hasSkin());
    shader_.setModelMatrix(modelTransform_);
    model_.bindTexture(0);
    model_.draw();
    if (skyboxEnabled_) skybox_.draw(view, projection);
    const glm::vec3 cameraRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    const glm::vec3 cameraUp = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
    particleRenderer_.draw(particles_, projection * view, cameraRight, cameraUp);
}

void ModelRenderer::renderOverlay(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPosition, const DirectionalLight& light, const glm::vec3& position,
    float yawDegrees, float scale, const glm::vec3& tint) const {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        !std::isfinite(yawDegrees) || !std::isfinite(scale) || scale <= 0.0F ||
        !std::isfinite(tint.x) || !std::isfinite(tint.y) || !std::isfinite(tint.z)) return;
    shadowMap_.bindForSampling(1);
    skybox_.bindEnvironment(2);
    gl_.activeTexture(GL_TEXTURE0);
    shader_.use();
    shader_.setViewProjection(projection * view);
    shader_.setModelMatrix(glm::translate(glm::mat4(1.0F), position) *
        glm::rotate(glm::mat4(1.0F), glm::radians(yawDegrees), glm::vec3(0.0F, 1.0F, 0.0F)) *
        glm::scale(glm::mat4(1.0F), glm::vec3(scale)));
    shader_.setLighting(light, viewPosition);
    shader_.setShadowSettings(false, shadowBias_);
    shader_.setPbrMaterial(metallic_, roughness_, ambientOcclusion_);
    shader_.setSkinPalette(model_.skinPalette(), model_.hasSkin());
    shader_.useTexture(true);
    shader_.setBaseColorFactor(model_.baseColorFactor() * tint);
    model_.bindTexture(0);
    model_.draw();
}

void ModelRenderer::setTransform(const glm::vec3& position, float yawDegrees) noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        !std::isfinite(yawDegrees)) return;
    modelTransform_ = glm::translate(glm::mat4(1.0F), position) *
        glm::rotate(glm::mat4(1.0F), glm::radians(yawDegrees), glm::vec3(0.0F, 1.0F, 0.0F));
}

void ModelRenderer::setShadowBias(float bias) noexcept {
    if (std::isfinite(bias)) shadowBias_ = std::clamp(bias, 0.0001F, 0.02F);
}

void ModelRenderer::setPbrMaterial(float metallic, float roughness, float ao) noexcept {
    if (std::isfinite(metallic)) metallic_ = std::clamp(metallic, 0.0F, 1.0F);
    if (std::isfinite(roughness)) roughness_ = std::clamp(roughness, 0.04F, 1.0F);
    if (std::isfinite(ao)) ambientOcclusion_ = std::clamp(ao, 0.0F, 1.0F);
}

void ModelRenderer::updateAnimation(float deltaSeconds) {
    model_.updateAnimation(deltaSeconds);
    particles_.update(deltaSeconds);
}

std::size_t ModelRenderer::spawnParticleBurst(const glm::vec3& position, std::size_t count) {
    ParticleConfig config;
    config.direction = {0.0F, 0.65F, 0.0F};
    config.gravity = {0.0F, -4.2F, 0.0F};
    config.color = {1.0F, 0.42F, 0.08F, 0.95F};
    config.coneAngleRadians = 1.35F;
    config.minimumSpeed = 1.0F;
    config.maximumSpeed = 4.5F;
    config.minimumSize = 0.025F;
    config.maximumSize = 0.09F;
    config.lifetime = 0.55F;
    config.randomSeed = static_cast<std::uint32_t>(particles_.activeCount() + 0xB17U);
    return particles_.spawnBurst(position, count, config);
}
void ModelRenderer::setAnimationPlaying(bool playing) noexcept { model_.setAnimationPlaying(playing); }
void ModelRenderer::setAnimationTime(float time) noexcept { model_.setAnimationTime(time); }
void ModelRenderer::selectAnimation(std::size_t index) noexcept { model_.selectAnimation(index); }
void ModelRenderer::setFadeDuration(float seconds) noexcept { model_.setFadeDuration(seconds); }
float ModelRenderer::fadeDuration() const noexcept { return model_.fadeDuration(); }
float ModelRenderer::blendFactor() const noexcept { return model_.blendFactor(); }
const std::string& ModelRenderer::animationState() const noexcept { return model_.animationState(); }
bool ModelRenderer::triggerAnimationState(const std::string& state) { return model_.triggerAnimationState(state); }
void ModelRenderer::setAnimationEventCallback(AnimationEventCallback callback) { model_.setAnimationEventCallback(std::move(callback)); }
std::size_t ModelRenderer::animationIndex() const noexcept { return model_.animationIndex(); }
std::size_t ModelRenderer::animationCount() const noexcept { return model_.animationCount(); }
bool ModelRenderer::hasAnimation() const noexcept { return model_.hasAnimation(); }
bool ModelRenderer::isAnimationPlaying() const noexcept { return model_.isAnimationPlaying(); }
float ModelRenderer::animationTime() const noexcept { return model_.animationTime(); }
float ModelRenderer::animationDuration() const noexcept { return model_.animationDuration(); }
const std::string& ModelRenderer::animationName() const noexcept { return model_.animationName(); }
const std::string& ModelRenderer::animationName(std::size_t index) const noexcept { return model_.animationName(index); }

} // namespace glory::engine

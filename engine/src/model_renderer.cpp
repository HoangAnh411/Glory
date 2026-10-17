#include "glory/engine/model_renderer.hpp"

#include "glory/engine/shadow_math.hpp"
#include "glory/engine/pbr_math.hpp"
#include "glory/engine/image_loader.hpp"
#include "glory/engine/color_space.hpp"
#include "glory/engine/render_pipeline.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>

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
layout(location=5) in vec4 aColor;
uniform mat4 u_ViewProjection;
uniform mat4 u_ViewMatrix;
uniform mat4 u_Model;
uniform mat4 u_JointMatrices[64];
uniform bool u_HasSkin;
out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;
out float vViewDepth;
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
    vViewDepth=-(u_ViewMatrix*worldPos).z;
    vNormal=normalize(mat3(u_Model)*skinnedNormal);
    vTexCoord=aTexCoord;
    vColor=aColor;
    gl_Position=u_ViewProjection*worldPos;
}
)glsl";

constexpr std::string_view fragmentSource = R"glsl(#version 450 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;
in float vViewDepth;
uniform sampler2D u_Texture;
uniform sampler2DShadow u_ShadowMap;
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
uniform float u_NormalOffsetBias;
uniform mat4 u_LightSpaceMatrices[4];
uniform vec4 u_CascadeSplits;
uniform int u_CascadeCount;
uniform int u_PcfSamples;
uniform int u_ShadowResolution;
uniform float u_ShadowFar;
uniform bool u_CascadeDebug;
uniform int u_LightingDebugView;
uniform bool u_ShadowEnabled;
uniform bool u_UseTexture;
uniform bool u_EncodeSrgb;
uniform vec3 u_EmissiveFactor;
uniform float u_Opacity;
uniform bool u_CharacterLighting;
uniform bool u_FogEnabled;
uniform bool u_HemisphereEnabled;
uniform float u_FogDensity;
uniform int u_SkyPreset;
out vec4 fragmentColor;
const float PI=3.14159265359;
const vec2 vogelDisk[16]=vec2[16](
    vec2(0.39774756,0.00000000),vec2(-0.50600648,0.46359826),
    vec2(0.07699518,-0.87695120),vec2(0.63738013,0.83107266),
    vec2(-1.17545342,-0.20783601),vec2(1.09201976,-0.69323575),
    vec2(-0.38563069,1.42172464),vec2(-0.73031095,-1.43165112),
    vec2(1.63849415,0.59840280),vec2(-1.69456649,0.63232072),
    vec2(0.99943070,-1.68609473),vec2(0.53272225,1.92035015),
    vec2(-1.84741186,-1.09207306),vec2(2.17128113,-0.27538942),
    vec2(-1.57984202,1.55766457),vec2(-0.08903172,-2.24734315));
vec3 srgbToLinear(vec3 value){
    vec3 low=value/12.92;
    vec3 high=pow((value+0.055)/1.055,vec3(2.4));
    return mix(high,low,lessThanEqual(value,vec3(0.04045)));
}
vec3 linearToSrgb(vec3 value){
    vec3 low=value*12.92;
    vec3 high=1.055*pow(max(value,vec3(0.0)),vec3(1.0/2.4))-0.055;
    return mix(high,low,lessThanEqual(value,vec3(0.0031308)));
}
vec3 evaluateSkyColor(vec3 direction){
    float horizon=1.0-smoothstep(-0.18,0.46,direction.y);
    vec3 zenith=vec3(0.12,0.16,0.38);
    vec3 horizonColor=vec3(0.94,0.36,0.20);
    vec3 midColor=vec3(0.88,0.29,0.32);
    if(u_SkyPreset==1){zenith=vec3(0.16,0.20,0.42);horizonColor=vec3(0.98,0.58,0.48);midColor=vec3(0.65,0.38,0.62);}
    else if(u_SkyPreset==2){zenith=vec3(0.12,0.36,0.68);horizonColor=vec3(0.78,0.84,0.91);midColor=vec3(0.34,0.58,0.82);}
    else if(u_SkyPreset==3){zenith=vec3(0.12,0.38,0.75);horizonColor=vec3(0.55,0.62,0.70);midColor=horizonColor;}
    else if(u_SkyPreset==4){zenith=vec3(0.16,0.25,0.40);horizonColor=vec3(0.66,0.69,0.72);midColor=vec3(0.40,0.52,0.64);}
    vec3 sky=mix(zenith,horizonColor,horizon);
    sky=mix(sky,midColor,exp(-pow((direction.y-0.25)*3.5,2.0))*0.52);
    float sun= max(dot(direction,normalize(-u_LightDirection)),0.0);
    float horizonBand=exp(-pow((direction.y-0.015)*7.0,2.0));
    sky+=vec3(0.72,0.30,0.10)*exp(-max(0.0,1.0-sun)*32.0)*(0.2+0.8*horizonBand);
    return srgbToLinear(sky);
}
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
float sampleCascadeShadow(int cascade, vec3 worldPosition, vec3 normal, vec3 lightDir){
    vec3 offsetPosition=worldPosition+normal*u_NormalOffsetBias;
    vec4 lightClip=u_LightSpaceMatrices[cascade]*vec4(offsetPosition,1.0);
    if(abs(lightClip.w)<0.000001 || any(isnan(lightClip)) || any(isinf(lightClip))) return 0.0;
    vec3 projected=lightClip.xyz/lightClip.w*0.5+0.5;
    if(any(isnan(projected)) || any(isinf(projected)) || projected.z<0.0 || projected.z>1.0 ||
        projected.x<0.0 || projected.x>1.0 || projected.y<0.0 || projected.y>1.0) return 0.0;
    float bias=max(u_ShadowBias*(1.0-max(dot(normal,lightDir),0.0)),u_ShadowBias*0.12);
    int rotationIndex=(int(gl_FragCoord.x)+int(gl_FragCoord.y))&3;
    vec2 atlasOrigin=vec2(float(cascade%2),float(cascade/2))*0.5;
    vec2 atlasUv=projected.xy*0.5+atlasOrigin;
    vec2 texel=vec2(1.0/float(u_ShadowResolution*2));
    float shadow=0.0;
    int fetchCount=max(1,u_PcfSamples/4);
    int sampleStride=16/fetchCount;
    for(int sampleIndex=0;sampleIndex<4;++sampleIndex){
        if(sampleIndex>=fetchCount) break;
        vec2 disk=vogelDisk[sampleIndex*sampleStride];
        if(rotationIndex==1) disk=vec2(-disk.y,disk.x);
        else if(rotationIndex==2) disk=-disk;
        else if(rotationIndex==3) disk=vec2(disk.y,-disk.x);
        vec2 localOffset=disk*texel;
        vec2 clampedUv=clamp(atlasUv+localOffset,atlasOrigin+texel*1.5,atlasOrigin+vec2(0.5)-texel*1.5);
        shadow+=1.0-texture(u_ShadowMap,vec3(clampedUv,projected.z-bias));
    }
    return shadow/float(fetchCount);
}
int selectCascade(float viewDistance){
    int cascade=0;
    for(int index=0;index<3;++index)
        if(index<u_CascadeCount-1 && viewDistance>u_CascadeSplits[index]) cascade=index+1;
    return cascade;
}
float calculateShadow(vec3 normal, vec3 lightDir){
    if(!u_ShadowEnabled || u_CascadeCount<=0) return 0.0;
    if(dot(normal,lightDir)<=0.015) return 0.0;
    float viewDistance=vViewDepth;
    if(viewDistance>u_ShadowFar) return 0.0;
    int cascade=selectCascade(viewDistance);
    float shadow=sampleCascadeShadow(cascade,vFragPos,normal,lightDir);
    float splitNear=cascade==0?0.1:u_CascadeSplits[cascade-1];
    float splitFar=u_CascadeSplits[cascade];
    float blendStart=mix(splitNear,splitFar,0.90);
    if(cascade<u_CascadeCount-1 && viewDistance>blendStart){
        float nextShadow=sampleCascadeShadow(cascade+1,vFragPos,normal,lightDir);
        shadow=mix(shadow,nextShadow,smoothstep(blendStart,splitFar,viewDistance));
    }
    return shadow;
}
void main(){
    vec4 baseSample=u_UseTexture?texture(u_Texture,vTexCoord):vec4(u_BaseColor,1.0);
    vec3 baseColor=baseSample.rgb*u_BaseColorFactor*vColor.rgb;
    vec3 normal=normalize(vNormal);
    if(any(isnan(normal)) || any(isinf(normal)) || dot(normal,normal)<0.000001) normal=vec3(0.0,1.0,0.0);
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
    vec3 envSky=texture(u_EnvironmentMap,normal).rgb;
    float envLuma=dot(envSky,vec3(0.299,0.587,0.114));
    vec3 skyLight=mix(vec3(envLuma),envSky,0.25);
    vec3 groundBounce=vec3(0.38,0.34,0.26);
    float hemi=clamp(normal.y*0.5+0.5,0.0,1.0);
    vec3 ambientDiffuseLight=mix(groundBounce,skyLight,hemi);
    vec3 environmentDiffuse=ambientDiffuseLight*baseColor*kD;
    vec3 environmentSpecular=texture(u_EnvironmentMap,reflect(-viewDir,normal)).rgb*fresnel*(1.0-roughness);
    vec3 ambient=max((environmentDiffuse+environmentSpecular)*max(u_AmbientStrength,0.0)*clamp(u_AO,0.0,1.0),vec3(0.0));
    if(u_HemisphereEnabled) ambient=max(ambient,baseColor*0.12);
    float shadow=calculateShadow(normal,lightDir);
    vec3 direct=(diffuse+specular)*srgbToLinear(u_LightColor)*u_DiffuseStrength*nDotL;
    vec3 litColor=ambient+(1.0-shadow)*direct;
    if(u_CascadeDebug && u_ShadowEnabled){
        const vec3 cascadeColors[4]=vec3[4](vec3(1.0,0.25,0.2),vec3(0.25,1.0,0.3),vec3(0.2,0.45,1.0),vec3(1.0,0.8,0.2));
        litColor=mix(litColor,cascadeColors[selectCascade(vViewDepth)],0.35);
    }
    if(u_CharacterLighting){
        float backLight=max(dot(normal,normalize(vec3(-lightDir.x,0.25,-lightDir.z))),0.0);
        float rim=pow(1.0-max(dot(normal,viewDir),0.0),3.0);
        litColor+=baseColor*vec3(0.12,0.19,0.28)*backLight+vec3(0.16,0.22,0.34)*rim;
    }
    float dist=length(u_ViewPosition-vFragPos);
    float heightDensity=exp(-max(vFragPos.y,0.0)*0.045);
    float fogAmount=1.0-exp(-pow(max(u_FogDensity,0.0)*max(dist,0.0)*heightDensity,2.0));
    float fogFactor=u_FogEnabled?clamp(fogAmount*smoothstep(12.0,38.0,dist),0.0,0.82):0.0;
    vec3 fogDirection=vFragPos-u_ViewPosition;
    vec3 fogColor=evaluateSkyColor(dot(fogDirection,fogDirection)>0.000001?normalize(fogDirection):vec3(0.0,1.0,0.0));
    vec3 directColor=max((1.0-shadow)*direct,vec3(0.0));
    vec3 finalColor=mix(litColor,fogColor,fogFactor)+max(u_EmissiveFactor,vec3(0.0));
    bool invalidValue=any(isnan(baseColor))||any(isinf(baseColor)||lessThan(baseColor,vec3(0.0)))||
        any(isnan(normal))||any(isinf(normal))||any(isnan(ambient))||any(isinf(ambient))||
        any(isnan(directColor))||any(isinf(directColor))||any(isnan(finalColor))||any(isinf(finalColor))||
        any(lessThan(finalColor,vec3(0.0)))||isnan(fogFactor)||isinf(fogFactor)||isnan(shadow)||isinf(shadow);
    if(u_LightingDebugView==1) finalColor=max(baseColor,vec3(0.0));
    else if(u_LightingDebugView==2) finalColor=normal*0.5+0.5;
    else if(u_LightingDebugView==3) finalColor=max(ambient,vec3(0.0));
    else if(u_LightingDebugView==4) finalColor=directColor;
    else if(u_LightingDebugView==5) finalColor=vec3(1.0-clamp(shadow,0.0,1.0));
    else if(u_LightingDebugView==6) finalColor=vec3(clamp(fogFactor,0.0,1.0));
    else if(u_LightingDebugView==7) finalColor=vec3(clamp(length(vFragPos-u_ViewPosition)/max(u_ShadowFar,1.0),0.0,1.0));
    else if(u_LightingDebugView==8 && invalidValue) finalColor=vec3(1.0,0.0,1.0);
    if(invalidValue && u_LightingDebugView==8) finalColor=vec3(1.0,0.0,1.0);
    if(u_EncodeSrgb) finalColor=linearToSrgb(finalColor);
    fragmentColor=vec4(finalColor,baseSample.a*u_Opacity);
}
)glsl";

} // namespace

ModelRenderer::ModelRenderer(const std::filesystem::path& path)
    : gl_(), depthShader_(gl_, depthVertexSource, depthFragmentSource),
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
    const auto worldGroundPath = path.parent_path().parent_path() / "textures" / "world_ground.png";
    const auto floorTexPath = std::filesystem::exists(worldGroundPath) ? worldGroundPath :
        path.parent_path().parent_path() / "textures" / "arena_floor.png";
    if (std::filesystem::exists(floorTexPath)) {
        try {
            arenaFloorTexture_ = std::make_unique<Texture2D>(gl_, loadImage(floorTexPath));
        } catch (...) {
            arenaFloorTexture_.reset();
        }
    }

    const auto worldMapPath = path.parent_path().parent_path() / "data" / "world_map.json";
    if (std::filesystem::exists(worldMapPath)) {
        try {
            std::ifstream file(worldMapPath);
            nlohmann::json root;
            file >> root;
            if (root.contains("props") && root["props"].is_array()) {
                const auto modelsDir = path.parent_path();
                std::size_t propIndex = 0;
                for (const auto& item : root["props"]) {
                    StaticWorldProp prop;
                    prop.instanceIndex = propIndex++;
                    prop.type = item.value("type", "");
                    if (item.contains("position") && item["position"].is_array() && item["position"].size() >= 3) {
                        prop.position = {item["position"][0].get<float>(), item["position"][1].get<float>(), item["position"][2].get<float>()};
                    }
                    prop.yaw = item.value("yaw", 0.0F);
                    prop.scale = item.value("scale", 1.0F);
                    staticProps_.push_back(prop);

                    if (!prop.type.empty() && !propModelTemplates_.contains(prop.type)) {
                        const auto propModelPath = modelsDir / (prop.type + ".gltf");
                        if (std::filesystem::exists(propModelPath)) {
                            try {
                                propModelTemplates_[prop.type] = std::make_unique<Model>(gl_, propModelPath);
                            } catch (...) {
                            }
                        }
                    }
                }
            }
        } catch (...) {
        }
    }
}

void ModelRenderer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPosition,
    const DirectionalLight& light, int width, int height, RenderPipeline& pipeline,
    const std::vector<RenderActor>& actors) const {
    if (width <= 0 || height <= 0) return;
    const std::vector<float> splits = calculateCascadeSplits(0.1F, shadowFar_, cascadeCount_, cascadeLambda_);
    const auto lightSpaces = calculateCascadeLightSpaceMatrices(
        view, projection, light.direction, splits, 0.1F, pipeline.shadowMap().resolution());
    const std::vector<glm::mat4> palette = model_.skinPalette();

    if (shadowsEnabled_) {
        ShadowMap& atlas = pipeline.shadowMap();
        atlas.bindForWriting(0);
        gl_.clear(GL_DEPTH_BUFFER_BIT);
        for (int cascade = 0; cascade < cascadeCount_; ++cascade) {
            atlas.bindForWriting(cascade);
            pipeline.beginPass(static_cast<GpuPass>(static_cast<std::size_t>(GpuPass::ShadowCascade0) +
                static_cast<std::size_t>(cascade)));
            depthShader_.use();
            depthShader_.setViewProjection(lightSpaces[static_cast<std::size_t>(cascade)]);
            depthShader_.setModelMatrix(glm::mat4(1.0F));
            depthShader_.setSkinPalette({}, false);
            groundPlane_.draw();
            for (const auto& prop : staticProps_) {
                auto it = propModelTemplates_.find(prop.type);
                if (it == propModelTemplates_.end() || !it->second ||
                    !sphereIntersectsClip(lightSpaces[static_cast<std::size_t>(cascade)], prop.position,
                        std::max(1.0F, std::abs(prop.scale) * 2.5F))) continue;
                const glm::mat4 transform = glm::translate(glm::mat4(1.0F), prop.position) *
                    glm::rotate(glm::mat4(1.0F), glm::radians(prop.yaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
                    glm::scale(glm::mat4(1.0F), glm::vec3(prop.scale));
                depthShader_.setModelMatrix(transform);
                it->second->draw();
            }
            depthShader_.setModelMatrix(modelTransform_);
            depthShader_.setSkinPalette(palette, model_.hasSkin());
            model_.draw();
            pipeline.endPass();

            pipeline.beginActorShadowTiming(cascade);
            for (const RenderActor& actor : actors) {
                if (!actor.renderer || !sphereIntersectsClip(lightSpaces[static_cast<std::size_t>(cascade)],
                    actor.position, actor.scale * 2.5F)) continue;
                const ModelRenderer& source = *actor.renderer;
                depthShader_.setModelMatrix(glm::translate(glm::mat4(1.0F), actor.position) *
                    glm::rotate(glm::mat4(1.0F), glm::radians(actor.yaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
                    glm::scale(glm::mat4(1.0F), glm::vec3(actor.scale)));
                depthShader_.setSkinPalette(source.model_.skinPalette(), source.model_.hasSkin());
                source.model_.draw();
            }
            pipeline.endActorShadowTiming(cascade);
        }
    }

    ShadowMap::bindDefaultFramebuffer(gl_);
    pipeline.bindSceneTarget();
    gl_.viewport(0, 0, width, height);
    const float clearRed = pipeline.hdrEnabled() ? srgbToLinear(0.035F) : 0.035F;
    const float clearGreen = pipeline.hdrEnabled() ? srgbToLinear(0.055F) : 0.055F;
    const float clearBlue = pipeline.hdrEnabled() ? srgbToLinear(0.075F) : 0.075F;
    gl_.clearColor(clearRed, clearGreen, clearBlue, 1.0F);
    gl_.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pipeline.shadowMap().bindForSampling(1);
    skybox_.bindEnvironment(2);
    gl_.activeTexture(GL_TEXTURE0);
    shader_.use();
    shader_.setViewProjection(projection * view);
    const GLint viewMatrixLocation = gl_.getUniformLocation(shader_.program(), "u_ViewMatrix");
    if (viewMatrixLocation >= 0) gl_.uniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(view));
    shader_.setModelMatrix(glm::mat4(1.0F));
    const GLint matricesLocation = gl_.getUniformLocation(shader_.program(), "u_LightSpaceMatrices[0]");
    if (matricesLocation >= 0) gl_.uniformMatrix4fv(matricesLocation, cascadeCount_, GL_FALSE,
        glm::value_ptr(lightSpaces[0]));
    const GLint splitsLocation = gl_.getUniformLocation(shader_.program(), "u_CascadeSplits");
    if (splitsLocation >= 0) gl_.uniform4f(splitsLocation,
        splits[0], splits.size() > 1 ? splits[1] : splits[0],
        splits.size() > 2 ? splits[2] : splits.back(), splits.size() > 3 ? splits[3] : splits.back());
    const GLint cascadeCountLocation = gl_.getUniformLocation(shader_.program(), "u_CascadeCount");
    if (cascadeCountLocation >= 0) gl_.uniform1i(cascadeCountLocation, cascadeCount_);
    const GLint shadowFarLocation = gl_.getUniformLocation(shader_.program(), "u_ShadowFar");
    if (shadowFarLocation >= 0) gl_.uniform1f(shadowFarLocation, shadowFar_);
    const GLint pcfLocation = gl_.getUniformLocation(shader_.program(), "u_PcfSamples");
    if (pcfLocation >= 0) gl_.uniform1i(pcfLocation, pcfSamples_);
    const GLint resolutionLocation = gl_.getUniformLocation(shader_.program(), "u_ShadowResolution");
    if (resolutionLocation >= 0) gl_.uniform1i(resolutionLocation, pipeline.shadowMap().resolution());
    const GLint normalBiasLocation = gl_.getUniformLocation(shader_.program(), "u_NormalOffsetBias");
    if (normalBiasLocation >= 0) gl_.uniform1f(normalBiasLocation, normalOffsetBias_);
    const GLint debugLocation = gl_.getUniformLocation(shader_.program(), "u_CascadeDebug");
    if (debugLocation >= 0) gl_.uniform1i(debugLocation, cascadeDebug_ ? GL_TRUE : GL_FALSE);
    shader_.setLighting(light, viewPosition);
    shader_.setShadowSettings(shadowsEnabled_, shadowBias_);
    shader_.setPbrMaterial(metallic_, roughness_, ambientOcclusion_);
    shader_.setSkinPalette({}, false);
    shader_.setEncodeSrgb(!pipeline.hdrEnabled());
    gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_CharacterLighting"), GL_FALSE);
    gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_FogEnabled"), fogEnabled_ ? GL_TRUE : GL_FALSE);
    gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_HemisphereEnabled"), hemisphereEnabled_ ? GL_TRUE : GL_FALSE);
    gl_.uniform1f(gl_.getUniformLocation(shader_.program(), "u_FogDensity"), fogDensity_);
    gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_SkyPreset"), skyPreset_);
    gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_LightingDebugView"), lightingDebugView_);
    shader_.setEmissiveFactor({0.0F, 0.0F, 0.0F});
    pipeline.beginPass(GpuPass::Scene);
    if (arenaFloorTexture_) {
        shader_.useTexture(true);
        arenaFloorTexture_->bind(0);
        shader_.setBaseColor({1.0F, 1.0F, 1.0F});
        shader_.setBaseColorFactor({1.0F, 1.0F, 1.0F});
        shader_.setPbrMaterial(0.1F, 0.7F, 1.0F);
    } else {
        shader_.useTexture(false);
        shader_.setBaseColor({srgbToLinear(0.36F), srgbToLinear(0.40F), srgbToLinear(0.44F)});
        shader_.setBaseColorFactor({1.0F, 1.0F, 1.0F});
        shader_.setPbrMaterial(0.05F, 0.75F, 1.0F);
    }
    groundPlane_.draw();

    for (const auto& prop : staticProps_) {
        auto it = propModelTemplates_.find(prop.type);
        if (it != propModelTemplates_.end() && it->second) {
            glm::mat4 m = glm::translate(glm::mat4(1.0F), prop.position) *
                glm::rotate(glm::mat4(1.0F), glm::radians(prop.yaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
                glm::scale(glm::mat4(1.0F), glm::vec3(prop.scale));
            shader_.setModelMatrix(m);
            shader_.setSkinPalette({}, false);
            shader_.setBaseColorFactor(it->second->baseColorFactor());
            shader_.setPbrMaterial(it->second->metallicFactor(), it->second->roughnessFactor(), it->second->ambientOcclusion());
            shader_.setEmissiveFactor(it->second->emissiveFactor());
            it->second->bindTexture(0);
            const bool fadeForCamera = cameraBlockerName_.starts_with(prop.type + "#") &&
                glm::length(prop.position - cameraBlockerPosition_) < 0.01F;
            const GLint opacityLocation = gl_.getUniformLocation(shader_.program(), "u_Opacity");
            if (opacityLocation >= 0) gl_.uniform1f(opacityLocation, fadeForCamera ? 0.28F : 1.0F);
            if (fadeForCamera) {
                gl_.enable(GL_BLEND);
                gl_.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                gl_.depthMask(GL_FALSE);
            }
            it->second->draw();
            if (fadeForCamera) {
                gl_.depthMask(GL_TRUE);
                gl_.disable(GL_BLEND);
            }
        }
    }

    const GLint opacityLocation = gl_.getUniformLocation(shader_.program(), "u_Opacity");
    if (opacityLocation >= 0) gl_.uniform1f(opacityLocation, 1.0F);
    shader_.useTexture(true);
    shader_.setBaseColorFactor(model_.baseColorFactor() * (deadAppearance_ ? 0.28F : 1.0F));
    shader_.setEmissiveFactor(model_.emissiveFactor());
    shader_.setPbrMaterial(metallic_, roughness_, ambientOcclusion_);
    shader_.setSkinPalette(palette, model_.hasSkin());
    shader_.setModelMatrix(modelTransform_);
    model_.bindTexture(0);
    const GLint characterOpacityLocation = gl_.getUniformLocation(shader_.program(), "u_Opacity");
    if (characterOpacityLocation >= 0) gl_.uniform1f(characterOpacityLocation,
        deadAppearance_ ? 0.28F : cameraFadeOpacity_);
    if (cameraFadeOpacity_ < 0.999F) {
        gl_.enable(GL_BLEND);
        gl_.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl_.depthMask(GL_FALSE);
    }
    model_.draw();
    if (cameraFadeOpacity_ < 0.999F) {
        gl_.depthMask(GL_TRUE);
        gl_.disable(GL_BLEND);
    }
    for (const RenderActor& actor : actors) {
        if (!actor.renderer) continue;
        const ModelRenderer& source = *actor.renderer;
        shader_.setModelMatrix(glm::translate(glm::mat4(1.0F), actor.position) *
            glm::rotate(glm::mat4(1.0F), glm::radians(actor.yaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
            glm::scale(glm::mat4(1.0F), glm::vec3(actor.scale)));
        shader_.setSkinPalette(source.model_.skinPalette(), source.model_.hasSkin());
        shader_.setBaseColorFactor(source.model_.baseColorFactor() * glm::vec3(
            srgbToLinear(actor.tint.r), srgbToLinear(actor.tint.g), srgbToLinear(actor.tint.b)));
        shader_.setEmissiveFactor(source.model_.emissiveFactor());
        shader_.setPbrMaterial(source.metallic_, source.roughness_, source.ambientOcclusion_);
        source.model_.bindTexture(0);
        gl_.uniform1i(gl_.getUniformLocation(shader_.program(), "u_CharacterLighting"),
            characterLightingEnabled_ ? GL_TRUE : GL_FALSE);
        source.model_.draw();
    }
    pipeline.endPass();
    pipeline.beginPass(GpuPass::Sky);
    if (skyboxEnabled_) skybox_.draw(view, projection, !pipeline.hdrEnabled(), light.direction, skyPreset_);
    pipeline.endPass();
    const glm::vec3 cameraRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    const glm::vec3 cameraUp = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
    pipeline.beginPass(GpuPass::ParticleOverlay);
    particleRenderer_.draw(particles_, projection * view, cameraRight, cameraUp, !pipeline.hdrEnabled());
    pipeline.endPass();
}

void ModelRenderer::setTransform(const glm::vec3& position, float yawDegrees) noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        !std::isfinite(yawDegrees)) return;
    modelTransform_ = glm::translate(glm::mat4(1.0F), position) *
        glm::rotate(glm::mat4(1.0F), glm::radians(yawDegrees), glm::vec3(0.0F, 1.0F, 0.0F));
}

float cameraColliderDistance(const glm::vec3& origin, const glm::vec3& desiredPosition,
    const std::vector<CameraCollider>& colliders, float cameraRadius, std::string* blockerName,
    glm::vec3* blockerPosition) noexcept {
    const glm::vec3 segment = desiredPosition - origin;
    const float segmentLength = glm::length(segment);
    if (!std::isfinite(segmentLength) || segmentLength <= 1.0e-4F ||
        !std::isfinite(cameraRadius) || cameraRadius < 0.0F) return 0.0F;
    const glm::vec3 direction = segment / segmentLength;
    float allowedDistance = segmentLength;
    if (blockerName) *blockerName = "none";
    if (blockerPosition) *blockerPosition = origin;
    for (const CameraCollider& collider : colliders) {
        if (!collider.blocksMovement) continue;
        const glm::vec3 size = collider.maximum - collider.minimum;
        if (std::max({size.x, size.y, size.z}) < 1.2F) continue;
        const glm::vec3 boundsMin = collider.minimum - glm::vec3(cameraRadius);
        const glm::vec3 boundsMax = collider.maximum + glm::vec3(cameraRadius);
        float entry = 0.0F;
        float exit = allowedDistance;
        bool hit = true;
        for (int axis = 0; axis < 3; ++axis) {
            if (std::abs(direction[axis]) < 1.0e-6F) {
                if (origin[axis] < boundsMin[axis] || origin[axis] > boundsMax[axis]) hit = false;
                continue;
            }
            float nearT = (boundsMin[axis] - origin[axis]) / direction[axis];
            float farT = (boundsMax[axis] - origin[axis]) / direction[axis];
            if (nearT > farT) std::swap(nearT, farT);
            entry = std::max(entry, nearT);
            exit = std::min(exit, farT);
            if (entry > exit) hit = false;
        }
        if (hit && entry >= 0.0F && entry < allowedDistance) {
            allowedDistance = entry;
            if (blockerName) *blockerName = collider.name;
            if (blockerPosition) *blockerPosition = (collider.minimum + collider.maximum) * 0.5F;
        }
    }
    return allowedDistance;
}

float ModelRenderer::cameraObstructionDistance(const glm::vec3& origin,
    const glm::vec3& desiredPosition, float cameraRadius) const noexcept {
    cameraBlockerName_ = "none";
    cameraBlockerPosition_ = origin;
    float allowedDistance = cameraColliderDistance(origin, desiredPosition, cameraColliders_, cameraRadius,
        &cameraBlockerName_, &cameraBlockerPosition_);
    const glm::vec3 segment = desiredPosition - origin;
    const float segmentLength = glm::length(segment);
    if (!std::isfinite(segmentLength) || segmentLength <= 1.0e-4F) return 0.0F;
    const glm::vec3 direction = segment / segmentLength;

    const glm::vec2 start{origin.x - cameraArenaCenter_.x, origin.z - cameraArenaCenter_.y};
    const glm::vec2 ray{direction.x, direction.z};
    const glm::vec2 end = start + ray * allowedDistance;
    if (cameraArenaRadius_ > 0.0F && glm::dot(end, end) > cameraArenaRadius_ * cameraArenaRadius_) {
        const float a = glm::dot(ray, ray);
        const float b = 2.0F * glm::dot(start, ray);
        const float c = glm::dot(start, start) - cameraArenaRadius_ * cameraArenaRadius_;
        const float discriminant = b * b - 4.0F * a * c;
        if (a > 1.0e-6F && discriminant >= 0.0F) {
            const float exitDistance = (-b + std::sqrt(discriminant)) / (2.0F * a);
            const float boundaryDistance = std::max(0.0F, exitDistance - cameraRadius);
            if (boundaryDistance < allowedDistance) {
                allowedDistance = boundaryDistance;
                cameraBlockerName_ = "arena perimeter";
                cameraBlockerPosition_ = origin + direction * boundaryDistance;
            }
        }
    }
    lastCameraObstructionDistance_ = allowedDistance;
    if (cameraBlockerName_.empty()) cameraBlockerName_ = "none";
    return allowedDistance;
}

void ModelRenderer::setCameraColliders(std::vector<CameraCollider> colliders,
    glm::vec2 arenaCenter, float arenaRadius) {
    cameraColliders_ = std::move(colliders);
    cameraArenaCenter_ = arenaCenter;
    cameraArenaRadius_ = std::isfinite(arenaRadius) ? std::max(0.0F, arenaRadius) : 0.0F;
}

std::vector<RenderedWorldPropBounds> ModelRenderer::renderedWorldPropBounds() const {
    std::vector<RenderedWorldPropBounds> result;
    result.reserve(staticProps_.size());
    for (const StaticWorldProp& prop : staticProps_) {
        const auto model = propModelTemplates_.find(prop.type);
        if (model == propModelTemplates_.end() || !model->second) continue;
        const auto [localMin, localMax] = model->second->localBounds();
        const glm::mat4 transform = glm::translate(glm::mat4(1.0F), prop.position) *
            glm::rotate(glm::mat4(1.0F), glm::radians(prop.yaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
            glm::scale(glm::mat4(1.0F), glm::vec3(prop.scale));
        RenderedWorldPropBounds bounds;
        bounds.name = prop.type + "#" + std::to_string(prop.instanceIndex);
        bounds.minimum = glm::vec3(std::numeric_limits<float>::max());
        bounds.maximum = glm::vec3(std::numeric_limits<float>::lowest());
        for (int corner = 0; corner < 8; ++corner) {
            const glm::vec3 local{(corner & 1) ? localMax.x : localMin.x,
                (corner & 2) ? localMax.y : localMin.y, (corner & 4) ? localMax.z : localMin.z};
            const glm::vec3 world = glm::vec3(transform * glm::vec4(local, 1.0F));
            bounds.minimum = glm::min(bounds.minimum, world);
            bounds.maximum = glm::max(bounds.maximum, world);
        }
        result.push_back(std::move(bounds));
    }
    return result;
}

void ModelRenderer::setShadowBias(float bias) noexcept {
    if (std::isfinite(bias)) shadowBias_ = std::clamp(bias, 0.0001F, 0.02F);
}

void ModelRenderer::setNormalOffsetBias(float bias) noexcept {
    if (std::isfinite(bias)) normalOffsetBias_ = std::clamp(bias, 0.0F, 0.1F);
}

void ModelRenderer::setCascadeLambda(float lambda) noexcept {
    if (std::isfinite(lambda)) cascadeLambda_ = std::clamp(lambda, 0.0F, 1.0F);
}

void ModelRenderer::setShadowFar(float distance) noexcept {
    if (std::isfinite(distance)) shadowFar_ = std::clamp(distance, 20.0F, 300.0F);
}

void ModelRenderer::setFogDensity(float density) noexcept {
    if (std::isfinite(density)) fogDensity_ = std::clamp(density, 0.0F, 0.05F);
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

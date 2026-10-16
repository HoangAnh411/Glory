#include "glory/engine/application.hpp"

#include "glory/engine/camera.hpp"
#include "glory/engine/third_person_camera.hpp"
#include "glory/engine/debug_panel.hpp"
#include "glory/engine/directional_light.hpp"
#include "glory/engine/fixed_timestep.hpp"
#include "glory/engine/input_system.hpp"
#include "glory/engine/model_renderer.hpp"
#include "glory/engine/audio_system.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <exception>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace glory::engine {
namespace {

bool setOpenGLAttribute(SDL_GLAttr attribute, int value) {
    if (SDL_GL_SetAttribute(attribute, value)) {
        return true;
    }

    std::cerr << "SDL_GL_SetAttribute failed: " << SDL_GetError() << '\n';
    return false;
}

} // namespace

int runApplication(const ApplicationConfig& config, const FixedUpdateCallback& fixedUpdate) {
    if (config.windowWidth <= 0 || config.windowHeight <= 0 || !fixedUpdate) {
        std::cerr << "Invalid application configuration\n";
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    if (!setOpenGLAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) ||
        !setOpenGLAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5) ||
        !setOpenGLAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
        !setOpenGLAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
        !setOpenGLAttribute(SDL_GL_DEPTH_SIZE, 24)) {
        SDL_Quit();
        return 1;
    }

    SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (config.hiddenWindow) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }
    SDL_Window* window = SDL_CreateWindow(
        "Glory - Lam Vuc",
        config.windowWidth,
        config.windowHeight,
        windowFlags);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_GL_MakeCurrent(window, context)) {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int actualMajorVersion = 0;
    int actualMinorVersion = 0;
    int actualProfile = 0;
    const bool versionRead =
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &actualMajorVersion) &&
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &actualMinorVersion) &&
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &actualProfile);
    const bool versionSupported = actualMajorVersion > 4 ||
        (actualMajorVersion == 4 && actualMinorVersion >= 5);
    if (!versionRead || !versionSupported || actualProfile != SDL_GL_CONTEXT_PROFILE_CORE) {
        std::cerr << "OpenGL 4.5 Core context required; received "
                  << actualMajorVersion << '.' << actualMinorVersion
                  << " profile " << actualProfile << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!config.hiddenWindow && !SDL_ShowWindow(window)) {
        std::cerr << "SDL_ShowWindow failed: " << SDL_GetError() << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const int swapInterval = config.smokeTestTickLimit == 0 ? 1 : 0;
    if (!SDL_GL_SetSwapInterval(swapInterval)) {
        std::cerr << "SDL_GL_SetSwapInterval warning: " << SDL_GetError() << '\n';
    }

    const std::uint64_t frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) {
        std::cerr << "SDL_GetPerformanceFrequency returned zero\n";
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    FixedTimestep timestep;
    InputSystem inputSystem;
    FreeCamera camera;
    ThirdPersonCamera thirdPersonCamera;
    ThirdPersonCameraConfig legacyCameraConfig;
    legacyCameraConfig.distance = 4.5F;
    legacyCameraConfig.shoulderOffset = 0.0F;
    legacyCameraConfig.targetOffset = {0.0F, 1.2F, 0.0F};
    legacyCameraConfig.fieldOfViewDegrees = 45.0F;
    ThirdPersonCamera legacyThirdPersonCamera(legacyCameraConfig);
    DirectionalLight light;
    PlayerFrameState player;
    bool thirdPersonMode = true;
    bool enhancedCamera = true;
    bool targetLockActive = false;
    bool attackActive = false;
    bool resetBossRequested = false;
    std::string requestedSkillId;
    std::string statusCommand;
    std::vector<SkillEditorSkill> editorSkills = config.editorSkills;
    std::vector<WorldEditorSpawnPoint> worldSpawns = config.worldSpawns;
    std::vector<WorldEditorTriggerZone> worldTriggers = config.worldTriggers;
    float attackRemaining = 0.0F;
    std::uint64_t completedTicks = 0;
    std::uint64_t previousCounter = SDL_GetPerformanceCounter();
    bool running = true;
        bool animationFrozenThisFrame = false;
    int result = 0;
    try {
        DebugPanel debugPanel(window, context);
        RenderPipeline renderPipeline;
        if (config.smokeTestTickLimit != 0) {
            int smokeWidth = 0;
            int smokeHeight = 0;
            SDL_GetWindowSizeInPixels(window, &smokeWidth, &smokeHeight);
            renderPipeline.beginFrame(smokeWidth, smokeHeight);
            renderPipeline.beginFrame(std::max(1, smokeWidth / 2), std::max(1, smokeHeight / 2));
            renderPipeline.beginFrame(smokeWidth, smokeHeight);
            if (!renderPipeline.framebufferComplete() || renderPipeline.resizeCount() < 2)
                throw std::runtime_error("HDR framebuffer smoke resize check failed");
        }
        ModelRenderer renderer(config.modelPath);
        ModelRenderer bossRenderer(config.bossModelPath);
        struct RemoteRenderer {
            std::unique_ptr<ModelRenderer> model;
            std::string animationState;
        };
        std::unordered_map<std::uint32_t, RemoteRenderer> remoteRenderers;
        bossRenderer.setPbrMaterial(0.75F, 0.32F, 1.0F);
        AudioSystem audio(config.audioDirectory);
        (void)renderer.triggerAnimationState("Idle");
        if (config.smokeTestTickLimit != 0) {
            std::cout << "Smoke audio engine: " << (audio.available() ? "ready" : "unavailable") << '\n';
        }
        renderer.setAnimationEventCallback([&](const AnimationEvent& event) {
            debugPanel.logAnimationEvent(event);
            if (event.name == "swing_sound") audio.playSwing();
        });
        if (config.smokeTestTickLimit != 0) {
            (void)renderer.triggerAnimationState("Attack");
            (void)renderer.spawnParticleBurst({0.0F, -0.15F, 0.0F}, 64);
            audio.playSwing();
            audio.playHit();
        }
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                debugPanel.processEvent(event);
                inputSystem.processEvent(event);
                if (event.type == SDL_EVENT_QUIT ||
                    event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                    (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                    running = false;
                }
            }

            const bool captureMouse = config.smokeTestTickLimit == 0 && !debugPanel.wantsCaptureMouse();
            (void)SDL_SetWindowRelativeMouseMode(window, captureMouse);

            const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
            const double frameSeconds = static_cast<double>(currentCounter - previousCounter) /
                static_cast<double>(frequency);
            previousCounter = currentCounter;
            animationFrozenThisFrame = player.hitStopActive;

            timestep.advance(frameSeconds, [&](double fixedDeltaSeconds) {
                if (config.smokeTestTickLimit != 0 &&
                    completedTicks >= config.smokeTestTickLimit) {
                    return;
                }
                const input::InputFrame inputFrame = inputSystem.consumeFrame();
                animationFrozenThisFrame = animationFrozenThisFrame || player.hitStopActive;
                input::InputFrame gameplayInput = inputFrame;
                if (debugPanel.wantsCaptureKeyboard()) {
                    gameplayInput.held.clear();
                    gameplayInput.pressed.clear();
                    gameplayInput.released.clear();
                }
                if (debugPanel.wantsCaptureMouse()) {
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                }
                if (std::find(gameplayInput.pressed.begin(), gameplayInput.pressed.end(), input::Control::KeyF1) !=
                    gameplayInput.pressed.end()) {
                    thirdPersonMode = !thirdPersonMode;
                }
                if (std::find(gameplayInput.pressed.begin(), gameplayInput.pressed.end(), input::Control::KeyTab) !=
                    gameplayInput.pressed.end()) {
                    targetLockActive = !targetLockActive;
                }
                if (player.bossDefeated) targetLockActive = false;
                thirdPersonCamera.setLocked(targetLockActive, player.bossPosition);
                legacyThirdPersonCamera.setLocked(targetLockActive, player.bossPosition);
                if (thirdPersonMode) {
                    thirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY);
                    legacyThirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY);
                } else {
                    camera.update(gameplayInput, fixedDeltaSeconds);
                }
                const float cameraYaw = thirdPersonMode ? thirdPersonCamera.yawDegrees() : camera.yawDegrees();
                player = fixedUpdate(fixedDeltaSeconds, gameplayInput, cameraYaw, resetBossRequested,
                    requestedSkillId, statusCommand);
                player.targetLocked = targetLockActive;
                player.targetLockPosition = player.bossPosition;
                animationFrozenThisFrame = animationFrozenThisFrame || player.hitStopActive;
                resetBossRequested = false;
                requestedSkillId.clear();
                statusCommand.clear();
                for (const CombatHitVisualEvent& hit : player.hitEvents) {
                    (void)renderer.spawnParticleBurst(hit.position, 64);
                    audio.playHit();
                }
                if (player.bossEnrageTriggered) {
                    (void)renderer.spawnParticleBurst(player.bossPosition + glm::vec3(0.0F, 1.0F, 0.0F), 96);
                    audio.playSwing();
                }
                if (player.bossAttackHit) {
                    (void)renderer.spawnParticleBurst(player.position + glm::vec3(0.0F, 0.8F, 0.0F), 32);
                    audio.playHit();
                }
                thirdPersonCamera.setTarget(player.position);
                legacyThirdPersonCamera.setTarget(player.position);
                if (player.attackCanceled) {
                    attackActive = false;
                    const char* locomotion = player.isMoving ? "Run" : "Idle";
                    (void)renderer.triggerAnimationState(locomotion);
                } else if (player.attackRequested) {
                    if (renderer.triggerAnimationState("Attack")) {
                        attackActive = true;
                        attackRemaining = renderer.animationDuration();
                        audio.playSwing();
                    }
                } else if (attackActive && !animationFrozenThisFrame) {
                    attackRemaining -= static_cast<float>(fixedDeltaSeconds);
                    if (attackRemaining <= 0.0F) attackActive = false;
                }
                if (!attackActive) {
                    const char* locomotion = player.isMoving ? "Run" : "Idle";
                    if (renderer.animationState() != locomotion)
                        (void)renderer.triggerAnimationState(locomotion);
                }
                ++completedTicks;
            });

            if (config.smokeTestTickLimit != 0 && completedTicks >= config.smokeTestTickLimit) {
                running = false;
            }

            int drawableWidth = 0;
            int drawableHeight = 0;
            SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
            if (drawableWidth > 0 && drawableHeight > 0) {
                camera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
                thirdPersonCamera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
                legacyThirdPersonCamera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
            }
            if (thirdPersonMode && enhancedCamera) {
                const float armLimit = renderer.cameraObstructionDistance(
                    thirdPersonCamera.targetPosition(), thirdPersonCamera.desiredPosition());
                thirdPersonCamera.constrainArm(armLimit, static_cast<float>(frameSeconds));
            }
            const ThirdPersonCamera& activeThirdPersonCamera = enhancedCamera ?
                thirdPersonCamera : legacyThirdPersonCamera;
            const glm::vec3 cameraPosition = thirdPersonMode ? activeThirdPersonCamera.position() : camera.position();
            const float cameraYaw = thirdPersonMode ? activeThirdPersonCamera.yawDegrees() : camera.yawDegrees();
            const float cameraPitch = thirdPersonMode ? activeThirdPersonCamera.pitchDegrees() : camera.pitchDegrees();
            const glm::mat4 view = thirdPersonMode ? activeThirdPersonCamera.viewMatrix() : camera.viewMatrix();
            const glm::mat4 projection = thirdPersonMode ? activeThirdPersonCamera.projectionMatrix() : camera.projectionMatrix();
            debugPanel.setViewProjection(projection * view);

            renderer.setTransform(player.position, player.facingYaw);
            if (!animationFrozenThisFrame) renderer.updateAnimation(static_cast<float>(frameSeconds));
            debugPanel.beginFrame(static_cast<float>(frameSeconds), completedTicks, cameraPosition,
                cameraYaw, cameraPitch, thirdPersonMode, enhancedCamera, renderPipeline,
                player, resetBossRequested,
                requestedSkillId,
                statusCommand,
                editorSkills, config.saveSkills, config.reloadSkills,
                worldSpawns, worldTriggers, config.saveWorld, config.reloadWorld,
                config.matchmaking,
                config.accountPanel, config.accountAction,
                config.inventoryPanel, config.inventoryAction, config.zonePanel, config.zoneChange,
                config.socialPanel, config.socialAction,
                config.dungeonPanel, config.dungeonAction,
                config.marketPanel, config.marketAction,
                config.feedbackPanel, config.feedbackSubmit,
                light, renderer, audio);
            renderPipeline.beginFrame(drawableWidth, drawableHeight);
            renderer.render(view, projection, cameraPosition, light, drawableWidth, drawableHeight, renderPipeline);
            std::unordered_set<std::uint32_t> visibleRemoteIds;
            renderPipeline.beginPass(GpuPass::ParticleOverlay);
            for (const auto& remote : player.remotePlayers) {
                visibleRemoteIds.insert(remote.clientId);
                auto [entry, inserted] = remoteRenderers.try_emplace(remote.clientId);
                if (inserted) {
                    entry->second.model = std::make_unique<ModelRenderer>(config.modelPath);
                    entry->second.model->setSkyboxEnabled(false);
                }
                ModelRenderer& remoteModel = *entry->second.model;
                remoteModel.setShadowsEnabled(renderer.shadowsEnabled());
                remoteModel.setCharacterLightingEnabled(renderer.characterLightingEnabled());
                remoteModel.setFogEnabled(renderer.fogEnabled());
                remoteModel.setFogDensity(renderer.fogDensity());
                remoteModel.setHemisphereEnabled(renderer.hemisphereEnabled());
                if (entry->second.animationState != remote.animationState &&
                    remoteModel.triggerAnimationState(remote.animationState))
                    entry->second.animationState = remote.animationState;
                remoteModel.updateAnimation(static_cast<float>(frameSeconds));
                const std::uint32_t colorIndex = remote.clientId % 3;
                const glm::vec3 tint = colorIndex == 0 ? glm::vec3(0.45F, 0.72F, 1.0F) :
                    (colorIndex == 1 ? glm::vec3(1.0F, 0.55F, 0.38F) : glm::vec3(0.55F, 1.0F, 0.62F));
                remoteModel.renderOverlay(view, projection, cameraPosition, light,
                    remote.position, remote.yaw, 1.0F, tint, !renderPipeline.hdrEnabled(),
                    drawableWidth, drawableHeight, renderPipeline);
            }
            std::erase_if(remoteRenderers, [&visibleRemoteIds](const auto& entry) {
                return !visibleRemoteIds.contains(entry.first);
            });
            bossRenderer.setShadowsEnabled(renderer.shadowsEnabled());
            bossRenderer.setCharacterLightingEnabled(renderer.characterLightingEnabled());
            bossRenderer.setFogEnabled(renderer.fogEnabled());
            bossRenderer.setFogDensity(renderer.fogDensity());
            bossRenderer.setHemisphereEnabled(renderer.hemisphereEnabled());
            if (!player.bossDefeated)
                bossRenderer.renderOverlay(view, projection, cameraPosition, light,
                    player.bossPosition, player.bossYaw, 1.8F, glm::vec3(1.15F, 0.45F, 0.45F),
                    !renderPipeline.hdrEnabled(), drawableWidth, drawableHeight, renderPipeline);
            renderPipeline.endPass();
            renderPipeline.finishFrame();
            debugPanel.render();
            SDL_GL_SwapWindow(window);
        }
        if (config.smokeTestTickLimit != 0) {
            std::cout << "OpenGL renderer: " << renderPipeline.rendererInfo() << '\n';
            int smokeWidth = 0;
            int smokeHeight = 0;
            SDL_GetWindowSizeInPixels(window, &smokeWidth, &smokeHeight);
            std::cout << "Smoke drawable resolution: " << smokeWidth << 'x' << smokeHeight << '\n';
            std::cout << "HDR framebuffer complete; resize recreations: " << renderPipeline.resizeCount() << '\n';
            if (renderPipeline.timerQueriesAvailable() && !renderPipeline.history().empty()) {
                std::array<double, static_cast<std::size_t>(GpuPass::Count)> totals{};
                for (const GpuTimingSample& sample : renderPipeline.history())
                    for (std::size_t pass = 0; pass < totals.size(); ++pass) totals[pass] += sample.milliseconds[pass];
                std::cout << std::fixed << std::setprecision(3)
                          << "GPU pass averages (ms, " << renderPipeline.history().size() << " frames):";
                constexpr const char* names[] = {"shadow", "scene", "sky", "particle_overlay", "tonemap", "gpu_frame"};
                for (std::size_t pass = 0; pass < totals.size(); ++pass)
                    std::cout << ' ' << names[pass] << '=' << totals[pass] / renderPipeline.history().size();
                std::cout << '\n';
            } else {
                std::cout << renderPipeline.timerStatus() << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "OpenGL renderer failed: " << error.what() << '\n';
        result = 1;
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

} // namespace glory::engine

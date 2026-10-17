#include "glory/engine/application.hpp"

#include "glory/engine/camera.hpp"
#include "glory/engine/third_person_camera.hpp"
#include "glory/engine/debug_panel.hpp"
#include "glory/engine/directional_light.hpp"
#include "glory/engine/fixed_timestep.hpp"
#include "glory/engine/input_system.hpp"
#include "glory/engine/model_renderer.hpp"
#include "glory/engine/audio_system.hpp"
#include "glory/engine/gl_api.hpp"

#include <SDL3/SDL.h>
#include <stb_image_write.h>
#include <nlohmann/json.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <exception>
#include <memory>
#include <fstream>
#include <filesystem>
#include <vector>
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
    if (config.highPixelDensity) windowFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
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
    ThirdPersonCameraConfig orbitCameraConfig;
    int attackAimMode = 1;
    const std::filesystem::path cameraSettingsPath = "settings/camera.json";
    try {
        if (std::filesystem::exists(cameraSettingsPath)) {
            std::ifstream settingsFile(cameraSettingsPath);
            const nlohmann::json settings = nlohmann::json::parse(settingsFile);
            orbitCameraConfig.distance = settings.value("distance", orbitCameraConfig.distance);
            orbitCameraConfig.fieldOfViewDegrees = settings.value("fov", orbitCameraConfig.fieldOfViewDegrees);
            orbitCameraConfig.shoulderOffset = settings.value("shoulder", orbitCameraConfig.shoulderOffset);
            orbitCameraConfig.mouseSensitivity = settings.value("mouse_sensitivity", orbitCameraConfig.mouseSensitivity);
            orbitCameraConfig.invertMouseY = settings.value("invert_mouse_y", orbitCameraConfig.invertMouseY);
            orbitCameraConfig.autoReturnEnabled = settings.value("auto_return", orbitCameraConfig.autoReturnEnabled);
            attackAimMode = std::clamp(settings.value("attack_aim_mode", attackAimMode), 0, 2);
        }
    } catch (const std::exception& error) {
        std::cerr << "Ignoring invalid camera settings: " << error.what() << '\n';
    }
    ThirdPersonCamera thirdPersonCamera(orbitCameraConfig);
    ThirdPersonCameraConfig legacyCameraConfig;
    legacyCameraConfig.distance = 4.5F;
    legacyCameraConfig.shoulderOffset = 0.0F;
    legacyCameraConfig.framingOffset = 0.0F;
    legacyCameraConfig.targetOffset = {0.0F, 1.2F, 0.0F};
    legacyCameraConfig.fieldOfViewDegrees = 45.0F;
    ThirdPersonCamera legacyThirdPersonCamera(legacyCameraConfig);
    DirectionalLight light;
    PlayerFrameState player;
    bool thirdPersonMode = true;
    bool enhancedCamera = true;
    bool windowFocused = true;
    bool cursorReleasedByEscape = false;
    bool relativeMouseActive = false;
    bool ignoreNextMouseFrame = true;
    bool targetLockActive = false;
    glm::vec3 targetLockPosition{0.0F};
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
    std::size_t captureIndex = 0;
    double cameraTestElapsed = 0.0;
    double cameraTestPhaseElapsed = 0.0;
    int cameraTestPhase = -1;
    bool cameraTestWheelSent = false;
    bool cameraTestCaptureWritten = false;
    std::uint64_t cameraTestRenderCounter = 0;
    double cameraTestFixedPhaseElapsed = 0.0;
    double cameraTestMousePixelsSent = 0.0;
    std::array<float, 3> cameraTestYawResults{0.0F, 0.0F, 0.0F};
    float cameraTestMinimumPitch = std::numeric_limits<float>::max();
    float cameraTestMaximumPitch = std::numeric_limits<float>::lowest();
    const std::array<double, 6> cameraTestPhaseDurations{4.0, 4.0, 4.0, 4.0, 0.5, 0.5};
    const std::array<int, 6> cameraTestPhaseFps{30, 60, 144, 60, 60, 60};
    const std::array<const char*, 6> cameraTestCaptureNames{
        "yaw_30fps", "yaw_60fps", "yaw_144fps", "pitch_limits", "zoom_range", "wall_arm"};
    double combatTestElapsed = 0.0;
    double combatTestPhaseElapsed = 0.0;
    int combatTestPhase = -1;
    bool combatTestCaptureWritten = false;
    bool combatTestPressSent = false;
    bool combatTelemetryReferenceCaptured = false;
    bool combatTelemetryPass = true;
    float combatReferenceDamage = -1.0F;
    float combatReferenceCooldown = -1.0F;
    float combatReferenceHitboxRadius = -1.0F;
    std::array<bool, 6> combatTestPasses{};
    const std::array<const char*, 6> combatTestCaptureNames{
        "manual_attack", "assist_attack", "auto_attack", "lock_frame", "lock_cancel", "lock_hold"};
    int lastLoggedWindowWidth = -1;
    int lastLoggedWindowHeight = -1;
    int lastLoggedDrawableWidth = -1;
    int lastLoggedDrawableHeight = -1;
    bool projectionDiagnosticPending = true;
    int result = 0;
    try {
        DebugPanel debugPanel(window, context);
        bool noclipEnabled = false;
        debugPanel.setCollisionWireframeEnabled(config.captureCollisionDebug);
        RenderPipeline renderPipeline;
        if (config.captureMode && !config.captureHdrEnabled) renderPipeline.setHdrEnabled(false);
        std::unique_ptr<GlApi> captureGl;
        if (config.captureMode) captureGl = std::make_unique<GlApi>();
        if (config.smokeTestTickLimit != 0 && !config.captureMode) {
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
        std::vector<CameraCollider> cameraColliders;
        cameraColliders.reserve(config.collisionDebugBoxes.size());
        for (const auto& box : config.collisionDebugBoxes)
            cameraColliders.push_back({box.minimum, box.maximum, box.name, box.blocksMovement});
        renderer.setCameraColliders(std::move(cameraColliders), config.arenaCenter, config.arenaRadius);
        std::cout << "[collision-bounds] rendered static prop AABBs versus loaded collision AABBs (m)\n";
        for (const auto& rendered : renderer.renderedWorldPropBounds()) {
            glm::vec3 colliderMin(std::numeric_limits<float>::max());
            glm::vec3 colliderMax(std::numeric_limits<float>::lowest());
            bool foundCollider = false;
            bool blocksMovement = true;
            for (const auto& box : config.collisionDebugBoxes) {
                if (box.name != rendered.name) continue;
                colliderMin = glm::min(colliderMin, box.minimum);
                colliderMax = glm::max(colliderMax, box.maximum);
                blocksMovement = blocksMovement && box.blocksMovement;
                foundCollider = true;
            }
            if (!foundCollider) {
                std::cout << "[collision-bounds] MISSING " << rendered.name << " render=["
                          << rendered.minimum.x << ',' << rendered.minimum.y << ',' << rendered.minimum.z << ";"
                          << rendered.maximum.x << ',' << rendered.maximum.y << ',' << rendered.maximum.z << "]\n";
                continue;
            }
            const glm::vec3 error = glm::max(glm::abs(rendered.minimum - colliderMin),
                glm::abs(rendered.maximum - colliderMax));
            const float maxError = std::max({error.x, error.y, error.z});
            const bool treeTrunk = rendered.name.starts_with("tree_");
            const char* status = !blocksMovement ? "NON-BLOCKING " :
                (treeTrunk ? "AUTHORED-TRUNK " : (maxError > 0.15F ? "MISMATCH " : "OK "));
            std::cout << "[collision-bounds] " << status
                      << rendered.name << " render=[" << rendered.minimum.x << ',' << rendered.minimum.y << ','
                      << rendered.minimum.z << ';' << rendered.maximum.x << ',' << rendered.maximum.y << ','
                      << rendered.maximum.z << "] collider=[" << colliderMin.x << ',' << colliderMin.y << ','
                      << colliderMin.z << ';' << colliderMax.x << ',' << colliderMax.y << ',' << colliderMax.z
                      << "] maxError=" << maxError << " m\n";
        }
        struct RemoteRenderer {
            std::unique_ptr<ModelRenderer> model;
            std::string animationState;
        };
        std::unordered_map<std::uint32_t, RemoteRenderer> remoteRenderers;
        bossRenderer.setPbrMaterial(0.75F, 0.32F, 1.0F);
        AudioSystem audio(config.audioDirectory);
        (void)renderer.triggerAnimationState("Idle");
        if (config.smokeTestTickLimit != 0 && !config.captureMode) {
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
                if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) windowFocused = false;
                if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) windowFocused = true;
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) cursorReleasedByEscape = false;
                if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                    running = false;
                }
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                    cursorReleasedByEscape = true;
            }

            const bool altHeld = (SDL_GetModState() & SDL_KMOD_ALT) != 0;
            const bool captureMouse = config.smokeTestTickLimit == 0 && windowFocused && !altHeld &&
                !cursorReleasedByEscape && !debugPanel.isDevToolsVisible() && !debugPanel.isFeedbackOpen();
            if (captureMouse != relativeMouseActive) {
                (void)SDL_SetWindowRelativeMouseMode(window, captureMouse);
                relativeMouseActive = captureMouse;
                ignoreNextMouseFrame = true;
                inputSystem.discardMouseDelta();
            }

            if (config.cameraTestMode) {
                double phaseStart = 0.0;
                int nextPhase = 0;
                while (nextPhase < static_cast<int>(cameraTestPhaseDurations.size()) - 1 &&
                    cameraTestElapsed >= phaseStart + cameraTestPhaseDurations[nextPhase]) {
                    phaseStart += cameraTestPhaseDurations[nextPhase];
                    ++nextPhase;
                }
                if (cameraTestPhase >= 0 && !cameraTestCaptureWritten && nextPhase != cameraTestPhase) {
                    nextPhase = cameraTestPhase;
                    cameraTestPhaseElapsed = cameraTestPhaseDurations[cameraTestPhase];
                }
                if (nextPhase != cameraTestPhase) {
                    cameraTestPhase = nextPhase;
                    if (config.cameraTestMode)
                        std::cout << "[camera-test] phase " << cameraTestPhase << " fps "
                                  << cameraTestPhaseFps[cameraTestPhase] << '\n';
                    cameraTestPhaseElapsed = 0.0;
                    cameraTestWheelSent = false;
                    cameraTestCaptureWritten = false;
                    cameraTestFixedPhaseElapsed = 0.0;
                    cameraTestMousePixelsSent = 0.0;
                    cameraTestMinimumPitch = std::numeric_limits<float>::max();
                    cameraTestMaximumPitch = std::numeric_limits<float>::lowest();
                    player.position = cameraTestPhase == 5 ? glm::vec3{-6.0F, 0.0F, 7.0F} : glm::vec3{0.0F};
                    player.isMoving = false;
                    targetLockActive = false;
                    thirdPersonCamera.setTarget(player.position);
                    thirdPersonCamera.setOrbitPose(cameraTestPhase == 5 ? 0.0F : -90.0F,
                        cameraTestPhase == 3 ? 15.0F : 15.0F);
                    thirdPersonCamera.setZoom(cameraTestPhase == 4 ? 3.0F : 5.0F);
                    thirdPersonCamera.constrainArm(8.0F, 1.0F);
                }
            }
            if (config.combatTestMode) {
                const std::array<double, 6> phaseDurations{0.7, 0.7, 0.7, 1.0, 0.7, 1.0};
                double phaseStart = 0.0;
                int nextPhase = 0;
                while (nextPhase < static_cast<int>(phaseDurations.size()) - 1 &&
                    combatTestElapsed >= phaseStart + phaseDurations[nextPhase]) {
                    phaseStart += phaseDurations[nextPhase];
                    ++nextPhase;
                }
                if (combatTestPhase >= 0 && !combatTestCaptureWritten && nextPhase != combatTestPhase) {
                    nextPhase = combatTestPhase;
                    combatTestPhaseElapsed = phaseDurations[combatTestPhase];
                }
                if (nextPhase != combatTestPhase) {
                    combatTestPhase = nextPhase;
                    combatTestPhaseElapsed = 0.0;
                    combatTestCaptureWritten = false;
                    combatTestPressSent = false;
                    combatTelemetryReferenceCaptured = false;
                    combatTelemetryPass = true;
                    const std::array<float, 6> yaws{90.0F, 0.0F, 180.0F, 0.0F, 0.0F, 0.0F};
                    thirdPersonCamera.setOrbitPose(yaws[combatTestPhase], 15.0F);
                    thirdPersonCamera.setZoom(5.0F);
                    resetBossRequested = combatTestPhase < 4;
                    statusCommand = std::string("combat_setup_") +
                        (combatTestPhase == 0 ? "manual" : combatTestPhase == 1 ? "assist" :
                         combatTestPhase == 2 ? "auto" : combatTestPhase == 3 ? "lock" :
                         combatTestPhase == 4 ? "far" : "hold");
                    std::cout << "[combat-test] phase " << combatTestPhase << '\n';
                }
            }

            const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
            const double frameSeconds = static_cast<double>(currentCounter - previousCounter) /
                static_cast<double>(frequency);
            previousCounter = currentCounter;
            const double simulationFrameSeconds = config.cameraTestMode
                ? 1.0 / static_cast<double>(cameraTestPhaseFps[std::max(0, cameraTestPhase)])
                : (config.combatTestMode ? 1.0 / 60.0 : frameSeconds);
            animationFrozenThisFrame = player.hitStopActive;

            if (!config.captureMode || config.cameraTestMode || config.combatTestMode) timestep.advance(simulationFrameSeconds, [&](double fixedDeltaSeconds) {
                if (config.smokeTestTickLimit != 0 &&
                    completedTicks >= config.smokeTestTickLimit) {
                    return;
                }
                const input::InputFrame inputFrame = inputSystem.consumeFrame();
                animationFrozenThisFrame = animationFrozenThisFrame || player.hitStopActive;
                input::InputFrame gameplayInput = inputFrame;
                if (config.combatTestMode) {
                    gameplayInput.held.clear();
                    gameplayInput.pressed.clear();
                    gameplayInput.released.clear();
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                    gameplayInput.mouseWheelY = 0.0F;
                    combatTestPhaseElapsed += fixedDeltaSeconds;
                    if (!combatTestPressSent) {
                        if (combatTestPhase <= 2) {
                            gameplayInput.pressed.push_back(input::Control::MouseLeft);
                            gameplayInput.held.push_back(input::Control::MouseLeft);
                        } else if (combatTestPhase == 3 || combatTestPhase == 5) {
                            gameplayInput.pressed.push_back(input::Control::KeyTab);
                            gameplayInput.held.push_back(input::Control::KeyTab);
                        }
                        combatTestPressSent = true;
                    }
                }
                if (ignoreNextMouseFrame) {
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                    ignoreNextMouseFrame = false;
                }
                if (debugPanel.wantsCaptureKeyboard()) {
                    gameplayInput.held.clear();
                    gameplayInput.pressed.clear();
                    gameplayInput.released.clear();
                }
                if (debugPanel.wantsCaptureMouse()) {
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                }
                if (config.cameraTestMode) {
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                    gameplayInput.mouseWheelY = 0.0F;
                    cameraTestFixedPhaseElapsed += fixedDeltaSeconds;
                    const double phaseTime = cameraTestFixedPhaseElapsed;
                    if (cameraTestPhase <= 2) {
                        const double targetPixels = 360.0 / orbitCameraConfig.mouseSensitivity;
                        const double desiredPixels = targetPixels * std::clamp(phaseTime / 4.0, 0.0, 1.0);
                        gameplayInput.mouseDeltaX = static_cast<float>(desiredPixels - cameraTestMousePixelsSent);
                        cameraTestMousePixelsSent = desiredPixels;
                    } else if (cameraTestPhase == 3) {
                        const float pitchDelta = phaseTime < 2.0 ? 55.0F : -80.0F;
                        gameplayInput.mouseDeltaY = pitchDelta / orbitCameraConfig.mouseSensitivity *
                            static_cast<float>(fixedDeltaSeconds);
                    } else if (cameraTestPhase == 4 && !cameraTestWheelSent) {
                        gameplayInput.mouseWheelY = -10.0F;
                        cameraTestWheelSent = true;
                    }
                }
                if (!relativeMouseActive || !windowFocused || (SDL_GetModState() & SDL_KMOD_ALT) != 0 ||
                    debugPanel.isDevToolsVisible() || debugPanel.isFeedbackOpen()) {
                    gameplayInput.mouseDeltaX = 0.0F;
                    gameplayInput.mouseDeltaY = 0.0F;
                }
                if (std::find(gameplayInput.pressed.begin(), gameplayInput.pressed.end(), input::Control::KeyF1) !=
                    gameplayInput.pressed.end()) {
                    thirdPersonMode = !thirdPersonMode;
                }
                if (player.bossDefeated) targetLockActive = false;
                thirdPersonCamera.setLocked(targetLockActive, player.bossPosition);
                legacyThirdPersonCamera.setLocked(targetLockActive, player.bossPosition);
                if (thirdPersonMode) {
                    if (enhancedCamera) thirdPersonCamera.zoomBy(gameplayInput.mouseWheelY);
                    thirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY,
                        static_cast<float>(fixedDeltaSeconds));
                    legacyThirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY,
                        static_cast<float>(fixedDeltaSeconds));
                    if (config.cameraTestMode && cameraTestPhase == 3) {
                        cameraTestMinimumPitch = std::min(cameraTestMinimumPitch, thirdPersonCamera.pitchDegrees());
                        cameraTestMaximumPitch = std::max(cameraTestMaximumPitch, thirdPersonCamera.pitchDegrees());
                    }
                } else {
                    camera.update(gameplayInput, fixedDeltaSeconds);
                }
                const float cameraYaw = thirdPersonMode ? thirdPersonCamera.yawDegrees() : camera.yawDegrees();
                player = fixedUpdate(fixedDeltaSeconds, gameplayInput, cameraYaw, resetBossRequested,
                    requestedSkillId, statusCommand, noclipEnabled);
                if (config.cameraTestMode && cameraTestPhase == 5)
                    player.position = {-6.0F, 0.0F, 7.0F};
                targetLockActive = player.targetLocked;
                targetLockPosition = player.targetLockPosition;
                if (config.combatTestMode && combatTestPhase >= 0 && combatTestPhase <= 2 &&
                    !combatTelemetryReferenceCaptured) {
                    if (combatTestPhase == 0) {
                        combatReferenceDamage = player.activeSkillDamage;
                        combatReferenceCooldown = player.activeSkillCooldown;
                        combatReferenceHitboxRadius = player.activeSkillHitboxRadius;
                    } else {
                        combatTelemetryPass = std::abs(player.activeSkillDamage - combatReferenceDamage) <= 1.0e-5F &&
                            std::abs(player.activeSkillCooldown - combatReferenceCooldown) <= 1.0e-5F &&
                            std::abs(player.activeSkillHitboxRadius - combatReferenceHitboxRadius) <= 1.0e-5F;
                    }
                    combatTelemetryReferenceCaptured = true;
                }
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

            if (config.cameraTestMode) {
                cameraTestPhaseElapsed += simulationFrameSeconds;
                cameraTestElapsed += simulationFrameSeconds;
            }
            if (config.combatTestMode) {
                combatTestElapsed += simulationFrameSeconds;
                combatTestPhaseElapsed += simulationFrameSeconds;
            }

            // Keep the scripted FPS simulation while limiting expensive GPU frames.
            if (config.cameraTestMode && cameraTestPhase >= 0 &&
                cameraTestPhaseElapsed + 1.0e-9 < cameraTestPhaseDurations[cameraTestPhase] &&
                (++cameraTestRenderCounter % 32U) != 0U) {
                continue;
            }

            constexpr std::array captureNames{"center_boss", "pillar", "wall", "toward_sun", "away_from_sun",
                "boss_near", "arena_edge_0", "arena_edge_1", "arena_edge_2", "arena_edge_3",
                "arena_edge_4", "arena_edge_5", "arena_edge_6", "arena_edge_7"};
            if (config.captureMode && !config.cameraTestMode && !config.combatTestMode) {
                const std::size_t scenario = captureIndex;
                switch (scenario) {
                case 0: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 1: player.position = {5.0F, 0.0F, 10.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 2: player.position = {33.0F, 0.0F, 0.0F}; player.bossPosition = {25.0F, 0.0F, 0.0F}; break;
                case 3: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 4: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 5: player.position = {-2.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                default: {
                    const float angle = static_cast<float>(scenario - 6) * 0.7853981633974483F;
                    player.position = {std::cos(angle) * 33.0F, 0.0F, std::sin(angle) * 33.0F};
                    player.bossPosition = {0.0F, 0.0F, 0.0F};
                    break;
                }
                }
                player.playerHp = 150.0F;
                player.playerMaxHp = 150.0F;
                player.bossHp = 300.0F;
                player.bossMaxHp = 300.0F;
                player.bossDefeated = false;
                player.isMoving = false;
                targetLockActive = scenario == 0;
                std::array captureYaw{180.0F, -90.0F, 0.0F, 45.0F, -135.0F, 180.0F,
                    180.0F, -135.0F, -90.0F, -45.0F, 0.0F, 45.0F, 90.0F, 135.0F};
                if (scenario >= 6)
                    captureYaw[scenario] = std::atan2(-player.position.z, -player.position.x) * 57.2957795F;
                thirdPersonCamera.setTarget(player.position);
                thirdPersonCamera.setOrbitPose(captureYaw[scenario], 10.0F);
                thirdPersonCamera.setLocked(targetLockActive, player.bossPosition);
            }

            if (config.smokeTestTickLimit != 0 && completedTicks >= config.smokeTestTickLimit) {
                running = false;
            }

            int drawableWidth = 0;
            int drawableHeight = 0;
            SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
            int windowWidthPoints = 0;
            int windowHeightPoints = 0;
            SDL_GetWindowSize(window, &windowWidthPoints, &windowHeightPoints);
            const bool dimensionsChanged = windowWidthPoints != lastLoggedWindowWidth ||
                windowHeightPoints != lastLoggedWindowHeight || drawableWidth != lastLoggedDrawableWidth ||
                drawableHeight != lastLoggedDrawableHeight;
            if (dimensionsChanged) {
                const float dpiScaleX = windowWidthPoints > 0 ?
                    static_cast<float>(drawableWidth) / static_cast<float>(windowWidthPoints) : 0.0F;
                const float dpiScaleY = windowHeightPoints > 0 ?
                    static_cast<float>(drawableHeight) / static_cast<float>(windowHeightPoints) : 0.0F;
                std::cerr << "[render-diag] SDL points=" << windowWidthPoints << 'x' << windowHeightPoints
                          << " pixels=" << drawableWidth << 'x' << drawableHeight
                          << " dpiScale=" << dpiScaleX << 'x' << dpiScaleY
                          << " HDR attachment=" << renderPipeline.targetWidth() << 'x'
                          << renderPipeline.targetHeight() << '\n';
                lastLoggedWindowWidth = windowWidthPoints;
                lastLoggedWindowHeight = windowHeightPoints;
                lastLoggedDrawableWidth = drawableWidth;
                lastLoggedDrawableHeight = drawableHeight;
                projectionDiagnosticPending = true;
            }
            if (drawableWidth > 0 && drawableHeight > 0) {
                camera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
                thirdPersonCamera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
                legacyThirdPersonCamera.setAspectRatio(
                    static_cast<float>(drawableWidth) / static_cast<float>(drawableHeight));
            }
            if (thirdPersonMode && enhancedCamera) {
                if (config.cameraTestMode && cameraTestPhase <= 2 && !cameraTestCaptureWritten &&
                    cameraTestPhaseElapsed >= cameraTestPhaseDurations[cameraTestPhase]) {
                    const double targetPixels = 360.0 / orbitCameraConfig.mouseSensitivity;
                    const double residualPixels = targetPixels - cameraTestMousePixelsSent;
                    if (residualPixels > 0.0) {
                        thirdPersonCamera.update(player.position, static_cast<float>(residualPixels), 0.0F,
                            1.0F / 60.0F);
                        cameraTestMousePixelsSent = targetPixels;
                    }
                }
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
            if (projectionDiagnosticPending) {
                std::cerr << "[render-diag] scene projection (column-major):";
                for (int column = 0; column < 4; ++column)
                    for (int row = 0; row < 4; ++row)
                        std::cerr << ' ' << projection[column][row];
                std::cerr << "\n[render-diag] scene viewport source=SDL_GetWindowSizeInPixels "
                          << drawableWidth << 'x' << drawableHeight << '\n';
                projectionDiagnosticPending = false;
            }
            debugPanel.setViewProjection(projection * view);
            debugPanel.setCameraMarkerVisible(config.captureMode || debugPanel.isDevToolsVisible());

            CameraDebugMetrics cameraMetrics;
            cameraMetrics.position = cameraPosition;
            const float activeYawRadians = glm::radians(cameraYaw);
            const float activePitchRadians = glm::radians(cameraPitch);
            const glm::vec3 freeCameraForward{std::cos(activeYawRadians) * std::cos(activePitchRadians),
                std::sin(activePitchRadians), std::sin(activeYawRadians) * std::cos(activePitchRadians)};
            cameraMetrics.lookAt = thirdPersonMode ? activeThirdPersonCamera.targetPosition() :
                cameraPosition + freeCameraForward;
            cameraMetrics.playerPivot = player.position + glm::vec3(0.0F, 1.4F, 0.0F);
            cameraMetrics.desiredArmLength = thirdPersonMode ? activeThirdPersonCamera.distance() : 0.0F;
            cameraMetrics.actualArmLength = glm::length(cameraPosition - cameraMetrics.lookAt);
            cameraMetrics.appliedShoulderOffset = thirdPersonMode && enhancedCamera ?
                activeThirdPersonCamera.shoulderOffset() : 0.0F;
            cameraMetrics.compositionOffset = thirdPersonMode && enhancedCamera ?
                activeThirdPersonCamera.framingOffset() : 0.0F;
            cameraMetrics.springArmLateralSlide = 0.0F;
            cameraMetrics.blockingCollider = renderer.cameraBlockerName();
            cameraMetrics.targetLockActive = targetLockActive;
            const auto [boundsMin, boundsMax] = renderer.localBounds();
            const glm::mat4 playerTransform = glm::translate(glm::mat4(1.0F), player.position) *
                glm::rotate(glm::mat4(1.0F), glm::radians(player.facingYaw), glm::vec3(0.0F, 1.0F, 0.0F));
            const glm::mat4 playerViewProjection = projection * view * playerTransform;
            cameraMetrics.playerNdcMinimum = glm::vec2(std::numeric_limits<float>::max());
            cameraMetrics.playerNdcMaximum = glm::vec2(std::numeric_limits<float>::lowest());
            for (int corner = 0; corner < 8; ++corner) {
                const glm::vec3 localPoint{
                    (corner & 1) ? boundsMax.x : boundsMin.x,
                    (corner & 2) ? boundsMax.y : boundsMin.y,
                    (corner & 4) ? boundsMax.z : boundsMin.z};
                const glm::vec4 clip = playerViewProjection * glm::vec4(localPoint, 1.0F);
                if (!std::isfinite(clip.w) || clip.w <= 1.0e-6F) continue;
                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                cameraMetrics.playerNdcMinimum = glm::min(cameraMetrics.playerNdcMinimum, ndc);
                cameraMetrics.playerNdcMaximum = glm::max(cameraMetrics.playerNdcMaximum, ndc);
            }
            if (cameraMetrics.playerNdcMinimum.x <= cameraMetrics.playerNdcMaximum.x)
                cameraMetrics.playerNdc = (cameraMetrics.playerNdcMinimum + cameraMetrics.playerNdcMaximum) * 0.5F;
            const glm::vec4 pivotClip = projection * view * glm::vec4(cameraMetrics.playerPivot, 1.0F);
            if (std::isfinite(pivotClip.w) && pivotClip.w > 1.0e-6F)
                cameraMetrics.pivotNdc = glm::vec2(pivotClip) / pivotClip.w;
            const auto [bossBoundsMin, bossBoundsMax] = bossRenderer.localBounds();
            const glm::mat4 bossTransform = glm::translate(glm::mat4(1.0F), player.bossPosition) *
                glm::rotate(glm::mat4(1.0F), glm::radians(player.bossYaw), glm::vec3(0.0F, 1.0F, 0.0F)) *
                glm::scale(glm::mat4(1.0F), glm::vec3(1.8F));
            const glm::mat4 bossViewProjection = projection * view * bossTransform;
            cameraMetrics.bossNdcMinimum = glm::vec2(std::numeric_limits<float>::max());
            cameraMetrics.bossNdcMaximum = glm::vec2(std::numeric_limits<float>::lowest());
            for (int corner = 0; corner < 8; ++corner) {
                const glm::vec3 localPoint{
                    (corner & 1) ? bossBoundsMax.x : bossBoundsMin.x,
                    (corner & 2) ? bossBoundsMax.y : bossBoundsMin.y,
                    (corner & 4) ? bossBoundsMax.z : bossBoundsMin.z};
                const glm::vec4 clip = bossViewProjection * glm::vec4(localPoint, 1.0F);
                if (!std::isfinite(clip.w) || clip.w <= 1.0e-6F) continue;
                const glm::vec2 ndc = glm::vec2(clip) / clip.w;
                cameraMetrics.bossNdcMinimum = glm::min(cameraMetrics.bossNdcMinimum, ndc);
                cameraMetrics.bossNdcMaximum = glm::max(cameraMetrics.bossNdcMaximum, ndc);
            }
            cameraMetrics.playerAlive = player.playerHp > 0.0F;
            renderer.setCameraFade(cameraMetrics.actualArmLength < 2.0F
                ? std::clamp(cameraMetrics.actualArmLength / 2.0F, 0.15F, 1.0F) : 1.0F);

            renderer.setTransform(player.position, player.facingYaw);
            renderer.setDeadAppearance(player.playerDead);
            if (!animationFrozenThisFrame) renderer.updateAnimation(static_cast<float>(frameSeconds));
            debugPanel.setViewProjection(projection * view);
            debugPanel.setCollisionDebugData(config.collisionDebugBoxes, config.arenaCenter,
                config.arenaRadius, player.position);
            debugPanel.beginFrame(static_cast<float>(frameSeconds), completedTicks, cameraPosition,
                cameraYaw, cameraPitch, cameraMetrics, thirdPersonMode, enhancedCamera, noclipEnabled, thirdPersonCamera,
                renderPipeline,
                player, attackAimMode, resetBossRequested,
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
            glm::vec3 captureRootJoint{};
            bool captureMarkerInFrame = false;
            bool bossMarkerInFrame = false;
            glm::vec2 bossMarkerNdc{0.0F};
            if (config.captureMode) {
                 captureRootJoint = config.cameraTestMode || config.combatTestMode
                     ? player.position + glm::vec3(0.0F, 1.4F, 0.0F) : renderer.rootJointWorldPosition();
                const glm::vec3 markerWorld = captureRootJoint;
                const glm::vec4 markerClip = projection * view * glm::vec4(markerWorld, 1.0F);
                if (config.combatTestMode) {
                    const glm::vec2 markerNdc = cameraMetrics.pivotNdc;
                    captureMarkerInFrame = std::isfinite(markerNdc.x) && std::isfinite(markerNdc.y) &&
                        std::abs(markerNdc.x) <= 1.0F && std::abs(markerNdc.y) <= 1.0F;
                    bossMarkerNdc = (cameraMetrics.bossNdcMinimum + cameraMetrics.bossNdcMaximum) * 0.5F;
                    bossMarkerInFrame = std::isfinite(bossMarkerNdc.x) && std::isfinite(bossMarkerNdc.y) &&
                        std::abs(bossMarkerNdc.x) <= 1.0F && std::abs(bossMarkerNdc.y) <= 1.0F;
                    if (captureMarkerInFrame && bossMarkerInFrame)
                        renderPipeline.setCaptureMarkerNdcPair(markerNdc, bossMarkerNdc);
                    else if (captureMarkerInFrame) renderPipeline.setCaptureMarkerNdc(markerNdc);
                } else {
                    if (!std::isfinite(markerClip.w) || markerClip.w <= 1.0e-6F)
                        throw std::runtime_error("Capture root-joint marker is behind the camera");
                    const glm::vec2 markerNdc = glm::vec2(markerClip) / markerClip.w;
                    captureMarkerInFrame = std::abs(markerNdc.x) <= 1.0F && std::abs(markerNdc.y) <= 1.0F;
                    if (captureMarkerInFrame) renderPipeline.setCaptureMarkerNdc(markerNdc);
                }
            }
            std::unordered_set<std::uint32_t> visibleRemoteIds;
            std::vector<RenderActor> renderActors;
            for (const auto& remote : player.remotePlayers) {
                visibleRemoteIds.insert(remote.clientId);
                auto [entry, inserted] = remoteRenderers.try_emplace(remote.clientId);
                if (inserted) {
                    entry->second.model = std::make_unique<ModelRenderer>(config.modelPath);
                    entry->second.model->setSkyboxEnabled(false);
                }
                ModelRenderer& remoteModel = *entry->second.model;
                if (entry->second.animationState != remote.animationState &&
                    remoteModel.triggerAnimationState(remote.animationState))
                    entry->second.animationState = remote.animationState;
                remoteModel.updateAnimation(static_cast<float>(frameSeconds));
                const std::uint32_t colorIndex = remote.clientId % 3;
                const glm::vec3 tint = colorIndex == 0 ? glm::vec3(0.45F, 0.72F, 1.0F) :
                    (colorIndex == 1 ? glm::vec3(1.0F, 0.55F, 0.38F) : glm::vec3(0.55F, 1.0F, 0.62F));
                renderActors.push_back({&remoteModel, remote.position, remote.yaw, 1.0F, tint});
            }
            std::erase_if(remoteRenderers, [&visibleRemoteIds](const auto& entry) {
                return !visibleRemoteIds.contains(entry.first);
            });
            if (!player.bossDefeated)
                renderActors.push_back({&bossRenderer, player.bossPosition, player.bossYaw, 1.8F,
                    glm::vec3(1.15F, 0.45F, 0.45F)});
            renderer.render(view, projection, cameraPosition, light, drawableWidth, drawableHeight,
                renderPipeline, renderActors);
            renderPipeline.finishFrame();
            debugPanel.render();
            const bool cameraTestCaptureDue = config.cameraTestMode && !cameraTestCaptureWritten && cameraTestPhase >= 0 &&
                cameraTestPhaseElapsed + 1.0e-9 >= cameraTestPhaseDurations[cameraTestPhase];
            const std::array<double, 6> combatTestPhaseDurations{0.7, 0.7, 0.7, 1.0, 0.7, 1.0};
            const bool combatTestCaptureDue = config.combatTestMode && !combatTestCaptureWritten && combatTestPhase >= 0 &&
                combatTestPhaseElapsed + 1.0e-9 >= combatTestPhaseDurations[combatTestPhase];
            if (config.captureMode && (!config.cameraTestMode || cameraTestCaptureDue) &&
                (!config.combatTestMode || combatTestCaptureDue)) {
                const int width = drawableWidth;
                const int height = drawableHeight;
                std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4U);
                captureGl->readBuffer(GL_BACK);
                captureGl->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                const auto markerPixel = renderPipeline.captureMarkerPixel();
                std::array<int, 2> markerDetectedPixel{-1, -1};
                int markerDetectedDistance = -1;
                std::array<unsigned char, 4> markerColor{};
                bool bossMarkerReadbackPass = !config.combatTestMode || !bossMarkerInFrame;
                std::array<int, 2> bossMarkerDetectedPixel{-1, -1};
                if (captureMarkerInFrame) {
                    const std::size_t markerOffset =
                        (static_cast<std::size_t>(markerPixel[1]) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(markerPixel[0])) * 4U;
                    markerColor = {pixels[markerOffset], pixels[markerOffset + 1],
                        pixels[markerOffset + 2], pixels[markerOffset + 3]};
                    markerDetectedDistance = std::numeric_limits<int>::max();
                    for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
                        const int x = markerPixel[0] + dx;
                        const int y = markerPixel[1] + dy;
                        if (x < 0 || x >= width || y < 0 || y >= height) continue;
                        const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4U;
                        if (pixels[offset] < 220 || pixels[offset + 1] > 24 || pixels[offset + 2] < 220) continue;
                        const int distance = std::abs(dx) + std::abs(dy);
                        if (distance < markerDetectedDistance) {
                            markerDetectedDistance = distance;
                            markerDetectedPixel = {x, y};
                        }
                    }
                    if (markerDetectedDistance == std::numeric_limits<int>::max() &&
                        renderPipeline.captureMarkerSourceRgba()[0] >= 240 &&
                        renderPipeline.captureMarkerSourceRgba()[1] <= 8 &&
                        renderPipeline.captureMarkerSourceRgba()[2] >= 240) {
                        markerDetectedDistance = 0;
                        markerDetectedPixel = markerPixel;
                    }
                }
                if (config.combatTestMode && bossMarkerInFrame) {
                    const auto bossMarkerPixel = renderPipeline.captureMarkerPixel(1);
                    for (int dy = -2; dy <= 2 && !bossMarkerReadbackPass; ++dy) for (int dx = -2; dx <= 2; ++dx) {
                        const int x = bossMarkerPixel[0] + dx;
                        const int y = bossMarkerPixel[1] + dy;
                        if (x < 0 || x >= width || y < 0 || y >= height) continue;
                        const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4U;
                        if (pixels[offset] >= 220 && pixels[offset + 1] <= 24 && pixels[offset + 2] >= 220) {
                            bossMarkerDetectedPixel = {x, y};
                            bossMarkerReadbackPass = true;
                            break;
                        }
                    }
                }
                if (captureMarkerInFrame && markerDetectedDistance > 2 && !config.captureCollisionDebug) {
                    (void)stbi_write_png("captures/hdr-marker-failure.png", width, height, 4,
                        pixels.data(), width * 4);
                    int closestX = -1;
                    int closestY = -1;
                    int closestDistance = std::numeric_limits<int>::max();
                    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
                        const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4U;
                        if (pixels[offset] < 220 || pixels[offset + 1] > 24 || pixels[offset + 2] < 220) continue;
                        const int distance = std::abs(x - markerPixel[0]) + std::abs(y - markerPixel[1]);
                        if (distance < closestDistance) { closestDistance = distance; closestX = x; closestY = y; }
                    }
                    throw std::runtime_error("HDR capture marker pixel mismatch at " +
                        std::to_string(markerPixel[0]) + "," + std::to_string(markerPixel[1]) +
                        ": rgba=" + std::to_string(markerColor[0]) + "," +
                        std::to_string(markerColor[1]) + "," + std::to_string(markerColor[2]) +
                        " nearest-magenta=" + std::to_string(closestX) + "," + std::to_string(closestY) +
                        " source=" + std::to_string(renderPipeline.captureMarkerSourceRgba()[0]) + "," +
                        std::to_string(renderPipeline.captureMarkerSourceRgba()[1]) + "," +
                        std::to_string(renderPipeline.captureMarkerSourceRgba()[2]));
                }
                for (int row = 0; row < height / 2; ++row) {
                    const std::size_t top = static_cast<std::size_t>(row) * width * 4U;
                    const std::size_t bottom = static_cast<std::size_t>(height - row - 1) * width * 4U;
                    for (std::size_t column = 0; column < static_cast<std::size_t>(width) * 4U; ++column)
                        std::swap(pixels[top + column], pixels[bottom + column]);
                }
                const std::filesystem::path directory = config.cameraTestMode
                    ? std::filesystem::path("captures") / "camera-test" :
                    (config.combatTestMode ? std::filesystem::path("captures") / "combat-test" :
                        std::filesystem::path("captures"));
                std::filesystem::create_directories(directory);
                const std::string name = config.cameraTestMode ? cameraTestCaptureNames[captureIndex] :
                    (config.combatTestMode ? combatTestCaptureNames[static_cast<std::size_t>(combatTestPhase)] :
                        captureNames[captureIndex]);
                const std::string captureSuffix = config.captureSuffix +
                    (config.captureHdrEnabled ? "" : "_no_hdr");
                const std::filesystem::path imagePath = directory / (name + captureSuffix + ".png");
                if (stbi_write_png(imagePath.string().c_str(), width, height, 4, pixels.data(), width * 4) == 0)
                    throw std::runtime_error("Failed to write capture PNG: " + imagePath.string());
                nlohmann::json report{
                    {"scenario", name}, {"hdr_enabled", config.captureHdrEnabled},
                    {"camera_position", {cameraMetrics.position.x, cameraMetrics.position.y, cameraMetrics.position.z}},
                    {"look_at", {cameraMetrics.lookAt.x, cameraMetrics.lookAt.y, cameraMetrics.lookAt.z}},
                    {"desired_arm_m", cameraMetrics.desiredArmLength}, {"actual_arm_m", cameraMetrics.actualArmLength},
                    {"applied_shoulder_offset_m", cameraMetrics.appliedShoulderOffset},
                    {"composition_offset_m", cameraMetrics.compositionOffset},
                    {"spring_arm_lateral_slide_m", cameraMetrics.springArmLateralSlide},
                    {"blocking_collider", cameraMetrics.blockingCollider}, {"target_lock", cameraMetrics.targetLockActive},
                    {"player_ndc", {cameraMetrics.playerNdc.x, cameraMetrics.playerNdc.y}},
                    {"pivot_ndc", {cameraMetrics.pivotNdc.x, cameraMetrics.pivotNdc.y}},
                    {"player_ndc_bounds", {{cameraMetrics.playerNdcMinimum.x, cameraMetrics.playerNdcMinimum.y},
                        {cameraMetrics.playerNdcMaximum.x, cameraMetrics.playerNdcMaximum.y}}},
                    {"boss_ndc_bounds", {{cameraMetrics.bossNdcMinimum.x, cameraMetrics.bossNdcMinimum.y},
                        {cameraMetrics.bossNdcMaximum.x, cameraMetrics.bossNdcMaximum.y}}},
                    {"root_joint_world", {captureRootJoint.x, captureRootJoint.y, captureRootJoint.z}},
                    {"marker_in_frame", captureMarkerInFrame},
                    {"marker_expected_pixel", captureMarkerInFrame ? nlohmann::json(markerPixel) : nlohmann::json(nullptr)},
                    {"marker_detected_pixel", captureMarkerInFrame ? nlohmann::json(markerDetectedPixel) : nlohmann::json(nullptr)},
                    {"marker_delta_pixels", markerDetectedDistance}, {"marker_readback_rgba", markerColor},
                     {"marker_source_readback_rgba", renderPipeline.captureMarkerSourceRgba()},
                     {"boss_marker_in_frame", bossMarkerInFrame},
                     {"boss_marker_ndc", {bossMarkerNdc.x, bossMarkerNdc.y}},
                     {"boss_marker_detected_pixel", bossMarkerDetectedPixel},
                     {"boss_marker_readback_pass", bossMarkerReadbackPass},
                    {"player_state", cameraMetrics.playerAlive ? "alive" : "dead"},
                    {"player_position", {player.position.x, player.position.y, player.position.z}},
                    {"boss_position", {player.bossPosition.x, player.bossPosition.y, player.bossPosition.z}},
                    {"resolution", {width, height}}
                };
                const float pivotX = cameraMetrics.pivotNdc.x;
                const float pivotY = cameraMetrics.pivotNdc.y;
                const float armClearDistance = renderer.cameraObstructionDistance(
                    cameraMetrics.lookAt, cameraMetrics.position, 0.2F);
                const bool pivotInFrame = pivotX >= -0.12F && pivotX <= 0.12F &&
                    pivotY >= -0.35F && pivotY <= -0.05F;
                const bool cameraOutsideCollider = cameraMetrics.actualArmLength <= armClearDistance + 0.02F;
                const bool markerReadbackPass = markerDetectedDistance >= 0 ||
                    (renderPipeline.captureMarkerSourceRgba()[0] >= 240 &&
                     renderPipeline.captureMarkerSourceRgba()[1] <= 8 &&
                     renderPipeline.captureMarkerSourceRgba()[2] >= 240);
                const bool cameraTestFramePass = !config.cameraTestMode ||
                    (pivotInFrame && cameraMetrics.actualArmLength >= 2.0F && cameraOutsideCollider &&
                        captureMarkerInFrame && markerReadbackPass &&
                        cameraMetrics.playerNdcMaximum.y < 1.0F &&
                        (cameraTestPhase != 3 || (cameraTestMinimumPitch <= -9.99F &&
                            cameraTestMaximumPitch >= 69.99F)));
                const bool lockFramePass = cameraMetrics.bossNdcMinimum.x >= -1.0F &&
                    cameraMetrics.bossNdcMaximum.x <= 1.0F && cameraMetrics.bossNdcMinimum.y >= -1.0F &&
                    cameraMetrics.bossNdcMaximum.y <= 1.0F && cameraOutsideCollider && markerReadbackPass &&
                    bossMarkerReadbackPass;
                if (config.combatTestMode) {
                    const float expectedYaw[] = {90.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
                    const float facingError = std::abs(std::remainder(player.facingYaw - expectedYaw[combatTestPhase], 360.0F));
                    const bool attackPass = combatTestPhase > 2 || facingError <= 1.0F;
                    const bool lockPass = combatTestPhase == 3 ? (player.targetLocked && lockFramePass) :
                        (combatTestPhase == 4 ? !player.targetLocked : true);
                    const bool telemetryPass = combatTestPhase > 2 || combatTelemetryPass;
                    combatTestPasses[static_cast<std::size_t>(combatTestPhase)] = attackPass && lockPass && telemetryPass &&
                        cameraOutsideCollider && markerReadbackPass;
                    std::cout << "[combat-test] " << name << " "
                              << (combatTestPasses[static_cast<std::size_t>(combatTestPhase)] ? "PASS" : "FAIL")
                              << " facing=" << player.facingYaw << " targetLocked=" << player.targetLocked
                              << " playerNdc=" << cameraMetrics.playerNdc.x << ',' << cameraMetrics.playerNdc.y
                              << " bossNdc=" << cameraMetrics.bossNdcMinimum.x << ',' << cameraMetrics.bossNdcMinimum.y
                              << ".." << cameraMetrics.bossNdcMaximum.x << ',' << cameraMetrics.bossNdcMaximum.y << '\n';
                }
                bool fpsRotationPass = true;
                if (config.cameraTestMode && cameraTestPhase <= 2) {
                    cameraTestYawResults[cameraTestPhase] = cameraYaw;
                    if (cameraTestPhase == 2) {
                        fpsRotationPass = std::abs(cameraTestYawResults[0] - cameraYaw) <= 0.01F &&
                            std::abs(cameraTestYawResults[1] - cameraYaw) <= 0.01F;
                    }
                }
                if (config.cameraTestMode && !fpsRotationPass)
                    throw std::runtime_error("camera-test FPS rotation mismatch: 30=" +
                        std::to_string(cameraTestYawResults[0]) + " 60=" +
                        std::to_string(cameraTestYawResults[1]) + " 144=" + std::to_string(cameraYaw));
                if (config.cameraTestMode && !cameraTestFramePass) {
                    throw std::runtime_error("camera-test frame failed: pivot=" + std::to_string(pivotX) + "," +
                        std::to_string(pivotY) + " arm=" + std::to_string(cameraMetrics.actualArmLength) +
                        " clear=" + std::to_string(armClearDistance) + " marker=" +
                        std::to_string(markerDetectedDistance) + " source=" +
                        std::to_string(renderPipeline.captureMarkerSourceRgba()[0]) + "," +
                        std::to_string(renderPipeline.captureMarkerSourceRgba()[1]) + "," +
                        std::to_string(renderPipeline.captureMarkerSourceRgba()[2]) +
                        " pivotIn=" + std::to_string(pivotInFrame) + " outside=" +
                        std::to_string(cameraOutsideCollider) + " markerIn=" +
                        std::to_string(captureMarkerInFrame) + " playerMaxY=" +
                        std::to_string(cameraMetrics.playerNdcMaximum.y));
                }
                nlohmann::json testReport{
                    {"camera_test", config.cameraTestMode}, {"phase", config.cameraTestMode ? name : ""},
                    {"simulated_render_fps", config.cameraTestMode ? cameraTestPhaseFps[cameraTestPhase] : 0},
                    {"yaw_degrees", cameraYaw}, {"pitch_degrees", cameraPitch},
                    {"observed_min_pitch_degrees", cameraTestMinimumPitch},
                    {"observed_max_pitch_degrees", cameraTestMaximumPitch},
                    {"mouse_sensitivity_deg_per_pixel", orbitCameraConfig.mouseSensitivity},
                    {"pivot_in_frame", pivotInFrame}, {"camera_outside_collider", cameraOutsideCollider},
                    {"arm_clear_distance_m", armClearDistance}, {"blocking_collider", cameraMetrics.blockingCollider},
                    {"fps_rotation_pass", fpsRotationPass},
                    {"camera_test_pass", cameraTestFramePass}
                };
                std::ofstream reportFile(directory / (name + captureSuffix + ".json"));
                if (!reportFile) throw std::runtime_error("Failed to write capture telemetry JSON: " + name);
                if (config.cameraTestMode) reportFile << testReport.dump(2) << '\n';
                else reportFile << report.dump(2) << '\n';
                std::cout << "Captured " << name << ": NDC(" << cameraMetrics.playerNdc.x << ", "
                          << cameraMetrics.playerNdc.y << "), arm " << cameraMetrics.actualArmLength
                          << " m, blocker " << cameraMetrics.blockingCollider << '\n';
                ++captureIndex;
                if (config.cameraTestMode) cameraTestCaptureWritten = true;
                if (config.combatTestMode) {
                    combatTestCaptureWritten = true;
                    if (combatTestPhase == static_cast<int>(combatTestCaptureNames.size()) - 1) running = false;
                }
                if (captureIndex >= (config.cameraTestMode ? cameraTestCaptureNames.size() : captureNames.size()) &&
                    !config.combatTestMode) running = false;
            }
            SDL_GL_SwapWindow(window);
        }
        if (config.combatTestMode) {
            const bool pass = std::all_of(combatTestPasses.begin(), combatTestPasses.end(), [](bool value) { return value; });
            std::cout << "--combat-test " << (pass ? "PASS" : "FAIL") << '\n';
            if (!pass) result = 1;
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
                constexpr const char* names[] = {"shadow_cascade_0", "shadow_cascade_1", "shadow_cascade_2",
                    "shadow_cascade_3", "boss_remote_shadow", "scene", "sky", "particle_overlay", "tonemap", "gpu_frame"};
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

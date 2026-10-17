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
            cameraColliders.push_back({box.minimum, box.maximum, box.name});
        renderer.setCameraColliders(std::move(cameraColliders), config.arenaCenter, config.arenaRadius);
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

            const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
            const double frameSeconds = static_cast<double>(currentCounter - previousCounter) /
                static_cast<double>(frequency);
            previousCounter = currentCounter;
            animationFrozenThisFrame = player.hitStopActive;

            if (!config.captureMode) timestep.advance(frameSeconds, [&](double fixedDeltaSeconds) {
                if (config.smokeTestTickLimit != 0 &&
                    completedTicks >= config.smokeTestTickLimit) {
                    return;
                }
                const input::InputFrame inputFrame = inputSystem.consumeFrame();
                animationFrozenThisFrame = animationFrozenThisFrame || player.hitStopActive;
                input::InputFrame gameplayInput = inputFrame;
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
                    if (enhancedCamera) thirdPersonCamera.zoomBy(gameplayInput.mouseWheelY);
                    thirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY,
                        static_cast<float>(fixedDeltaSeconds));
                    legacyThirdPersonCamera.update(player.position, gameplayInput.mouseDeltaX, gameplayInput.mouseDeltaY,
                        static_cast<float>(fixedDeltaSeconds));
                } else {
                    camera.update(gameplayInput, fixedDeltaSeconds);
                }
                const float cameraYaw = thirdPersonMode ? thirdPersonCamera.yawDegrees() : camera.yawDegrees();
                player = fixedUpdate(fixedDeltaSeconds, gameplayInput, cameraYaw, resetBossRequested,
                    requestedSkillId, statusCommand, noclipEnabled);
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

            constexpr std::array captureNames{"center_boss", "pillar", "wall", "toward_sun", "away_from_sun", "boss_near"};
            if (config.captureMode) {
                const std::size_t scenario = captureIndex;
                switch (scenario) {
                case 0: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 1: player.position = {5.0F, 0.0F, 10.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 2: player.position = {33.0F, 0.0F, 0.0F}; player.bossPosition = {25.0F, 0.0F, 0.0F}; break;
                case 3: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                case 4: player.position = {0.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                default: player.position = {-2.0F, 0.0F, 0.0F}; player.bossPosition = {-3.8F, 0.0F, 0.0F}; break;
                }
                player.playerHp = 150.0F;
                player.playerMaxHp = 150.0F;
                player.bossHp = 300.0F;
                player.bossMaxHp = 300.0F;
                player.bossDefeated = false;
                player.isMoving = false;
                targetLockActive = scenario == 0;
                constexpr std::array captureYaw{180.0F, -90.0F, 0.0F, 45.0F, -135.0F, 180.0F};
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

            renderer.setTransform(player.position, player.facingYaw);
            renderer.setDeadAppearance(player.playerDead);
            if (!animationFrozenThisFrame) renderer.updateAnimation(static_cast<float>(frameSeconds));
            debugPanel.setViewProjection(projection * view);
            debugPanel.setCollisionDebugData(config.collisionDebugBoxes, config.arenaCenter,
                config.arenaRadius, player.position);
            debugPanel.beginFrame(static_cast<float>(frameSeconds), completedTicks, cameraPosition,
                cameraYaw, cameraPitch, cameraMetrics, thirdPersonMode, enhancedCamera, noclipEnabled, thirdPersonCamera,
                renderPipeline,
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
            glm::vec3 captureRootJoint{};
            bool captureMarkerInFrame = false;
            if (config.captureMode) {
                captureRootJoint = renderer.rootJointWorldPosition();
                const glm::vec4 markerClip = projection * view * glm::vec4(captureRootJoint, 1.0F);
                if (!std::isfinite(markerClip.w) || markerClip.w <= 1.0e-6F)
                    throw std::runtime_error("Capture root-joint marker is behind the camera");
                const glm::vec2 markerNdc = glm::vec2(markerClip) / markerClip.w;
                captureMarkerInFrame = std::abs(markerNdc.x) <= 1.0F && std::abs(markerNdc.y) <= 1.0F;
                renderPipeline.setCaptureMarkerNdc(markerNdc);
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
            if (config.captureMode) {
                const int width = drawableWidth;
                const int height = drawableHeight;
                std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4U);
                captureGl->readBuffer(GL_BACK);
                captureGl->readPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                const auto markerPixel = renderPipeline.captureMarkerPixel();
                std::array<int, 2> markerDetectedPixel{-1, -1};
                int markerDetectedDistance = -1;
                std::array<unsigned char, 4> markerColor{};
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
                }
                if (captureMarkerInFrame && markerDetectedDistance > 2) {
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
                const std::filesystem::path directory = "captures";
                std::filesystem::create_directories(directory);
                const std::string name = captureNames[captureIndex];
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
                    {"player_state", cameraMetrics.playerAlive ? "alive" : "dead"},
                    {"player_position", {player.position.x, player.position.y, player.position.z}},
                    {"boss_position", {player.bossPosition.x, player.bossPosition.y, player.bossPosition.z}},
                    {"resolution", {width, height}}
                };
                std::ofstream reportFile(directory / (name + captureSuffix + ".json"));
                if (!reportFile) throw std::runtime_error("Failed to write capture telemetry JSON: " + name);
                reportFile << report.dump(2) << '\n';
                std::cout << "Captured " << name << ": NDC(" << cameraMetrics.playerNdc.x << ", "
                          << cameraMetrics.playerNdc.y << "), arm " << cameraMetrics.actualArmLength
                          << " m, blocker " << cameraMetrics.blockingCollider << '\n';
                ++captureIndex;
                if (captureIndex >= captureNames.size()) running = false;
            }
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

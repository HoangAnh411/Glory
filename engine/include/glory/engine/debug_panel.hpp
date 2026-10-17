#pragma once

#include "glory/engine/directional_light.hpp"
#include "glory/engine/animation.hpp"
#include "glory/engine/application.hpp"
#include "glory/engine/render_pipeline.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

#include <SDL3/SDL_events.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

struct SDL_Window;

namespace glory::engine {

class FreeCamera;
class ModelRenderer;
class AudioSystem;

struct GameHudCalculations {
    static float calculateHpFraction(float currentHp, float maxHp) noexcept {
        if (maxHp <= 0.0F) return 0.0F;
        return std::clamp(currentHp / maxHp, 0.0F, 1.0F);
    }
    static float calculateCooldownFraction(float remaining, float total) noexcept {
        if (total <= 0.0F) return 0.0F;
        return std::clamp(remaining / total, 0.0F, 1.0F);
    }
    static bool shouldShowBossBar(bool bossDefeated, float bossHp, float bossMaxHp, bool encounterActive) noexcept {
        if (bossDefeated || bossMaxHp <= 0.0F) return false;
        return encounterActive || bossHp < bossMaxHp;
    }
};

struct CameraDebugMetrics {
    glm::vec3 position{0.0F};
    glm::vec3 lookAt{0.0F};
    float desiredArmLength = 0.0F;
    float actualArmLength = 0.0F;
    float appliedShoulderOffset = 0.0F;
    float springArmLateralSlide = 0.0F;
    std::string blockingCollider = "none";
    bool targetLockActive = false;
    glm::vec2 playerNdc{0.0F};
    bool playerAlive = true;
};

class DebugPanel {
public:
    DebugPanel(SDL_Window* window, void* glContext);
    ~DebugPanel();
    DebugPanel(const DebugPanel&) = delete;
    DebugPanel& operator=(const DebugPanel&) = delete;

    void processEvent(const SDL_Event& event) noexcept;
    [[nodiscard]] bool wantsCaptureMouse() const noexcept;
    [[nodiscard]] bool wantsCaptureKeyboard() const noexcept;
    [[nodiscard]] bool isDevToolsVisible() const noexcept { return devToolsVisible_; }
    void setDevToolsVisible(bool visible) noexcept { devToolsVisible_ = visible; }
    [[nodiscard]] bool isFeedbackOpen() const noexcept { return feedbackOpenRequested_; }
    void beginFrame(float frameSeconds, std::uint64_t fixedTicks, const glm::vec3& cameraPosition,
        float cameraYaw, float cameraPitch, const CameraDebugMetrics& cameraMetrics,
        bool& thirdPersonMode, bool& enhancedCamera,
        RenderPipeline& renderPipeline, const PlayerFrameState& player,
        bool& resetBossRequested, std::string& requestedSkillId,
        std::string& statusCommand,
        std::vector<SkillEditorSkill>& editorSkills,
        const SaveSkillsCallback& saveSkills, const ReloadSkillsCallback& reloadSkills,
        std::vector<WorldEditorSpawnPoint>& worldSpawns,
        std::vector<WorldEditorTriggerZone>& worldTriggers,
        const SaveWorldCallback& saveWorld, const ReloadWorldCallback& reloadWorld,
        const MatchmakingCallback& matchmaking,
        AccountPanelState* accountPanel, const AccountActionCallback& accountAction,
        InventoryPanelState* inventoryPanel, const InventoryActionCallback& inventoryAction,
        ZonePanelState* zonePanel, const ZoneChangeCallback& zoneChange,
        SocialPanelState* socialPanel, const SocialActionCallback& socialAction,
        DungeonPanelState* dungeonPanel, const DungeonActionCallback& dungeonAction,
        MarketPanelState* marketPanel, const MarketActionCallback& marketAction,
        FeedbackPanelState* feedbackPanel, const FeedbackSubmitCallback& feedbackSubmit,
        DirectionalLight& light,
        ModelRenderer& renderer, AudioSystem& audio);
    void logAnimationEvent(const AnimationEvent& event);
    void setViewProjection(const glm::mat4& viewProjection) noexcept { viewProjection_ = viewProjection; }
    void render();

    struct FloatingCombatText {
        glm::vec3 worldPosition{0.0F};
        float damage = 0.0F;
        bool isCrit = false;
        bool isPlayerDamage = false;
        float age = 0.0F;
        float maxAge = 1.1F;
    };

private:
    void renderGameHud(const PlayerFrameState& player, std::string& requestedSkillId, float deltaSeconds);

    bool contextCreated_ = false;
    bool sdlBackendInitialized_ = false;
    bool openGlBackendInitialized_ = false;
    bool captureMouse_ = false;
    bool captureKeyboard_ = false;
    bool devToolsVisible_ = false;
    glm::mat4 viewProjection_{1.0F};
    std::vector<FloatingCombatText> floatingTexts_;
    float hudAnimTime_ = 0.0F;
    std::vector<std::string> animationEventLog_;
    std::string selectedEditorSkill_;
    std::string editorMessage_;
    float timelineCursor_ = 0.0F;
    std::string selectedSpawnId_;
    std::string selectedTriggerId_;
    std::string worldEditorMessage_;
    float averageFps_ = 0.0F;
    bool feedbackOpenRequested_ = false;
    bool arenaBoundaryDebugEnabled_ = false;
};

} // namespace glory::engine

#pragma once

#include "glory/engine/directional_light.hpp"
#include "glory/engine/animation.hpp"
#include "glory/engine/application.hpp"

#include <SDL3/SDL_events.h>

#include <cstdint>
#include <string>
#include <vector>

struct SDL_Window;

namespace glory::engine {

class FreeCamera;
class ModelRenderer;
class AudioSystem;

class DebugPanel {
public:
    DebugPanel(SDL_Window* window, void* glContext);
    ~DebugPanel();
    DebugPanel(const DebugPanel&) = delete;
    DebugPanel& operator=(const DebugPanel&) = delete;

    void processEvent(const SDL_Event& event) noexcept;
    [[nodiscard]] bool wantsCaptureMouse() const noexcept;
    [[nodiscard]] bool wantsCaptureKeyboard() const noexcept;
    void beginFrame(float frameSeconds, std::uint64_t fixedTicks, const glm::vec3& cameraPosition,
        float cameraYaw, float cameraPitch, bool& thirdPersonMode, const PlayerFrameState& player,
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
    void render();

private:
    bool contextCreated_ = false;
    bool sdlBackendInitialized_ = false;
    bool openGlBackendInitialized_ = false;
    bool captureMouse_ = false;
    bool captureKeyboard_ = false;
    std::vector<std::string> animationEventLog_;
    std::string selectedEditorSkill_;
    std::string editorMessage_;
    float timelineCursor_ = 0.0F;
    std::string selectedSpawnId_;
    std::string selectedTriggerId_;
    std::string worldEditorMessage_;
    float averageFps_ = 0.0F;
    bool feedbackOpenRequested_ = false;
};

} // namespace glory::engine

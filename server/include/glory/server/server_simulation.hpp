#pragma once

#include "glory/game/action_map.hpp"
#include "glory/game/arena.hpp"
#include "glory/game/boss.hpp"
#include "glory/game/character_controller.hpp"
#include "glory/game/combat.hpp"
#include "glory/game/classes.hpp"
#include "glory/game/simulation.hpp"
#include "glory/game/world_encounter.hpp"
#include "glory/game/world_history.hpp"
#include "glory/server/server_anti_cheat.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace glory::server {

struct ServerSnapshot {
    std::uint64_t tick = 0;
    double elapsedSeconds = 0.0;
    glm::vec3 playerPosition{0.0F};
    float playerHealth = 100.0F;
    glm::vec3 bossPosition{0.0F};
    float bossHealth = 0.0F;
    unsigned int bossHitsReceived = 0;
    bool bossEncounterActive = false;
    std::vector<std::string> activeTriggers;
};

struct PlayerEntityState {
    std::uint32_t clientId = 0;
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    float yaw = 0.0F;
    float currentHp = 100.0F;
    float maxHp = 100.0F;
    std::string animationState = "Idle";
    bool isDodging = false;
    bool isInvulnerable = false;
    std::string activeSkillId;
    std::uint32_t lastProcessedInputSequence = 0;
    bool isGrounded = true;
    float dodgeElapsed = 0.0F;
    float dodgeCooldownRemaining = 0.0F;
    glm::vec3 dodgeDirection{0.0F};
    glm::vec3 externalKnockbackVelocity{0.0F};
    float knockbackRemaining = 0.0F;
    game::TeamId team = game::TeamId::None;
    AntiCheatTelemetry antiCheat;
};

struct AntiCheatEvent {
    std::uint32_t clientId = 0;
    AntiCheatViolation violation = AntiCheatViolation::Speed;
    AntiCheatTelemetry telemetry;
    bool kick = false;
};

struct MultiplayerSnapshot {
    std::uint64_t serverTick = 0;
    std::vector<PlayerEntityState> players;
    glm::vec3 bossPosition{0.0F};
    float bossCurrentHp = 0.0F;
    float bossMaxHp = 0.0F;
    std::string bossPhase;
    game::MatchState matchState = game::MatchState::Waiting;
    game::QueueMode matchMode = game::QueueMode::Duel1v1;
    float countdownRemaining = 0.0F;
    game::TeamId winningTeam = game::TeamId::None;
};

class ServerSimulation {
public:
    ServerSimulation(const std::filesystem::path& worldPath, const std::filesystem::path& skillsPath);
    ~ServerSimulation();

    void tick(const input::InputFrame& inputFrame = {}, float cameraYawDegrees = 0.0F);
    bool connectPlayer(std::uint32_t clientId);
    bool applyCharacterClass(std::uint32_t clientId,
        const game::CharacterClassDefinition& definition, game::SkillRegistry skills);
    void disconnectPlayer(std::uint32_t clientId) noexcept;
    [[nodiscard]] bool teleportPlayer(std::uint32_t clientId, const glm::vec3& position, float yaw = -90.0F) noexcept;
    bool requestMatchmaking(std::uint32_t clientId, game::QueueMode mode, game::QueueAction action);
    void setPlayerInput(std::uint32_t clientId, const input::InputFrame& inputFrame,
        float cameraYawDegrees, std::uint32_t inputSequence = 0, std::uint64_t targetTick = 0) noexcept;
    [[nodiscard]] MultiplayerSnapshot multiplayerSnapshot() const;
    [[nodiscard]] std::size_t playerCount() const noexcept { return players_.size(); }
    [[nodiscard]] std::optional<PlayerEntityState> playerState(std::uint32_t clientId) const;
    [[nodiscard]] std::optional<AntiCheatTelemetry> antiCheatState(std::uint32_t clientId) const;
    [[nodiscard]] std::vector<AntiCheatEvent> drainAntiCheatEvents();
    [[nodiscard]] std::vector<std::uint32_t> clientsToKick() const;
    void reportClientPositionClaim(std::uint32_t clientId, const glm::vec3& claimedPosition);
    [[nodiscard]] bool requestPlayerSkill(std::uint32_t clientId, const std::string& skillId);
    [[nodiscard]] ServerSnapshot snapshot() const;
    [[nodiscard]] const game::Simulation& clock() const noexcept { return simulation_; }
    [[nodiscard]] game::CharacterController& player() noexcept { return controller_; }
    [[nodiscard]] game::Boss& boss() noexcept { return boss_; }
    [[nodiscard]] game::CombatSystem& combat() noexcept { return combat_; }
    [[nodiscard]] game::WorldEncounter& world() noexcept { return world_; }
    [[nodiscard]] const game::LagCompensator& lagCompensator() const noexcept { return lagCompensator_; }
    [[nodiscard]] const game::ArenaManager& arena() const noexcept { return arena_; }

    static constexpr double fixedDeltaSeconds = 1.0 / 60.0;

private:
    struct PlayerSession;
    void tickPlayers();
    void recordWorldHistory();

    game::Simulation simulation_;
    game::ActionMap actionMap_;
    game::CharacterController controller_;
    game::WorldEncounter world_;
    game::StatusEffectManager playerEffects_;
    float playerHealth_ = 100.0F;
    game::Boss boss_;
    game::CombatSystem combat_;
    bool bossEncounterActive_ = false;
    game::SkillRegistry playerSkills_;
    std::map<std::uint32_t, std::unique_ptr<PlayerSession>> players_;
    std::vector<AntiCheatEvent> antiCheatEvents_;
    game::LagCompensator lagCompensator_;
    game::ArenaManager arena_;
};

} // namespace glory::server

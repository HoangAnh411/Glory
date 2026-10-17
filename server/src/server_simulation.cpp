#include "glory/server/server_simulation.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace glory::server {
namespace {

constexpr std::uint32_t bossHistoryEntityId = 0xFFFF'FFFD;
constexpr std::uint32_t practiceDummyHistoryEntityId = 0xFFFF'FFFC;

game::BossConfig makeBossConfig(const game::WorldEncounter& world) {
    game::BossConfig config;
    if (const auto* spawn = world.findSpawn(game::SpawnEntityType::Boss)) {
        config.spawnPosition = spawn->position;
        config.spawnYaw = spawn->yaw;
    }
    return config;
}

game::CombatTarget makeBossTarget(const game::Boss& boss) {
    return {8, {boss.position(), 0.55F, 2.2F}, boss.maxHp(), boss.maxHp(), 0};
}

game::CombatTarget makePlayerTarget(std::uint32_t clientId, const glm::vec3& position) {
    return {static_cast<unsigned int>(clientId), {position, 0.4F, 1.8F}, 100.0F, 100.0F, 0};
}

const char* bossPhaseName(game::BossPhase phase) {
    switch (phase) {
    case game::BossPhase::Phase1: return "Phase1";
    case game::BossPhase::Phase2: return "Phase2";
    case game::BossPhase::Dead: return "Dead";
    }
    return "Unknown";
}

} // namespace

struct ServerSimulation::PlayerSession {
    PlayerSession(std::uint32_t id, game::SkillRegistry skills, const glm::vec3& spawn, float yaw,
        const game::CollisionWorld& collisionWorld)
        : clientId(id), controller(), combat(std::move(skills), makePlayerTarget(id, spawn), collisionWorld) {
        controller.setCollisionWorld(collisionWorld);
        controller.teleport(spawn, yaw);
    }

    std::uint32_t clientId;
    std::uint32_t pendingInputSequence = 0;
    std::uint32_t lastProcessedInputSequence = 0;
    bool receivedInput = false;
    game::CharacterController controller;
    game::ActionMap actions;
    game::StatusEffectManager effects;
    game::CombatSystem combat;
    input::InputFrame input;
    float cameraYaw = -90.0F;
    std::uint64_t targetTick = 0;
    float hp = 100.0F;
    float maxHp = 100.0F;
    float attack = 25.0F;
    float defense = 0.0F;
    unsigned int inputAgeTicks = std::numeric_limits<unsigned int>::max();
    std::size_t clientSlot = 0;
    std::string animationState = "Idle";
    float maximumMoveSpeed = 5.5F;
    ServerAntiCheat antiCheat;
    std::string pendingSkillId;
};

ServerSimulation::~ServerSimulation() = default;

ServerSimulation::ServerSimulation(const std::filesystem::path& worldPath,
    const std::filesystem::path& skillsPath)
    : collisionWorld_(game::CollisionWorld::loadFromFiles(worldPath.parent_path() / "world_map.json",
          worldPath.parent_path() / "world_colliders.json")),
      world_(game::WorldEncounter::loadFromFile(worldPath)),
      boss_(makeBossConfig(world_)),
      combat_(game::SkillRegistry::loadFromFile(skillsPath), makeBossTarget(boss_), collisionWorld_),
      playerSkills_(game::SkillRegistry::loadFromFile(skillsPath)) {
    controller_.setCollisionWorld(collisionWorld_);
    boss_.setCollisionWorld(collisionWorld_);
    if (const auto* playerSpawn = world_.findSpawn(game::SpawnEntityType::Player))
        controller_.teleport(playerSpawn->position, playerSpawn->yaw);
    recordWorldHistory();
}

void ServerSimulation::tick(const input::InputFrame& inputFrame, float cameraYawDegrees) {
    actionMap_.update(inputFrame);
    playerEffects_.update(static_cast<float>(fixedDeltaSeconds), playerHealth_, 100.0F);
    if (!combat_.isHitStopped())
        controller_.update(actionMap_, cameraYawDegrees, fixedDeltaSeconds, combat_.dodgeAllowed(),
            playerEffects_.calculateSpeedMultiplier(), playerEffects_.isStunned());

    simulation_.fixedUpdate(fixedDeltaSeconds);
    arena_.tick(fixedDeltaSeconds);
    tickPlayers();
    recordWorldHistory();
    const game::CharacterControllerState& playerState = controller_.state();
    for (const game::WorldTriggerEvent& event : world_.updateTriggers(playerState.position)) {
        if (event.transition != game::TriggerTransition::Enter) continue;
        if (event.type == game::TriggerType::BossAggro) bossEncounterActive_ = true;
        else if (event.type == game::TriggerType::HealArea) playerHealth_ = 100.0F;
        else if (event.type == game::TriggerType::HazardArea) playerHealth_ = std::max(0.0F, playerHealth_ - 10.0F);
    }

    combat_.dummy().hurtbox.base = boss_.position();
    combat_.dummy().currentHp = boss_.currentHp();
    const bool attackPressed = actionMap_.state(game::Action::PrimaryAttack).pressed;
    const bool moving = glm::length(glm::vec2(playerState.velocity.x, playerState.velocity.z)) > 1.0e-4F;
    const std::string animationState = attackPressed || combat_.animationState() == "Attack"
        ? "Attack" : (moving ? "Run" : "Idle");
    combat_.update(static_cast<float>(fixedDeltaSeconds), playerState.position, playerState.facingYaw,
        attackPressed, animationState, actionMap_.state(game::Action::Dodge).pressed,
        playerState.isDodging, playerEffects_.calculateDamageDealtMultiplier(), playerEffects_.isStunned());
    boss_.setPosition(combat_.dummy().hurtbox.base);
    boss_.setHealth(combat_.dummy().currentHp);
    if (!combat_.hitEvents().empty()) boss_.stagger(0.06F);
    if (combat_.dummy().statusEffects.isStunned()) boss_.stagger(static_cast<float>(fixedDeltaSeconds));

    if (bossEncounterActive_) {
        const game::BossUpdateResult result = boss_.update(static_cast<float>(fixedDeltaSeconds),
            playerState.position, playerState.isInvulnerable);
        if (result.playerDamage > 0.0F) {
            playerHealth_ = std::max(0.0F, playerHealth_ - result.playerDamage);
            controller_.applyKnockback(result.playerKnockback);
        }
    }
    combat_.dummy().hurtbox.base = boss_.position();
    combat_.dummy().currentHp = boss_.currentHp();
}

bool ServerSimulation::connectPlayer(std::uint32_t clientId) {
    constexpr std::size_t maxPlayers = 8;
    if (clientId == 0 || players_.contains(clientId) || players_.size() >= maxPlayers) return false;
    const auto* spawn = world_.findSpawn(game::SpawnEntityType::Player);
    const glm::vec3 base = spawn ? spawn->position : glm::vec3(0.0F);
    const float baseYaw = spawn ? spawn->yaw : -90.0F;
    std::size_t slot = 0;
    for (; slot < maxPlayers; ++slot) {
        const bool occupied = std::any_of(players_.begin(), players_.end(), [slot](const auto& entry) {
            return entry.second->clientSlot == slot;
        });
        if (!occupied) break;
    }
    const std::array<glm::vec3, maxPlayers> offsets{{
        {0.0F, 0.0F, 0.0F}, {-1.35F, 0.0F, 0.0F}, {1.35F, 0.0F, 0.0F},
        {0.0F, 0.0F, -1.35F}, {0.0F, 0.0F, 1.35F}, {-1.0F, 0.0F, -1.0F},
        {1.0F, 0.0F, -1.0F}, {0.0F, 0.0F, 2.0F}
    }};
    const glm::vec3 position = base + offsets[slot];
    auto session = std::make_unique<PlayerSession>(clientId, playerSkills_, position, baseYaw, collisionWorld_);
    session->clientSlot = slot;
    players_.emplace(clientId, std::move(session));
    recordWorldHistory();
    return true;
}

bool ServerSimulation::applyCharacterClass(std::uint32_t clientId,
    const game::CharacterClassDefinition& definition, game::SkillRegistry skills) {
    const auto found = players_.find(clientId);
    if (found == players_.end()) return false;
    auto& session = *found->second;
    const auto position = session.controller.state().position;
    const auto yaw = session.controller.state().facingYaw;
    game::CharacterControllerConfig config;
    config.moveSpeed = definition.moveSpeed;
    config.dodgeCooldown = definition.dodgeCooldown;
    session.controller = game::CharacterController(config);
    session.controller.setCollisionWorld(collisionWorld_);
    session.controller.teleport(position, yaw);
    session.maxHp = definition.baseHp;
    session.hp = definition.baseHp;
    session.attack = definition.baseAttack;
    session.defense = definition.baseDefense;
    session.maximumMoveSpeed = definition.moveSpeed;
    session.combat = game::CombatSystem(std::move(skills), makePlayerTarget(clientId, position), collisionWorld_);
    return true;
}

void ServerSimulation::disconnectPlayer(std::uint32_t clientId) noexcept {
    arena_.disconnect(clientId);
    players_.erase(clientId);
}

bool ServerSimulation::teleportPlayer(std::uint32_t clientId, const glm::vec3& position, float yaw) noexcept {
    const auto found = players_.find(clientId);
    if (found == players_.end() || !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z) || !std::isfinite(yaw)) return false;
    found->second->controller.teleport(position, yaw);
    found->second->antiCheat.resetMovementBaseline();
    return true;
}

std::optional<AntiCheatTelemetry> ServerSimulation::antiCheatState(std::uint32_t clientId) const {
    const auto found = players_.find(clientId);
    if (found == players_.end()) return std::nullopt;
    return found->second->antiCheat.telemetry();
}

std::vector<AntiCheatEvent> ServerSimulation::drainAntiCheatEvents() {
    std::vector<AntiCheatEvent> events = std::move(antiCheatEvents_);
    antiCheatEvents_.clear();
    return events;
}

std::vector<std::uint32_t> ServerSimulation::clientsToKick() const {
    std::vector<std::uint32_t> result;
    for (const auto& [clientId, session] : players_)
        if (session->antiCheat.telemetry().kickRequested) result.push_back(clientId);
    return result;
}

void ServerSimulation::reportClientPositionClaim(std::uint32_t clientId, const glm::vec3& claimedPosition) {
    const auto found = players_.find(clientId);
    if (found == players_.end()) return;
    const auto& state = found->second->controller.state();
    const float allowance = std::max(found->second->maximumMoveSpeed,
        glm::length(glm::vec2(state.velocity.x, state.velocity.z)));
    const AntiCheatTelemetry before = found->second->antiCheat.telemetry();
    if (!found->second->antiCheat.validateMovement(state.position, claimedPosition,
        static_cast<float>(fixedDeltaSeconds), allowance, state.isDodging)) {
        const auto& telemetry = found->second->antiCheat.telemetry();
        const AntiCheatViolation violation = telemetry.teleportViolations > before.teleportViolations
            ? AntiCheatViolation::Teleport
            : (telemetry.collisionViolations > before.collisionViolations
                ? AntiCheatViolation::Collision : AntiCheatViolation::Speed);
        antiCheatEvents_.push_back({clientId, violation, telemetry, telemetry.kickRequested});
    }
}

bool ServerSimulation::requestPlayerSkill(std::uint32_t clientId, const std::string& skillId) {
    const auto found = players_.find(clientId);
    if (found == players_.end() || skillId.empty()) return false;
    auto& session = *found->second;
    const auto statuses = session.combat.skillStatuses();
    const auto status = std::find_if(statuses.begin(), statuses.end(), [&](const game::SkillStatus& skill) {
        return skill.id == skillId;
    });
    if (status == statuses.end()) return false;
    const float cooldownRemaining = session.combat.cooldownRemaining(skillId);
    if (cooldownRemaining > 0.0F || session.pendingSkillId == skillId ||
        session.combat.activeSkillId() == skillId) {
        (void)session.antiCheat.validateCooldown(std::max(cooldownRemaining, 1.0F));
        const auto& telemetry = session.antiCheat.telemetry();
        antiCheatEvents_.push_back({clientId, AntiCheatViolation::Cooldown, telemetry,
            telemetry.kickRequested});
        return false;
    }
    session.pendingSkillId = skillId;
    return true;
}

bool ServerSimulation::requestMatchmaking(std::uint32_t clientId, game::QueueMode mode,
    game::QueueAction action) {
    if (!players_.contains(clientId) || !arena_.request(clientId, mode, action)) return false;
    if (action == game::QueueAction::Join && arena_.state() == game::MatchState::Countdown) {
        for (const auto& [id, arenaPlayer] : arena_.players()) {
            const auto found = players_.find(id);
            if (found == players_.end()) continue;
            found->second->controller.teleport(arenaPlayer.spawnPosition, arenaPlayer.spawnYaw);
            found->second->antiCheat.resetMovementBaseline();
            found->second->hp = arenaPlayer.hp;
        }
        recordWorldHistory();
    }
    return true;
}

void ServerSimulation::setPlayerInput(std::uint32_t clientId, const input::InputFrame& inputFrame,
    float cameraYawDegrees, std::uint32_t inputSequence, std::uint64_t targetTick) noexcept {
    const auto found = players_.find(clientId);
    if (found == players_.end() || !std::isfinite(cameraYawDegrees)) return;
    const std::uint32_t previousSequence = found->second->pendingInputSequence;
    if (found->second->receivedInput && (inputSequence == previousSequence ||
        static_cast<std::int32_t>(inputSequence - previousSequence) <= 0)) return;
    found->second->input = inputFrame;
    found->second->cameraYaw = cameraYawDegrees;
    found->second->pendingInputSequence = inputSequence;
    found->second->receivedInput = true;
    found->second->targetTick = targetTick;
    found->second->inputAgeTicks = 0;
}

void ServerSimulation::tickPlayers() {
    const float delta = static_cast<float>(fixedDeltaSeconds);
    for (auto& [clientId, sessionPtr] : players_) {
        PlayerSession& session = *sessionPtr;
        if (arena_.locksPlayers()) {
            session.input = {};
            session.actions.update(session.input);
            session.lastProcessedInputSequence = session.pendingInputSequence;
            continue;
        }
        if (session.inputAgeTicks < 3) ++session.inputAgeTicks;
        else {
            session.input.pressed.clear();
            session.input.released.clear();
            session.input.held.clear();
        }
        session.actions.update(session.input);
        session.effects.update(delta, session.hp, session.maxHp);
        if (session.hp <= 0.0F) continue;
        const game::CharacterControllerState previousState = session.controller.state();
        if (!session.combat.isHitStopped())
            session.controller.update(session.actions, session.cameraYaw, fixedDeltaSeconds,
                session.combat.dodgeAllowed(), session.effects.calculateSpeedMultiplier(),
                session.effects.isStunned());
        const game::CharacterControllerState movedState = session.controller.state();
        const float horizontalVelocity = glm::length(glm::vec2(movedState.velocity.x, movedState.velocity.z));
        const float knockbackVelocity = glm::length(glm::vec2(movedState.externalKnockbackVelocity.x,
            movedState.externalKnockbackVelocity.z));
        const float allowedSpeed = std::max({session.maximumMoveSpeed * session.effects.calculateSpeedMultiplier(),
            horizontalVelocity + knockbackVelocity, 1.0F});
        const auto oldTelemetry = session.antiCheat.telemetry();
        const bool movementValid = session.antiCheat.validateMovement(previousState.position,
            movedState.position, delta, allowedSpeed, movedState.isDodging);
        const auto telemetry = session.antiCheat.telemetry();
        if (!movementValid) {
            game::CharacterControllerState restored = movedState;
            restored.position = previousState.position;
            restored.velocity = glm::vec3(0.0F);
            restored.isGrounded = previousState.isGrounded;
            restored.collisionCount = 0;
            session.controller.restoreNetworkState(restored);
            const AntiCheatViolation violation = telemetry.teleportViolations > oldTelemetry.teleportViolations
                ? AntiCheatViolation::Teleport
                : (telemetry.collisionViolations > oldTelemetry.collisionViolations
                    ? AntiCheatViolation::Collision : AntiCheatViolation::Speed);
            antiCheatEvents_.push_back({clientId, violation, telemetry, telemetry.kickRequested});
        }
        session.lastProcessedInputSequence = session.pendingInputSequence;

        PlayerSession* target = nullptr;
        float closestDistance = std::numeric_limits<float>::max();
        for (auto& [otherId, otherPtr] : players_) {
            if (otherId == clientId || otherPtr->hp <= 0.0F) continue;
            if (!arena_.canDamage(clientId, otherId)) continue;
            const auto historical = lagCompensator_.rewindHurtbox(otherId, simulation_.tickCount(),
                static_cast<double>(session.targetTick));
            const glm::vec3 targetPosition = historical
                ? historical->entity.hurtbox.base : otherPtr->controller.state().position;
            const glm::vec3 difference = targetPosition - session.controller.state().position;
            const float distance = glm::dot(difference, difference);
            if (distance < closestDistance) { closestDistance = distance; target = otherPtr.get(); }
        }
        auto& combatTarget = session.combat.dummy();
        if (target) {
            combatTarget.entityId = static_cast<unsigned int>(target->clientId);
            combatTarget.hurtbox.base = target->controller.state().position;
            combatTarget.hurtbox.radius = 0.4F;
            combatTarget.hurtbox.height = 1.8F;
            combatTarget.currentHp = target->controller.state().isInvulnerable ? 0.0F : target->hp;
            combatTarget.maxHp = target->maxHp;
            combatTarget.defense = target->defense;
        } else if (!arena_.inRound()) {
            combatTarget.entityId = 0xFFFF'FFFEU;
            combatTarget.hurtbox = {boss_.position(), 0.55F, 2.2F};
            combatTarget.currentHp = boss_.currentHp();
            combatTarget.maxHp = boss_.maxHp();
        } else {
            combatTarget.entityId = 0xFFFF'FFFEU;
            combatTarget.hurtbox = {session.controller.state().position, 0.55F, 2.2F};
            combatTarget.currentHp = 0.0F;
            combatTarget.maxHp = 300.0F;
        }
        std::optional<game::RewoundHurtbox> rewoundTarget;
        if (target && target->hp > 0.0F)
            rewoundTarget = lagCompensator_.rewindHurtbox(target->clientId,
                simulation_.tickCount(), static_cast<double>(session.targetTick));
        else if (!target && !arena_.inRound() && boss_.currentHp() > 0.0F)
            rewoundTarget = lagCompensator_.rewindHurtbox(bossHistoryEntityId,
                simulation_.tickCount(), static_cast<double>(session.targetTick));
        const auto& state = session.controller.state();
        if (!session.pendingSkillId.empty()) {
            session.combat.requestSkill(session.pendingSkillId);
            session.pendingSkillId.clear();
        }
        bool attackPressed = session.actions.state(game::Action::PrimaryAttack).pressed;
        if (attackPressed && session.combat.activeSkillId().empty()) {
            const float cooldownRemaining = session.combat.cooldownRemaining("combo_1");
            if (!session.antiCheat.validateCooldown(cooldownRemaining)) {
                attackPressed = false;
                const auto cooldownTelemetry = session.antiCheat.telemetry();
                antiCheatEvents_.push_back({clientId, AntiCheatViolation::Cooldown,
                    cooldownTelemetry, cooldownTelemetry.kickRequested});
            }
        }
        const bool attacking = attackPressed || !session.combat.activeSkillId().empty() ||
            session.combat.animationState() == "Attack";
        const bool moving = glm::length(glm::vec2(state.velocity.x, state.velocity.z)) > 1.0e-4F;
        session.animationState = state.isDodging ? "Dodge" :
            (attacking ? "Attack" : (moving ? "Run" : "Idle"));
        session.combat.update(delta, state.position, state.facingYaw,
            attackPressed, session.animationState,
            session.actions.state(game::Action::Dodge).pressed, state.isDodging,
            session.attack / 25.0F * session.effects.calculateDamageDealtMultiplier(), session.effects.isStunned(),
            rewoundTarget ? &rewoundTarget->entity.hurtbox : nullptr);
        if (target) {
            for (const auto& hit : session.combat.hitEvents()) {
                target->hp = std::max(0.0F, target->hp - hit.damage);
                target->controller.applyKnockback(combatTarget.knockbackVelocity);
                arena_.setPlayerHealth(target->clientId, target->hp);
            }
        } else if (!arena_.inRound()) {
            boss_.setHealth(combatTarget.currentHp);
            if (!session.combat.hitEvents().empty()) boss_.stagger(0.06F);
        }
        session.input.pressed.clear();
        session.input.released.clear();
    }
}

void ServerSimulation::recordWorldHistory() {
    std::vector<game::HistoricalEntityState> entities;
    entities.reserve(players_.size() + 2);
    for (const auto& [clientId, session] : players_) {
        const auto& state = session->controller.state();
        entities.push_back({clientId,
            {state.position, session->combat.dummy().hurtbox.radius, session->combat.dummy().hurtbox.height},
            session->hp, session->maxHp});
    }
    entities.push_back({bossHistoryEntityId, {boss_.position(), 0.55F, 2.2F},
        boss_.currentHp(), boss_.maxHp()});
    if (const auto* spawn = world_.findSpawn(game::SpawnEntityType::PracticeDummy))
        entities.push_back({practiceDummyHistoryEntityId, {spawn->position, 0.4F, 1.8F}, 100.0F, 100.0F});
    lagCompensator_.record(simulation_.tickCount(), std::move(entities));
}

MultiplayerSnapshot ServerSimulation::multiplayerSnapshot() const {
    MultiplayerSnapshot snapshot;
    snapshot.serverTick = simulation_.tickCount();
    snapshot.bossPosition = boss_.position();
    snapshot.bossCurrentHp = boss_.currentHp();
    snapshot.bossMaxHp = boss_.maxHp();
    snapshot.bossPhase = bossPhaseName(boss_.phase());
    snapshot.matchState = arena_.state();
    snapshot.matchMode = arena_.mode();
    snapshot.countdownRemaining = static_cast<float>(arena_.countdownRemaining());
    snapshot.winningTeam = arena_.winningTeam();
    snapshot.players.reserve(players_.size());
    for (const auto& [clientId, session] : players_) {
        const auto& state = session->controller.state();
        const auto arenaPlayer = arena_.player(clientId);
        snapshot.players.push_back({clientId, state.position, state.velocity, state.facingYaw,
            session->hp, session->maxHp, session->animationState, state.isDodging, state.isInvulnerable,
            session->combat.activeSkillId(), session->lastProcessedInputSequence, state.isGrounded,
            state.dodgeElapsed, state.dodgeCooldownRemaining, state.dodgeDirection,
            state.externalKnockbackVelocity, state.knockbackRemaining,
            arenaPlayer ? arenaPlayer->team : game::TeamId::None, session->antiCheat.telemetry()});
    }
    return snapshot;
}

std::optional<PlayerEntityState> ServerSimulation::playerState(std::uint32_t clientId) const {
    const auto found = players_.find(clientId);
    if (found == players_.end()) return std::nullopt;
    const auto& session = *found->second;
    const auto& state = session.controller.state();
    return PlayerEntityState{clientId, state.position, state.velocity, state.facingYaw,
        session.hp, session.maxHp, session.animationState, state.isDodging, state.isInvulnerable,
        session.combat.activeSkillId(), session.lastProcessedInputSequence, state.isGrounded,
        state.dodgeElapsed, state.dodgeCooldownRemaining, state.dodgeDirection,
        state.externalKnockbackVelocity, state.knockbackRemaining,
        arena_.player(clientId).value_or(game::ArenaPlayer{}).team, session.antiCheat.telemetry()};
}

ServerSnapshot ServerSimulation::snapshot() const {
    ServerSnapshot result;
    result.tick = simulation_.tickCount();
    result.elapsedSeconds = simulation_.elapsedSeconds();
    result.playerPosition = controller_.state().position;
    result.playerHealth = playerHealth_;
    result.bossPosition = boss_.position();
    result.bossHealth = boss_.currentHp();
    result.bossHitsReceived = combat_.dummy().hitCount;
    result.bossEncounterActive = bossEncounterActive_;
    result.activeTriggers = world_.activeTriggerIds();
    return result;
}

} // namespace glory::server

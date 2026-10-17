#include "glory/game/combat.hpp"
#include "glory/game/classes.hpp"

#include <nlohmann/json.hpp>

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace glory::game {
namespace {

using Json = nlohmann::json;
constexpr float degreesToRadians = 0.01745329251994329577F;

float readFiniteFloat(const Json& object, const char* key, const std::string& context) {
    if (!object.contains(key) || !object.at(key).is_number())
        throw std::runtime_error(context + ": missing or non-numeric field '" + key + "'");
    const float value = object.at(key).get<float>();
    if (!std::isfinite(value)) throw std::runtime_error(context + ": field '" + key + "' must be finite");
    return value;
}

std::string readString(const Json& object, const char* key, const std::string& context) {
    if (!object.contains(key) || !object.at(key).is_string())
        throw std::runtime_error(context + ": missing or non-string field '" + key + "'");
    const std::string value = object.at(key).get<std::string>();
    if (value.empty()) throw std::runtime_error(context + ": field '" + key + "' cannot be empty");
    return value;
}

float readOptionalFloat(const Json& object, const char* key, float fallback, const std::string& context) {
    return object.contains(key) ? readFiniteFloat(object, key, context) : fallback;
}

StatusEffectKind readEffectKind(const std::string& type, const std::string& context) {
    if (type == "AttackBoost") return StatusEffectKind::AttackBoost;
    if (type == "SpeedBoost") return StatusEffectKind::SpeedBoost;
    if (type == "Regen") return StatusEffectKind::Regen;
    if (type == "Bleed") return StatusEffectKind::Bleed;
    if (type == "Burn") return StatusEffectKind::Burn;
    if (type == "Slow") return StatusEffectKind::Slow;
    if (type == "Stun") return StatusEffectKind::Stun;
    if (type == "ArmorBreak") return StatusEffectKind::ArmorBreak;
    throw std::runtime_error(context + ": unknown status effect type '" + type + "'");
}

} // namespace

SkillRegistry SkillRegistry::loadFromFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Unable to open skills JSON: " + path.string());
    Json root;
    try {
        input >> root;
    } catch (const Json::exception& error) {
        throw std::runtime_error("Invalid skills JSON '" + path.string() + "': " + error.what());
    }
    if (!root.is_object() || !root.contains("skills") || !root.at("skills").is_array())
        throw std::runtime_error("Skills JSON must contain a 'skills' array");

    SkillRegistry registry;
    std::unordered_set<std::string> ids;
    for (const Json& entry : root.at("skills")) {
        if (!entry.is_object()) throw std::runtime_error("Each skill entry must be an object");
        SkillDefinition skill;
        skill.id = readString(entry, "id", "skill");
        const std::string context = "skill '" + skill.id + "'";
        skill.name = readString(entry, "name", context);
        skill.damage = readFiniteFloat(entry, "damage", context);
        skill.cooldown = readFiniteFloat(entry, "cooldown", context);
        skill.castTime = readFiniteFloat(entry, "castTime", context);
        skill.animationState = readString(entry, "animationState", context);
        skill.comboNext = entry.contains("comboNext") ? readString(entry, "comboNext", context) : "";
        skill.comboWindow = readOptionalFloat(entry, "comboWindow", 0.3F, context);
        skill.inputBuffer = readOptionalFloat(entry, "inputBuffer", 0.15F, context);
        skill.cancelWindowStart = readOptionalFloat(entry, "cancelWindowStart", 0.3F, context);
        skill.hitStopDuration = readOptionalFloat(entry, "hitStopDuration", 0.06F, context);
        skill.knockbackSpeed = readOptionalFloat(entry, "knockbackSpeed", 3.5F, context);
        skill.knockbackDuration = readOptionalFloat(entry, "knockbackDuration", 0.25F, context);
        if (entry.contains("onHitEffects")) {
            if (!entry.at("onHitEffects").is_array()) throw std::runtime_error(context + ": onHitEffects must be an array");
            for (const Json& effectJson : entry.at("onHitEffects")) {
                StatusEffect effect;
                effect.id = readString(effectJson, "id", context + " effect");
                effect.name = readString(effectJson, "name", context + " effect");
                effect.kind = readEffectKind(readString(effectJson, "type", context + " effect"), context);
                effect.duration = readFiniteFloat(effectJson, "duration", context + " effect");
                effect.magnitude = readFiniteFloat(effectJson, "magnitude", context + " effect");
                effect.tickInterval = readOptionalFloat(effectJson, "tickInterval", 0.0F, context + " effect");
                effect.maxStacks = static_cast<unsigned int>(std::max(1.0F,
                    readOptionalFloat(effectJson, "maxStacks", 1.0F, context + " effect")));
                if (effect.duration <= 0.0F || effect.magnitude < 0.0F || effect.tickInterval < 0.0F ||
                    ((effect.kind == StatusEffectKind::Regen || effect.kind == StatusEffectKind::Bleed ||
                      effect.kind == StatusEffectKind::Burn) && effect.tickInterval <= 0.0F))
                    throw std::runtime_error(context + ": invalid on-hit status effect values");
                skill.onHitEffects.push_back(std::move(effect));
            }
        }
        if (entry.contains("animationEvents")) {
            if (!entry.at("animationEvents").is_array())
                throw std::runtime_error(context + ": animationEvents must be an array");
            for (const Json& eventJson : entry.at("animationEvents")) {
                if (!eventJson.is_object()) throw std::runtime_error(context + ": animation event must be an object");
                SkillDefinition::AnimationEvent event;
                event.time = readFiniteFloat(eventJson, "time", context + " animation event");
                event.type = readString(eventJson, "type", context + " animation event");
                event.name = readString(eventJson, "name", context + " animation event");
                if (event.time < 0.0F || (event.type != "sound" && event.type != "particle"))
                    throw std::runtime_error(context + ": invalid animation event time or type");
                skill.animationEvents.push_back(std::move(event));
            }
        }
        if (!entry.contains("hitbox") || !entry.at("hitbox").is_object())
            throw std::runtime_error(context + ": missing hitbox object");
        const Json& hitbox = entry.at("hitbox");
        if (readString(hitbox, "shape", context + " hitbox") != "sphere")
            throw std::runtime_error(context + ": only sphere hitboxes are supported");
        skill.hitboxRadius = readFiniteFloat(hitbox, "radius", context + " hitbox");
        skill.activeStart = readFiniteFloat(hitbox, "activeStart", context + " hitbox");
        skill.activeEnd = readFiniteFloat(hitbox, "activeEnd", context + " hitbox");
        if (!hitbox.contains("offset") || !hitbox.at("offset").is_array() || hitbox.at("offset").size() != 3 ||
            !std::all_of(hitbox.at("offset").begin(), hitbox.at("offset").end(),
                [](const Json& component) { return component.is_number() && std::isfinite(component.get<float>()); }))
            throw std::runtime_error(context + ": hitbox offset must be three finite numbers");
        skill.hitboxOffset = {hitbox.at("offset")[0].get<float>(), hitbox.at("offset")[1].get<float>(),
            hitbox.at("offset")[2].get<float>()};
        if (skill.damage <= 0.0F || skill.cooldown < 0.0F || skill.castTime < 0.0F ||
            skill.hitboxRadius <= 0.0F || skill.activeStart < 0.0F || skill.activeEnd <= skill.activeStart ||
            skill.comboWindow < 0.0F || skill.inputBuffer < 0.0F ||
            skill.cancelWindowStart <= skill.activeEnd ||
            skill.cancelWindowStart > skill.activeEnd + skill.comboWindow || skill.hitStopDuration < 0.0F ||
            skill.knockbackSpeed < 0.0F || skill.knockbackDuration < 0.0F)
            throw std::runtime_error(context + ": invalid damage, hitbox, timing, or impact metadata");
        if (!ids.insert(skill.id).second) throw std::runtime_error("Duplicate skill id '" + skill.id + "'");
        registry.skills_.push_back(std::move(skill));
    }
    if (registry.skills_.empty()) throw std::runtime_error("Skills JSON contains no skills");
    for (const SkillDefinition& skill : registry.skills()) {
        if (!skill.comboNext.empty() && !ids.contains(skill.comboNext))
            throw std::runtime_error("Skill '" + skill.id + "' references unknown comboNext '" + skill.comboNext + "'");
    }
    return registry;
}

SkillRegistry SkillRegistry::fromSkills(std::vector<SkillDefinition> skills) {
    if (skills.empty()) throw std::runtime_error("Skills JSON contains no skills");
    std::unordered_set<std::string> ids;
    for (const SkillDefinition& skill : skills) {
        const auto finiteNonNegative = [](float value) { return std::isfinite(value) && value >= 0.0F; };
        if (skill.id.empty() || skill.name.empty() || skill.animationState.empty() ||
            !ids.insert(skill.id).second || !std::isfinite(skill.damage) || skill.damage <= 0.0F ||
            !finiteNonNegative(skill.cooldown) || !finiteNonNegative(skill.castTime) ||
            !std::isfinite(skill.hitboxRadius) || skill.hitboxRadius <= 0.0F ||
            !finiteNonNegative(skill.activeStart) || !std::isfinite(skill.activeEnd) ||
            skill.activeEnd <= skill.activeStart || !finiteNonNegative(skill.comboWindow) ||
            !finiteNonNegative(skill.inputBuffer) || !finiteNonNegative(skill.cancelWindowStart) ||
            skill.cancelWindowStart <= skill.activeEnd ||
            skill.cancelWindowStart > skill.activeEnd + skill.comboWindow ||
            !finiteNonNegative(skill.hitStopDuration) || !finiteNonNegative(skill.knockbackSpeed) ||
            !finiteNonNegative(skill.knockbackDuration) || !std::isfinite(skill.hitboxOffset.x) ||
            !std::isfinite(skill.hitboxOffset.y) || !std::isfinite(skill.hitboxOffset.z))
            throw std::runtime_error("Invalid skill data for '" + skill.id + "'");
        for (const auto& effect : skill.onHitEffects) {
            if (effect.id.empty() || effect.name.empty() || !std::isfinite(effect.duration) ||
                effect.duration <= 0.0F || !finiteNonNegative(effect.magnitude) ||
                !finiteNonNegative(effect.tickInterval) ||
                ((effect.kind == StatusEffectKind::Regen || effect.kind == StatusEffectKind::Bleed ||
                  effect.kind == StatusEffectKind::Burn) && effect.tickInterval <= 0.0F))
                throw std::runtime_error("Invalid on-hit effect in skill '" + skill.id + "'");
        }
        for (const auto& event : skill.animationEvents) {
            if (!finiteNonNegative(event.time) || event.name.empty() ||
                (event.type != "sound" && event.type != "particle"))
                throw std::runtime_error("Invalid animation event in skill '" + skill.id + "'");
        }
    }
    for (const SkillDefinition& skill : skills) {
        if (!skill.comboNext.empty() && !ids.contains(skill.comboNext))
            throw std::runtime_error("Skill '" + skill.id + "' references unknown comboNext '" + skill.comboNext + "'");
    }
    SkillRegistry registry;
    registry.skills_ = std::move(skills);
    return registry;
}

void SkillRegistry::saveToFile(const std::filesystem::path& path) const {
    (void)fromSkills(skills_);
    const auto effectName = [](StatusEffectKind kind) -> const char* {
        switch (kind) {
        case StatusEffectKind::AttackBoost: return "AttackBoost";
        case StatusEffectKind::SpeedBoost: return "SpeedBoost";
        case StatusEffectKind::Regen: return "Regen";
        case StatusEffectKind::Bleed: return "Bleed";
        case StatusEffectKind::Burn: return "Burn";
        case StatusEffectKind::Slow: return "Slow";
        case StatusEffectKind::Stun: return "Stun";
        case StatusEffectKind::ArmorBreak: return "ArmorBreak";
        }
        throw std::runtime_error("Unknown status effect kind");
    };
    Json root = Json::object();
    root["skills"] = Json::array();
    for (const SkillDefinition& skill : skills_) {
        Json entry{{"id", skill.id}, {"name", skill.name}, {"damage", skill.damage},
            {"cooldown", skill.cooldown}, {"castTime", skill.castTime},
            {"animationState", skill.animationState},
            {"comboWindow", skill.comboWindow}, {"inputBuffer", skill.inputBuffer},
            {"cancelWindowStart", skill.cancelWindowStart}, {"hitStopDuration", skill.hitStopDuration},
            {"knockbackSpeed", skill.knockbackSpeed}, {"knockbackDuration", skill.knockbackDuration},
            {"hitbox", {{"shape", "sphere"}, {"radius", skill.hitboxRadius},
                {"offset", {skill.hitboxOffset.x, skill.hitboxOffset.y, skill.hitboxOffset.z}},
                {"activeStart", skill.activeStart}, {"activeEnd", skill.activeEnd}}}};
        if (!skill.comboNext.empty()) entry["comboNext"] = skill.comboNext;
        if (!skill.onHitEffects.empty()) {
            entry["onHitEffects"] = Json::array();
            for (const auto& effect : skill.onHitEffects)
                entry["onHitEffects"].push_back({{"id", effect.id}, {"name", effect.name},
                    {"type", effectName(effect.kind)}, {"duration", effect.duration},
                    {"magnitude", effect.magnitude}, {"tickInterval", effect.tickInterval},
                    {"maxStacks", effect.maxStacks}});
        }
        if (!skill.animationEvents.empty()) {
            entry["animationEvents"] = Json::array();
            for (const auto& event : skill.animationEvents)
                entry["animationEvents"].push_back({{"time", event.time}, {"type", event.type}, {"name", event.name}});
        }
        root["skills"].push_back(std::move(entry));
    }
    const std::filesystem::path temporaryPath = path.string() + ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Unable to write skills JSON: " + temporaryPath.string());
        output << root.dump(4) << '\n';
        if (!output) throw std::runtime_error("Failed while writing skills JSON: " + temporaryPath.string());
    }
#ifdef _WIN32
    if (!MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const std::error_code error(static_cast<int>(GetLastError()), std::system_category());
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Unable to replace skills JSON: " + error.message());
    }
#else
    std::error_code error;
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Unable to replace skills JSON: " + error.message());
    }
#endif
}

const SkillDefinition* SkillRegistry::find(const std::string& id) const noexcept {
    const auto found = std::find_if(skills_.begin(), skills_.end(), [&](const SkillDefinition& skill) {
        return skill.id == id;
    });
    return found == skills_.end() ? nullptr : &*found;
}

bool intersects(const Hitbox& hitbox, const Hurtbox& hurtbox) noexcept {
    if (!std::isfinite(hitbox.radius) || hitbox.radius < 0.0F || !std::isfinite(hurtbox.radius) ||
        hurtbox.radius < 0.0F || !std::isfinite(hurtbox.height) || hurtbox.height < 0.0F) return false;
    const glm::vec3 segmentStart = hurtbox.base + glm::vec3(0.0F, hurtbox.radius, 0.0F);
    const glm::vec3 segmentEnd = hurtbox.base + glm::vec3(0.0F,
        std::max(hurtbox.radius, hurtbox.height - hurtbox.radius), 0.0F);
    const glm::vec3 segment = segmentEnd - segmentStart;
    const float segmentLengthSquared = glm::dot(segment, segment);
    const float t = segmentLengthSquared > 0.0F
        ? std::clamp(glm::dot(hitbox.center - segmentStart, segment) / segmentLengthSquared, 0.0F, 1.0F)
        : 0.0F;
    const glm::vec3 closest = segmentStart + t * segment;
    const float combinedRadius = hitbox.radius + hurtbox.radius;
    return glm::dot(hitbox.center - closest, hitbox.center - closest) <= combinedRadius * combinedRadius;
}

CombatSystem::CombatSystem(SkillRegistry registry, CombatTarget dummy, CollisionWorld collisionWorld)
    : registry_(std::move(registry)), dummy_(dummy), collisionWorld_(std::move(collisionWorld)) {
    if (dummy_.entityId == 0) dummy_.entityId = 1;
    if (!std::isfinite(dummy_.maxHp) || dummy_.maxHp <= 0.0F) dummy_.maxHp = 100.0F;
    dummy_.currentHp = std::clamp(dummy_.currentHp, 0.0F, dummy_.maxHp);
    for (const SkillDefinition& skill : registry_.skills()) cooldowns_.emplace(skill.id, 0.0F);
}

void CombatSystem::replaceRegistry(SkillRegistry registry) {
    std::unordered_map<std::string, float> updatedCooldowns;
    for (const SkillDefinition& skill : registry.skills()) {
        const auto current = cooldowns_.find(skill.id);
        updatedCooldowns.emplace(skill.id, current == cooldowns_.end()
            ? 0.0F : std::min(current->second, skill.cooldown));
    }
    registry_ = std::move(registry);
    cooldowns_ = std::move(updatedCooldowns);
}

bool CombatSystem::startSkill(const std::string& skillId, bool comboTransition) {
    const SkillDefinition* skill = registry_.find(skillId);
    if (!skill || cooldowns_[skillId] > 0.0F) return false;
    activeSkillId_ = skillId;
    activeSkillElapsed_ = 0.0F;
    castElapsed_ = 0.0F;
    comboBufferRemaining_ = 0.0F;
    comboWindowOpen_ = false;
    cancelWindowAvailable_ = false;
    casting_ = skill->castTime > 0.0F;
    attacking_ = true;
    cooldowns_[skillId] = skill->cooldown;
    hitTargets_.clear();
    animationState_ = skill->animationState;
    attackAnimationRequested_ = true;
    if (comboTransition) ++comboStep_;
    else if (skillId == "combo_1") comboStep_ = 1;
    return true;
}

void CombatSystem::updateDummy(float deltaSeconds) noexcept {
    const float speed = glm::length(dummy_.knockbackVelocity);
    if (speed <= 1.0e-5F || knockbackRemaining_ <= 0.0F) {
        dummy_.knockbackVelocity = glm::vec3(0.0F);
        knockbackRemaining_ = 0.0F;
        return;
    }
    const float step = std::min(deltaSeconds, knockbackRemaining_);
    const float deceleration = speed / knockbackRemaining_;
    const float nextSpeed = std::max(0.0F, speed - deceleration * step);
    const glm::vec3 direction = dummy_.knockbackVelocity / speed;
    const glm::vec3 desiredPosition = dummy_.hurtbox.base + direction * ((speed + nextSpeed) * 0.5F * step);
    const glm::vec3 nextVelocity = direction * nextSpeed;
    const CollisionResult collision = collisionWorld_.resolve(
        {dummy_.hurtbox.base, dummy_.hurtbox.radius, dummy_.hurtbox.height}, desiredPosition,
        nextVelocity, 0.25F);
    dummy_.hurtbox.base = collision.position;
    dummy_.knockbackVelocity = collision.velocity;
    knockbackRemaining_ = std::max(0.0F, knockbackRemaining_ - step);
    if (knockbackRemaining_ <= 0.0F || glm::length(dummy_.knockbackVelocity) <= 1.0e-4F) {
        dummy_.knockbackVelocity = glm::vec3(0.0F);
        knockbackRemaining_ = 0.0F;
    }
}

void CombatSystem::update(float deltaSeconds, const glm::vec3& ownerPosition, float facingYawDegrees,
    bool attackPressed, const std::string& animationState, bool dodgePressed, bool ownerDodging,
    float damageDealtMultiplier, bool ownerStunned, const Hurtbox* hitTestHurtbox) {
    hitEvents_.clear();
    attackAnimationRequested_ = false;
    attackCanceled_ = false;
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F || !std::isfinite(facingYawDegrees)) return;

    if (hitStopRemaining_ > 0.0F) {
        if (attackPressed && attacking_) {
            const SkillDefinition* current = registry_.find(activeSkillId_);
            if (current && !current->comboNext.empty()) comboBufferRemaining_ = current->inputBuffer;
        }
        hitStopRemaining_ = std::max(0.0F, hitStopRemaining_ - deltaSeconds);
        return;
    }
    for (auto& [id, remaining] : cooldowns_) remaining = std::max(0.0F, remaining - deltaSeconds);
    updateDummy(deltaSeconds);
    dummy_.statusEffects.update(deltaSeconds, dummy_.currentHp, dummy_.maxHp);

    if (attacking_ && dodgePressed && ownerDodging && cancelWindowAvailable_) {
        attacking_ = false;
        casting_ = false;
        activeSkillId_.clear();
        comboStep_ = 1;
        comboBufferRemaining_ = 0.0F;
        comboWindowOpen_ = false;
        cancelWindowAvailable_ = false;
        attackCanceled_ = true;
        animationState_ = "Idle";
        return;
    }

    if (comboBufferRemaining_ > 0.0F) comboBufferRemaining_ = std::max(0.0F, comboBufferRemaining_ - deltaSeconds);
    if (!attacking_) {
        const std::string requested = pendingSkillId_.empty()
            ? (attackPressed && !ownerStunned ? "combo_1" : "") : (ownerStunned ? "" : pendingSkillId_);
        pendingSkillId_.clear();
        if (!requested.empty()) (void)startSkill(requested, false);
    } else if (attackPressed && !ownerStunned) {
        const SkillDefinition* current = registry_.find(activeSkillId_);
        if (current && !current->comboNext.empty()) {
            if (comboWindowOpen_) {
                if (startSkill(current->comboNext, true)) return;
                comboBufferRemaining_ = 0.0F;
            } else {
                comboBufferRemaining_ = current->inputBuffer;
            }
        }
    }

    const SkillDefinition* skill = registry_.find(activeSkillId_);
    if (!skill || !attacking_) {
        animationState_ = animationState;
        comboWindowOpen_ = false;
        cancelWindowAvailable_ = false;
        return;
    }
    animationState_ = animationState;
    if (animationState_ != skill->animationState) return;

    if (casting_) {
        castElapsed_ += deltaSeconds;
        if (castElapsed_ < skill->castTime) return;
        casting_ = false;
        activeSkillElapsed_ = std::max(0.0F, castElapsed_ - skill->castTime);
    } else {
        activeSkillElapsed_ += deltaSeconds;
    }

    if (activeSkillElapsed_ >= skill->activeStart && activeSkillElapsed_ <= skill->activeEnd &&
        !hitTargets_.contains(dummy_.entityId)) {
        const float yaw = facingYawDegrees * degreesToRadians;
        const glm::vec3 localOffset = skill->hitboxOffset;
        const glm::vec3 rotatedOffset{
            std::cos(yaw) * localOffset.x + std::sin(yaw) * localOffset.z,
            localOffset.y,
            -std::sin(yaw) * localOffset.x + std::cos(yaw) * localOffset.z,
        };
        const Hitbox hitbox{ownerPosition + rotatedOffset, skill->hitboxRadius};
        const Hurtbox& targetHurtbox = hitTestHurtbox ? *hitTestHurtbox : dummy_.hurtbox;
        if (dummy_.currentHp > 0.0F && intersects(hitbox, targetHurtbox)) {
            hitTargets_.insert(dummy_.entityId);
            const float outgoing = std::isfinite(damageDealtMultiplier)
                ? std::clamp(damageDealtMultiplier, 0.0F, 10.0F) : 1.0F;
            const float damage = calculateDamageTaken(skill->damage * outgoing *
                dummy_.statusEffects.calculateDamageTakenMultiplier(), dummy_.defense);
            dummy_.currentHp = std::max(0.0F, dummy_.currentHp - damage);
            ++dummy_.hitCount;
            hitEvents_.push_back({dummy_.entityId, hitbox.center, damage});
            for (StatusEffect effect : skill->onHitEffects) dummy_.statusEffects.apply(std::move(effect));
            const glm::vec3 delta = dummy_.hurtbox.base - ownerPosition;
            const glm::vec3 fallback{std::cos(yaw), 0.0F, std::sin(yaw)};
            glm::vec3 direction{delta.x, 0.0F, delta.z};
            const float directionLength = glm::length(direction);
            direction = directionLength > 1.0e-5F ? direction / directionLength : fallback;
            dummy_.knockbackVelocity = direction * skill->knockbackSpeed;
            knockbackRemaining_ = skill->knockbackDuration;
            hitStopRemaining_ = skill->hitStopDuration;
        }
    }

    comboWindowOpen_ = !skill->comboNext.empty() && activeSkillElapsed_ > skill->activeEnd &&
        activeSkillElapsed_ <= skill->activeEnd + skill->comboWindow;
    cancelWindowAvailable_ = activeSkillElapsed_ >= skill->cancelWindowStart &&
        activeSkillElapsed_ <= skill->activeEnd + skill->comboWindow;

    if (comboWindowOpen_ && comboBufferRemaining_ > 0.0F) {
        if (startSkill(skill->comboNext, true)) return;
        comboBufferRemaining_ = 0.0F;
    }
    if (activeSkillElapsed_ > skill->activeEnd + skill->comboWindow) {
        attacking_ = false;
        casting_ = false;
        activeSkillId_.clear();
        comboStep_ = 1;
        comboBufferRemaining_ = 0.0F;
        comboWindowOpen_ = false;
        cancelWindowAvailable_ = false;
        animationState_ = "Idle";
    }
}

void CombatSystem::requestSkill(const std::string& skillId) {
    if (registry_.find(skillId)) pendingSkillId_ = skillId;
}

float CombatSystem::maximumAttackRange() const noexcept {
    float maximumRange = 0.0F;
    for (const SkillDefinition& skill : registry_.skills())
        maximumRange = std::max(maximumRange, glm::length(glm::vec2(skill.hitboxOffset.x, skill.hitboxOffset.z)) +
            skill.hitboxRadius + dummy_.hurtbox.radius);
    return maximumRange;
}

float CombatSystem::activeSkillDamage() const noexcept {
    const SkillDefinition* skill = registry_.find(activeSkillId_);
    return skill ? skill->damage : -1.0F;
}

float CombatSystem::activeSkillCooldown() const noexcept {
    const SkillDefinition* skill = registry_.find(activeSkillId_);
    return skill ? skill->cooldown : -1.0F;
}

float CombatSystem::activeSkillHitboxRadius() const noexcept {
    const SkillDefinition* skill = registry_.find(activeSkillId_);
    return skill ? skill->hitboxRadius : -1.0F;
}

void CombatSystem::resetDummy() noexcept {
    dummy_.currentHp = dummy_.maxHp;
    dummy_.hitCount = 0;
    dummy_.hurtbox.base = {-1.15F, 0.0F, 0.0F};
    dummy_.knockbackVelocity = glm::vec3(0.0F);
    dummy_.statusEffects.clear();
    knockbackRemaining_ = 0.0F;
}

void CombatSystem::resetEncounter() noexcept {
    resetDummy();
    for (auto& [id, remaining] : cooldowns_) remaining = 0.0F;
    hitTargets_.clear();
    pendingSkillId_.clear();
    activeSkillId_.clear();
    animationState_ = "Idle";
    castElapsed_ = 0.0F;
    activeSkillElapsed_ = 0.0F;
    comboBufferRemaining_ = 0.0F;
    hitStopRemaining_ = 0.0F;
    knockbackRemaining_ = 0.0F;
    comboStep_ = 1;
    casting_ = false;
    attacking_ = false;
    comboWindowOpen_ = false;
    cancelWindowAvailable_ = false;
    attackAnimationRequested_ = false;
    attackCanceled_ = false;
}

std::vector<SkillStatus> CombatSystem::skillStatuses() const {
    std::vector<SkillStatus> result;
    result.reserve(registry_.skills().size());
    for (const SkillDefinition& skill : registry_.skills()) {
        const auto found = cooldowns_.find(skill.id);
        result.push_back({skill.id, skill.name, found == cooldowns_.end() ? 0.0F : found->second,
            skill.cooldown});
    }
    return result;
}

float CombatSystem::cooldownRemaining(const std::string& skillId) const noexcept {
    const auto found = cooldowns_.find(skillId);
    return found == cooldowns_.end() ? 0.0F : found->second;
}

} // namespace glory::game

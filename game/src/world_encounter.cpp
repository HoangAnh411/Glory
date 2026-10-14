#include "glory/game/world_encounter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace glory::game {
namespace {

using Json = nlohmann::json;

std::string readString(const Json& object, const char* field) {
    if (!object.contains(field) || !object.at(field).is_string() || object.at(field).get<std::string>().empty())
        throw std::runtime_error(std::string("Invalid or missing world field '") + field + "'");
    return object.at(field).get<std::string>();
}

float readFloat(const Json& object, const char* field) {
    if (!object.contains(field) || !object.at(field).is_number())
        throw std::runtime_error(std::string("Invalid or missing world field '") + field + "'");
    const float value = object.at(field).get<float>();
    if (!std::isfinite(value)) throw std::runtime_error(std::string("World field '") + field + "' must be finite");
    return value;
}

glm::vec3 readVector(const Json& object, const char* field) {
    if (!object.contains(field) || !object.at(field).is_array() || object.at(field).size() != 3)
        throw std::runtime_error(std::string("World field '") + field + "' must contain three values");
    const Json& value = object.at(field);
    for (const Json& component : value)
        if (!component.is_number() || !std::isfinite(component.get<float>()))
            throw std::runtime_error(std::string("World vector '") + field + "' must be finite");
    return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>()};
}

bool finiteVector(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool validEntityType(SpawnEntityType value) {
    return value == SpawnEntityType::Player || value == SpawnEntityType::PracticeDummy ||
        value == SpawnEntityType::Boss;
}

bool validShape(TriggerShape value) {
    return value == TriggerShape::Box || value == TriggerShape::Sphere;
}

bool validTriggerType(TriggerType value) {
    return value == TriggerType::EncounterBoundary || value == TriggerType::BossAggro ||
        value == TriggerType::HealArea || value == TriggerType::HazardArea;
}

SpawnEntityType parseEntityType(const std::string& value) {
    if (value == "Player") return SpawnEntityType::Player;
    if (value == "PracticeDummy") return SpawnEntityType::PracticeDummy;
    if (value == "Boss") return SpawnEntityType::Boss;
    throw std::runtime_error("Unknown spawn entityType '" + value + "'");
}

TriggerShape parseShape(const std::string& value) {
    if (value == "Box") return TriggerShape::Box;
    if (value == "Sphere") return TriggerShape::Sphere;
    throw std::runtime_error("Unknown trigger shape '" + value + "'");
}

TriggerType parseTriggerType(const std::string& value) {
    if (value == "EncounterBoundary") return TriggerType::EncounterBoundary;
    if (value == "BossAggro") return TriggerType::BossAggro;
    if (value == "HealArea") return TriggerType::HealArea;
    if (value == "HazardArea") return TriggerType::HazardArea;
    throw std::runtime_error("Unknown triggerType '" + value + "'");
}

const char* entityTypeName(SpawnEntityType value) {
    switch (value) {
    case SpawnEntityType::Player: return "Player";
    case SpawnEntityType::PracticeDummy: return "PracticeDummy";
    case SpawnEntityType::Boss: return "Boss";
    }
    throw std::runtime_error("Unknown spawn entity type");
}

const char* shapeName(TriggerShape value) {
    switch (value) {
    case TriggerShape::Box: return "Box";
    case TriggerShape::Sphere: return "Sphere";
    }
    throw std::runtime_error("Unknown trigger shape");
}

const char* triggerTypeName(TriggerType value) {
    switch (value) {
    case TriggerType::EncounterBoundary: return "EncounterBoundary";
    case TriggerType::BossAggro: return "BossAggro";
    case TriggerType::HealArea: return "HealArea";
    case TriggerType::HazardArea: return "HazardArea";
    }
    throw std::runtime_error("Unknown trigger type");
}

} // namespace

WorldEncounter WorldEncounter::loadFromFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Unable to open world spawns JSON: " + path.string());
    Json root;
    try {
        input >> root;
    } catch (const Json::exception& error) {
        throw std::runtime_error("Invalid world spawns JSON '" + path.string() + "': " + error.what());
    }
    if (!root.is_object() || !root.contains("spawnPoints") || !root.at("spawnPoints").is_array() ||
        !root.contains("triggerZones") || !root.at("triggerZones").is_array())
        throw std::runtime_error("World spawns JSON requires spawnPoints and triggerZones arrays");

    std::vector<SpawnPoint> spawns;
    for (const Json& entry : root.at("spawnPoints")) {
        if (!entry.is_object()) throw std::runtime_error("Each spawn point must be an object");
        spawns.push_back({readString(entry, "id"), readString(entry, "name"),
            parseEntityType(readString(entry, "entityType")), readVector(entry, "position"),
            readFloat(entry, "yaw")});
    }
    std::vector<TriggerZone> zones;
    for (const Json& entry : root.at("triggerZones")) {
        if (!entry.is_object()) throw std::runtime_error("Each trigger zone must be an object");
        TriggerZone zone;
        zone.id = readString(entry, "id");
        zone.name = readString(entry, "name");
        zone.shape = parseShape(readString(entry, "shape"));
        zone.center = readVector(entry, "center");
        zone.triggerType = parseTriggerType(readString(entry, "triggerType"));
        if (zone.shape == TriggerShape::Box) zone.extents = readVector(entry, "extents");
        else zone.radius = readFloat(entry, "radius");
        zones.push_back(std::move(zone));
    }
    return fromData(std::move(spawns), std::move(zones));
}

WorldEncounter WorldEncounter::fromData(std::vector<SpawnPoint> spawnPoints,
    std::vector<TriggerZone> triggerZones) {
    if (spawnPoints.empty()) throw std::runtime_error("World data must contain at least one spawn point");
    std::unordered_set<std::string> ids;
    for (const SpawnPoint& spawn : spawnPoints) {
        if (spawn.id.empty() || spawn.name.empty() || !validEntityType(spawn.entityType) ||
            !ids.insert(spawn.id).second ||
            !finiteVector(spawn.position) || !std::isfinite(spawn.yaw))
            throw std::runtime_error("Invalid or duplicate world spawn point '" + spawn.id + "'");
    }
    for (const TriggerZone& zone : triggerZones) {
        if (zone.id.empty() || zone.name.empty() || !validShape(zone.shape) ||
            !validTriggerType(zone.triggerType) || !ids.insert(zone.id).second || !finiteVector(zone.center))
            throw std::runtime_error("Invalid or duplicate world trigger zone '" + zone.id + "'");
        if (zone.shape == TriggerShape::Box && (!finiteVector(zone.extents) || zone.extents.x <= 0.0F ||
            zone.extents.y <= 0.0F || zone.extents.z <= 0.0F))
            throw std::runtime_error("Box trigger extents must be finite and positive");
        if (zone.shape == TriggerShape::Sphere && (!std::isfinite(zone.radius) || zone.radius <= 0.0F))
            throw std::runtime_error("Sphere trigger radius must be finite and positive");
    }
    WorldEncounter result;
    result.spawnPoints_ = std::move(spawnPoints);
    result.triggerZones_ = std::move(triggerZones);
    return result;
}

void WorldEncounter::saveToFile(const std::filesystem::path& path) const {
    (void)fromData(spawnPoints_, triggerZones_);
    Json root{{"spawnPoints", Json::array()}, {"triggerZones", Json::array()}};
    for (const SpawnPoint& spawn : spawnPoints_)
        root["spawnPoints"].push_back({{"id", spawn.id}, {"name", spawn.name},
            {"entityType", entityTypeName(spawn.entityType)},
            {"position", {spawn.position.x, spawn.position.y, spawn.position.z}}, {"yaw", spawn.yaw}});
    for (const TriggerZone& zone : triggerZones_) {
        Json entry{{"id", zone.id}, {"name", zone.name}, {"shape", shapeName(zone.shape)},
            {"center", {zone.center.x, zone.center.y, zone.center.z}},
            {"triggerType", triggerTypeName(zone.triggerType)}};
        if (zone.shape == TriggerShape::Box)
            entry["extents"] = {zone.extents.x, zone.extents.y, zone.extents.z};
        else entry["radius"] = zone.radius;
        root["triggerZones"].push_back(std::move(entry));
    }
    const std::filesystem::path temporaryPath = path.string() + ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Unable to write world spawns JSON: " + temporaryPath.string());
        output << root.dump(4) << '\n';
        if (!output) throw std::runtime_error("Failed while writing world spawns JSON: " + temporaryPath.string());
    }
#ifdef _WIN32
    if (!MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const std::error_code error(static_cast<int>(GetLastError()), std::system_category());
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Unable to replace world spawns JSON: " + error.message());
    }
#else
    std::error_code error;
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        std::filesystem::remove(temporaryPath);
        throw std::runtime_error("Unable to replace world spawns JSON: " + error.message());
    }
#endif
}

const SpawnPoint* WorldEncounter::findSpawn(SpawnEntityType entityType) const noexcept {
    const auto found = std::find_if(spawnPoints_.begin(), spawnPoints_.end(),
        [entityType](const SpawnPoint& spawn) { return spawn.entityType == entityType; });
    return found == spawnPoints_.end() ? nullptr : &*found;
}

const SpawnPoint* WorldEncounter::findSpawn(const std::string& id) const noexcept {
    const auto found = std::find_if(spawnPoints_.begin(), spawnPoints_.end(),
        [&id](const SpawnPoint& spawn) { return spawn.id == id; });
    return found == spawnPoints_.end() ? nullptr : &*found;
}

bool WorldEncounter::intersects(const glm::vec3& point, const TriggerZone& zone) noexcept {
    if (!finiteVector(point) || !finiteVector(zone.center)) return false;
    const glm::vec3 relative = point - zone.center;
    if (zone.shape == TriggerShape::Box)
        return std::abs(relative.x) <= zone.extents.x && std::abs(relative.y) <= zone.extents.y &&
            std::abs(relative.z) <= zone.extents.z;
    return std::isfinite(zone.radius) && zone.radius >= 0.0F &&
        relative.x * relative.x + relative.y * relative.y + relative.z * relative.z <= zone.radius * zone.radius;
}

std::vector<WorldTriggerEvent> WorldEncounter::updateTriggers(const glm::vec3& playerPosition) {
    std::unordered_set<std::string> previous(activeTriggerIds_.begin(), activeTriggerIds_.end());
    std::vector<std::string> currentlyActive;
    std::vector<WorldTriggerEvent> events;
    for (const TriggerZone& zone : triggerZones_) {
        const bool wasInside = previous.contains(zone.id);
        const bool isInside = intersects(playerPosition, zone);
        if (isInside) currentlyActive.push_back(zone.id);
        if (isInside)
            events.push_back({zone.id, zone.name, zone.triggerType,
                wasInside ? TriggerTransition::Inside : TriggerTransition::Enter});
        else if (wasInside)
            events.push_back({zone.id, zone.name, zone.triggerType, TriggerTransition::Exit});
    }
    activeTriggerIds_ = std::move(currentlyActive);
    return events;
}

std::vector<std::string> WorldEncounter::activeTriggerIds() const {
    return activeTriggerIds_;
}

} // namespace glory::game

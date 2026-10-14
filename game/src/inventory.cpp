#include "glory/game/inventory.hpp"

#include <nlohmann/json.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace glory::game {
namespace {

using Json = nlohmann::json;

ItemType parseItemType(const std::string& value) {
    if (value == "Weapon") return ItemType::Weapon;
    if (value == "Armor") return ItemType::Armor;
    if (value == "Accessory") return ItemType::Accessory;
    if (value == "Consumable") return ItemType::Consumable;
    throw std::runtime_error("Unknown item type: " + value);
}

bool validId(const std::string& value) {
    return !value.empty() && value.size() <= 32 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_';
    });
}

std::size_t slotIndex(EquipmentSlot slot) { return static_cast<std::size_t>(slot); }

} // namespace

ItemCatalog ItemCatalog::loadFromFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Unable to open item catalog: " + path.string());
    const Json root = Json::parse(input);
    if (!root.is_object() || !root.contains("items") || !root["items"].is_array())
        throw std::runtime_error("Item catalog must contain an items array");
    ItemCatalog catalog;
    std::unordered_set<std::string> identifiers;
    for (const Json& value : root["items"]) {
        ItemDefinition item;
        item.id = value.at("id").get<std::string>();
        item.name = value.at("name").get<std::string>();
        item.type = parseItemType(value.at("type").get<std::string>());
        item.rarity = value.at("rarity").get<std::string>();
        item.bonusAttack = value.at("bonusAttack").get<int>();
        item.bonusDefense = value.at("bonusDefense").get<int>();
        item.bonusMaxHp = value.at("bonusMaxHp").get<int>();
        item.maxStack = value.at("maxStack").get<int>();
        if (!validId(item.id) || item.name.empty() || item.name.size() > 80 || item.rarity.empty() ||
            item.rarity.size() > 24 || item.maxStack < 1 || item.maxStack > 999 ||
            (item.type != ItemType::Consumable && item.maxStack != 1) ||
            item.bonusAttack < 0 || item.bonusAttack > 100000 || item.bonusDefense < 0 ||
            item.bonusDefense > 100000 || item.bonusMaxHp < 0 || item.bonusMaxHp > 100000 ||
            !identifiers.insert(item.id).second)
            throw std::runtime_error("Invalid or duplicate item definition: " + item.id);
        catalog.items_.push_back(std::move(item));
    }
    if (catalog.items_.empty()) throw std::runtime_error("Item catalog cannot be empty");
    return catalog;
}

const ItemDefinition* ItemCatalog::find(const std::string& itemId) const noexcept {
    const auto found = std::find_if(items_.begin(), items_.end(), [&](const ItemDefinition& item) {
        return item.id == itemId;
    });
    return found == items_.end() ? nullptr : &*found;
}

bool Inventory::addItem(const std::string& itemId, int quantity, const ItemCatalog& catalog) {
    const ItemDefinition* definition = catalog.find(itemId);
    if (!definition || quantity <= 0 || quantity > definition->maxStack * static_cast<int>(capacity)) return false;
    auto updated = slots_;
    int remaining = quantity;
    for (auto& slot : updated) {
        if (slot.itemId != itemId || slot.quantity >= definition->maxStack) continue;
        const int added = std::min(remaining, definition->maxStack - slot.quantity);
        slot.quantity += added;
        remaining -= added;
        if (remaining == 0) { slots_ = std::move(updated); return true; }
    }
    for (auto& slot : updated) {
        if (!slot.empty()) continue;
        const int added = std::min(remaining, definition->maxStack);
        slot = {itemId, added};
        remaining -= added;
        if (remaining == 0) { slots_ = std::move(updated); return true; }
    }
    return false;
}

bool Inventory::removeItem(std::size_t slotIndexValue, int quantity) {
    if (slotIndexValue >= capacity || quantity <= 0 || slots_[slotIndexValue].empty() ||
        quantity > slots_[slotIndexValue].quantity) return false;
    auto& slot = slots_[slotIndexValue];
    slot.quantity -= quantity;
    if (slot.quantity == 0) slot = {};
    return true;
}

bool Inventory::moveItem(std::size_t from, std::size_t to, const ItemCatalog& catalog) {
    if (from >= capacity || to >= capacity || from == to || slots_[from].empty()) return false;
    auto updated = slots_;
    if (updated[to].empty()) {
        std::swap(updated[from], updated[to]);
    } else if (updated[from].itemId == updated[to].itemId) {
        const ItemDefinition* definition = catalog.find(updated[from].itemId);
        if (!definition || updated[to].quantity >= definition->maxStack) return false;
        const int moved = std::min(updated[from].quantity, definition->maxStack - updated[to].quantity);
        updated[to].quantity += moved;
        updated[from].quantity -= moved;
        if (updated[from].quantity == 0) updated[from] = {};
    } else {
        std::swap(updated[from], updated[to]);
    }
    slots_ = std::move(updated);
    return true;
}

bool Inventory::restore(const std::vector<std::pair<std::size_t, InventorySlot>>& slots,
    const ItemCatalog& catalog) {
    std::array<InventorySlot, capacity> restored{};
    for (const auto& [index, slot] : slots) {
        const ItemDefinition* definition = catalog.find(slot.itemId);
        if (index >= capacity || !definition || slot.quantity <= 0 ||
            slot.quantity > definition->maxStack || !restored[index].empty()) return false;
        restored[index] = slot;
    }
    slots_ = std::move(restored);
    return true;
}

bool EquipmentManager::equip(Inventory& inventory, std::size_t inventorySlot, EquipmentSlot slot,
    const ItemCatalog& catalog) {
    if (slotIndex(slot) >= items_.size() || inventorySlot >= Inventory::capacity) return false;
    const InventorySlot& source = inventory.slots()[inventorySlot];
    const ItemDefinition* incoming = catalog.find(source.itemId);
    if (!incoming || equipmentSlotFor(incoming->type) != slot || source.quantity < 1) return false;
    const std::string incomingId = source.itemId;
    const auto existing = items_[slotIndex(slot)];
    if (existing && *existing == source.itemId) return true;
    Inventory updated = inventory;
    if (!updated.removeItem(inventorySlot, 1)) return false;
    if (existing && !updated.addItem(*existing, 1, catalog)) return false;
    inventory = std::move(updated);
    items_[slotIndex(slot)] = incomingId;
    return true;
}

bool EquipmentManager::unequip(Inventory& inventory, EquipmentSlot slot, const ItemCatalog& catalog) {
    if (slotIndex(slot) >= items_.size() || !items_[slotIndex(slot)]) return false;
    Inventory updated = inventory;
    if (!updated.addItem(*items_[slotIndex(slot)], 1, catalog)) return false;
    inventory = std::move(updated);
    items_[slotIndex(slot)].reset();
    return true;
}

CharacterStats EquipmentManager::stats(const ItemCatalog& catalog, CharacterStats base) const {
    CharacterStats result = base;
    for (const auto& itemId : items_) {
        if (!itemId) continue;
        const ItemDefinition* definition = catalog.find(*itemId);
        if (!definition) continue;
        result.attack += definition->bonusAttack;
        result.defense += definition->bonusDefense;
        result.maxHp += definition->bonusMaxHp;
    }
    return result;
}

bool EquipmentManager::restore(const std::vector<EquipmentItem>& items, const ItemCatalog& catalog) {
    std::array<std::optional<std::string>, 3> restored{};
    for (const EquipmentItem& item : items) {
        const std::size_t index = slotIndex(item.slot);
        const ItemDefinition* definition = catalog.find(item.itemId);
        if (index >= restored.size() || !definition || equipmentSlotFor(definition->type) != item.slot ||
            restored[index]) return false;
        restored[index] = item.itemId;
    }
    items_ = std::move(restored);
    return true;
}

std::uint64_t LootDropSystem::spawnLoot(const std::string& itemId, int quantity,
    const glm::vec3& position, const ItemCatalog& catalog) {
    if (!catalog.find(itemId) || quantity <= 0 || quantity > 999 || drops_.size() >= 16 ||
        !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) return 0;
    const std::uint64_t lootId = nextLootId_++;
    drops_.push_back({lootId, position, itemId, quantity});
    return lootId;
}

bool LootDropSystem::pickupLoot(std::uint64_t lootId, const glm::vec3& playerPosition,
    Inventory& inventory, const ItemCatalog& catalog, float pickupRange) {
    const auto found = std::find_if(drops_.begin(), drops_.end(), [lootId](const LootDrop& drop) {
        return drop.lootId == lootId;
    });
    const bool finitePlayerPosition = std::isfinite(playerPosition.x) && std::isfinite(playerPosition.y) &&
        std::isfinite(playerPosition.z);
    const glm::vec3 offset = found == drops_.end() ? glm::vec3(0.0F) : found->position - playerPosition;
    if (found == drops_.end() || !std::isfinite(pickupRange) || pickupRange < 0.0F ||
        !finitePlayerPosition || glm::dot(offset, offset) > pickupRange * pickupRange ||
        !inventory.addItem(found->itemId, found->quantity, catalog)) return false;
    drops_.erase(found);
    return true;
}

bool LootDropSystem::restoreDrops(std::vector<LootDrop> drops, const ItemCatalog& catalog) {
    if (drops.size() > 16) return false;
    std::unordered_set<std::uint64_t> identifiers;
    std::uint64_t nextId = 1;
    for (const LootDrop& drop : drops) {
        if (drop.lootId == 0 || !identifiers.insert(drop.lootId).second || !catalog.find(drop.itemId) ||
            drop.quantity <= 0 || drop.quantity > 999 || !std::isfinite(drop.position.x) ||
            !std::isfinite(drop.position.y) || !std::isfinite(drop.position.z)) return false;
        nextId = std::max(nextId, drop.lootId + 1);
    }
    drops_ = std::move(drops);
    nextLootId_ = nextId;
    return true;
}

const char* equipmentSlotName(EquipmentSlot slot) noexcept {
    switch (slot) {
    case EquipmentSlot::Weapon: return "Weapon";
    case EquipmentSlot::Armor: return "Armor";
    case EquipmentSlot::Accessory: return "Accessory";
    }
    return "Unknown";
}

std::optional<EquipmentSlot> equipmentSlotFor(ItemType type) noexcept {
    switch (type) {
    case ItemType::Weapon: return EquipmentSlot::Weapon;
    case ItemType::Armor: return EquipmentSlot::Armor;
    case ItemType::Accessory: return EquipmentSlot::Accessory;
    case ItemType::Consumable: return std::nullopt;
    }
    return std::nullopt;
}

} // namespace glory::game

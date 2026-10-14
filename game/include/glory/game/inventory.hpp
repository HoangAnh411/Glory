#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace glory::game {

enum class ItemType : std::uint8_t { Weapon, Armor, Accessory, Consumable };
enum class EquipmentSlot : std::uint8_t { Weapon, Armor, Accessory };

struct ItemDefinition {
    std::string id;
    std::string name;
    ItemType type = ItemType::Consumable;
    std::string rarity;
    int bonusAttack = 0;
    int bonusDefense = 0;
    int bonusMaxHp = 0;
    int maxStack = 1;
};

class ItemCatalog {
public:
    static ItemCatalog loadFromFile(const std::filesystem::path& path);
    [[nodiscard]] const ItemDefinition* find(const std::string& itemId) const noexcept;
    [[nodiscard]] const std::vector<ItemDefinition>& items() const noexcept { return items_; }
private:
    std::vector<ItemDefinition> items_;
};

struct InventorySlot {
    std::string itemId;
    int quantity = 0;
    [[nodiscard]] bool empty() const noexcept { return itemId.empty() || quantity <= 0; }
};

class Inventory {
public:
    static constexpr std::size_t capacity = 24;
    [[nodiscard]] bool addItem(const std::string& itemId, int quantity, const ItemCatalog& catalog);
    [[nodiscard]] bool removeItem(std::size_t slotIndex, int quantity);
    [[nodiscard]] bool moveItem(std::size_t from, std::size_t to, const ItemCatalog& catalog);
    [[nodiscard]] bool restore(const std::vector<std::pair<std::size_t, InventorySlot>>& slots,
        const ItemCatalog& catalog);
    [[nodiscard]] const std::array<InventorySlot, capacity>& slots() const noexcept { return slots_; }
private:
    std::array<InventorySlot, capacity> slots_{};
};

struct CharacterStats {
    int attack = 10;
    int defense = 5;
    int maxHp = 100;
};

struct EquipmentItem {
    EquipmentSlot slot = EquipmentSlot::Weapon;
    std::string itemId;
};

class EquipmentManager {
public:
    [[nodiscard]] bool equip(Inventory& inventory, std::size_t inventorySlot, EquipmentSlot slot,
        const ItemCatalog& catalog);
    [[nodiscard]] bool unequip(Inventory& inventory, EquipmentSlot slot, const ItemCatalog& catalog);
    [[nodiscard]] CharacterStats stats(const ItemCatalog& catalog, CharacterStats base = {}) const;
    [[nodiscard]] const std::array<std::optional<std::string>, 3>& items() const noexcept { return items_; }
    [[nodiscard]] bool restore(const std::vector<EquipmentItem>& items, const ItemCatalog& catalog);
private:
    std::array<std::optional<std::string>, 3> items_{};
};

struct LootDrop {
    std::uint64_t lootId = 0;
    glm::vec3 position{0.0F};
    std::string itemId;
    int quantity = 0;
};

class LootDropSystem {
public:
    [[nodiscard]] std::uint64_t spawnLoot(const std::string& itemId, int quantity,
        const glm::vec3& position, const ItemCatalog& catalog);
    [[nodiscard]] bool pickupLoot(std::uint64_t lootId, const glm::vec3& playerPosition,
        Inventory& inventory, const ItemCatalog& catalog, float pickupRange = 2.0F);
    [[nodiscard]] const std::vector<LootDrop>& drops() const noexcept { return drops_; }
    [[nodiscard]] bool restoreDrops(std::vector<LootDrop> drops, const ItemCatalog& catalog);
private:
    std::uint64_t nextLootId_ = 1;
    std::vector<LootDrop> drops_;
};

[[nodiscard]] const char* equipmentSlotName(EquipmentSlot slot) noexcept;
[[nodiscard]] std::optional<EquipmentSlot> equipmentSlotFor(ItemType type) noexcept;

} // namespace glory::game

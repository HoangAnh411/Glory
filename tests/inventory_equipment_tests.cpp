#include "glory/game/inventory.hpp"
#include "glory/server/account_database.hpp"
#include "glory/network/network.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#ifndef GLORY_ASSET_DIR
#define GLORY_ASSET_DIR "assets"
#endif

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

glory::game::ItemCatalog loadCatalog() {
    return glory::game::ItemCatalog::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "items.json");
}

void testInventoryOperations(const glory::game::ItemCatalog& catalog) {
    using namespace glory::game;
    Inventory inventory;
    require(inventory.addItem("healing_tonic", 25, catalog), "stacked item should fit in two slots");
    require(inventory.slots()[0].quantity == 20 && inventory.slots()[1].quantity == 5,
        "add should respect max stack size");
    require(inventory.removeItem(0, 3) && inventory.slots()[0].quantity == 17,
        "remove should decrement the chosen stack");
    require(inventory.moveItem(1, 2, catalog) && inventory.slots()[1].empty() &&
        inventory.slots()[2].quantity == 5, "move to empty slot should preserve item count");
    require(inventory.moveItem(2, 0, catalog) && inventory.slots()[0].quantity == 20 &&
        inventory.slots()[2].quantity == 2, "move should merge up to maxStack and retain remainder");
    require(!inventory.removeItem(0, 99) && !inventory.moveItem(0, 24, catalog),
        "invalid operations should not mutate inventory");

    Inventory full;
    for (int index = 0; index < 24; ++index)
        require(full.addItem("healing_tonic", 20, catalog), "fill inventory slot");
    require(!full.addItem("training_blade", 1, catalog) && full.slots()[0].quantity == 20,
        "failed add into a full bag should be atomic");
}

void testEquipmentStats(const glory::game::ItemCatalog& catalog) {
    using namespace glory::game;
    Inventory inventory;
    EquipmentManager equipment;
    require(inventory.addItem("training_blade", 1, catalog) &&
        inventory.addItem("iron_armor", 1, catalog) && inventory.addItem("sunstone_ring", 1, catalog),
        "add equipment items to bag");
    require(equipment.equip(inventory, 0, EquipmentSlot::Weapon, catalog) &&
        equipment.equip(inventory, 1, EquipmentSlot::Armor, catalog) &&
        equipment.equip(inventory, 2, EquipmentSlot::Accessory, catalog),
        "gear should equip into compatible slots");
    const auto total = equipment.stats(catalog);
    require(total.attack == 17 && total.defense == 10 && total.maxHp == 125,
        "equipment should add attack, defense and max HP bonuses");
    require(!equipment.equip(inventory, 0, EquipmentSlot::Weapon, catalog),
        "accessory must not equip into weapon slot");
    require(equipment.unequip(inventory, EquipmentSlot::Armor, catalog), "armor should unequip to inventory");
    const auto afterUnequip = equipment.stats(catalog);
    require(afterUnequip.attack == 17 && afterUnequip.defense == 6 && afterUnequip.maxHp == 110,
        "unequipping gear should remove its bonuses");
}

void testDatabasePersistence(const glory::game::ItemCatalog& catalog) {
    using glory::network::AuthResult;
    glory::server::AccountDatabase database(":memory:");
    require(database.registerAccount("inventory_user", "long-password") == AuthResult::Success,
        "account should register for persistence test");
    const auto login = database.authenticateAccount("inventory_user", "long-password");
    require(login.result == AuthResult::Success, "account should authenticate");
    const auto character = database.createCharacter(login.sessionToken, "LoadoutHero", "Vanguard");
    require(character.result == AuthResult::Success, "character should be created");

    const std::vector<glory::server::PersistedInventorySlot> inventory{
        {0, "healing_tonic", 7}, {4, "training_blade", 1}};
    const std::vector<glory::server::PersistedEquipmentItem> equipment{{2, "sunstone_ring"}};
    require(database.saveCharacterState(login.sessionToken, character.character.characterId, inventory, equipment),
        "character state should save transactionally");
    require(!database.saveCharacterState("invalid-token", character.character.characterId, {}, {}),
        "state must not save without an authenticated owner session");
    auto invalidState = inventory;
    invalidState.push_back({0, "iron_armor", 1});
    require(!database.saveCharacterState(login.sessionToken, character.character.characterId, invalidState, equipment),
        "duplicate inventory slots should be rejected");

    std::vector<glory::server::PersistedInventorySlot> loadedInventory;
    std::vector<glory::server::PersistedEquipmentItem> loadedEquipment;
    require(database.loadCharacterState(character.character.characterId, loadedInventory, loadedEquipment),
        "character state should load from SQLite");
    require(loadedInventory.size() == 2 && loadedInventory[0].slotIndex == 0 &&
        loadedInventory[0].itemId == "healing_tonic" && loadedInventory[0].quantity == 7 &&
        loadedInventory[1].slotIndex == 4 && loadedInventory[1].itemId == "training_blade",
        "inventory round-trip should preserve slots and quantities");
    require(loadedEquipment.size() == 1 && loadedEquipment[0].slotType == 2 &&
        loadedEquipment[0].itemId == "sunstone_ring", "equipment round-trip should preserve slot and item");

    glory::game::Inventory restored;
    std::vector<std::pair<std::size_t, glory::game::InventorySlot>> slots;
    for (const auto& item : loadedInventory)
        slots.push_back({static_cast<std::size_t>(item.slotIndex), {item.itemId, item.quantity}});
    require(restored.restore(slots, catalog) && restored.slots()[0].quantity == 7,
        "persisted rows should restore through game inventory validation");
    glory::game::EquipmentManager restoredEquipment;
    require(restoredEquipment.restore({{glory::game::EquipmentSlot::Accessory, "sunstone_ring"}}, catalog) &&
        restoredEquipment.stats(catalog).maxHp == 110,
        "persisted equipment should restore with item bonuses intact");
}

void testLootDropAndPickup(const glory::game::ItemCatalog& catalog) {
    using namespace glory::game;
    LootDropSystem loot;
    Inventory inventory;
    const auto lootId = loot.spawnLoot("training_blade", 1, {1.0F, 0.0F, 1.0F}, catalog);
    require(lootId != 0 && loot.drops().size() == 1, "boss death should create a loot drop");
    require(!loot.pickupLoot(lootId, {20.0F, 0.0F, 20.0F}, inventory, catalog) && loot.drops().size() == 1,
        "loot outside pickup range should remain in world");
    require(loot.pickupLoot(lootId, {1.5F, 0.0F, 1.0F}, inventory, catalog) && loot.drops().empty() &&
        inventory.slots()[0].itemId == "training_blade", "nearby player should pick loot into inventory");

    Inventory full;
    for (int index = 0; index < 24; ++index) (void)full.addItem("healing_tonic", 20, catalog);
    const auto blockedDrop = loot.spawnLoot("training_blade", 1, {0.0F, 0.0F, 0.0F}, catalog);
    require(!loot.pickupLoot(blockedDrop, {0.0F, 0.0F, 0.0F}, full, catalog) && loot.drops().size() == 1,
        "full inventory must not consume a world drop");
}

void testInventoryNetworkCodecs() {
    using namespace glory::network;
    InventorySyncPacket inventory{17, {{0, "healing_tonic", 12}, {8, "training_blade", 1}}}, decodedInventory;
    require(decodeInventorySync(encodeInventorySync(inventory), decodedInventory) &&
        decodedInventory.characterId == 17 && decodedInventory.items.size() == 2 &&
        decodedInventory.items[0].quantity == 12, "inventory sync should round-trip");

    EquipmentSyncPacket equipment{17, {{EquipmentSlotPacket::Weapon, "training_blade"}}, 5, 0, 0}, decodedEquipment;
    require(decodeEquipmentSync(encodeEquipmentSync(equipment), decodedEquipment) &&
        decodedEquipment.bonusAttack == 5 && decodedEquipment.items[0].slot == EquipmentSlotPacket::Weapon,
        "equipment sync should round-trip");

    EquipItemRequestPacket equip{"session", 17, 8, EquipmentSlotPacket::Weapon}, decodedEquip;
    require(decodeEquipItemRequest(encodeEquipItemRequest(equip), decodedEquip) &&
        decodedEquip.inventorySlot == 8 && decodedEquip.characterId == 17, "equip request should round-trip");
    UnequipItemRequestPacket unequip{"session", 17, EquipmentSlotPacket::Armor}, decodedUnequip;
    require(decodeUnequipItemRequest(encodeUnequipItemRequest(unequip), decodedUnequip) &&
        decodedUnequip.slot == EquipmentSlotPacket::Armor, "unequip request should round-trip");

    WorldLootSyncPacket worldLoot{{{5, 1.0F, 2.0F, 3.0F, "training_blade", 1}}}, decodedWorldLoot;
    require(decodeWorldLootSync(encodeWorldLootSync(worldLoot), decodedWorldLoot) &&
        decodedWorldLoot.drops.size() == 1 && decodedWorldLoot.drops[0].z == 3.0F,
        "world loot sync should round-trip");
    PickupLootRequestPacket pickup{"session", 17, 5}, decodedPickup;
    require(decodePickupLootRequest(encodePickupLootRequest(pickup), decodedPickup) &&
        decodedPickup.lootId == 5, "pickup request should round-trip");
    SimulateBossLootDropRequestPacket simulate{"session", 17}, decodedSimulate;
    require(decodeSimulateBossLootDropRequest(encodeSimulateBossLootDropRequest(simulate), decodedSimulate) &&
        decodedSimulate.characterId == 17, "debug boss loot request should round-trip");
    InventoryActionResponsePacket action{InventoryActionResult::TooFar}, decodedAction;
    require(decodeInventoryActionResponse(encodeInventoryActionResponse(action), decodedAction) &&
        decodedAction.result == action.result, "inventory action response should round-trip");
    const auto malformed = encodeInventorySync(inventory);
    require(!decodeInventorySync(std::span<const std::uint8_t>(malformed.data(), malformed.size() - 1), decodedInventory),
        "truncated inventory sync should be rejected");
}

} // namespace

int main() {
    try {
        const auto catalog = loadCatalog();
        testInventoryOperations(catalog);
        testEquipmentStats(catalog);
        testDatabasePersistence(catalog);
        testLootDropAndPickup(catalog);
        testInventoryNetworkCodecs();
        std::cout << "Inventory, equipment, persistence, loot and codec tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

#include "glory/game/dungeon.hpp"
#include "glory/network/network.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testInstanceIsolationAndLifecycle() {
    using namespace glory::game;
    DungeonManager manager;
    const auto first = manager.createInstance(1, {{101, 0, {2.0F, 0.0F, 3.0F}, 100.0F, 100.0F},
        {102, 0, {3.0F, 0.0F, 3.0F}, 100.0F, 100.0F}});
    const auto second = manager.createInstance(1, {{202, 1, {40.0F, 0.0F, 50.0F}, 100.0F, 100.0F}});
    require(first && second && *first != *second, "separate groups get distinct instance IDs");
    const auto* firstInstance = manager.find(*first);
    const auto* secondInstance = manager.find(*second);
    const float maximumBossHealth = firstInstance->definition.bossMaxHp;
    const float maximumDuration = firstInstance->definition.durationSeconds;
    require(firstInstance && secondInstance && firstInstance->bossHealth == firstInstance->definition.bossMaxHp &&
        secondInstance->bossHealth == secondInstance->definition.bossMaxHp, "each instance starts with its own boss health");
    require(firstInstance->members.size() == 2 && manager.activeInstanceForCharacter(102) == firstInstance,
        "party members share one instance membership");
    require(manager.applyBossDamage(*first, 101, 125.0F), "active instance member damages its boss");
    require(manager.find(*first)->bossHealth == maximumBossHealth - 125.0F &&
        manager.find(*second)->bossHealth == maximumBossHealth,
        "damage in one instance does not change another boss");
    manager.tick(30.0F);
    require(manager.find(*first)->timerRemaining == maximumDuration - 30.0F,
        "active instance timer counts down");
    require(manager.applyBossDamage(*first, 101, 10000.0F) &&
        manager.find(*first)->state == DungeonState::Completed, "boss defeat completes the instance");

    const auto timeout = manager.createInstance(1, {{303, 0, {}, 100.0F, 100.0F}});
    require(timeout.has_value(), "timeout test instance is created");
    manager.tick(1000.0F);
    require(manager.find(*timeout)->state == DungeonState::Failed &&
        manager.find(*timeout)->timerRemaining == 0.0F, "expired timer fails an active instance");

    const auto wipe = manager.createInstance(1, {{404, 0, {}, 100.0F, 100.0F}});
    require(wipe.has_value() && manager.updateMember(*wipe, 404, 0.0F, {}), "member vitals can be updated");
    manager.tick(0.1F);
    require(manager.find(*wipe)->state == DungeonState::Failed, "a full party wipe fails the instance");

    const auto leaving = manager.createInstance(1, {{505, 2, {8.0F, 0.0F, 9.0F}, 100.0F, 100.0F}});
    require(leaving.has_value(), "leave test instance is created");
    const auto origin = manager.removeMember(*leaving, 505);
    require(origin && origin->originPosition.x == 8.0F && manager.find(*leaving)->state == DungeonState::Closed,
        "leaving returns saved origin and closes the empty instance");
    require(manager.closeInstance(*leaving) && manager.find(*leaving) == nullptr,
        "closed instance can be safely reclaimed");
}

void testDungeonProtocol() {
    using namespace glory::network;
    const EnterDungeonRequestPacket enter{"token", 12, 1, true};
    EnterDungeonRequestPacket decodedEnter;
    require(decodeEnterDungeonRequest(encodeEnterDungeonRequest(enter), decodedEnter) &&
        decodedEnter.characterId == 12 && decodedEnter.enterParty, "entry request round-trip");
    const EnterDungeonResponsePacket response{DungeonResult::Success, 1, 9001};
    EnterDungeonResponsePacket decodedResponse;
    require(decodeEnterDungeonResponse(encodeEnterDungeonResponse(response), decodedResponse) &&
        decodedResponse.instanceId == 9001, "entry response round-trip");
    const LeaveDungeonRequestPacket leave{"token", 12};
    LeaveDungeonRequestPacket decodedLeave;
    require(decodeLeaveDungeonRequest(encodeLeaveDungeonRequest(leave), decodedLeave) &&
        decodedLeave.characterId == 12, "leave request round-trip");
    const DungeonStateSyncPacket state{DungeonResult::Success, 1, 9001, 42.5F, true, false, 0.0F, 1000.0F};
    DungeonStateSyncPacket decodedState;
    const auto bytes = encodeDungeonStateSync(state);
    require(decodeDungeonStateSync(bytes, decodedState) && decodedState.isCompleted &&
        decodedState.timerRemaining == 42.5F && decodedState.bossMaxHealth == 1000.0F,
        "dungeon state round-trip");
    require(!decodeDungeonStateSync({bytes.data(), bytes.size() - 1}, decodedState),
        "truncated dungeon state is rejected");
    auto malformed = bytes;
    malformed[0] = 0xff;
    require(!decodeDungeonStateSync(malformed, decodedState), "unknown dungeon result is rejected");
}

} // namespace

int main() {
    try {
        testInstanceIsolationAndLifecycle();
        testDungeonProtocol();
        std::cout << "Dungeon instance isolation, lifecycle, cleanup, and codec tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

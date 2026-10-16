#include "glory/engine/debug_panel.hpp"
#include "glory/engine/third_person_camera.hpp"
#include "glory/game/action_map.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testHpFraction() {
    using glory::engine::GameHudCalculations;
    require(std::abs(GameHudCalculations::calculateHpFraction(100.0F, 100.0F) - 1.0F) < 1e-4F,
        "Full HP fraction should be 1.0");
    require(std::abs(GameHudCalculations::calculateHpFraction(50.0F, 100.0F) - 0.5F) < 1e-4F,
        "Half HP fraction should be 0.5");
    require(std::abs(GameHudCalculations::calculateHpFraction(0.0F, 100.0F) - 0.0F) < 1e-4F,
        "Zero HP fraction should be 0.0");
    require(std::abs(GameHudCalculations::calculateHpFraction(-10.0F, 100.0F) - 0.0F) < 1e-4F,
        "Negative HP should clamp to 0.0");
    require(std::abs(GameHudCalculations::calculateHpFraction(150.0F, 100.0F) - 1.0F) < 1e-4F,
        "Overheated HP should clamp to 1.0");
    require(std::abs(GameHudCalculations::calculateHpFraction(50.0F, 0.0F) - 0.0F) < 1e-4F,
        "Zero max HP should safely return 0.0");
    require(std::abs(GameHudCalculations::calculateHpFraction(50.0F, -20.0F) - 0.0F) < 1e-4F,
        "Negative max HP should safely return 0.0");
}

void testCooldownFraction() {
    using glory::engine::GameHudCalculations;
    require(std::abs(GameHudCalculations::calculateCooldownFraction(0.5F, 2.0F) - 0.25F) < 1e-4F,
        "Cooldown fraction 0.5/2.0 should be 0.25");
    require(std::abs(GameHudCalculations::calculateCooldownFraction(2.0F, 2.0F) - 1.0F) < 1e-4F,
        "Cooldown fraction 2.0/2.0 should be 1.0");
    require(std::abs(GameHudCalculations::calculateCooldownFraction(0.0F, 2.0F) - 0.0F) < 1e-4F,
        "Cooldown fraction 0.0/2.0 should be 0.0");
    require(std::abs(GameHudCalculations::calculateCooldownFraction(3.0F, 2.0F) - 1.0F) < 1e-4F,
        "Exceeded cooldown should clamp to 1.0");
    require(std::abs(GameHudCalculations::calculateCooldownFraction(1.0F, 0.0F) - 0.0F) < 1e-4F,
        "Zero total cooldown should safely return 0.0");
}

void testBossBarVisibility() {
    using glory::engine::GameHudCalculations;
    // When boss is already dead
    require(!GameHudCalculations::shouldShowBossBar(true, 0.0F, 300.0F, true),
        "Defeated boss should not show boss bar");
    // Invalid max HP
    require(!GameHudCalculations::shouldShowBossBar(false, 100.0F, 0.0F, true),
        "Boss with zero max HP should not show boss bar");
    // Encounter active
    require(GameHudCalculations::shouldShowBossBar(false, 300.0F, 300.0F, true),
        "Active encounter with full HP boss should show boss bar");
    // Damaged boss even if encounter flag was not set yet
    require(GameHudCalculations::shouldShowBossBar(false, 250.0F, 300.0F, false),
        "Damaged boss should show boss bar");
    // Untouched boss and inactive encounter
    require(!GameHudCalculations::shouldShowBossBar(false, 300.0F, 300.0F, false),
        "Full HP boss outside active encounter should hide boss bar");
}

void testSkillActionBindings() {
    using glory::game::Action;
    using glory::game::ActionMap;
    using glory::input::Control;
    using glory::input::InputFrame;

    ActionMap actionMap;
    require(actionMap.binding(Action::Skill1) == Control::Key1, "Skill1 should bind to Key1 by default");
    require(actionMap.binding(Action::Skill2) == Control::Key2, "Skill2 should bind to Key2 by default");
    require(actionMap.binding(Action::Skill3) == Control::Key3, "Skill3 should bind to Key3 by default");
    require(actionMap.binding(Action::Skill4) == Control::Key4, "Skill4 should bind to Key4 by default");
    require(actionMap.binding(Action::Dodge) == Control::KeyLeftShift, "Dodge should bind to KeyLeftShift");

    InputFrame frame;
    frame.pressed.push_back(Control::Key1);
    frame.pressed.push_back(Control::KeyLeftShift);
    actionMap.update(frame);

    require(actionMap.state(Action::Skill1).pressed, "Skill1 should report pressed");
    require(!actionMap.state(Action::Skill2).pressed, "Skill2 should not report pressed");
    require(actionMap.state(Action::Dodge).pressed, "Dodge should report pressed");
}

void testTargetLockAndCombatVisuals() {
    using glory::game::Action;
    using glory::game::ActionMap;
    using glory::input::Control;
    using glory::input::InputFrame;
    using glory::engine::ThirdPersonCamera;

    ActionMap actionMap;
    require(actionMap.binding(Action::TargetLock) == Control::KeyTab, "TargetLock should bind to KeyTab");

    InputFrame frame;
    frame.pressed.push_back(Control::KeyTab);
    actionMap.update(frame);
    require(actionMap.state(Action::TargetLock).pressed, "TargetLock should report pressed");

    ThirdPersonCamera camera;
    require(!camera.isLocked(), "Camera should default to unlocked");
    camera.setLocked(true, glm::vec3(10.0F, 0.0F, 0.0F));
    require(camera.isLocked(), "Camera should report locked");
    camera.update(glm::vec3(0.0F), 0.0F, 0.0F);
    require(camera.isLocked(), "Camera should remain locked after update");
}

} // namespace

int main() {
    try {
        testHpFraction();
        testCooldownFraction();
        testBossBarVisibility();
        testSkillActionBindings();
        testTargetLockAndCombatVisuals();
        std::cout << "All Game HUD unit tests passed successfully!\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Game HUD test failure: " << error.what() << '\n';
        return 1;
    }
}

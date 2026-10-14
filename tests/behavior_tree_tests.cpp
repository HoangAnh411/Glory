#include "glory/game/behavior_tree.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    using namespace glory::game;
    try {
        AiContext context;
        auto selector = std::make_unique<Selector>();
        selector->addChild(std::make_unique<ActionNode>("Fallback", [](AiContext&) {
            return NodeStatus::Failure;
        }));
        selector->addChild(std::make_unique<ActionNode>("Selected", [](AiContext&) {
            return NodeStatus::Success;
        }));
        require(selector->tick(context) == NodeStatus::Success, "Selector should continue through failures");
        Selector failedSelector;
        failedSelector.addChild(std::make_unique<ActionNode>("Fail", [](AiContext&) {
            return NodeStatus::Failure;
        }));
        require(failedSelector.tick(context) == NodeStatus::Failure,
            "Selector should fail when all children fail");

        auto runningSelector = Selector{};
        runningSelector.addChild(std::make_unique<ActionNode>("Running", [](AiContext&) {
            return NodeStatus::Running;
        }));
        require(runningSelector.tick(context) == NodeStatus::Running,
            "Selector should propagate Running before later branches");

        unsigned int actionRuns = 0;
        Sequence sequence;
        sequence.addChild(std::make_unique<ConditionNode>("Condition", [](const AiContext& data) {
            return data.bossHp > 0.0F;
        }));
        sequence.addChild(std::make_unique<ActionNode>("Action", [&actionRuns](AiContext&) {
            ++actionRuns;
            return NodeStatus::Success;
        }));
        context.bossHp = 10.0F;
        require(sequence.tick(context) == NodeStatus::Success && actionRuns == 1 &&
            context.activeNode == "Action", "Sequence should run condition then action");
        context.bossHp = 0.0F;
        require(sequence.tick(context) == NodeStatus::Failure && actionRuns == 1,
            "Sequence should stop at the first failed condition");
        Sequence runningSequence;
        runningSequence.addChild(std::make_unique<ConditionNode>("Pass", [](const AiContext&) { return true; }));
        runningSequence.addChild(std::make_unique<ActionNode>("Wait", [](AiContext&) {
            return NodeStatus::Running;
        }));
        require(runningSequence.tick(context) == NodeStatus::Running,
            "Sequence should propagate Running and stop at that child");

        Inverter inverter(std::make_unique<ConditionNode>("False", [](const AiContext&) { return false; }));
        require(inverter.tick(context) == NodeStatus::Success, "Inverter should turn Failure into Success");
        Inverter inverseSuccess(std::make_unique<ActionNode>("Succeed", [](AiContext&) {
            return NodeStatus::Success;
        }));
        require(inverseSuccess.tick(context) == NodeStatus::Failure,
            "Inverter should turn Success into Failure");
        Inverter runningInverter(std::make_unique<ActionNode>("StillRunning", [](AiContext&) {
            return NodeStatus::Running;
        }));
        require(runningInverter.tick(context) == NodeStatus::Running,
            "Inverter should preserve Running");

        std::cout << "Behavior tree tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

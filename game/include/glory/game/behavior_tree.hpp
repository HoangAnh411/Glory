#pragma once

#include <glm/vec3.hpp>

#include <functional>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace glory::game {

enum class NodeStatus { Success, Failure, Running };

struct AiContext {
    glm::vec3 bossPosition{0.0F};
    glm::vec3 playerPosition{0.0F};
    float distanceToPlayer = 0.0F;
    float bossHp = 0.0F;
    float bossMaxHp = 0.0F;
    float deltaSeconds = 0.0F;
    float slashCooldown = 0.0F;
    float chargeCooldown = 0.0F;
    float slamCooldown = 0.0F;
    unsigned int phase = 1;
    bool chargeActive = false;
    std::string command;
    std::string activeNode;
};

class BehaviorNode {
public:
    explicit BehaviorNode(std::string name) : name_(std::move(name)) {}
    virtual ~BehaviorNode() = default;
    [[nodiscard]] virtual NodeStatus tick(AiContext& context) = 0;
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

protected:
    std::string name_;
};

class Selector final : public BehaviorNode {
public:
    explicit Selector(std::string name = "Selector") : BehaviorNode(std::move(name)) {}
    void addChild(std::unique_ptr<BehaviorNode> child) { children_.push_back(std::move(child)); }
    [[nodiscard]] NodeStatus tick(AiContext& context) override {
        for (const auto& child : children_) {
            const NodeStatus status = child->tick(context);
            if (status == NodeStatus::Running) return status;
            if (status == NodeStatus::Success) return status;
        }
        return NodeStatus::Failure;
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};

class Sequence final : public BehaviorNode {
public:
    explicit Sequence(std::string name = "Sequence") : BehaviorNode(std::move(name)) {}
    void addChild(std::unique_ptr<BehaviorNode> child) { children_.push_back(std::move(child)); }
    [[nodiscard]] NodeStatus tick(AiContext& context) override {
        for (const auto& child : children_) {
            const NodeStatus status = child->tick(context);
            if (status == NodeStatus::Running) return status;
            if (status == NodeStatus::Failure) return status;
        }
        return NodeStatus::Success;
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};

class ActionNode final : public BehaviorNode {
public:
    using Action = std::function<NodeStatus(AiContext&)>;
    ActionNode(std::string name, Action action) : BehaviorNode(std::move(name)), action_(std::move(action)) {}
    [[nodiscard]] NodeStatus tick(AiContext& context) override {
        context.activeNode = name_;
        return action_ ? action_(context) : NodeStatus::Failure;
    }

private:
    Action action_;
};

class ConditionNode final : public BehaviorNode {
public:
    using Predicate = std::function<bool(const AiContext&)>;
    ConditionNode(std::string name, Predicate predicate)
        : BehaviorNode(std::move(name)), predicate_(std::move(predicate)) {}
    [[nodiscard]] NodeStatus tick(AiContext& context) override {
        context.activeNode = name_;
        return predicate_ && predicate_(context) ? NodeStatus::Success : NodeStatus::Failure;
    }

private:
    Predicate predicate_;
};

class Inverter final : public BehaviorNode {
public:
    explicit Inverter(std::unique_ptr<BehaviorNode> child)
        : BehaviorNode("Inverter"), child_(std::move(child)) {}
    [[nodiscard]] NodeStatus tick(AiContext& context) override {
        if (!child_) return NodeStatus::Failure;
        const NodeStatus status = child_->tick(context);
        if (status == NodeStatus::Success) return NodeStatus::Failure;
        if (status == NodeStatus::Failure) return NodeStatus::Success;
        return NodeStatus::Running;
    }

private:
    std::unique_ptr<BehaviorNode> child_;
};

} // namespace glory::game

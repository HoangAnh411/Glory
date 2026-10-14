#include "glory/game/arena.hpp"

#include <algorithm>
#include <cmath>

namespace glory::game {
namespace {

std::size_t playersRequired(QueueMode mode) noexcept {
    return mode == QueueMode::Duel1v1 ? 2U : 6U;
}

bool validMode(QueueMode mode) noexcept {
    return mode == QueueMode::Duel1v1 || mode == QueueMode::Team3v3;
}

} // namespace

bool ArenaManager::request(std::uint32_t clientId, QueueMode mode, QueueAction action) {
    if (clientId == 0 || !validMode(mode)) return false;
    if (action == QueueAction::Leave) {
        auto found = queues_.find(mode);
        if (found == queues_.end()) return false;
        const auto before = found->second.size();
        std::erase(found->second, clientId);
        return found->second.size() != before;
    }
    if (action != QueueAction::Join) return false;
    if (state_ == MatchState::MatchFinished) {
        players_.clear();
        state_ = MatchState::Waiting;
        winningTeam_ = TeamId::None;
    }
    if (state_ == MatchState::Countdown || state_ == MatchState::InRound || players_.contains(clientId))
        return false;
    for (auto& [queuedMode, queue] : queues_) {
        if (queuedMode != mode) std::erase(queue, clientId);
    }
    auto& queue = queues_[mode];
    if (std::find(queue.begin(), queue.end(), clientId) != queue.end()) return false;
    queue.push_back(clientId);
    tryStartMatch(mode);
    return true;
}

void ArenaManager::tryStartMatch(QueueMode mode) {
    if (state_ != MatchState::Waiting) return;
    auto& queue = queues_[mode];
    const std::size_t required = playersRequired(mode);
    if (queue.size() < required) return;
    mode_ = mode;
    winningTeam_ = TeamId::None;
    countdownRemaining_ = 3.0;
    for (std::size_t index = 0; index < required; ++index) {
        const std::uint32_t clientId = queue.front();
        queue.pop_front();
        const TeamId team = index < required / 2 ? TeamId::Red : TeamId::Blue;
        const float side = team == TeamId::Red ? -5.0F : 5.0F;
        const float lane = static_cast<float>(index % (required / 2)) -
            static_cast<float>(required / 2 - 1) * 0.5F;
        players_.emplace(clientId, ArenaPlayer{clientId, team, {side, 0.0F, lane * 2.0F},
            team == TeamId::Red ? 90.0F : -90.0F, 100.0F});
    }
    state_ = MatchState::Countdown;
}

void ArenaManager::disconnect(std::uint32_t clientId) noexcept {
    for (auto& [mode, queue] : queues_) {
        (void)mode;
        std::erase(queue, clientId);
    }
    players_.erase(clientId);
    if (state_ == MatchState::Countdown) {
        auto& queue = queues_[mode_];
        for (auto it = players_.rbegin(); it != players_.rend(); ++it)
            queue.push_front(it->first);
        players_.clear();
        state_ = MatchState::Waiting;
        countdownRemaining_ = 0.0;
    } else if (state_ == MatchState::InRound) {
        evaluateWinner();
    } else if (state_ == MatchState::MatchFinished && players_.empty()) {
        state_ = MatchState::Waiting;
        winningTeam_ = TeamId::None;
    }
}

void ArenaManager::tick(double deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0) return;
    if (state_ == MatchState::Countdown) {
        countdownRemaining_ = std::max(0.0, countdownRemaining_ - deltaSeconds);
        if (countdownRemaining_ <= 1.0e-9) {
            countdownRemaining_ = 0.0;
            state_ = MatchState::InRound;
        }
    } else if (state_ == MatchState::InRound) {
        evaluateWinner();
    }
}

void ArenaManager::setPlayerHealth(std::uint32_t clientId, float hp) {
    const auto found = players_.find(clientId);
    if (found == players_.end() || !std::isfinite(hp)) return;
    found->second.hp = std::clamp(hp, 0.0F, 100.0F);
    if (state_ == MatchState::InRound) evaluateWinner();
}

void ArenaManager::evaluateWinner() {
    bool redAlive = false;
    bool blueAlive = false;
    for (const auto& [clientId, playerState] : players_) {
        (void)clientId;
        if (playerState.hp <= 0.0F) continue;
        redAlive |= playerState.team == TeamId::Red;
        blueAlive |= playerState.team == TeamId::Blue;
    }
    if (!redAlive || !blueAlive) {
        winningTeam_ = redAlive ? TeamId::Red : (blueAlive ? TeamId::Blue : TeamId::None);
        state_ = MatchState::MatchFinished;
        countdownRemaining_ = 0.0;
    }
}

std::optional<ArenaPlayer> ArenaManager::player(std::uint32_t clientId) const {
    const auto found = players_.find(clientId);
    if (found == players_.end()) return std::nullopt;
    return found->second;
}

std::size_t ArenaManager::queuedCount(QueueMode mode) const noexcept {
    const auto found = queues_.find(mode);
    return found == queues_.end() ? 0 : found->second.size();
}

bool ArenaManager::canDamage(std::uint32_t attackerId, std::uint32_t targetId) const noexcept {
    if (state_ != MatchState::InRound) return true;
    const auto attacker = players_.find(attackerId);
    const auto target = players_.find(targetId);
    return attacker != players_.end() && target != players_.end() &&
        attacker->second.team != target->second.team;
}

} // namespace glory::game

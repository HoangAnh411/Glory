#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <vector>

namespace glory::game {

enum class QueueMode : std::uint8_t { Duel1v1 = 1, Team3v3 = 2 };
enum class MatchState : std::uint8_t { Waiting = 0, Countdown = 1, InRound = 2, MatchFinished = 3 };
enum class TeamId : std::uint8_t { None = 0, Red = 1, Blue = 2 };
enum class QueueAction : std::uint8_t { Join = 1, Leave = 2 };

struct ArenaPlayer {
    std::uint32_t clientId = 0;
    TeamId team = TeamId::None;
    glm::vec3 spawnPosition{0.0F};
    float spawnYaw = 0.0F;
    float hp = 100.0F;
};

class ArenaManager {
public:
    bool request(std::uint32_t clientId, QueueMode mode, QueueAction action);
    void disconnect(std::uint32_t clientId) noexcept;
    void tick(double deltaSeconds);
    void setPlayerHealth(std::uint32_t clientId, float hp);

    [[nodiscard]] MatchState state() const noexcept { return state_; }
    [[nodiscard]] QueueMode mode() const noexcept { return mode_; }
    [[nodiscard]] TeamId winningTeam() const noexcept { return winningTeam_; }
    [[nodiscard]] double countdownRemaining() const noexcept { return countdownRemaining_; }
    [[nodiscard]] bool locksPlayers() const noexcept {
        return state_ == MatchState::Countdown || state_ == MatchState::MatchFinished;
    }
    [[nodiscard]] bool inRound() const noexcept { return state_ == MatchState::InRound; }
    [[nodiscard]] bool canDamage(std::uint32_t attackerId, std::uint32_t targetId) const noexcept;
    [[nodiscard]] std::optional<ArenaPlayer> player(std::uint32_t clientId) const;
    [[nodiscard]] const std::map<std::uint32_t, ArenaPlayer>& players() const noexcept { return players_; }
    [[nodiscard]] std::size_t queuedCount(QueueMode mode) const noexcept;

private:
    void tryStartMatch(QueueMode mode);
    void evaluateWinner();

    std::map<QueueMode, std::deque<std::uint32_t>> queues_;
    std::map<std::uint32_t, ArenaPlayer> players_;
    MatchState state_ = MatchState::Waiting;
    QueueMode mode_ = QueueMode::Duel1v1;
    TeamId winningTeam_ = TeamId::None;
    double countdownRemaining_ = 0.0;
};

} // namespace glory::game

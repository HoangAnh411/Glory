#pragma once

#include <filesystem>
#include <memory>

namespace glory::engine {

class AudioSystem {
public:
    explicit AudioSystem(std::filesystem::path assetDirectory);
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void playSwing() noexcept;
    void playHit() noexcept;
    void setMasterVolume(float volume) noexcept;
    [[nodiscard]] float masterVolume() const noexcept;
    void setMuted(bool muted) noexcept;
    [[nodiscard]] bool muted() const noexcept;
    [[nodiscard]] bool available() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace glory::engine

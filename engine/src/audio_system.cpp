#include "glory/engine/audio_system.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "miniaudio.h"

namespace glory::engine {

struct AudioSystem::Impl {
    ma_engine engine{};
    std::filesystem::path swingPath;
    std::filesystem::path hitPath;
    float masterVolume = 0.7F;
    bool isMuted = false;
    bool isAvailable = false;

    explicit Impl(const std::filesystem::path& assetDirectory)
        : swingPath(assetDirectory / "swing.wav"), hitPath(assetDirectory / "hit.wav") {
        isAvailable = ma_engine_init(nullptr, &engine) == MA_SUCCESS;
        if (isAvailable) ma_engine_set_volume(&engine, masterVolume);
    }

    ~Impl() {
        if (isAvailable) ma_engine_uninit(&engine);
    }

    void play(const std::filesystem::path& path) noexcept {
        if (!isAvailable || isMuted) return;
        try {
            const std::string filename = path.string();
            (void)ma_engine_play_sound(&engine, filename.c_str(), nullptr);
        } catch (...) {
        }
    }
};

AudioSystem::AudioSystem(std::filesystem::path assetDirectory)
    : impl_(std::make_unique<Impl>(assetDirectory)) {}

AudioSystem::~AudioSystem() = default;

void AudioSystem::playSwing() noexcept { impl_->play(impl_->swingPath); }
void AudioSystem::playHit() noexcept { impl_->play(impl_->hitPath); }

void AudioSystem::setMasterVolume(float volume) noexcept {
    if (!std::isfinite(volume)) return;
    impl_->masterVolume = std::clamp(volume, 0.0F, 1.0F);
    if (impl_->isAvailable) ma_engine_set_volume(&impl_->engine, impl_->isMuted ? 0.0F : impl_->masterVolume);
}

float AudioSystem::masterVolume() const noexcept { return impl_->masterVolume; }

void AudioSystem::setMuted(bool muted) noexcept {
    impl_->isMuted = muted;
    if (impl_->isAvailable) ma_engine_set_volume(&impl_->engine, muted ? 0.0F : impl_->masterVolume);
}

bool AudioSystem::muted() const noexcept { return impl_->isMuted; }
bool AudioSystem::available() const noexcept { return impl_->isAvailable; }

} // namespace glory::engine

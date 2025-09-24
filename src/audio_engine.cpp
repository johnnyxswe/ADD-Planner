#include "audio_engine.h"
#include <iostream>
#include <algorithm>

#include "spdlog/spdlog.h"

AudioEngine::AudioEngine() : masterVolume_(100.0f), muted_(false) {
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    // SFML automatically initializes audio subsystem when first used
    // You can add any additional initialization here
    return true;
}

void AudioEngine::shutdown() {
    stopAll();
    soundBuffers_.clear();
    activeSounds_.clear();
    currentMusic_.stop();
}

bool AudioEngine::loadSound(const std::string& name, const std::string& filepath) {
    // Check if already loaded
    if (soundBuffers_.find(name) != soundBuffers_.end()) {
       spdlog::info("Sound {} already loaded", name);
        return true;
    }
    
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filepath)) {
        spdlog::error("Failed to load sound: ", filepath);
        return false;
    }
    
    soundBuffers_[name] = std::move(buffer);

    return true;
}

void AudioEngine::unloadSound(const std::string& name) {
    auto it = soundBuffers_.find(name);
    if (it != soundBuffers_.end()) {
        // Stop any sounds using this buffer
        for (auto& sound : activeSounds_) {
            if (sound.getBuffer() == &it->second) {
                sound.stop();
            }
        }
        
        soundBuffers_.erase(it);
    }
}

void AudioEngine::preloadSounds(const std::vector<std::pair<std::string, std::string>>& sounds) {
    std::cout << "Preloading " << sounds.size() << " sounds..." << std::endl;
    
    int loaded = 0;
    for (const auto& [name, filepath] : sounds) {
        if (loadSound(name, filepath)) {
            loaded++;
        }
    }
}

void AudioEngine::playSound(const std::string& name, float volume, bool loop) {
    // Find the sound buffer
    auto it = soundBuffers_.find(name);
    if (it == soundBuffers_.end()) {
        spdlog::error("Sound {} not found. Did you load it?", name);
        return;
    }
    
    // Clean up finished sounds before adding new one
    update();
    
    // Create new sound instance
    sf::Sound sound;
    sound.setBuffer(it->second);
    sound.setVolume(muted_ ? 0.0f : (volume * masterVolume_ / 100.0f));
    sound.setLoop(loop);

    // DEBUG: Print sound info
    sound.play();
    
    // Store in active sounds
    activeSounds_.push_back(std::move(sound));
}

void AudioEngine::playMusic(const std::string& filepath, bool loop) {
    currentMusic_.stop();
    
    if (!currentMusic_.openFromFile(filepath)) {
        spdlog::error("Failed to load music: {}", filepath);
        return;
    }
    
    currentMusic_.setLoop(loop);
    currentMusic_.setVolume(muted_ ? 0.0f : masterVolume_);
    currentMusic_.play();
}

void AudioEngine::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 100.0f);
    
    // Update all active sounds
    for (auto& sound : activeSounds_) {
        if (!muted_) {
            sound.setVolume(masterVolume_);
        }
    }
    
    // Update music
    if (!muted_) {
        currentMusic_.setVolume(masterVolume_);
    }
}

void AudioEngine::setMuted(bool isMuted) {
    muted_ = isMuted;
    
    float effectiveVolume = muted_ ? 0.0f : masterVolume_;
    
    // Update all active sounds
    for (auto& sound : activeSounds_) {
        sound.setVolume(effectiveVolume);
    }
    
    // Update music
    currentMusic_.setVolume(effectiveVolume);
}

void AudioEngine::pauseAll() {
    for (auto& sound : activeSounds_) {
        if (sound.getStatus() == sf::Sound::Playing) {
            sound.pause();
        }
    }
    
    if (currentMusic_.getStatus() == sf::Music::Playing) {
        currentMusic_.pause();
    }
    
    std::cout << "All audio paused" << std::endl;
}

void AudioEngine::resumeAll() {
    for (auto& sound : activeSounds_) {
        if (sound.getStatus() == sf::Sound::Paused) {
            sound.play();
        }
    }
    
    if (currentMusic_.getStatus() == sf::Music::Paused) {
        currentMusic_.play();
    }
    
    std::cout << "All audio resumed" << std::endl;
}

void AudioEngine::stopAll() {
    for (auto& sound : activeSounds_) {
        sound.stop();
    }
    activeSounds_.clear();
    
    currentMusic_.stop();
}

void AudioEngine::stopMusic() {
    currentMusic_.stop();
}

void AudioEngine::update() {
    // Remove finished sounds to prevent memory buildup
    activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
            [](const sf::Sound& sound) {
                return sound.getStatus() == sf::Sound::Stopped;
            }),
        activeSounds_.end());
}

bool AudioEngine::isMusicPlaying() const {
    return currentMusic_.getStatus() == sf::Music::Playing;
}

float AudioEngine::getMasterVolume() const {
    return masterVolume_;
}

bool AudioEngine::isMuted() const {
    return muted_;
}

size_t AudioEngine::getActiveSoundCount() const {
    return activeSounds_.size();
}

size_t AudioEngine::getLoadedSoundCount() const {
    return soundBuffers_.size();
}

void AudioEngine::printStatus() const {
    if (!soundBuffers_.empty()) {
        std::cout << "Loaded sound names: ";
        for (const auto& [name, buffer] : soundBuffers_) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

void AudioEngine::soundCheck()
{
    // Test system audio with a simple SFML sound
    sf::SoundBuffer testBuffer;
    sf::Int16 samples[44100];
    for (int i = 0; i < 44100; ++i) {
        samples[i] = 5000 * sin(2 * 3.14159 * 440 * i / 44100); // 440Hz beep
    }
    testBuffer.loadFromSamples(samples, 44100, 1, 44100);

    sf::Sound testSound;
    testSound.setBuffer(testBuffer);
    testSound.setVolume(50);
    testSound.play();

    spdlog::info("Playing test beep...");
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

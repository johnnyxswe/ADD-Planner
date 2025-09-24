#pragma once

#include <SFML/Audio.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

class AudioEngine {
private:
    std::unordered_map<std::string, sf::SoundBuffer> soundBuffers_;
    std::vector<sf::Sound> activeSounds_;
    sf::Music currentMusic_;
    float masterVolume_;
    bool muted_;

public:
    AudioEngine();
    ~AudioEngine();

    // Core functionality
    bool initialize();
    void shutdown();

    // Resource management
    bool loadSound(const std::string& name, const std::string& filepath);
    void unloadSound(const std::string& name);
    void preloadSounds(const std::vector<std::pair<std::string, std::string>>& sounds);

    // Playback
    void playSound(const std::string& name, float volume = 100.0f, bool loop = false);
    void playMusic(const std::string& filepath, bool loop = true);

    // Global controls
    void setMasterVolume(float volume);
    void setMuted(bool muted);
    void pauseAll();
    void resumeAll();
    void stopAll();
    void stopMusic();

    // Maintenance
    void update(); // Call this regularly to clean up finished sounds

    // Status queries
    bool isMusicPlaying() const;
    float getMasterVolume() const;
    bool isMuted() const;
    size_t getActiveSoundCount() const;
    size_t getLoadedSoundCount() const;
    void printStatus() const; // Debug helper

    void soundCheck();
};
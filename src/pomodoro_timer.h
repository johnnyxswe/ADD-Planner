#include "imgui.h"
#include <chrono>
#include <string>
#include "audio_engine.h"
#include "utilities.h"

class PomodoroTimer
{
public:
    enum class TimerState
    {
        STOPPED,
        RUNNING,
        PAUSED,
        COMPLETE
    };

    enum class TimerType
    {
        WORK,
        SHORT_BREAK,
        LONG_BREAK
    };

private:
    TimerState state = TimerState::STOPPED;
    TimerType type = TimerType::WORK;

    // Default durations in seconds
    int workDuration = 25 * 60; // 25 minutes
    int shortBreakDuration = 5 * 60; // 5 minutes
    int longBreakDuration = 15 * 60; // 15 minutes

    int currentDuration = workDuration;
    int remainingTime = workDuration;

    std::chrono::steady_clock::time_point lastUpdateTime;

    int completedPomodoros = 0;
    bool timerFinished = false;
    AudioEngine *audio_ = nullptr;

public:
    void Update()
    {
        if (state == TimerState::RUNNING)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime).count();

            if (elapsed >= 1)
            {
                remainingTime -= static_cast<int>(elapsed);
                lastUpdateTime = now;

                if (remainingTime <= 0)
                {
                    remainingTime = 0;
                    state = TimerState::COMPLETE;
                    timerFinished = true;

                    audio_->playMusic(getResourcesPath() + "/sounds/ringtone_fixed.wav");

                    if (type == TimerType::WORK)
                    {
                        completedPomodoros++;
                    }
                }
            }
        }
    }

    void Start()
    {
        if (state == TimerState::STOPPED)
        {
            remainingTime = currentDuration;
            timerFinished = false;
        }
        state = TimerState::RUNNING;
        lastUpdateTime = std::chrono::steady_clock::now();
    }

    void Pause()
    {
        if (state == TimerState::RUNNING)
        {
            state = TimerState::PAUSED;
        } else if (state == TimerState::PAUSED)
        {
            state = TimerState::RUNNING;
            lastUpdateTime = std::chrono::steady_clock::now();
        }
    }

    void Stop()
    {
        state = TimerState::STOPPED;
        remainingTime = currentDuration;
        timerFinished = false;
        audio_->stopMusic();
    }

    void SetTimerType(TimerType newType)
    {
        if (state == TimerState::STOPPED)
        {
            type = newType;
            switch (type)
            {
                case TimerType::WORK:
                    currentDuration = workDuration;
                    break;
                case TimerType::SHORT_BREAK:
                    currentDuration = shortBreakDuration;
                    break;
                case TimerType::LONG_BREAK:
                    currentDuration = longBreakDuration;
                    break;
            }
            remainingTime = currentDuration;
        }
    }

    std::string FormatTime(int seconds)
    {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, secs);
        return std::string(buffer);
    }

    const char *GetTimerTypeName()
    {
        switch (type)
        {
            case TimerType::WORK: return "Work";
            case TimerType::SHORT_BREAK: return "Short Break";
            case TimerType::LONG_BREAK: return "Long Break";
            default: return "Unknown";
        }
    }

    void DrawWidget()
    {
        // Timer type selection
        if (state == TimerState::STOPPED)
        {
            const char *items[] = {"Work", "Short Break", "Long Break"};
            int currentItem = static_cast<int>(type);

            if (ImGui::Combo("Timer Type", &currentItem, items, IM_ARRAYSIZE(items)))
            {
                SetTimerType(static_cast<TimerType>(currentItem));
            }
        } else
        {
            ImGui::Text("Timer Type: %s", GetTimerTypeName());
        }

        ImGui::Separator();

        // Time display
        ImGui::PushFont(nullptr); // Use default font, but you can push a larger font here

        // Center the time display within available width
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float textWidth = ImGui::CalcTextSize(FormatTime(remainingTime).c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - textWidth) * 0.5f);

        // Color the time based on urgency
        if (remainingTime <= 60 && state == TimerState::RUNNING)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255)); // Red for last minute
        } else if (remainingTime <= 300 && state == TimerState::RUNNING)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 100, 255)); // Orange for last 5 minutes
        }

        ImGui::Text("%s", FormatTime(remainingTime).c_str());

        if (remainingTime <= 300 && state == TimerState::RUNNING)
        {
            ImGui::PopStyleColor();
        }
        ImGui::PopFont();

        // Progress bar
        float progress = 1.0f - (float) remainingTime / (float) currentDuration;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

        ImGui::Separator();

        // Control buttons
        if (state == TimerState::STOPPED)
        {
            if (ImGui::Button("Start", ImVec2(80, 30)))
            {
                Start();
            }
        } else if (state == TimerState::RUNNING)
        {
            if (ImGui::Button("Pause", ImVec2(80, 30)))
            {
                Pause();
            }
        } else if (state == TimerState::PAUSED)
        {
            if (ImGui::Button("Resume", ImVec2(80, 30)))
            {
                Pause(); // Pause() toggles between paused and running
            }
        }

        ImGui::SameLine();

        if (state != TimerState::STOPPED)
        {
            if (ImGui::Button("Stop", ImVec2(80, 30)))
            {
                Stop();
            }
        }

        // Timer finished notification
        if (timerFinished)
        {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, 255));
            ImGui::Text("Timer Finished!");
            ImGui::PopStyleColor();

            if (type == TimerType::WORK)
            {
                ImGui::Text("Great job! Time for a break.");
            } else
            {
                ImGui::Text("Break's over! Ready to work?");
            }
        }

        // Statistics
        ImGui::Separator();
        ImGui::Text("Completed Pomodoros: %d", completedPomodoros);

        // Optional: Reset statistics button
        if (ImGui::Button("Reset Stats"))
        {
            completedPomodoros = 0;
        }
    }

    // Getters for external use
    bool IsFinished() const { return timerFinished; }
    TimerState GetState() const { return state; }
    TimerType GetType() const { return type; }
    int GetCompletedPomodoros() const { return completedPomodoros; }
    void initialize(AudioEngine *audio)
    {
        audio_ = audio;
    }
};

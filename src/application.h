//
// Created by Johnny Gonzales on 8/19/25.
//

#pragma once
#include <optional>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "audio_engine.h"
#include "card_database.h"
#include "glm/vec2.hpp"
#include "graphics.h"
#include "imgui_renderer.h"
#include "spdlog/spdlog.h"
#include "utilities.h"  // Add this include


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;


namespace todo {
    struct TodoCard;

    class Application
    {
    public:
        Application()
            : db_(getDatabasePath()) // <--- initialize here, or in constructor body
        {
        }

        void run();
        void initWindow();
        void mainLoop();
        void shutdown();

        void reloadAppState();
        void loadCards();
        void loadProjects();

        // Getters for other classes to access what they need
        [[nodiscard]] GLFWwindow *getWindow() const { return window_; }
        [[nodiscard]] Graphics *getGraphics() const { return graphics_.get(); }

        CardDatabase &db() { return db_; }
        AudioEngine &audio() { return audio_; }

        bool framebufferResized_ = true;

        // Getters
        void getWindowSize(glm::ivec2 &size) const;
        [[nodiscard]] const std::vector<TodoCard> &getCards() const { return cards_; }
        [[nodiscard]] const std::vector<proj::Project> &getProjects() const { return projects_; }

    private:
        CardDatabase db_;
        AudioEngine audio_;

        GLFWwindow *window_ = VK_NULL_HANDLE;

        std::vector<TodoCard> cards_{};
        std::vector<proj::Project> projects_{};

        proj::Project defaultProject_;

        std::unique_ptr<Graphics> graphics_;
        std::unique_ptr<ImGuiRenderer> imguiRenderer_;

        // Application-specific data
        std::vector<TodoCard> todoCards_{};
        std::vector<TodoCard> inProgressCards_{};
        std::vector<TodoCard> doneCards_{};
        std::vector<TodoCard> archivedCards_{};




        void renderUI();
    };
}

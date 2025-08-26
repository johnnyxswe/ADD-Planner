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

#include "card_database.h"
#include "imgui.h"
#include "glm/vec2.hpp"
#include "graphics.h"
#include "imgui_renderer.h"
#include "spdlog/spdlog.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;


namespace todo {
    struct TodoCard;

    class Application
    {
    public:
        Application()
            : db_("cards.db") // <--- initialize here, or in constructor body
        {
        }

        void run();
        void initWindow();
        void mainLoop();
        void shutdown();

        // Getters for other classes to access what they need
        [[nodiscard]] GLFWwindow *getWindow() const { return window_; }
        [[nodiscard]] Graphics *getGraphics() const { return graphics_.get(); }

        CardDatabase& db() { return db_; }

        bool framebufferResized_ = true;

        // Getters
        void getWindowSize(glm::ivec2 &size) const;
        [[nodiscard]] std::vector<TodoCard> &getTodoCards() { return todoCards_; }
        [[nodiscard]] std::vector<TodoCard> &getInProgressCards() { return inProgressCards_; }
        [[nodiscard]] std::vector<TodoCard> &getDoneCards() { return doneCards_; }
        [[nodiscard]] std::vector<TodoCard> &getArchivedCards() { return archivedCards_; }

    private:
        CardDatabase db_;
        GLFWwindow *window_ = VK_NULL_HANDLE;

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

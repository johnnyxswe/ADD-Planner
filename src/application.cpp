//
// Created by Johnny Gonzales on 8/19/25.
//

#include "application.h"
#include "card_database.h"
#include <set>
#include "spdlog/spdlog.h"
#include <fstream>
#include "graphics.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_renderer.h"
#include "todo_card.h"

static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    const auto app = static_cast<todo::Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
}


namespace todo {
    void Application::run()
    {
        initWindow();

        // Initialize database
        auto cards = db_.getAllCards();
        for (const auto &c: cards)
        {
            printf("id=%d title=%s desc=%s status=%d completed=%d\n", c.id, c.title.c_str(), c.description.c_str(),
                   c.status, c.completed);
            switch (c.status)
            {
                case CardStatus::Todo: todoCards_.emplace_back(c);
                    break;
                case CardStatus::InProgress: inProgressCards_.emplace_back(c);
                    break;
                case CardStatus::Done: doneCards_.emplace_back(c);
                    break;
                default: todoCards_.emplace_back(c);
                    break;
            }
        }


        // Initialize graphics system
        graphics_ = std::make_unique<Graphics>(window_);
        graphics_->initialize();

        // Initialize ImGui renderer after graphics
        imguiRenderer_ = std::make_unique<ImGuiRenderer>(graphics_.get(), this);
        imguiRenderer_->initialize();

        mainLoop();

        shutdown();
    }

    void Application::initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Enable transparent framebuffer
        // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Remove window decorations (border, title bar, etc.)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Make window non-resizable (optional)
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); // Keep window always on top (optional)


        window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    }


    void Application::mainLoop()
    {
        while (!glfwWindowShouldClose(window_))
        {
            glfwPollEvents();

            // Begin graphics frame
            graphics_->beginFrame();

            // Begin ImGui frame
            imguiRenderer_->beginFrame();

            // Your ImGui UI code here
            renderUI();

            // End ImGui frame
            imguiRenderer_->endFrame();

            // Render ImGui to command buffer
            VkCommandBuffer cmdBuffer = graphics_->getCurrentCommandBuffer();
            imguiRenderer_->render(cmdBuffer);

            // End graphics frame and present
            graphics_->endFrame();
        }
        vkDeviceWaitIdle(graphics_->getDevice());
    }


    void Application::shutdown()
    {
        if (graphics_)
        {
            if (graphics_->getDevice())
                vkDeviceWaitIdle(graphics_->getDevice());
        }

        // Cleanup in reverse order
        imguiRenderer_.reset(); // This will call ImGuiRenderer destructor
        graphics_.reset(); // This will call Graphics destructor

        if (window_)
        {
            glfwDestroyWindow(window_);
            glfwTerminate();
        }

        glfwDestroyWindow(window_);

        glfwTerminate();
    }

    void Application::getWindowSize(glm::ivec2 &size) const
    {
        glfwGetWindowSize(window_, &size.x, &size.y);
    }

    void Application::renderUI()
    {
        imguiRenderer_->renderUI();
    }
}

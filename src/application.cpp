//
// Created by Johnny Gonzales on 8/19/25.
//

#include "application.h"
#include "card_database.h"
#include <set>
#include "graphics.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_renderer.h"


#include "audio_engine.h"

#ifdef __APPLE__
#include <GLFW/glfw3.h>
#include <objc/objc.h>
#include <objc/message.h>
#endif

static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    const auto app = static_cast<todo::Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
}


namespace todo {
    void Application::reloadAppState()
    {
        loadCards();
        loadProjects();
    }
    void Application::loadCards()
    {
        cards_.clear();
        cards_ = db_.getAllCards();
    }

    void Application::loadProjects()
    {
        projects_.clear();
        projects_ = db_.getAllProjects();
        projects_.push_back(defaultProject_);
    }

    void Application::run()
    {
        initWindow();

        // Initialize database
        loadCards();
        loadProjects();

        // Initialize graphics system
        graphics_ = std::make_unique<Graphics>(window_);
        graphics_->initialize();

        // Initialize ImGui renderer after graphics
        imguiRenderer_ = std::make_unique<ImGuiRenderer>(graphics_.get(), this);
        imguiRenderer_->initialize();

        // Initialize sound system
        if (!audio_.initialize())
        {
            spdlog::error("Failed to initialize audio engine!");
            throw std::runtime_error("Failed to initialize audio engine!");
        }

        if (!audio_.loadSound("timer_finished", getResourcesPath() + "sounds/ringtone_fixed.wav"))
        {
            spdlog::error("Failed to load sound!");
        }

        mainLoop();

        shutdown();
    }

    void Application::initWindow()
    {
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Enable transparent framebuffer
        // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Remove window decorations (border, title bar, etc.)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Make window non-resizable (optional)
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); // Keep window always on top (optional)


        window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
#ifdef __APPLE__
        id app = ((id (*)(Class, SEL))objc_msgSend)(
            (Class)objc_getClass("NSApplication"),
            sel_registerName("sharedApplication")
        );

        // Correct cast: method takes (id, SEL, BOOL)
        ((void (*)(id, SEL, BOOL))objc_msgSend)(
            app,
            sel_registerName("activateIgnoringOtherApps:"),
            YES
        );
#endif

        // // Or handle DPI scaling properly:
        // float xscale, yscale;
        // glfwGetWindowContentScale(window_, &xscale, &yscale);

        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    }


    void Application::mainLoop()
    {
        while (!glfwWindowShouldClose(window_))
        {
            glfwPollEvents();

            // Update audio engine to maintain active sounds
            audio_.update();

            if (framebufferResized_) {
                framebufferResized_ = false;
                graphics_->recreateSwapChain();
            }

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

        audio_.shutdown();
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

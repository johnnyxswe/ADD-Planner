//
// Created by Johnny Gonzales on 8/23/25.
//

#pragma once
#include <optional>
#include <vulkan/vulkan_core.h>
#include <vector>

#include "GLFW/glfw3.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif
namespace todo {
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class Graphics
    {
    public:
        explicit Graphics(GLFWwindow* window) : window_(window) {}
        ~Graphics();

        void initialize();
        void shutdown();

        // Core rendering functions
        void beginFrame();
        void endFrame();

        // Getters for other systems
        [[nodiscard]] VkDevice getDevice() const;
        [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const;
        [[nodiscard]] VkInstance getInstance() const;
        [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const;
        [[nodiscard]] std::uint32_t getGraphicsQueueFamilyIndex() const;
        [[nodiscard]] std::uint32_t getPresentQueueFamilyIndex() const;
        [[nodiscard]] VkSurfaceKHR getSurface() const;
        [[nodiscard]] VkSwapchainKHR getSwapChain() const;
        [[nodiscard]] std::vector<VkImageView> getSwapChainImageViews() const;
        [[nodiscard]] VkFormat getSwapChainImageFormat() const;
        [[nodiscard]] std::vector<VkSemaphore> getImageAvailableSemaphores() const;
        [[nodiscard]] std::vector<VkSemaphore> getRenderFinishedSemaphores() const;
        [[nodiscard]] std::vector<VkFence> getInFlightFences() const;
        [[nodiscard]] std::vector<VkCommandBuffer> getCommandBuffers() const;
        [[nodiscard]] std::uint32_t getCurrentFrame() const;
        [[nodiscard]] VkRenderPass getRenderPass() const;
        [[nodiscard]] VkPipeline getGraphicsPipeline() const;
        [[nodiscard]] VkPipelineLayout getPipelineLayout() const;
        [[nodiscard]] VkExtent2D getSwapChainExtent() const;
        [[nodiscard]] std::vector<VkFramebuffer> getSwapChainFramebuffers() const;
        [[nodiscard]] std::vector<VkImage> getSwapChainImages() const;
        [[nodiscard]] VkCommandPool getCommandPool() const;
        [[nodiscard]] VkQueue getGraphicsQueue() const;
        [[nodiscard]] VkQueue getPresentQueue() const;


        void recreateSwapChain();
        bool framebufferResized_ = false;

    private:
        GLFWwindow* window_ = nullptr;
        uint32_t currentFrame_ = 0;
        uint32_t imageIndex_ = -1;

        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        VkQueue presentQueue_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        std::vector<const char *> deviceExtensions_{};
        VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages_{};
        VkFormat swapChainImageFormat_{};
        VkExtent2D swapChainExtent_{};
        std::vector<VkImageView> swapChainImageViews_{};
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
        VkRenderPass renderPass_ = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> swapChainFramebuffers_;
        VkCommandPool commandPool_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

        std::vector<VkCommandBuffer> commandBuffers_{};

        std::vector<VkSemaphore> imageAvailableSemaphores_{};
        std::vector<VkSemaphore> renderFinishedSemaphores_{};
        std::vector<VkFence> inFlightFences_{};

        void setupDebugMessenger();
        void createInstance();
        static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        std::vector<const char *> getRequiredExtensions();
        bool checkValidationLayerSupport();
        void pickPhysicalDevice();
        void createLogicalDevice();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void createSurface();
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        void createSwapChain();
        void createImageViews();
        void createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char> &code);
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffer();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void drawFrame();
        void createSyncObjects();
        void cleanupSwapChain();
        VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
        void EndSingleTimeCommands(
            VkDevice device,
            VkQueue graphicsQueue,
            VkCommandPool commandPool,
            VkCommandBuffer commandBuffer
        );

    };
} // todo
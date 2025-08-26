//
// Created by Johnny Gonzales on 8/23/25.
//

#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "todo_card.h""


#include "imgui.h"


namespace todo {
    enum class CardStatus;
    class Graphics;
    class Application;

    struct DragDropPayload
    {
        int sourceIndex;
        int sourceColumnType; // Use an int/enum representing which column, not string!
    };

    enum CardColumnType
    {
        TodoColumn = 0,
        InProgressColumn = 1,
        DoneColumn = 2
    };


    class ImGuiRenderer
    {
    public:
        ImGuiRenderer(Graphics *graphics, Application *app) : graphics_(graphics), app_(app)
        {
        };

        ~ImGuiRenderer();

        void initialize();
        void renderUI();
        void shutdown();

        void beginFrame();
        void endFrame();
        void render(VkCommandBuffer commandBuffer);

    private:
        Graphics *graphics_ = nullptr;
        Application *app_ = nullptr;

        VkDescriptorPool descriptorPool_ = nullptr;

        float columnWidth_ = 0;
        float columnHeight_ = 0;

        const std::string todoDropZoneId_ = "TodoDropZone";
        const std::string inProgressDropZoneId_ = "InProgressDropZone";
        const std::string doneDropZoneId_ = "DoneDropZone";

        const float columnCount_ = 3.0;

        void createDescriptorPool();

        bool deleteCard(TodoCard &card);
        void drawCardColumn(std::vector<TodoCard> &cards, const char *dragDropType, const char *columnName);
        void handleCardDrop(const std::string &id, const ImVec2 &columnPos, const ImVec2 &columnSize);
    };
}

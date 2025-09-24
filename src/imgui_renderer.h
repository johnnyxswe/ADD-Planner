//
// Created by Johnny Gonzales on 8/23/25.
//

#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "todo_card.h"


#include "imgui.h"
#include "pomodoro_timer.h"


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
        void handleDropZoneTarget(CardColumnType cardColumn);
        void renderDropZoneOverlays();
        void handleDropZoneMove(const DragDropPayload *payload, CardColumnType targetColumn);
        void assignCardList(TodoCard &card, std::vector<TodoCard> &cards) const;
        void loadCardLists(std::vector<TodoCard> &todoCards,
                           std::vector<TodoCard> &inProgressCards, std::vector<TodoCard> &doneCards) const;
        void renderUI();
        void shutdown();

        void beginFrame();
        void endFrame();
        void render(VkCommandBuffer commandBuffer);

        void renderHeader();

        void openConfirmDeleteModal(int cardId);
        void renderConfirmDeleteModal();
        void openViewCardModal();
        void renderViewCardModal();
        void renderAddProjectModal();
        void createDescriptorPool();

        void openEditCardModal();
        int get_value();
        void renderEditCardModal();

        void openNewCardModal();
        void renderNewCardModal();

    private:
        Graphics *graphics_ = nullptr;
        Application *app_ = nullptr;

        PomodoroTimer pomodoroTimer;

        VkDescriptorPool descriptorPool_ = nullptr;
        int currentProject_= 0;
        int selectedProject_ = 0;

        const float columnCount_ = 3.0;
        float columnWidth_ = 0;
        float columnHeight_ = 0;


        const std::string todoDropZoneId_ = "TodoDropZone";
        const std::string inProgressDropZoneId_ = "InProgressDropZone";
        const std::string doneDropZoneId_ = "DoneDropZone";

        TodoCard pendingEditCard_{};
        TodoCard pendingViewCard_{};

        int cardToDelete_ = -1;
        char cardTitle_[256] = "";
        char cardDescription_[1024] = "";
        int selectedStatus_ = 0; // 0=Todo, 1=InProgress, 2=Done
        int selectedSequence_ = -1;
        int pendingDeleteCardId_ = -1;

        bool shouldOpenAddModal_ = false;
        bool shouldOpenEditModal_ = false;
        bool shouldOpenDeleteModal_ = false;
        bool shouldOpenViewCardModal_ = false;
        bool shouldOpenProjectModal_ = false;

        bool showAddCardModal_ = false;
        bool showEditCardModal_ = false;
        bool showProjectModal_ = false;

        char projectName_[256] = "";
        int selectedProjectStatus_ = 0;
        const float comboBoxSize_ = 200.0f;

        // CRUD Actions
        bool createCard();
        bool createProject();
        void updateCard(TodoCard &card) const;
        bool deleteCard(TodoCard &card);

        void drawCardColumn(std::vector<TodoCard> &cards, const char *dragDropType);
        void handleCardReorder(const ImGuiPayload *imGuiPayload, int currentCardIndex, const char *columnType);
        void ReorderCards(int from_index, int to_index);
        void drawSingleCard(TodoCard &card, int cardIndex, const char *dragDropType);
        void drawCardContent(const TodoCard &card);

        void handleColumnDrop(const char *dragDropType);
        void handleInProgressDrop(std::vector<TodoCard> inProgressCards, TodoCard &card);
        void handleDoneCardDrop(std::vector<TodoCard> doneCards, TodoCard &card);
        void handleTodoCardDrop(std::vector<TodoCard> todoCards, TodoCard &card);
        void handleCardDrop(const std::string &id, const ImVec2 &columnPos, const ImVec2 &columnSize);

        // Helpers
        int getColumnTypeFromDragDrop(const char * dragDropType);

    };
}

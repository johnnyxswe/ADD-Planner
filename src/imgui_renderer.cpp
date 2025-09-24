//
// Created by Johnny Gonzales on 8/23/25.
//

#include "imgui_renderer.h"

#include "application.h"
#include "card_database.h"
#include "graphics.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "glm/vec2.hpp"
#include "spdlog/spdlog.h"
#include "todo_card.h"
#include <algorithm>

namespace todo {
    void ImGuiRenderer::initialize()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        float xscale, yscale;
        glfwGetWindowContentScale(app_->getWindow(), &xscale, &yscale);
        io.DisplayFramebufferScale = ImVec2(xscale, yscale);

        // Method 2: Load with custom configuration
        ImFontConfig fontConfig;
        fontConfig.SizePixels = 18.0f;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = true;
        // io.Fonts->AddFontFromFileTTF("In ter-VariableFont_opsz,wght.ttf", 16.0f, &fontConfig);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Customize style for transparency
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg].w = 0.0f; // Make window background fully transparent
        // You can also adjust other elements:
        // style.Colors[ImGuiCol_ChildBg].w = 0.0f; // Child window background
        // style.Colors[ImGuiCol_PopupBg].w = 0.8f; // Popup background (semi-transparent)

        ImGui_ImplGlfw_InitForVulkan(app_->getWindow(), true);
        createDescriptorPool();

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = graphics_->getInstance();
        initInfo.PhysicalDevice = graphics_->getPhysicalDevice();
        initInfo.Device = graphics_->getDevice();
        // initInfo.QueueFamily = graphics_->getGraphicsQueueFamily();
        initInfo.Queue = graphics_->getGraphicsQueue();
        initInfo.DescriptorPool = descriptorPool_;
        initInfo.RenderPass = graphics_->getRenderPass();
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = graphics_->getSwapChainImages().size();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            throw std::runtime_error("Failed to initialize ImGui Vulkan backend!");
        }

        // Initialize Pomodoro Timer
        pomodoroTimer.initialize(&app_->audio());
    }

    void ImGuiRenderer::handleDropZoneTarget(CardColumnType cardColumn)
    {
        // Get the current window size
        glm::ivec2 windowSize;
        app_->getWindowSize(windowSize);
    }

    void ImGuiRenderer::renderDropZoneOverlays()
    {
        // Get main kanban window pos and region size
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

        ImVec2 tableStart = ImVec2(winPos.x + contentMin.x, winPos.y + contentMin.y + 60); // +60 if you have a header
        ImVec2 tableEnd = ImVec2(winPos.x + contentMax.x, winPos.y + contentMax.y);

        float totalWidth = tableEnd.x - tableStart.x;
        float totalHeight = tableEnd.y - tableStart.y;

        int numColumns = 3;
        float columnWidth = totalWidth / numColumns;

        const char *dropTypes[] = {"TODO_CARD", "PROGRESS_CARD", "DONE_CARD"};
        const CardColumnType columns[] = {
            CardColumnType::TodoColumn, CardColumnType::InProgressColumn, CardColumnType::DoneColumn
        };
        for (int i = 0; i < numColumns; ++i)
        {
            ImVec2 dropZonePos(tableStart.x + i * columnWidth, tableStart.y);
            ImVec2 dropZoneSize(columnWidth, totalHeight);

            // Move cursor for invisible button
            ImGui::SetCursorScreenPos(dropZonePos);
            std::string id = "DropZone" + std::to_string(i);
            ImGui::InvisibleButton(id.c_str(), dropZoneSize);

            // Visual feedback for hovered zone while dragging
            if (ImGui::IsItemHovered() && ImGui::GetDragDropPayload())
            {
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                drawList->AddRect(dropZonePos,
                                  ImVec2(dropZonePos.x + dropZoneSize.x, dropZonePos.y + dropZoneSize.y),
                                  IM_COL32(0, 255, 0, 120), 0.f, 0, 4.f);
            }

            // Drag drop target for columns
            if (ImGui::BeginDragDropTarget())
            {
                for (int payloadType = 0; payloadType < 3; ++payloadType)
                {
                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dropTypes[payloadType]))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(DragDropPayload));
                        auto *ddPayload = reinterpret_cast<const DragDropPayload *>(payload->Data);
                        // Call your handler for moving cards across columns:
                        handleDropZoneMove(ddPayload, columns[i]);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
    }

    void ImGuiRenderer::handleDropZoneMove(const DragDropPayload *payload, CardColumnType targetColumn)
    {
        // Use your logic to move card at payload->sourceIndex from payload->sourceColumnType to targetColumn
        // For example:
        std::vector<TodoCard> todoCards;
        std::vector<TodoCard> inProgressCards;
        std::vector<TodoCard> doneCards;
        loadCardLists(todoCards, inProgressCards, doneCards);

        std::vector<TodoCard> *columns[] = {
            &todoCards, // assumes you have such accessors
            &inProgressCards,
            &doneCards
        };

        std::vector<TodoCard> &fromVec = *columns[payload->sourceColumnType];
        std::vector<TodoCard> &toVec = *columns[static_cast<int>(targetColumn)];

        if (payload->sourceColumnType != static_cast<int>(targetColumn))
        {
            TodoCard moved = fromVec[payload->sourceIndex];
            moved.status = static_cast<CardStatus>(targetColumn);
            toVec.push_back(moved);
            fromVec.erase(fromVec.begin() + payload->sourceIndex);

            if (moved.status == CardStatus::Done)
            {
                std::string currentTime = getCurrentTimestamp();
                moved.completedAt = currentTime;
            } else
            {
                moved.completedAt = "";
            }

            app_->db().updateCard(moved);
            app_->reloadAppState();
        }
    }


    void ImGuiRenderer::openNewCardModal()
    {
        showAddCardModal_ = true;
        // Clear input fields
        strcpy(cardTitle_, "");
        strcpy(cardDescription_, "");
        selectedStatus_ = 0;
        ImGui::OpenPopup("Add New Card");
    }

    void ImGuiRenderer::renderNewCardModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        constexpr glm::vec2 modalRatio = glm::vec2(0.75f, 0.75f);
        const auto modalSize = ImVec2(windowSize.x * modalRatio.x, windowSize.y * modalRatio.y);
        ImGui::SetNextWindowSize(modalSize, ImGuiCond_Appearing);

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoMove; // Prevent moving if needed
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::BeginPopupModal("Add New Card", &showAddCardModal_, window_flags))
        {
            ImGui::Text("Create a new task card:");
            ImGui::Separator();

            // Title input
            ImGui::Text("Title:");
            constexpr auto inputTextRatio = glm::vec2(0.95f, 0.25f);
            const auto inputTextSize = ImVec2(modalSize.x * inputTextRatio.x, modalSize.y * inputTextRatio.y);
            ImGui::SetNextItemWidth(inputTextSize.x);
            ImGui::InputText("##title", cardTitle_, sizeof(cardTitle_));

            ImGui::Text("Description:");
            constexpr auto multilineRatio = glm::vec2(0.95f, 0.25f);
            const auto multilineSize = ImVec2(modalSize.x * multilineRatio.x, modalSize.y * multilineRatio.y);
            ImGui::InputTextMultiline("##description", cardDescription_, sizeof(cardDescription_),
                                      multilineSize,
                                      ImGuiInputTextFlags_AllowTabInput);
            ImGui::TextDisabled("Tip: Use Enter for new lines. Text will scroll horizontally for long lines.");

            // Status selection
            ImGui::Text("Initial Status:");
            ImGui::SetNextItemWidth(comboBoxSize_);
            const char *statusItems[] = {"Todo", "In Progress", "Done"};
            ImGui::Combo("##status", &selectedStatus_, statusItems, IM_ARRAYSIZE(statusItems));

            ImGui::Separator();

            // Buttons
            if (ImGui::Button("Create Card", ImVec2(120, 0)))
            {
                if (strlen(cardTitle_) > 0)
                {
                    if (!createCard())
                    {
                        spdlog::error("Failed to create card!");
                        return;
                    }
                    ImGui::CloseCurrentPopup();
                    showAddCardModal_ = false;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                showAddCardModal_ = false;
            }

            ImGui::EndPopup();
        }
    }


    void ImGuiRenderer::renderHeader()
    {
        // Header background (optional)
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 headerStart = ImGui::GetCursorScreenPos();
        ImVec2 headerSize = ImVec2(ImGui::GetContentRegionAvail().x, 150);

        // Draw header background
        drawList->AddRectFilled(headerStart,
                                ImVec2(headerStart.x + headerSize.x, headerStart.y + headerSize.y),
                                IM_COL32(40, 40, 40, 255)); // Dark background

        // Header content
        if (ImGui::BeginTable("##header", 3, ImGuiTableRowFlags_Headers, headerSize))
        {
            auto projects = app_->getProjects();

            ImGui::TableSetupColumn("Column 1");
            ImGui::TableSetupColumn("Column 2");
            ImGui::TableSetupColumn("Column 3");

            // Skip TableHeadersRow() and go straight to custom content
            ImGui::TableNextRow();

            // Left side - Project Dropdown
            ImGui::TableNextColumn();
            ImGui::Text("Project:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);

            // Convert std::vector<std::string> to std::vector<const char*>
            std::vector<const char *> project_items;
            project_items.reserve(projects.size());
            for (auto &project: projects)
            {
                project_items.push_back(project.name.c_str());
            }
            ImGui::Combo("##project", &currentProject_, project_items.data(), project_items.size());

            // Center - Pomodoro Timer
            ImGui::TableNextColumn();
            pomodoroTimer.Update();
            pomodoroTimer.DrawWidget();

            // Optional: Check if timer finished for notifications/sounds
            if (pomodoroTimer.IsFinished())
            {
                // Play notification sound, show popup, etc.
            }

            // Right side - Button
            ImGui::TableNextColumn();
            float availableWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 200;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);

            if (ImGui::Button("+ Add New Card", ImVec2(buttonWidth, headerSize.y * .25)))
            {
                // Handle add card
                // openNewCardModal(); // This will open the modal
                shouldOpenAddModal_ = true;
            }

            ImGui::EndTable();
        };

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            spdlog::info("Right click on header");
            ImGui::OpenPopup("HeaderContextMenu");
        }

        // Add some padding after header
        ImGui::Dummy(ImVec2(0, 10));

        if (ImGui::BeginPopup("HeaderContextMenu"))
        {
            if (ImGui::MenuItem("Add Project"))
            {
                shouldOpenProjectModal_ = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (shouldOpenProjectModal_)
        {
            shouldOpenProjectModal_ = false;
            // Clear input fields
            strcpy(projectName_, "");
            selectedProjectStatus_ = 0;

            showProjectModal_ = true;
            ImGui::OpenPopup("Add Project");
        }
    }

    void ImGuiRenderer::beginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiRenderer::endFrame()
    {
        ImGui::Render();
    }

    void ImGuiRenderer::render(VkCommandBuffer commandBuffer)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    bool ImGuiRenderer::createCard()
    {
        // Add to database
        auto projects = app_->getProjects();
        if (!app_->db().addCard(cardTitle_, cardDescription_, selectedStatus_, selectedSequence_,
                                projects[currentProject_].id))
        {
            spdlog::error("Failed to add card to database!");
            return false;
        }
        app_->loadCards(); // Update cards in app
        return true;
    }

    bool ImGuiRenderer::createProject()
    {
        // Add to database
        auto projects = app_->getProjects();
        if (!app_->db().addProject(projectName_, selectedProjectStatus_))
        {
            spdlog::error("Failed to add card to database!");
            return false;
        }
        app_->reloadAppState(); // Update cards in app
        return true;
    }

    void ImGuiRenderer::openEditCardModal()
    {
        shouldOpenViewCardModal_ = true;
        ImGui::OpenPopup("Edit Card");
    }

    int ImGuiRenderer::get_value()
    {
        return selectedProject_;
    }

    void ImGuiRenderer::renderEditCardModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        constexpr glm::vec2 modalRatio = glm::vec2(0.75f, 0.75f);
        const auto modalSize = ImVec2(windowSize.x * modalRatio.x, windowSize.y * modalRatio.y);
        ImGui::SetNextWindowSize(modalSize, ImGuiCond_Appearing);

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoMove; // Prevent moving if needed
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::BeginPopupModal("Edit Card", &showEditCardModal_, window_flags))
        {
            // Modal content...
            // Title input
            ImGui::Text("Title:");
            constexpr auto inputTextRatio = glm::vec2(0.95f, 0.25f);
            const auto inputTextSize = ImVec2(modalSize.x * inputTextRatio.x, modalSize.y * inputTextRatio.y);
            ImGui::SetNextItemWidth(inputTextSize.x);
            ImGui::InputText("##title", cardTitle_, sizeof(cardTitle_));

            ImGui::Separator();
            ImGui::NewLine();

            // Description input
            ImGui::Text("Description:");
            constexpr auto multilineRatio = glm::vec2(0.95f, 0.25f);
            const auto multilineSize = ImVec2(modalSize.x * multilineRatio.x, modalSize.y * multilineRatio.y);
            ImGui::InputTextMultiline("##description", cardDescription_, sizeof(cardDescription_),
                                      multilineSize,
                                      ImGuiInputTextFlags_AllowTabInput);
            ImGui::TextDisabled("Tip: Use Enter for new lines. Text will scroll horizontally for long lines.");

            ImGui::NewLine();

            // Status Dropdown
            // ImGui::Text("Status:");
            // ImGui::SameLine();
            // ImGui::SetNextItemWidth(200.0f);
            // const char *statusItems[] = {"Todo", "In Progress", "Done"};
            // ImGui::Combo("##status", &selectedStatus_, statusItems, IM_ARRAYSIZE(statusItems));


            // Project Dropdown
            ImGui::Text("Project:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(comboBoxSize_);

            // Create array of project names for the combo
            std::vector<const char *> projectNames;
            const auto &projects = app_->getProjects();

            for (const auto &project: projects)
            {
                projectNames.push_back(project.name.c_str());
            }

            // Ensure valid index
            selectedProject_ = std::clamp(selectedProject_, 0, (int) projects.size() - 1);

            // CORRECT - check if selection changed
            if (ImGui::Combo("Project", &selectedProject_, projectNames.data(), projectNames.size()))
            {
                spdlog::info("Project selection changed to: {}", selectedProject_);
                // Do something when selection changes
            }

            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                // Update the card
                pendingEditCard_.title = std::string(cardTitle_);
                pendingEditCard_.description = std::string(cardDescription_);
                // pendingEditCard_.status = static_cast<CardStatus>(selectedStatus_);
                pendingEditCard_.projectId = projects[selectedProject_].id;

                app_->db().updateCard(pendingEditCard_);
                app_->reloadAppState(); // Update cards in app

                ImGui::CloseCurrentPopup();
                shouldOpenEditModal_ = false;
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                shouldOpenEditModal_ = false;
            }

            ImGui::EndPopup();
        }
    }

    void ImGuiRenderer::openConfirmDeleteModal(int cardId)
    {
        cardToDelete_ = cardId;
        shouldOpenDeleteModal_ = true;
        ImGui::OpenPopup("Confirm Delete");
    }

    void ImGuiRenderer::renderConfirmDeleteModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x * 0.50, windowSize.y * 0.20), ImGuiCond_Appearing);
        // Each time it appears

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoResize; // Prevent resizing if needed
        window_flags |= ImGuiWindowFlags_NoMove; // Prevent moving if needed

        if (ImGui::BeginPopupModal("Confirm Delete", &shouldOpenDeleteModal_, window_flags))
        {
            ImGui::Text("Are you sure you want to delete this card?");
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                if (cardToDelete_ != -1)
                {
                    app_->db().removeCard(cardToDelete_);
                    app_->reloadAppState(); // Update cards in app
                }
                ImGui::CloseCurrentPopup();
                shouldOpenDeleteModal_ = false;
                cardToDelete_ = -1;
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                shouldOpenDeleteModal_ = false;
                cardToDelete_ = -1;
            }

            ImGui::EndPopup();
        }
    }

    void ImGuiRenderer::openViewCardModal()
    {
        // Pre-fill the form fields
        strncpy(cardTitle_, pendingViewCard_.title.c_str(), sizeof(cardTitle_) - 1);
        strncpy(cardDescription_, pendingViewCard_.description.c_str(), sizeof(cardDescription_) - 1);
        selectedStatus_ = static_cast<int>(pendingViewCard_.status);

        shouldOpenViewCardModal_ = true;
        ImGui::OpenPopup("View Card");
    }

    void ImGuiRenderer::renderViewCardModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x * 0.75, windowSize.y * 0.75), ImGuiCond_Appearing);
        // Each time it appears

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoResize; // Prevent resizing if needed
        window_flags |= ImGuiWindowFlags_NoMove; // Prevent moving if needed

        if (ImGui::BeginPopupModal("View Card", &shouldOpenViewCardModal_, window_flags))
        {
            ImVec2 cardPos = ImGui::GetItemRectMin();
            ImVec2 cardSize = ImGui::GetItemRectSize();

            // Modal content...
            ImGui::Text("Edit");
            ImGui::Separator();

            // Title input
            ImGui::Text("Title:");
            ImGui::LabelText("##title", cardTitle_, sizeof(cardTitle_));

            // Description input (with text wrapping)
            ImVec2 descPos(cardPos.x + 8, cardPos.y + 28);
            float maxWidth = cardSize.x - 16;
            ImGui::Text("Description:");
            ImGui::TextWrapped("%s", cardDescription_);

            // Card description (with text wrapping)

            // drawList->AddText(nullptr, 0, descPos, IM_COL32(200, 200, 200, 255),
            //                   card.description.c_str(), nullptr, maxWidth);

            // Status selection
            ImGui::Text("Status:");
            const char *statusItems[] = {"Todo", "In Progress", "Done"};
            ImGui::LabelText("##status", "todo");

            ImGui::Separator();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                shouldOpenViewCardModal_ = false;
            }

            ImGui::EndPopup();
        }
    }

    void ImGuiRenderer::renderAddProjectModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x * 0.75, windowSize.y * 0.75), ImGuiCond_Appearing);
        // Each time it appears

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoResize; // Prevent resizing if needed
        window_flags |= ImGuiWindowFlags_NoMove; // Prevent moving if needed

        if (ImGui::BeginPopupModal("Add Project", &showProjectModal_, window_flags))
        {
            ImVec2 cardPos = ImGui::GetItemRectMin();
            ImVec2 cardSize = ImGui::GetItemRectSize();

            // Modal content...
            ImGui::Text("Add Project");
            ImGui::Separator();

            // Title input
            ImGui::Text("Name:");
            ImGui::InputText("##name", projectName_, sizeof(projectName_));

            ImGui::Text("Initial Status:");
            // const char *statusItems[] = {"Todo", "In Progress", "Done"};
            ImGui::Combo("##status", &selectedProjectStatus_, proj::projectStatusNames.data(),
                         proj::projectStatusNames.size());

            ImGui::Separator();

            // Buttons
            if (ImGui::Button("Add Project", ImVec2(120, 0)))
            {
                if (strlen(projectName_) > 0)
                {
                    if (!createProject())
                    {
                    }
                    ImGui::CloseCurrentPopup();
                    showProjectModal_ = false;
                }
            }

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                shouldOpenViewCardModal_ = false;
            }

            ImGui::EndPopup();
        }
    }


    void ImGuiRenderer::createDescriptorPool()
    {
        // Create descriptor pool for ImGui
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = std::size(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(graphics_->getDevice(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create ImGui descriptor pool!");
        }
    }

    bool ImGuiRenderer::deleteCard(TodoCard &card)
    {
    }

    void render(VkCommandBuffer commandBuffer)
    {
        ImDrawData *drawData = ImGui::GetDrawData();
        if (drawData)
        {
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }
    }

    // Define a custom comparator function or functor
    struct CardSequenceComparator
    {
        bool operator()(const TodoCard &a, const TodoCard &b) const
        {
            return a.sequence < b.sequence;
        }
    };

    void ImGuiRenderer::assignCardList(TodoCard &card, std::vector<TodoCard> &cards) const
    {
        if (card.status == CardStatus::Done) card.color = glm::vec4(0.144f, 0.238f, 0.144f, 1.0f);
        cards.push_back(card);
    }

    void ImGuiRenderer::loadCardLists(std::vector<TodoCard> &todoCards,
                                      std::vector<TodoCard> &inProgressCards, std::vector<TodoCard> &doneCards) const
    {
        auto cards = app_->getCards();
        auto projects = app_->getProjects();
        int projectId = 0;

        if (projects.size())
        {
            projectId = projects[currentProject_].id;
        }

        for (auto &card: cards)
        {
            if (card.projectId != projectId) continue;

            switch (card.status)
            {
                case CardStatus::Todo: assignCardList(card, todoCards);
                    break;
                case CardStatus::InProgress: assignCardList(card, inProgressCards);
                    break;
                case CardStatus::Done: assignCardList(card, doneCards);
                    break;
                default: break;
            }
        }

        // Sort each vector by the sequence field
        std::ranges::sort(todoCards, CardSequenceComparator());
        std::ranges::sort(inProgressCards, CardSequenceComparator());
        std::ranges::sort(doneCards, CardSequenceComparator());
    }

    void ImGuiRenderer::renderUI()
    {
        std::vector<TodoCard> todoCards;
        std::vector<TodoCard> inProgressCards;
        std::vector<TodoCard> doneCards;
        loadCardLists(todoCards, inProgressCards, doneCards);

        // Get the current window size
        glm::ivec2 windowSize;
        app_->getWindowSize(windowSize);

        // Create main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        ImGui::SetNextWindowBgAlpha(0.85f); // 0.0f = fully transparent, 1.0f = fully opaque
        // ImGui::SetNextWindowBgAlpha(0.0f); // 0.0f = fully transparent, 1.0f = fully opaque

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoTitleBar;

        bool p_open = true;
        // Start ImGui UI Window

        if (!ImGui::Begin("ADD - TODO", &p_open, window_flags))
        {
            // Early out if the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }

        // Calculate column width (subtract padding and spacing)
        float availableWidth = ImGui::GetContentRegionAvail().x;
        columnWidth_ = (availableWidth - 20) / columnCount_; // 20 for spacing between columns
        columnHeight_ = ImGui::GetContentRegionAvail().y - 50; // Leave space for bottom padding

        // Create header section
        renderHeader();

        // Add separator between header and table
        ImGui::Separator();
        ImGui::Spacing();

        ImVec2 tablePos = ImGui::GetCursorScreenPos();

        ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        // Create table with 3 columns
        if (ImGui::BeginTable("KanbanTable", 3, table_flags))
        {
            // Setup columns
            ImGui::TableSetupColumn("TODO", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("IN PROGRESS", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("DONE", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();

            // Column 1: TODO
            ImGui::TableSetColumnIndex(CardColumnType::TodoColumn);
            drawCardColumn(todoCards, "TODO_CARD");

            // Column 2: IN PROGRESS
            ImGui::TableSetColumnIndex(CardColumnType::InProgressColumn);
            drawCardColumn(inProgressCards, "PROGRESS_CARD");

            // Column 3: DONE
            ImGui::TableSetColumnIndex(CardColumnType::DoneColumn);
            drawCardColumn(doneCards, "DONE_CARD");

            ImGui::EndTable();
        }

        // Create invisible overlay drop zones
        renderDropZoneOverlays();

        // Handle deferred modal opening BEFORE calling modal render functions
        if (shouldOpenEditModal_)
        {
            shouldOpenEditModal_ = false;

            if (!showEditCardModal_)
            {
                // Copy data to input buffers
                strncpy(cardTitle_, pendingEditCard_.title.c_str(), sizeof(cardTitle_) - 1);
                cardTitle_[sizeof(cardTitle_) - 1] = '\0';

                strncpy(cardDescription_, pendingEditCard_.description.c_str(), sizeof(cardDescription_) - 1);
                cardDescription_[sizeof(cardDescription_) - 1] = '\0';

                auto &projects = app_->getProjects();
                for (int i = 0; i < projects.size(); i++)
                {
                    if (projects[i].id == pendingEditCard_.projectId)
                    {
                        selectedProject_ = i;
                        break;
                    }
                }

                // selectedStatus_ = static_cast<int>(pendingEditCard_.status);

                showEditCardModal_ = true;
                ImGui::OpenPopup("Edit Card");
            }
        }

        if (shouldOpenAddModal_)
        {
            shouldOpenAddModal_ = false;
            openNewCardModal();
        }

        if (shouldOpenDeleteModal_)
        {
            shouldOpenDeleteModal_ = false;
            openConfirmDeleteModal(pendingDeleteCardId_);
        }

        if (shouldOpenViewCardModal_)
        {
            shouldOpenViewCardModal_ = false;
            openViewCardModal();
        }

        // Render modals
        renderNewCardModal();
        renderEditCardModal();
        renderConfirmDeleteModal();
        renderViewCardModal();

        renderAddProjectModal();

        ImGui::End();
    }

    void ImGuiRenderer::shutdown()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (descriptorPool_ != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(graphics_->getDevice(), descriptorPool_, nullptr);
        }
    }

    // Helper function to draw cards in a column
    void ImGuiRenderer::drawCardColumn(std::vector<TodoCard> &cards, const char *dragDropType)
    {
        // Create scrollable region for cards
        float availableHeight = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginChild((std::string(dragDropType) + "_ScrollRegion").c_str(),
                              ImVec2(0, availableHeight), false))
        {
            for (int i = 0; i < cards.size(); i++)
            {
                ImGui::PushID(cards[i].id);
                drawSingleCard(cards[i], i, dragDropType);
                ImGui::PopID();

                if (i < cards.size() - 1)
                    ImGui::Spacing();
            }
        }
        ImGui::EndChild();

        // Handle drop target for the entire column
        // handleColumnDrop(dragDropType);
    }

    void ImGuiRenderer::handleCardReorder(const ImGuiPayload *imGuiPayload, int currentCardIndex,
                                          const char *columnType)
    {
        // Get payload data
        auto *payload = static_cast<DragDropPayload *>(imGuiPayload->Data);
        int sourceIndex = payload->sourceIndex;
        int sourceColumnType = payload->sourceColumnType;

        auto cards = app_->getCards();
        auto &sourceCard = cards[sourceIndex];
        auto &currentCard = cards[currentCardIndex];

        int sourceCardSequence = sourceCard.sequence;
        int currentCardSequence = currentCard.sequence;

        app_->db().reorderCards(sourceIndex, currentCardIndex);

        //
        //
        // // Update the sequence of the source card to match the sequence of the current card
        // sourceCard.sequence = currentCardSequence;
        //
        // // Update the database with the new sequence for the source card
        // if (!app_->db().updateCard(sourceCard))
        // {
        //     spdlog::error("Failed to update card sequence");
        //     return;
        // }
        //
        // // Move the card from the source index to the current index in the vector
        // // std::swap(cards[sourceIndex], cards[currentCardIndex]);
        //
        // // Update the database with the new position for the current card (if necessary)
        // // if (sourceIndex != currentCardIndex) {
        // currentCard.sequence = sourceCardSequence;
        // if (!app_->db().updateCard(currentCard))
        // {
        //     spdlog::error("Failed to update card sequence");
        //     return;
        // }
        // // }

        // // Load the updated list of cards
        app_->reloadAppState(); // Update cards in app
    }

    int ImGuiRenderer::getColumnTypeFromDragDrop(const char *dragDropType)
    {
        if (dragDropType == "TODO_CARD") return CardColumnType::TodoColumn;
        else if (dragDropType == "PROGRESS_CARD") return CardColumnType::InProgressColumn;
        else if (dragDropType == "DONE_CARD") return CardColumnType::DoneColumn;
        else return -1;
    }

    void ImGuiRenderer::updateCard(TodoCard &card) const
    {
        spdlog::info("Card Completed at: {}", card.completedAt);
        if (!app_->db().updateCard(card))
        {
            spdlog::error("Failed to update card");
        }
        app_->reloadAppState(); // Update cards in app
    }

    void ImGuiRenderer::drawSingleCard(TodoCard &card, int cardIndex, const char *dragDropType)
    {
        // Set card colors
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(card.color.r, card.color.g, card.color.b, card.color.a));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(card.color.r * 1.2f, card.color.g * 1.2f, card.color.b * 1.2f, card.color.a));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(card.color.r * 0.8f, card.color.g * 0.8f, card.color.b * 0.8f, card.color.a));

        // Card button
        float buttonWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 buttonSize(buttonWidth, 100); // Fixed height for cards

        bool cardPressed = ImGui::Button("##card", buttonSize);

        // Handle drag and drop
        if (ImGui::BeginDragDropSource())
        {
            DragDropPayload payload = {cardIndex, getColumnTypeFromDragDrop(dragDropType)};
            ImGui::SetDragDropPayload(dragDropType, &payload, sizeof(DragDropPayload));
            ImGui::Text("Moving: %s", card.title.c_str());
            ImGui::EndDragDropSource();
        }

        // Handle drop between cards
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *imguiPayload = ImGui::AcceptDragDropPayload(dragDropType))
            {
                // Handle reordering within same column
                handleCardReorder(imguiPayload, cardIndex, dragDropType);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopStyleColor(3);

        // Draw card content overlay
        drawCardContent(card);

        // Handle card interactions
        if (cardPressed)
        {
            // card.completed = !card.completed;
            // Update card in database
            // updateCard(card);
            pendingViewCard_ = card;
            shouldOpenViewCardModal_ = true;
        }


        // Handle right-click context menu
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("CardContextMenu");
        }

        if (ImGui::BeginPopup("CardContextMenu"))
        {
            if (ImGui::MenuItem("Edit"))
            {
                shouldOpenEditModal_ = true;
                pendingEditCard_ = card;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("Delete"))
            {
                shouldOpenDeleteModal_ = true;
                pendingDeleteCardId_ = card.id;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void ImGuiRenderer::drawCardContent(const TodoCard &card)
    {
        ImVec2 cardPos = ImGui::GetItemRectMin();
        ImVec2 cardSize = ImGui::GetItemRectSize();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        // Card title (with text wrapping)
        ImVec2 titlePos(cardPos.x + 8, cardPos.y + 8);
        float maxWidth = cardSize.x - 16;
        drawList->AddText(nullptr, 0, titlePos, IM_COL32(255, 255, 255, 255),
                          card.title.c_str(), nullptr, maxWidth);


        // Card description (with text wrapping)
        // ImVec2 descPos(cardPos.x + 8, cardPos.y + 28);
        // float maxWidth = cardSize.x - 16;
        // drawList->AddText(nullptr, 0, descPos, IM_COL32(200, 200, 200, 255),
        //                   card.description.c_str(), nullptr, maxWidth);

        // Status indicator
        constexpr auto statusTextOffset = glm::vec2 (8, -34);
        const char *checkmark = "[X] Completed";
        const char *circle = "[ ] Pending";
        const char *statusText = card.status == CardStatus::Done ? checkmark : circle;
        ImU32 statusColor = card.status == CardStatus::Done ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255);
        drawList->AddText(ImVec2(cardPos.x + statusTextOffset.x, cardPos.y + cardSize.y + statusTextOffset.y),
                          statusColor, statusText);

        if (card.status == CardStatus::Done)
        {
            constexpr auto completedAtOffset = glm::vec2 (8, -18);
            const char *completedAtText = card.completedAt.empty() ? "" : card.completedAt.c_str();
            drawList->AddText(ImVec2(cardPos.x + completedAtOffset.x, cardPos.y + cardSize.y + completedAtOffset.y), statusColor, completedAtText);
        }

    }

    void ImGuiRenderer::handleColumnDrop(const char *dragDropType)
    {
        if (ImGui::BeginDragDropTarget())
        {
            // Accept drops from other columns
            const char *otherTypes[] = {"TODO_CARD", "PROGRESS_CARD", "DONE_CARD"};

            for (const char *otherType: otherTypes)
            {
                if (strcmp(otherType, dragDropType) != 0) // Don't accept from same type
                {
                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(otherType))
                    {
                        // handleCrossColumnMove(payload, otherType, dragDropType);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }


    void ImGuiRenderer::handleInProgressDrop(std::vector<TodoCard> inProgressCards, TodoCard &card)
    {
        card.status = CardStatus::InProgress;
        inProgressCards.push_back(card);
    }

    void ImGuiRenderer::handleDoneCardDrop(std::vector<TodoCard> doneCards, TodoCard &card)
    {
        card.status = CardStatus::Done;
        card.completedAt = std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        updateCard(card);
        doneCards.push_back(card);
    }

    void ImGuiRenderer::handleTodoCardDrop(std::vector<TodoCard> todoCards, TodoCard &card)
    {
        card.status = CardStatus::Todo;
        todoCards.push_back(card);
    }

    void ImGuiRenderer::handleCardDrop(const std::string &id, const ImVec2 &columnPos, const ImVec2 &columnSize)
    {
        std::vector<TodoCard> todoCards;
        std::vector<TodoCard> inProgressCards;
        std::vector<TodoCard> doneCards;
        loadCardLists(todoCards, inProgressCards, doneCards);


        // Overlay button to receive drops
        ImGui::SetCursorScreenPos(columnPos);
        ImGui::InvisibleButton(id.c_str(), columnSize);

        if (ImGui::BeginDragDropTarget())
        {
            // Handle Card Drop from TODO
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TODO_CARD"))
            {
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = todoCards[cardPayload->sourceIndex];
                todoCards.erase(todoCards.begin() + cardPayload->sourceIndex);

                if (id == inProgressDropZoneId_)
                {
                    handleInProgressDrop(inProgressCards, card);
                } else if (id == doneDropZoneId_)
                {
                    handleDoneCardDrop(doneCards, card);
                } else
                {
                    spdlog::error("Invalid destination column for card drop");
                }

                app_->db().updateCard(card);
            }

            // Handle Card Drop from IN PROGRESS
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("PROGRESS_CARD"))
            {
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = inProgressCards[cardPayload->sourceIndex];
                inProgressCards.erase(inProgressCards.begin() + cardPayload->sourceIndex);

                if (id == todoDropZoneId_)
                {
                    handleTodoCardDrop(todoCards, card);
                } else if (id == doneDropZoneId_)
                {
                    handleDoneCardDrop(doneCards, card);
                } else
                {
                    spdlog::error("Invalid destination column for card drop");
                }

                app_->db().updateCard(card);
            }

            // Handle Card Drop from DONE
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DONE_CARD"))
            {
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = doneCards[cardPayload->sourceIndex];
                doneCards.erase(doneCards.begin() + cardPayload->sourceIndex);

                if (id == todoDropZoneId_)
                {
                    handleTodoCardDrop(todoCards, card);
                } else if (id == inProgressDropZoneId_)
                {
                    handleInProgressDrop(inProgressCards, card);
                } else
                {
                    spdlog::error("Invalid destination column for card drop");
                }

                app_->db().updateCard(card);
            }
        }

        // Visual feedback when hovering over drop zone
        if (ImGui::IsItemHovered() && ImGui::GetDragDropPayload())
        {
            ImDrawList *drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(columnPos,
                              ImVec2(columnPos.x + columnSize.x, columnPos.y + columnSize.y),
                              IM_COL32(0, 255, 0, 100), 0.0f, 0, 3.0f);
        }
    }

    ImGuiRenderer::~ImGuiRenderer()
    {
        shutdown();
    }
}

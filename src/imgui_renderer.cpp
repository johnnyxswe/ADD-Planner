//
// Created by Johnny Gonzales on 8/23/25.
//

#include "imgui_renderer.h"

#include "application.h"
#include "card_database.h"
#include "graphics.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "glm/vec2.hpp"
#include "spdlog/spdlog.h"
#include "todo_card.h"

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

        // Method 2: Load with custom configuration
        ImFontConfig fontConfig;
        fontConfig.SizePixels = 18.0f;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF("Inter-VariableFont_opsz,wght.ttf", 16.0f, &fontConfig);

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

    void ImGuiRenderer::renderUI()
    {
        auto &todoCards = app_->getTodoCards();
        auto &inProgressCards = app_->getInProgressCards();
        auto &doneCards = app_->getDoneCards();

        // Get the current window size
        glm::ivec2 windowSize;
        app_->getWindowSize(windowSize);

        // Set the ImGui window to cover the entire GLFW window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        ImGui::SetNextWindowBgAlpha(0.85f); // 0.0f = fully transparent, 1.0f = fully opaque
        // ImGui::SetNextWindowBgAlpha(0.0f); // 0.0f = fully transparent, 1.0f = fully opaque

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;

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

        // First Column
        ImGui::BeginChild("TodoColumn", ImVec2(columnWidth_, columnHeight_), true);

        // Column Header
        ImGui::Text("TODO");

        // Store column position and size
        ImVec2 todoColumnPos = ImGui::GetWindowPos();
        ImVec2 todoColumnSize = ImGui::GetWindowSize();

        // Draw your cards normally
        drawCardColumn(todoCards, "TODO_CARD", "TodoColumn");

        ImGui::EndChild();

        ImGui::SameLine();

        // Second Column
        ImGui::BeginChild("InProgressColumn", ImVec2(columnWidth_, columnHeight_), true);

        // Column Header
        ImGui::Text("IN PROGRESS");

        // Store column position and size
        ImVec2 progressColumnPos = ImGui::GetWindowPos();
        ImVec2 progressColumnSize = ImGui::GetWindowSize();

        // Draw your cards normally
        drawCardColumn(inProgressCards, "PROGRESS_CARD", "InProgressColumn");

        ImGui::EndChild();
        ImGui::SameLine();

        // Third Column
        ImGui::BeginChild("DoneColumn", ImVec2(columnWidth_, columnHeight_), true);

        // Column Header
        ImGui::Text("DONE");

        // Store column position and size
        ImVec2 doneColumnPos = ImGui::GetWindowPos();
        ImVec2 doneColumnSize = ImGui::GetWindowSize();

        // Draw your cards normally
        drawCardColumn(doneCards, "DONE_CARD", "DoneColumn");

        ImGui::EndChild();

        // Now create invisible overlay buttons for cross-column drag and drop
        handleCardDrop(todoDropZoneId_, todoColumnPos, todoColumnSize);

        // Now create invisible overlay buttons for cross-column drag and drop
        handleCardDrop(inProgressDropZoneId_, progressColumnPos, progressColumnSize);

        // Now create invisible overlay buttons for cross-column drag and drop
        handleCardDrop(doneDropZoneId_, doneColumnPos, doneColumnSize);

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
    void ImGuiRenderer::drawCardColumn(std::vector<TodoCard> &cards, const char *dragDropType, const char *columnName)
    {
        // Create payload with card info
        // Send only index and column type as payload:
        int columnEnum = 0;
        if (strcmp(columnName, "TodoColumn") == 0) columnEnum = 0;
        else if (strcmp(columnName, "InProgressColumn") == 0) columnEnum = 1;
        else if (strcmp(columnName, "DoneColumn") == 0) columnEnum = 2;

        for (int i = 0; i < cards.size(); i++)
        {
            ImGui::PushID(cards[i].id);

            // Create a colored button for each card
            ImGui::PushStyleColor(ImGuiCol_Button, {
                                      cards[i].color.r, cards[i].color.g, cards[i].color.b, cards[i].color.a
                                  });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(cards[i].color.x * 1.2f, cards[i].color.y * 1.2f,
                                         cards[i].color.z * 1.2f, cards[i].color.w));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(cards[i].color.x * 0.8f, cards[i].color.y * 0.8f,
                                         cards[i].color.z * 0.8f, cards[i].color.w));

            // Calculate button size based on available column width
            float buttonWidth = ImGui::GetContentRegionAvail().x - 10;
            ImVec2 buttonSize(buttonWidth, 80);

            // Draw the card button
            bool cardPressed = ImGui::Button("", buttonSize);

            // Handle right clicks
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                spdlog::info("Right click on card {}", cards[i].title);
                ImGui::OpenPopup("RightClickMenu");
            }

            if (ImGui::BeginPopup("RightClickMenu")) {
                if (ImGui::MenuItem("Do Something")) {
                    // Your menu logic here
                }
                if (ImGui::MenuItem("Delete")) {
                    if (app_->db().removeCard(cards[i].id))
                    {
                        cards.erase(cards.begin() + i);
                        i--;
                    } else
                    {
                        spdlog::error("Failed to delete card");
                    }
                }
                ImGui::EndPopup();
            }

            // Handle drag and drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                DragDropPayload payload = {i, columnEnum};
                ImGui::SetDragDropPayload(dragDropType, &payload, sizeof(DragDropPayload));

                // Display a preview of what we're dragging
                ImGui::Text("Moving: %s", cards[i].title.c_str());
                ImGui::EndDragDropSource();
            }

            // Handle drop target within same column
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *imguiPayload = ImGui::AcceptDragDropPayload(dragDropType))
                {
                    IM_ASSERT(imguiPayload->DataSize == sizeof(DragDropPayload));
                    auto *payload = static_cast<DragDropPayload *>(imguiPayload->Data);

                    // Use the payload->sourceIndex and payload->sourceColumnType to access the correct vector/column
                    // And move cards accordingly
                    // Only reorder if dropping in the same column
                    if (payload->sourceColumnType == columnEnum && payload->sourceIndex != i)
                    {
                        TodoCard temp = cards[payload->sourceIndex];
                        cards.erase(cards.begin() + payload->sourceIndex);
                        cards.insert(cards.begin() + i, temp);

                        // TODO: Add sequence column and update sequence here
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopStyleColor(3);

            // Draw card content over the button
            ImVec2 cardPos = ImGui::GetItemRectMin();
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            // Draw card title
            drawList->AddText(ImVec2(cardPos.x + 10, cardPos.y + 10),
                              IM_COL32(255, 255, 255, 255), cards[i].title.c_str());

            // Draw card description
            drawList->AddText(ImVec2(cardPos.x + 10, cardPos.y + 30),
                              IM_COL32(200, 200, 200, 255), cards[i].description.c_str());

            // Draw completion status
            const char *status = cards[i].completed ? "✓ Completed" : "○ Pending";
            drawList->AddText(ImVec2(cardPos.x + 10, cardPos.y + 50),
                              cards[i].completed ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255),
                              status);

            // Handle card click (toggle completion)
            if (cardPressed)
            {
                cards[i].completed = !cards[i].completed;
                cards[i].color = cards[i].completed
                                     ? glm::vec4(0.2f, 0.8f, 0.2f, 1.0f)
                                     : glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    void ImGuiRenderer::handleCardDrop(const std::string &id, const ImVec2 &columnPos, const ImVec2 &columnSize)
    {
        auto &todoCards = app_->getTodoCards();
        auto &inProgressCards = app_->getInProgressCards();
        auto &doneCards = app_->getDoneCards();

        // Overlay button to receive drops
        ImGui::SetCursorScreenPos(columnPos);
        ImGui::InvisibleButton(id.c_str(), columnSize);

        if (ImGui::BeginDragDropTarget())
        {
            // Handle Card Drop from TODO
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TODO_CARD"))
            {
                if (id == todoDropZoneId_)
                {
                    spdlog::info("Dropping card into the same column - do nothing");
                    return;
                }
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = todoCards[cardPayload->sourceIndex];
                todoCards.erase(todoCards.begin() + cardPayload->sourceIndex);

                if (id == inProgressDropZoneId_)
                {
                    card.status = CardStatus::InProgress;
                    inProgressCards.push_back(card);
                } else if (id == doneDropZoneId_)
                {
                    card.status = CardStatus::Done;
                    doneCards.push_back(card);
                } else
                {
                    spdlog::error("Invalid destination column for card drop");
                }

                app_->db().updateCard(card);
            }

            // Handle Card Drop from IN PROGRESS
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("PROGRESS_CARD"))
            {
                if (id == inProgressDropZoneId_)
                {
                    spdlog::info("Dropping card into the same column - do nothing");
                    return;
                }
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = inProgressCards[cardPayload->sourceIndex];
                inProgressCards.erase(inProgressCards.begin() + cardPayload->sourceIndex);

                if (id == todoDropZoneId_)
                {
                    card.status = CardStatus::Todo;
                    todoCards.push_back(card);
                } else if (id == doneDropZoneId_)
                {
                    card.status = CardStatus::Done;
                    doneCards.push_back(card);
                } else
                {
                    spdlog::error("Invalid destination column for card drop");
                }

                app_->db().updateCard(card);
            }

            // Handle Card Drop from DONE
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DONE_CARD"))
            {
                if (id == doneDropZoneId_)
                {
                    spdlog::info("Dropping card into the same column - do nothing");
                    return;
                }
                auto *cardPayload = static_cast<DragDropPayload *>(payload->Data);
                TodoCard card = doneCards[cardPayload->sourceIndex];
                doneCards.erase(doneCards.begin() + cardPayload->sourceIndex);

                if (id == todoDropZoneId_)
                {
                    card.status = CardStatus::Todo;
                    todoCards.push_back(card);
                } else if (id == inProgressDropZoneId_)
                {
                    card.status = CardStatus::InProgress;
                    inProgressCards.push_back(card);
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

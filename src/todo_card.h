//
// Created by Johnny Gonzales on 8/26/25.
//

#pragma once
#include <string>
#include "glm/vec4.hpp"


namespace todo {
    enum class CardStatus
    {
        Todo = 0,
        InProgress = 1,
        Done = 2,
        // ...add more statuses as needed
    };

    struct TodoCard
    {
        int id;
        std::string title;
        std::string description;
        CardStatus status;
        bool completed;
        glm::vec4 color{};

        TodoCard(const int id, std::string title, std::string desc, const CardStatus status, const bool completed = false)
            : id(id), title(std::move(title)), description(std::move(desc)), status(status)
              , completed(completed)
        {
            // Assign different colors based on completion status
            color = completed ? glm::vec4(0.2f, 0.8f, 0.2f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
        }
    };

    inline int statusToInt(CardStatus status)
    {
        return static_cast<int>(status);
    }

    inline CardStatus intToStatus(const int i)
    {
        switch (i)
        {
            case 0: return CardStatus::Todo;
            case 1: return CardStatus::InProgress;
            case 2: return CardStatus::Done;
            default: return CardStatus::Todo; // or throw, or a special Unknown value
        }
    }
}

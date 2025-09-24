//
// Created by Johnny Gonzales on 8/26/25.
//

#pragma once
#include <string>
#include "glm/vec4.hpp"
#include <chrono>
#include <iomanip>
#include <utility>


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
        int sequence;
        int projectId;
        std::string createdAt;
        std::string completedAt;
        glm::vec4 color{};

        // Default constructor
        TodoCard() : id(-1), status(CardStatus::Todo), sequence(-1), projectId(0)
        {
            color = glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
        }

        TodoCard(const int _id, std::string _title, std::string _desc, const CardStatus _status,
                 const int _sequence = -1, const int _projectId = 0, std::string _createdAt = "",
                 std::string _completedAt = "")
            : id(_id), title(std::move(_title)), description(std::move(_desc)), status(_status)
              , sequence(_sequence), projectId(_projectId),
              createdAt(std::move(_createdAt)), completedAt(std::move(_completedAt))
        {
            // Assign different colors based on completion status
            color = glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
        }
    };

    // Convert std::chrono::system_clock::time_point to string
    inline std::__iom_t10<char> timePointToString(const std::chrono::system_clock::time_point &tp)
    {
        auto in_time_t = std::chrono::system_clock::to_time_t(tp);
        return std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    }

    // Convert string to std::chrono::system_clock::time_point
    inline std::chrono::system_clock::time_point stringToTimePoint(const std::string &str)
    {
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }


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

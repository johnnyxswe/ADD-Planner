
#pragma once
#include <string>
#include "glm/vec4.hpp"
#include <chrono>
#include <iomanip>
#include <utility>


namespace proj {

    enum class ProjectStatus
    {
        INACTIVE = 0,
        ACTIVE = 1,
        ARCHIVED = 2,
        // ...add more statuses as needed
    };

    constexpr std::array<const char*, 3> projectStatusNames = { "INACTIVE", "ACTIVE", "ARCHIVED" };

    struct Project
    {
        int id;
        std::string name;
        ProjectStatus status;
        std::string createdAt;

        // Default constructor
        Project() : id(0), status(ProjectStatus::INACTIVE)
        {
            name = "";
            createdAt = "";
        }

        Project(const int _id, std::string _name, ProjectStatus _status, std::string _createdAt)
        {
            id = _id;
            name = std::move(_name);
            status = _status;
            createdAt = std::move(_createdAt);
        }
    };

    inline int statusToInt(ProjectStatus status)
    {
        return static_cast<int>(status);
    }

    inline ProjectStatus intToStatus(const int i)
    {
        switch (i)
        {
            case 0: return ProjectStatus::INACTIVE;
            case 1: return ProjectStatus::ACTIVE;
            case 2: return ProjectStatus::ARCHIVED;
            default: return ProjectStatus::INACTIVE; // or throw, or a special Unknown value
        }
    }
}

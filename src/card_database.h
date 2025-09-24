//
// Created by Johnny Gonzales on 8/25/25.
//

#pragma once
#include <vector>

#include "imgui_renderer.h"
#include "project.h"
#include "sqlite3.h"

namespace todo {
    class CardDatabase
    {
    public:
        void createTablesIfNotExist();
        void updateDatabaseSchema();
        int getDatabaseVersion();
        void migrateDatabaseToVersion(int targetVersion);
        bool addProject(const std::string &projectName, const int &projectStatus);
        explicit CardDatabase(const std::string& dbPath);
        ~CardDatabase();

        bool addCard(const std::string &title, const std::string &desc, const int &status, int sequence, const int &project) const;
        bool updateCard(TodoCard& card) const;
        [[nodiscard]] bool removeCard(int cardId) const;
        std::vector<proj::Project> getAllProjects() const;
        std::vector<TodoCard> getAllCards() const;

        void updateSequence(int card_id, int new_sequence) const;
        void reorderCards(int from_index, int to_index) const;

    private:
        sqlite3* db_ = nullptr;

    };
}

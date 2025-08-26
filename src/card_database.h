//
// Created by Johnny Gonzales on 8/25/25.
//

#pragma once
#include <vector>

#include "sqlite3.h"

namespace todo {
    struct TodoCard;


    class CardDatabase
    {
    public:
        explicit CardDatabase(const std::string& dbPath);
        ~CardDatabase();

        bool addCard(const std::string &title, const std::string &desc, const std::string &status, bool completed);
        bool updateCard(TodoCard& card);
        [[nodiscard]] bool removeCard(int cardId) const;
        std::vector<TodoCard> getAllCards() const;
        // ... other queries

    private:
        sqlite3* db_ = nullptr;

    };
}

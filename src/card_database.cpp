//
// Created by Johnny Gonzales on 8/25/25.
//

#include "card_database.h"

#include <objc/objc.h>

#include "imgui_renderer.h"
#include "todo_card.h"

namespace todo {
    CardDatabase::CardDatabase(const std::string &dbPath)
    {
        if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
            throw std::runtime_error("Failed to open database");

        const char *schema = "CREATE TABLE IF NOT EXISTS cards ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "title TEXT, description TEXT, status INTEGER, completed INTEGER);";
        char *err = nullptr;
        if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK)
        {
            std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error("DB schema error: " + msg);
        }
    }

    CardDatabase::~CardDatabase()
    {
        // Close Connectins
        if (db_) sqlite3_close(db_);
    }

    bool CardDatabase::addCard(const std::string &title, const std::string &desc, const std::string &status,
                               bool completed)
    {
        const char *sql = "INSERT INTO cards (title, description, status, completed) VALUES (?, ?, ?, ?);";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, statusToInt(CardStatus::Todo));
        sqlite3_bind_int(stmt, 4, completed ? 1 : 0);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }


    bool CardDatabase::updateCard(TodoCard &card)
    {
        const char *sql = "UPDATE cards SET title = ?, description = ?, status = ?, completed = ? WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, card.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, card.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, statusToInt(card.status));
        sqlite3_bind_int(stmt, 4, card.completed ? 1 : 0);
        sqlite3_bind_int(stmt, 5, card.id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }


    bool CardDatabase::removeCard(int cardId) const
    {
        const char *sql = "DELETE FROM cards WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, cardId);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }


    std::vector<TodoCard> CardDatabase::getAllCards() const
    {
        std::vector<TodoCard> cards;
        const char *sql = "SELECT id, title, description, status, completed FROM cards;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return cards;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            TodoCard card(
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
                intToStatus(sqlite3_column_int(stmt, 3)),
                sqlite3_column_int(stmt, 4) != 0
            );
            cards.push_back(card);
        }
        sqlite3_finalize(stmt);
        return cards;
    }
}

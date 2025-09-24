//
// Created by Johnny Gonzales on 8/25/25.
//

#include "card_database.h"

#include <objc/objc.h>

#include "imgui_renderer.h"
#include "project.h"
#include "todo_card.h"
#include "spdlog/spdlog.h"

namespace todo {
    void CardDatabase::createTablesIfNotExist()
    {
        // Create Cards Table
        const char *schema = "CREATE TABLE IF NOT EXISTS cards ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "title TEXT NOT NULL,"
                "description TEXT,"
                "status INTEGER DEFAULT 0,"
                "sequence INTEGER NOT NULL DEFAULT 0,"
                "project INTEGER DEFAULT 0,"
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                "completed_at TIMESTAMP NULL);";
        char *err = nullptr;
        if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK)
        {
            const std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error("DB schema error: " + msg);
        }

        const char *sql = "CREATE INDEX IF NOT EXISTS idx_cards_sequence ON cards(sequence);";
        err = nullptr;
        if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK)
        {
            std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error("DB index error: " + msg);
        }

        // Create Projects Table
        schema = "CREATE TABLE IF NOT EXISTS projects ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL,"
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
        err = nullptr;
        if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK)
        {
            const std::string msg = err;
            sqlite3_free(err);
            throw std::runtime_error("DB schema error: " + msg);
        }
    }

    void CardDatabase::updateDatabaseSchema()
    {
        // Check current database version
        int currentVersion = getDatabaseVersion();
        const int TARGET_VERSION = 4; // Increment when you make schema changes

        if (currentVersion < TARGET_VERSION)
        {
            // Apply migrations
            migrateDatabaseToVersion(TARGET_VERSION);
        }
    }

    int CardDatabase::getDatabaseVersion()
    {
        // Store version in a metadata table
        const char *createVersionTable = R"(
            CREATE TABLE IF NOT EXISTS app_metadata (
                key TEXT PRIMARY KEY,
                value TEXT
            );
        )";
        sqlite3_exec(db_, createVersionTable, nullptr, nullptr, nullptr);

        sqlite3_stmt *stmt;
        const char *sql = "SELECT value FROM app_metadata WHERE key = 'db_version'";

        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                int version = std::stoi((char *) sqlite3_column_text(stmt, 0));
                sqlite3_finalize(stmt);
                return version;
            }
        }
        sqlite3_finalize(stmt);
        return 0; // Default version for new databases
    }

    void CardDatabase::migrateDatabaseToVersion(int targetVersion)
    {
        // Apply migrations based on target version
        spdlog::info("Migrating database to version {}", targetVersion);
        if (targetVersion >= 1)
        {
            spdlog::info("Migration in progress...");
            // Example migration: Add new column
            // v2 & v3
            // sqlite3_exec(db_, "ALTER TABLE cards ALTER COLUMN project INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
            // sqlite3_exec(db_, "ALTER TABLE projects ADD COLUMN status INTEGER DEFAULT 0", nullptr, nullptr, nullptr);

            // v4
            sqlite3_exec(db_, "ALTER TABLE cards ADD COLUMN completed_at TIMESTAMP NULL", nullptr, nullptr, nullptr);
        }

        // Update version
        const char *updateVersionSQL = R"(
            INSERT OR REPLACE INTO app_metadata (key, value)
            VALUES ('db_version', ?);
        )";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db_, updateVersionSQL, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, std::to_string(targetVersion).c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);

        spdlog::info("Migration complete!");
    }

    bool CardDatabase::addProject(const std::string &projectName, const int &projectStatus)
    {
        // SQL query to fetch the next sequence value
        const char *sql = "INSERT INTO projects (name, status) VALUES (?, ?);";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return false;
        }
        sqlite3_bind_text(stmt, 1, projectName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, projectStatus);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    CardDatabase::CardDatabase(const std::string &dbPath)
    {
        if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
            throw std::runtime_error("Failed to open database");

        // Check if tables exist, create if they don't
        createTablesIfNotExist();

        // Handle database migrations/updates
        updateDatabaseSchema();
    }

    CardDatabase::~CardDatabase()
    {
        // Close Connectins
        if (db_) sqlite3_close(db_);
    }

    bool CardDatabase::addCard(const std::string &title, const std::string &desc, const int &status,
                               const int sequence, const int &project) const
    {
        // SQL query to fetch the next sequence value
        const char *selectSql = "SELECT MAX(sequence) + 1 AS next_sequence FROM cards";

        sqlite3_stmt *selectStmt = nullptr;
        if (sqlite3_prepare_v2(db_, selectSql, -1, &selectStmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return false;
        }

        // Execute the SELECT statement
        int nextSequence = -1;
        while (sqlite3_step(selectStmt) == SQLITE_ROW)
        {
            nextSequence = sqlite3_column_int(selectStmt, 0);
        }

        // SQL query with a CTE and INSERT statement
        const char *sql = "INSERT INTO cards (title, description, status, sequence, project) VALUES (?, ?, ?, ?, ?);";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return false;
        }
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, status);
        sqlite3_bind_int(stmt, 4, nextSequence);
        sqlite3_bind_int(stmt, 5, project);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }


    bool CardDatabase::updateCard(TodoCard &card) const
    {
        const char *sql =
                "UPDATE cards SET title = ?, description = ?, status = ?, "
                "sequence = ?, project = ?, completed_at = ?"
                "WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return false;
        }
        // INSERT params
        sqlite3_bind_text(stmt, 1, card.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, card.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, todo::statusToInt(card.status));
        sqlite3_bind_int(stmt, 4, card.sequence);
        sqlite3_bind_int(stmt, 5, card.projectId);
        sqlite3_bind_text(stmt, 6, card.completedAt.c_str(), -1, SQLITE_TRANSIENT);

        // WHERE clause
        sqlite3_bind_int(stmt, 7, card.id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    bool CardDatabase::removeCard(int cardId) const
    {
        const char *sql = "DELETE FROM cards WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return false;
        }
        sqlite3_bind_int(stmt, 1, cardId);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    std::vector<proj::Project> CardDatabase::getAllProjects() const
    {
        std::vector<proj::Project> projects = {};
        const char *sql =
                "SELECT id, name, created_at, status FROM projects ORDER BY id ASC;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return projects;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            proj::Project project(
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
                proj::intToStatus(sqlite3_column_int(stmt, 2)),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3))
            );
            projects.push_back(project);
        }
        sqlite3_finalize(stmt);
        return projects;
    }

    std::vector<TodoCard> CardDatabase::getAllCards() const
    {
        std::vector<TodoCard> cards;
        const char *sql =
                "SELECT id, title, description, status, sequence, IFNULL(project,0), CAST(created_at AS TEXT), IFNULL(CAST(completed_at AS TEXT), '')"
                " FROM cards ORDER BY sequence ASC;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            spdlog::error("SQL error: {}", sqlite3_errmsg(db_));
            return cards;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            TodoCard card(
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
                intToStatus(sqlite3_column_int(stmt, 3)),
                sqlite3_column_int(stmt, 4),
                sqlite3_column_int(stmt, 5),
                (sqlite3_column_text(stmt, 6)),
                (sqlite3_column_text(stmt, 7))
            );
            cards.push_back(card);
        }
        sqlite3_finalize(stmt);
        return cards;
    }

    void CardDatabase::updateSequence(int card_id, int new_sequence) const
    {
        const char *sql = "UPDATE cards SET sequence = ? WHERE id = ?";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, new_sequence);
            sqlite3_bind_int(stmt, 2, card_id);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    void CardDatabase::reorderCards(int from_index, int to_index) const
    {
        auto cards = getAllCards();

        if (from_index == to_index || from_index < 0 || to_index < 0 ||
            from_index >= cards.size() || to_index >= cards.size())
        {
            return;
        }

        sqlite3_exec(db_, "BEGIN TRANSACTION", NULL, NULL, NULL);

        try
        {
            // Update sequences for affected cards
            if (from_index < to_index)
            {
                // Moving down: shift cards up
                for (int i = from_index + 1; i <= to_index; i++)
                {
                    cards[i].sequence--;
                    updateSequence(cards[i].id, cards[i].sequence);
                }
            } else
            {
                // Moving up: shift cards down
                for (int i = to_index; i < from_index; i++)
                {
                    cards[i].sequence++;
                    updateSequence(cards[i].id, cards[i].sequence);
                }
            }

            // Update the moved card
            cards[from_index].sequence = to_index;
            updateSequence(cards[from_index].id, to_index);

            // Update local vector
            TodoCard temp = cards[from_index];
            cards.erase(cards.begin() + from_index);
            cards.insert(cards.begin() + to_index, temp);

            sqlite3_exec(db_, "COMMIT", NULL, NULL, NULL);
        } catch (char **e)
        {
            sqlite3_exec(db_, "ROLLBACK", NULL, NULL, e);
            spdlog::error("Error: {}", *e);
            throw;
        }
    }
}

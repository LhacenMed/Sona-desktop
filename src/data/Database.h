#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

// The app's data that isn't a preference (bookmarks, and later the library, playlists and recents),
// in one SQLite file: %LOCALAPPDATA%\Sona\sona.db. Opening it brings its tables up to date with the
// migrations in Database.cpp. It's used on the UI thread only.
class Database
{
public:
    // A prepared SQL statement: bind its ? parameters (from 1), then step through its rows.
    class Statement
    {
    public:
        Statement(sqlite3 *connection, std::string_view sql);
        ~Statement();
        Statement(const Statement &) = delete;
        Statement &operator=(const Statement &) = delete;

        Statement &bind(int index, std::wstring_view text);
        Statement &bind(int index, int64_t value);
        // Runs the statement to its next row; false once there are no more.
        bool step();
        std::wstring text(int column) const;
        int64_t integer(int column) const;

    private:
        sqlite3 *m_connection;
        sqlite3_stmt *m_statement = nullptr;
    };

    Database();
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    Statement prepare(std::string_view sql) const { return Statement(m_connection, sql); }

private:
    sqlite3 *m_connection = nullptr;
};

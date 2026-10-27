#include "data/Database.h"

#include <sqlite3.h>

#include <filesystem>
#include <format>
#include <iterator>
#include <stdexcept>

#include "platform/KnownFolders.h"

namespace {

// Each migration takes the database from version i (its PRAGMA user_version) to i + 1. A new table
// or column is a new entry at the end; released entries never change.
constexpr const char *migrations[] = {
    // Bookmarks: a named time in a media, by its location. Times are in milliseconds.
    "CREATE TABLE bookmarks (id INTEGER PRIMARY KEY, media TEXT NOT NULL, name TEXT NOT NULL, time INTEGER NOT NULL);"
    "CREATE INDEX bookmarks_media ON bookmarks (media);",
    // History: each media played, where it was left (milliseconds) and how often it played; when it
    // last played in seconds since 1970.
    "CREATE TABLE history (location TEXT PRIMARY KEY, title TEXT NOT NULL, artist TEXT NOT NULL, audio INTEGER NOT NULL,"
    " position INTEGER NOT NULL, duration INTEGER NOT NULL, last_played INTEGER NOT NULL, play_count INTEGER NOT NULL);",
    // Presets: the preferences panel's values under a name, as "key=value" lines; and the preset each
    // folder plays with.
    "CREATE TABLE presets (name TEXT PRIMARY KEY, preferences TEXT NOT NULL);"
    "CREATE TABLE folder_presets (folder TEXT PRIMARY KEY, preset TEXT NOT NULL);",
};

[[noreturn]] void fail(sqlite3 *connection)
{
    throw std::runtime_error(std::string("The app's data could not be read: ") + sqlite3_errmsg(connection));
}

void execute(sqlite3 *connection, const std::string &sql)
{
    if (sqlite3_exec(connection, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
        fail(connection);
}

} // namespace

Database::Statement::Statement(sqlite3 *connection, std::string_view sql)
    : m_connection(connection)
{
    if (sqlite3_prepare_v2(connection, sql.data(), static_cast<int>(sql.size()), &m_statement, nullptr) != SQLITE_OK)
        fail(connection);
}

Database::Statement::~Statement()
{
    sqlite3_finalize(m_statement);
}

Database::Statement &Database::Statement::bind(int index, std::wstring_view text)
{
    sqlite3_bind_text16(m_statement, index, text.data(), static_cast<int>(text.size() * sizeof(wchar_t)), SQLITE_TRANSIENT);
    return *this;
}

Database::Statement &Database::Statement::bind(int index, int64_t value)
{
    sqlite3_bind_int64(m_statement, index, value);
    return *this;
}

bool Database::Statement::step()
{
    switch (sqlite3_step(m_statement)) {
    case SQLITE_ROW:
        return true;
    case SQLITE_DONE:
        return false;
    default:
        fail(m_connection);
    }
}

std::wstring Database::Statement::text(int column) const
{
    const auto *text = static_cast<const wchar_t *>(sqlite3_column_text16(m_statement, column));
    return std::wstring(text, sqlite3_column_bytes16(m_statement, column) / sizeof(wchar_t));
}

int64_t Database::Statement::integer(int column) const
{
    return sqlite3_column_int64(m_statement, column);
}

Database::Database()
{
    const std::filesystem::path folder = std::filesystem::path(knownFolderPath(FOLDERID_LocalAppData)) / SONA_APP_NAME;
    std::filesystem::create_directories(folder);
    if (sqlite3_open16((folder / L"sona.db").c_str(), &m_connection) != SQLITE_OK)
        fail(m_connection);

    const int current = [this] {
        Statement query = prepare("PRAGMA user_version");
        query.step();
        return static_cast<int>(query.integer(0));
    }();
    for (int version = current; version < static_cast<int>(std::size(migrations)); ++version)
        execute(m_connection, std::format("BEGIN; {} PRAGMA user_version = {}; COMMIT;", migrations[version], version + 1));
}

Database::~Database()
{
    sqlite3_close(m_connection);
}

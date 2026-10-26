# SQLite 3.54.1, the amalgamation from https://sqlite.org/download.html, unchanged. It's the app's data
# store (data/Database). To update it, replace sqlite3.c and sqlite3.h and the version above.
add_library(sqlite3 STATIC sqlite3.c)
target_include_directories(sqlite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# The options sqlite.org recommends for an app that uses SQLite directly.
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_DQS=0
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_LOAD_EXTENSION
)
# Third-party code: its warnings aren't ours to fix.
target_compile_options(sqlite3 PRIVATE /W0)

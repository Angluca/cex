#define CEX_IMPLEMENTATION
#include "cex.h"
#include "lib/sqlite3.h" // This will be dowloaded automatically at first ./cex run

Exception
sqlite_hello_world(const char* db_name)
{
    Exc result = Error.runtime;
    char* err_msg = NULL;

    // This is using global heap allocator (mem$) and will be free'd at end: label
    char* db_path = os$path_join(mem$, "build", db_name);
    io.printf("db_path: '%s'\n", db_path);


    sqlite3* db;
    // NOTE: e$except_true() produces handy error tracebacks like this:
    //[ERROR]   ( main.c:16 sqlite_hello_world() ) `sqlite3_open(db_path, &db)` returned non zero
    //    Cannot open database [build/bar.db]: unable to open database file
    e$except_true(sqlite3_open(db_path, &db))
    {
        io.printf("\tCannot open database [%s]: %s\n", db_path, sqlite3_errmsg(db));
        goto end;
    }

    const char* create_table_sql = "CREATE TABLE IF NOT EXISTS Messages ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "content TEXT NOT NULL);";
    e$except_true(sqlite3_exec(db, create_table_sql, 0, 0, &err_msg))
    {
        io.printf("\tSQL error: %s\n", err_msg);
        goto end;
    }

    const char* insert_sql = "INSERT INTO Messages (content) VALUES ('Hello, from CEX!');";
    e$except_true(sqlite3_exec(db, insert_sql, 0, 0, &err_msg))
    {
        io.printf("\tSQL error: %s\n", err_msg);
        goto end;
    }

    io.printf("Table '%s' created successfully!\n", db_path);
    io.printf("For test run: ./build/sqlite3 %s \"SELECT * FROM Messages;\"\n", db_path);

    result = EOK;
end:
    if (err_msg != NULL) { sqlite3_free(err_msg); }
    if (db_path) { mem$free(mem$, db_path); }
    sqlite3_close(db);
    return result;
}

int
main(int argc, char** argv)
{
    argparse_c arg = { 0 };
    e$except (err, argparse.parse(&arg, argc, argv)) { return 1; }
    const char* db_name = argparse.next(&arg);
    if (db_name == NULL) {
        io.printf("Usage: %s db_name\n", arg.program_name);
        return 1;
    }

    e$except (err, sqlite_hello_world(db_name)) { return 1; }
    return 0;
}

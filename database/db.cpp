#include "db.hpp"

static const char *createTableSQL = R"(
    CREATE TABLE IF NOT EXISTS cards (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	due DATETIME,
	stability REAL,
	difficulty REAL,
	elapsedDays INTEGER,
	scheduledDays INTEGER,
	reps INTEGER,
	lapses INTEGER,
	state TEXT,
	lastReview DATETIME,
	question TEXT NOT NULL,
	answer TEXT NOT NULL
    );
)";

// Callback function to handle results from SQL execution
static int callback(void *data, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << std::endl;
    return 0;
}

bool openDB(sqlite3 **db, const char *dbFile, int &rc)
{
    rc = sqlite3_open(dbFile, db);
    if (rc) {
	std::cerr << "Can't open database " << sqlite3_errmsg(*db) << std::endl;
	return false;
    }

    std::cout << "Opened database " << dbFile << std::endl;
    return true;
}

bool createDBTables(sqlite3 *db, char **errMsg, int &rc)
{
    rc = sqlite3_exec(db, createTableSQL, callback, 0, errMsg);    

    if (rc != SQLITE_OK) {
	std::cerr << "SQL Error: " << *errMsg << std::endl;
	sqlite3_free(*errMsg);
	return false;
    }

    std::cout << "Table created or already exists." << std::endl;

    return true;
}

void closeDB(sqlite3 *db)
{
    sqlite3_close(db);
    std::cout << "Closed database." << std::endl;
}

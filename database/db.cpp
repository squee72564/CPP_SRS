#include "db.hpp"


// Callback function to handle results from SQL execution
static int callback(void *data, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << std::endl;
    return 0;
}

bool openDB(sqlite3 **db, const char *dbFile)
{
    int rc = sqlite3_open(dbFile, db);
    if (rc) {
        std::cerr << "Can't open database " << sqlite3_errmsg(*db) << std::endl;
        return false;
    }

    std::cout << "Opened database " << dbFile << std::endl;
    return true;
}

bool createDBTables(sqlite3 *db, char **errMsg)
{
    const char *createTableQuery = R"(
	CREATE TABLE IF NOT EXISTS cards (
	    id INTEGER PRIMARY KEY AUTOINCREMENT,
	    due DATETIME NOT NULL,
	    stability TEXT NOT NULL,
	    difficulty TEXT NOT NULL,
	    elapsedDays TEXT NOT NULL,
	    scheduledDays TEXT NOT NULL,
	    reps TEXT NOT NULL,
	    lapses TEXT NOT NULL,
	    state TEXT NOT NULL,
	    lastReview DATETIME,
	    question TEXT NOT NULL,
	    answer TEXT NOT NULL
	);
    )";

    int rc = sqlite3_exec(db, createTableQuery, callback, 0, errMsg);    

    if (rc != SQLITE_OK) {
        std::cerr << "SQL Error: " << *errMsg << std::endl;
        sqlite3_free(*errMsg);
        return false;
    }

    std::cout << "Card table created or already exists." << std::endl;

    return true;
}

bool pullCard(sqlite3 *db, FlashCard **flash_card) {

    // Format current time into string for SQL query
    time_t now_t = time(nullptr);
    std::tm now_tm = *gmtime(&now_t);
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");
    std::string curr_time_str = oss.str();

    const char *pullQuery = R"(
    	SELECT * FROM cards
        WHERE julianday(due) <= julianday(?)
        ORDER BY ABS(julianday(due) - julianday(?))
        ASC LIMIT 1;
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, pullQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare sql: " << sqlite3_errmsg(db) << std::endl;
        return false;;
    }

    sqlite3_bind_text(stmt, 1, curr_time_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, curr_time_str.c_str(), -1, SQLITE_STATIC);

    std::unordered_map<std::string, std::string> card_map;
    int uuid = 0;
    std::string question_string = "";
    std::string answer_string = "";

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        uuid = sqlite3_column_int(stmt, 0);
        card_map["due"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        card_map["stability"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        card_map["difficulty"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        card_map["elapsedDays"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        card_map["scheduledDays"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        card_map["reps"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        card_map["lapses"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        card_map["state"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        const char* lr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

        if (strcmp(lr,"NULL") != 0)
            card_map["lastReview"] = lr;

        question_string = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        answer_string = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        Card card = Card::fromMap(card_map);

        *flash_card = allocFlashCard(uuid, card, answer_string, question_string);
    }

    sqlite3_finalize(stmt);

    return true;
}

bool createCard(sqlite3 *db, char **errMsg, char* const question, char* const answer)
{
    Card new_card = Card();

    std::unordered_map<std::string, std::string> card_map = new_card.toMap();

    const char *insertQuery = R"(
        INSERT INTO cards
        (due, stability, difficulty, elapsedDays, scheduledDays,
        reps, lapses, state, lastReview, question, answer)
        VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, insertQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, card_map["due"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, card_map["stability"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, card_map["difficulty"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, card_map["elapsedDays"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, card_map["scheduledDays"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, card_map["reps"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, card_map["lapses"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, card_map["state"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, "NULL", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, question, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, answer, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
       std::cerr << "Failed to create card: " << sqlite3_errmsg(db) << std::endl;
       sqlite3_finalize(stmt);
       return false;
    }

    sqlite3_finalize(stmt);

    return true;

}

bool updateCard(sqlite3 *db, const Card& new_card, const FlashCard *flash_card)
{
    std::unordered_map<std::string, std::string> card_map = new_card.toMap();

    const char *updateQuery = R"(
	UPDATE cards
	SET due = ?, stability = ?, difficulty = ?, elapsedDays = ?,
	    scheduledDays = ?, reps = ?, lapses = ?, state = ?,
	    lastReview = ?, question = ?, answer = ?
	WHERE id = ?;
    )";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, updateQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, card_map["due"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, card_map["stability"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, card_map["difficulty"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, card_map["elapsedDays"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, card_map["scheduledDays"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, card_map["reps"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, card_map["lapses"].c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, card_map["state"].c_str(), -1, SQLITE_STATIC);

    std::string lr = (card_map.find("lastReview") == card_map.end()) ? "NULL" : card_map["lastReview"]; 
    sqlite3_bind_text(stmt, 9, lr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, flash_card->q.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, flash_card->a.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 12, flash_card->uuid);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
       std::cerr << "Failed to update row: " << sqlite3_errmsg(db) << std::endl;
       sqlite3_finalize(stmt);
       return false;
    }

    sqlite3_finalize(stmt);

    return true;
}

bool getNextCardTime(sqlite3 *db, std::string& time_str) 
{
    time_str = "";
    const char *nextCardTimeQuery = R"(SELECT due FROM cards ORDER BY due ASC LIMIT 1;)";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, nextCardTimeQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare sql: " << sqlite3_errmsg(db) << std::endl;
        return false;;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        time_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);

    return true;
}

void closeDB(sqlite3 *db)
{
    sqlite3_close(db);
    std::cout << "Closed database." << std::endl;
}

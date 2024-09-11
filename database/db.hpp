#ifndef DBMODELS_HPP
#define DBMODELS_HPP

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <ctime>
#include <sstream>
#include <cstring>

#include "FlashCard.hpp"

bool openDB(sqlite3 **db, const char *dbFile);
bool createDBTables(sqlite3 *db, char **errMsg);
bool pullCard(sqlite3 *db, FlashCard **flash_card);
bool createCard(sqlite3 *db, char **errMsg, char* const question, char* const answer);
bool updateCard(sqlite3 *db, const Card& new_card, const FlashCard *flash_card);
bool getNextCardTime(sqlite3 *db, std::string& time_str);

void closeDB(sqlite3 *db);
#endif

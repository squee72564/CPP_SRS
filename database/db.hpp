#ifndef DBMODELS_HPP
#define DBMODELS_HPP

#include <sqlite3.h>
#include <string>
#include <iostream>

bool openDB(sqlite3 **db, const char *dbFile, int &rc);
bool createDBTables(sqlite3 *db, char **errMsg, int &rc);
void closeDB(sqlite3 *db);

#endif

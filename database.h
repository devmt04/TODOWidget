#ifndef DATABASE_H
#define DATABASE_H
#include <sqlite3.h>

int prepare_database(sqlite3 **db);
int insert_into_todotable(sqlite3 **db, int id, char *string);
int fetch_all_from_table(sqlite3 **db);
int fetch_todo_count(sqlite3 **db);
void close_db(sqlite3 **db);

#endif

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"

int count_digits(int num) {
    int count = 0;
    if (num < 0) {
        num = -num;
    }
    do {
        num /= 10;
        count++;
    } while (num != 0);
    return count;
}

int fetch_all_from_table(sqlite3 **db){
	int rc;
	sqlite3_stmt *stmt;
	char *sql = "SELECT * FROM TODOTABLE";
	rc = sqlite3_prepare_v2(*db, sql, -1, &stmt, NULL);
	if( rc != SQLITE_OK){
		fprintf(stderr, "DB (error): Failed to fetch data: %s\n", sqlite3_errmsg(*db));
		sqlite3_finalize(stmt);
		return -1;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	    printf("%d | %s | %s \n", 
	           sqlite3_column_int(stmt, 0), 
	           sqlite3_column_text(stmt, 1), 
	           sqlite3_column_text(stmt, 2));
	}

	sqlite3_finalize(stmt);
}

int fetch_max_id(sqlite3 **db){
	int rc, result;
	sqlite3_stmt *stmt;
	char *sql = "SELECT MAX(ID) FROM TODOTABLE;";
	rc = sqlite3_prepare_v2(*db, sql, -1, &stmt, NULL);
	if( rc != SQLITE_OK){
		fprintf(stderr, "DB (error): Failed to fetch MAX(ID): %s\n", sqlite3_errmsg(*db));
		sqlite3_finalize(stmt);
		return -1;
	}
	if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }
	sqlite3_finalize(stmt);
	return result;
}

int fetch_todo_count(sqlite3 **db){
	int rc, result;
	sqlite3_stmt *stmt;
	char *sql = "SELECT COUNT(ID) FROM TODOTABLE";
	rc = sqlite3_prepare_v2(*db, sql, -1, &stmt, NULL);
	if( rc != SQLITE_OK){
		fprintf(stderr, "DB (error): Failed to fetch MAX(ID): %s\n", sqlite3_errmsg(*db));
		sqlite3_finalize(stmt);
		return -1;
	}
	if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }
	sqlite3_finalize(stmt);
	return result;
}

int is_table_exists(sqlite3 **db){
	sqlite3_stmt *stmt;
	const char *sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
	int result = 0;

	if(sqlite3_prepare_v2(*db, sql, -1, &stmt, NULL) == SQLITE_OK){
		sqlite3_bind_text(stmt, 1, "TODOTABLE", -1, SQLITE_STATIC);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = 1;
            printf("DB: TABLE TODO ALREAD EXISTS.\n");
        }
        sqlite3_finalize(stmt);
	}else {
        fprintf(stderr, "DB (error): Failed to check table existence: %s\n", sqlite3_errmsg(*db));
        sqlite3_finalize(stmt);
    }

    return result;
}

int insert_into_todotable(sqlite3 **db, int id, char *string){
	char *errmsg = 0;
	// char *sql = "INSERT INTO TODOTABLE VALUES(0,'Hello world', '10-12-23');"\
	// 			"INSERT INTO TODOTABLE VALUES(1,'Hello world 2 ', '11-12-23');";
	char *sql = malloc(strlen(string)+strlen("INSERT INTO TODOTABLE VALUES(,'','30-12-2050');")+count_digits(id) +1);
	
	sprintf(sql, "INSERT INTO TODOTABLE VALUES(%d,'%s','10-12-23');", id, string);

	if(sqlite3_exec(*db, sql, NULL, 0, &errmsg) != SQLITE_OK){
		fprintf(stderr, "DB (error): SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return 0;
	}else {
        fprintf(stderr, "DB: Records created successfully\n");
    }
}


void close_db(sqlite3 **db){
	sqlite3_close(*db);
}

int prepare_database(sqlite3 **db){
	int rc = sqlite3_open("data.db", db);

	if(rc){
		fprintf(stderr, "DB (error): Can't open database: %s\n", sqlite3_errmsg(*db));
		return 0;
	}else{
		printf("DB: Opened database successfully\n");
	}


	char *errmsg = 0;

	if(!is_table_exists(db)){
		const char *sql = "CREATE TABLE TODOTABLE( "\
						"ID INT PRIMARY KEY NOT NULL," \
						"DATA CHAR(100) NOT NULL, " \
						"DATE TEXT NOT NULL);";

		rc = sqlite3_exec(*db, sql, NULL, NULL,  &errmsg);
		if(rc != SQLITE_OK){
			fprintf(stderr, "DB (error): SQL error: %s\n", errmsg);
	    	sqlite3_free(errmsg);
		}else{
			fprintf(stderr, "DB: Table created successfully\n");
		}
		return 0; 
	}else{
		fetch_all_from_table(db);
		return fetch_max_id(db)+1;
	}

}


/*
	char *errmsg = 0;
	int rc = sqlite3_open("data.db", &db);

	if (rc){
	    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	    return(0);
	} else fprintf(stderr, "Opened database successfully\n");
	
	char *sql = "CREATE TABLE TODOTABLE( "\
					"ID INT PRIMARY KEY NOT NULL," \
					"DATA CHAR(100) NOT NULL, " \
					"DATE TEXT NOT NULL);"; 

	rc = sqlite3_exec(db, sql, NULL,0,  &errmsg);
	if(rc!=SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", errmsg);
    	sqlite3_free(errmsg);
	}else{
		fprintf(stderr, "Table created successfully\n");
	}

	const char cmd = "SELECT * FROM TODOTABLE";
	sqlite3_stmt *stmt;
	rc = 

*/
#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>

// Database connection config
#define DB_HOST     "localhost"
#define DB_USER     "root"
#define DB_PASS     "123456"  
#define DB_NAME     "meeting_db"
#define DB_PORT     3306

// Initialize database connection
MYSQL* db_connect();

// Close connection
void db_close(MYSQL* conn);

// Execute query và trả về result
MYSQL_RES* db_query(MYSQL* conn, const char* query);

// Execute insert/update/delete
int db_execute(MYSQL* conn, const char* query);

// Escape string để tránh SQL injection
char* db_escape_string(MYSQL* conn, const char* str);

#endif

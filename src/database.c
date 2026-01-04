#include "database.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// ============= CONNECT =============
MYSQL* db_connect() {
    MYSQL* conn = mysql_init(NULL);
    
    if (!conn) {
        log_message("ERROR", "mysql_init() failed");
        return NULL;
    }
    
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
        log_message("ERROR", "MySQL connection failed: %s", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }
    
    log_message("INFO", "MySQL connected successfully");
    return conn;
}

// ============= CLOSE =============
void db_close(MYSQL* conn) {
    if (conn) {
        mysql_close(conn);
        log_message("INFO", "MySQL connection closed");
    }
}

// ============= QUERY =============
MYSQL_RES* db_query(MYSQL* conn, const char* query) {
    if (!conn) {
        log_message("ERROR", "db_query: connection is NULL");
        return NULL;
    }
    
    if (!query) {
        log_message("ERROR", "db_query: query is NULL");
        return NULL;
    }
    
    if (mysql_query(conn, query)) {
        log_message("ERROR", "Query failed: %s", mysql_error(conn));
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    
    if (!result) {
        if (mysql_field_count(conn) > 0) {
            log_message("ERROR", "mysql_store_result() failed: %s", mysql_error(conn));
            return NULL;
        }
    }
    
    return result;
}

// ============= EXECUTE =============
int db_execute(MYSQL* conn, const char* query) {
    if (!conn) {
        log_message("ERROR", "db_execute: connection is NULL");
        return -1;
    }
    
    if (!query) {
        log_message("ERROR", "db_execute: query is NULL");
        return -1;
    }
    
    if (mysql_query(conn, query)) {
        log_message("ERROR", "Execute failed: %s", mysql_error(conn));
        return -1;
    }
    
    int affected = mysql_affected_rows(conn);
    
    return affected;
}

// ============= ESCAPE STRING =============
char* db_escape_string(MYSQL* conn, const char* str) {
    if (!conn || !str) return NULL;
    
    size_t len = strlen(str);
    char* escaped = malloc(len * 2 + 1);
    
    mysql_real_escape_string(conn, escaped, str, len);
    return escaped;
}
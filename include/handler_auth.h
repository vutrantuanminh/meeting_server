#ifndef HANDLER_AUTH_H
#define HANDLER_AUTH_H

#include "protocol.h"
#include <mysql/mysql.h>

// REGISTER: username||password
Response* handle_register(Request* req, MYSQL* db_conn);

// LOGIN: username&password
Response* handle_login(Request* req, MYSQL* db_conn);

// LOGOUT (cần token)
Response* handle_logout(Request* req, MYSQL* db_conn);

#endif

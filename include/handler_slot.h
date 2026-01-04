#ifndef HANDLER_SLOT_H
#define HANDLER_SLOT_H

#include "protocol.h"
#include <mysql/mysql.h>

Response *handle_add_slot(Request *req, MYSQL *db_conn);
Response *handle_update_slot(Request *req, MYSQL *db_conn);
Response *handle_delete_slot(Request *req, MYSQL *db_conn);
Response *handle_list_free_slots(Request *req, MYSQL *db_conn);
Response *handle_list_my_slots(Request *req, MYSQL *db_conn);
Response *handle_list_students(Request *req, MYSQL *db_conn);

#endif
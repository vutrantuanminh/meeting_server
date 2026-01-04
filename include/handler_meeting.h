#ifndef HANDLER_MEETING_H
#define HANDLER_MEETING_H

#include "protocol.h"
#include <mysql/mysql.h>

// BOOK_INDIVIDUAL: teacher_id&slot_id
Response* handle_book_individual(Request* req, MYSQL* db_conn);

// BOOK_GROUP: teacher_id&slot_id&member_id|member_id|...
Response* handle_book_group(Request* req, MYSQL* db_conn);

// CANCEL_MEETING: meeting_id
Response* handle_cancel_meeting(Request* req, MYSQL* db_conn);

// LIST_MEETINGS: date|week
Response* handle_list_meetings(Request* req, MYSQL* db_conn);

// LIST_APPOINTMENTS: date|week (teacher only)
Response* handle_list_appointments(Request* req, MYSQL* db_conn);

// ADD_MINUTES: meeting_id||<base64_content>
Response* handle_add_minutes(Request* req, MYSQL* db_conn);

// GET_MINUTES: meeting_id
Response* handle_get_minutes(Request* req, MYSQL* db_conn);

// VIEW_HISTORY: student_id (teacher only)
Response* handle_view_history(Request* req, MYSQL* db_conn);

#endif

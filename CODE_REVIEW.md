# Meeting Server - Code Review Document

## 📁 Cấu trúc dự án

```
meeting_server/
├── src/                    # Server source code
├── include/                # Server headers
├── client/                 # Client application
│   ├── src/                # Client source code
│   └── include/            # Client headers
├── minutes/                # Meeting minutes files
└── setup_test_data.sql     # Test data
```

---

## 🖥️ SERVER (`/src/`)

### `server.c` - Entry point & Request Router
**Chức năng:** Khởi động server, accept connections, route requests

**Quan trọng:**
```c
// Fork process cho mỗi client
pid_t pid = fork();
if (pid == 0) {
    handle_client(client_fd, child_db);  // Child process
}

// Route command đến handler tương ứng
if (strcmp(req->command, "LOGIN") == 0) {
    return handle_login(req, db_conn);
}
```

---

### `handler_auth.c` - Authentication
| Function | Chức năng |
|----------|-----------|
| `handle_login()` | Xác thực user, trả về JWT token |
| `handle_register()` | Đăng ký user mới |
| `handle_logout()` | Invalidate session |

**Quan trọng - Password hashing:**
```c
// SHA256 hash password
unsigned char hash[SHA256_DIGEST_LENGTH];
SHA256((unsigned char*)password, strlen(password), hash);
```

**Quan trọng - JWT Token:**
```c
// Generate token: base64(user_id|role|expiry|signature)
char token_data[256];
snprintf(token_data, sizeof(token_data), "%d|%s|%ld", user_id, role, expiry);
```

---

### `handler_slot.c` - Slot Management
| Function | Chức năng |
|----------|-----------|
| `handle_add_slot()` | Teacher thêm slot mới |
| `handle_update_slot()` | Sửa thời gian slot |
| `handle_delete_slot()` | Xóa slot (chỉ khi chưa book) |
| `handle_list_free_slots()` | List slots trống cho student |
| `handle_list_my_slots()` | List slots của teacher |
| `handle_list_students()` | List students có history với teacher |
| `handle_list_all_students()` | List tất cả students (cho book group) |

**Quan trọng - Slot Type Query:**
```sql
-- Hiển thị loại slot: Individual/Group/Both
CASE s.slot_type WHEN 0 THEN 'Individual' WHEN 1 THEN 'Group' ELSE 'Both' END
```

---

### `handler_meeting.c` - Meeting Management
| Function | Chức năng |
|----------|-----------|
| `handle_book_individual()` | Book meeting cá nhân |
| `handle_book_group()` | Book meeting nhóm |
| `handle_cancel_meeting()` | Hủy meeting |
| `handle_list_meetings()` | List meetings của student |
| `handle_list_appointments()` | List appointments của teacher |
| `handle_add_minutes()` | Thêm/sửa biên bản |
| `handle_get_minutes()` | Lấy biên bản |
| `handle_view_history()` | Xem lịch sử meetings |

**Quan trọng - Include group members:**
```sql
-- List meetings bao gồm cả group members
SELECT ... WHERE m.student_id = ?
UNION
SELECT ... JOIN group_members gm ... WHERE gm.student_id = ?
```

**Quan trọng - Check meeting đã diễn ra:**
```sql
-- Chỉ cho add minutes khi meeting đã bắt đầu
SELECT ..., s.start_time <= NOW() as has_started FROM meetings m ...
```

---

### `protocol.c` - Protocol Parser
**Chức năng:** Parse request/response theo format

**Format:**
```
Request:  COMMAND|TOKEN|DATA\r\n
Response: STATUS_CODE||PAYLOAD\r\n
```

**Quan trọng:**
```c
// Parse request fields
Request* parse_request(const char* raw_message);
// Build response string
char* build_response(int status_code, const char* payload);
```

---

### `database.c` - MySQL Connection
```c
MYSQL* db_connect();       // Mở connection
void db_close(MYSQL* conn); // Đóng connection
MYSQL_RES* db_query();     // Execute SELECT
int db_execute();          // Execute INSERT/UPDATE/DELETE
```

---

### `auth.c` - Token Management
```c
char* generate_token(int user_id, const char* role);  // Tạo JWT
TokenData* validate_token(const char* token);          // Verify JWT
```

---

## 📱 CLIENT (`/client/src/`)

### `main.c` - Entry point
```c
int sockfd = connect_to_server(SERVER_HOST, SERVER_PORT);
show_auth_screen(sockfd);  // Login/Register menu
```

### `network.c` - Socket connection
```c
int connect_to_server(const char* host, int port);  // TCP connect
int send_request(...);      // Gửi request
char* receive_response(...); // Nhận response
```

### `ui_auth.c` - Login/Register UI
```c
do_login();     // Form đăng nhập
do_register();  // Form đăng ký
```

### `ui_student.c` - Student menu
| Function | Chức năng |
|----------|-----------|
| `view_free_slots()` | Xem & book slots |
| `view_my_meetings()` | Xem meetings của mình |
| `view_meeting_minutes()` | Xem biên bản |
| `cancel_meeting()` | Hủy meeting |

### `ui_teacher.c` - Teacher menu
| Function | Chức năng |
|----------|-----------|
| `manage_slots()` | CRUD slots |
| `view_appointments()` | Xem appointments + add minutes |
| `view_student_history()` | Xem history với student |

### `ui_components.c` - Reusable ncurses components
```c
show_menu();        // Menu lựa chọn
show_input_form();  // Input form
show_text_editor(); // Multi-line editor
show_error();       // Error popup
show_success();     // Success popup
```

---

## 🔑 Status Codes

| Code | Constant | Ý nghĩa |
|------|----------|---------|
| 2000 | STATUS_OK | Thành công |
| 4000 | STATUS_BAD_REQUEST | Request sai format |
| 4010 | STATUS_TOKEN_MISSING | Thiếu token |
| 4011 | STATUS_TOKEN_INVALID | Token hết hạn/sai |
| 4030 | STATUS_FORBIDDEN | Không có quyền |
| 4040 | STATUS_NOT_FOUND | Không tìm thấy |
| 4041 | STATUS_WRONG_PASSWORD | Sai mật khẩu |
| 4090 | STATUS_USERNAME_EXISTS | Username đã tồn tại |
| 5000 | STATUS_INTERNAL_ERROR | Lỗi server |

---

## 🗄️ Database Schema

```sql
users (user_id, username, password_hash, role)
slots (slot_id, teacher_id, start_time, end_time, slot_type, is_booked)
meetings (meeting_id, slot_id, student_id, is_group, status)
group_members (id, meeting_id, student_id)
```

---

## 🔄 Flow Example: Book Meeting

```
1. Client: LIST_FREE_SLOTS|token|
2. Server: 2000||LIST_FREE_SLOTS_SUCCESS||1&5&teacher1&2026-01-12 09:00&...

3. Client: BOOK_INDIVIDUAL|token|3
4. Server: Check slot exists, not booked, type allows individual
5. Server: INSERT INTO meetings, UPDATE slots SET is_booked=1
6. Server: 2000||BOOK_INDIVIDUAL_SUCCESS||7
```

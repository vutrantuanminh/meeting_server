# Meeting Management System

Hệ thống quản lý cuộc họp giữa Student và Teacher với giao diện TUI (Terminal User Interface) sử dụng ncurses.

## 📋 Features

### Student
- **View Free Slots**: Xem danh sách slots trống, nhập SlotID + TeacherID để đặt meeting
- **My Meetings**: Xem danh sách meetings của mình, nhập MeetingID để:
  - View Meeting Minutes
  - Cancel Meeting

### Teacher
- **Manage Slots**: Xem danh sách slots hiện có, thêm/sửa/xóa slot
- **View Appointments**: Xem danh sách appointments, nhập MeetingID để add/edit minutes
- **View Student History**: Xem danh sách students, chọn để xem lịch sử meetings

### Common
- Register với role selection (Student/Teacher)
- Login/Logout

## 🛠️ Technology Stack

- **Server**: C (socket, fork, MySQL)
- **Client**: C (ncurses TUI)
- **Database**: MySQL
- **Protocol**: Custom text-based protocol

## 📁 Project Structure

```
meeting_server/
├── src/                    # Server source code
│   ├── server.c           # Main server
│   ├── handler_auth.c     # Authentication handlers
│   ├── handler_slot.c     # Slot management handlers
│   ├── handler_meeting.c  # Meeting handlers
│   ├── auth.c             # Password hashing, token
│   ├── database.c         # MySQL wrapper
│   ├── protocol.c         # Request/Response parsing
│   └── utils.c            # Logging, utilities
├── include/               # Server headers
├── client/
│   ├── src/              # Client source code
│   │   ├── main.c        # Entry point
│   │   ├── network.c     # Socket connection
│   │   ├── ui_auth.c     # Auth screens
│   │   ├── ui_student.c  # Student menu
│   │   ├── ui_teacher.c  # Teacher menu
│   │   ├── ui_components.c # Reusable UI
│   │   └── ui_core.c     # Screen utilities
│   └── include/          # Client headers
├── minutes/              # Meeting minutes storage
├── setup_test_data.sql   # Test data (date: 2026-01-05)
└── test_flow.md          # Test scenarios
```

## 🚀 Quick Start

### Prerequisites
- GCC
- MySQL Server
- ncurses library

### 1. Database Setup
```bash
# Create database
mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS meeting_db"

# Import schema (create tables)
mysql -u root -p meeting_db < schema.sql

# Load test data (optional)
mysql -u root -p123456 meeting_db < setup_test_data.sql
```

### 2. Build
```bash
# Build server
cd /home/oc/meeting_server
make

# Build client
cd client
make
```

### 3. Run
```bash
# Terminal 1: Start server
./bin/server

# Terminal 2: Start client
cd client
./bin/meeting-client
```

## 🧪 Test Accounts

| Username | Password | Role |
|----------|----------|------|
| student1 | pass123 | Student |
| student2 | pass123 | Student |
| student3 | pass123 | Student |
| student4 | pass123 | Student |
| teacher1 | pass123 | Teacher (ID=5) |
| teacher2 | pass123 | Teacher (ID=6) |

## 📖 Test Scenarios

See [test_flow.md](test_flow.md) for detailed test scenarios.

## 🔧 Configuration

### Server
- Port: 8080 (defined in `include/server.h`)
- Database: Configure in `include/database.h`

### Client
- Server Host: localhost (default)
- Server Port: 8080

## 📝 Protocol Format

### Request
```
COMMAND|TOKEN|DATA\r\n
```

### Response
```
STATUS_CODE||PAYLOAD\r\n
```

### Status Codes
- 2000: OK
- 4001: Bad Request
- 4002: Token Invalid
- 4003: Forbidden
- 5000: Internal Error

## 📜 License

MIT License

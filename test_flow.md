# Quick Test Flow - Meeting Management System

## Setup (Chạy 1 lần)

```bash
# 1. Load test data
cd /home/oc/meeting_server
mysql -u root -p123456 meeting_db < setup_test_data.sql

# 2. Start server
./bin/server
```

## Test Accounts
- **Students:** student1, student2, student3, student4 (pass: pass123)
- **Teachers:** teacher1, teacher2 (pass: pass123)

---

## Flow 1: Student Complete Workflow

### Login
1. Start client: `cd client && ./bin/meeting-client`
2. Login → username: `student1`, password: `pass123`
3. ✅ Vào Student Menu

### View Free Slots
1. Chọn "View Free Slots"
2. ✅ Thấy 7 slots available

### Book Individual Meeting
1. Chọn "Book Individual Meeting"
2. Teacher ID: `5` (teacher1)
3. Slot ID: `1` (slot đầu tiên)
4. ✅ Success

### Book Group Meeting
1. Chọn "Book Group Meeting"
2. Teacher ID: `5`
3. Slot ID: `2` (group only slot)
4. Member IDs: `2,3` (student2, student3)
5. ✅ Success

### View My Meetings
1. Chọn "My Meetings"
2. Filter: "All"
3. ✅ Thấy 2 meetings vừa book

### View Minutes
1. Chọn "View Meeting Minutes"
2. Meeting ID: `2` (có minutes sẵn)
3. ✅ Hiển thị nội dung

### Cancel Meeting
1. Chọn "Cancel Meeting"
2. Meeting ID: `1` (meeting vừa book)
3. Confirm: Y
4. ✅ Success
5. View My Meetings → chỉ còn 1 meeting

### Logout
1. Chọn "Logout"
2. ✅ Về welcome screen

---

## Flow 2: Teacher Complete Workflow

### Login
1. Login → username: `teacher1`, password: `pass123`
2. ✅ Vào Teacher Menu

### Add Slot
1. Chọn "Manage Slots" → "Add New Slot"
2. Date: `2025-01-20`
3. Start: `09:00`
4. End: `10:00`
5. Type: "Individual Only"
6. ✅ Success

### Update Slot
1. Chọn "Manage Slots" → "Update Slot"
2. Slot ID: `1`
3. New Date: `2025-01-21`
4. New Start: `10:00`
5. New End: `11:00`
6. ✅ Success

### View Appointments
1. Chọn "View Appointments"
2. Filter: "All"
3. ✅ Thấy meetings đã được book

### Add Minutes
1. Chọn "Add Meeting Minutes"
2. Meeting ID: `2`
3. Nhập nội dung:
   ```
   Meeting Summary
   Date: 2025-01-17
   Attendees: student2, student3, student4
   Topics: Project discussion
   ```
4. Ctrl+D
5. ✅ Success

### View Student History
1. Chọn "View Student History"
2. Student ID: `1`
3. ✅ Thấy lịch sử meetings của student1

### Delete Slot
1. Chọn "Manage Slots" → "Delete Slot"
2. Slot ID: `4` (slot chưa book)
3. Confirm: Y
4. ✅ Success

### Logout
1. Chọn "Logout"

---

## Flow 3: Register New User

### Register as Student
1. Welcome screen → "Register"
2. Username: `newstudent`
3. Password: `pass123`
4. Confirm: `pass123`
5. Role: "Student"
6. ✅ Success
7. Login với newstudent/pass123
8. ✅ Vào Student Menu

### Register as Teacher
1. Register
2. Username: `newteacher`
3. Password: `pass123`
4. Confirm: `pass123`
5. Role: "Teacher"
6. ✅ Success
7. Login với newteacher/pass123
8. ✅ Vào Teacher Menu

---

## Quick Verification

```bash
# Check database
mysql -u root -p123456 meeting_db -e "
SELECT u.username, u.role, COUNT(m.meeting_id) as meetings
FROM users u
LEFT JOIN meetings m ON u.user_id = m.organizer_id
GROUP BY u.user_id;
"

# Check slots
mysql -u root -p123456 meeting_db -e "
SELECT s.slot_id, u.username as teacher, s.start_time, s.is_booked
FROM slots s
JOIN users u ON s.teacher_id = u.user_id
ORDER BY s.start_time;
"
```

---

## Expected Results Summary

✅ Students can:
- View free slots
- Book individual meetings
- Book group meetings
- View their meetings
- Cancel meetings
- View minutes

✅ Teachers can:
- Add/update/delete slots
- View appointments
- Add minutes
- View student history

✅ Both can:
- Register with role selection
- Login with correct menu
- Logout

---

**Total test time:** ~5 phút

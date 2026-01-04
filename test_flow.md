# Quick Test Flow - Meeting Management System

## Setup (Chạy 1 lần)

```bash
# 1. Load test data (ngày mốc: 2026-01-05)
cd /home/oc/meeting_server
mysql -u root -p123456 meeting_db < setup_test_data.sql

# 2. Start server
./bin/server
```

## Test Accounts
- **Students:** student1, student2, student3, student4 (pass: pass123)
- **Teachers:** teacher1 (ID=5), teacher2 (ID=6) (pass: pass123)

---

## Flow 1: Student Complete Workflow

### Login
1. Start client: `cd client && ./bin/meeting-client`
2. Login → username: `student1`, password: `pass123`
3. ✅ Vào Student Menu (3 options: View Free Slots, My Meetings, Logout)

### View Free Slots & Book Meeting
1. Chọn "View Free Slots"
2. ✅ Thấy danh sách slots (SlotID, TeacherID, Teacher, Start, End)
3. Nhập SlotID: `1`
4. Nhập TeacherID: `5`
5. Chọn "Individual Meeting" hoặc "Group Meeting"
6. Nếu Group: nhập Member IDs: `2,3`
7. ✅ Booking success

### View My Meetings & Manage
1. Chọn "My Meetings"
2. Chọn Filter: "All"
3. ✅ Thấy danh sách meetings (MeetingID, Date, Time, Teacher, Type)
4. Nhập MeetingID: `1`
5. Sub-menu xuất hiện:
   - "View Meeting Minutes" → xem nội dung minutes
   - "Cancel Meeting" → hủy meeting
   - "Back"

### View Minutes
1. Trong My Meetings, nhập MeetingID có minutes (ID từ test data)
2. Chọn "View Meeting Minutes"
3. ✅ Hiển thị nội dung minutes

### Cancel Meeting
1. Trong My Meetings, nhập MeetingID: `1`
2. Chọn "Cancel Meeting"
3. Confirm: Y
4. ✅ Cancel success

### Logout
1. Chọn "Logout"
2. ✅ Về welcome screen

---

## Flow 2: Teacher Complete Workflow

### Login
1. Login → username: `teacher1`, password: `pass123`
2. ✅ Vào Teacher Menu (4 options: Manage Slots, View Appointments, View Student History, Logout)

### Manage Slots
1. Chọn "Manage Slots"
2. ✅ Thấy danh sách slots hiện có (SlotID, Date, Start, End, Type, Booked)
3. **Add Slot:**
   - Nhập `1` → Add
   - Date: `2026-01-10`
   - Start: `09:00`
   - End: `10:00`
   - Type: "Individual Only"
   - ✅ Success
4. **Update Slot:**
   - Nhập `2` → Update
   - Slot ID to update: `<slot_id>`
   - Nhập thông tin mới
   - ✅ Success
5. **Delete Slot:**
   - Nhập `3` → Delete
   - Slot ID to delete: `<slot_id>` (chưa booked)
   - Confirm: Y
   - ✅ Success
6. Nhập `0` → Back

### View Appointments & Add/Edit Minutes
1. Chọn "View Appointments"
2. Chọn Filter: "Today" / "This Week" / "All"
3. ✅ Thấy danh sách appointments (MeetingID, Date, Time, Student, Type)
4. Nhập MeetingID để add/edit minutes: `2`
5. ✅ Load existing minutes (nếu có) vào editor
6. Sửa/thêm nội dung
7. Ctrl+D để save
8. ✅ "Minutes added successfully!" hoặc "Minutes updated successfully!"

### View Student History
1. Chọn "View Student History"
2. ✅ Thấy danh sách students (StudentID, Username)
3. Nhập StudentID: `1`
4. ✅ Thấy lịch sử meetings của student1 (MeetingID, Date, Has Minutes)
5. Nhập MeetingID để xem minutes
6. ✅ Hiển thị nội dung minutes

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
# Check users
mysql -u root -p123456 meeting_db -e "SELECT user_id, username, role FROM users;"

# Check slots
mysql -u root -p123456 meeting_db -e "
SELECT s.slot_id, u.username as teacher, DATE(s.start_time) as date, 
       TIME(s.start_time) as start, s.is_booked
FROM slots s JOIN users u ON s.teacher_id = u.user_id
ORDER BY s.start_time;"

# Check meetings
mysql -u root -p123456 meeting_db -e "
SELECT m.meeting_id, u1.username as student, u2.username as teacher, 
       DATE(s.start_time) as date, m.status
FROM meetings m 
JOIN slots s ON m.slot_id = s.slot_id
JOIN users u1 ON m.student_id = u1.user_id
JOIN users u2 ON s.teacher_id = u2.user_id;"
```

---

## Expected Results Summary

### Students can:
- ✅ View free slots (by SlotID, TeacherID)
- ✅ Book individual/group meetings (nhập SlotID + TeacherID)
- ✅ View their meetings (by MeetingID)
- ✅ Cancel meetings (by MeetingID)
- ✅ View minutes (by MeetingID)

### Teachers can:
- ✅ View their slots list
- ✅ Add/update/delete slots (by SlotID)
- ✅ View appointments with filters
- ✅ Add/edit minutes (load existing, edit, save)
- ✅ View student list and history

### Both can:
- ✅ Register with role selection
- ✅ Login with correct menu
- ✅ Logout

---

**Total test time:** ~5 phút

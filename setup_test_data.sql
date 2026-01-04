-- Setup Test Data for Meeting Management System
-- Run: mysql -u root -p123456 meeting_db < setup_test_data.sql
-- Fixed date: 2026-01-05 (presentation day)

-- Clear existing data
DELETE FROM group_members;
DELETE FROM meetings;
DELETE FROM slots;
DELETE FROM users;

-- Reset auto increment
ALTER TABLE users AUTO_INCREMENT = 1;
ALTER TABLE slots AUTO_INCREMENT = 1;
ALTER TABLE meetings AUTO_INCREMENT = 1;

-- Create test users
INSERT INTO users (username, password_hash, role) VALUES
-- Students (password: pass123)
('student1', SHA2('pass123', 256), 'student'),
('student2', SHA2('pass123', 256), 'student'),
('student3', SHA2('pass123', 256), 'student'),
('student4', SHA2('pass123', 256), 'student'),

-- Teachers (password: pass123)
('teacher1', SHA2('pass123', 256), 'teacher'),
('teacher2', SHA2('pass123', 256), 'teacher');

-- =====================================================
-- Create slots for teacher1 (user_id=5)
-- Base date: 2026-01-05 (presentation day)
-- =====================================================

INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
-- TODAY's slots (2026-01-05) - for testing "Today" filter
(5, '2026-01-05 09:00:00', '2026-01-05 10:00:00', 0, 0),  -- Individual only
(5, '2026-01-05 14:00:00', '2026-01-05 15:00:00', 1, 0),  -- Group only
(5, '2026-01-05 16:00:00', '2026-01-05 17:00:00', 2, 0),  -- Both

-- TOMORROW's slots (2026-01-06)
(5, '2026-01-06 09:00:00', '2026-01-06 10:00:00', 0, 0),  -- Individual only
(5, '2026-01-06 14:00:00', '2026-01-06 15:00:00', 2, 0),  -- Both

-- DAY AFTER TOMORROW (2026-01-07)
(5, '2026-01-07 10:00:00', '2026-01-07 11:00:00', 2, 0),  -- Both

-- NEXT WEEK slots (2026-01-12) - for testing "This Week" vs "All" filter
(5, '2026-01-12 09:00:00', '2026-01-12 10:00:00', 0, 0),  -- Individual only
(5, '2026-01-12 14:00:00', '2026-01-12 15:00:00', 1, 0);  -- Group only

-- =====================================================
-- Create slots for teacher2 (user_id=6)
-- =====================================================
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(6, '2026-01-05 11:00:00', '2026-01-05 12:00:00', 2, 0),  -- Today
(6, '2026-01-06 09:00:00', '2026-01-06 10:00:00', 0, 0),  -- Tomorrow
(6, '2026-01-08 14:00:00', '2026-01-08 15:00:00', 2, 0);  -- 3 days later

-- =====================================================
-- Create booked slots and meetings for testing
-- =====================================================

-- Booked slot for TODAY (2026-01-05)
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-05 10:00:00', '2026-01-05 11:00:00', 0, 1);  -- Booked individual

-- Meeting 1: Individual meeting today (student1 with teacher1)
INSERT INTO meetings (slot_id, student_id, is_group, status) VALUES
((SELECT MAX(slot_id) FROM slots), 1, 0, 'pending');

-- Booked slot for TOMORROW (2026-01-06) - Group meeting
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-06 11:00:00', '2026-01-06 12:00:00', 1, 1);  -- Booked group

-- Meeting 2: Group meeting tomorrow (student2 with teacher1)
INSERT INTO meetings (slot_id, student_id, is_group, status) VALUES
((SELECT MAX(slot_id) FROM slots), 2, 1, 'pending');

-- Add group members for meeting 2 (student3 and student4)
INSERT INTO group_members (meeting_id, student_id) VALUES
((SELECT MAX(meeting_id) FROM meetings), 3),
((SELECT MAX(meeting_id) FROM meetings), 4);

-- =====================================================
-- Summary output
-- =====================================================
SELECT '=== Test Data Setup Complete ===' as Status;
SELECT 'Presentation Date: 2026-01-05' as Info;
SELECT '' as '';

SELECT 'Test Accounts:' as '';
SELECT '  Students: student1, student2, student3, student4 (pass: pass123)' as '';
SELECT '  Teachers: teacher1 (ID=5), teacher2 (ID=6) (pass: pass123)' as '';
SELECT '' as '';

SELECT 'Available Slots (not booked):' as '';
SELECT slot_id as SlotID, teacher_id as TeacherID, 
       DATE(start_time) as Date, TIME(start_time) as Start,
       CASE slot_type WHEN 0 THEN 'Individual' WHEN 1 THEN 'Group' ELSE 'Both' END as Type
FROM slots WHERE is_booked=0 ORDER BY start_time;
SELECT '' as '';

SELECT 'Booked Meetings:' as '';
SELECT m.meeting_id as MeetingID, u1.username as Student, u2.username as Teacher,
       DATE(s.start_time) as Date, TIME(s.start_time) as Time,
       CASE m.is_group WHEN 0 THEN 'Individual' ELSE 'Group' END as Type
FROM meetings m 
JOIN slots s ON m.slot_id = s.slot_id 
JOIN users u1 ON m.student_id = u1.user_id
JOIN users u2 ON s.teacher_id = u2.user_id;

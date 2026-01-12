-- ============================================
-- MEETING SERVER - TEST DATA
-- Base date: 2026-01-12 (Sunday) 14:00
-- ============================================

-- Clear existing data
DELETE FROM group_members;
DELETE FROM meetings;
DELETE FROM slots;
DELETE FROM users WHERE user_id > 0;

-- Reset auto increment
ALTER TABLE users AUTO_INCREMENT = 1;
ALTER TABLE slots AUTO_INCREMENT = 1;
ALTER TABLE meetings AUTO_INCREMENT = 1;

-- ============================================
-- USERS (password: pass123)
-- ============================================
-- Students (ID 1-4)
INSERT INTO users (username, password_hash, role) VALUES
('student1', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'student'),
('student2', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'student'),
('student3', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'student'),
('student4', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'student');

-- Teachers (ID 5-6)
INSERT INTO users (username, password_hash, role) VALUES
('teacher1', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'teacher'),
('teacher2', '9b8769a4a742959a2d0298c36fb70623f2dfacda8436237df08d8dfd5b37374c', 'teacher');

-- ============================================
-- SLOTS (teacher1 = ID 5, teacher2 = ID 6)
-- slot_type: 0=Individual, 1=Group, 2=Both
-- ============================================

-- PAST MEETINGS (can add minutes)
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-10 09:00:00', '2026-01-10 10:00:00', 0, 1),  -- Slot 1: Past, booked
(5, '2026-01-11 14:00:00', '2026-01-11 15:00:00', 1, 1),  -- Slot 2: Yesterday, booked (group)
(5, '2026-01-12 09:00:00', '2026-01-12 10:00:00', 2, 1);  -- Slot 3: Today morning, booked

-- TODAY - FREE SLOTS (for booking demo)
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-12 15:00:00', '2026-01-12 16:00:00', 0, 0),  -- Slot 4: Today 15:00, free
(5, '2026-01-12 16:00:00', '2026-01-12 17:00:00', 1, 0),  -- Slot 5: Today 16:00, free
(6, '2026-01-12 14:00:00', '2026-01-12 15:00:00', 2, 0);  -- Slot 6: Today 14:00, free (teacher2)

-- THIS WEEK (Mon-Fri) - FREE SLOTS
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-13 09:00:00', '2026-01-13 10:00:00', 0, 0),  -- Slot 7: Mon
(5, '2026-01-14 14:00:00', '2026-01-14 15:00:00', 1, 0),  -- Slot 8: Tue
(5, '2026-01-15 10:00:00', '2026-01-15 11:00:00', 2, 0),  -- Slot 9: Wed
(6, '2026-01-16 09:00:00', '2026-01-16 10:00:00', 0, 0),  -- Slot 10: Thu (teacher2)
(6, '2026-01-17 14:00:00', '2026-01-17 15:00:00', 2, 0);  -- Slot 11: Fri (teacher2)

-- NEXT WEEK - FREE SLOTS (for "All" filter)
INSERT INTO slots (teacher_id, start_time, end_time, slot_type, is_booked) VALUES
(5, '2026-01-19 09:00:00', '2026-01-19 10:00:00', 2, 0),  -- Slot 12: Next Mon
(5, '2026-01-20 14:00:00', '2026-01-20 15:00:00', 1, 0);  -- Slot 13: Next Tue

-- ============================================
-- MEETINGS (Past meetings - can add minutes)
-- ============================================

-- Meeting 1: Past individual (slot 1) - student1
INSERT INTO meetings (slot_id, student_id, is_group, status) VALUES
(1, 1, 0, 'pending');

-- Meeting 2: Yesterday group (slot 2) - student2 leader, student3 & student4 members
INSERT INTO meetings (slot_id, student_id, is_group, status) VALUES
(2, 2, 1, 'pending');

INSERT INTO group_members (meeting_id, student_id) VALUES
(2, 3),
(2, 4);

-- Meeting 3: Today morning (slot 3) - student1
INSERT INTO meetings (slot_id, student_id, is_group, status) VALUES
(3, 1, 0, 'pending');

-- ============================================
-- SUMMARY
-- ============================================
-- Password: pass123 for all users
-- Students: student1 (ID=1), student2 (ID=2), student3 (ID=3), student4 (ID=4)
-- Teachers: teacher1 (ID=5), teacher2 (ID=6)
--
-- PAST MEETINGS (can add minutes):
--   Meeting 1: 2026-01-10 09:00, student1, individual
--   Meeting 2: 2026-01-11 14:00, student2 + student3,4, group
--   Meeting 3: 2026-01-12 09:00, student1, individual
--
-- FREE SLOTS for booking:
--   Today (after 14:00): Slots 4, 5, 6
--   This week: Slots 7, 8, 9, 10, 11
--   Next week: Slots 12, 13

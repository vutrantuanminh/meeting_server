#!/bin/bash
# Quick Test Setup Script
# Usage: ./quick_test_setup.sh

set -e

echo "=== Meeting Management System - Quick Test Setup ==="
echo ""

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 1. Check MySQL
echo -e "${YELLOW}[1/5] Checking MySQL...${NC}"
if ! systemctl is-active --quiet mysql; then
    echo -e "${RED}MySQL is not running. Starting...${NC}"
    sudo systemctl start mysql
fi
echo -e "${GREEN}✓ MySQL is running${NC}"

# 2. Check database
echo -e "${YELLOW}[2/5] Checking database...${NC}"
DB_EXISTS=$(mysql -u root -p123456 -e "SHOW DATABASES LIKE 'meeting_db';" 2>/dev/null | grep meeting_db || echo "")
if [ -z "$DB_EXISTS" ]; then
    echo -e "${RED}Database 'meeting_db' not found!${NC}"
    echo "Please create database first."
    exit 1
fi
echo -e "${GREEN}✓ Database exists${NC}"

# 3. Create test users
echo -e "${YELLOW}[3/5] Creating test users...${NC}"
mysql -u root -p123456 meeting_db << 'EOF' 2>/dev/null
-- Create test users
INSERT IGNORE INTO users (username, password_hash, role) VALUES
('student1', SHA2('pass123', 256), 'student'),
('student2', SHA2('pass123', 256), 'student'),
('student3', SHA2('pass123', 256), 'student'),
('teacher1', SHA2('pass123', 256), 'teacher'),
('teacher2', SHA2('pass123', 256), 'teacher');
EOF
echo -e "${GREEN}✓ Test users created${NC}"

# 4. Build server
echo -e "${YELLOW}[4/5] Building server...${NC}"
cd /home/oc/meeting_server
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo -e "${GREEN}✓ Server built${NC}"

# 5. Build client
echo -e "${YELLOW}[5/5] Building client...${NC}"
cd client
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo -e "${GREEN}✓ Client built${NC}"

echo ""
echo -e "${GREEN}=== Setup Complete! ===${NC}"
echo ""
echo "Test Accounts:"
echo "  Students: student1, student2, student3 (password: pass123)"
echo "  Teachers: teacher1, teacher2 (password: pass123)"
echo ""
echo "To start testing:"
echo "  1. Terminal 1: cd /home/oc/meeting_server && ./bin/server"
echo "  2. Terminal 2: cd /home/oc/meeting_server/client && ./bin/meeting-client"
echo ""
echo "View logs: tail -f /home/oc/meeting_server/logs/server.log"
echo ""

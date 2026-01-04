# Meeting Management System - Terminal Client

Terminal UI client cho Meeting Management System sử dụng ncurses.

## Yêu cầu

- GCC compiler
- ncurses library
- Server đang chạy trên localhost:1234

## Cài đặt ncurses (nếu chưa có)

```bash
# Ubuntu/Debian
sudo apt-get install libncurses5-dev libncursesw5-dev

# Fedora/RHEL
sudo dnf install ncurses-devel

# macOS
brew install ncurses
```

## Build

```bash
cd client
make
```

## Chạy

```bash
# Đảm bảo server đang chạy trước
cd ..
make run &

# Chạy client
cd client
make run
```

## Sử dụng

### Điều hướng
- **Phím mũi tên**: Di chuyển trong menu
- **Enter**: Chọn
- **ESC**: Quay lại/Hủy
- **Ctrl+C**: Thoát chương trình

### Tính năng Student
1. View Free Slots - Xem các slot trống
2. Book Individual Meeting - Đặt lịch cá nhân
3. Book Group Meeting - Đặt lịch nhóm
4. My Meetings - Xem lịch của mình
5. Cancel Meeting - Hủy lịch
6. View Meeting Minutes - Xem biên bản

### Tính năng Teacher
1. Manage Slots - Quản lý slot (thêm/sửa/xóa)
2. View Appointments - Xem lịch hẹn
3. Add Meeting Minutes - Thêm biên bản
4. View Student History - Xem lịch sử sinh viên

## Cấu trúc

```
client/
├── src/           # Source files
├── include/       # Header files
├── obj/           # Object files (generated)
├── bin/           # Binary (generated)
└── Makefile
```

## Troubleshooting

**Lỗi kết nối server:**
- Kiểm tra server đang chạy: `ps aux | grep server`
- Kiểm tra port 1234 đang listen: `netstat -an | grep 1234`

**Lỗi màu sắc không hiển thị:**
- Terminal phải hỗ trợ màu (hầu hết terminal hiện đại đều hỗ trợ)

**Lỗi build:**
- Kiểm tra ncurses đã cài: `ldconfig -p | grep ncurses`

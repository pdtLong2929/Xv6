#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    // 1. Kiểm tra đối số dòng lệnh: phải có 2 đối số (src và dst)
    if (argc != 3) {
        fprintf(2, "usage: cp src dst\n");
        exit(1);
    }

    char *src_path = argv[1];
    char *dst_path = argv[2];

    // 2. Mở file nguồn ở chế độ chỉ đọc
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        fprintf(2, "cp: cannot open %s\n", src_path);
        exit(1);
    }

    // 3. Mở file đích: nếu chưa có thì tạo mới, mở ở chế độ ghi
    int dst_fd = open(dst_path, O_CREATE | O_WRONLY);
    if (dst_fd < 0) {
        fprintf(2, "cp: cannot create %s\n", dst_path);
        close(src_fd);
        exit(1);
    }

    // 4. Vòng lặp đọc và ghi qua buffer
    char buf[512];
    int bytes_read;
    while ((bytes_read = read(src_fd, buf, sizeof(buf))) > 0) {
        int bytes_written = write(dst_fd, buf, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(2, "cp: write error\n");
            close(src_fd);
            close(dst_fd);
            exit(1);
        }
    }

    // 5. Kiểm tra lỗi đọc nếu bytes_read < 0
    if (bytes_read < 0) {
        fprintf(2, "cp: read error\n");
        close(src_fd);
        close(dst_fd);
        exit(1);
    }

    // 6. Đóng file và kết thúc
    close(src_fd);
    close(dst_fd);
    exit(0);
}
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// Biến cấu hình toàn cục
int opt_only_dir = 0;  // Cờ -d: chỉ in thư mục
int opt_max_depth = -1; // Cờ -L: giới hạn độ sâu (-1 nghĩa là vô hạn)

// Hàm in thụt lề theo cấp bậc (level)
void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("    "); // 4 khoảng trắng mỗi cấp
    }
}

// Hàm duyệt đệ quy cây thư mục
void tree_traverse(char *path, int current_level) {
    // 1. Kiểm tra giới hạn độ sâu
    if (opt_max_depth > 0 && current_level >= opt_max_depth) {
        return;
    }

    // 2. Mở thư mục
    int fd = open(path, 0);
    if (fd < 0) {
        fprintf(2, "tree: cannot open %s\n", path);
        return;
    }

    // 3. Kiểm tra xem path có thực sự là thư mục hay không
    struct stat st;
    if (fstat(fd, &st) < 0 || st.type != T_DIR) {
        close(fd);
        return;
    }

    // 4. Chuẩn bị bộ đệm để ghép đường dẫn con
    char buf[512];
    char *p;
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        fprintf(2, "tree: path too long\n");
        close(fd);
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    // 5. Đọc từng bản ghi dirent trong thư mục
    struct dirent de;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // Bỏ qua entry không dùng (inum == 0)
        if (de.inum == 0) {
            continue;
        }

        // Bỏ qua thư mục hiện tại "." và thư mục cha ".."
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }

        // Ghép tên file vào sau đường dẫn cha
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0; // Đảm bảo kết thúc chuỗi an toàn

        // Lấy thông tin stat của mục con
        struct stat entry_st;
        if (stat(buf, &entry_st) < 0) {
            continue;
        }

        // Nếu có cờ -d và đây không phải là thư mục thì bỏ qua
        if (opt_only_dir && entry_st.type != T_DIR) {
            continue;
        }

        // In mục con với thụt lề tương ứng
        print_indent(current_level);
        printf("|-- %s\n", de.name);

        // Nếu mục con là thư mục, tiếp tục đệ quy xuống cấp tiếp theo
        if (entry_st.type == T_DIR) {
            tree_traverse(buf, current_level + 1);
        }
    }

    // 6. Luôn đóng file descriptor khi duyệt xong
    close(fd);
}

int main(int argc, char *argv[]) {
    char *root_path = ".";

    // Phân tích tham số dòng lệnh linh hoạt
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            opt_only_dir = 1;
        } else if (strcmp(argv[i], "-L") == 0) {
            if (i + 1 < argc) {
                opt_max_depth = atoi(argv[++i]);
                if (opt_max_depth <= 0) {
                    fprintf(2, "tree: invalid depth\n");
                    exit(1);
                }
            } else {
                fprintf(2, "tree: option -L requires an argument\n");
                exit(1);
            }
        } else if (argv[i][0] != '-') {
            // Đây là đường dẫn path do người dùng chỉ định
            root_path = argv[i];
        } else {
            fprintf(2, "tree: invalid option %s\n", argv[i]);
            exit(1);
        }
    }

    // In thư mục gốc ban đầu
    printf("%s\n", root_path);

    // Bắt đầu duyệt đệ quy từ level 0
    tree_traverse(root_path, 0);

    exit(0);
}

// ```mermaid
// flowchart TD
//     A["Bắt đầu tree(path, current_level)"] --> B{"current_level >= max_depth?"}
//     B -- Có --> End["Dừng đệ quy"]
//     B -- Không --> C["Mở thư mục: fd = open(path, 0)"]
//     C --> D{"Đọc từng struct dirent"}
//     D -- Hết file --> Close["close(fd) và kết thúc"]
//     D -- Có entry --> E{"de.inum == 0 hoặc . hoặc .."}
//     E -- Đúng --> D
//     E -- Sai --> F["Ghép full_path = path + '/' + de.name"]
//     F --> G["Lấy metadata: stat(full_path, &st)"]
//     G --> H{"Có cờ -d và st.type != T_DIR?"}
//     H -- Đúng --> D
//     H -- Sai --> I["In thụt lề: |-- de.name"]
//     I --> J{"st.type == T_DIR?"}
//     J -- Có --> K["Gọi đệ quy: tree(full_path, current_level + 1)"]
//     K --> D
//     J -- Không --> D
// ```
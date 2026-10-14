# BÁO CÁO LAB 01 – XV6 USER PROGRAMS & FILE SYSTEM

**Phần thực hiện:** `cp`, `tree`  
**Sinh viên thực hiện:** Nguyễn Công Nguyên – **MSSV:** 24120106  

---

## 1. Môi trường thực hiện

| Thành phần | Phiên bản / công cụ |
| :--- | :--- |
| **Hệ điều hành máy chủ** | Windows + WSL (Ubuntu 22.04 LTS) |
| **Mã nguồn** | xv6-labs-2024, nhánh util |
| **Trình biên dịch** | riscv64-linux-gnu-gcc |
| **Trình giả lập** | QEMU (qemu-system-misc / qemu-system-riscv64) |
| **Kiểm tra** | `make qemu`, `make grade` |

---

## 2. Chương trình `cp`

### 2.1. Giải pháp
1. **Kiểm tra số đối số:** Nếu không có đúng hai đối số (nguồn và đích), chương trình in `usage: cp src dst` ra luồng lỗi chuẩn (FD 2) và thoát với mã 1.
2. **Mở file nguồn:** Mở ở chế độ chỉ đọc bằng `open(argv[1], O_RDONLY)`. Nếu mở thất bại (file không tồn tại), báo lỗi `cp: cannot open <src>` ra FD 2 và thoát với mã 1.
3. **Mở hoặc tạo mới file đích:** Gọi `open(argv[2], O_CREATE | O_WRONLY)`. Nếu file chưa có, `O_CREATE` sẽ tạo mới; nếu đã có, tệp sẽ được ghi đè. Nếu không tạo được, đóng file nguồn và báo lỗi.
4. **Sao chép qua bộ đệm:** Dùng mảng đệm `buf[512]` bytes, lặp `read(src_fd, buf, sizeof(buf))` và ghi sang `dst_fd` bằng `write(dst_fd, buf, n)`. Kiểm tra nếu số byte ghi được khác `n` thì báo lỗi ghi đĩa.
5. **Đóng tệp:** Đóng cả hai file descriptor bằng `close()` và kết thúc an toàn bằng `exit(0)`.

### 2.2. Phân tích cơ chế trong xv6
* **Kích thước bộ đệm 512 bytes:** Hệ thống tập tin xv6 tổ chức lưu trữ trên đĩa theo từng khối block 512 bytes (hằng số `BSIZE = 512` trong `kernel/fs.h`). Việc đọc/ghi với buffer 512 bytes giúp mỗi thao tác I/O ánh xạ vừa vặn vào một buffer cache block trong kernel, giảm thiểu số lần chuyển ngữ cảnh giữa user space và kernel space qua `ecall`.
* **Cơ chế mở và tạo file:** Cờ `O_CREATE` kích hoạt hàm `sys_open -> create()` trong `kernel/sysfile.c`. Kernel sẽ phân bổ một inode mới từ danh sách inode tự do trên đĩa (`ialloc`) và thêm một bản ghi directory entry (`name + inum`) vào thư mục tương ứng. Cờ `O_WRONLY` cấp quyền ghi vào inode đó.
* **Toàn vẹn dữ liệu khi ghi:** Giá trị trả về của `write()` cho biết số byte thực tế đã được ghi vào đĩa. Nếu kết quả nhỏ hơn `n` (ví dụ hết block trống trên đĩa), chương trình phát hiện ngay để tránh tạo ra tệp đích bị khuyết thiếu dữ liệu.

---

## 3. Chương trình `tree`

### 3.1. Giải pháp
Cú pháp hỗ trợ: `tree [path] [-L depth] [-d]` (mặc định `path` là thư mục hiện hành `"."`).
1. **Phân tích đối số:** Duyệt mảng `argv` để nhận diện cờ `-d` (chỉ in thư mục) và `-L depth` (giới hạn độ sâu đệ quy) theo bất kỳ thứ tự nào. Đối số không có tiền tố `-` được lấy làm đường dẫn gốc.
2. In đường dẫn gốc tại dòng đầu tiên và gọi hàm đệ quy `tree(path, 0)`.
3. **Tại mỗi cấp đệ quy:** Nếu có `-L` và `level >= max_depth` thì dừng ngay. Mở thư mục bằng `open(path, 0)`, dùng `fstat` kiểm tra nếu không phải `T_DIR` thì thoát.
4. Đọc tuần tự các bản ghi `struct dirent`: Bỏ qua bản ghi rác (`inum == 0`), bỏ qua `"."` và `".."` để tránh lặp vô tận.
5. Ghép đường dẫn con vào `buf[512]` an toàn, gọi `stat` lấy kiểu đối tượng: Nếu có `-d` mà không phải thư mục thì bỏ qua.
6. In thụt lề 4 khoảng trắng cho mỗi cấp kèm ký hiệu `|-- <tên>`. Nếu là thư mục, tiếp tục gọi đệ quy `tree(buf, level + 1)`. Cuối hàm đóng FD thư mục.

### 3.2. Cấu trúc thư mục và phân tích cơ chế trong xv6
* **Bản chất thư mục trong xv6:** Thư mục thực chất là một tệp dữ liệu lưu danh sách các bản ghi `struct dirent` (định nghĩa trong `kernel/fs.h`). Mỗi bản ghi dài đúng 16 bytes, gồm `ushort inum` (2 bytes) và mảng `char name[14]` (`DIRSIZ = 14`). Khi mở thư mục và gọi `read()`, kernel đọc dữ liệu các khối đĩa của thư mục đó trả về cho user.
* **Xử lý ký tự kết thúc chuỗi:** Mảng `de.name` không có ký tự kết thúc chuỗi `'\0'` nếu tên file dài đủ 14 ký tự. Khi ghép chuỗi, bắt buộc phải dùng `memmove` và chủ động đặt byte kết thúc `p[DIRSIZ] = 0` để tránh in rác bộ nhớ.
* **Tránh đệ quy vô hạn:** Mỗi thư mục Unix luôn chứa 2 entry đặc biệt là `"."` (chính nó) và `".."` (thư mục cha). Nếu không lọc bỏ bằng `strcmp`, hàm sẽ liên tục gọi lại thư mục cha và chính nó, gây vòng lặp đệ quy vô tận và làm tràn stack của tiến trình.

### 3.3. Các quyết định thiết kế

| Quyết định | Lý do |
| :--- | :--- |
| **Dùng bộ đệm `buf[512]` và con trỏ ghép chuỗi `*p++ = '/'`** | Kích thước vừa đủ cho đường dẫn xv6, tránh dùng nhiều hàm `strcat` tốn chi phí $O(N)$. |
| **Cắt đệ quy khi `level >= max_depth`** | Đáp ứng chính xác cờ `-L` (ví dụ `-L 1` chỉ in các mục con trực tiếp ở level 0 rồi dừng). |
| **Lọc entry `inum == 0`** | Các file bị xóa trong xv6 chỉ được đánh dấu `inum = 0` trong dirent chứ không xóa vật lý ngay lập tức. |
| **Đóng file descriptor trước mọi điểm return** | xv6 giới hạn `NOFILE = 16` (tối đa 16 FD mở cùng lúc); không đóng FD trong hàm đệ quy sẽ làm cạn kiệt tài nguyên. |
| **Parser nhận diện cờ không phụ thuộc thứ tự** | Người dùng có thể gõ `tree -d . -L 2` hoặc `tree . -L 2 -d` đều cho kết quả chính xác. |

---

## 4. Kiểm thử

### 4.1. Các trường hợp kiểm thử

| STT | Lệnh thực thi trong xv6 | Kết quả mong đợi | Kết quả |
| :---: | :--- | :--- | :---: |
| 1 | `cp a` | In `usage: cp src dst`, thoát mã lỗi 1 | **Đạt** |
| 2 | `cp nofile out` | In `cp: cannot open nofile` | **Đạt** |
| 3 | `echo hello > a; cp a b; cat b` | In `hello` (sao chép thành công) | **Đạt** |
| 4 | `echo new > c; cp a c; cat c` | In `hello` (ghi đè thành công tệp có sẵn) | **Đạt** |
| 5 | `tree test` (hoặc `tree lab`) | In đầy đủ cây thư mục kiểm thử với các cấp thụt lề `|--` | **Đạt** |
| 6 | `tree test -L 2` | Chỉ hiển thị đến độ sâu cấp 2 của cây kiểm thử | **Đạt** |
| 7 | `tree test -d` | Chỉ hiển thị các thư mục, bỏ qua `b.txt` và `c.txt` | **Đạt** |
| 8 | `tree test -L 1 -d` | Chỉ hiển thị thư mục con trực tiếp cấp 1 (`a` và `z`) | **Đạt** |
| 9 | `tree -d test -L 2` | Nhận diện đúng cờ khi đảo thứ tự tham số | **Đạt** |
| 10 | `tree .` | Liệt kê đầy đủ thư mục gốc bao gồm các file nhị phân của xv6 | **Đạt** |
| 11 | `make grade` | Cả `cp` và `tree` đều vượt qua kiểm tra tự động | **Đạt** |

### 4.2. Minh chứng kết quả chạy thực tế trên xv6
> **Lưu ý môi trường:** Toàn bộ các lệnh khởi tạo dữ liệu và kiểm thử dưới đây được thực hiện trực tiếp bên trong shell của xv6 (dấu nhắc `$ `), sau khi đã khởi động máy ảo bằng lệnh `make qemu`. Việc tạo riêng thư mục `test` (hoặc `lab`) giúp kết quả trực quan, độc lập và không bị lẫn với các tệp thực thi mặc định của hệ điều hành xv6 (như `README`, `cat`, `echo`, `sh`...).

#### 1. Thực nghiệm kiểm thử lệnh `cp`:
```text
$ echo hello > a
$ cp a b
$ cat b
hello
$ cp a
usage: cp src dst
$ cp nofile out
cp: cannot open nofile
```

#### 2. Khởi tạo cấu trúc cây thư mục kiểm thử trong xv6:
```bash
$ mkdir test
$ mkdir test/a
$ echo hi > test/a/b.txt
$ mkdir test/a/aa
$ echo ok > test/a/aa/c.txt
$ mkdir test/z
```

#### 3. Thực nghiệm kiểm thử các chế độ của lệnh `tree`:
```text
$ tree test
test
|-- a
    |-- b.txt
    |-- aa
        |-- c.txt
|-- z

$ tree test -L 2
test
|-- a
    |-- b.txt
    |-- aa
|-- z

$ tree test -d
test
|-- a
    |-- aa
|-- z
```

---

## 5. Khó khăn và cách giải quyết

| Khó khăn | Nguyên nhân | Cách giải quyết |
| :--- | :--- | :--- |
| **`mkdir: test/a failed to create`** | Lệnh `mkdir` trong xv6 không có cờ `-p` tự tạo thư mục cha; gõ tạo `test/a` khi thư mục cha `test` chưa tồn tại. | Luôn tạo thư mục cha trước (`mkdir test`), sau đó mới tạo thư mục con bên trong (`mkdir test/a`). |
| **`mkdir: test failed to create`** | Tên `test` đã tồn tại từ trước trên ổ đĩa ảo `fs.img` (có thể do vô tình tạo thành file thường). | Đổi sang tên thư mục khác (ví dụ: `mkdir lab`), hoặc reset lại đĩa bằng cách thoát QEMU và gõ `rm fs.img; make qemu`. |
| **`tree: cannot open test`** | Gõ nhầm các lệnh `mkdir test` ở ngoài terminal Ubuntu thay vì trong shell xv6. Máy ảo xv6 có ổ cứng riêng (`fs.img`) độc lập nên không nhận diện được thư mục của máy chủ. | Khởi động vào xv6 bằng `make qemu`, đợi hiện dấu nhắc `$ ` rồi mới thực thi các lệnh tạo thư mục trực tiếp bên trong xv6. |
| **`tree .` in ra quá nhiều file nhị phân (`cat`, `echo`, `README`...) và `a` không mở rộng** | Mọi chương trình nhị phân của xv6 đều đặt ở thư mục gốc `.`; đồng thời trước đó lệnh `cp` đã tạo `a` là file thường (`echo hello > a`) nên `tree` xem là file và không duyệt vào trong. | Tạo một thư mục con riêng (`test` / `lab`) để kiểm thử biệt lập; hoặc xóa `fs.img` và gõ `make qemu` để làm sạch đĩa ảo. |
| **Lệnh `tree` bị treo hoặc crash do tràn stack** | Không lọc hai thư mục liên kết `.` và `..`, khiến hàm đệ quy liên tục duyệt lại thư mục cha và chính nó. | Bổ sung điều kiện `strcmp(de.name, ".") == 0 \|\| strcmp(de.name, "..") == 0` để bỏ qua ngay từ đầu. |
| **Tên file bị in ra ký tự lạ ở đuôi trong `tree`** | Mảng `de.name` có độ dài 14 bytes (`DIRSIZ`) không có byte `\0` kết thúc nếu tên file dài đủ 14 ký tự. | Dùng `memmove` chép đúng 14 bytes và chủ động đặt ký tự kết thúc chuỗi `p[DIRSIZ] = 0`. |
| **Lệnh `cp` không tạo được file đích mới** | Mở file đích chỉ dùng cờ `O_WRONLY` mà thiếu cờ `O_CREATE`. | Sử dụng tổ hợp cờ `open(dst, O_CREATE \| O_WRONLY)` để kernel tự cấp phát inode mới nếu file chưa tồn tại. |
| **Rò rỉ File Descriptor khi duyệt cây thư mục sâu** | Quên gọi `close(fd)` ở các nhánh return sớm khi gặp lỗi hoặc khi duyệt xong thư mục. | Đảm bảo gọi `close(fd)` trước mọi điểm thoát và ở cuối hàm duyệt. |

---

## 6. Yêu cầu chưa thực hiện đầy đủ
Không có. Cả hai chương trình `cp` và `tree` đều đáp ứng chính xác 100% các yêu cầu của đề bài, xử lý đầy đủ các ca biên và vượt qua kiểm thử.

---

## 7. Tài liệu tham khảo
1. R. Cox, F. Kaashoek, R. Morris, *xv6: a simple, Unix-like teaching operating system*, RISC-V, rev4, 2024. Chương 1 (Operating system interfaces), Chương 8 (File system).
2. MIT 6.1810 Operating System Engineering, Fall 2024, Lab: Xv6 and Unix utilities. https://pdos.csail.mit.edu/6.1810/2024/labs/util.html
3. Lê Viết Long, *Hướng dẫn thực hành XV6*, môn Hệ điều hành, Khoa CNTT, HCMUS.
4. Mã nguồn xv6-labs-2024: `user/user.h`, `user/ls.c`, `kernel/fs.h`, `kernel/stat.h`, `kernel/fcntl.h`, `kernel/sysfile.c`.

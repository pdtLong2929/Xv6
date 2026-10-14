// sleep.c - Tạm dừng chương trình trong một số tick do người dùng nhập.
// Cách dùng (trong xv6):  sleep <số_tick>     ví dụ: sleep 10
// 1 tick ~ 0,1 giây trên QEMU, nên "sleep 10" dừng khoảng 1 giây.

#include "kernel/types.h"   // Kiểu dữ liệu cơ bản (uint, uint64...). Phải include TRƯỚC user.h
#include "user/user.h"      // Khai báo system call (sleep, exit...) và hàm thư viện (atoi, fprintf...)

int
main(int argc, char *argv[])
{
  // argc = số từ trên dòng lệnh, argv = mảng các từ đó (dạng chuỗi).
  // Gõ "sleep 10"  ->  argc = 2, argv[0] = "sleep", argv[1] = "10".
  // Shell tách dòng lệnh, kernel (exec) đặt sẵn argc/argv trước khi main chạy,
  // nên chương trình không cần read() để lấy số.

  // 1. Kiểm tra số đối số: phải có đúng 1 đối số là số tick.
  if(argc != 2){
    // fprintf(2, ...) in ra FD 2 = stderr (luồng báo lỗi chuẩn).
    fprintf(2, "Usage: sleep ticks\n");
    exit(1);                // Mã thoát khác 0 = có lỗi.
  }

  // 2. Kiểm tra đối số chỉ gồm chữ số 0-9.
  //    atoi("abc") trả về 0 mà không báo lỗi, nên ta tự kiểm tra trước.
  for(char *s = argv[1]; *s != '\0'; s++){   // Duyệt từng ký tự đến hết chuỗi
    if(*s < '0' || *s > '9'){
      fprintf(2, "sleep: invalid number '%s'\n", argv[1]);
      exit(1);
    }
  }

  // 3. Đổi chuỗi sang số nguyên rồi gọi system call sleep.
  //    sleep() chuyển vào kernel (sys_sleep trong kernel/sysproc.c); tiến trình
  //    ở trạng thái SLEEPING, không tốn CPU, đến khi đủ số tick thì được đánh thức.
  int ticks = atoi(argv[1]);
  sleep(ticks);

  // 4. Kết thúc thành công. Trong xv6 luôn gọi exit(), không dùng return ở main.
  exit(0);
}

// pingpong.c - Cha và con "chuyền" 1 byte qua lại bằng 2 pipe.
// Kết quả mong đợi:
//   <pid con>: received ping
//   <pid cha>: received pong
//
// Sơ đồ:
//            p2c[1] ---- byte "ping" ----> p2c[0]
//     CHA                                          CON
//            c2p[0] <--- byte "pong" ----- c2p[1]
//
// Vì sao 2 pipe? Pipe chỉ truyền MỘT CHIỀU. Nếu dùng chung 1 pipe, cha ghi xong
// rồi đọc ngay có thể tự đọc lại byte của chính mình trước khi con kịp đọc.

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p2c[2];   // pipe cha -> con: p2c[0] là đầu ĐỌC, p2c[1] là đầu GHI
  int c2p[2];   // pipe con -> cha: c2p[0] là đầu ĐỌC, c2p[1] là đầu GHI
  char b;       // 1 byte dùng để chuyền qua lại

  // 1. Tạo 2 pipe TRƯỚC khi fork, để cả cha và con đều có 4 file descriptor này.
  if(pipe(p2c) < 0 || pipe(c2p) < 0){
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }

  // 2. fork() nhân bản tiến trình. Từ dòng này có 2 tiến trình cùng chạy tiếp.
  //    Giá trị trả về: 0 ở tiến trình con, PID của con ở tiến trình cha, <0 nếu lỗi.
  int pid = fork();
  if(pid < 0){
    fprintf(2, "pingpong: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // ===================== TIẾN TRÌNH CON =====================
    // Đóng ngay các đầu ống con không dùng (con chỉ ĐỌC p2c và GHI c2p).
    close(p2c[1]);
    close(c2p[0]);

    // Đọc 1 byte từ cha. read() trên pipe rỗng sẽ CHẶN (đứng chờ)
    // cho đến khi cha ghi vào -> đây là cách 2 tiến trình đồng bộ với nhau.
    if(read(p2c[0], &b, 1) != 1){
      fprintf(2, "pingpong: child read failed\n");
      exit(1);
    }
    printf("%d: received ping\n", getpid());   // getpid() = PID của chính con

    // Gửi trả lại 1 byte cho cha.
    if(write(c2p[1], &b, 1) != 1){
      fprintf(2, "pingpong: child write failed\n");
      exit(1);
    }

    // Dọn dẹp và thoát.
    close(p2c[0]);
    close(c2p[1]);
    exit(0);
  }

  // ======================= TIẾN TRÌNH CHA =======================
  // Đóng các đầu ống cha không dùng (cha chỉ GHI p2c và ĐỌC c2p).
  close(p2c[0]);
  close(c2p[1]);

  // Gửi "ping" (1 byte bất kỳ) cho con.
  b = 'x';
  if(write(p2c[1], &b, 1) != 1){
    fprintf(2, "pingpong: parent write failed\n");
    exit(1);
  }

  // Chờ con trả lời. read() chặn đến khi con ghi vào c2p,
  // nên dòng "pong" luôn in SAU dòng "ping" mà không cần sleep.
  if(read(c2p[0], &b, 1) != 1){
    fprintf(2, "pingpong: parent read failed\n");
    exit(1);
  }
  printf("%d: received pong\n", getpid());     // PID của cha

  close(p2c[1]);
  close(c2p[0]);

  // Chờ con kết thúc hẳn, tránh để con thành "zombie" (đã thoát nhưng chưa được thu dọn).
  wait(0);
  exit(0);
}

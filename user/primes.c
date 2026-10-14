// primes.c - In các số nguyên tố từ 2 đến 280 bằng "sàng song song" (Doug McIlroy).
//
// Ý tưởng: mỗi số nguyên tố là MỘT tiến trình lọc, các tiến trình nối nhau bằng pipe:
//
//   main --2,3,4,5,...,280--> [lọc 2] --3,5,7,9,...--> [lọc 3] --5,7,11,...--> [lọc 5] --> ...
//                             in "prime 2"             in "prime 3"            in "prime 5"
//
// Mỗi tiến trình lọc:
//   1. Đọc số ĐẦU TIÊN từ pipe bên trái -> chắc chắn là số nguyên tố p -> in ra.
//      (Mọi số nhỏ hơn đều đã bị các tiến trình phía trước lọc bội của chúng.)
//   2. Đọc tiếp các số n: n chia hết cho p thì bỏ, ngược lại chuyển sang pipe bên phải.
//   3. Khi pipe trái hết dữ liệu (read trả 0), đóng pipe phải để tiến trình sau
//      cũng biết là hết, chờ nó thoát, rồi tự thoát.
//
// 3 quy tắc quan trọng:
//   - Đóng mọi file descriptor (FD) không dùng. read() chỉ trả 0 (EOF) khi MỌI đầu ghi
//     của pipe đã đóng; quên đóng -> chờ mãi -> treo. Mỗi tiến trình cũng chỉ có 16 FD.
//   - Chỉ fork khi thật sự có số cần chuyển đi (xv6 chỉ có 64 tiến trình, có 59 số
//     nguyên tố <= 280 nên chuỗi tiến trình rất sát giới hạn).
//   - Mỗi tiến trình wait() con của nó, để main chỉ thoát khi cả chuỗi đã in xong.

#include "kernel/types.h"
#include "user/user.h"

// Khai báo trước: hàm primes không bao giờ return (luôn kết thúc bằng exit).
// Thiếu dòng này gcc có thể báo lỗi "infinite recursion" (do -Werror).
void primes(int left) __attribute__((noreturn));

// primes: một tiến trình lọc.
// left = đầu ĐỌC của pipe nối với tiến trình bên trái.
void
primes(int left)
{
  int p;   // số nguyên tố của tiến trình này
  int n;   // các số đọc được sau đó

  // Bước 1: đọc số đầu tiên. Số được truyền dạng nhị phân 4 byte (sizeof(int)),
  // không phải chuỗi ký tự, nên mỗi lần read 4 byte là được đúng 1 số.
  if(read(left, &p, sizeof(p)) != sizeof(p)){
    // Không có số nào: pipe đã đóng và rỗng -> không còn gì để lọc.
    close(left);
    exit(0);
  }
  printf("prime %d\n", p);

  int right[2];      // pipe nối sang tiến trình bên phải (chỉ tạo khi cần)
  int started = 0;   // 0 = chưa tạo tiến trình bên phải, 1 = đã tạo

  // Bước 2: đọc lần lượt các số còn lại cho tới khi read trả 0 (EOF).
  while(read(left, &n, sizeof(n)) == sizeof(n)){
    if(n % p == 0)
      continue;      // n là bội của p -> không phải số nguyên tố -> bỏ

    if(!started){
      // Lần đầu tiên có số cần chuyển đi -> lúc này mới tạo pipe và tiến trình con.
      if(pipe(right) < 0){
        fprintf(2, "primes: pipe failed\n");
        exit(1);
      }
      int pid = fork();
      if(pid < 0){
        fprintf(2, "primes: fork failed\n");
        exit(1);
      }
      if(pid == 0){
        // ---- TIẾN TRÌNH CON: trở thành tiến trình lọc kế tiếp ----
        close(right[1]);   // con chỉ ĐỌC từ right, không ghi
        close(left);       // con được thừa hưởng 'left' của cha nhưng không dùng -> đóng
        primes(right[0]);  // bắt đầu lọc; hàm này không quay lại
      }
      // ---- TIẾN TRÌNH CHA (tiến trình hiện tại) ----
      close(right[0]);     // cha chỉ GHI vào right, không đọc
      started = 1;
    }

    // Chuyển số n (không chia hết cho p) sang tiến trình bên phải.
    if(write(right[1], &n, sizeof(n)) != sizeof(n)){
      fprintf(2, "primes: write failed\n");
      exit(1);
    }
  }

  // Bước 3: pipe trái đã hết số.
  close(left);
  if(started){
    close(right[1]);   // đóng đầu ghi -> tiến trình bên phải sẽ đọc được EOF
    wait(0);           // chờ tiến trình bên phải (và cả chuỗi phía sau nó) kết thúc
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int p[2];   // pipe đầu tiên: main (ghi) -> tiến trình lọc đầu tiên (đọc)

  if(pipe(p) < 0){
    fprintf(2, "primes: pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "primes: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    // Con: tiến trình lọc đầu tiên (sẽ lọc bội của 2).
    close(p[1]);    // con chỉ đọc
    primes(p[0]);   // không quay lại
  }

  // Cha (main): sinh dãy số 2..280 đưa vào pipe.
  close(p[0]);      // main chỉ ghi
  for(int i = 2; i <= 280; i++){
    if(write(p[1], &i, sizeof(i)) != sizeof(i)){   // ghi số dạng nhị phân 4 byte
      fprintf(2, "primes: write failed\n");
      exit(1);
    }
  }
  close(p[1]);      // báo "hết số" cho tiến trình lọc đầu tiên

  wait(0);          // chờ toàn bộ chuỗi tiến trình in xong rồi mới thoát,
                    // để dấu nhắc $ của shell không hiện chen giữa các dòng "prime"
  exit(0);
}

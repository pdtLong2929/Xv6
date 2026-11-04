///Mã nguồn kiểm thử
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

void testmem() {
  struct sysinfo info;
  if (sysinfo(&info) < 0) {
    printf("sysinfotest: FAIL sysinfo\n");
    exit(1);
  }
  if (info.freemem == 0) {
    printf("sysinfotest: FAIL freemem is 0\n");
    exit(1);
  }
}

void testproc() {
  struct sysinfo info;
  if (sysinfo(&info) < 0) {
    printf("sysinfotest: FAIL sysinfo\n");
    exit(1);
  }
  int n = info.nproc;
  int pid = fork();
  if (pid < 0) {
    printf("sysinfotest: FAIL fork\n");
    exit(1);
  }
  if (pid == 0) {
    sysinfo(&info);
    if (info.nproc != n + 1) {
      // Ép kiểu (int) cho info.nproc ở cuối câu lệnh
      printf("sysinfotest: FAIL nproc after fork (expected %d, got %d)\n", n + 1, (int)info.nproc);
      exit(1);
    }
    exit(0);
  }
  wait(0);
}

int main(int argc, char *argv[]) {
  printf("sysinfotest: start\n");
  testmem();
  testproc();
  printf("sysinfotest: OK\n");
  exit(0);
}
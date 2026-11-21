#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"
int main(void){
  struct usyscall *u = (struct usyscall *)USYSCALL;
  printf("getpid()=%d ugetpid()=%d\n", getpid(), ugetpid());
  int pid = fork();
  if(pid == 0){
    printf("child: getpid()=%d ugetpid()=%d\n", getpid(), ugetpid());
    printf("child: thu ghi vao USYSCALL...\n");
    u->pid = 999;
    printf("LOI: ghi duoc vao trang chi doc!\n");
    exit(0);
  }
  int st; wait(&st);
  printf("parent: child da bi kill (exit status %d)\n", st);
  exit(0);
}

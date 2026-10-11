#include "kernel/types.h"
#include "user/user.h"

void primes(int left) __attribute__((noreturn));

// Doc so tu pipe ben trai, in so dau tien (la so nguyen to),
// chuyen cac so khong chia het sang tien trinh ke tiep.
void
primes(int left)
{
  int p, n;

  if(read(left, &p, sizeof(p)) != sizeof(p)){
    close(left);          // pipe rong: het day so
    exit(0);
  }
  printf("prime %d\n", p);

  int right[2];
  int started = 0;        // da tao tien trinh ke tiep chua

  while(read(left, &n, sizeof(n)) == sizeof(n)){
    if(n % p == 0)
      continue;
    if(!started){
      pipe(right);
      if(fork() == 0){
        close(right[1]);  // con chi doc tu right[0]
        close(left);      // con khong can pipe ben trai cua cha
        primes(right[0]);
      }
      close(right[0]);    // cha chi ghi vao right[1]
      started = 1;
    }
    write(right[1], &n, sizeof(n));
  }

  close(left);
  if(started){
    close(right[1]);      // bao EOF cho tien trinh ke tiep
    wait(0);
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);

  if(fork() == 0){
    close(p[1]);
    primes(p[0]);
  }

  close(p[0]);
  for(int i = 2; i <= 280; i++)
    write(p[1], &i, sizeof(i));
  close(p[1]);            // bao het so cho tien trinh loc dau tien
  wait(0);
  exit(0);
}
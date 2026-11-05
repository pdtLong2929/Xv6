#include "kernel/ptree.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

#define MAX_PROC 64

// In thụt lề và đệ quy in các tiến trình con của parent_pid
static void print_tree(struct ptreeinfo *buf, int n, int parent_pid,
                       int depth) {
  for (int i = 0; i < n; i++) {
    if (buf[i].ppid == parent_pid) {
      for (int s = 0; s < depth * 2; s++) {
        printf(" ");
      }
      printf("%d %s state=%d mem=%d\n", buf[i].pid, buf[i].name, buf[i].state,
             (int)buf[i].memsize);
      print_tree(buf, n, buf[i].pid, depth + 1);
    }
  }
}

// Kiểm tra xem một tiến trình có phải là nút gốc trong danh sách không
static int is_root(struct ptreeinfo *buf, int n, int idx) {
  if (buf[idx].ppid == 0)
    return 1;

  for (int i = 0; i < n; i++) {
    if (buf[i].pid == buf[idx].ppid)
      return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  struct ptreeinfo buf[MAX_PROC];
  int count;

  count = ptree(buf, MAX_PROC);
  if (count < 0) {
    fprintf(2, "pstree: ptree failed\n");
    exit(1);
  }

  for (int i = 0; i < count; i++) {
    if (is_root(buf, count, i)) {
      printf("%d %s state=%d mem=%d\n", buf[i].pid, buf[i].name, buf[i].state,
             (int)buf[i].memsize);
      print_tree(buf, count, buf[i].pid, 1);
    }
  }

  exit(0);
}

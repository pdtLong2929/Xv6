#ifndef PTREE_H_
#define PTREE_H_

#include "types.h"

struct ptreeinfo {
  int pid;
  int ppid;
  int state;
  uint64 memsize;
  char name[16];
};

#endif
#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include "types.h"

struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};

#endif

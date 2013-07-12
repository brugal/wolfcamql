#ifndef cg_mem_h_included
#define cg_mem_h_included

#include "cg_local.h"

void *CG_MallocMem (size_t size);
void *CG_CallocMem (size_t size);
void CG_FreeMem (void *ptr);

#endif  // cg_mem_h_included

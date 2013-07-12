#include "cg_mem.h"

#ifdef Q3_VM
//FIXME
typedef unsigned int uintptr_t;
#endif


#ifndef Q3_VM

// native

#include <stdlib.h>

void *CG_MallocMem (size_t size)
{
    return malloc(size);
}

void *CG_CallocMem (size_t size)
{
    return calloc(1, size);
}

void CG_FreeMem (void *ptr)
{
    free(ptr);
}

#else  // ifndef Q3_VM

// virtual machine

#define MEMORY_POOL_SIZE (1024 * 1024 * 2)

static byte MemoryPool[MEMORY_POOL_SIZE];
static byte *Hint;

typedef struct mem_info_t {
    qboolean allocated;
    size_t size;
} mem_info_t;

static ID_INLINE void *AlignPtr (void *ptr, uintptr_t a)
{
    uintptr_t align;
    uintptr_t p;

    p = (uintptr_t)ptr;
    align = p % a;
    if (align) {
        p += a - align;
    }

    return (void *)p;
}

void *CG_MallocMem (size_t size)
{
    int i;
    byte *p;
    byte *possibleMatch;
    int loops;

    if (size <= 0) {
        Com_Printf("^1CG_MallocMem invalid memory size: %d\n", size);
        return NULL;
    }

    if (!Hint) {
        Hint = &MemoryPool[0];
    }

    p = Hint;
    p = AlignPtr(p, sizeof(size_t));

    loops = 0;

    while (1) {
        size_t sz;

mainloop:

        if (p >= &MemoryPool[MEMORY_POOL_SIZE]) {
            if (loops > 0) {
                // couldn't find space
                Com_Printf("^1CG_MallocMem(%d) out of memory\n", size);
                return NULL;
            } else {
                loops++;
                Hint = &MemoryPool[0];
                p = Hint;
                p = AlignPtr(p, sizeof(size_t));
            }
        }

        sz = *(size_t *)p;

        if (sz == 0) {  // possible match, check size
            qboolean allZero;

            possibleMatch = p;

            allZero = qtrue;
            for (i = 0;  i < (size / sizeof(size_t) + 1);  i++) {
                p += sizeof(size_t);

                if (p >= &MemoryPool[MEMORY_POOL_SIZE]) {
                    if (loops > 0) {
                        // couldn't find space
                        Com_Printf("^1CG_MallocMem(%d) out of memory (size)\n", size);
                        return NULL;
                    } else {
                        loops++;
                        Hint = &MemoryPool[0];
                        p = Hint;
                        p = AlignPtr(p, sizeof(size_t));
                        goto mainloop;
                    }
                }

                if (*p != 0) {
                    // not enough space
                    allZero = qfalse;
                    break;
                }
            }

            if (allZero) {
                // got it
                *((size_t *)possibleMatch) = size;
                possibleMatch += sizeof(size_t);
                Hint = possibleMatch + size;
                return possibleMatch;
            } else {
                Com_Printf("pass...\n");
                // keep trying
                // pass
                sz = *(size_t *)p;
                p += sz;
                p = AlignPtr(p, sizeof(size_t));
            }
        } else {
            //Com_Printf("skipping %p  size %d\n", p, sz);
            p += sizeof(size_t);
            p += sz;
            p = AlignPtr(p, sizeof(size_t));
        }
    }

    //FIXME defragment memory

    return NULL;
}

void *CG_CallocMem (size_t size)
{
    return CG_MallocMem(size);
}

void CG_FreeMem (void *ptr)
{
    size_t *szPtr;
    size_t sz;

    if ((byte *)ptr < &MemoryPool[0]  ||  (byte *)ptr >= &MemoryPool[MEMORY_POOL_SIZE]) {
        Com_Printf("^1CG_FreeMem invalid pointer %p, not in memory pool %p -> %p\n", ptr, &MemoryPool[0], &MemoryPool[MEMORY_POOL_SIZE]);
        return;
    }

    szPtr = ptr;
    if ((uintptr_t)szPtr % sizeof(size_t)) {
        Com_Printf("^1CG_FreeMem invalid ptr alignment %d  %p\n", (uintptr_t)szPtr % sizeof(size_t), ptr);
        return;
    }

    sz = *(szPtr - 1);
    if ((byte *)ptr + sz >= &MemoryPool[MEMORY_POOL_SIZE]) {
        Com_Printf("^1CG_FreeMem invalid size %d, overflows memory pool\n", sz);
        return;
    }

    memset(ptr, 0, sz);
    *(szPtr - 1) = 0;

    //Com_Printf("^3CG_FreeMem %p size %d\n", ptr, sz);
}

#endif  // ifndef Q3_VM

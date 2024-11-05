#include "allocator.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

struct blk_allocator *blka_new(void)
{
    return calloc(1, sizeof(struct blk_allocator));
}

size_t align(size_t size)
{
    size_t reste = (size % sizeof(long double));
    if (!reste)
    {
        return size;
    }
    size_t ret;
    if (__builtin_add_overflow(size, sizeof(long double) - reste, &ret))
    {
        return 0;
    }
    else
    {
        return ret;
    }
}

struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    size_t pSize = sysconf(_SC_PAGE_SIZE);
    size_t fSize = 0;
    while (fSize < size + sizeof(struct blk_meta))
    {
        fSize += pSize;
    }
    void *addr = mmap(NULL, fSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
    {
        return NULL;
    }
    struct blk_meta *blkM = addr;
    blkM->next = blka->meta;
    blkM->size = fSize - sizeof(struct blk_meta);
    blka->meta = blkM;
    return blkM;
}

void blka_free(struct blk_meta *blk)
{
    int returnVal = munmap(blk, blk->size + sizeof(struct blk_meta));
    while (returnVal == -1)
    {
        returnVal = munmap(blk, blk->size + sizeof(struct blk_meta));
    }
}

void blka_pop(struct blk_allocator *blka)
{
    if (!blka || !blka->meta)
    {
        return;
    }
    struct blk_meta *save = blka->meta;
    blka->meta = blka->meta->next;
    blka_free(save);
}

void blka_delete(struct blk_allocator *blka)
{
    while (blka->meta)
    {
        blka_pop(blka);
    }
    free(blka);
}

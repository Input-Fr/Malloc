#include "my_recycler.h"

struct recycler *recycler_create(size_t block_size, size_t total_size)
{
    if (!block_size || !total_size || block_size % sizeof(size_t) != 0)
    {
        return NULL;
    }
    struct recycler *recy = malloc(sizeof(struct recycler));
    if (total_size % block_size != 0 || !recy)
    {
        return NULL;
    }
    recy->capacity = total_size / block_size;
    recy->chunk = malloc(block_size * total_size);
    if (!recy->chunk)
    {
        free(recy);
        return NULL;
    }
    size_t idx = 1;
    recy->free = recy->chunk;
    struct free_list *fl = recy->free;
    fl->next = NULL;
    struct free_list *traveler = fl;
    while (idx < recy->capacity)
    {
        char *nextBlock = fl;
        traveler = nextBlock + block_size;
        traveler->next = NULL;
        fl->next = traveler;
        fl = traveler;
        idx++;
    }
    return recy;
}

void recycler_destroy(struct recycler *r)
{
    if (!r)
    {
        return;
    }
    free(r->chunk);
    free(r);
}

void *recycler_allocate(struct recycler *r)
{
    if (!r || !r->free)
    {
        return r;
    }
    struct free_list *fl = r->free;
    r->free = fl->next;
    char *flChar = fl;
    void *newBlock = flChar + sizeof(struct free_list *);
    return r->free;
}

void recycler_free(struct recycler *r, void *block)
{
    if (!r || !block)
    {
        return;
    }
    char *ptrToMeta = block - sizeof(struct free_list *);
    struct free_list *fl = ptrToMeta;
    fl->next = r->free;
    r->free = fl;
}









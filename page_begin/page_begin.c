#include "page_begin.h"

void *page_begin(void *ptr, size_t page_size)
{
    if (!ptr)
    {
        return NULL;
    }
    return (void *)((size_t)(ptr) & ~(page_size - 1));
}

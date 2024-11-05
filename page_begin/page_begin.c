#include "page_begin.h"

void *page_begin(void *ptr, size_t page_size)
{
    if (!ptr)
    {
        return NULL;
    }
    char *ptr2 = ptr;
    size_t addr = (size_t)ptr;
    while (addr > page_size)
    {
        addr -= page_size;
    }
    ptr2 -= addr;
    return ptr2;
}

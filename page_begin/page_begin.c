#include "page_begin.h"

void *page_begin(void *ptr, size_t page_size)
{
    if (!ptr)
    {
        return NULL;
    }
    size_t res = ((size_t)(ptr) % page_size);
    char *ptr2 = ptr;
    ptr2 -= res;
    void *bordel = ptr2;
    return bordel;
}

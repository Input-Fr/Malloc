#include "beware_overflow.h"

void *beware_overflow(void *ptr, size_t nmemb, size_t size)
{
    size_t add = 0;
    if (!__builtin_mul_overflow(nmemb, size, &add))
    {
        char *ptdr = ptr;
        ptdr += add;
        ptr = ptdr;
        return ptr;
    }
    else
    {
        return NULL;
    }
}

#include "alignment.h"

size_t align(size_t size)
{
    size -= (size % sizeof(long double));
    size_t ret;
    if (__builtin_add_overflow(size, sizeof(long double), &ret))
    {
        return 0;
    }
    else
    {
        return ret;
    }
}

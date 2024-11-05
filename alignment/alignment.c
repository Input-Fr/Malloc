#include "alignment.h"

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

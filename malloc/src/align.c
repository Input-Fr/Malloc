#include <stddef.h>
#include <unistd.h>

size_t align(size_t size)
{
    size_t reste = (size % sysconf(_SC_PAGE_SIZE));
    if (!reste)
    {
        return size;
    }

    size_t ret;
    if (__builtin_add_overflow(size, sysconf(_SC_PAGE_SIZE) - reste, &ret))
    {
        return 0;
    }
    else
    {
        return ret;
    }
}

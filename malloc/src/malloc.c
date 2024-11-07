#define _GNU_SOURCE

#include <err.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "align.h"

struct block
{
    size_t curBlockSize;
    size_t remainingSize;
    void *ptrToPage;
};

struct metaBlock
{
    unsigned int status : 1; // 1-bit field : 0 = used, 1 = free
    unsigned int newPage : 1; // 1-bit field : 0 = inPage, 1 = atBeginingOfPage
    size_t blockSize;
    struct metaBlock *next;
    struct metaBlock *prev;
};

struct block b = { .curBlockSize = 0, .remainingSize = 0, .ptrToPage = NULL };

static struct metaBlock *findMemSpace(size_t wSize, struct metaBlock **traveler)
{
    struct metaBlock *wantedBlock = NULL;
    size_t curSize = 0;
    curSize--;
    while ((*traveler)->next)
    {
        if ((*traveler)->status)
        {
            if ((*traveler)->blockSize >= wSize)
            {
                wantedBlock = curSize > wSize ? (*traveler) : wantedBlock;
                curSize = curSize > wSize ? wSize : curSize;
            }
        }
        (*traveler) = (*traveler)->next;
    }
    // check if last is correct
    if ((*traveler)->status)
    {
        if ((*traveler)->blockSize >= wSize)
        {
            wantedBlock = curSize > wSize ? (*traveler) : wantedBlock;
            curSize = curSize > wSize ? wSize : curSize;
        }
    }
    return wantedBlock;
}

static char *newMetaBlock(struct metaBlock *traveler, size_t size)
{
    void *interPtr = traveler;
    char *tmpPtr = interPtr;
    tmpPtr += traveler->blockSize + sizeof(struct metaBlock);
    interPtr = tmpPtr;
    struct metaBlock *mB = interPtr;
    mB->status = 0;
    mB->newPage = 0;
    mB->blockSize = size;
    mB->next = NULL;
    mB->prev = traveler;
    traveler->next = mB;
    return tmpPtr;
}

static char *createFirstBlock(void *addr, size_t alignedSize, size_t size)
{
    b.ptrToPage = addr;
    b.curBlockSize = alignedSize;
    b.remainingSize = alignedSize - (size + sizeof(struct metaBlock));
    struct metaBlock *mB = b.ptrToPage;
    mB->status = 0; // used
    mB->newPage = 1; // begining of the newPage
    mB->blockSize = size;
    void *interPtr = mB; // cast to void so we can cast to char
    char *tmpPtr = interPtr; // cast to char so we can increment
    mB->next = NULL; // last elt, no next
    mB->prev = NULL; // firstBlock nothing before
    return tmpPtr;
}

static void splitSpace(struct metaBlock **mB, size_t size)
{
    if ((*mB)->blockSize
        > size + sizeof(struct metaBlock)) // if space is too big
    {
        void *interPtr = (*mB);
        char *beginNewPtr = interPtr;
        beginNewPtr += size + sizeof(struct metaBlock);
        interPtr = beginNewPtr;
        struct metaBlock *nmB = interPtr;
        nmB->status = 1;
        nmB->newPage = 0;
        nmB->blockSize = (*mB)->blockSize - (size + sizeof(struct metaBlock));
        nmB->next = (*mB)->next;
        nmB->prev = (*mB);
        if ((*mB)->next)
        {
            (*mB)->next->prev = nmB;
        }
        (*mB)->next = nmB;
        (*mB)->blockSize = size;
    }
}

static void reallocBlock(size_t size, struct metaBlock **traveler)
{
    size_t newSize = align(size + sizeof(struct metaBlock));
    void *ptrToPage = mmap(NULL, newSize, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptrToPage == MAP_FAILED)
    {
        return;
    }
    b.curBlockSize += newSize;
    b.remainingSize += (newSize - size - sizeof(struct metaBlock));

    struct metaBlock *mousse = b.ptrToPage;
    while (mousse->next)
    {
        mousse = mousse->next;
    }
    *traveler = mousse;
    struct metaBlock *mB = ptrToPage;
    mB->status = 0;
    mB->newPage = 1;
    mB->blockSize = size;
    mB->next = NULL;
    mB->prev = *traveler;
    (*traveler)->next = mB;
}

size_t align16(size_t size)
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

char *foundMem(struct metaBlock *mBfind, size_t size)
{
    splitSpace(&mBfind, size); // improvement : split when free space
    mBfind->status = 0; // too big
    void *interPtr = mBfind;
    char *tmpPtr = interPtr;
    return tmpPtr + sizeof(struct metaBlock);
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    size = align16(size);
    if (PTRDIFF_MAX < size)
    {
        return NULL;
    }
    if (b.ptrToPage == NULL)
    {
        size_t alignedSize = align(size + sizeof(struct metaBlock));
        if (!alignedSize)
        {
            return NULL;
        }
        void *addr = mmap(NULL, alignedSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (addr == MAP_FAILED)
        {
            return NULL;
        }
        char *tmpPtr = createFirstBlock(addr, alignedSize, size);
        return tmpPtr + sizeof(struct metaBlock);
    }
    else
    {
        struct metaBlock *traveler = b.ptrToPage;
        struct metaBlock *mBfind = findMemSpace(size, &traveler);
        if (!mBfind) // no free, create new one
        {
            if (b.remainingSize < size + sizeof(struct metaBlock))
            { // if not enough space, "realloc"
                reallocBlock(size, &traveler);
                if (traveler->next == NULL)
                {
                    return NULL;
                }
                void *p = traveler->next;
                char *tmpPtr = p;
                tmpPtr += sizeof(struct metaBlock);
                return tmpPtr;
            }
            b.remainingSize -= (size + sizeof(struct metaBlock));
            char *tmpPtr = newMetaBlock(traveler, size);
            return tmpPtr + sizeof(struct metaBlock);
        }
        else // one large enough memory was found
        {
            return foundMem(mBfind, size);
        }
    }
}

/*
#include <stdio.h>

__attribute__((visibility("default"))) void print(void)
{
    printf("MAIN BLOCK : b.curBlockSize = %zu\n b.remainingSize = %zu\n",
            b.curBlockSize, b.remainingSize);
    struct metaBlock *mB = b.ptrToPage;
    while (mB)
    {
        printf("META BLOCK : status = %d\n blockSize = %zu\n\n", mB->status,
                mB->blockSize);
        mB = mB->next;
    }
}*/

void mergeFree(struct metaBlock *mB)
{
    if (mB->next && mB->next->status && !mB->next->newPage)
    {
        mB->blockSize += mB->next->blockSize + sizeof(struct metaBlock);
        if (mB->next->next)
        {
            mB->next->next->prev = mB;
        }
        mB->next = mB->next->next;
    }
    if (mB->prev && mB->prev->status && !mB->newPage)
    {
        struct metaBlock *mBn = mB->prev;
        mBn->blockSize += mBn->next->blockSize + sizeof(struct metaBlock);
        if (mBn->next->next)
        {
            mBn->next->next->prev = mBn;
        }
        mBn->next = mBn->next->next;
    }
}

static void unallocAll(void)
{
    struct metaBlock *traveler = b.ptrToPage;
    struct metaBlock *next = NULL;
    int up = 0;
    while (traveler)
    {
        if (traveler->newPage && traveler->status) // free and page begining
        {
            if (!traveler->next || (traveler->next && traveler->next->newPage))
            {
                if (!traveler->prev) // if firstBlock, redirect block pointer
                {
                    b.ptrToPage = traveler->next;
                }
                if (traveler->prev) // update next
                {
                    traveler->prev->next = traveler->next;
                }
                if (traveler->next) // update prev
                {
                    traveler->next->prev = traveler->prev;
                }
                size_t totSz = traveler->blockSize + sizeof(struct metaBlock);
                size_t curPageSize = align(totSz);
                b.curBlockSize -= curPageSize;
                b.remainingSize -= (curPageSize - totSz);
                next = traveler->next;
                up = 1;
                munmap(traveler, curPageSize);
            }
        }
        if (up)
        {
            traveler = next;
            up = 0;
        }
        else
        {
            traveler = traveler->next;
        }
    }
    if (b.ptrToPage == NULL)
    {
        b.remainingSize = 0;
        b.curBlockSize = 0;
    }
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    char *tmpPtr = ptr;
    tmpPtr -= sizeof(struct metaBlock);
    void *interPtr = tmpPtr;
    struct metaBlock *mB = interPtr;
    mB->status = 1;
    mergeFree(mB); // improvement : merge if two neighbors are free
    unallocAll();
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (!ptr || !size)
    {
        return NULL;
    }
    return NULL;
    /*
    if (!size)
    {
        free(ptr);
        return NULL;
    }
    if (!ptr)
    {
        return malloc(size);
    }
    char *tmpPtr = ptr;
    tmpPtr -= sizeof(struct metaBlock);
    void *interPtr = tmpPtr;
    struct metaBlock *oldMeta = interPtr;
    if (oldMeta->blockSize >= size)
    {
        return ptr;
    }
    void *newAl = malloc(size);
    if (!newAl)
    {
        return NULL;
    }
    tmpPtr = newAl;
    tmpPtr -= sizeof(struct metaBlock);
    interPtr = tmpPtr;
    struct metaBlock *newMeta = interPtr;
    if (oldMeta->blockSize > newMeta->blockSize)
    {
        memcpy(newAl, ptr, size);
    }
    else
    {
        memcpy(newAl, ptr, oldMeta->blockSize);
    }
    free(ptr);
    return newAl;
    */
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    if (__builtin_mul_overflow(nmemb, size, &size))
    {
        return NULL;
    }
    if (!size || !nmemb)
    {
        return malloc(0);
    }
    void *mem = malloc(nmemb * size);
    memset(mem, 0, nmemb * size);
    return mem;
}

#include "ide-common-util-stub.h"

void *IdeXmalloc (int size) {
    return malloc(size);
}

void IdeXfree (void *ptr) {
    if (ptr != NULL)
        free (ptr);
}

void *GetIdeDaemonHdcClient()
{
    return NULL;
}

#include "ide-cmd-stub.h"
#include <memory>
int CommandRes(struct IdeSockHandle, int cmd_or_file) {
    return 0;
}

void* IdeXmalloc(size_t size)
{
    if (size == 0) {
        return nullptr;
    }

    void* val = malloc(size);
    if (val == nullptr) {
        return nullptr;
    }

    return val;
}

/**
 * @brief free memory
 * @param ptr: the memory to free
 *
 * @return
 */
void IdeXfree(void* ptr)
{
    if (ptr != nullptr) {
        free(ptr);
        ptr = nullptr;
    }
}


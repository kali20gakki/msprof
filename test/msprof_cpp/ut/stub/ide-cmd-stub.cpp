#include <memory>

#include "mmpa/mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace Adx {
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
        FREE_BUF(ptr);
    }
}
} // namespace Adx
} // namespace Dvvp
} // namespace Analysis

#include "profiling/ge_profiling.h"

using aclrtStream = void *;
ge::Status ProfSetStepInfo(uint64_t index_id, uint16_t tag_id, aclrtStream stream)
{
    return ge::SUCCESS;
}

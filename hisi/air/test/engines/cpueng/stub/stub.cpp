#include "stub.h"
#include "common/aicore_util_types.h"

using namespace aicpu;
using namespace ge;
using namespace std;

RunContext CreateContext()
{
    rtStream_t stream = nullptr;
    rtModel_t model = nullptr;

    assert(rtStreamCreate(&stream, 0) == RT_ERROR_NONE);
    assert(rtModelCreate(&model, 0) == RT_ERROR_NONE);
    assert(rtModelBindStream(model, stream, 0) == RT_ERROR_NONE);

    RunContext context;
    context.model = model;
    context.stream = stream;
    context.dataMemSize = 10000;
    context.dataMemBase = (uint8_t *) (intptr_t) 1000;
    context.sessionId = 10000011011;

    return context;
}

void DestroyContext(RunContext& context)
{
    assert(rtModelUnbindStream(context.model, context.stream) == RT_ERROR_NONE);
    assert(rtModelDestroy(context.model) == RT_ERROR_NONE);
    assert(rtStreamDestroy(context.stream) == RT_ERROR_NONE);
}

TestForGetSoPath &TestForGetSoPath::Instance()
{
    static TestForGetSoPath instance;
    return instance;
}

namespace fe {
class OpSliceUtil {
 public:
  explicit OpSliceUtil();
  virtual ~OpSliceUtil();
  static Status SetOpSliceInfo(const ge::NodePtr &node, const SlicePattern &slice_pattern,
                               const bool &support_stride_write);
};

Status OpSliceUtil::SetOpSliceInfo(const ge::NodePtr &node,
                      const SlicePattern &slice_pattern,
                      const bool &support_stride_write) {
  if (node == nullptr) {
    return FAILED;
  }
  return SUCCESS;
}
};
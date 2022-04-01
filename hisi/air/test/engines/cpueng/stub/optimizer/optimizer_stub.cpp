#include "optimizer_stub.h"
#include <mutex>

using namespace ge;
using namespace std;

namespace aicpu {
OptimizerPtr OptimzierStub::instance_ = nullptr;

OptimizerPtr OptimzierStub::Instance()
{
    static once_flag flag;
    call_once(flag, [&]() {
        instance_.reset(new (nothrow) OptimzierStub);
    });
    return instance_;
}

// FACTORY_GRAPH_OPTIMIZER_CLASS_KEY(OptimzierStub, "stub")
}
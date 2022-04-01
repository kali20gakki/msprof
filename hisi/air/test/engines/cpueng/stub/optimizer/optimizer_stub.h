#ifndef OPTIMIZER_STUB_H
#define OPTIMIZER_STUB_H
#include "common/aicpu_graph_optimizer/optimizer.h"

namespace aicpu {
using OptimizerPtr = std::shared_ptr<Optimizer>;

class OptimzierStub : public Optimizer {
public:
    /**
     * Destructor
     */
    virtual ~OptimzierStub() = default;

    static OptimizerPtr Instance();

private:
    // singleton instance
    static OptimizerPtr instance_;
};
}

#endif
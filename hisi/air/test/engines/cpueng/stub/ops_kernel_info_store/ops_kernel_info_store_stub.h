#ifndef OPS_KERNEL_INFO_STORE_STUB_H
#define OPS_KERNEL_INFO_STORE_STUB_H
#include "common/aicpu_ops_kernel_info_store/kernel_info.h"

namespace aicpu {
using KernelInfoPtr = std::shared_ptr<KernelInfo>;

class OpskernelInfoStoreStub : public KernelInfo {
public:
    /**
     * Destructor
     */
    virtual ~OpskernelInfoStoreStub() = default;

    static KernelInfoPtr Instance();
protected:
    bool ReadOpInfoFromJsonFile() override;

private:
    // singleton instance
    static KernelInfoPtr instance_;
};
}

#endif
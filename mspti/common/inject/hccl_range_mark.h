/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_range_mark.h
 * Description        : hccl自动打点类
 * Author             : msprof team
 * Creation Date      : 2024/11/7
 * *****************************************************************************
*/
#ifndef MSPTI_PROJECT_HCCL_RANGE_MARK_H
#define MSPTI_PROJECT_HCCL_RANGE_MARK_H

#include <memory>
#include "activity/ascend/entity/hccl_op_desc.h"
#include "activity/ascend/parser/parser_manager.h"

namespace Mspti {
namespace Parser {
class InnerMark {
public:
    InnerMark() = default;
    virtual ~InnerMark() = default;
protected:
    uint64_t markId{0};
};

class HcclInnerMark : public InnerMark {
public:
    HcclInnerMark(aclrtStream stream, HcclComm comm, std::shared_ptr<HcclOpDesc> opDesc);
    ~HcclInnerMark() override;
};

class HcclMarkFactory {
public:
    static std::unique_ptr<InnerMark> createMarker(aclrtStream stream, HcclComm comm,
                                                   std::shared_ptr<HcclOpDesc> opDesc);
};
}
}

#endif // MSPTI_PROJECT_HCCL_RANGE_MARK_H

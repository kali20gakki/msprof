/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/
#ifndef MSPTI_PROJECT_HCCL_RANGE_MARK_H
#define MSPTI_PROJECT_HCCL_RANGE_MARK_H

#include <memory>
#include "csrc/activity/ascend/entity/hccl_op_desc.h"
#include "csrc/activity/ascend/parser/parser_manager.h"

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

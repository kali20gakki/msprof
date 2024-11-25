/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_range_mark.cpp
 * Description        : hccl自动打点类
 * Author             : msprof team
 * Creation Date      : 2024/11/7
 * *****************************************************************************
*/
#include "hccl_range_mark.h"
#include "activity/ascend/parser/parser_manager.h"
#include "common/utils.h"
#include "activity/activity_manager.h"
#include "activity/ascend/parser/hccl_reporter.h"

namespace Mspti {
namespace Parser {
using namespace Mspti::Activity;
HcclInnerMark::HcclInnerMark(aclrtStream stream, HcclComm comm, std::shared_ptr<HcclOpDesc> opDesc)
{
    char markMsg[] = "msptiInnerMark";
    char commName[COMM_NAME_MAX_LENGTH];
    HcclGetCommName(comm, commName);
    opDesc->commName.assign(commName);
    Mspti::Parser::ParserManager::GetInstance()->InnerDeviceStartA(markMsg, stream, markId);
    Mspti::Parser::HcclReporter::GetInstance()->RecordHcclOp(markId, opDesc);
};

HcclInnerMark::~HcclInnerMark()
{
    Mspti::Parser::ParserManager::GetInstance()->InnerDeviceEndA(markId);
}

std::unique_ptr<InnerMark> HcclMarkFactory::createMarker(aclrtStream stream, HcclComm comm,
                                                         std::shared_ptr<HcclOpDesc> opDesc)
{
    if (ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_HCCL)) {
        return std::move(static_cast<std::unique_ptr<InnerMark>>(
            std::make_unique<HcclInnerMark>(stream, comm, opDesc)));
    }
    return std::make_unique<InnerMark>();
}
}
};


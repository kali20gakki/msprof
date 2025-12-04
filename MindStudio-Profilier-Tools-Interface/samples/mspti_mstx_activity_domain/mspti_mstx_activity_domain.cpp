/**
* @file mspti_mstx_activity_domain.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* Sample to demonstrate the usage of the msptiActivity Enable Marker Domain API.
* Users can control the collection of marker data for different domains by calling msptiActivity Enable Marker Domain
*/

// System header
#include <vector>
#include <thread>
#include <cstring>
#include <string>

// Acl header
#include "acl/acl.h"
#include "aclnnop/aclnn_add.h"
#include "common/util_acl.h"

// MSPTI header
#include "mspti.h"
#include "common/helper_mspti.h"

// MSTX header
#include "mstx/ms_tools_ext.h"

aclrtContext context;
aclrtStream stream;
mstxDomainHandle_t domainRange;
std::string g_domainRangeName = "domainRange";

static void TestMstx()
{
    aclrtSetCurrentContext(context);
    for (int i = 0; i < 1; i++) {
        // By default, mark activity are collected
        uint64_t id = mstxDomainRangeStartA(domainRange, "s.c_str()", stream);
        mstxDomainRangeEnd(domainRange, id);

        // close target domain makr collect
        msptiActivityDisableMarkerDomain(g_domainRangeName.c_str());
        id = mstxDomainRangeStartA(domainRange, "closeDomainMark", stream);
        mstxDomainRangeEnd(domainRange, id);

        msptiActivityEnableMarkerDomain(g_domainRangeName.c_str());
        id = mstxDomainRangeStartA(domainRange, "openDomainMark", nullptr);
        mstxDomainRangeEnd(domainRange, id);
    }
}

void MstxDomainInit()
{
    domainRange = mstxDomainCreateA(g_domainRangeName.c_str());
}

void MstxDomainDeInit()
{
    mstxDomainDestroy(domainRange);
}

void SetUpMspti()
{
    InitMspti(nullptr, nullptr);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER);
}

int main(int argc, const char **argv)
{
    int32_t deviceId = 6;
    SetUpMspti();
    Init(deviceId, &context, &stream);

    MstxDomainInit();
    TestMstx();
    DeInit(deviceId, &context, &stream);
    MstxDomainDeInit();

    DeInitMspti();
    return 0;
}

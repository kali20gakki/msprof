/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: load dev manager api
 * Author:
 * Create: 2019-12-20
 */
#include "transport/hdc/dev_mgr_api.h"
#include "msprof_dlog.h"
#include "transport/hdc/device_transport.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
void LoadDevMgrAPI(DevMgrAPI &devMgrAPI)
{
    MSPROF_LOGI("LoadDevMgrAPI init begin");
    devMgrAPI.pfDevMgrInit = &DevTransMgr::InitDevTransMgr;
    devMgrAPI.pfDevMgrUnInit = &DevTransMgr::UnInitDevTransMgr;
    devMgrAPI.pfDevMgrCloseDevTrans = &DevTransMgr::CloseDevTrans;
    devMgrAPI.pfDevMgrGetDevTrans = &DevTransMgr::GetDevTrans;
    MSPROF_LOGI("LoadDevMgrAPI init end");
}
}}}

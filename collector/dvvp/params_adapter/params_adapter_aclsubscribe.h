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
 * -------------------------------------------------------------------------*/
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_ACLSUBSCIRBE_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_ACLSUBSCIRBE_H

#include "params_adapter.h"


namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

class ParamsAdapterAclSubscribe : public ParamsAdapter {
public:
    ParamsAdapterAclSubscribe() : params_(nullptr) {};
    int GetParamFromInputCfg(PROF_SUB_CONF_CONST_PTR profSubscribeConfig,
                             const uint64_t dataTypeConfig,
                             SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void GenAclSubscribeContainer(const ProfAicoreMetrics aicMetrics, const uint64_t dataTypeConfig);

private:
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    SHARED_PTR_ALIA<ProfileParams> params_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif
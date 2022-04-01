/**
 * @file adx_get_file.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_LOG_HDC_H
#define ADX_LOG_HDC_H
#include "adx_component.h"
#include "adx_comm_opt_manager.h"
#include "common_utils.h"
namespace Adx {
using LogNotifyMsg = struct {
    int timeout;
    char data[0];
};

class AdxLogHdc : public AdxComponent {
public:
    ~AdxLogHdc() override {}
    int32_t Init() override;
    const std::string GetInfo() override { return "Log hdc server process"; }
    ComponentType GetType() override { return ComponentType::COMPONENT_LOG; }
    int32_t Process(const CommHandle &handle,
        const SharedPtr<MsgProto> &proto) override;
    int32_t UnInit() override;
};
}
#endif

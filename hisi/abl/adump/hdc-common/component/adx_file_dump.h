/**
 * @file adx_file_dump.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_FILE_DUMP_COMPONENT_H
#define ADX_FILE_DUMP_COMPONENT_H
#include "adx_component.h"
#include "adx_comm_opt_manager.h"
#include "common_utils.h"
namespace Adx {
class AdxFileDump : public AdxComponent {
public:
    ~AdxFileDump() override {}
    int32_t Init() override;
    const std::string GetInfo() override { return "TransferFile"; }
    ComponentType GetType() override { return ComponentType::COMPONENT_GETD_FILE; }
    int32_t Process(const CommHandle &handle,
        const SharedPtr<MsgProto> &proto) override;
    int32_t UnInit() override;
};
}
#endif
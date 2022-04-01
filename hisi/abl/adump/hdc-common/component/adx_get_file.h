/**
 * @file adx_get_file.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_GET_FILE_COMPONENT_H
#define ADX_GET_FILE_COMPONENT_H
#include "adx_component.h"
#include "adx_comm_opt_manager.h"
#include "common_utils.h"
namespace Adx {
const std::string SEND_END_MSG = "game_over";

constexpr int NON_DOCKER     = 1;
constexpr int IS_DOCKER      = 2;
constexpr int VM_NON_DOCKER  = 3;
class AdxGetFile : public AdxComponent {
public:
    ~AdxGetFile() override {}
    int32_t Init() override;
    const std::string GetInfo() override { return "TransferFile"; }
    ComponentType GetType() override { return ComponentType::COMPONENT_GETD_FILE; }
    int32_t Process(const CommHandle &handle,
        const SharedPtr<MsgProto> &proto) override;
    int32_t UnInit() override;
private:
    int32_t TransferFile(const CommHandle &handle, const std::string &filePath, int pid);
    int32_t GetFileList(const std::string &path, std::vector<std::string> &list);
    int32_t GetPathPrefix(const std::string &tmpFilePath, std::string &matchStr);
    int32_t PrepareMessagesFile(std::string &messagesFilePath);
};
}
#endif
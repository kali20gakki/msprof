/**
 * @file hdc_comm_opt.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef HDC_COMM_OPT_H
#define HDC_COMM_OPT_H
#include "adx_comm_opt.h"
namespace Adx {
class HdcCommOpt : public AdxCommOpt {
public:
    HdcCommOpt() {}
    ~HdcCommOpt() override {}
    std::string CommOptName() override;
    OptType GetOptType() override;
    OptHandle OpenServer(const std::map<std::string, std::string> &info) override;
    int32_t CloseServer(const OptHandle &handle) const override;
    OptHandle Accept(const OptHandle handle) const override;
    OptHandle Connect(const OptHandle handle, const std::map<std::string, std::string> &info) override;
    int32_t Write(const OptHandle handle, IdeSendBuffT buffer, int32_t length, int32_t flag) override;
    int32_t Read(const OptHandle handle, IdeRecvBuffT buffer, int32_t &length, int32_t flag) override;
    OptHandle OpenClient(const std::map<std::string, std::string> &info) override;
    int32_t CloseClient(OptHandle &handle) const override;
    int32_t Close(OptHandle &handle) const override;
    SharedPtr<AdxDevice> GetDevice() override;
    void Timer(void) const override;
};
}
#endif

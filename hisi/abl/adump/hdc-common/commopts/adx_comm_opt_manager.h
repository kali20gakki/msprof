/**
 * @file adx_comm_opt_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_COMMON_COMMOPT_MANAGER_H
#define ADX_COMMON_COMMOPT_MANAGER_H
#include <memory>
#include <mutex>
#include <map>
#include "adx_comm_opt.h"
#include "common/singleton.h"
#include "common/common_utils.h"
namespace Adx {
#define ADX_COMMOPT_INVALID_HANDLE(type) {(type), ADX_OPT_INVALID_HANDLE}
struct CommHandle {
    OptType type;
    OptHandle session;
};

class AdxCommOptManager : public Adx::Common::Singleton::Singleton<AdxCommOptManager> {
public:
    ~AdxCommOptManager();
    bool CommOptsRegister(std::unique_ptr<AdxCommOpt> &opt);
    CommHandle OpenServer(OptType type, const std::map<std::string, std::string> &info);
    int32_t CloseServer(const CommHandle &handle) const;
    CommHandle OpenClient(OptType type, const std::map<std::string, std::string> &info);
    int32_t CloseClient(CommHandle &handle) const;
    CommHandle Accept(const CommHandle &handle) const;
    CommHandle Connect(const CommHandle &handle, const std::map<std::string, std::string> &info);
    int32_t Close(CommHandle &handle) const;
    int32_t Write(const CommHandle &handle, IdeSendBuffT buffer, int32_t length, int32_t flag);
    int32_t Read(const CommHandle &handle, IdeRecvBuffT buffer, int32_t &length, int32_t flag);
    SharedPtr<AdxDevice> GetDevice(OptType type);
    void Timer(OptType type) const;
private:
    std::map<OptType, std::unique_ptr<AdxCommOpt>> commOptMap_;
    std::mutex commOptMapMtx_;
};
}
#endif
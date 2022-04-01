/**
 * @file adx_comm_opt_manager.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_comm_opt_manager.h"
#include "log/adx_log.h"
namespace Adx {
AdxCommOptManager::~AdxCommOptManager()
{
    commOptMap_.clear();
}


bool AdxCommOptManager::CommOptsRegister(std::unique_ptr<AdxCommOpt> &opt)
{
    if (opt == nullptr) {
        return false;
    }

    {
        std::unique_lock<std::mutex> lock {commOptMapMtx_};
        if (commOptMap_.find(opt->GetOptType()) == commOptMap_.end()) {
            commOptMap_[opt->GetOptType()] = std::move(opt);
            return true;
        }
    }

    IDE_LOGW("have been commopt registered");
    return true;
}

CommHandle AdxCommOptManager::OpenServer(OptType type, const std::map<std::string, std::string> &info)
{
    auto it = commOptMap_.find(type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        OptHandle opHandle = it->second->OpenServer(info);
        return {type, opHandle};
    }

    IDE_LOGW("commopt(%d) not registered", type);
    return ADX_COMMOPT_INVALID_HANDLE(type);
}

int32_t AdxCommOptManager::CloseServer(const CommHandle &handle) const
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->CloseServer(handle.session);
    }
    return IDE_DAEMON_OK;
}

CommHandle AdxCommOptManager::OpenClient(OptType type, const std::map<std::string, std::string> &info)
{
    auto it = commOptMap_.find(type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        OptHandle opHandle = it->second->OpenClient(info);
        return {type, opHandle};
    }

    return ADX_COMMOPT_INVALID_HANDLE(type);
}

int32_t AdxCommOptManager::CloseClient(CommHandle &handle) const
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->CloseClient(handle.session);
    }
    return IDE_DAEMON_OK;
}

CommHandle AdxCommOptManager::Accept(const CommHandle &handle) const
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        OptHandle opHandle = it->second->Accept(handle.session);
        return {handle.type, opHandle};
    }

    return ADX_COMMOPT_INVALID_HANDLE(handle.type);
}

CommHandle AdxCommOptManager::Connect(const CommHandle &handle, const std::map<std::string, std::string> &info)
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        OptHandle opHandle = it->second->Connect(handle.session, info);
        return {handle.type, opHandle};
    }

    return ADX_COMMOPT_INVALID_HANDLE(handle.type);
}

int32_t AdxCommOptManager::Close(CommHandle &handle) const
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->Close(handle.session);
    }
    return IDE_DAEMON_OK;
}

int32_t AdxCommOptManager::Write(const CommHandle &handle, IdeSendBuffT buffer, int32_t length, int32_t flag)
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->Write(handle.session, buffer, length, flag);
    }

    return -1;
}

int32_t AdxCommOptManager::Read(const CommHandle &handle, IdeRecvBuffT buffer, int32_t &length, int32_t flag)
{
    auto it = commOptMap_.find(handle.type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->Read(handle.session, buffer, length, flag);
    }

    return -1;
}

SharedPtr<AdxDevice> AdxCommOptManager::GetDevice(OptType type)
{
    auto it = commOptMap_.find(type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        return it->second->GetDevice();
    }

    return nullptr;
}

void AdxCommOptManager::Timer(OptType type) const
{
    auto it = commOptMap_.find(type);
    if (it != commOptMap_.end() && it->second != nullptr) {
        it->second->Timer();
    }
}
}

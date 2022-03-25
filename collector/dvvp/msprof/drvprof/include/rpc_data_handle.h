/**
* @file rpc_data_handle.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_RPC_DATA_HANDLE_H
#define MSPROF_ENGINE_RPC_DATA_HANDLE_H

#include "error_code.h"
#include "utils/utils.h"
#include "hdc/hdc_sender.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;

class IDataHandle {
public:
    IDataHandle() {}
    virtual ~IDataHandle() {}

public:
    virtual int Init() = 0;
    virtual int UnInit() = 0;
    virtual int Flush() = 0;
    virtual int SendData(CONST_VOID_PTR data, uint32_t dataLen, const std::string fileName = "",
        const std::string jobCtx = "") = 0;
};

class HdcDataHandle : public IDataHandle {
public:
    HdcDataHandle(const std::string &moduleNameWithId, int32_t hostPid, int32_t devId);
    ~HdcDataHandle() override;

public:
    int Init() override;
    int UnInit() override;
    int Flush() override;
    int SendData(CONST_VOID_PTR data, uint32_t dataLen, const std::string fileName = "",
        const std::string jobCtx = "") override;

private:
    std::string moduleNameWithId_; // like: DATA_PREPROCESS-80858-3
    int32_t hostPid_;
    int32_t devId_;
    SHARED_PTR_ALIA<HdcSender> hdcSender_;
};

class RpcDataHandle {
public:
    RpcDataHandle(const std::string &moduleNameWithId, const std::string &module, int32_t hostPid, int32_t devId);
    virtual ~RpcDataHandle();

public:
    int Init();
    int UnInit();
    int Flush();
    int SendData(CONST_VOID_PTR data, uint32_t dataLen, const std::string fileName, const std::string jobCtx);
    bool IsReady();
    int TryToConnect();

private:
    std::string moduleNameWithId_; // like: DATA_PREPROCESS-80858-3
    std::string module_; // the module name, like: DATA_PREPROCESS
    int32_t hostPid_;
    int32_t devId_;
    SHARED_PTR_ALIA<IDataHandle> dataHandle_;
};
}}
#endif

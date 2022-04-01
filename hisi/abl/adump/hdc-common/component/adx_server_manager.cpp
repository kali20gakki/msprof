/**
 * @file adx_server_manager.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_server_manager.h"
#include "log/adx_log.h"
#include "memory_utils.h"
#include "device/adx_hdc_device.h"
namespace Adx {
AdxServerManager::AdxServerManager() noexcept
    : waitOver_(true),
      type_(OptType::NR_COMM),
      info_(""),
      epoll_(nullptr),
      handleQue_(DEFAULT_EPOLL_HANDLE_QUEUE_SIZE)
{
    servers_.clear();
}

AdxServerManager::~AdxServerManager()
{
    IDE_EVENT("start to deconstruct adx server manager");
    Exit();
}

bool AdxServerManager::RegisterEpoll(std::unique_ptr<AdxEpoll> &epoll)
{
    if (epoll == nullptr) {
        IDE_LOGE("register epoll input error");
        return false;
    }

    if (epoll_ == nullptr) {
        epoll_ = std::move(epoll);
        return true;
    }

    return false;
}

bool AdxServerManager::RegisterCommOpt(std::unique_ptr<AdxCommOpt> &op,
    const std::string &info)
{
    if (op == nullptr) {
        IDE_LOGE("register commopt input error");
        return false;
    }

    info_ = info;
    type_ = op->GetOptType();
    return AdxCommOptManager::Instance().CommOptsRegister(op);
}

bool AdxServerManager::ServerInit(const std::map<std::string, std::string> &info)
{
    EpollEvent event;
    if (epoll_ == nullptr || type_ == OptType::NR_COMM || info.empty()) {
        IDE_LOGE("server init failed for epoll not register");
        return false;
    }

    CommHandle handle = AdxCommOptManager::Instance().OpenServer(type_, info);
    if (handle.session == ADX_OPT_INVALID_HANDLE) {
        return false;
    }

    event.events = ADX_EPOLL_CONN_IN;
    event.data = handle.session;
    if (epoll_->EpollCreate(DEFAULT_EPOLL_SIZE) == IDE_DAEMON_ERROR) {
        IDE_LOGE("create epoll failed");
        (void)AdxCommOptManager::Instance().CloseServer(handle);
        return false;
    }

    if (epoll_->EpollAdd(handle.session, event) != IDE_DAEMON_OK) {
        IDE_LOGE("epoll add listen event falied");
        (void)AdxCommOptManager::Instance().CloseServer(handle);
        return false;
    }

    auto it = info.find(OPT_DEVICE_KEY);
    if (it != info.end()) {
        servers_[it->second] = handle.session;
    }
    IDE_LOGI("create server info");
    return true;
}

bool AdxServerManager::ServerUnInit(OptHandle epHandle)
{
    EpollEvent event;
    if (epoll_ == nullptr || epHandle == ADX_OPT_INVALID_HANDLE) {
        IDE_LOGE("server uninit failed for epoll not register");
        return false;
    }
    event.events = ADX_EPOLL_CONN_IN;
    event.data = epHandle;
    if (epoll_->EpollDel(epHandle, event) == IDE_DAEMON_ERROR) {
        IDE_LOGE("epoll del listen event falied");
        return false;
    }

    CommHandle handle = {type_, epHandle};
    if (AdxCommOptManager::Instance().CloseServer(handle) != IDE_DAEMON_OK) {
        IDE_LOGE("close server failed");
        return false;
    }

    return true;
}

bool AdxServerManager::ComponentAdd(std::unique_ptr<AdxComponent> &comp)
{
    if (comp == nullptr) {
        IDE_LOGE("add component input error");
        return false;
    }

    auto it = compMap_.find(comp->GetType());
    if (it != compMap_.end()) {
        return false;
    }
    IDE_LOGI("server manager add component (%d)", comp->GetType());
    compMap_[comp->GetType()] = std::move(comp);
    return true;
}

bool AdxServerManager::ComponentInit() const
{
    if (epoll_ == nullptr) {
        return false;
    }

    auto it = compMap_.begin();
    while (it != compMap_.end()) {
        (void)it->second->Init();
        it++;
    }
    IDE_LOGI("server manager components init successfully");
    return true;
}

void AdxServerManager::HandleConnectEvent(CommHandle handle)
{
    CommHandle conHandle = AdxCommOptManager::Instance().Accept(handle);
    if (conHandle.session == ADX_OPT_INVALID_HANDLE) {
        return;
    }
    handleQue_.Push(conHandle.session);
    mmUserBlock_t funcBlock;
    funcBlock.procFunc = AdxServerManager::ThreadProcess;
    funcBlock.pulArg = this;
    mmThread tid = 0;
    int32_t ret = Thread::CreateDetachTask(tid, funcBlock);
    if (ret != EN_OK) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("creare commponent process thread failed, strerror is %s",
                 mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
    }
}
bool AdxServerManager::ComponentWaitEvent()
{
    IDE_CTRL_VALUE_FAILED(epoll_ != nullptr, return false, "epoll_ check failed, nullptr");
    const int32_t epollSize = epoll_->EpollGetSize();
#if (OS_TYPE == WIN)
    EpollEvent* events = new EpollEvent[epollSize];
#else
    EpollEvent events[epollSize];
#endif
    for (int32_t i = 0; i < epollSize; i++) {
        events[i].data = 0;
        events[i].events = 0;
    }
    IDE_EVENT("Run Server(%d) Process", type_);
    waitOver_ = false;
    while (!IsQuit()) {
        TimerProcess();
        int32_t handles = epoll_->EpollWait(events, epollSize, DEFAULT_EPOLL_TIMEOUT);
        for (int32_t i = 0; i < handles && i < epollSize; i++) {
            IDE_LOGI("sock EpollWait accept event %d", handles);
            if ((events[i].events & ADX_EPOLL_CONN_IN) != 0) {
                IDE_LOGI("sock connect EpollWait event %d", handles);
                CommHandle handle = {type_, events[i].data};
                HandleConnectEvent(handle);
            } else if ((events[i].events & ADX_EPOLL_DATA_IN) != 0) {
                IDE_LOGI("data in");
            } else if ((events[i].events & ADX_EPOLL_HANG_UP) != 0) {
                IDE_LOGW("hang up state");
            } else {
                IDE_LOGW("other epoll state");
                epoll_->EpollErrorHandle();
            }
        }
        if (handles < 0) {
            epoll_->EpollErrorHandle();
        }
    }

    waitOver_ = true;
    return true;
}

void AdxServerManager::Run()
{
    if (ComponentWaitEvent()) {
        IDE_EVENT("server manager stop");
    }
}

IdeThreadArg AdxServerManager::ThreadProcess(IdeThreadArg arg)
{
    if (arg == nullptr) {
        return nullptr;
    }
    auto runnable = reinterpret_cast<AdxServerManager *>(arg);
    (void)mmSetCurrentThreadName("adx_commponent_process");
    runnable->ComponentProcess();
    return nullptr;
}

void AdxServerManager::ComponentProcess()
{
    EpollHandle epHandle = ADX_INVALID_HANDLE;
    IDE_EVENT("process new connect");
    if (handleQue_.Pop(epHandle) == false) {
        return;
    }

    if (epHandle == ADX_INVALID_HANDLE) {
        IDE_LOGE("server run process handle invalied");
        return;
    }

    CommHandle handle;
    handle.type = type_;
    handle.session = epHandle;
    SubComponentProcess(handle);
#if (!defined SESSION_ACTIVE)
    (void)AdxCommOptManager::Instance().Close(handle);
    handle.session = ADX_OPT_INVALID_HANDLE;
#endif
}

void AdxServerManager::SubComponentProcess(CommHandle handle)
{
    MsgProto *req = nullptr;
    int32_t length = 0;

    int32_t ret = AdxCommOptManager::Instance().Read(handle, (IdeRecvBuffT)&req, length, COMM_OPT_NOBLOCK);
    if (ret == IDE_DAEMON_ERROR || req == nullptr || length <= 0) {
        IDE_LOGE("reveice request failed ret %d, length(%d)", ret, length);
        return;
    }

    SharedPtr<MsgProto> msgPtr(req, IdeXfree);
    req = nullptr;
    if (msgPtr->sliceLen + sizeof(MsgProto) != (uint32_t)length) {
        IDE_LOGE("reveice request package(%u) length(%d) exception", msgPtr->sliceLen, length);
        return;
    }

    IDE_EVENT("commopt type(%d), request type(%u)", type_, msgPtr->reqType);
    ComponentType cmptType = GetComponentTypeByReqType((CmdClassT)msgPtr->reqType);
    auto it = compMap_.find(cmptType);
    if (it != compMap_.end()) {
        IDE_EVENT("begin to process [%s] component", it->second->GetInfo().c_str());
        if (it->second->Process(handle, msgPtr) != IDE_DAEMON_OK) {
            IDE_LOGE("end of processing [%s] component failed, req->type: %d",
                it->second->GetInfo().c_str(), msgPtr->reqType);
        } else {
            IDE_EVENT("end of processing [%s] component succcessfully", it->second->GetInfo().c_str());
        }
    } else {
        IDE_LOGE("Unable to find the corresponding component type(%d)", static_cast<int>(cmptType));
    }
}

ComponentType AdxServerManager::GetComponentTypeByReqType(CmdClassT cmdType)
{
    ComponentType cmptType = ComponentType::NR_COMPONENTS;
    for (uint32_t i = 0; i < ARRAY_LEN(g_componentsInfo, AdxComponentMap); i++) {
        if (cmdType == g_componentsInfo[i].cmdType) {
            cmptType = g_componentsInfo[i].cmptType;
            break;
        }
    }
    return cmptType;
}

void AdxServerManager::TimerProcess()
{
    std::vector<std::string> devLogIds;
    SharedPtr<AdxDevice> device = AdxCommOptManager::Instance().GetDevice(type_);
    if (device == nullptr) {
        return;
    }

    device->GetAllEnableDevices(devLogIds);
    if (!devLogIds.empty()) {
        std::map<std::string, std::string> info;
        info[OPT_SERVICE_KEY] = info_;
        for (uint32_t i = 0; i < devLogIds.size(); i++) {
            auto it = servers_.find(devLogIds[i]);
            if (it == servers_.end()) {
                info[OPT_DEVICE_KEY] = devLogIds[i];
                IDE_LOGI("device up %s", devLogIds[i].c_str());
                ServerInit(info);
            }
        }
    }

    device->GetDisableDevices(devLogIds);
    if (!devLogIds.empty()) {
        for (uint32_t i = 0; i < devLogIds.size(); i++) {
            auto it = servers_.find(devLogIds[i]);
            if (it != servers_.end()) {
                IDE_LOGI("device suspend %s", devLogIds[i].c_str());
                (void)ServerUnInit(it->second);
                servers_.erase(it);
            }
        }
    }

    AdxCommOptManager::Instance().Timer(type_);
}

void AdxServerManager::Exit()
{
    Terminate();
    IDE_LOGI("waitOver is %d", waitOver_);
    // wait epoll wait timeout
    while (!waitOver_) {
        mmSleep(DEFAULT_EPOLL_TIMEOUT);
    }

    auto its = servers_.begin();
    while (its != servers_.end()) {
        auto eit = its++;
        (void)ServerUnInit(eit->second);
        servers_.erase(eit);
    }

    auto it = compMap_.begin();
    while (it != compMap_.end()) {
        (void)it->second->UnInit();
        it++;
    }
    compMap_.clear();

    if (epoll_ != nullptr) {
        epoll_->EpollDestroy();
        epoll_ = nullptr;
    }
}
}

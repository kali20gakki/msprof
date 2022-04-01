/**
 * @file adx_server_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_COMPONENTS_MANAGER_H
#define ADX_COMPONENTS_MANAGER_H
#include <map>
#include <memory>
#include <queue>
#include "common/thread.h"
#include "bound_queue.h"
#include "adx_component.h"
#include "epoll/adx_epoll.h"
#include "adx_comm_opt_manager.h"
#include "common/extra_config.h"
namespace Adx {
constexpr uint32_t DEFAULT_EPOLL_HANDLE_QUEUE_SIZE = 256;
class AdxServerManager : public Runnable {
public:
    AdxServerManager() noexcept;
    ~AdxServerManager();
    bool RegisterEpoll(std::unique_ptr<AdxEpoll> &epoll);
    bool RegisterCommOpt(std::unique_ptr<AdxCommOpt> &opt,
        const std::string &info);
    bool ComponentAdd(std::unique_ptr<AdxComponent> &comp);
    bool ComponentInit() const;
    bool ComponentWaitEvent();
    void Run();
    void ComponentProcess();
    void SubComponentProcess(CommHandle handle);
    static IdeThreadArg ThreadProcess(IdeThreadArg arg);
    void Exit();
private:
    void TimerProcess(void);
    bool ServerInit(const std::map<std::string, std::string> &info);
    bool ServerUnInit(OptHandle epHandle);
    ComponentType GetComponentTypeByReqType(CmdClassT type);
    void HandleConnectEvent(CommHandle handle);
private:
    bool waitOver_;
    OptType type_;
    std::string info_;
    std::unique_ptr<AdxEpoll> epoll_;
    std::map<ComponentType, std::unique_ptr<AdxComponent>> compMap_;
    std::map<std::string, EpollHandle> servers_;
    BoundQueue<EpollHandle> handleQue_;
};
}
#endif

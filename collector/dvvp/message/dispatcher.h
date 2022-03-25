/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_MESSAGE_DISPATCHER_H
#define ANALYSIS_DVVP_MESSAGE_DISPATCHER_H

#include <map>
#include <memory>
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace message {
class IMsgHandler {
public:
    virtual ~IMsgHandler() {}

public:
    virtual void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message) = 0;
};

class MsgDispatcher {
public:
    MsgDispatcher();
    virtual ~MsgDispatcher();

public:
    void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message);

    template<typename T>
    void RegisterMessageHandler(SHARED_PTR_ALIA<IMsgHandler> handler)
    {
        if (handler == nullptr) {
            return;
        }
        handlerMap_[T::descriptor()] = handler;
    }

private:
    std::map<const google::protobuf::Descriptor *, SHARED_PTR_ALIA<IMsgHandler> > handlerMap_;
};
}  // namespace message
}  // namespace dvvp
}  // namespace analysis

#endif
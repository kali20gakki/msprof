/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "dispatcher.h"
#include "config/config.h"
#include "message/prof_params.h"
#include "msprof_dlog.h"

namespace analysis {
namespace dvvp {
namespace message {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
MsgDispatcher::MsgDispatcher()
{
}

MsgDispatcher::~MsgDispatcher()
{
}

void MsgDispatcher::OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("message is null");
        return;
    }

    auto iter = handlerMap_.find(message->GetDescriptor());
    if (iter != handlerMap_.end()) {
        iter->second->OnNewMessage(message);
    } else {
        MSPROF_LOGE("Failed to find handler for message:%s", message->GetTypeName().c_str());
    }
}
}  // namespace message
}  // namespace dvvp
}  // namespace analysis
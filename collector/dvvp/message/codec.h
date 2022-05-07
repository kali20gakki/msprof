/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_MESSAGE_CODEC_H
#define ANALYSIS_DVVP_MESSAGE_CODEC_H

#include <memory>
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace message {
using namespace analysis::dvvp::common::utils;
enum class E_CODEC_FORMAT {
    CODEC_FORMAT_JSON = 0,
    CODEC_FORMAT_BINARY = 1,
};
const google::protobuf::Descriptor *FindMessageTypeByName(const std::string &name);
SHARED_PTR_ALIA<google::protobuf::Message> CreateMessage(const std::string &name);
bool AppendMessage(std::string &out, SHARED_PTR_ALIA<google::protobuf::Message> message);

std::string EncodeJson(SHARED_PTR_ALIA<google::protobuf::Message> message,
                       bool printAll = false, bool format = true);
SHARED_PTR_ALIA<std::string> EncodeMessageShared(SHARED_PTR_ALIA<google::protobuf::Message> message = nullptr);
std::string EncodeMessage(SHARED_PTR_ALIA<google::protobuf::Message> message = nullptr);
SHARED_PTR_ALIA<google::protobuf::Message> DecodeMessage(const std::string &buf);
SHARED_PTR_ALIA<google::protobuf::Message> DecodeMessage2(CONST_VOID_PTR data, uint32_t length);
}  // namespace message
}  // namespace dvvp
}  // namespace analysis

#endif

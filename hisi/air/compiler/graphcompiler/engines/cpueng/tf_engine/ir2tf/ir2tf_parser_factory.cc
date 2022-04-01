/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ir2tf_parser_factory.h"
#include "common/util/log.h"

namespace aicpu {
Ir2tfParserFactory& Ir2tfParserFactory::Instance()
{
  static Ir2tfParserFactory instance;
  return instance;
}

std::shared_ptr<Ir2tfBaseParser> Ir2tfParserFactory::CreateIRParser(const std::string &op_type)
{
  auto iter = creator_map_.find(op_type);
  if (iter != creator_map_.end()) {
    AICPUE_LOGI("Ir2tfBaseParser::CreateIRParser: [%s] creator exist",
                op_type.c_str());
    return iter->second();
  }
  AICPUE_LOGI("IR2TFParserFactory::CreateIRParser: No matched parser: [%s], use base parser.",
              op_type.c_str());
  return Ir2tfBaseParser::Instance();
}

void Ir2tfParserFactory::RegisterCreator(const std::string &op_type, CREATOR_FUN fun)
{
  if (creator_map_.find(op_type) != creator_map_.end()) {
    AICPUE_LOGW("Ir2tfBaseParser::RegisterCreator: [%s] creator already exist, it will be covered.",
                op_type.c_str());
  }
  creator_map_[op_type] = fun;
}
}

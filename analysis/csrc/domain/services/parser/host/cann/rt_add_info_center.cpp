/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"

#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"

namespace Analysis {
namespace Domain {
namespace Host {
namespace Cann {

using namespace Analysis::Utils;
using namespace Analysis::Infra;

void Cann::RTAddInfoCenter::Load(const std::string &path)
{
    LoadDB(path);
}

RuntimeOpInfo RTAddInfoCenter::Get(uint16_t deviceId, uint32_t streamId, uint16_t taskId)
{
    std::string key = Utils::Join("_", deviceId, streamId, taskId);
    auto it = runtimeOpInfoData_.find(key);
    if (it != runtimeOpInfoData_.end()) {
        return it->second;
    }
    return RuntimeOpInfo();
}


void RTAddInfoCenter::LoadDB(const std::string &path)
{
    RtsTrackDB rtsTrackDb;
    std::string hostDbDirectory = Utils::File::PathJoin({path, rtsTrackDb.GetDBName()});
    DBRunner dbRunner(hostDbDirectory);
    if (!File::Exist(hostDbDirectory) || !dbRunner.CheckTableExists("RuntimeOpInfo")) {
        return;
    }

    std::string sql{"SELECT device_id, model_id, stream_id, task_id, op_name, task_type, op_type, "
                    "block_num, mix_block_num, op_flag, is_dynamic, tensor_num, input_formats, "
                    "input_data_types, input_shapes, output_formats, output_data_types, output_shapes "
                    "FROM RuntimeOpInfo"};
    using dataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint32_t, uint16_t, std::string, std::string,
                                              std::string, uint16_t, uint16_t, uint16_t, std::string, uint16_t,
                                              std::string, std::string, std::string, std::string, std::string,
                                              std::string>>;
    dataFormat result;
    if (!dbRunner.QueryData(sql, result)) {
        ERROR("Query runtime op info data failed, db path is %.", hostDbDirectory);
        return;
    }

    for (auto row : result) {
        uint16_t deviceId, taskId, blockNum, mixBlockNum, opFlag, tensorNum;
        uint32_t streamId;
        uint64_t modelId, opNameU64, opTypeU64;
        std::string opName, taskType, opType, isDynamic, inputFormats, inputDataTypes, inputShapes,
                    outputFormats, outputDataTypes, outputShapes;
        std::tie(deviceId, modelId, streamId, taskId, opName, taskType, opType, blockNum, mixBlockNum, opFlag,
                 isDynamic, tensorNum, inputFormats, inputDataTypes, inputShapes,
                 outputFormats, outputDataTypes, outputShapes) = row;
        std::string key = Utils::Join("_", deviceId, streamId, taskId);
        if (Utils::StrToU64(opNameU64, opName) != ANALYSIS_OK || Utils::StrToU64(opTypeU64, opType) != ANALYSIS_OK) {
            ERROR("opName hash is %, opType hash is %.", opName, opType);
            continue;
        }
        opName = HashData::GetInstance().Get(opNameU64);
        opType = HashData::GetInstance().Get(opTypeU64);
        RuntimeOpInfo info{deviceId, taskId, blockNum, mixBlockNum, opFlag, tensorNum, streamId, modelId, taskType,
                           opType, opName, isDynamic, inputFormats, inputDataTypes, inputShapes,
                           outputFormats, outputDataTypes, outputShapes};
        runtimeOpInfoData_[key] = info;
    }
}

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

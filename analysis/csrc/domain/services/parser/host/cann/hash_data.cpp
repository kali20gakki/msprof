/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_data.cpp
 * Description        : hash数据
 * Author             : MSPROF TEAM
 * Creation Date      : 2023/11/2023/11/18
 * *****************************************************************************
 */

#include "hash_data.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Domain {
namespace Host {
namespace Cann {

using namespace Analysis::Utils;

bool Cann::HashData::Load(const std::string &path)
{
    auto files = File::GetOriginData(path, filePrefix_, fileFilter_);
    if (files.empty()) {
        WARN("No hash data found.");
        return false;
    }
    return ReadFiles(files);
}

std::string HashData::Get(uint64_t hash)
{
    if (hashData_.find(hash) != hashData_.end()) {
        return hashData_[hash];
    }
    ERROR("Failed to found str for hash: %", hash);
    return std::to_string(hash);
}

std::unordered_map<uint64_t, std::string> &HashData::GetAll()
{
    return hashData_;
}

bool HashData::ReadFiles(const std::vector<std::string> &files)
{
    const int expectTokenSize = 2; // 2代表：前后的key和value
    for (auto &file: files) {
        FileReader reader(file);
        std::vector<std::string> text;
        auto status = reader.ReadText(text);
        if (status != ANALYSIS_OK) {
            ERROR("Read txt from % error", file);
            return false;
        }
        for (const auto &line : text) {
            const int splitPosition = 1;
            auto tokens = Utils::Split(line, ":", splitPosition);
            if (tokens.size() != expectTokenSize) {
                ERROR("Found illegal hash line: %", line);
                continue;
            }

            uint64_t key = 0;
            status = Utils::StrToU64(key, tokens[0]);
            if (status != ANALYSIS_OK) {
                ERROR("Convert str % to uint64 error", tokens[0]);
                continue;
            }
            if (hashData_.find(key) != hashData_.end()) {
                // 采集侧修复后该行变为ERROR
                WARN("Duplicate hash found key: %", key);
            }
            hashData_[key] = tokens[1];
        }
    }
    return true;
}

void HashData::Clear()
{
    hashData_.clear();
}
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

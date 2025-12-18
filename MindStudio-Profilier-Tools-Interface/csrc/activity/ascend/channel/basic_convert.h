/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : basic_convert.h
 * Description        : convert channel data to general data
 * Author             : msprof team
 * Creation Date      : 2025/07/29
 * *****************************************************************************
 */

#ifndef MSPTI_CONVERT_BASICCONVERT_H
#define MSPTI_CONVERT_BASICCONVERT_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <memory>
#include <functional>
#include "csrc/common/plog_manager.h"
#include "csrc/common/context_manager.h"

namespace Mspti {
namespace Convert {
template <typename Derived, typename T>
class BasicConvert {
public:
    using TransFunc = std::function<bool(const void* const, T&)>;

public:
    static constexpr size_t INVALID_STRUCT_SIZE = static_cast<size_t>(-1);
    std::vector<T> TransData(const char buffer[], size_t valid_size, uint32_t deviceId, size_t& pos)
    {
        auto chipType = Common::ContextManager::GetInstance()->GetChipType(deviceId);
        auto parseFunc = this->GetTransFunc(deviceId, chipType);
        size_t structSize = this->GetStructSize(deviceId, chipType);
        if (parseFunc == nullptr || structSize == INVALID_STRUCT_SIZE) {
            return {};
        }

        std::vector<T> ans;
        ans.reserve(valid_size / structSize);
        for (size_t i = 0; i < valid_size / structSize; i++) {
            T item;
            auto parseAns = parseFunc(buffer + pos, item);
            if (!parseAns) {
                MSPTI_LOGW("cannot parse target func");
                return {};
            }
            pos += structSize;
            ans.push_back(std::move(item));
        }
        return ans;
    };

    bool TransData(const char buffer[], size_t valid_size, uint32_t deviceId, size_t& pos, T& t)
    {
        auto chipType = Common::ContextManager::GetInstance()->GetChipType(deviceId);
        auto parseFunc = this->GetTransFunc(deviceId, chipType);
        size_t structSize = this->GetStructSize(deviceId, chipType);
        if (parseFunc == nullptr || structSize == INVALID_STRUCT_SIZE) {
            return false;
        }
        auto ans = parseFunc(buffer + pos, t);
        pos += structSize;
        return ans;
    };

private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

    size_t GetStructSize(uint32_t deviceId, Common::PlatformType chipType) const
    {
        return derived().GetStructSize(deviceId, chipType);
    }

    TransFunc GetTransFunc(uint32_t deviceId, Common::PlatformType chipType) const
    {
        return derived().GetTransFunc(deviceId, chipType);
    }
};

}
}


#endif // MSPTI_CONVERT_BASICCONVERT_H
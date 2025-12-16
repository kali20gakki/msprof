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

#ifndef MSPTI_PROJECT_MEMORY_REPORTER_H
#define MSPTI_PROJECT_MEMORY_REPORTER_H

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include "csrc/include/mspti_activity.h"
#include "csrc/common/inject/runtime_mem_inject.h"

namespace Mspti {
namespace Reporter {
struct MemoryRecord {
    MemoryRecord(VOID_PTR_PTR devPtr, uint64_t size, msptiActivityMemoryKind memKind,
                 msptiActivityMemoryOperationType opType);
    ~MemoryRecord();

    VOID_PTR_PTR devPtr{nullptr};
    uint64_t size{0};
    msptiActivityMemoryKind memKind{MSPTI_ACTIVITY_MEMORY_UNKNOWN};
    msptiActivityMemoryOperationType opType{};
    uint64_t start{0};
    uint64_t end{0};
};

struct MemsetRecord {
    MemsetRecord(uint32_t value, uint64_t bytes, RtStreamT stream, uint8_t isAsync);
    ~MemsetRecord();

    uint32_t value{0};
    uint64_t bytes{0};
    RtStreamT stream{nullptr};
    uint8_t isAsync{0};
    uint64_t start{0};
    uint64_t end{0};
};

struct MemcpyRecord {
    MemcpyRecord(RtMemcpyKindT copyKind, uint64_t bytes, RtStreamT stream, uint8_t isAsync);
    ~MemcpyRecord();

    RtMemcpyKindT copyKind{};
    uint64_t bytes{0};
    RtStreamT stream{nullptr};
    uint8_t isAsync{0};
    uint64_t start{0};
    uint64_t end{0};
};

class MemoryReporter {
public:
    static MemoryReporter* GetInstance();

    // Memory Activity
    msptiResult ReportMemoryActivity(const MemoryRecord &record);
    // Memset Activity
    msptiResult ReportMemsetActivity(const MemsetRecord &record);
    // Memcpy Activity
    msptiResult ReportMemcpyActivity(const MemcpyRecord &record);
private:
    std::mutex addrMtx_;
    std::unordered_map<uintptr_t, uint64_t> addrBytesInfo_;
};
}
}

#endif // MSPTI_PROJECT_MEMORY_REPORTER_H

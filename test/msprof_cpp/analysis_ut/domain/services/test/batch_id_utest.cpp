/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_inventory_utest.cpp
 * Description        : DataInventory UT
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <map>
#include "analysis/csrc/infrastructure/context/include/device_context.h"
#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"

namespace Analysis {

using namespace Infra;
using namespace Domain;

TEST(TestParser, should_return_batch_id_when_compute)
{
    // 申请内存
    HalLogData log[10] = {};
    uint64_t timestamp = 0;
    for (uint32_t i = 0; i < 10; i++) {
        log[i].hd.timestamp = timestamp;
        timestamp++;
    }

    HalTrackData track[2]{};
    track[0].hd.timestamp = 2;
    track[1].hd.timestamp = 4;

    // 分组存入vector
    std::vector<HalLogData*> plog;
    for (uint32_t i = 0; i < 10; i++) {
        plog.push_back(&log[i]);
    }

    std::vector<HalTrackData*> ptrack;
    for (uint32_t i = 0; i < 2; i++) {
        ptrack.push_back(&track[i]);
    }

    ModelingComputeBatchIdBinary(reinterpret_cast<HalUniData**>(plog.data()), plog.size(),
                                 reinterpret_cast<HalUniData**>(ptrack.data()), ptrack.size());

    uint32_t expectBatchId[10] = {0, 0, 0, 1, 1, 2, 2, 2, 2, 2};
    for (uint32_t i = 0; i < 10; i++) {
        ASSERT_EQ(log[i].hd.taskId.batchId, expectBatchId[i]);
    }
}

TEST(TestParser, should_return_batch_id_when_task_is_not_continuoes)
{
    // 申请内存
    HalLogData log[2] = {};
    log[0].hd.timestamp = 1001;
    log[1].hd.timestamp = 5001;

    HalTrackData track[10]{};
    uint64_t timestamp = 0;
    for (uint32_t i = 0; i < 10; i++) {
        timestamp = i * 1000;
        track[i].hd.timestamp = timestamp;
    }

    // 分组存入vector
    std::vector<HalLogData*> plog;
    for (uint32_t i = 0; i < 2; i++) {
        plog.push_back(&log[i]);
    }

    std::vector<HalTrackData*> ptrack;
    for (uint32_t i = 0; i < 10; i++) {
        ptrack.push_back(&track[i]);
    }

    ModelingComputeBatchIdBinary(reinterpret_cast<HalUniData**>(plog.data()), plog.size(),
                                 reinterpret_cast<HalUniData**>(ptrack.data()), ptrack.size());

    uint32_t expectBatchId[2] = {2, 6};
    for (uint32_t i = 0; i < 2; i++) {
        ASSERT_EQ(log[i].hd.taskId.batchId, expectBatchId[i]);
    }
}

TEST(TestParser, should_return_batch_id_is_default_when_flip_is_none)
{
    // 申请内存
    HalLogData log[10] = {};
    uint64_t timestamp = 0;
    for (uint32_t i = 0; i < 10; i++) {
        log[i].hd.taskId.batchId = INVALID_BATCH_ID;
        log[i].hd.timestamp = timestamp;
        timestamp++;
    }

    // 分组存入vector
    std::vector<HalLogData*> plog;
    for (uint32_t i = 0; i < 10; i++) {
        plog.push_back(&log[i]);
    }

    // ptrack为空
    std::vector<HalTrackData*> ptrack;

    ModelingComputeBatchIdBinary(reinterpret_cast<HalUniData**>(plog.data()), plog.size(),
                                 reinterpret_cast<HalUniData**>(ptrack.data()), ptrack.size());

    uint32_t expectBatchId[10] = {INVALID_BATCH_ID, INVALID_BATCH_ID, INVALID_BATCH_ID,
        INVALID_BATCH_ID, INVALID_BATCH_ID, INVALID_BATCH_ID, INVALID_BATCH_ID,
        INVALID_BATCH_ID, INVALID_BATCH_ID, INVALID_BATCH_ID};

    for (uint32_t i = 0; i < 10; i++) {
        ASSERT_EQ(log[i].hd.taskId.batchId, expectBatchId[i]);
    }
}

TEST(TestParser, should_return_batch_id_when_compute_need_calibrate)
{
    // 申请内存
    HalLogData log[10] = {};
    uint64_t timestamp = 0;
    for (uint32_t i = 0; i < 10; i++) {
        log[i].hd.timestamp = timestamp;
        timestamp++;
    }
    log[2].hd.taskId.taskId = UINT16_MAX;
    log[4].hd.taskId.taskId = UINT16_MAX;
    log[5].hd.taskId.taskId = 0;
    log[6].hd.taskId.taskId = 1;

    HalTrackData track[2]{};
    track[0].hd.timestamp = 2;
    track[0].hd.taskId.taskId = 0;
    track[1].hd.timestamp = 6;
    track[1].hd.taskId.taskId = 2;

    // 分组存入vector
    std::vector<HalLogData*> plog;
    for (uint32_t i = 0; i < 10; i++) {
        plog.push_back(&log[i]);
    }

    std::vector<HalTrackData*> ptrack;
    for (uint32_t i = 0; i < 2; i++) {
        ptrack.push_back(&track[i]);
    }

    ModelingComputeBatchIdBinary(reinterpret_cast<HalUniData**>(plog.data()), plog.size(),
                                 reinterpret_cast<HalUniData**>(ptrack.data()), ptrack.size());

    uint32_t expectBatchId[10] = {0, 0, 0, 1, 1, 2, 2, 2, 2, 2};
    for (uint32_t i = 0; i < 10; i++) {
        ASSERT_EQ(log[i].hd.taskId.batchId, expectBatchId[i]);
    }
}

}
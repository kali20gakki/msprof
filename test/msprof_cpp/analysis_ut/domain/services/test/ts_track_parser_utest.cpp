/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ts_track_parser_utest.cpp
 * Description        : ts_track_parser ut用例
 * Author             : msprof team
 * Creation Date      : 2024/5/6
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace testing;
using namespace Analysis::Utils;

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
const std::string TS_TRACK_PATH = "./ts_track";
}

class TsTrackParserUtest : public Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(TS_TRACK_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({TS_TRACK_PATH, "data"})));
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(TS_TRACK_PATH, 0));
    }
    TaskFlip CreateTaskFlip(int funcType, int taskId, int streamId, int timestamp, int flipNum)
    {
        TaskFlip taskFlip{};
        taskFlip.funcType = funcType;
        taskFlip.taskId = taskId;
        taskFlip.streamId = streamId;
        taskFlip.timestamp = timestamp;
        taskFlip.flipNum = flipNum;
        return taskFlip;
    }

protected:
    Infra::DataInventory dataInventory_;
};

TEST_F(TsTrackParserUtest, ShouldReturnTaskFlipDataWhenParserRun)
{
    std::vector<int> expectTimestamp{100, INVALID_TIMESTAMP};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskFlip> taskFlip{CreateTaskFlip(0x0e, 1, 1, 100, 5),
                                   CreateTaskFlip(0x0e, 1, 1, INVALID_TIMESTAMP, 5)};
    WriteBin(taskFlip, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    ASSERT_EQ(Analysis::ANALYSIS_OK, tsTrackParser.Run(dataInventory_, context));
    auto logData = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(2ul, logData->size());
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnTaskFlipDataWhenMultiData)
{
    std::vector<int> expectTimestamp{10, 20, 100, INVALID_TIMESTAMP};
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = TS_TRACK_PATH;
    std::vector<TaskFlip> taskFlip1{CreateTaskFlip(0x0e, 1, 1, 10, 5), CreateTaskFlip(0x0e, 1, 1, 20, 5)};
    WriteBin(taskFlip1, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_0");
    std::vector<TaskFlip> taskFlip2{CreateTaskFlip(0x0e, 1, 1, 100,  5),
                                    CreateTaskFlip(0x0e, 1, 1, INVALID_TIMESTAMP, 5)};
    WriteBin(taskFlip2, File::PathJoin({TS_TRACK_PATH, "data"}), "ts_track.data.0.slice_1");
    ASSERT_EQ(tsTrackParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto logData = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    ASSERT_EQ(4ul, logData->size());
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(TsTrackParserUtest, ShouldReturnParserReadDataErrorWhenPathError)
{
    TsTrackParser tsTrackParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::PARSER_READ_DATA_ERROR, tsTrackParser.Run(dataInventory_, context));
}
}

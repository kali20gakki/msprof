/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ffts_profile_parser_utest.cpp
 * Description        : ffts_profile_parser类UT
 * Author             : msprof team
 * Creation Date      : 2024/4/30
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"
#include "analysis/csrc/domain/services/parser/pmu/include/ffts_profile_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/block_pmu_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/chip4_pmu_parser_item.h"

using namespace testing;
using namespace Analysis::Utils;
using namespace Analysis::Infra;

namespace Analysis {
namespace Domain {
namespace {
const std::string FFTS_PROFILE_PATH = "./ffts_profile";
const int PMU_COUNT = 8;
const int TIME_COUNT = 2;
}
class FftsProfileParserUTest : public Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(FFTS_PROFILE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({FFTS_PROFILE_PATH, "data"})));
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(FFTS_PROFILE_PATH, 0));
    }

    ContextPmu GenerateContextPmu()
    {
        ContextPmu contextPmu;
        contextPmu.funcType = 40; // 40为context级别PMU
        contextPmu.streamId = 32; // 32 streamId
        contextPmu.taskId = 1000;  // 1000 taskId
        contextPmu.subTaskType = 6;  // 6 subTaskType
        contextPmu.ovFlag = 1; // 1 ovFlag
        contextPmu.subTaskId = 1; // 1 即contextId
        contextPmu.fftsType = 4; // 4 与subTaskType组合标识MIX_AIC
        contextPmu.totalCycle = 10000; // 10000 totalCycle
        contextPmu.cnt = 0; // 0 校验位（0-15）
        for (int i = 0; i < PMU_COUNT; ++i) {
            contextPmu.pmuList[i] = i + 1;  // PMU值
        }
        for (int i = 0; i < TIME_COUNT; ++i) {
            contextPmu.timeList[i] = (i + 1) * 100;  // 100构造时间字段值
        }
        return contextPmu;
    }

    BlockPmu GenerateBlockPmu()
    {
        BlockPmu blockPmu;
        blockPmu.funcType = 41; // 41为block级别PMU
        blockPmu.streamId = 32; // 32 streamId
        blockPmu.taskId = 1000; // 1000 taskId
        blockPmu.subTaskType = 6; // 6 subTaskType
        blockPmu.subTaskId = 1; // 1 即contextId
        blockPmu.coreType = 1; // 1标识AIV上报
        blockPmu.coreId = 0;  // 1 coreId
        blockPmu.fftsType = 4; // 4 与subTaskType组合标识MIX_AIC
        blockPmu.subBlockId = 1; // 1 subBlockId
        blockPmu.totalCycle = 10000; // 10000 totalCycle
        blockPmu.cnt = 1; // 1 校验位（0-15）
        for (int i = 0; i < PMU_COUNT; ++i) {
            blockPmu.pmuList[i] = 2 * (i + 1); // 2用于区分context级别PMU值
        }
        for (int i = 0; i < TIME_COUNT; ++i) {
            blockPmu.timeList[i] = (i + 1) * 100; // 100构造时间字段值
        }
        return blockPmu;
    }

protected:
    DataInventory dataInventory_;
};

TEST_F(FftsProfileParserUTest, ShouldReturnContextPmuWhenParserRun)
{
    std::vector<uint64_t> pmuList{1, 2, 3, 4, 5, 6, 7, 8};
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = FFTS_PROFILE_PATH;
    std::vector<ContextPmu> pmu{GenerateContextPmu()};
    WriteBin(pmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context));
    auto pmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_EQ(1ul, pmuData->size());
    ASSERT_EQ(AcceleratorType::MIX_AIC, pmuData->data()[0].pmu.acceleratorType);
    for (int i = 0; i < PMU_COUNT; ++i) {
        ASSERT_EQ(pmuList[i], pmuData->data()[0].pmu.pmuList[i]);
    }
}

TEST_F(FftsProfileParserUTest, ShouldReturnBlockPmuWhenParserRun)
{
    std::vector<uint64_t> pmuList{2, 4, 6, 8, 10, 12, 14, 16};
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = FFTS_PROFILE_PATH;
    std::vector<BlockPmu> pmu{GenerateBlockPmu()};
    WriteBin(pmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context));
    auto pmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_EQ(1ul, pmuData->size());
    ASSERT_EQ(AcceleratorType::MIX_AIC, pmuData->data()[0].pmu.acceleratorType);
    for (int i = 0; i < PMU_COUNT; ++i) {
        ASSERT_EQ(pmuList[i], pmuData->data()[0].pmu.pmuList[i]);
    }
}

TEST_F(FftsProfileParserUTest, ShouldReturnContextPmuAndBlockPmuWhenParserRun)
{
    std::vector<uint64_t> contextPmuList{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<uint64_t> blockPmuList{2, 4, 6, 8, 10, 12, 14, 16};
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = FFTS_PROFILE_PATH;
    std::vector<ContextPmu> contextPmu{GenerateContextPmu()};
    WriteBin(contextPmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    std::vector<BlockPmu> blockPmu{GenerateBlockPmu()};
    WriteBin(blockPmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context));
    auto pmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_EQ(2ul, pmuData->size());
    ASSERT_EQ(AcceleratorType::MIX_AIC, pmuData->data()[0].pmu.acceleratorType);
    for (int i = 0; i < PMU_COUNT; ++i) {
        ASSERT_EQ(contextPmuList[i], pmuData->data()[0].pmu.pmuList[i]);
        ASSERT_EQ(blockPmuList[i], pmuData->data()[1].pmu.pmuList[i]);
    }
}

TEST_F(FftsProfileParserUTest, ShouldReturnContextPmuAndBlockPmuWhenParserMultiFiles)
{
    std::vector<uint64_t> contextPmuList{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<uint64_t> blockPmuList{2, 4, 6, 8, 10, 12, 14, 16};
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = FFTS_PROFILE_PATH;
    std::vector<ContextPmu> contextPmu{GenerateContextPmu()};
    WriteBin(contextPmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    std::vector<BlockPmu> blockPmu{GenerateBlockPmu()};
    WriteBin(blockPmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_1");
    ASSERT_EQ(ANALYSIS_OK, parser.Run(dataInventory_, context));
    auto pmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_EQ(2ul, pmuData->size());
    ASSERT_EQ(AcceleratorType::MIX_AIC, pmuData->data()[0].pmu.acceleratorType);
    for (int i = 0; i < PMU_COUNT; ++i) {
        ASSERT_EQ(contextPmuList[i], pmuData->data()[0].pmu.pmuList[i]);
        ASSERT_EQ(blockPmuList[i], pmuData->data()[1].pmu.pmuList[i]);
    }
}

TEST_F(FftsProfileParserUTest, ShouldReturnNoDataWhenPathNull)
{
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::ANALYSIS_OK, parser.Run(dataInventory_, context));
    auto pmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_EQ(0ul, pmuData->size());
}

TEST_F(FftsProfileParserUTest, ShouldParseErrorWhenResizeException)
{
    FftsProfileParser parser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = FFTS_PROFILE_PATH;
    std::vector<BlockPmu> pmu{GenerateBlockPmu()};
    WriteBin(pmu, File::PathJoin({FFTS_PROFILE_PATH, "data"}), "ffts_profile.data.0.slice_0");
    MOCKER_CPP(&std::vector<HalPmuData>::resize, void(std::vector<HalPmuData>::*)(size_t)).stubs()
        .will(throws(std::bad_alloc()));
    ASSERT_EQ(PARSER_PARSE_DATA_ERROR, parser.Run(dataInventory_, context));
}
}
}

/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_parser_utest.cpp
 * Description        : freq_parser类UT
 * Author             : msprof team
 * Creation Date      : 2024/4/30
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/freq/include/freq_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace testing;
using namespace Analysis::Utils;

namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
    const std::string FREQ_LPM_PATH = "./lpmFreqConv";
    const int FREQ_COUNT = 50;
    const int AIC_FREQ = 1000;
}

class FreqParserUtest : public Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(FREQ_LPM_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({FREQ_LPM_PATH, "data"})));
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(FREQ_LPM_PATH, 0));
    }
    FreqData CreateFreqData()
    {
        FreqData freqData{};
        freqData.count = FREQ_COUNT;
        for (int i = 0; i < FREQ_COUNT; ++i) {
            freqData.lpmDataS[i].freq = i + 1;
            freqData.lpmDataS[i].sysCnt = i;
        }
        return freqData;
    }

protected:
    Infra::DataInventory dataInventory_;
};

TEST_F(FreqParserUtest, ShouldReturnFreqLpmDataWhenParserRun)
{
    int expectSize = 2 * FREQ_COUNT + 1;
    std::vector<int> expectCount{FREQ_COUNT};
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(expectSize, data->size());
    ASSERT_EQ(0, data->data()[0].sysCnt);
    ASSERT_EQ(0, data->data()[1].sysCnt);
}

TEST_F(FreqParserUtest, ShouldReturnFreqLpmDataWhenMultiFile)
{
    int expectSize = 4 * FREQ_COUNT + 1;
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_1");
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(expectSize, data->size());
    ASSERT_EQ(0, data->data()[0].sysCnt);
    ASSERT_EQ(0, data->data()[1].sysCnt);
}

TEST_F(FreqParserUtest, ShouldReturnNoDataWhenNoFile)
{
    FreqParser freqParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::ANALYSIS_OK, freqParser.Run(dataInventory_, context));
    auto data = dataInventory_.GetPtr<std::vector<HalFreqLpmData>>();
    ASSERT_EQ(0ul, data->size());
}

TEST_F(FreqParserUtest, ShouldParseErrorWhenResizeException)
{
    FreqParser freqParser;
    DeviceContext context;
    context.isInitialized_ = true;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    context.deviceContextInfo.deviceInfo.aicFrequency = AIC_FREQ;
    context.deviceContextInfo.deviceStart.cntVct = 0;
    context.deviceContextInfo.deviceFilePath = FREQ_LPM_PATH;
    std::vector<FreqData> freqLpm{CreateFreqData(), CreateFreqData()};
    WriteBin(freqLpm, File::PathJoin({FREQ_LPM_PATH, "data"}), "lpmFreqConv.data.0.slice_0");
    MOCKER_CPP(&std::vector<HalFreqData>::resize, void(std::vector<HalFreqData>::*)(size_t)).stubs()
        .will(throws(std::bad_alloc()));
    ASSERT_EQ(Analysis::PARSER_PARSE_DATA_ERROR, freqParser.Run(dataInventory_, context));
}
}
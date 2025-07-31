/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_db_dumper_utest.cpp
 * Description        : CANNTraceDBDumper UT
 * Author             : msprof team
 * Creation Date      : 2025/07/23
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"

using namespace Analysis::Domain;

const std::string HOST_FILE_PATH = "host";

class ModelNameDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(ModelNameDBDumperUtest, TestGenerateDataSuccess)
{
    ModelNameDBDumper dumper(HOST_FILE_PATH);
    ModelNameDBDumper::GraphIdMaps maps = {std::make_shared<MsprofAdditionalInfo>()};
    dumper.GenerateData(maps);
    EXPECT_EQ(maps.size(), 1ul);
}
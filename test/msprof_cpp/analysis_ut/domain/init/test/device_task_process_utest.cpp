/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task_process_utest.cpp
 * Description        : device_task_process UT
 * Author             : msprof team
 * Creation Date      : 2024/4/25
 * *****************************************************************************
 */

#include <vector>
#include <gtest/gtest.h>
#include "analysis/csrc/domain/services/init/include/device_task_process.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

const std::string DEVICE_PATH = "./device_0";
class DeviceTaskProcessUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(File::PathJoin({DEVICE_PATH, "sqlite"}), 0));
    }
};

TEST_F(DeviceTaskProcessUTest, ShouldInitDeviceTaskSuccess)
{
    DeviceTaskProcess deviceTaskProcess;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(Analysis::ANALYSIS_OK, deviceTaskProcess.Run(dataInventory_, deviceContext));
}
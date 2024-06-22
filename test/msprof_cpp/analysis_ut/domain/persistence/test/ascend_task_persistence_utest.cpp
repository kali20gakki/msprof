/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_persistence_utest.cpp
 * Description        : PmuAssociation UT
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/ascend_task_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;


namespace Analysis {
namespace Domain {
namespace {
const std::string DEVICE_PATH = "./device_0";
const int TEST_ID = 1;
}
class AscendTaskPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<TopDownTask>>();
        dataInventory_.Inject(data);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<TopDownTask> GenerateTopDownTask()
{
    std::vector<TopDownTask> res;
    res.emplace_back(TEST_ID, TEST_ID, TEST_ID, UINT32_MAX, TEST_ID, "AI_CORE", "AIC", UINT32_MAX, TEST_ID, TEST_ID,
                     TEST_ID);
    res.emplace_back(TEST_ID, TEST_ID, TEST_ID, UINT32_MAX, TEST_ID, "AI_CPU", "AI_CPU", UINT32_MAX, TEST_ID, TEST_ID,
                     TEST_ID);
    return res;
}

TEST_F(AscendTaskPersistenceUTest, ShouldSaveAscendTaskDataSuccess)
{
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    auto ascendTaskS = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    auto ascendTask = GenerateTopDownTask();
    ascendTaskS->swap(ascendTask);
    ASSERT_EQ(ANALYSIS_OK, taskPersistence.Run(dataInventory_, context));
}

TEST_F(AscendTaskPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_ERROR, taskPersistence.Run(dataInventory_, context));
}
}
}
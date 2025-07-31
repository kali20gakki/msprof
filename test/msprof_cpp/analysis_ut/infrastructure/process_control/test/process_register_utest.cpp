/* ******************************************************************************
版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_control_utest.cpp
 * Description        : ProcessControl UT
 * Author             : msprof team
 * Creation Date      : 2025/7/15
 * *****************************************************************************
 */
#include <gtest/gtest.h>
#include "analysis/csrc/infrastructure/process/include/process_register.h"

using namespace Analysis::Infra;

// 使用 type_index 作为测试用键
class ProcessRegisterUtest : public ::testing::Test {
protected:
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(ProcessRegisterUtest, TestGetContainerSuccess)
{
    ProcessCollection& container1 = ProcessRegister::GetContainer();
    ProcessCollection& container2 = ProcessRegister::GetContainer();
    EXPECT_EQ(&container1, &container2);
}

TEST_F(ProcessRegisterUtest, TestProcessRegisterWithCreator)
{
    std::type_index selfType(typeid(int));
    ProcessCreator creator = []() {
        return std::unique_ptr<Process>();
    };
    bool mandatory = true;
    std::string processName = "MyProcess";
    std::vector<std::type_index> preProcessType = {typeid(float), typeid(double)};

    ProcessRegister registerObj(selfType, creator, mandatory, processName.c_str(), std::move(preProcessType));

    const auto& info = ProcessRegister::GetContainer()[selfType];

    EXPECT_EQ(info.mandatory, mandatory);
    EXPECT_EQ(info.processDependence, preProcessType);
    EXPECT_EQ(info.processName, processName);
}

TEST_F(ProcessRegisterUtest, TestProcessRegisterWithParamTypes)
{
    std::type_index selfType(typeid(double));
    std::vector<std::type_index> paramTypes = {typeid(int), typeid(float)};

    ProcessRegister registerObj(selfType, std::move(paramTypes));

    const auto& info = ProcessRegister::GetContainer()[selfType];

    EXPECT_EQ(info.paramTypes, paramTypes);
}

TEST_F(ProcessRegisterUtest, TestProcessRegisterWithChipIds)
{
    std::type_index selfType(typeid(char));
    std::vector<uint32_t> chipIds = {1, 2, 3};

    ProcessRegister registerObj(selfType, {1, 2, 3});

    const auto& info = ProcessRegister::GetContainer()[selfType];

    EXPECT_EQ(info.chipIds, chipIds);
}
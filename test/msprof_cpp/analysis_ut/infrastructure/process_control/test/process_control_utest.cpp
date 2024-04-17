/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_control_utest.cpp
 * Description        : ProcessControl UT
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "fake_process/pah_d.h"
#include "fake_process/pstart_a.h"
#include "fake_process/pstart_h.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"

using namespace testing;
using namespace Analysis;
using namespace Infra;

class ProcessControlUTest : public Test {
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
        dataInventory.RemoveRestData({});
    }

    DataInventory dataInventory;
};

/* 此测试套的TOPO图如下
                        ┌───────────────┐
                        │     PgF       │
                        │               │
            ┌───────────┤  CHIP_V1_1_0  │
            │           │  CHIP_V2_1_0  │
            │           └───────┬───────┘
            │                   │
            ▼                   ▼
   ┌───────────────┐    ┌───────────────┐
   │     PabC      │    │     PabG      │
   │               │    │               │
   │  CHIP_V1_1_0  │    │  CHIP_V1_1_0  │
   │               │    │  CHIP_V2_1_0  │
   └──┬────────┬───┘    └┬──────┬───────┘
      │        │         │      │
      │        ▼         ▼      │
      │    ┌───────────────┐    │         ┌───────────────┐
      │    │     PdB       │    │         │     PahD      │
      │    │               ├────┼────────►│               │
      │    │  CHIP_V1_1_0  │    │         │  CHIP_V1_1_0  │
      │    │  CHIP_V2_1_0  │    │ ┌───────┤  CHIP_V2_1_0  │
      │    └───────────────┘    │ │       └──────┬────────┘
      │                         │ │              │
      │                         │ │              │
      │                         ▼ ▼              ▼
      │             ┌───────────────┐      ┌───────────────┐
      │             │    PStartA    │      │    PstartH    │
      └────────────►│               │      │               │
                    │  CHIP_V1_1_0  │      │  CHIP_V1_1_0  │
                    │  CHIP_V2_1_0  │      │               │
                    └───────────────┘      └───────────────┘
*/

TEST_F(ProcessControlUTest, ShouldGetAllRegisterInfoRight)
{
    std::vector<uint32_t> expChipsPA{CHIP_V1_1_0, CHIP_V2_1_0};
    std::vector<std::type_index> expProcPreProc{typeid(Ps::PstartA), typeid(Ps::PstartH)};
    std::vector<std::type_index> exparamTypes{typeid(Ps::StartA), typeid(Ps::StartH)};
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ASSERT_EQ(regInfo.size(), 7ul);   // 应该有7个相关Process
    EXPECT_EQ(regInfo[typeid(Ps::PstartA)].chipIds, expChipsPA);
    EXPECT_EQ(regInfo[typeid(Ps::PahD)].processDependence, expProcPreProc);
    EXPECT_EQ(regInfo[typeid(Ps::PahD)].paramTypes, exparamTypes);
}

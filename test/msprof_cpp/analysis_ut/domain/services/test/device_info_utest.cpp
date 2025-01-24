/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_info_utest.cpp
 * Description        : device info json UT
 * Author             : msprof team
 * Creation Date      : 2024/6/22
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/device_context/device_context.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

const auto DEVICE_DIR = "./device_0";
const std::string INFO_JSON = "info.json";


class DeviceInfoUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_DIR));
        CreateInfoJson(DEVICE_DIR, 0);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DEVICE_DIR, 0));
    }

    static void CreateInfoJson(const std::string &filePath, uint16_t deviceId)
    {
        // info.json
        nlohmann::json info = {
            {"platform_version", "5"},
            {"devices", "0"},
            {"pid", "2376271"},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"},
                             {"aic_frequency", "1850"},
                             {"aiv_frequency", "1850aa"},
                             {"ai_core_num", 25},
                             {"aiv_num", 25}}}},
            {"hostname", "localhost"}
        };
        FileWriter infoWriter(File::PathJoin({filePath, INFO_JSON}));
        infoWriter.WriteText(info.dump());
    }
};

TEST_F(DeviceInfoUTest, TestGetInfoJsonShouldReturnFalseWhenInfoJsonInvalid)
{
    DeviceContext deviceContext;
    DeviceContextInfo deviceContextInfo;
    deviceContextInfo.deviceFilePath = DEVICE_DIR;
    deviceContext.deviceContextInfo = deviceContextInfo;
    EXPECT_FALSE(deviceContext.GetInfoJson());
}
}  // Domain
}  // Analysis

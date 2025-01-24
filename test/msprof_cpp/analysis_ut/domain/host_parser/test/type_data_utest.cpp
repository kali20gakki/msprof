/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : type_data_utest.cpp
 * Description        : type_data UT
 * Author             : MSPROF TEAM
 * Creation Date      : 2023/11/2023/11/20
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

const std::string TEST_FILE = "./unaging.additional.type_info_dic.slice_0";

class TypeDataUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        // 创建test_file文件
        FileWriter fw(TEST_FILE);
        // 有效输入
        fw.WriteText("20000_655361:isGraphNeedRebuild\n");
        fw.WriteText("5000_1125:CpuKernelLaunchExWithArgs_Big\n");
        fw.WriteText("5000_1077:MemCopy2DAsync\n");
        // 无效输入
        fw.WriteText("20000_655361:isGraphNeedRebuild:t1\n");
        fw.WriteText("5000_1125_t2:CpuKernelLaunchExWithArgs_Big\n");
        fw.WriteText("5000t3_1077:MemCopy2DAsync\n");
        fw.WriteText("5000_1077_t4:MemCopy2DAsync\n");
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile(TEST_FILE);  // 删除test_file文件
    }
};

TEST_F(TypeDataUTest, LoadShouldReturnFalseWhenFileNotExist)
{
    auto status = TypeData::GetInstance().Load("test");
    EXPECT_FALSE(status);
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, LoadShouldReturnTrueWhenFileIsOK)
{
    auto status = TypeData::GetInstance().Load("./");
    EXPECT_TRUE(status);
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetShouldReturnNumWhenNoTypeInDict)
{
    TypeData::GetInstance().Load("./");
    std::string expectRes = "10";
    auto str = TypeData::GetInstance().Get(100, 10);
    auto str2 = TypeData::GetInstance().Get(5000, 10);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    EXPECT_STREQ(str2.c_str(), expectRes.c_str());
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetShouldReturnStrWhenTypeInDict)
{
    TypeData::GetInstance().Load("./");
    std::string expectRes = "CpuKernelLaunchExWithArgs_Big";
    auto str = TypeData::GetInstance().Get(5000, 1125);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    TypeData::GetInstance().Clear();
}

TEST_F(TypeDataUTest, GetAllShouldReturn3ValidResInFile)
{
    TypeData::GetInstance().Load("./");
    int expectResSize = 2;
    int expect5000ResSize = 2;
    int expect20000ResSize = 1;
    uint16_t hcclLevel = 5000;
    uint16_t aclLevel = 20000;
    EXPECT_EQ(expectResSize, TypeData::GetInstance().GetAll().size());
    EXPECT_EQ(expect5000ResSize, TypeData::GetInstance().GetAll()[hcclLevel].size());
    EXPECT_EQ(expect20000ResSize, TypeData::GetInstance().GetAll()[aclLevel].size());
    TypeData::GetInstance().Clear();
}
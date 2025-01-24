/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_data_utest.cpp
 * Description        : hash_data UT
 * Author             : MSPROF TEAM
 * Creation Date      : 2023/11/2023/11/20
 * *****************************************************************************
 */

#include "gtest/gtest.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

const std::string TEST_FILE = "./unaging.additional.hash_dic.slice_0";

class HashDataUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        // 创建test_file文件
        FileWriter fw(TEST_FILE);
        // AssignAdd和AssignAdd:t1均为合法数据
        fw.WriteText("836640106292564866:AssignAdd\n");
        fw.WriteText("7419384796023234053:npu_runconfig/iterations_per_loop/Assign\n");
        fw.WriteText("836640106292564867:AssignAdd:t1\n");
        // 异常数据
        fw.WriteText("7419384796023234053t2:npu_runconfig/iterations_per_loop/Assign\n");
        fw.WriteText("836640106292564868:AssignAdd::t1\n");
        fw.WriteText("7419384796023234053t2:npu_runconfig/iterations_per_loop/Assign\n");
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile("unaging.additional.hash_dic.slice_0");  // 删除test_file文件
    }
};

TEST_F(HashDataUTest, LoadShouldReturnFalseWhenFileNotExist)
{
    auto status = HashData::GetInstance().Load("test");
    EXPECT_FALSE(status);
    HashData::GetInstance().Clear();
}

TEST_F(HashDataUTest, LoadShouldReturnTrueWhenFileIsOK)
{
    auto status = HashData::GetInstance().Load("./");
    EXPECT_TRUE(status);
    HashData::GetInstance().Clear();
}

TEST_F(HashDataUTest, GetShouldReturnNumWhenNoHashInDict)
{
    std::string expectRes = "10";
    auto str = HashData::GetInstance().Get(10);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
}

TEST_F(HashDataUTest, GetShouldReturnStrWhenHashInDictWithoutDelimiter)
{
    HashData::GetInstance().Load("./");
    std::string expectRes = "AssignAdd";
    auto str = HashData::GetInstance().Get(836640106292564866);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    HashData::GetInstance().Clear();
}

TEST_F(HashDataUTest, GetShouldReturnStrWhenHashInDict)
{
    HashData::GetInstance().Load("./");
    std::string expectRes = "AssignAdd:t1";
    auto str = HashData::GetInstance().Get(836640106292564867);
    EXPECT_STREQ(str.c_str(), expectRes.c_str());
    HashData::GetInstance().Clear();
}

TEST_F(HashDataUTest, GetAllShouldReturn2ValidResInFile)
{
    HashData::GetInstance().Load("./");
    int expectResSize = 4;
    EXPECT_EQ(expectResSize, HashData::GetInstance().GetAll().size());
    HashData::GetInstance().Clear();
}


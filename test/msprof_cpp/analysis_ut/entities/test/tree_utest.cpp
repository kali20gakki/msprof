/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_utest.cpp
 * Description        : Tree UT
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#include <string>
#include <memory>
#include <vector>
#include <random>
#include "gtest/gtest.h"
#include "event.h"
#include "tree.h"

using namespace Analysis::Entities;

class TreeUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(TreeUTest, GetRootNullptrAndNotNullptr)
{
    EventInfo testInfo{EventType::EVENT_TYPE_API, 0, 0, 0};
    auto eventPtr = std::make_shared<Event>(nullptr, "test", testInfo);
    auto treeNode = std::make_shared<TreeNode>(eventPtr);

    auto treeNull = std::make_shared<Tree>(nullptr);
    auto checkNullptr = treeNull->GetRoot();
    EXPECT_EQ(nullptr, checkNullptr);

    auto tree = std::make_shared<Tree>(treeNode);
    auto check = tree->GetRoot();
    EXPECT_NE(nullptr, check);
}
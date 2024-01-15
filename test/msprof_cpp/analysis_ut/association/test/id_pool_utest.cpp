/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_builder_utest.cpp
 * Description        : id pool UT
 * Author             : msprof team
 * Creation Date      : 2023/12/11
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <unordered_map>
#include "gtest/gtest.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/thread_pool.h"

using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;

class IdPoolUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(IdPoolUTest, GetIdShouldReturnNewIndexWhenInputIsStrAndKeyNotExist)
{
    uint64_t expect = 0;
    std::string opName = "conv2d";
    auto res = IdPool::GetInstance().GetId(opName);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetIdShouldReturnOriIndexWhenInputIsStrAndKeyExist)
{
    uint64_t expect = 0;
    std::string opName = "pool";
    IdPool::GetInstance().GetId(opName);
    auto res = IdPool::GetInstance().GetId(opName);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetIdShouldReturnNewIndexWhenInputIsTupleAndKeyNotExist)
{
    uint64_t expect = 0;
    uint32_t deviceId = 1;
    uint32_t modelId = 2;
    uint32_t streamId = 3;
    uint32_t taskId = 4;
    uint32_t contextId = 5;
    uint32_t batchId = 6;
    CorrelationTuple key = std::make_tuple(deviceId, modelId, streamId, taskId, contextId, batchId);
    auto res = IdPool::GetInstance().GetId(key);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetIdShouldReturnNewIndexWhenInputIsTupleAndKeyExist)
{
    uint64_t expect = 0;
    uint32_t deviceId = 2;
    uint32_t modelId = 3;
    uint32_t streamId = 4;
    uint32_t taskId = 4;
    uint32_t contextId = 5;
    uint32_t batchId = 6;
    CorrelationTuple key = std::make_tuple(deviceId, modelId, streamId, taskId, contextId, batchId);
    IdPool::GetInstance().GetId(key);
    auto res = IdPool::GetInstance().GetId(key);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetAllStringIdsShouldReturnStringIds)
{
    std::unordered_map<std::string, uint64_t> expect;
    uint64_t denseId = 2;
    uint64_t poolId = 1;
    uint64_t conv2dId = 0;
    expect["dense"] = denseId;
    expect["pool"] = poolId;
    expect["conv2d"] = conv2dId;
    std::string opName = "conv2d";
    IdPool::GetInstance().GetId(opName);
    opName = "pool";
    IdPool::GetInstance().GetId(opName);
    opName = "dense";
    IdPool::GetInstance().GetId(opName);
    auto res = IdPool::GetInstance().GetAllStringIds();
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, IdsShouldBeUniqueWhenInputIsStrInTheMultiThreadScenario)
{
    const int threadsNum = 200;
    const int tasksNum = 100;
    std::vector<uint64_t> taskResult;
    std::vector<uint64_t> taskRepeatedResult;
    ThreadPool pool(threadsNum);
    std::mutex taskMutex;
    std::mutex taskRepeatedMutex;

    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (uint64_t i = 0; i < tasksNum; i++) {
        auto taskGet = [i, &taskResult, &taskMutex]() {
            auto res = IdPool::GetInstance().GetId(std::to_string(i));
            std::lock_guard<std::mutex> lock(taskMutex);
            taskResult.emplace_back(res);
        };
        auto taskRepeatedGet = [i, &taskRepeatedResult, &taskRepeatedMutex]() {
            auto res = IdPool::GetInstance().GetId(std::to_string(i));
            std::lock_guard<std::mutex> lock(taskRepeatedMutex);
            taskRepeatedResult.emplace_back(res);
        };
        pool.AddTask(taskGet);
        pool.AddTask(taskRepeatedGet);
    }

    pool.WaitAllTasks();
    pool.Stop();
    std::sort(taskResult.begin(), taskResult.end());
    std::sort(taskRepeatedResult.begin(), taskRepeatedResult.end());
    for (uint64_t i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, taskResult[i]);
        EXPECT_EQ(i, taskRepeatedResult[i]);
    }
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, IdsShouldBeUniqueWhenInputIsTupleInTheMultiThreadScenario)
{
    const int threadsNum = 200;
    const int tasksNum = 100;
    std::vector<uint64_t> taskResult;
    std::vector<uint64_t> taskRepeatedResult;
    ThreadPool pool(threadsNum);
    std::mutex taskMutex;
    std::mutex taskRepeatedMutex;

    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (uint64_t i = 0; i < tasksNum; i++) {
        auto taskGet = [i, &taskResult, &taskMutex]() {
            auto res = IdPool::GetInstance().GetId(std::make_tuple(i, i, i, i, i, i));
            std::lock_guard<std::mutex> lock(taskMutex);
            taskResult.emplace_back(res);
        };
        auto taskRepeatedGet = [i, &taskRepeatedResult, &taskRepeatedMutex]() {
            auto res = IdPool::GetInstance().GetId(std::make_tuple(i, i, i, i, i, i));
            std::lock_guard<std::mutex> lock(taskRepeatedMutex);
            taskRepeatedResult.emplace_back(res);
        };
        pool.AddTask(taskGet);
        pool.AddTask(taskRepeatedGet);
    }

    pool.WaitAllTasks();
    pool.Stop();
    std::sort(taskResult.begin(), taskResult.end());
    std::sort(taskRepeatedResult.begin(), taskRepeatedResult.end());
    for (uint64_t i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, taskResult[i]);
        EXPECT_EQ(i, taskRepeatedResult[i]);
    }
    IdPool::GetInstance().Clear();
}
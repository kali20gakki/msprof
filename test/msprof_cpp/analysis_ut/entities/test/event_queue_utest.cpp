/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_queue_utest.cpp
 * Description        : EventQueue UT
 * Author             : msprof team
 * Creation Date      : 2023/11/8
 * *****************************************************************************
 */

#include <string>
#include <memory>
#include <vector>
#include <random>
#include "gtest/gtest.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/event_queue.h"
#include "collector/inc/toolchain/prof_common.h"

using namespace Analysis::Entities;

class EventQueueUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(EventQueueUTest, EventQueue)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 1);
    EXPECT_NE(nullptr, eventQueue);
}

TEST_F(EventQueueUTest, TestPushCheckBound)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 11);

    const int pushNum = 10;
    /* 测试Push函数 lock前Push需要满足维护队列bound值正确性 */
    for (int i = 0; i < pushNum; ++i) {
        EventInfo testInfo{EventType::EVENT_TYPE_API, 0, (uint64_t) i, (uint64_t) (2 * i)};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofApi>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    /* eventQueue中最大值应该为 2 * 9 == 18 */
    auto check = eventQueue->GetBound();
    const int expectNum = 18;
    EXPECT_EQ(expectNum, check);
}

TEST_F(EventQueueUTest, TestPushNullptr)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 11);

    const int pushNum = 10;
    /* nullptr无法push */
    for (int i = 0; i < pushNum; ++i) {
        eventQueue->Push(nullptr);
    }

    /* eventQueue仍然为空 */
    auto checkEmpty = eventQueue->Empty();
    EXPECT_EQ(true, checkEmpty);
    auto checkZero = eventQueue->GetSize();
    EXPECT_EQ(0, checkZero);
}

TEST_F(EventQueueUTest, PopWhenQueueNotEmpty)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 10);
    const int pushNum = 10;
    for (int i = 0; i < pushNum; ++i) {
        EventInfo testInfo{EventType::EVENT_TYPE_API, 0, (uint64_t) i, (uint64_t) (2 * i)};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofApi>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    /* 测试Pop函数 正常Pop */
    while (!eventQueue->Empty()) {
        auto checkEmpty = eventQueue->Pop();
        EXPECT_NE(nullptr, checkEmpty);
    }
}

TEST_F(EventQueueUTest, PopWhenQueueEmpty)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 10);
    /* 测试Pop函数 Empty时Pop失败 */
    auto checkEmpty = eventQueue->Empty();
    EXPECT_EQ(true, checkEmpty);
    auto checkPop = eventQueue->Pop();
    EXPECT_EQ(nullptr, checkPop);
}

TEST_F(EventQueueUTest, SortWhenStartEqualOrLevelEqual)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 30);
    const int pushStartNum = 10;
    /* 测试Sort函数 start不同 Level相同时 排序正确性 */
    for (int i = pushStartNum; i > 0; --i) {
        EventInfo testInfo{EventType::EVENT_TYPE_API, 0, (uint64_t) i, (uint64_t) (2 * i)};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofApi>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    const int pushLevelNum = 11;
/* 测试Sort函数 start 相同时按照level排序的正确性 */
    for (int i = 1; i < pushLevelNum; ++i) {
        EventInfo testInfo{EventType::EVENT_TYPE_API, (uint16_t) i, (uint64_t) 1, (uint64_t) 1};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofApi>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    eventQueue->Sort();
    auto checkFunc = [](const std::shared_ptr<Event> &event1, const std::shared_ptr<Event> &event2) {
        return event1->info.start < event2->info.start
            || (event1->info.start == event2->info.start && event1->info.level > event2->info.level);
    };

    uint64_t expectSize = 20;
    auto checkSize = eventQueue->GetSize();
    EXPECT_EQ(checkSize, expectSize);

    auto lastEvent = eventQueue->Pop();
    while (!eventQueue->Empty()) {
        auto nowEvent = eventQueue->Pop();
        auto check = checkFunc(lastEvent, nowEvent);
        EXPECT_EQ(true, check);
        lastEvent = nowEvent;
    }

    auto checkEmpty = eventQueue->Empty();
    EXPECT_EQ(true, checkEmpty);
}

TEST_F(EventQueueUTest, TopShouldTopValueSmallest)
{
    auto eventQueue = std::make_shared<EventQueue>(1, 10);
    const int pushStartNum = 10;
    for (int i = 0; i < pushStartNum; ++i) {
        EventInfo testInfo{EventType::EVENT_TYPE_API, 0, (uint64_t) i, (uint64_t) (2 * i)};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofApi>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    /* 测试Top函数 Sort后获取Top成功 */
    eventQueue->Sort();
    auto checkTop = eventQueue->Top();
    const int smallestNum = 0;
    EXPECT_EQ(smallestNum, checkTop->info.start);
    while (!eventQueue->Empty()) {
        auto checkNoEmpty = eventQueue->Top();
        EXPECT_NE(nullptr, checkNoEmpty);
        eventQueue->Pop();
    }

    /* 测试Top函数 Empty后获取Top失败 */
    auto checkPopEmpty = eventQueue->Top();
    EXPECT_EQ(nullptr, checkPopEmpty);
}
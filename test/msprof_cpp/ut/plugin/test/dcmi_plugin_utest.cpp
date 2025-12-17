/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "dcmi_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::error;

constexpr int DCMI_CARD_NUM = 4;
constexpr int CARD_IDX = 2;
constexpr int DEVICE_NUM_LESS_THAN_CARD_IDX = 1;
constexpr int DEVICE_NUM_GREATER_THAN_CARD_IDX = 2;
constexpr unsigned long long MAC_TX_PFC_PKT_NUM = 100;

int DcmiGetCardList(int *cardNum, int *cardList, int listLen)
{
    *cardNum = DCMI_CARD_NUM;
    listLen = std::min(*cardNum, listLen);
    for (int i = 0; i < *cardNum; i++) {
        cardList[i] = i;
    }
    return PROFILING_SUCCESS;
}

int DcmiGetDeviceNumInCard(int cardId, int *deviceNum)
{
    *deviceNum = cardId < CARD_IDX ? DEVICE_NUM_LESS_THAN_CARD_IDX : DEVICE_NUM_GREATER_THAN_CARD_IDX;
    return PROFILING_SUCCESS;
}

int DcmiGetNetdevPktStatsInfo(int cardId, int deviceId, int portId, struct dcmi_network_pkt_stats_info *statsInfo)
{
    statsInfo->mac_tx_pfc_pkt_num = MAC_TX_PFC_PKT_NUM;
    return PROFILING_SUCCESS;
}

class DcmiPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(DcmiPluginUtest, LoadDcmiSoWillCreatePluginHandleWhenFirstLoad)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&DcmiPlugin::GetAllFunction)
        .stubs();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(nullptr, dcmiPlugin->pluginHandle_);
    dcmiPlugin->LoadDcmiSo();
    EXPECT_NE(nullptr, dcmiPlugin->pluginHandle_);
    dcmiPlugin->pluginHandle_ = nullptr;
}

TEST_F(DcmiPluginUtest, LoadDcmiSoWillReturnWhenOpenPluginFromEnvAndOpenPluginFromLdcfgFail)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    dcmiPlugin->LoadDcmiSo();
    EXPECT_EQ(nullptr, dcmiPlugin->dcmiInitFunc_);
    dcmiPlugin->pluginHandle_ = nullptr;
}

TEST_F(DcmiPluginUtest, LoadDcmiSoWillGetFunctionWhenOpenPluginFromEnvSucc)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dcmiPlugin->LoadDcmiSo();
    dcmiPlugin->pluginHandle_ = nullptr;
}

TEST_F(DcmiPluginUtest, LoadDcmiSoWillGetFunctionWhenOpenPluginFromLdcfgSucc)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dcmiPlugin->LoadDcmiSo();
    dcmiPlugin->pluginHandle_ = nullptr;
}

TEST_F(DcmiPluginUtest, LoadDcmiSo)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dcmiPlugin->LoadDcmiSo();
    EXPECT_NE(nullptr, dcmiPlugin->pluginHandle_);
}

TEST_F(DcmiPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    const std::string funcName = "dcmi_get_card_list";
    EXPECT_EQ(true, dcmiPlugin->IsFuncExist(funcName));
    EXPECT_EQ(false, dcmiPlugin->IsFuncExist(funcName));
}

TEST_F(DcmiPluginUtest, MsprofDcmiInit)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    dcmiPlugin->dcmiInitFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, dcmiPlugin->MsprofDcmiInit());
    dcmiPlugin->dcmiInitFunc_ = []() -> int { return PROFILING_SUCCESS; };
    EXPECT_EQ(PROFILING_SUCCESS, dcmiPlugin->MsprofDcmiInit());
}

TEST_F(DcmiPluginUtest, MsprofDcmiGetCardList)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    int cardNum = 0;
    constexpr int listLen = 8;
    int cardList[listLen] = {0};
    dcmiPlugin->dcmiGetCardListFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, dcmiPlugin->MsprofDcmiGetCardList(&cardNum, cardList, listLen));
    dcmiPlugin->dcmiGetCardListFunc_ = DcmiGetCardList;
    EXPECT_EQ(PROFILING_SUCCESS, dcmiPlugin->MsprofDcmiGetCardList(&cardNum, cardList, listLen));
    EXPECT_EQ(DCMI_CARD_NUM, cardNum);
    for (int i = 0; i < cardNum; i++) {
        EXPECT_EQ(i, cardList[i]);
    }
}

TEST_F(DcmiPluginUtest, MsprofDcmiGetDeviceNumInCard)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    int cardId = 0;
    int deviceNum = 0;
    dcmiPlugin->dcmiGetDeviceNumInCardFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, dcmiPlugin->MsprofDcmiGetDeviceNumInCard(cardId, &deviceNum));
    dcmiPlugin->dcmiGetDeviceNumInCardFunc_ = DcmiGetDeviceNumInCard;
    EXPECT_EQ(PROFILING_SUCCESS, dcmiPlugin->MsprofDcmiGetDeviceNumInCard(cardId, &deviceNum));
    EXPECT_EQ(DEVICE_NUM_LESS_THAN_CARD_IDX, deviceNum);
    cardId = CARD_IDX;
    EXPECT_EQ(PROFILING_SUCCESS, dcmiPlugin->MsprofDcmiGetDeviceNumInCard(cardId, &deviceNum));
    EXPECT_EQ(DEVICE_NUM_GREATER_THAN_CARD_IDX, deviceNum);
}

TEST_F(DcmiPluginUtest, MsprofDcmiGetNetdevPktStatsInfo)
{
    GlobalMockObject::verify();
    auto dcmiPlugin = DcmiPlugin::instance();
    int cardId = 0;
    int deviceId = 0;
    int portId = 0;
    struct dcmi_network_pkt_stats_info statsInfo;
    dcmiPlugin->dcmiGetNetdevPktStatsInfoFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, dcmiPlugin->MsprofDcmiGetNetdevPktStatsInfo(cardId, deviceId, portId, &statsInfo));
    dcmiPlugin->dcmiGetNetdevPktStatsInfoFunc_ = DcmiGetNetdevPktStatsInfo;
    EXPECT_EQ(PROFILING_SUCCESS, dcmiPlugin->MsprofDcmiGetNetdevPktStatsInfo(cardId, deviceId, portId, &statsInfo));
    EXPECT_EQ(MAC_TX_PFC_PKT_NUM, statsInfo.mac_tx_pfc_pkt_num);
}

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
#include <iostream>
#include <stdexcept>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "message/prof_params.h"
#include "message/prof_json_config.h"

using namespace analysis::dvvp::message;

class ProfJsonConfigTest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(ProfJsonConfigTest, TestJsonStringToAclCfgShouldReturnSuccessWhenCfgValid)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"aic_metrics\": \"PipeUtilization\",\"aicpu\": \"on\", \"ascendcl\": \"on\",\
                        \"dvpp_freq\": \"50\", \"hccl\": \"off\", \"host_sys\": \"cpu,mem\",\
                        \"host_sys_usage\": \"cpu,mem\", \"host_sys_usage_freq\": \"50\",\
                        \"l2\": \"off\", \"msproftx\": \"off\",\
                        \"output\": \"/home/result_dir/test_IsSwitchAclJson\",\
                        \"runtime_api\": \"on\", \"storage_limit\": \"200MB\", \"switch\": \"on\",\
                        \"sys_hardware_mem_freq\": \"50\", \"sys_interconnection_freq\": \"50\",\
                        \"sys_io_sampling_freq\": \"100\", \"task_time\": \"on\"}";
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfAclConfig>();
    EXPECT_EQ(PROFILING_SUCCESS, JsonStringToAclCfg(cfg, inputCfgPb));
    uint32_t dvppFreq = 50;
    uint32_t hostSysUsageFreq = 50;
    uint32_t instrProfilingFreq = 0;
    uint32_t sysHardwareMemFreq = 50;
    uint32_t sysInterconnectionFreq = 50;
    uint32_t sysIoSamplingFreq = 100;
    EXPECT_EQ(dvppFreq, inputCfgPb->dvppFreq);
    EXPECT_EQ(hostSysUsageFreq, inputCfgPb->hostSysUsageFreq);
    EXPECT_EQ(instrProfilingFreq, inputCfgPb->instrProfilingFreq);
    EXPECT_EQ(sysHardwareMemFreq, inputCfgPb->sysHardwareMemFreq);
    EXPECT_EQ(sysInterconnectionFreq, inputCfgPb->sysInterconnectionFreq);
    EXPECT_EQ(sysIoSamplingFreq, inputCfgPb->sysIoSamplingFreq);
    EXPECT_EQ("PipeUtilization", inputCfgPb->aicMetrics);
    EXPECT_EQ("on", inputCfgPb->aicpu);
    EXPECT_EQ("", inputCfgPb->aivMetrics);
    EXPECT_EQ("on", inputCfgPb->ascendcl);
    EXPECT_EQ("off", inputCfgPb->hccl);
    EXPECT_EQ("cpu,mem", inputCfgPb->hostSys);
    EXPECT_EQ("cpu,mem", inputCfgPb->hostSysUsage);
    EXPECT_EQ("off", inputCfgPb->l2);
    EXPECT_EQ("", inputCfgPb->llcProfiling);
    EXPECT_EQ("off", inputCfgPb->msproftx);
    EXPECT_EQ("/home/result_dir/test_IsSwitchAclJson", inputCfgPb->output);
    EXPECT_EQ("on", inputCfgPb->runtimeApi);
    EXPECT_EQ("200MB", inputCfgPb->storageLimit);
    EXPECT_EQ("on", inputCfgPb->profSwitch);
    EXPECT_EQ("on", inputCfgPb->taskTime);
}

TEST_F(ProfJsonConfigTest, TestJsonStringToAclCfgShouldReturnFailedWhenCfgHaveInValidKey)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"aic_metrics\": \"PipeUtilization\", \"task_time\": \"on\", \"x\" : \"on\"}";
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfAclConfig>();
    EXPECT_EQ(PROFILING_FAILED, JsonStringToAclCfg(cfg, inputCfgPb));
}

TEST_F(ProfJsonConfigTest, TestJsonStringToAclCfgShouldReturnFailedWhenCfgInValid)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"aic_metrics\", \"task_time\": \"on\",}";
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfAclConfig>();
    EXPECT_EQ(PROFILING_FAILED, JsonStringToAclCfg(cfg, inputCfgPb));
}

TEST_F(ProfJsonConfigTest, TestJsonStringToGeCfgShouldReturnSuccessWhenCfgValid)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"output\": \"./result_dir\",\"training_trace\": \"on\",\"task_trace\": \"on\",\
                        \"hccl\": \"off\",\"aicpu\": \"on\", \"fp_point\": \"\",\"bp_point\": \"\",\
                        \"aic_metrics\":\"PipeUtilization\", \"l2\": \"on\",\"msproftx\": \"off\",\
                        \"task_time\": \"on\", \"runtime_api\": \"on\",\"sys_hardware_mem_freq\": \"50\",\
                        \"llc_profiling\": \"write\",\"sys_io_sampling_freq\": \"100\",\
                        \"sys_interconnection_freq\": \"50\", \"dvpp_freq\": \"50\",\"host_sys\":\"cpu,mem\",\
                        \"host_sys_usage\": \"cpu,mem\", \"host_sys_usage_freq\": \"50\"}";
    SHARED_PTR_ALIA<ProfGeOptionsConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfGeOptionsConfig>();
    EXPECT_EQ(PROFILING_SUCCESS, JsonStringToGeCfg(cfg, inputCfgPb));
    uint32_t dvppFreq = 50;
    uint32_t hostSysUsageFreq = 50;
    uint32_t instrProfilingFreq = 0;
    uint32_t sysHardwareMemFreq = 50;
    uint32_t sysInterconnectionFreq = 50;
    uint32_t sysIoSamplingFreq = 100;
    EXPECT_EQ(dvppFreq, inputCfgPb->dvppFreq);
    EXPECT_EQ(hostSysUsageFreq, inputCfgPb->hostSysUsageFreq);
    EXPECT_EQ(instrProfilingFreq, inputCfgPb->instrProfilingFreq);
    EXPECT_EQ(sysHardwareMemFreq, inputCfgPb->sysHardwareMemFreq);
    EXPECT_EQ(sysInterconnectionFreq, inputCfgPb->sysInterconnectionFreq);
    EXPECT_EQ(sysIoSamplingFreq, inputCfgPb->sysIoSamplingFreq);
    EXPECT_EQ("PipeUtilization", inputCfgPb->aicMetrics);
    EXPECT_EQ("on", inputCfgPb->aicpu);
    EXPECT_EQ("", inputCfgPb->bpPoint);
    EXPECT_EQ("", inputCfgPb->fpPoint);
    EXPECT_EQ("off", inputCfgPb->hccl);
    EXPECT_EQ("cpu,mem", inputCfgPb->hostSys);
    EXPECT_EQ("cpu,mem", inputCfgPb->hostSysUsage);
    EXPECT_EQ("on", inputCfgPb->l2);
    EXPECT_EQ("write", inputCfgPb->llcProfiling);
    EXPECT_EQ("off", inputCfgPb->msproftx);
    EXPECT_EQ("./result_dir", inputCfgPb->output);
    EXPECT_EQ("on", inputCfgPb->runtimeApi);
    EXPECT_EQ("on", inputCfgPb->taskTime);
    EXPECT_EQ("on", inputCfgPb->trainingTrace);
    EXPECT_EQ("on", inputCfgPb->taskTrace);
}

TEST_F(ProfJsonConfigTest, TestJsonStringToGeCfgShouldReturnFailedWhenCfgHaveInValidKey)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"aic_metrics\": \"PipeUtilization\", \"task_time\": \"on\", \"x\" : \"on\"}";
    SHARED_PTR_ALIA<ProfGeOptionsConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfGeOptionsConfig>();
    EXPECT_EQ(PROFILING_FAILED, JsonStringToGeCfg(cfg, inputCfgPb));
}

TEST_F(ProfJsonConfigTest, TestJsonStringToGeCfgShouldReturnFailedWhenCfgInValid)
{
    GlobalMockObject::verify();
    std::string cfg = "{\"aic_metrics\", \"task_time\": \"on\",}";
    SHARED_PTR_ALIA<ProfGeOptionsConfig> inputCfgPb = nullptr;
    inputCfgPb = std::make_shared<ProfGeOptionsConfig>();
    EXPECT_EQ(PROFILING_FAILED, JsonStringToGeCfg(cfg, inputCfgPb));
}
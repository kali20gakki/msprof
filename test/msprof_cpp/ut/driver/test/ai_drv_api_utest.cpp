#include <dlfcn.h>
#include <map>
#include <errno.h>
#include <iostream>
#include <stdexcept>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "errno/error_code.h"
#include "config/config_manager.h"
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "driver_plugin.h"
#include "utils/utils.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace Collector::Dvvp::Plugin;
using namespace  Analysis::Dvvp::Common::Config;
#define CHANNEL_STR(s) #s

///////////////////////////////////////////////////////////////////
class DRIVER_AI_DRV_API_TEST: public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetDevNum) {
    uint32_t num_dev = 0;

    MOCKER(&DriverPlugin::MsprofDrvGetDevNum)
        .stubs()
        .with(outBoundP(&num_dev))
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetDevNum());
    EXPECT_EQ((int)num_dev, analysis::dvvp::driver::DrvGetDevNum());
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetDevIds) {
    std::vector<int> dev_ids;

    uint32_t num_dev = 0;

    MOCKER(&DriverPlugin::MsprofDrvGetDevIDs)
        .stubs()
        .with(outBoundP(&num_dev))
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetDevIds(0, dev_ids));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetDevIds(1, dev_ids));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetDevIds(1, dev_ids));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvPeripheralStart) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_NIC;
    int prof_sample_period = 10;
    std::string prof_data_file_path = "/path/to/data";

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.configP          = nullptr;
    peripheralCfg.configSize       = 0;
    peripheralCfg.profChannel      = prof_channel;
    peripheralCfg.profDeviceId     = prof_device_id;
    peripheralCfg.profSamplePeriod = prof_sample_period;
    peripheralCfg.profDataFilePath = prof_data_file_path;

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvPeripheralStart(peripheralCfg));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvPeripheralStart(peripheralCfg));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvTscpuStart) {
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.profDeviceId = 0;
    peripheralCfg.profChannel = analysis::dvvp::driver::PROF_CHANNEL_TS_CPU;
    peripheralCfg.profSamplePeriod = 10;
    peripheralCfg.profDataFilePath = "/path/to/data";

    std::vector<std::string> prof_events;
    prof_events.push_back("0x11");

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvTscpuStart(peripheralCfg, prof_events));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvTscpuStart(peripheralCfg, prof_events));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvAicoreStart) {
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.profDeviceId = 0;
    peripheralCfg.profChannel = analysis::dvvp::driver::PROF_CHANNEL_AI_CORE;
    peripheralCfg.profSamplePeriod = 10;
    peripheralCfg.profDataFilePath = "/path/to/data";

    std::vector<std::string> prof_events;
    std::vector<int> prof_cores;
    prof_events.push_back("0x11");
    prof_cores.push_back(0);

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvAicoreStart(peripheralCfg, prof_cores, prof_events));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvAicoreStart(peripheralCfg, prof_cores, prof_events));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvInstrProfileStart)
{
    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.profDeviceId = 0;
    peripheralCfg.profChannel = analysis::dvvp::driver::PROF_CHANNEL_AI_CORE;
    peripheralCfg.profSamplePeriod = 1;
    peripheralCfg.profDataFilePath = "/path/to/data";
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvInstrProfileStart(0, peripheralCfg.profChannel, 1));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvInstrProfileStart(0, peripheralCfg.profChannel, 1));
}
TEST_F(DRIVER_AI_DRV_API_TEST, DrvAicoreTaskBasedStart) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_AI_CORE;
    std::vector<std::string> prof_events;
    std::vector<int> prof_cores;
    std::string prof_data_file_path = "/path/to/data";

    prof_events.push_back("0x11");

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvAicoreTaskBasedStart(
        prof_device_id, prof_channel, prof_events));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvAicoreTaskBasedStart(
        prof_device_id, prof_channel, prof_events));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvFftsProfileStart) {
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_FFTS_PROFILIE_TASK;
    std::vector<int>  prof_cores;
    std::vector<std::string> prof_events;
    std::vector<int>  prof_aivCores;
    std::vector<std::string> prof_aivEvents;
    std::string prof_data_file_path = "/path/to/data";

    prof_cores.push_back(0);
    prof_events.push_back("0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55");
    prof_aivCores.push_back(0);
    prof_aivEvents.push_back("0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55");
    analysis::dvvp::driver::DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = 0;
    drvPeripheralProfileCfg.profChannel = prof_channel;
    drvPeripheralProfileCfg.profSamplePeriod = 10;
    drvPeripheralProfileCfg.profSamplePeriodHi = 10;
    drvPeripheralProfileCfg.cfgMode = 1;
    drvPeripheralProfileCfg.aicMode = 1;
    drvPeripheralProfileCfg.aivMode = 1;

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvFftsProfileStart(drvPeripheralProfileCfg,
                prof_cores, prof_events, prof_aivCores, prof_aivEvents));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvFftsProfileStart(drvPeripheralProfileCfg,
                prof_cores, prof_events, prof_aivCores, prof_aivEvents));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvTsFwStart) {
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.profDeviceId = 0;
    peripheralCfg.profChannel = analysis::dvvp::driver::PROF_CHANNEL_TS_FW;
    peripheralCfg.profSamplePeriod = 1;
    peripheralCfg.profDataFilePath = "/path/to/data";

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvTsFwStart(peripheralCfg, nullptr));

    auto profileParams = std::make_shared<analysis::dvvp::message::ProfileParams>();

    profileParams->ts_timeline = "on";
    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvTsFwStart(peripheralCfg, profileParams));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvTsFwStart(peripheralCfg, profileParams));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvStarsSocLogStart) {
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg;
    peripheralCfg.profDeviceId = 0;
    peripheralCfg.profChannel = analysis::dvvp::driver::PROF_CHANNEL_STARS_SOC_LOG;
    peripheralCfg.profSamplePeriod = 10;
    peripheralCfg.profDataFilePath = "/path/to/data";

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvStarsSocLogStart(peripheralCfg, nullptr));

    auto profileParams = std::make_shared<analysis::dvvp::message::ProfileParams>();

    profileParams->stars_acsq_task = "on";
    profileParams->low_power  = "on";
    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvStarsSocLogStart(peripheralCfg, profileParams));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvStarsSocLogStart(peripheralCfg, profileParams));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvStop) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_HBM;

    MOCKER(&DriverPlugin::MsprofDrvStop)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvStop(prof_device_id, prof_channel));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvStop(prof_device_id, prof_channel));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvChannelRead) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_HBM;
    unsigned char out_buf[4096];
    uint32_t buf_size = 4096;

    MOCKER(&DriverPlugin::MsprofChannelRead)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(64))
	.then(returnValue(PROF_STOPPED_ALREADY));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvChannelRead(
        prof_device_id, prof_channel, nullptr, 0));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvChannelRead(
        prof_device_id, prof_channel, out_buf, buf_size));

    EXPECT_EQ(64, analysis::dvvp::driver::DrvChannelRead(
        prof_device_id, prof_channel, out_buf, buf_size));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvChannelRead(
        prof_device_id, prof_channel, out_buf, buf_size));

}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvChannelPoll) {
    struct prof_poll_info out_buf[2];

    MOCKER(&DriverPlugin::MsprofChannelPoll)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvChannelPoll(
        nullptr, 0, 1));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvChannelPoll(
        out_buf, 2, 1));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvChannelPoll(
        out_buf, 2, 1));
}

int halProfDataFlush(unsigned int deviceId, unsigned int channelId, unsigned int *bufSize)
{
    return 0;
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvProfFlush) {
    MOCKER(&DriverPlugin::MsprofHalProfDataFlush)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_STOPPED_ALREADY))
        .then(returnValue(PROF_OK));
    unsigned int bufSize = 0;
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvProfFlush(
        0, 0, bufSize));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvProfFlush(
        0, 2, bufSize));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvProfFlush(
        0, 2, bufSize));
}
//////////////////////////////////////////////////////////////////////////

TEST_F(DRIVER_AI_DRV_API_TEST, DrvHwtsLogStart) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_HWTS_LOG;
    int prof_sample_period = 10;
    std::string prof_data_file_path = "/path/to/data";

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvHwtsLogStart(prof_device_id, prof_channel));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvHwtsLogStart(prof_device_id, prof_channel));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvFmkDataStart) {
    int prof_device_id = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_FMK;
    int prof_sample_period = 10;
    int real_time = 0;
    std::string prof_data_file_path = "/path/to/data";
    std::vector<std::string> prof_events;

    prof_events.push_back("0x5b");

    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvFmkDataStart(prof_device_id, prof_channel));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvFmkDataStart(prof_device_id, prof_channel));
}


TEST_F(DRIVER_AI_DRV_API_TEST, DrvL2CacheTaskStart) {
    int profDeviceId = 0;
    analysis::dvvp::driver::AI_DRV_CHANNEL prof_channel = analysis::dvvp::driver::PROF_CHANNEL_FMK;
    int profSamplePeriod = 10;
    std::string profDataFilePath = "/path/to/data";
    std::vector<std::string> prof_events;

    prof_events.push_back("0x5b");
    MOCKER(&DriverPlugin::MsprofDrvStart)
        .stubs()
        .will(returnValue(PROF_ERROR))
        .then(returnValue(PROF_OK));

    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvL2CacheTaskStart(
        profDeviceId, prof_channel, prof_events));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvL2CacheTaskStart(
        profDeviceId, prof_channel, prof_events));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetHostPhyIdByDeviceIndex) {
    MOCKER(&DriverPlugin::MsprofDrvGetDevIDByLocalDevID)
        .stubs()
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));

    EXPECT_EQ(1, analysis::dvvp::driver::DrvGetHostPhyIdByDeviceIndex(1));
    EXPECT_EQ(0, analysis::dvvp::driver::DrvGetHostPhyIdByDeviceIndex(1));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetDevIdsStr) {
    uint32_t num_dev = 0;
    std::vector<int> devIds;
    devIds.push_back(1);
    MOCKER(&DriverPlugin::MsprofDrvGetDevNum)
        .stubs()
        .with(outBoundP(&num_dev))
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));

    num_dev = 1;
    MOCKER(&DriverPlugin::MsprofDrvGetDevIDs)
        .stubs()
        .with(outBoundP(&num_dev))
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));

    EXPECT_EQ("", analysis::dvvp::driver::DrvGetDevIdsStr());

    EXPECT_EQ("", analysis::dvvp::driver::DrvGetDevIdsStr());
    EXPECT_EQ("", analysis::dvvp::driver::DrvGetDevIdsStr());
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetPlatformInfo) {
    uint32_t platformInfo = 0;
    MOCKER(&DriverPlugin::MsprofDrvGetPlatformInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(MSPROF_HELPER_HOST))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetPlatformInfo(platformInfo));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetPlatformInfo(platformInfo));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetPlatformInfo(platformInfo));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvCheckIfHelperHost) {
    MOCKER(&DriverPlugin::MsprofDrvGetDevNum)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(MSPROF_HELPER_HOST));

    EXPECT_EQ(false, analysis::dvvp::driver::DrvCheckIfHelperHost());
    EXPECT_EQ(true, analysis::dvvp::driver::DrvCheckIfHelperHost());
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetDeviceFreq)
{
    uint32_t deviceId = 0;
    std::string freq;
    int64_t deviceFreq = 1000;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .with(any(), any(), any(), outBoundP(&deviceFreq))
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(true, analysis::dvvp::driver::DrvGetDeviceFreq(deviceId, freq));
    EXPECT_EQ(false, analysis::dvvp::driver::DrvGetDeviceFreq(deviceId, freq));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetApiVersion)
{
    int32_t ver = 1;
    MOCKER(&DriverPlugin::MsprofHalGetApiVersion)
        .stubs()
        .with(outBoundP(&ver))
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(1, analysis::dvvp::driver::DrvGetApiVersion());
    EXPECT_EQ(0, analysis::dvvp::driver::DrvGetApiVersion());
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetEnvType)
{
    uint32_t deviceId = 0;
    int64_t envType = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetEnvType(deviceId, envType));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetEnvType(deviceId, envType));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetCtrlCpuId)
{
    uint32_t deviceId = 0;
    int64_t ctrlCpuId = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetCtrlCpuId(deviceId, ctrlCpuId));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetCtrlCpuId(deviceId, ctrlCpuId));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetCtrlCpuCoreNum)
{
    uint32_t deviceId = 0;
    int64_t ctrlNum = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetCtrlCpuCoreNum(deviceId, ctrlNum));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetCtrlCpuCoreNum(deviceId, ctrlNum));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetCtrlCpuEndianLittle)
{
    uint32_t deviceId = 0;
    int64_t ctrlCpuEndianLittle = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetCtrlCpuEndianLittle(deviceId, ctrlCpuEndianLittle));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetCtrlCpuEndianLittle(deviceId, ctrlCpuEndianLittle));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAiCpuCoreNum)
{
    uint32_t deviceId = 0;
    int64_t aiCpuCoreNum = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAiCpuCoreNum(deviceId, aiCpuCoreNum));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAiCpuCoreNum(deviceId, aiCpuCoreNum));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAiCpuCoreId)
{
    uint32_t deviceId = 0;
    int64_t aiCpuCoreId = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAiCpuCoreId(deviceId, aiCpuCoreId));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAiCpuCoreId(deviceId, aiCpuCoreId));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAiCpuOccupyBitmap)
{
    uint32_t deviceId = 0;
    int64_t aiCpuOccupyBitmap = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap(deviceId, aiCpuOccupyBitmap));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap(deviceId, aiCpuOccupyBitmap));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetTsCpuCoreNum)
{
    uint32_t deviceId = 0;
    int64_t tsCpuCoreNum = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetTsCpuCoreNum(deviceId, tsCpuCoreNum));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetTsCpuCoreNum(deviceId, tsCpuCoreNum));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAiCoreId)
{
    uint32_t deviceId = 0;
    int64_t aiCoreId = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAiCoreId(deviceId, aiCoreId));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAiCoreId(deviceId, aiCoreId));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAiCoreNum)
{
    uint32_t deviceId = 0;
    int64_t aiCoreNum = 0;
    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAiCoreNum(deviceId, aiCoreNum));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAiCoreNum(deviceId, aiCoreNum));
}

TEST_F(DRIVER_AI_DRV_API_TEST, DrvGetAivNum)
{
    uint32_t deviceId = 0;
    int64_t aivNum = 0;

    MOCKER_CPP(&ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(0))
        .then(returnValue(5)); // 5 platform with aiv
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAivNum(deviceId, aivNum));

    MOCKER(&DriverPlugin::MsprofHalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NOT_SUPPORT))
        .then(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INNER_ERR));

    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAivNum(deviceId, aivNum));
    EXPECT_EQ(PROFILING_SUCCESS, analysis::dvvp::driver::DrvGetAivNum(deviceId, aivNum));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetAivNum(deviceId, aivNum));
}

TEST_F(DRIVER_AI_DRV_API_TEST, TestDrvIsSupportAdprofShouldReturnFalseWhenMINITYPE)
{
    uint32_t deviceId = 0;
    MOCKER_CPP(&ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(0));
    EXPECT_FALSE(analysis::dvvp::driver::DrvIsSupportAdprof(deviceId));
}

TEST_F(DRIVER_AI_DRV_API_TEST, TestDrvIsSupportAdprofShouldReturnTrueWhenNotSplitMode)
{
    uint32_t deviceId = 0;
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetApiVersion)
        .stubs()
        .will(returnValue(analysis::dvvp::driver::SUPPORT_ADPROF_VERSION));
    MOCKER_CPP(&ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(5));  // chip 5
    EXPECT_TRUE(analysis::dvvp::driver::DrvIsSupportAdprof(deviceId));
}

drvError_t g_error = (drvError_t)0;
extern "C" drvError_t halGetDeviceInfoByBuff(DriverPlugin *This, uint32_t devId, int32_t moduleType, int32_t infoType,
                                             VOID_PTR value, int32_t *len)
{
    if (moduleType = MODULE_TYPE_QOS) {
        QosProfileInfo *info = (QosProfileInfo*)value;
        uint8_t mpamId = 12;
        if (info->mode == 0) {
            info->streamNum = 1;
            info->mpamId[0] = mpamId;
        } else if (info->mode == 1 && info->mpamId[0] == mpamId) {
            strcpy_s(info->streamName, QOS_STREAM_NAME_MAX_LENGTH, "st_mpamid_12");
        }
    }
    return g_error;
}

TEST_F(DRIVER_AI_DRV_API_TEST, GetQosProfileInfo)
{
    MOCKER(&Collector::Dvvp::Plugin::DriverPlugin::MsprofHalGetDeviceInfoByBuff)
        .stubs()
        .will(returnValue(DRV_ERROR_NOT_SUPPORT))
        .then(returnValue(DRV_ERROR_NO_DEVICE))
        .then(invoke(halGetDeviceInfoByBuff));
    MOCKER_CPP(&ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(0))
        .then(returnValue(5)); // chip 5
    std::string info;
    std::vector<uint8_t> events;
    GetQosProfileInfo(0, info, events);
    GetQosProfileInfo(0, info, events);
    GetQosProfileInfo(0, info, events);
    GetQosProfileInfo(0, info, events);
    EXPECT_EQ(1, events.size());
}
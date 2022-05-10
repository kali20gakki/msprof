#include <dlfcn.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "mmpa_stub.h"
#include <unistd.h>
#include <memory>
#include <thread>
#include <sstream>
#include <iostream>
#include <string>
#include "errno/error_code.h"
#include "securec.h"
#include "message/codec.h"
#include "utils/utils.h"
#include "prof_engine.h"
#include "prof_reporter.h"
#include "engine_mgr.h"
#include "config/config_manager.h"
#include "proto/profiler.pb.h"
#include "proto/profiler_ext.pb.h"
#include "uploader_mgr.h"
#include "prof_stamp_pool.h"
#include "msprof_tx_manager.h"
#include "prof_acl_mgr.h"

static uint8_t g_device_id = 0;
static bool createPluginTrue = true;
static bool pluginInitTrue = true;
static std::string moduleName= "counter";
using namespace analysis::dvvp::common::error;
using namespace Msprof::Engine;
using namespace Msprof::MsprofTx;
using namespace analysis::dvvp::proto;

class MSPROF_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

void func_counter(Reporter* reporter) {
    uint64_t loop = 0;
    std::string data_str = "This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data testThis is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test,This is func counter test, second send data test, third send data test\n";

    while (loop < 2500) {
        loop++;
        unsigned char * data = (unsigned char *)malloc(1024);
        memset(data, '\0', 1024);
        memcpy(data, data_str.c_str(), data_str.size());
        ReporterData r_data;
        r_data.deviceId = g_device_id;
        r_data.data = data;
        memcpy(r_data.tag, "tag", 4);
        r_data.dataLen = data_str.size();
        reporter->Report(&r_data);
        free(data);
        usleep(100);
    }
    printf("send data discardable==true finished %lu\n", loop);
    reporter->Flush();
}

class PluginImpl: public PluginIntf {
public:
    explicit PluginImpl(const std::string & module)
    : reporter_(nullptr)
    , module_(module) {

    }
    virtual ~PluginImpl() {
    }

public:
    virtual int Init(const Reporter* reporter) {
        if (pluginInitTrue) {
            reporter_ = const_cast<Reporter*>(reporter);
            t1_ = std::make_shared<std::thread>(&func_counter, reporter_);
            return 0; //PROFILING_SUCCESS;
        } else {
            return -1; //PROFILING_FAILED
        }
    }

    int OnNewConfig(const ModuleJobConfig * config) {
        if (config == nullptr) {
            printf("Input parameter is null\n");
            return -1;
        }

        std::cout<<"==> Module Started:"<<module_<<std::endl;
        for (auto iter = config->switches.begin(); iter != config->switches.end(); ++iter) {
            std::cout<<iter->first<<"="<<iter->second<<std::endl;
        }
        return 0;
    }

    virtual int UnInit() {
        std::cout<<"==> Module Ended:"<<module_<<std::endl;
        t1_->join();
        t1_.reset();
        printf("OnJobEnd finished\n\n");
        return 0;
    }

private:
    Reporter* reporter_;
    std::string module_;
    std::shared_ptr<std::thread> t1_;
};

class EngineImpl_0 : public EngineIntf {
public:
    EngineImpl_0() {}
    virtual ~EngineImpl_0() {}

public:
    virtual PluginIntf * CreatePlugin() {
        if (createPluginTrue) {
            return new PluginImpl(moduleName);
        } else {
            return nullptr;
        }
    }

    virtual int ReleasePlugin(PluginIntf * plugin) {
        if (plugin) {
            delete plugin;
            plugin = nullptr;
        }
        return 0;
    }
};

int GetDiskFreeSpaceStub(const char *path, mmDiskSize *diskSize) {
    std::string paths(path);
    try {
        diskSize->availSize = 6*1024*1024;
    } catch(...) {
        return -1;
    }
    return EN_OK;
}

TEST_F(MSPROF_TEST, init_engine) {
    GlobalMockObject::verify();
    static EngineImpl_0 engine_0;

    moduleName = "Framework";
    int ret = Msprof::Engine::Init(moduleName, &engine_0);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    moduleName = "Framework";
    ret = Msprof::Engine::Init(moduleName, &engine_0);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = Msprof::Engine::Init("aaa", &engine_0);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = EngineMgr::instance()->ProfStart("Framework"); //start the module again
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = EngineMgr::instance()->ProfStop("Framework"); //stop the registered module
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_TEST, uninit_engine) {
    GlobalMockObject::verify();
    static EngineImpl_0 engine_0;


    moduleName = "testModule2";
    int ret = Msprof::Engine::Init(moduleName, &engine_0);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = Msprof::Engine::UnInit(moduleName);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = Msprof::Engine::UnInit(moduleName);
    EXPECT_EQ(PROFILING_FAILED, ret);
}


TEST_F(MSPROF_TEST, RegisterEngine) {
    GlobalMockObject::verify();
    static EngineImpl_0 engine_0;

    moduleName = "Framework";
    int ret = Msprof::Engine::RegisterEngine(moduleName, &engine_0);
    EXPECT_EQ(PROFILING_FAILED, ret);

    moduleName = "Framework";
    ret = Msprof::Engine::RegisterEngine(moduleName, &engine_0);
    EXPECT_EQ(PROFILING_FAILED, ret);

    // moduleName = "Framework";
    // ret = Msprof::Engine::RegisterEngine(moduleName, &engine_0);
    // EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = EngineMgr::instance()->ProfStart("Framework"); //start the module again
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = EngineMgr::instance()->ProfStop("Framework"); //stop the registered module
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_TEST, ConfigHandler)
{
    GlobalMockObject::verify();

    auto config = std::make_shared<ProfilerJobConfig>();
    EXPECT_EQ(PROFILING_FAILED, EngineMgr::instance()->ConfigHandler("", config));
    EXPECT_EQ(PROFILING_FAILED, EngineMgr::instance()->ConfigHandler("test", config));
    EngineMgr::instance()->jobs_["test"] = nullptr;
    EXPECT_EQ(PROFILING_FAILED, EngineMgr::instance()->ConfigHandler("test", config));
}

TEST_F(MSPROF_TEST, ProfStartCfgToMsprofCfg) {
    std::string metrics;
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));

    SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature = nullptr;
    feature = std::make_shared<analysis::dvvp::proto::MsProfStartReq>();

    Msprofiler::Api::ProfAclMgr::instance()->ProfStartCfgToMsprofCfg(
        PROF_TASK_TIME_MASK, PROF_AICORE_ARITHMETIC_UTILIZATION, feature);
    EXPECT_EQ(2, feature->hwts_log().size());
}

TEST_F(MSPROF_TEST, MsprofTxMemPool)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofStampPool> stampPool;
    MSVP_MAKE_SHARED0_BREAK(stampPool, MsprofStampPool);

    int ret = stampPool->Init(100);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->Init(100);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    MsprofStampInstance *instance = stampPool->CreateStamp();
    EXPECT_NE(nullptr, instance);

    ret = stampPool->MsprofStampPush(instance);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->MsprofStampPush(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    int id = stampPool->GetIdByStamp(instance);
    EXPECT_EQ(0, id);

    MsprofStampInstance *instanceTmp = stampPool->GetStampById(id);
    EXPECT_EQ(instance, instanceTmp);

    id = stampPool->GetIdByStamp(nullptr);
    EXPECT_EQ(PROFILING_FAILED, id);

    instanceTmp = stampPool->MsprofStampPop();
    EXPECT_EQ(instance, instanceTmp);

    instanceTmp = stampPool->MsprofStampPop();
    EXPECT_EQ(nullptr, instanceTmp);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_FAILED, ret);

    stampPool->DestroyStamp(instance);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

int32_t MsprofReporterCallbackStub(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    return 0;
}

TEST_F(MSPROF_TEST, MsprofTxManager)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);

    int ret = manager->Init();
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->SetCategoryName(1, "test");
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->SetStampCategory(nullptr, 1);
    EXPECT_EQ(PROFILING_FAILED, ret);

    std::string msg = "Test msg";
    ret = manager->SetStampTraceMessage(nullptr, msg.c_str(), msg.size());
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->SetStampPayload(nullptr, 0, nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Mark(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Push(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Pop();
    EXPECT_EQ(PROFILING_FAILED, ret);

    uint32_t rangeId;
    ret = manager->RangeStart(nullptr, &rangeId);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->RangeStop(rangeId);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ACL_PROF_STAMP_PTR stamp = manager->CreateStamp();
    EXPECT_EQ(nullptr, stamp);

    manager->DestroyStamp(stamp);

    manager->SetReporterCallback(MsprofReporterCallbackStub);
    ret = manager->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    stamp = manager->CreateStamp();
    EXPECT_NE(nullptr, stamp);

    ret = manager->SetCategoryName(1, "test");
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->SetStampCategory(stamp, 1);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->SetStampTraceMessage(stamp, msg.c_str(), msg.size());
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->SetStampPayload(stamp, 0, nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Mark(stamp);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->Mark(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Push(stamp);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->Push(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = manager->Pop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->RangeStart(stamp, &rangeId);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = manager->RangeStop(rangeId);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    manager->DestroyStamp(stamp);
    manager->UnInit();
}
class EngineImplFmk : public EngineIntf {
public:
    EngineImplFmk() {}
    virtual ~EngineImplFmk() {}

public:
    PluginIntf * CreatePlugin() {
        return new PluginImpl("runtime");
    }

    int ReleasePlugin(PluginIntf * plugin) {
        if (plugin) {
            delete plugin;
            plugin = nullptr;
        }
        return 0;
    }
};

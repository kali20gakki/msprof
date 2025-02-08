/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: UT for analyzer module
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-07-01
 */
#include <string>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "mmpa_api.h"
#include "errno/error_code.h"
#include "utils/utils.h"

using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
class MmpaUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(MmpaUtest, MmGetCpuInfoWillReturnInvalidParamWhenInputIsInvalid)
{
    int32_t count = 0;
    MmCpuDesc *cpuInfo = nullptr;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCpuInfo(nullptr, &count));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCpuInfo(&cpuInfo, nullptr));
}

TEST_F(MmpaUtest, MmGetCpuInfoWillReturnFailWhenMemSetFail)
{
    int32_t count = 0;
    MmCpuDesc *cpuInfo = nullptr;
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, MmGetCpuInfo(&cpuInfo, &count));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmGetOsName)
{
    std::string name = "";
    int32_t nameSize = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsName(nullptr, nameSize));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsName(CHAR_PTR(name.c_str()), -1));
    MOCKER(gethostname)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    EXPECT_EQ(PROFILING_FAILED, MmGetOsName(CHAR_PTR(name.c_str()), MMPA_MAX_PATH));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetOsName(CHAR_PTR(name.c_str()), MMPA_MAX_PATH));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmGetOsVersion)
{
    std::string versionInfo = "";
    int32_t len = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsVersion(nullptr, len));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsVersion(CHAR_PTR(versionInfo.c_str()), -1));
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1))
        .then(returnValue(EOK));
    MOCKER(uname)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    EXPECT_EQ(PROFILING_FAILED, MmGetOsVersion(CHAR_PTR(versionInfo.c_str()), MMPA_MAX_PATH));
    EXPECT_EQ(PROFILING_FAILED, MmGetOsVersion(CHAR_PTR(versionInfo.c_str()), MMPA_MAX_PATH));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetOsVersion(CHAR_PTR(versionInfo.c_str()), MMPA_MAX_PATH));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmGetTimeOfDay)
{
    MmTimeval tv;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetTimeOfDay(nullptr, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetTimeOfDay(&tv, nullptr));
}

TEST_F(MmpaUtest, MmGetAndFreeMac)
{
    MmMacInfo *macInfo = nullptr;
    int32_t count = 0;
    
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetMacFree(macInfo, count));
    
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetMac(nullptr, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetMac(&macInfo, &count));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetMacFree(macInfo, count));
}


TEST_F(MmpaUtest, MmWaitPid)
{
    MmProcess pid1 = 2;
    int32_t status1 = -1;
    int32_t options = 3;
    // option is invalid
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmWaitPid(pid1, &status1, options));

    // waitpid return PROFILING_FAILED
    int32_t ret1 = 1;
    int32_t ret2 = 2;
    MOCKER_CPP(&waitpid).stubs().will(returnValue(PROFILING_FAILED)).then(returnValue(ret1)).then(returnValue(ret2));
    options = M_WAIT_NOHANG;
    EXPECT_EQ(PROFILING_FAILED, MmWaitPid(pid1, &status1, options));

    // waitpid return success, ret is 1, status1 < 0
    EXPECT_EQ(PROFILING_FAILED, MmWaitPid(pid1, &status1, options));

    // waitpid return success, ret is 2, ret == pid, status2 > 0
    int32_t status2 = 1;
    EXPECT_EQ(PROFILING_ERROR, MmWaitPid(pid1, &status2, options));

    // waitpid return success, ret is 2, ret != pid, status2 > 0
    MmProcess pid2 = 3;
    EXPECT_EQ(PROFILING_SUCCESS, MmWaitPid(pid2, &status2, options));
}

TEST_F(MmpaUtest, MmRmdir)
{
    // 超出递归深度限制
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("mmpaTest", 6)); // 6是递归深度

    uint32_t depth = 0;
    // 指定删除目录文件名为空
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("", depth));

    MOCKER_CPP(&MmIsSoftLink).stubs().will(returnValue(true)).then(returnValue(false));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("testtesttest", depth));

    // 指定删除文件名不存在，opendir拿到空指针
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("mmpaTestjdflsaihhgepaihglehgllhgelahl", depth));

    // 递归删除真实目录
    std::string dirName = Utils::RelativePathToAbsolutePath("mmpaTestTest");
    EXPECT_EQ(Utils::CreateDir(dirName), PROFILING_SUCCESS);
    EXPECT_EQ(Utils::CreateDir(dirName + "/test"), PROFILING_SUCCESS);
    EXPECT_EQ(PROFILING_SUCCESS, MmRmdir(dirName, depth));

    MOCKER_CPP(&MmIsSoftLink).reset();
}

TEST_F(MmpaUtest, MmIsSoftLink)
{
    // 路径名空
    EXPECT_FALSE(MmIsSoftLink(""));
    // memset_s 失败
    MOCKER_CPP(&memset_s).stubs().will(returnValue(1)).then(returnValue(0));
    EXPECT_FALSE(MmIsSoftLink("mmpaTest"));
    // lstat 失败
    MOCKER_CPP(&lstat).stubs().will(returnValue(1)).then(returnValue(0));
    EXPECT_FALSE(MmIsSoftLink("mmpaTest"));

    MOCKER_CPP(&lstat).reset();
    MOCKER_CPP(&memset_s).reset();

    // 新建目录 校验结果预期非软链
    const std::string trueDirName = "./MmIsSoftLink";
    EXPECT_EQ(Utils::CreateDir(trueDirName), PROFILING_SUCCESS);
    EXPECT_FALSE(MmIsSoftLink(trueDirName));
    uint32_t depth = 0;
    EXPECT_EQ(PROFILING_SUCCESS, MmRmdir(trueDirName, depth));

    // 创建假的软链并校验
    const std::string softLinkDirName = "./MmIsSoftLink_soft_link";
    EXPECT_EQ(0, symlink(trueDirName.c_str(), softLinkDirName.c_str()));  // 创建软连接
    EXPECT_TRUE(MmIsSoftLink(softLinkDirName));
    // 利用unlink删除软链
    EXPECT_EQ(0, unlink(softLinkDirName.c_str()));
}

TEST_F(MmpaUtest, MmSocketSendWillReturnInvalidParamWhenInputIsInvalid)
{
    // invalid fd
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmSocketSend(-1, nullptr, 0, 0));
    // nullptr buff
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmSocketSend(0, nullptr, 0, 0));
    // invalid sendLen
    int buf = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmSocketSend(0, static_cast<VOID_PTR>(&buf), 0, 0));
    // invalid sendFlag
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmSocketSend(0, static_cast<VOID_PTR>(&buf), 1, -1));
}

TEST_F(MmpaUtest, MmSocketSendWillReturnFailWhenSendFail)
{
    GlobalMockObject::verify();
    MOCKER(send)
        .stubs()
        .will(returnValue(ssize_t(0)));
    int buf = 0;
    EXPECT_EQ(PROFILING_FAILED, MmSocketSend(0, static_cast<VOID_PTR>(&buf), 1, 0));
}

TEST_F(MmpaUtest, MmSocketSendWillReturnSendSizeWhenSendSucc)
{
    GlobalMockObject::verify();
    MOCKER(send)
        .stubs()
        .will(returnValue(ssize_t(1)));
    int buf = 0;
    EXPECT_EQ(1, MmSocketSend(0, static_cast<VOID_PTR>(&buf), 1, 0));
}

TEST_F(MmpaUtest, MmReadWillReturnInvalidParamWhenInputIsInvalid)
{
    // invalid fd
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRead(-1, nullptr, 0));
    // nullptr buff
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRead(0, nullptr, 0));
}

TEST_F(MmpaUtest, MmReadWillReturnFailWhenReadFail)
{
    GlobalMockObject::verify();
    MOCKER(read)
        .stubs()
        .will(returnValue(ssize_t(-1)));
    int buf = 0;
    EXPECT_EQ(PROFILING_FAILED, MmRead(0, static_cast<VOID_PTR>(&buf), 0));
}

TEST_F(MmpaUtest, MmReadWillReturnReadSizeWhenReadSucc)
{
    GlobalMockObject::verify();
    MOCKER(read)
        .stubs()
        .will(returnValue(ssize_t(1)));
    int buf = 0;
    EXPECT_EQ(1, MmRead(0, static_cast<VOID_PTR>(&buf), 0));
}

TEST_F(MmpaUtest, MmWriteWillReturnInvalidParamWhenInputIsInvalid)
{
    // invalid fd
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmWrite(-1, nullptr, 0));
    // nullptr buff
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmWrite(0, nullptr, 0));
}

TEST_F(MmpaUtest, MmWriteWillReturnFailWhenReadFail)
{
    GlobalMockObject::verify();
    MOCKER(write)
        .stubs()
        .will(returnValue(ssize_t(-1)));
    int buf = 0;
    EXPECT_EQ(PROFILING_FAILED, MmWrite(0, static_cast<VOID_PTR>(&buf), 0));
}

TEST_F(MmpaUtest, MmWriteWillReturnReadSizeWhenReadSucc)
{
    GlobalMockObject::verify();
    MOCKER(write)
        .stubs()
        .will(returnValue(ssize_t(1)));
    int buf = 0;
    EXPECT_EQ(1, MmWrite(0, static_cast<VOID_PTR>(&buf), 0));
}

TEST_F(MmpaUtest, MmOpen2WillReturnInvalidParamWhenInputInvalid)
{
    int32_t flags;
    MmMode_t mode;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmOpen2("", 0, mode));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmOpen2("x", -1, mode));
}

TEST_F(MmpaUtest, MmCloseWillReturnInvalidParamWhenInputInvalid)
{
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmClose(-1));
}

TEST_F(MmpaUtest, MmStatGetWillReturnInvalidParamWhenInputInvalid)
{
    MmStatT buff;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmStatGet("", &buff));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmStatGet("x", nullptr));
}

TEST_F(MmpaUtest, MmGetLocalTimeWillReturnInvalidParamWhenInputInvalids)
{
    GlobalMockObject::verify();
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetLocalTime(nullptr));
}

TEST_F(MmpaUtest, MmGetLocalTimeWillReturnFailWhenRunFail)
{
    GlobalMockObject::verify();
    struct tm fakeTime;
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1))
        .then(returnValue(EOK));
    MOCKER(gettimeofday)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER(localtime_r)
        .stubs()
        .will(returnValue(static_cast<struct tm *>(nullptr)))
        .then(returnValue(&fakeTime));
    MmSystemTimeT time;
    EXPECT_EQ(PROFILING_FAILED, MmGetLocalTime(&time));
    EXPECT_EQ(PROFILING_FAILED, MmGetLocalTime(&time));
    EXPECT_EQ(PROFILING_FAILED, MmGetLocalTime(&time));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetLocalTime(&time));
}

TEST_F(MmpaUtest, MmGetCwdWillReturnInvalidParamWhenInputInvalids)
{
    GlobalMockObject::verify();
    char buff;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCwd(nullptr, 0));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCwd(&buff, -1));
}

TEST_F(MmpaUtest, MmGetCwdWillReturnFailWhenGetcwdFail)
{
    GlobalMockObject::verify();
    char buff;
    std::string fakeCwd = "x";
    MOCKER(getcwd)
        .stubs()
        .will(returnValue((CHAR_PTR)nullptr))
        .then(returnValue((CHAR_PTR)fakeCwd.c_str()));
    EXPECT_EQ(PROFILING_FAILED, MmGetCwd(&buff, 0));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetCwd(&buff, 0));
}

TEST_F(MmpaUtest, MmGetEnvWillReturnInvalidParamWhenInputInvalids)
{
    GlobalMockObject::verify();
    std::string name = "x";
    char value;
    uint32_t len = 1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetEnv("", &value, len));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetEnv(name, nullptr, len));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetEnv(name, &value, 0));
}

VOID_PTR fakeFunc(VOID_PTR pulArg)
{
    static int a = 1;
    return static_cast<VOID_PTR>(&a);
}

TEST_F(MmpaUtest, MmCreateTaskWithThreadAttrWillReturnInvalidParamWhenInputIsInvalid)
{
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    MmThreadAttr attr;
    // nullptr thread handle
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(nullptr, &funcBlock, &attr));
    // nullptr func block
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, nullptr, &attr));
    // nullptr proc func
    funcBlock.procFunc = nullptr;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    // nullptr attr
    funcBlock.procFunc = fakeFunc;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, nullptr));
}

TEST_F(MmpaUtest, MmCreateTaskWithThreadAttrWillReturnFailWhenMemSetFail)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    MmThreadAttr attr;
    funcBlock.procFunc = fakeFunc;
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, MmCreateTaskWithThreadAttrWillReturnFailWhenPthreadInitFail)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    MmThreadAttr attr;
    funcBlock.procFunc = fakeFunc;
    MOCKER(pthread_attr_init)
        .stubs()
        .will(returnValue(-1));
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, MmCreateTaskWithThreadAttrWillReturnFailWhenSetThreadAttrFail)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    // invalid attr, so that LocalSetThreadAttr return fail
    attr.stackFlag = TRUE;
    attr.stackSize = 0;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, MmCreateTaskWithThreadAttrWillReturnSuccWhenSetThreadAttrSucc)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    EXPECT_EQ(PROFILING_SUCCESS, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    MmJoinTask(&threadHandle);
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnFailWhenInputAttrWithInvalidPolicyValue)
{
    // use MmCreateTaskWithThreadAttr to set thread attr
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(-1));
    attr.policyFlag = TRUE;
    attr.priorityFlag = FALSE;
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    attr.policyFlag = FALSE;
    attr.priorityFlag = TRUE;
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    attr.policyFlag = TRUE;
    attr.priorityFlag = TRUE;
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));

    GlobalMockObject::verify();
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    attr.policy = -1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    attr.policy = MMPA_THREAD_SCHED_FIFO;
    MOCKER(pthread_attr_setschedpolicy)
        .stubs()
        .will(returnValue(-1));
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));

    GlobalMockObject::verify();
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setschedpolicy)
        .stubs()
        .will(returnValue(0));
    attr.priorityFlag  = FALSE;
    EXPECT_EQ(PROFILING_SUCCESS, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnValueWhenInputAttrWithInvalidPriorityValueOrRunFailOrSucc)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    attr.policyFlag = FALSE;
    attr.priorityFlag = TRUE;
    attr.priority = MMPA_MIN_THREAD_PIO - 1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    attr.priority = MMPA_MAX_THREAD_PIO + 1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    attr.priority = MMPA_MIN_THREAD_PIO;
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1))
        .then(returnValue(EOK));
    MOCKER(pthread_attr_setschedparam)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
    EXPECT_EQ(PROFILING_SUCCESS, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnInvalidParamWhenInputAttrWithInvalidStackValue)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    attr.policyFlag = FALSE;
    attr.priorityFlag = FALSE;
    attr.stackFlag = TRUE;
    attr.stackSize = MMPA_THREAD_MIN_STACK_SIZE - 1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnFailWhenSetStackSizeFail)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setstacksize)
        .stubs()
        .will(returnValue(-1));
    attr.policyFlag = FALSE;
    attr.priorityFlag = FALSE;
    attr.stackFlag = TRUE;
    attr.stackSize = MMPA_THREAD_MIN_STACK_SIZE;
    attr.detachFlag = FALSE;
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnFailWhenSetDetachStateFail)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setstacksize)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setdetachstate)
        .stubs()
        .will(returnValue(-1));
    attr.policyFlag = FALSE;
    attr.priorityFlag = FALSE;
    attr.stackFlag = TRUE;
    attr.stackSize = MMPA_THREAD_MIN_STACK_SIZE;
    attr.detachFlag = TRUE;
    EXPECT_EQ(PROFILING_FAILED, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, SetThreadAttrWillReturnSuccWhenInputValidAttr)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MOCKER(pthread_attr_destroy)
        .stubs()
        .will(returnValue(0));
    MOCKER(pthread_attr_setinheritsched)
        .stubs()
        .will(returnValue(0));
    attr.policyFlag = FALSE;
    attr.priorityFlag = FALSE;
    attr.stackFlag = FALSE;
    attr.detachFlag = FALSE;
    EXPECT_EQ(PROFILING_SUCCESS, MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr));
}

TEST_F(MmpaUtest, MmJoinTaskWillReturnInvalidParamWhenInputNull)
{
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmJoinTask(nullptr));
}

TEST_F(MmpaUtest, MmJoinTaskWillReturnFailWhenPthreadJoinFail)
{
    GlobalMockObject::verify();
    MOCKER(pthread_join)
        .stubs()
        .will(returnValue(-1));
    MmThread threadHandle;
    EXPECT_EQ(PROFILING_FAILED, MmJoinTask(&threadHandle));
}

TEST_F(MmpaUtest, MmJoinTaskWillReturnSuccWhenPthreadJoinSucc)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr);
    EXPECT_EQ(PROFILING_SUCCESS, MmJoinTask(&threadHandle));
}

TEST_F(MmpaUtest, MmSetCurrentThreadNameWillReturnInvalidParamWhenInputEmptyName)
{
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmSetCurrentThreadName(""));
}

TEST_F(MmpaUtest, MmSetCurrentThreadNameWillReturnSuccWhenprctlSucc)
{
    GlobalMockObject::verify();
    MmThread threadHandle;
    MmUserBlockT funcBlock;
    funcBlock.procFunc = fakeFunc;
    MmThreadAttr attr;
    MmCreateTaskWithThreadAttr(&threadHandle, &funcBlock, &attr);
    EXPECT_EQ(PROFILING_SUCCESS, MmSetCurrentThreadName("xx"));
    MmJoinTask(&threadHandle);
}

TEST_F(MmpaUtest, MmGetTickCountWillReturnFailWhenMemSetFail)
{
    GlobalMockObject::verify();
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    MmTimespec rts;
    EXPECT_EQ(PROFILING_FAILED, MmGetTickCount(rts));
}

TEST_F(MmpaUtest, MmGetFileSizeWillReturnInvalidParamWhenInputInvalid)
{
    unsigned long long length;
    std::string fileName = "x";
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetFileSize("", &length));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetFileSize(fileName, nullptr));
}

TEST_F(MmpaUtest, MmGetFileSizeWillReturnFailWhenMemSetFail)
{
    unsigned long long length;
    std::string fileName = "x";
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, MmGetFileSize(fileName, &length));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmGetFileSizeWillReturnFailWhenLstatFail)
{
    unsigned long long length;
    std::string fileName = "x";
    MOCKER(lstat)
        .stubs()
        .will(returnValue(-1));
    EXPECT_EQ(PROFILING_FAILED, MmGetFileSize(fileName, &length));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmGetFileSizeWillReturnSuccWhenLstatSucc)
{
    unsigned long long length;
    std::string fileName = "x";
    MOCKER(lstat)
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetFileSize(fileName, &length));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmIsDirWillReturnInvalidParamWhenInputInvalid)
{
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmIsDir(""));
}

TEST_F(MmpaUtest, MmIsDirWillReturnFailWhenMemSetFail)
{
    std::string fileName = "x";
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, MmIsDir(fileName));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmIsDirWillReturnFailWhenLstatFail)
{
    std::string fileName = "x";
    MOCKER(lstat)
        .stubs()
        .will(returnValue(-1));
    EXPECT_EQ(PROFILING_FAILED, MmIsDir(fileName));
    GlobalMockObject::verify();
}

int32_t LstatReturnFileIsDirStub(CONST_CHAR_PTR pathName, struct stat *buf)
{
    buf->st_mode = S_IFDIR;
    return 0;
}

int32_t LstatReturnFileIsNotDirStub(CONST_CHAR_PTR pathName, struct stat *buf)
{
    buf->st_mode = S_IFREG;
    return 0;
}

TEST_F(MmpaUtest, MmIsDirWillReturnFailWhenInputNotDir)
{
    std::string fileName = "x";
    MOCKER(lstat)
        .stubs()
        .will(invoke(LstatReturnFileIsNotDirStub));
    EXPECT_EQ(PROFILING_FAILED, MmIsDir(fileName));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmIsDirWillReturnSuccWhenInputDir)
{
    std::string fileName = "x";
    MOCKER(lstat)
        .stubs()
        .will(invoke(LstatReturnFileIsDirStub));
    EXPECT_EQ(PROFILING_SUCCESS, MmIsDir(fileName));
    GlobalMockObject::verify();
}

TEST_F(MmpaUtest, MmAccess2WillReturnInvalidParamWhenInputInvalid)
{
    int32_t mode;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmAccess2("", mode));
}

TEST_F(MmpaUtest, MmDirNameWillReturnNullWhenInputInvalid)
{
    EXPECT_EQ(nullptr, MmDirName(nullptr));
}

TEST_F(MmpaUtest, MmBaseNameWillReturnNullWhenInputInvalid)
{
    EXPECT_EQ(nullptr, MmBaseName(nullptr));
}

TEST_F(MmpaUtest, MmMkdirWillReturnInvalidParamWhenInputInvalid)
{
    MmMode_t mode;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmMkdir("", mode));
}

TEST_F(MmpaUtest, MmChmodWillReturnInvalidParamWhenInputInvalid)
{
    int32_t mode;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmChmod("", mode));
}

TEST_F(MmpaUtest, MmGetErrorFormatMessageWillReturnInvalidParamWhenInputInvalid)
{
    MmErrorMsg msg;
    char buf;
    EXPECT_EQ(nullptr, MmGetErrorFormatMessage(msg, nullptr, 1));
    EXPECT_EQ(nullptr, MmGetErrorFormatMessage(msg, &buf, 0));
}

TEST_F(MmpaUtest, MmScandirWillReturnInvalidParamWhenInputInvalid)
{
    MmDirent entryList;
    MmDirent *entryListPtr = &entryList;
    MmDirent **entryList2Ptr = &entryListPtr;
    MmFilter filterFunc;
    MmSort sort;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmScandir("", &entryList2Ptr, filterFunc, sort));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmScandir("path", nullptr, filterFunc, sort));
}

TEST_F(MmpaUtest, MmScandirFreeWillReturnWhenInputNull)
{
    int32_t cnt = 0;
    MmScandirFree(nullptr, cnt);
    EXPECT_EQ(0, cnt);
}

TEST_F(MmpaUtest, MmRealPathWillReturnInvalidParamWhenInputInvalid)
{
    char path;
    char realPath;
    int32_t realPathLen = MMPA_MAX_PATH - 1;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRealPath(nullptr, &realPath, realPathLen));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRealPath(&path, nullptr, realPathLen));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRealPath(&path, &realPath, realPathLen));
}

TEST_F(MmpaUtest, MmChdirWillReturnInvalidParamWhenInputInvalid)
{
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmChdir(""));
}

TEST_F(MmpaUtest, MmCreateProcessWillReturnInvalidParamWhenInputInvalid)
{
    std::string fileName = "x";
    MmArgvEnv env;
    std::string stdoutRedirectFile;
    MmProcess id;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateProcess(fileName, &env, stdoutRedirectFile, nullptr));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCreateProcess("", &env, stdoutRedirectFile, &id));
}

TEST_F(MmpaUtest, MmCreateProcessWillReturnFailWhenForkFail)
{
    std::string fileName = "x";
    MmArgvEnv env;
    std::string stdoutRedirectFile;
    MmProcess id;
    MOCKER(fork)
        .stubs()
        .will(returnValue(-1));
    EXPECT_EQ(PROFILING_FAILED, MmCreateProcess(fileName, &env, stdoutRedirectFile, &id));
    GlobalMockObject::verify();
}

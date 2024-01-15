/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kernel_parser_worker_utest.cpp
 * Description        : KernelParserWorker UT
 * Author             : msprof team
 * Creation Date      : 2023/11/23
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/worker/kernel_parser_worker.h"
#include "analysis/csrc/viewer/database/drafts/hash_db_dumper.h"
#include "analysis/csrc/viewer/database/drafts/type_info_db_dumper.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/worker/host_trace_worker.h"


using namespace Analysis::Worker;

const std::string TEST_HOST_FILE_PATH = ".";
using HashData = Analysis::Parser::Host::Cann::HashData;
using TypeData = Analysis::Parser::Host::Cann::TypeData;
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using DBRunner = Analysis::Viewer::Database::DBRunner;
using namespace Analysis::Utils;

class KernelParserWorkerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_HOST_FILE_PATH);
        File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "data"}));
        File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "data", "sqlite"}));
        TypeInfoData typeInfoData{{1, {{1, "123"}}}};
        std::unordered_map<uint64_t, std::string> hashData{{0, "a"},
                                                           {1, "b"}};
        MOCKER_CPP(&HashData::GetAll).stubs().will(returnValue(hashData));
        MOCKER_CPP(&TypeData::GetAll).stubs().will(returnValue(typeInfoData));
        MOCKER_CPP(&HostTraceWorker::Run).stubs().will(returnValue(true));
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(File::PathJoin({TEST_HOST_FILE_PATH, "data"}), 0);
    }
};

TEST_F(KernelParserWorkerUtest, TestKernalParserWorkerShouldReturnSuccessWhenAllFunctionWell)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 0);
}
TEST_F(KernelParserWorkerUtest, TestKernalParserWorkerShouldReturnFailedWhenHostTraceWorkerReturnFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&HostTraceWorker::Run).reset();
    MOCKER_CPP(&HostTraceWorker::Run).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernalParserWorkerShouldReturnFailedWhenDumpHashFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernalParserWorkerShouldReturnFailedWhenTypeInfoDumpFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}
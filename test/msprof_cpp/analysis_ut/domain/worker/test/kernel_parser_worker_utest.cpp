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

#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/type_info_db_dumper.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"


using namespace Analysis::Domain;

const std::string TEST_HOST_FILE_PATH = ".";
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using DBRunner = Analysis::Infra::DBRunner;
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

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnSuccessWhenAllFunctionWell)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 0);
}
TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenHostTraceWorkerReturnFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&HostTraceWorker::Run).reset();
    MOCKER_CPP(&HostTraceWorker::Run).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenDumpHashFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenTypeInfoDumpFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnErrorWhenContextLoadFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::Load).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}
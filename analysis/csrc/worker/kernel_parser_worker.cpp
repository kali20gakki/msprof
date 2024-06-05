/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kernel_parser_worker.cpp
 * Description        : KernelParserWorker类，启动解析任务并跟踪任务结果
 * Author             : msprof team
 * Creation Date      : 2023-11-03
 * *****************************************************************************
 */

#include "analysis/csrc/worker/kernel_parser_worker.h"

#include <utility>
#include "analysis/csrc/worker/host_trace_worker.h"
#include "analysis/csrc/viewer/database/drafts/hash_db_dumper.h"
#include "analysis/csrc/viewer/database/drafts/type_info_db_dumper.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/parser/host/cann/type_data.h"

namespace Analysis {
namespace Worker {
using HostTraceWorker = Analysis::Worker::HostTraceWorker;
using ThreadPool = Analysis::Utils::ThreadPool;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Parser::Host::Cann;
using namespace Analysis::Viewer::Database::Drafts;
// 解析流程控制类，传入host data所在路径，启动采集流程并返回结果
KernelParserWorker::KernelParserWorker(std::string hostFilePath) : hostFilePath_(std::move(hostFilePath)),
                                                                   result_(true) {
}

int KernelParserWorker::Run()
{
    Log::GetInstance().Init(Utils::File::PathJoin({hostFilePath_, "..", "mindstudio_profiler_log"}));
    std::set<std::string> profPaths {Utils::File::PathJoin({hostFilePath_, ".."})};
    Context::GetInstance().Load(profPaths);
    INFO("Start run KernelParserWorker");
    // 先创建目录
    std::string sqlBaseDir = Utils::File::PathJoin({hostFilePath_, "sqlite"});
    if (!Utils::File::CreateDir(sqlBaseDir)) {
        INFO("Create path failed");
    }
    // 启动线程
    const uint16_t taskNumber = 3;
    ThreadPool pool(taskNumber);
    pool.Start();
    pool.AddTask([this]() {
        INFO("Start parse hash data");
        DumpHashData();
    });
    pool.AddTask([this]() {
        INFO("Start parse type info data");
        DumpTypeInfoData();
    });
    pool.WaitAllTasks();
    pool.Stop();
    LaunchTraceParser();
    if (!result_) {
        ERROR("Parse failed or dump failed");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

void KernelParserWorker::DumpHashData()
{
    auto dataPath = Utils::File::PathJoin({hostFilePath_, "data"});
    INFO("Hash data load from path: %", dataPath);
    HashData::GetInstance().Load(dataPath);
    auto hashDataContent = HashData::GetInstance().GetAll();
    INFO("success get hash data");
    if (hashDataContent.empty()) {
        WARN("Empty hash data");
        return;
    }
    std::shared_ptr<HashDBDumper> hashDbDumper;
    MAKE_SHARED_RETURN_VOID(hashDbDumper, HashDBDumper, hostFilePath_);
    INFO("success get hashDbDumper");
    auto dumpresult = hashDbDumper->DumpData(hashDataContent);
    if (!dumpresult) {
        ERROR("Hash data parse failed");
        result_ = false;
    }
}

void KernelParserWorker::DumpTypeInfoData()
{
    auto dataPath = Utils::File::PathJoin({hostFilePath_, "data"});
    INFO("Typeinfo data load from path: %", dataPath);
    TypeData::GetInstance().Load(dataPath);
    auto typeInfoContent = TypeData::GetInstance().GetAll();
    if (typeInfoContent.empty()) {
        WARN("Empty type info data");
        return;
    }
    TypeInfoDBDumper typeDbDumper(hostFilePath_);
    auto dumpresult = typeDbDumper.DumpData(typeInfoContent);
    if (!dumpresult) {
        ERROR("Type info parse failed");
        result_ = false;
    }
}

void KernelParserWorker::LaunchTraceParser()
{
    HostTraceWorker hostTraceWorker{hostFilePath_};
    if (!hostTraceWorker.Run()) {
        ERROR("Host trace parse failed");
        result_ = false;
    }
}
} // Worker
} // Analysis
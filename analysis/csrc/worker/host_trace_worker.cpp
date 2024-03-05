/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_trace_worker.cpp
 * Description        : HostTraceWorker模块：
 *                      负责拉起数据解析->EventQueue->建树->分析树->Dump 流程
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "analysis/csrc/worker/host_trace_worker.h"
#include "analysis/csrc/utils/time_logger.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/parser/host/cann/event_grouper.h"
#include "analysis/csrc/association/cann/tree_builder.h"
#include "analysis/csrc/association/cann/tree_analyzer.h"
#include "analysis/csrc/viewer/database/drafts/cann_trace_db_dumper.h"
#include "analysis/csrc/viewer/database/drafts/api_event_db_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Host::Cann;
using namespace Analysis::Association::Cann;
using namespace Analysis::Viewer::Database::Drafts;

namespace Analysis {
namespace Worker {

bool HostTraceWorker::Run()
{
    TimeLogger t{"HostTraceWorker"};
    // 1. 解析生成CANNWarehouse
    auto hostDataPath = Utils::File::PathJoin({hostPath_, "data"});
    std::shared_ptr<EventGrouper> grouper;
    MAKE_SHARED_RETURN_VALUE(grouper, EventGrouper, false, hostDataPath);
    grouper->Group();

    cannWarehouses_ = grouper->GetGroupEvents();
    if (cannWarehouses_.Empty()) {
        WARN("Group events result is empty, stop the rest of the process");
        return false;
    }
    threadIds_ = grouper->GetThreadIdSet();
    bool ret = true;
    ThreadPool pool(poolSize_);
    pool.Start();
    pool.AddTask([this, &grouper, &ret]() {
        TimeLogger t{"Dump api data start"};
        // api event 数据落盘
        auto apiTraces = grouper->GetApiTraces();
        std::shared_ptr<ApiEventDBDumper> apiDumper;
        MAKE_SHARED_RETURN_VALUE(apiDumper, ApiEventDBDumper, false, hostPath_);
        ret = apiDumper->DumpData(apiTraces);
        if (!ret) {
            ERROR("Dump api traces data failed");
        }
    });
    pool.AddTask([this]() {
        // 建树
        // 建树前需要先对KernelEvents排序
        SortEvents();
        MultiThreadBuildTree();
        // 分析树 & DB Dump
        MultiThreadAnalyzeTreeDumpData();
    });
    pool.WaitAllTasks();
    pool.Stop();
    return ret;
}

void HostTraceWorker::SortEvents()
{
    for (auto tid: threadIds_) {
        if (cannWarehouses_[tid].kernelEvents != nullptr) {
            cannWarehouses_[tid].kernelEvents->Sort();
        }
        if (cannWarehouses_[tid].fusionOpInfoEvents != nullptr) {
            cannWarehouses_[tid].fusionOpInfoEvents->Sort();
        }
        if (cannWarehouses_[tid].nodeBasicInfoEvents != nullptr) {
            cannWarehouses_[tid].nodeBasicInfoEvents->Sort();
        }
        if (cannWarehouses_[tid].tensorInfoEvents != nullptr) {
            cannWarehouses_[tid].tensorInfoEvents->Sort();
        }
        if (cannWarehouses_[tid].contextIdEvents != nullptr) {
            cannWarehouses_[tid].contextIdEvents->Sort();
        }
        if (cannWarehouses_[tid].hcclInfoEvents != nullptr) {
            cannWarehouses_[tid].hcclInfoEvents->Sort();
        }
        if (cannWarehouses_[tid].hcclInfoEvents != nullptr) {
            cannWarehouses_[tid].hcclInfoEvents->Sort();
        }
    }
}

void HostTraceWorker::MultiThreadBuildTree()
{
    TimeLogger t{"Multi thread build tree"};
    ThreadPool pool(poolSize_);
    pool.Start();
    for (auto tid: threadIds_) {
        pool.AddTask([this, tid]() {
            INFO("Start multi thread build tree, threadId = %", tid);
            std::shared_ptr<CANNWarehouse> cannWareHouse;
            MAKE_SHARED_RETURN_VOID(cannWareHouse, CANNWarehouse, cannWarehouses_[tid]);
            std::shared_ptr<TreeBuilder> treeBuilder;
            MAKE_SHARED_RETURN_VOID(treeBuilder, TreeBuilder, cannWareHouse, tid);
            auto treeNode = treeBuilder->Build();
            if (treeNode) {
                // 保存建树完成的根节点
                std::lock_guard<std::mutex> lock(mutex_);
                treeNodes_.emplace_back(tid, treeNode);
                INFO("Multi thread build tree done, threadId = %", tid);
            }
        });
    }

    pool.WaitAllTasks();
    pool.Stop();
}

void HostTraceWorker::MultiThreadAnalyzeTreeDumpData()
{
    TimeLogger t{"Multi thread analyze tree and dump data"};
    ThreadPool pool(poolSize_);
    pool.Start();
    for (auto &p: treeNodes_) {
        pool.AddTask([this, p]() {
            INFO("Start analyze tree and dump data, threadId = %", p.first);
            // 分析
            TreeAnalyzer ana{p.second, p.first};
            ana.Analyze();
            // 落盘
            std::shared_ptr<CANNTraceDBDumper> dumper;
            MAKE_SHARED_RETURN_VOID(dumper, CANNTraceDBDumper, hostPath_);
            if (dumper->DumpData(ana)) {
                INFO("Dump cann trace data done, threadId = %", p.first);
            } else {
                ERROR("Dump cann trace data failed, threadId = %", p.first);
            }
        });
    }

    pool.WaitAllTasks();
    pool.Stop();
}
} // namespace Worker
} // namespace Analysis
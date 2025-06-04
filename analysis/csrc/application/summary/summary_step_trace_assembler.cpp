/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_step_trace_assembler.h
 * Description        : 组合step trace数据
 * Author             : msprof team
 * Creation Date      : 2025/5/27
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/summary_step_trace_assembler.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;

namespace {
const std::string HEADER_REDUCE_START = "Reduce Start(us)";
const std::string HEADER_REDUCE_DURATION = "Reduce Duration(us)";
}

void SummaryStepTraceAssembler::AddAllReduceHeaders()
{
    if (!formatedAllReduceData_.empty()) {
        for (auto &allReducePair : formatedAllReduceData_) {
            uint64_t allReduceGroupSize = allReducePair.second.size();
            allReduceGroupMaxSize_ = allReduceGroupSize > allReduceGroupMaxSize_ ?
                    allReduceGroupSize : allReduceGroupMaxSize_;
        }
    }

    for (uint64_t i = 0; i < allReduceGroupMaxSize_; ++i) {
        headers_.emplace_back(HEADER_REDUCE_START);
        headers_.emplace_back(HEADER_REDUCE_DURATION);
    }
}

SummaryStepTraceAssembler::SummaryStepTraceAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "Iteration ID", "FP Start(us)", "BP End(us)", "Iteration End(us)",
                "Iteration Time(us)", "FP to BP Time(us)", "Iteration Refresh(us)",
                "Data Aug Bound(us)", "Model ID"};
}

uint8_t SummaryStepTraceAssembler::AssembleData(DataInventory &dataInventory)
{
    auto trainTraceData = dataInventory.GetPtr<std::vector<TrainTraceData>>();
    auto allReduceData = dataInventory.GetPtr<std::vector<AllReduceData>>();

    if (trainTraceData == nullptr) {
        WARN("trainTraceData not exists, can't export step trace data.");
        return DATA_NOT_EXIST;
    }

    if (allReduceData == nullptr) {
        WARN("No all reduce data collected, maybe the all_reduce table is not created, "
             "now try to export data with no all reduce");
    } else {
        FormatAllReduceData(*allReduceData);
    }

    // get final headers
    AddAllReduceHeaders();
    // assemble trace and all reduce data
    AssembleStepTraceData(*trainTraceData);
    
    if (res_.empty()) {
        ERROR("Can't match any step trace data, failed to generate step_trace_*.csv");
        return ASSEMBLE_FAILED;
    }

    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, STEP_TRACE_SUMMARY_NAME}), {});

    return ASSEMBLE_SUCCESS;
}

void SummaryStepTraceAssembler::FormatAllReduceData(const std::vector<AllReduceData> &allReduceData)
{
    if (allReduceData.empty()) {
        WARN("all reduce data is empty, no all reduce data for step trace, check table all_reduce please.");
        return;
    }
    for (auto &allReduceDatum : allReduceData) {
        TraceId traceId = {allReduceDatum.modelId, allReduceDatum.iterEnd};
        auto it = formatedAllReduceData_.find(traceId);
        if (it != formatedAllReduceData_.end()) {
            it->second.emplace_back(DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp),
                                    DivideByPowersOfTenWithPrecision(allReduceDatum.end - allReduceDatum.timestamp));
        } else {
            formatedAllReduceData_[traceId] = {{DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp),
            DivideByPowersOfTenWithPrecision(allReduceDatum.timestamp - allReduceDatum.end)}};
        }
    }
}

void SummaryStepTraceAssembler::AssembleStepTraceData(const std::vector<TrainTraceData> &trainTraceData)
{
    if (trainTraceData.empty()) {
        WARN("train trace data is empty, no train trace data for step trace, check table training_trace please.");
        return;
    }

    const std::string DIVIDE_CHAR = "\t";
    for (auto &trainTraceDatum : trainTraceData) {
        TraceId traceId = {trainTraceDatum.modelId, trainTraceDatum.iterEnd};
        std::vector<std::string> row = {std::to_string(trainTraceDatum.deviceId),
                                        std::to_string(trainTraceDatum.indexId),
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.fpStart) + DIVIDE_CHAR,
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.bpEnd) + DIVIDE_CHAR,
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.iterEnd) + DIVIDE_CHAR,
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.iterTime) + DIVIDE_CHAR,
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.fpBpTime),
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.gradRefreshBound),
                                        DivideByPowersOfTenWithPrecision(trainTraceDatum.dataAugBound),
                                        std::to_string(trainTraceDatum.modelId)};
        auto it = formatedAllReduceData_.find(traceId);
        if (it != formatedAllReduceData_.end()) {
            int count = 0;
            for (auto &allReduceData : it->second) {
                row.emplace_back(allReduceData.first + DIVIDE_CHAR);
                row.emplace_back(allReduceData.second + DIVIDE_CHAR);
                ++count;
            }
            row.insert(row.end(), allReduceGroupMaxSize_ - count, NA);
        } else {
            row.insert(row.end(), allReduceGroupMaxSize_, NA);
        }
        res_.emplace_back(row);
    }
}


} // Application
} // Analysis
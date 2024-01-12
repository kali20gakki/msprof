/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_db_dumper.h
 * Description        : 数据库模块对外接口
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H

#include <utility>
#include <vector>
#include <thread>
#include <mutex>

#include "analysis/csrc/association/cann/tree_analyzer.h"
#include "analysis/csrc/entities/tree.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/viewer/database/db_runner.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {

// 供HostTraceWorker调用， 传入Analyzer对象，将所有数据落盘
class CANNTraceDBDumper {
using TreeAnalyzer = Analysis::Association::Cann::TreeAnalyzer;
using HostTask = Analysis::Entities::HostTask;
using OpDesc = Analysis::Entities::OpDesc;
using HostTasks = std::vector<std::shared_ptr<HostTask>>;
using TaskInfoData = std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string,
        std::string, std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string, std::string, std::string,
        std::string, std::string, std::string, uint32_t, uint32_t, std::string>>;
using HCCLBigOpDescs = Analysis::Association::Cann::HCCLBigOpDescs;
public:
    // 创建时传入host路径
    explicit CANNTraceDBDumper(std::string hostFilePath);

    // 提供DumpData方法，调用后执行落盘操作，成功返回True，失败返回False。
    bool DumpData(TreeAnalyzer analyzer);

private:
    // 落盘HCCLOP
    void DumpHcclOps(const HCCLBigOpDescs &hcclOps);

    // 落盘HostTask
    void DumpHostTasks(const HostTasks &hostTasks);

    // 落盘TaskInfo
    void DumpOpDesc(const HostTasks &computeTasks);

    // 落盘HCCLTask
    void DumpHcclTasks(const HostTasks &hcclTasks);

    void AddTaskInfo(const std::shared_ptr<HostTask> &task, TaskInfoData &data);

    void AddTensorShapeInfo(const std::shared_ptr<ConcatTensorInfo> &tensorDesc, MsprofNodeBasicInfo nodeBasicInfo,
                            TaskInfoData &data, const std::shared_ptr<HostTask> &task);

    std::string hostFilePath_;
    bool result_;
};
} // Drafts
} // Database
} // Viewer
} // Analysis


#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_CANN_DB_DUMPER_H

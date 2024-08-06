/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_data_handle.h
 * Description        : Common definition of mstx data handle class.
 * Author             : msprof team
 * Creation Date      : 2024/08/02
 * *****************************************************************************
*/
#ifndef MSTX_DATA_HANDLER_H
#define MSTX_DATA_HANDLER_H

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "msprof_error_manager.h"
#include "singleton/singleton.h"
#include "thread/thread.h"
#include "queue/ring_buffer.h"
#include "msprof_tx_manager.h"

namespace Collector {
namespace Dvvp {
namespace Mstx {
using namespace Msprof::MsprofTx;

constexpr size_t RING_BUFFER_DEFAULT_CAPACITY = 512;

enum class MstxDataType {
    DATA_MARK = 0,
    DATA_RANGE_START,
    DATA_RANGE_END,
    DATA_INVALID
};

class MstxDataHandler : public analysis::dvvp::common::singleton::Singleton<MstxDataHandler>,
                        public analysis::dvvp::common::thread::Thread {
public:
    MstxDataHandler();
    ~MstxDataHandler();

    int Start();
    int Stop();
    void Run(const struct error_message::Context &errorContext) override;
    bool IsStart();
    int SaveMstxData(const char* msg, uint64_t mstxEventId, MstxDataType type);

private:
    void Init();
    void Uninit();
    int SaveMarkData(const char* msg, uint64_t mstxEventId);
    int SaveRangeDate(const char* msg, uint64_t mstxEventId, MstxDataType type);

    void Flush();
    void ReportData();

private:
    uint32_t processId_{0};
    std::atomic<bool> init_{false};
    std::atomic<bool> start_{false};
    analysis::dvvp::common::queue::RingBuffer<MsprofTxExInfo> mstxDataBuf_;
    std::mutex tmpRangeDataMutex_;
    std::unordered_map<uint64_t, MsprofTxExInfo> tmpMstxRangeData_;
};
}
}
}
#endif

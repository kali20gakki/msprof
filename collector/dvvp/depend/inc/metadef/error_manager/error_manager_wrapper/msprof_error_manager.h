#ifndef MSPROF_ERROR_MANAGER_H_
#define MSPROF_ERROR_MANAGER_H_

#include "metadef/error_manager/error_manager.h"

#define MSPROF_INPUT_ERROR REPORT_INPUT_ERROR
#define MSPROF_ENV_ERROR REPORT_ENV_ERROR
#define MSPROF_INNER_ERROR REPORT_INNER_ERROR
#define MSPROF_CALL_ERROR REPORT_CALL_ERROR

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {

class MsprofErrorManager {
public:
    static MsprofErrorManager &GetInstance();
    error_message::Context &GetErrorManagerContext();
    void SetErrorContext(const error_message::Context errorContext);
private:

    MsprofErrorManager() {}
    ~MsprofErrorManager() {}

    MsprofErrorManager(const MsprofErrorManager &) = delete;
    MsprofErrorManager(MsprofErrorManager &&) = delete;
    MsprofErrorManager &operator=(const MsprofErrorManager &) = delete;
    MsprofErrorManager &operator=(MsprofErrorManager &&) = delete;
    static error_message::Context errorContext_;
};

}  // ErrorManager
}  // Dvvp
}  // namespace Analysis
#endif
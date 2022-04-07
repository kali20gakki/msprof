#ifndef MSPROF_ERROR_MANAGER_STUB_H_
#define MSPROF_ERROR_MANAGER_STUB_H_

#include "metadef/error_manager/error_manager.h"

#define MSPROF_INPUT_ERROR(error_code, key, value)
#define MSPROF_ENV_ERROR(error_code, key, value)
#define MSPROF_INNER_ERROR(error_code, fmt, ...)
#define MSPROF_CALL_ERROR MSPROF_INNER_ERROR

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
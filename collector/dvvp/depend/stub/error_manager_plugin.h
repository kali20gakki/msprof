#ifndef ERROR_MANAGER_PLUGIN_STUB_H
#define ERROR_MANAGER_PLUGIN_STUB_H

#include "common/util/error_manager/error_manager.h"
#include "common/singleton/singleton.h"

#define MSPROF_INPUT_ERROR(error_code, key, value)
#define MSPROF_ENV_ERROR(error_code, key, value)
#define MSPROF_INNER_ERROR(error_code, fmt, ...)
#define MSPROF_CALL_ERROR MSPROF_INNER_ERROR

namespace Analysis {
namespace Dvvp {
namespace Plugin {

class ErrMgrPlugin : public analysis::dvvp::common::singleton::Singleton<MsprofErrorManager> {
public:
    void SetErrorContext(const error_message::Context errorContext) const;
    error_message::Context &GetErrorManagerContext() const;
    ErrMgrPlugin() {}
    ~ErrMgrPlugin() override {}
private:
    static error_message::Context errorContext_;
};

} // Plugin
} // Dvvp
} // Analysis

#endif
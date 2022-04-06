#include "common/util/error_manager/error_manager.h"

ErrorManager &ErrorManager::GetInstance() {
  static ErrorManager instance;
  return instance;
}

error_message::Context &ErrorManager::GetErrorManagerContext() {
    error_message::Context error_context = {0UL, "", "", ""};
    return error_context;
}
    
void ErrorManager::SetErrorContext(const error_message::Context error_context)
{
    (void)(error_context);
}

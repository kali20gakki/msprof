#include "common/util/error_manager/error_manager.h"
#include <map>
#include <string>
#include <iostream>

using namespace std;
namespace {
    std::string logHeader;
}


ErrorManager &ErrorManager::GetInstance() {
  static ErrorManager instance;
  return instance;
}

int ErrorManager::Init(std::string path) {
  return 0;
}

const std::string &ErrorManager::GetLogHeader()
{
    return logHeader;
}

int ErrorManager::ReportErrMessage(std::string error_code, const std::map<std::string, std::string> &args_map)
{
    cout << "err code: " << error_code << endl;
    for (auto item : args_map) {
        cout << item.first << ": " << item.second <<endl;
    }
    return 0;
}

int ErrorManager::ReportInterErrMessage(std::string error_code,  const std::string &error_msg)
{
    cout << "err code: " << error_code << ", err_msg: " << error_msg << endl;

    return 0;
}

void ErrorManager::ATCReportErrMessage(std::string error_code, const std::vector<std::string> &key,
                                      const std::vector<std::string> &value) {
}

int ErrorManager::OutputErrMessage(int handle)
{
    return 0;
}

int ErrorManager::ParseJsonFile(std::string path)
{
    return 0;
}

int ErrorManager::ReadJsonFile(const std::string &file_path, void *handle)
{
    return 0;
}


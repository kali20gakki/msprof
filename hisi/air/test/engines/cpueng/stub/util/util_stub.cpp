#include "util_stub.h"
using namespace std;
using namespace ge;
namespace aicpu {

void GetDataTypeStub(const std::map<std::string, std::string> &dataTypes,
                 const std::string &dataTypeName,
                 std::set<ge::DataType> &dataType) 
{
    dataType.insert(DataType::DT_FLOAT16);
}

void GetDataTypeStub(const std::map<std::string, std::string> &dataTypes,
                 const std::string &dataTypeName,
                 std::vector<ge::DataType> &dataType) {}

const std::string RealPathStub(const std::string &path)
{
    const char *curDirName = getenv("HOME");
    if (curDirName == nullptr) {
        return "/home/";
    }
    std::string dirName(curDirName);
    return dirName;
}
}
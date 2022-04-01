#include <string>
#include <set>
#include <vector>
#include <map>
#include "external/ge/ge_api_error_codes.h"
#include "graph/types.h"
#include "graph/compute_graph.h"
#include "ge/ge_api_types.h"

namespace aicpu {
    void GetDataTypeStub(const std::map<std::string, std::string> &dataTypes,
                 const std::string &dataTypeName,
                 std::set<ge::DataType> &dataType);
    void GetDataTypeStub(const std::map<std::string, std::string> &dataTypes,
                 const std::string &dataTypeName,
                 std::vector<ge::DataType> &dataType);

    const std::string RealPathStub(const std::string &path);
}
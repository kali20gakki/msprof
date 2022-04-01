#include "common/engine/base_engine.h"

ge::RunContext CreateContext();

void DestroyContext(ge::RunContext& context);

namespace aicpu{
class TestForGetSoPath{
public:
    TestForGetSoPath() = default;
    ~TestForGetSoPath() = default;
    static TestForGetSoPath &Instance();
};
}
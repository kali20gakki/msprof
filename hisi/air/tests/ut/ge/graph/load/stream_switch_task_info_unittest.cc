#include <gtest/gtest.h>

#include <vector>

#define protected public
#define private public
#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/task_info/stream_switch_task_info.h"
#undef protected
#undef private

using namespace std;
using namespace testing;

namespace ge {
class UtestStreamSwitchTaskInfo : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestStreamSwitchTaskInfo, stream_switch_task_success) {
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_size = 0x40000;

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_.push_back(stream);

  GeTensorDesc desc;
  auto op_desc = std::make_shared<OpDesc>("stream_switch", STREAMSWITCH);
  AttrUtils::SetInt(op_desc, ATTR_NAME_STREAM_SWITCH_COND, 0);
  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, {0});
  op_desc->SetStreamId(0);
  op_desc->SetId(2);
  op_desc->AddInputDesc(desc);
  op_desc->AddInputDesc(desc);
  op_desc->SetInputOffset({8, 16});
  model.op_list_[op_desc->GetId()] = op_desc;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::StreamSwitchDef *stream_switch_def = task_def.mutable_stream_switch();
  stream_switch_def->set_op_index(op_desc->GetId());

  StreamSwitchTaskInfo task_info;
  EXPECT_EQ(task_info.CalculateArgs(task_def, &model), SUCCESS);
  EXPECT_EQ(task_info.Init(task_def, &model), SUCCESS);
  EXPECT_EQ(task_info.Distribute(), SUCCESS);
}
TEST_F(UtestStreamSwitchTaskInfo, stream_switch_task_fail) {
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_size = 0x40000;

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_.push_back(stream);

  GeTensorDesc desc;
  auto op_desc = std::make_shared<OpDesc>("stream_switch", STREAMSWITCH);
  op_desc->SetStreamId(0);
  op_desc->SetId(2);
  op_desc->SetInputOffset({8, 16});
  model.op_list_[op_desc->GetId()] = op_desc;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::StreamSwitchDef *stream_switch_def = task_def.mutable_stream_switch();
  stream_switch_def->set_op_index(op_desc->GetId());

  StreamSwitchTaskInfo task_info;
  // fail for STREAM_SWITCH_COND
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);
  EXPECT_EQ(task_info.CalculateArgs(task_def, &model), FAILED);

  // fail for input_size
  AttrUtils::SetInt(op_desc, ATTR_NAME_STREAM_SWITCH_COND, 0);
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  // fail for ACTIVE_STREAM_LIST
  op_desc->AddInputDesc(desc);
  op_desc->AddInputDesc(desc);
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, {});
  EXPECT_EQ(task_info.Init(task_def, &model), FAILED);

  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, {1});
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, {0});
  AttrUtils::SetBool(op_desc, ATTR_NAME_SWITCH_DATA_TYPE, RT_SWITCH_INT32);
  EXPECT_EQ(task_info.Init(task_def, &model), FAILED);
}
}  // end of namespace ge
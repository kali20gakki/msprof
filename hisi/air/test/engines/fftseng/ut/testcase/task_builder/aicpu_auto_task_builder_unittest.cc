/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <iostream>

#define private public
#define protected public
#include "task_builder/fftsplus_ops_kernel_builder.h"
#include "testcase/test_utils.h"

using namespace std;
using namespace fe;
using namespace ge;
using namespace ffts;

using AicpuAutoTaskBuilderPtr = shared_ptr<AicpuAutoTaskBuilder>;
class FFTSPlusAicpuAutoTaskBuilderUTest : public testing::Test
{
protected:
	void SetUp()
	{
		aicpu_auto_task_builder_ptr_ = make_shared<AicpuAutoTaskBuilder>();
		ffts_plus_def_ptr_ = new domi::FftsPlusTaskDef;
		node_ = CreateNode();
	}
	void TearDown() {
		delete ffts_plus_def_ptr_;
	}
	static NodePtr CreateNode()
	{
		FeTestOpDescBuilder builder;
		builder.SetName("aicpu_node_name");
		builder.SetType("aicpu_node_type");
		auto node = builder.Finish();

    FftsPlusCtxDefPtr ctxDefPtr = std::make_shared<domi::FftsPlusCtxDef>();
    node->GetOpDesc()->SetExtAttr("_ffts_plus_aicpu_ctx_def", ctxDefPtr);
		vector<uint32_t> context_id_list = {3, 4};
		ge::AttrUtils::SetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list);

		ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>();
    tsmp_ptr->thread_mode = 1;
		tsmp_ptr->parallel_window_size = 2;
		node->GetOpDesc()->SetExtAttr("_sgt_struct_info", tsmp_ptr);
		return node;
	}

public:
	AicpuAutoTaskBuilderPtr aicpu_auto_task_builder_ptr_;
	domi::FftsPlusTaskDef *ffts_plus_def_ptr_;
	NodePtr node_{ nullptr };
};

TEST_F(FFTSPlusAicpuAutoTaskBuilderUTest, Gen_Aicpu_AUTO_ContextDef_SUCCESS)
{
	Status ret = aicpu_auto_task_builder_ptr_->GenContextDef(node_, ffts_plus_def_ptr_);
	EXPECT_EQ(fe::SUCCESS, ret);
}
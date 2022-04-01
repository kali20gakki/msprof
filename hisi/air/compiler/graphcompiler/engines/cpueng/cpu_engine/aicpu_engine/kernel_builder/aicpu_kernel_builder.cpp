/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "aicpu_kernel_builder.h"

namespace aicpu {
KernelBuilderPtr AicpuKernelBuilder::instance_ = nullptr;

inline KernelBuilderPtr AicpuKernelBuilder::Instance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        instance_.reset(new (std::nothrow) AicpuKernelBuilder);
    });
    return instance_;
}

FACTORY_KERNEL_BUILDER_CLASS_KEY(AicpuKernelBuilder, "AICPUBuilder")
}

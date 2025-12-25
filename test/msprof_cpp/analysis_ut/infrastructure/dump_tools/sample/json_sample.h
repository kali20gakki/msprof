/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_JSON_SAMPLE_H
#define ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_JSON_SAMPLE_H

#include "sample/sample_types.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

Analysis::Infra::JsonWriter& operator<<(Analysis::Infra::JsonWriter& stream, const DumpToolSampleStruct& sample);

#endif
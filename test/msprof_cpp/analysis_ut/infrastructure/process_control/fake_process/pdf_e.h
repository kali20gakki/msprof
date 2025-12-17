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
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PDF_E_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PDF_E_H
#include <iostream>
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Ps {

class PdfE : public Infra::Process {
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
};

}

}

#endif
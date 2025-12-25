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

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ERROR_CODE_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ERROR_CODE_H

#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
enum {
    PARSER_GET_FILE_ERROR                               = ERROR_NO(SERVICE_ID_PARSER, 0, 0x1),
    PARSER_GET_FFTS_PROFILE_FILE_ERROR                  = ERROR_NO(SERVICE_ID_PARSER, 1, 0x1),
    PARSER_GET_STARS_SOC_FILE_ERROR                     = ERROR_NO(SERVICE_ID_PARSER, 2, 0x1),
    PARSER_GET_TS_TRACK_FILE_ERROR                      = ERROR_NO(SERVICE_ID_PARSER, 3, 0x1),
    PARSER_GET_FREQ_FILE_ERROR                          = ERROR_NO(SERVICE_ID_PARSER, 4, 0x1),
    PARSER_NEW_BINARY_DATA_ERROR                        = ERROR_NO(SERVICE_ID_PARSER, 0, 0x2),
    PARSER_READ_DATA_ERROR                              = ERROR_NO(SERVICE_ID_PARSER, 0, 0x3),
    PARSER_PARSE_DATA_ERROR                             = ERROR_NO(SERVICE_ID_PARSER, 0, 0x4),
    PARSER_FOPEN_ERROR                                  = ERROR_NO(SERVICE_ID_PARSER, 0, 0x5),
    PARSER_FREAD_ERROR                                  = ERROR_NO(SERVICE_ID_PARSER, 0, 0x6),

    PARSER_TS_TRACK_NO_NEED_ITEM                        = ERROR_NO(SERVICE_ID_PARSER, 0, 0x7),
    PARSER_ERROR_SIZE_MISMATCH                          = ERROR_NO(SERVICE_ID_PARSER, 0, 0x8)
};
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ERROR_CODE_H

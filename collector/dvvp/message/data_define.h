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

#ifndef ANALYSIS_DVVP_MESSAGE_DATA_DEFINE_H
#define ANALYSIS_DVVP_MESSAGE_DATA_DEFINE_H

#include "message.h"

namespace analysis {
namespace dvvp {
namespace message {

struct CollectionStartEndTime : analysis::dvvp::message::BaseInfo {
    std::string collectionDateBegin;
    std::string collectionDateEnd;
    std::string collectionTimeBegin;
    std::string collectionTimeEnd;
    std::string clockMonotonicRaw;

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, collectionDateBegin);
        SET_VALUE(object, collectionDateEnd);
        SET_VALUE(object, collectionTimeBegin);
        SET_VALUE(object, collectionTimeEnd);
        SET_VALUE(object, clockMonotonicRaw);
    }

    void FromObject(const nlohmann::json &object) override {}
};

}
}
}

#endif

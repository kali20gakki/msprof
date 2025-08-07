/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: data struct define header
 * Create: 2025-07-31
 */

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

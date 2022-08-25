/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_STREAMIO_EPOLL_OPERATION_H
#define ANALYSIS_DVVP_STREAMIO_EPOLL_OPERATION_H

#include <stdint.h>
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#include <winsock2.h>
#else
#include <sys/select.h>
#include <sys/time.h>
#endif
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace streamio {
namespace common {
using MmSockHandle = Collector::Dvvp::Mmpa::MmSockHandle;
class SelectOperation {
public:
    SelectOperation();
    ~SelectOperation();
    void SelectAdd(MmSockHandle fd);
    void SelectDel(MmSockHandle fd);
    bool SelectIsSet(MmSockHandle fd);
    void SelectClear();

private:
    MmSockHandle maxFd_;
    fd_set readfd_;

    SelectOperation &operator=(const SelectOperation &op);
    SelectOperation(const SelectOperation &op);
};
}  // namespace common
}  // namespace streamio
}  // namespace dvvp
}  // namespace analysis

#endif

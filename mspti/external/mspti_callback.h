/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_callback.h
 * Description        : Callback API.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_CALLBACK_H
#define MSPTI_CALLBACK_H

#include "mspti_base.h"
#include "mspti_cbid.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    MSPTI_CB_DOMAIN_INVALID           = 0,
    MSPTI_CB_DOMAIN_RUNTIME           = 1,
    MSPTI_CB_DOMAIN_SIZE,
    MSPTI_CB_DOMAIN_FORCE_INT         = 0x7fffffff
} msptiCallbackDomain;

using msptiCallbackId = uint32_t;

typedef enum {
    MSPTI_API_ENTER                 = 0,
    MSPTI_API_EXIT                  = 1,
    MSPTI_API_CBSITE_FORCE_INT     = 0x7fffffff
} msptiApiCallbackSite;

typedef struct {
    msptiApiCallbackSite callbackSite;
    const char *functionName;
} msptiCallbackData;

typedef void (*msptiCallbackFunc)(
    void *userdata,
    msptiCallbackDomain domain,
    msptiCallbackId cbid,
    const msptiCallbackData *cbdata);

struct msptiSubscriber_st {
    msptiCallbackFunc handle;
    void* userdata;
};

typedef struct msptiSubscriber_st *msptiSubscriberHandle;

MSPTI_API msptiResult msptiSubscribe(
    msptiSubscriberHandle *subscriber,
    msptiCallbackFunc callback,
    void* userdata);

MSPTI_API msptiResult msptiUnsubscribe(msptiSubscriberHandle subscriber);

MSPTI_API msptiResult msptiEnableCallback(
    uint32_t enable,
    msptiSubscriberHandle subscriber,
    msptiCallbackDomain domain,
    msptiCallbackId cbid);

MSPTI_API msptiResult msptiEnableDomain(
    uint32_t enable,
    msptiSubscriberHandle subscriber,
    msptiCallbackDomain domain);

#if defined(__cplusplus)
}
#endif

#endif

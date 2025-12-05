# msptiCallbackData<a name="ZH-CN_TOPIC_0000002014599005"></a>

msptiCallbackData为[msptiCallbackFunc](msptiCallbackFunc.md)的**cbdata**对应的结构体，用于指定传递到回调函数的数据。

定义如下：

```cpp
typedef struct {
    msptiApiCallbackSite callbackSite;    // 回调触发点的位置（开始或结束）
    const char *functionName;    // 当前函数名称
    const void *functionParams;    // 当前函数的参数
    const void *functionReturnValue;    // 指向Runtime或驱动API返回值的指针
    const char *symbolName;    // 当前函数所操作的符号的名称
    uint64_t correlationId;    // 此回调的活动记录关联ID。对于一个驱动域回调MSPTI_CB_DOMAIN_DRIVER_API，此ID将等于相关ID，CANN驱动对应的MSPTI_ActivityAPI记录中的函数调用。对于运行时域回调MSPTI_CB_DOMAIN_RUNTIME_API，此ID将等于相关CANN对应的MSPTI_ActivityAPI记录的ID。运行时函数调用。在回调中，这个ID用于将用户数据与活动记录关联。
    uint64_t reserved1;    // 内部预留，未定义
    uint64_t reserved2;    // 内部预留，未定义
    uint64_t *correlationData;    // 入口和出口回调之间共享数据的指针。调用运行时或驱动API函数。这个字段可用于将64位值从入口回调传递到对应的退出回调。
} msptiCallbackData;
```


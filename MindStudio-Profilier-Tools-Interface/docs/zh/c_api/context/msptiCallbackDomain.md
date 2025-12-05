# msptiCallbackDomain<a name="ZH-CN_TOPIC_0000002049212081"></a>

msptiCallbackDomain为[msptiEnableCallback](msptiEnableCallback.md)、[msptiEnableDomain](msptiEnableDomain.md)和[msptiCallbackFunc](msptiCallbackFunc.md)调用的回调领域枚举类。

每个枚举值代表一组相关API函数或CANN驱动程序活动的回调点。定义如下：

```cpp
typedef enum {
	MSPTI_CB_DOMAIN_INVALID = 0,    // 非法值
	MSPTI_CB_DOMAIN_RUNTIME = 1,    // Runtime API相关回调点
	MSPTI_CB_DOMAIN_HCCL = 2,    // 通信API相关回调点
	MSPTI_CB_DOMAIN_SIZE,
	MSPTI_CB_DOMAIN_FORCE_INT = 0x7fffffff
} msptiCallbackDomain;
```


# msptiApiCallbackSite<a name="ZH-CN_TOPIC_0000002013131536"></a>

msptiApiCallbackSite为[msptiCallbackData](msptiCallbackData.md)调用的枚举类。

指定API调用中发出回调的点。定义如下：

```cpp
typedef enum {
	MSPTI_API_ENTER = 0,    // 在进入API时回调
	MSPTI_API_EXIT = 1,    // 退出API后回调
	MSPTI_API_CBSITE_FORCE_INT = 0x7fffffff
} msptiApiCallbackSite;
```


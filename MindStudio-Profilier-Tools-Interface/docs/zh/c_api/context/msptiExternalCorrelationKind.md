# msptiExternalCorrelationKind<a name="ZH-CN_TOPIC_0000002157365565"></a>

支持关联的外部API的类型。

msptiExternalCorrelationKind为[msptiActivityPushExternalCorrelationId](msptiActivityPushExternalCorrelationId.md)和[msptiActivityExternalCorrelation](msptiActivityExternalCorrelation.md)调用的枚举类，定义如下：

```cpp
typedef enum {
	MSPTI_EXTERNAL_CORRELATION_KIND_INVALID = 0,   // 非法值
	MSPTI_EXTERNAL_CORRELATION_KIND_UNKNOWN = 1,   // MSPTI未知的外部API
	MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0 = 2,   // 外部API为CUSTOM0
	MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM1 = 3,   // 外部API为CUSTOM1
	MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM2 = 4,   // 外部API为CUSTOM2
	MSPTI_EXTERNAL_CORRELATION_KIND_SIZE,   // 在此行之前添加新的类型
	MSPTI_EXTERNAL_CORRELATION_KIND_FORCE_INT = 0x7fffffff,
} msptiExternalCorrelationKind;
```


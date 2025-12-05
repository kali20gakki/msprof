# msptiActivityExternalCorrelation<a name="ZH-CN_TOPIC_0000002122003924"></a>

msptiActivityExternalCorrelation为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_EXTERNAL\_CORRELATION](msptiActivityKind.md)对应的结构体，用于关联Activity Record，定义如下：

```cpp
typedef struct {
	msptiActivityKind kind;   // Activity Record类型MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION
	msptiExternalCorrelationKind externalKind;   // 记录关联的外部API的类型
	uint64_t externalId;   // 关联外部API的关联ID
	uint64_t correlationId;   // 关联CANN API的关联ID
} msptiActivityExternalCorrelation;
```


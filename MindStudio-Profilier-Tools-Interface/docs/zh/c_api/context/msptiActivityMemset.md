# msptiActivityMemset<a name="ZH-CN_TOPIC_0000002155706481"></a>

msptiActivityMemset为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_MEMSET](msptiActivityKind.md)对应的结构体，用于上报Memset Activity信息，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;    // Activity Record类型MSPTI_ACTIVITY_KIND_MEMSET
	uint32_t value;    // Memset设置的目标值
	uint64_t bytes;    // Memset设置的字节数
	uint64_t start;    //  Memset操作的开始时间戳，单位ns
	uint64_t end;    //  Memset操作的结束时间戳，单位ns
	uint32_t deviceId;    // Memset操作所在的设备ID
	uint32_t streamId;    // Memset操作的流ID
	uint64_t correlationId;    // Memset操作的关联ID
	uint8_t isAsync;    // 是否通过异步内存API进行内存设置操作
} msptiActivityMemset;
```


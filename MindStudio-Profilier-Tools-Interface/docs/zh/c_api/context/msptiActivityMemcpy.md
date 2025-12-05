# msptiActivityMemcpy<a name="ZH-CN_TOPIC_0000002155584865"></a>

msptiActivityMemcpy为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_MEMCPY](msptiActivityKind.md)对应的结构体，用于上报Memcpy Activity信息，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;    // Activity Record类型MSPTI_ACTIVITY_KIND_MEMCPY
	msptiActivityMemcpyKind copyKind;    // 内存拷贝操作的类型
	uint64_t bytes;    // 内存拷贝操作传输的字节数
	uint64_t start;    // 内存拷贝操作的开始时间戳，单位ns
	uint64_t end;    // 内存拷贝操作的结束时间戳，单位ns
	uint32_t deviceId;    // 内存拷贝操作所在的设备ID
	uint32_t streamId;    // 内存拷贝操作的流ID
	uint64_t correlationId;    // 内存拷贝操作的关联ID
	uint8_t isAsync;    // 是否通过异步内存API进行内存拷贝操作
} msptiActivityMemcpy;
```


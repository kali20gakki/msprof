# msptiActivityApi<a name="ZH-CN_TOPIC_0000002045864213"></a>

msptiActivityApi为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_API](msptiActivityKind.md)对应的结构体，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;   // Activity Record类型MSPTI_ACTIVITY_KIND_API
	uint64_t start;   // API执行的开始时间戳，单位ns。开始和结束时间戳均为0时则无法收集API的时间戳信息
	uint64_t end;   // API执行的结束时间戳，单位ns。开始和结束时间戳均为0时则无法收集API的时间戳信息
	struct {
		uint32_t processId;   // API运行设备的进程ID
		uint32_t threadId;   // API运行流的线程ID
	} pt;
	uint64_t correlationId;   // API的关联ID。每个API执行都被分配一个唯一的关联ID，该关联ID与启动API的驱动程序或运行时API Activity Record的关联ID相同
	const char* name;   // API的名称，该名称在整个Activity Record中保持一致，不建议更改
} msptiActivityApi;
```


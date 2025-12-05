# msptiActivityKernel<a name="ZH-CN_TOPIC_0000002045983217"></a>

msptiActivityKernel为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_KERNEL](msptiActivityKind.md)对应的结构体，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;   // Activity Record类型MSPTI_ACTIVITY_KIND_KERNEL
	uint64_t start;   // Kernel在NPU设备上执行开始时间戳，单位ns。开始和结束时间戳均为0时则无法收集Kernel的时间戳信息
	uint64_t end;   // Kernel执行的结束时间戳，单位ns。开始和结束时间戳均为0时则无法收集Kernel的时间戳信息
	struct {
		uint32_t deviceId;   // Kernel运行设备的Device ID
		uint32_t streamId;   // Kernel运行流的Stream ID
	} ds;
	uint64_t correlationId;   // Runtime在launch Kernel时生成的唯一ID，其他Activity可通过该值与Kernel进行关联
	const char *type;   // Kernel的类型
	const char *name;   // Kernel的名称，该名称在整个Activity Record中保持一致，不建议修改
} msptiActivityKernel;
```


# msptiActivityHccl<a name="ZH-CN_TOPIC_0000002086158432"></a>

msptiActivityHccl为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_HCCL](msptiActivityKind.md)对应的结构体，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
	msptiActivityKind kind;   // Activity Record类型MSPTI_ACTIVITY_KIND_HCCL
	uint64_t start;   // 通信算子在NPU设备上执行开始时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
	uint64_t end;   // 通信算子执行的结束时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
	struct {
		uint32_t deviceId;   // 通信算子运行设备的Device ID
		uint32_t streamId;   // 通信算子运行流的Stream ID
	} ds;
	uint64_t bandWidth;   // 通信算子运行时的带宽，单位GB/s
	const char *name;   // 通信算子的名称
	const char *commName;   // 通信域的名称
} msptiActivityHccl;
```


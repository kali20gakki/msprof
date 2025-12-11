# HcclData<a name="ZH-CN_TOPIC_0000002119450412"></a>

HcclData为[HcclMonitor.start](HcclMonitor-start.md)调用的结构体，定义如下：

```python
class HcclData:
	self.kind   # Activity Record类型MSPTI_ACTIVITY_KIND_HCCL
	self.start   # 通信算子在NPU设备上执行开始时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
	self.end   # 通信算子执行的结束时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
	self.device_id   # 通信算子运行设备的Device ID
	self.stream_id   # 通信算子运行流的Stream ID
	self.bandwidth   # 通信算子运行时的带宽，单位GB/s
	self.name   # 通信算子的名称
	self.comm_name   # 通信域的名称
```


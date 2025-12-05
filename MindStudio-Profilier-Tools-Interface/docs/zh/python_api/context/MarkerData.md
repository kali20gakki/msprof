# MarkerData<a name="ZH-CN_TOPIC_0000002154732221"></a>

展示mstx接口的瞬时打点数据，mstx接口详细介绍请参见[mstx API使用示例](zh-cn_topic_0000002017958049.md)。

MarkerData为[MstxMonitor.start](MstxMonitor-start.md)调用的结构体，定义如下：

```python
class MarkerData:
	[self.kind](msptiActivityKind.md)   # Activity Record类型MSPTI_ACTIVITY_KIND_MARKER
	self.flag: MsptiActivityFlag   # Marker数据的flag标记
	self.source_kind: MsptiActivitySourceKind   # 标记数据的来源类型
	self.timestamp   # 标记的时间戳，单位ns。值为0时表示无法为标记收集时间戳信息
	self.id   # 标记的ID
	self.object_id: MsptiObjectId   # 识别Marker的进程ID、线程ID、Device ID、Stream ID
	self.name   # 标记的名称，结束标记时，值为空
	self.domain   # 标记所属domain域的名称，默认域为default
class MsptiObjectId:
	PROCESS_ID = "processId"   # 进程ID，如果为device侧数据，则固定为-1
	THREAD_ID = "threadId"   # 线程ID，如果为device侧数据，则固定为-1
	DEVICE_ID = "deviceId"   # 设备ID，如果为host侧数据，则固定为-1
	STREAM_ID = "streamId"   # 流ID，如果为host侧数据，则固定为-1
```


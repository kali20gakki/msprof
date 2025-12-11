# RangeMarkerData<a name="ZH-CN_TOPIC_0000002279641170"></a>

展示mstx接口的Range打点数据，mstx接口详细介绍请参见《mstx API使用示例》。

RangeMarkerData为[MstxMonitor.start](MstxMonitor-start.md)调用的结构体，定义如下：

```python
class RangeMarkerData:
	self.kind   # Activity Record类型MSPTI_ACTIVITY_KIND_MARKER
	self.source_kind: MsptiActivitySourceKind   # 标记数据的来源类型
	self.id   # 标记的ID
	self.object_id: MsptiObjectId   # 识别Marker的进程ID、线程ID、Device ID、Stream ID
	self.name   # 标记的名称，结束标记时值为空
	self.domain   # 标记所属domain域的名称，默认域为default
	self.start   # Range打点的开始时间，mark打点值为0
	self.end   # Range打点的结束时间，mark打点值为0
class MsptiObjectId:
	PROCESS_ID = "processId"   # 进程ID，如果为device侧数据，则固定为-1
	THREAD_ID = "threadId"   # 线程ID，如果为device侧数据，则固定为-1
	DEVICE_ID = "deviceId"   # 设备ID，如果为host侧数据，则固定为-1
	STREAM_ID = "streamId"   # 流ID，如果为host侧数据，则固定为-1
```


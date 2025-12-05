# msptiActivityFlag<a name="ZH-CN_TOPIC_0000002154732225"></a>

Activity Record的活动标记。标记可以通过按位OR组合，将多个标记与Activity Record关联。每个标记都特定与某一Activity Record关联。

msptiActivityFlag为[MarkerData](MarkerData.md)结构体内调用的枚举类，定义如下：

```python
class MsptiActivityFlag(Enum):
	MSPTI_ACTIVITY_FLAG_NONE = 0   # 表示没有Activity Record的活动标记
	MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS = 1 << 0 # 调用mstxMarkA接口传入stream设置为nullptr时，Host侧标记瞬时事件，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
	MSPTI_ACTIVITY_FLAG_MARKER_START = 1 << 1 # 调用mstxRangeStartA接口传入stream设置为nullptr时，Host侧标识打点开始，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
	MSPTI_ACTIVITY_FLAG_MARKER_END = 1 << 2 # 调用mstxRangeEnd传入的ID来自传入的stream设置为nullptr的mstxRangeStartA，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
	MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE = 1 << 3 # 调用mstxMarkA接口传入有效stream时，对应的打点数据类型，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
	MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE = 1 << 4 # 调用mstxRangeStartA接口传入有效stream时，对应的打点数据类型，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
	MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE = 1 << 5 # 调用mstxRangeEnd传入的ID来自传入有效stream时的mstxRangeStartA，[MSPTI_ACTIVITY_KIND_MARKER](msptiActivityKind.md)使用
```


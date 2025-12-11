# msptiActivityKind<a name="ZH-CN_TOPIC_0000002154810617"></a>

msptiActivityKind为[HcclData](HcclData.md)、[KernelData](KernelData.md)、[MarkerData](MarkerData.md)和[RangeMarkerData](RangeMarkerData.md)调用的枚举类。

MSPTI通过msptiActivityKind对所有能采集到的数据进行分类，每个枚举值对应一个数据的结构体类型。定义如下：

```python
class MsptiActivityKind(Enum):
	MSPTI_ACTIVITY_KIND_INVALID = 0   # 非法值
	MSPTI_ACTIVITY_KIND_MARKER = 1   # MSPTI打点能力（标记瞬时时刻）的Activity Record类型，支持最大打点个数为uint32_t最大值，返回结构体MarkerData或RangeMarkerData
	MSPTI_ACTIVITY_KIND_KERNEL = 2   # aclnn场景下，计算类算子信息采集的Activity Record类型，返回结构体KernelData
	MSPTI_ACTIVITY_KIND_API = 3   # 预留参数，暂未开放
	MSPTI_ACTIVITY_KIND_HCCL = 4   # 通信算子采集Activity Record类型，返回结构体HcclData
	MSPTI_ACTIVITY_KIND_COUNT
	MSPTI_ACTIVITY_KIND_FORCE_INT = 0x7fffffff
```


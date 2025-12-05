# msptiActivitySourceKind<a name="ZH-CN_TOPIC_0000002119627750"></a>

标记Activity数据来源。标记数据的来源是Host还是Device。

msptiActivitySourceKind为[MarkerData](MarkerData.md)结构体内调用的枚举类，定义如下：

```python
class MsptiActivitySourceKind(Enum):
	MSPTI_ACTIVITY_SOURCE_KIND_HOST = 0   # 标记数据的来源是Host
	MSPTI_ACTIVITY_SOURCE_KIND_DEVICE = 1   # 标记数据的来源是Device
```


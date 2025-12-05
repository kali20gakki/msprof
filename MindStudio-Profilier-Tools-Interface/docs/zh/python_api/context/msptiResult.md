# msptiResult<a name="ZH-CN_TOPIC_0000002119450416"></a>

msptiResult是MSPTI返回的错误和结果代码，为枚举类。定义如下：

```python
class MsptiResult(Enum):
	MSPTI_SUCCESS = 0    # MSPTI执行成功，无错误
	MSPTI_ERROR_INVALID_PARAMETER = 1    # 回调函数为NULL时返回，表示MSPTI执行失败
	MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED = 2    # 已存在MSPTI用户时返回，表示MSPTI执行失败
	MSPTI_ERROR_MAX_LIMIT_REACHED = 3    # Activity Buffer没有更多的Record数据时返回，表示MSPTI执行失败
	MSPTI_ERROR_DEVICE_OFFLINE = 4    # 无法获取DEVICE侧信息
	MSPTI_ERROR_INNER = 999    # 无法初始化MSPTI时返回，表示MSPTI执行失败
	MSPTI_ERROR_FORCE_INT = 0x7fffffff
```


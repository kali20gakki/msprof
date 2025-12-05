# msptiActivityMemcpyKind<a name="ZH-CN_TOPIC_0000002120344714"></a>

内存拷贝类型

msptiActivityMemcpyKind为[msptiActivityMemcpy](msptiActivityMemcpy.md)调用的枚举类，定义如下：

```cpp
typedef enum {
	MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN = 0,    // 内部预留，未定义
	MSPTI_ACTIVITY_MEMCPY_KIND_HOST = 1,    // Host到Host的内存拷贝类型
	MSPTI_ACTIVITY_MEMCPY_KIND_HTOD = 2,    // Host到Device的内存拷贝类型
	MSPTI_ACTIVITY_MEMCPY_KIND_DTOH = 3,    // Device到Host的内存拷贝类型
	MSPTI_ACTIVITY_MEMCPY_KIND_DTOD = 4,    // Device到Device的内存拷贝类型
	MSPTI_ACTIVITY_MEMCPY_KIND_DEFAULT = 5    // 同一Device上的设备内存到设备内存的拷贝类型
} msptiActivityMemcpyKind;
```


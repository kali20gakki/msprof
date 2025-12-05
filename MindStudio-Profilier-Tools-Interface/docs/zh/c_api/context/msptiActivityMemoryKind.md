# msptiActivityMemoryKind<a name="ZH-CN_TOPIC_0000002155584861"></a>

请求的内存类型。

msptiActivityMemoryKind为[msptiActivityMemory](msptiActivityMemory.md)调用的枚举类，定义如下：

```cpp
typedef enum {
	MSPTI_ACTIVITY_MEMORY_UNKNOWN = 0,    // 内部预留，未定义
	MSPTI_ACTIVITY_MEMORY_DEVICE = 1,    // 设备内存
} msptiActivityMemoryKind;
```


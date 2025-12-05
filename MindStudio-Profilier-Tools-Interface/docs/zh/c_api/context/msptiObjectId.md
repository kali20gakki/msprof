# msptiObjectId<a name="ZH-CN_TOPIC_0000002012690846"></a>

msptiObjectId为[msptiActivityMarker](msptiActivityMarker.md)调用，用于识别Marker的进程ID、线程ID、Device ID、Stream ID。定义如下：

```cpp
typedef union PACKED_ALIGNMENT {
	struct {
		uint32_t processId;   // ActivityMarker的进程ID
		uint32_t threadId;   // ActivityMarker的线程ID
	} pt;
	struct {
		uint32_t deviceId;   // ActivityMarker进程所在设备的Device ID
		uint32_t streamId;   //  ActivityMarker进程所在流的Stream ID
	} ds;
} msptiObjectId;
```


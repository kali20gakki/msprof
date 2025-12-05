# msptiActivityCommunication<a name="ZH-CN_TOPIC_0000002305037540"></a>

msptiActivityCommunication为Activity Record类型[MSPTI\_ACTIVITY\_KIND\_COMMUNICATION](msptiActivityKind.md)对应的结构体，用于关联Activity Record，定义如下：

```cpp
typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;   // Activity Record类型MSPTI_ACTIVITY_KIND_COMMUNICATION
    msptiCommunicationDataType dataType;   // 记录通信算子的数据类型
    uint64_t count;   // 记录通信算子的数据量
    struct {
        uint32_t deviceId;   // 通信算子运行设备的Device ID
        uint32_t streamId;   // 通信算子运行流的Stream ID
    } ds;
    uint64_t start;   // 通信算子在NPU设备上执行开始时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
    uint64_t end;   // 通信算子执行的结束时间戳，单位ns。开始和结束时间戳均为0时则无法收集通信算子的时间戳信息
    const char* algType;   // 通信算子使用的算法
    const char* name;   // 通信算子的名称
    const char* commName;   // 通信算子所在通信域的名称
    uint64_t correlationId;   // 通信算子执行时生成的唯一ID，其他Activity可通过该值与通信算子进行关联
} msptiActivityCommunication;
```


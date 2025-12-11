# 总体说明<a name="ZH-CN_TOPIC_0000002108084148"></a>

## 接口简介<a name="section172424491588"></a>

Profiling模块提供msPTI Python接口，用于实现采集各模块性能数据。

msPTI API的功能介绍和使用示例请参见[msPTI工具用户指南](..//README.md)。

## 接口列表<a name="section18403103610813"></a>

具体接口如下：

**表 1**  msPTI Python API

|接口|说明|
|--|--|
|**HcclMonitor类型**|**HcclMonitor说明**|
|[HcclMonitor.start](./context/HcclMonitor-start.md)|标识通信算子性能数据采集的开始。|
|[HcclMonitor.stop](./context/HcclMonitor-stop.md)|标识通信算子性能数据采集的结束。|
|[HcclMonitor.flush_all](./context/HcclMonitor-flush_all.md)|调用回调函数，将缓冲区中的所有Activity数据写入用户内存。|
|[HcclMonitor.set_buffer_size](./context/HcclMonitor-set_buffer_size.md)|在采集开始前设置Activity Buffer的大小。|
|**KernelMonitor类型**|**KernelMonitor说明**|
|[KernelMonitor.start](./context/KernelMonitor-start.md)|标识Kernel性能数据采集的开始。|
|[KernelMonitor.stop](./context/KernelMonitor-stop.md)|标识Kernel性能数据采集的结束。|
|[KernelMonitor.flush_all](./context/KernelMonitor-flush_all.md)|调用回调函数，将缓冲区中的所有Activity数据写入用户内存。|
|[KernelMonitor.set_buffer_size](./context/KernelMonitor-set_buffer_size.md)|在采集开始前设置Activity Buffer的大小。|
|**MstxMonitor类型**|**MstxMonitor说明**|
|[MstxMonitor.start](./context/MstxMonitor-start.md)|标识数据采集mstx打点的开始。|
|[MstxMonitor.stop](./context/MstxMonitor-stop.md)|标识数据采集mstx打点的结束。|
|[MstxMonitor.enable_domain](./context/MstxMonitor-enable_domain.md)|开启对应域打点的采集。|
|[MstxMonitor.disable_domain](./context/MstxMonitor-disable_domain.md)|关闭对应域打点的采集。|
|[MstxMonitor.flush_all](./context/MstxMonitor-flush_all.md)|调用回调函数，将缓冲区中的所有Activity数据写入用户内存。|
|[MstxMonitor.set_buffer_size](./context/MstxMonitor-set_buffer_size.md)|在采集开始前设置Activity Buffer的大小。|
|**Data Structure类型**||
|[HcclData](./context/HcclData.md)|Activity Record类型MSPTI_ACTIVITY_KIND_HCCL对应的结构体。|
|[KernelData](./context/KernelData.md)|Activity Record类型MSPTI_ACTIVITY_KIND_KERNEL对应的结构体。|
|[MarkerData](./context/MarkerData)|Activity Record类型MSPTI_ACTIVITY_KIND_MARKER对应的结构体。|
|[RangeMarkerData](./context/RangeMarkerData.md)|Activity Record类型MSPTI_ACTIVITY_KIND_MARKER对应的结构体。|
|**Enumeration类型**||
|[msptiResult](./context/msptiResult.md)|MSPTI返回的错误和结果代码。|
|[msptiActivityKind](./context/msptiActivityKind.md)|MSPTI支持的所有Activity类型。|
|[msptiActivityFlag](./context/msptiActivityFlag.md)|Activity Record的活动标记。|
|[msptiActivitySourceKind](./context/msptiActivitySourceKind.md)|标记Activity数据来源。|



# 总体说明<a name="ZH-CN_TOPIC_0000001977973392"></a>

## 接口简介<a name="section883612815318"></a>

Profiling模块提供MSPTI C接口，用于实现采集各模块性能数据。

msPTI API的功能介绍和使用示例请参见[msPTI工具](../README.md)。

头文件路径：$\{INSTALL\_DIR\}/include/mspti。

库文件路径：$\{INSTALL\_DIR\}/lib64/libmspti.so。

$\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。若安装的Ascend-cann-toolkit软件包，以root安装举例，则安装后文件存储路径为：/usr/local/Ascend/ascend-toolkit/latest。

## 接口列表<a name="section2321145165316"></a>

具体接口如下：

**表 1**  Activity API

|接口|说明|
|--|--|
|**Function类型**|**Function说明**|
|[msptiActivityRegisterCallbacks](./context/msptiActivityRegisterCallbacks.md)|向MSPTI注册回调函数，用于Activity Buffer处理。|
|[msptiActivityEnable](./context/msptiActivityEnable.md)|用于使能指定Activity类型数据的采集。|
|[msptiActivityDisable](./context/msptiActivityDisable.md)|停止收集特定类型的Activity Record。|
|[msptiActivityGetNextRecord](./context/msptiActivityGetNextRecord.md)|依次从Activity Buffer获取Activity Record数据。|
|[msptiActivityFlushAll](./context/msptiActivityFlushAll.md)|订阅者手动Flush Activity Buffer中记录的数据。|
|[msptiActivityFlushPeriod](./context/msptiActivityFlushPeriod.md)|设置Flush的执行周期。|
|[msptiActivityPushExternalCorrelationId](./context/msptiActivityPushExternalCorrelationId.md)|为调用线程推送外部关联ID。|
|[msptiActivityPopExternalCorrelationId](./context/msptiActivityPopExternalCorrelationId.md)|为调用线程拉取外部关联ID。|
|[msptiActivityEnableMarkerDomain](./context/msptiActivityEnableMarkerDomain.md)|开启对应域打点的采集。|
|[msptiActivityDisableMarkerDomain](./context/msptiActivityDisableMarkerDomain.md)|关闭对应域打点的采集。|
|**Typedef类型**|**Typedef说明**|
|[msptiBuffersCallbackRequestFunc](./context/msptiBuffersCallbackRequestFunc.md)|向MSPTI注册回调函数，申请Activity Buffer的存储空间。|
|[msptiBuffersCallbackCompleteFunc](./context/msptiBuffersCallbackCompleteFunc.md)|向MSPTI注册回调函数，释放Activity Buffer中的数据。|
|**Enumeration类型**|**Enumeration说明**|
|[msptiActivityKind](./context/msptiActivityKind.md)|MSPTI支持的所有Activity类型。|
|[msptiActivityFlag](./context/msptiActivityFlag.md)|Activity Record的活动标记。|
|[msptiActivitySourceKind](./context/msptiActivitySourceKind.md)|标记Activity数据来源。|
|[msptiActivityMemoryOperationType](./context/msptiActivityMemoryOperationType.md)|内存操作类型的枚举类。|
|[msptiActivityMemoryKind](./context/msptiActivityMemoryKind.md)|内存类型的枚举类。|
|[msptiActivityMemcpyKind](./context/msptiActivityMemcpyKind.md)|内存拷贝类型的枚举类。|
|[msptiExternalCorrelationKind](./context/msptiExternalCorrelationKind.md)|支持关联的外部API的类型。|
|[msptiCommunicationDataType](./context/msptiCommunicationDataType.md)|记录通信算子的数据类型。|
|**Data Structure类型**|**Data Structure说明**|
|[msptiActivity](./context/msptiActivity.md)|Activity Record的基础结构体。|
|[msptiActivityApi](./context/msptiActivityApi.md)|Activity Record类型MSPTI_ACTIVITY_KIND_API对应的结构体。|
|[msptiActivityHccl](./context/msptiActivityHccl.md)|Activity Record类型MSPTI_ACTIVITY_KIND_HCCL对应的结构体。|
|[msptiActivityKernel](./context/msptiActivityKernel.md)|Activity Record类型MSPTI_ACTIVITY_KIND_KERNEL对应的结构体。|
|[msptiActivityMarker](./context/msptiActivityMarker.md)|Activity Record类型MSPTI_ACTIVITY_KIND_MARKER对应的结构体。|
|[msptiActivityMemory](./context/msptiActivityMemory.md)|Activity Record类型MSPTI_ACTIVITY_KIND_MEMORY对应的结构体。|
|[msptiActivityMemset](./context/msptiActivityMemset.md)|Activity Record类型MSPTI_ACTIVITY_KIND_MEMSET对应的结构体。|
|[msptiActivityMemcpy](./context/msptiActivityMemcpy.md)|Activity Record类型MSPTI_ACTIVITY_KIND_MEMCPY对应的结构体。|
|[msptiActivityExternalCorrelation](./context/msptiActivityExternalCorrelation.md)|Activity Record类型MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION对应的结构体。|
|[msptiActivityCommunication](./context/msptiActivityCommunication.md)|Activity Record类型MSPTI_ACTIVITY_KIND_COMMUNICATION对应的结构体。|
|**Union类型**|**Union说明**|
|[msptiObjectId](./context/msptiObjectId.md)|用于识别Marker的进程ID、线程ID、Device ID、Stream ID。|


Activity Record：NPU的Profiling记录，使用结构体表示，如msptiActivityApi、msptiActivityMarker等。

Activity Buffer：用于缓存Activity Record数据，并将一个或多个Activity Record从MSPTI传输到客户端。用户根据业务需要提供空的Activity Buffer缓冲区，以确保Activity Record不会被遗漏。

**表 2**  Callback API

|接口|说明|
|--|--|
|**Function类型**|**Function说明**|
|[msptiSubscribe](./context/msptiSubscribe.md)|通过该接口向MSPTI注册回调函数。|
|[msptiUnsubscribe](./context/msptiUnsubscribe.md)|向MSPTI注销当前订阅者。|
|[msptiEnableCallback](./context/msptiEnableCallback.md)|为特定**domain**和**CallbackId**的订阅者开启或关闭回调。|
|[msptiEnableDomain](./context/msptiEnableDomain.md)|为特定**domain**的订阅者开启或关闭所有回调。|
|**Typedef类型**|**Typedef说明**|
|[msptiCallbackFunc](./context/msptiCallbackFunc.md)|回调函数类型。|
|[msptiCallbackId](./context/msptiCallbackId.md)|注册的Callback调用点ID。|
|[msptiSubscriberHandle](./context/msptiSubscriberHandle.md)|订阅者的句柄。|
|**Enumeration类型**|**Enumeration说明**|
|[msptiCallbackDomain](./context/msptiCallbackDomain.md)|相关API函数或CANN驱动程序活动的回调点。|
|[msptiApiCallbackSite](./context/msptiApiCallbackSite.md)|指定API调用中发出回调的点，如回调的开始和回调的结束。|
|[msptiCallbackIdRuntime](./context/msptiCallbackIdRuntime.md)|Runtime API函数的索引定义。|
|[msptiCallbackIdHccl](./context/msptiCallbackIdHccl.md)|通信API函数的索引的简要定义。|
|**Data Structure类型**|**Data Structure说明**|
|[msptiCallbackData](./context/msptiCallbackData.md)|用于指定传递到回调函数的数据。|


**表 3**  Result Codes

|接口|说明|
|--|--|
|**Enumeration类型**|**Enumeration说明**|
|[msptiResult](./context/msptiResult.md)|MSPTI返回的错误和结果代码。|



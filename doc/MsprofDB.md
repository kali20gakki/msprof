# msprof系统调优导出db格式表结构说明
1. 基础数据表（msprof和ascend pytorch profiler等生成，作为基础数据格式）
2. 分析与汇总类数据表（recipe和建议类基于基础数据表生成，作为分析与汇总类格式，用于前端显示）


## 1. msprofiler数据db格式表结构
db命名：msprof_{时间戳}.db

### 单位相关
1、时间相关，统一使用纳秒（ns），且为本地unix时间。

2、内存相关，统一使用字节（Byte）

3、带宽相关，统一使用Byte/s

4、频率相关，统一使用MHz


### ENUM_API_TYPE

格式：

| 字段名  | 类型      | 索引  | 含义    |
|------|---------|-----|-------|
| id   | INTEGER | 主键  | id    |
| name | TEXT    |     | api类型 |

内容：

| id    | name    |
|-------|---------|
| 20000 | acl     |
| 15000 | model   |
| 10000 | node    |
| 5500  | hccl    |
| 5000  | runtime |
| 50001 | op |
| 50002 | queue |
| 50003 | trace |
| 50004 | mstx |

变更记录：

| 日期        | 内容      |
|-----------|---------|
| 2024/3/7  | 330首次上线 |
| 2024/3/13 | 补充表内容   |
| 2024/8/5  | 新增PYTORCH_API表中type字段的枚举值 |


### ENUM_MEMORY

变更记录：

| 日期        | 内容               |
|-----------|------------------|
| 2024/3/7  | 330首次上线          |
| 2024/3/13 | 删除该表，STRING_IDS中 |


### ENUM_MODULE

格式：

| 字段名  | 类型      | 索引  | 含义              |
|------|---------|-----|-----------------|
| id   | INTEGER | 主键  | id              |
| name | TEXT    |     | 组件名(数据来源slog组件) |

内容：

| id  | name             |
|-----|------------------|
| 0   | SLOG             |
| 1   | IDEDD            |
| 2   | IDEDH            |
| 3   | HCCL             |
| 4   | FMK              |
| 5   | HIAIENGINE       |
| 6   | DVPP             |
| 7   | RUNTIME          |
| 8   | CCE              |
| 9   | HDC              |
| 10  | DRV              |
| 11  | MDCFUSION        |
| 12  | MDCLOCATION      |
| 13  | MDCPERCEPTION    |
| 14  | MDCFSM           |
| 15  | MDCCOMMON        |
| 16  | MDCMONITOR       |
| 17  | MDCBSWP          |
| 18  | MDCDEFAULT       |
| 19  | MDCSC            |
| 20  | MDCPNC           |
| 21  | MLL              |
| 22  | DEVMM            |
| 23  | KERNEL           |
| 24  | LIBMEDIA         |
| 25  | CCECPU           |
| 26  | ASCENDDK         |
| 27  | ROS              |
| 28  | HCCP             |
| 29  | ROCE             |
| 30  | TEFUSION         |
| 31  | PROFILING        |
| 32  | DP               |
| 33  | APP              |
| 34  | TS               |
| 35  | TSDUMP           |
| 36  | AICPU            |
| 37  | LP               |
| 38  | TDT              |
| 39  | FE               |
| 40  | MD               |
| 41  | MB               |
| 42  | ME               |
| 43  | IMU              |
| 44  | IMP              |
| 45  | GE               |
| 46  | MDCFUSA          |
| 47  | CAMERA           |
| 48  | ASCENDCL         |
| 49  | TEEOS            |
| 50  | ISP              |
| 51  | SIS              |
| 52  | HSM              |
| 53  | DSS              |
| 54  | PROCMGR          |
| 55  | BBOX             |
| 56  | AIVECTOR         |
| 57  | TBE              |
| 58  | FV               |
| 59  | MDCMAP           |
| 60  | TUNE             |
| 61  | HSS              |
| 62  | FFTS             |
| 63  | OP               |
| 64  | UDF              |
| 65  | HICAID           |
| 66  | TSYNC            |
| 67  | AUDIO            |
| 68  | TPRT             |
| 69  | ASCENDCKERNEL    |
| 70  | ASYS             |
| 71  | ATRACE           |
| 72  | RTC              |
| 73  | SYSMONITOR       |
| 74  | AML              |
| 75  | ADETECT          |
| 76  | INVLID_MOUDLE_ID |



变更记录：

| 日期         | 内容                                                    |
|------------|-------------------------------------------------------|
| 2024/3/7   | 330首次上线                                               |
| 2024/3/13  | ENUM_NPU_MODULE更名为ENUM_MODULE，当前模块并不只针对NPU，通用性考虑做表名变更 |
| 2024/3/26  | 修正当前枚举表中使用的枚举值（变更72 73，新增74）                     |
| 2024/11/12 | 修正当前枚举表中使用的枚举值（新增74 75）                          |


### ENUM_HCCL_DATA_TYPE

格式：

| 字段名  | 类型      | 索引  | 含义               |
|------|---------|-----|------------------|
| id   | INTEGER | 主键  | id               |
| name | TEXT    |     | hccl data type类型 |

内容：

| id    | name         |
|-------|--------------|
| 0     | INT8         |
| 1     | INT16        |
| 2     | INT32        |
| 3     | FP16         |
| 4     | FP32         |
| 5     | INT64        |
| 6     | UINT64       |
| 7     | UINT8        |
| 8     | UINT16       |
| 9     | UINT32       |
| 10    | FP64         |
| 11    | BFP16        |
| 12    | INT128       |
| 65534 | N/A          |
| 65535 | INVALID_TYPE |

变更记录：

| 日期        | 内容    |
|-----------|-------|
| 2024/3/13 | 计划新增  |
| 2024/5/19 | 630新增 |
| 2024/6/25 | 更新HcclDataType |
| 2024/7/11 | 新增NA字段，避免L0场景下异常日志记录 |

### ENUM_HCCL_LINK_TYPE

格式：

| 字段名  | 类型      | 索引  | 含义               |
|------|---------|-----|------------------|
| id   | INTEGER | 主键  | id               |
| name | TEXT    |     | hccl link type类型 |

内容：

| id    | name          |
|-------|---------------|
| 0     | ON_CHIP       |
| 1     | HCCS          |
| 2     | PCIE          |
| 3     | ROCE          |
| 4     | SIO           |
| 5     | HCCS_SW       |
| 6     | STANDARD_ROCE |
| 7     | RESERVED      |
| 65534 | N/A          |
| 65535 | INVALID_TYPE  |

变更记录：

| 日期        | 内容                      |
|-----------|-------------------------|
| 2024/3/13 | 计划新增                    |
| 2024/5/19 | 630新增,同时增加新的type字段（4-7） |
| 2024/7/11 | 新增NA字段，避免L0场景下异常日志记录 |

### ENUM_HCCL_TRANSPORT_TYPE

格式：

| 字段名  | 类型      | 索引  | 含义                    |
|------|---------|-----|-----------------------|
| id   | INTEGER | 主键  | id                    |
| name | TEXT    |     | hccl transport type类型 |

内容：

| id    | name         |
|-------|--------------|
| 0     | SDMA         |
| 1     | RDMA         |
| 2     | LOCAL        |
| 65534 | N/A          |
| 65535 | INVALID_TYPE |

变更记录：

| 日期        | 内容    |
|-----------|-------|
| 2024/3/13 | 计划新增  |
| 2024/5/19 | 630新增 |
| 2024/7/11 | 新增NA字段，避免L0场景下异常日志记录 |


### ENUM_HCCL_RDMA_TYPE

格式：

| 字段名  | 类型      | 索引  | 含义               |
|------|---------|-----|------------------|
| id   | INTEGER | 主键  | id               |
| name | TEXT    |     | hccl rdma type类型 |

内容：

| id    | name                 |
|-------|----------------------|
| 0     | RDMA_SEND_NOTIFY     |
| 1     | RDMA_SEND_PAYLOAD    |
| 2     | RDMA_PAYLOAD_PREPARE |
| 3     | RDMA_PAYLOAD_CHECK   |
| 4     | RDMA_PAYLOAD_ACK     |
| 5     | RDMA_SEND_OP         |
| 65534 | N/A          |
| 65535 | INVALID_TYPE         |

变更记录：

| 日期        | 内容                      |
|-----------|-------------------------|
| 2024/3/13 | 计划新增                    |
| 2024/5/19 | 630新增,同时增加新的type字段（2-5） |
| 2024/7/11 | 新增NA字段，避免L0场景下异常日志记录 |

### ENUM_MEMCPY_OPERATION

格式：

| 字段名  | 类型      | 索引  | 含义               |
|------|---------|-----|------------------|
| id   | INTEGER | 主键  | id               |
| name | TEXT    |     | 拷贝类型 |

内容：

| id    | name                 |
|-------|----------------------|
| 0     | host to host     |
| 1     | host to device    |
| 2     | device to host |
| 3     | device to device   |
| 4     | managed memory     |
| 5     | addr device to device         |
| 6     | host to device ex    |
| 7     | device to host ex         |
| 65535 | other  

变更记录：

| 日期        | 内容                      |
|-----------|-------------------------|
| 2024/12/18 | 新增              |

### STRING_IDS

格式：

| 字段名   | 类型      | 索引  | 含义           |
|-------|---------|-----|--------------|
| id    | INTEGER | 主键  | string id    |
| value | TEXT    |     | string value |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### SESSION_TIME_INFO

格式：

| 字段名         | 类型      | 索引  | 含义                |
|-------------|---------|-----|-------------------|
| startTimeNs | INTEGER |     | 任务开启时的unix时间，单位ns |
| endTimeNs   | INTEGER |     | 任务结束时的unix时间，单位ns |


变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### NPU_INFO

格式：

| 字段名  | 类型      | 索引  | 含义            |
|------|---------|-----|---------------|
| id   | INTEGER |     | deviceId      |
| name | TEXT    |     | device对应的芯片型号 |


变更记录：

| 日期        | 内容                |
|-----------|-------------------|
| 2024/3/7  | 330首次上线           |
| 2024/3/26 | 删除id主键设置，适配落盘目录结构 |


### HOST_INFO

格式：

| 字段名      | 类型      | 索引  | 含义                  |
|----------|---------|-----|---------------------|
| hostUid  | TEXT    |     | 标识host的唯一id         |
| hostName | TEXT    |     | host主机名称，如localhost |


变更记录：

| 日期        | 内容      |
|-----------|---------|
| 2024/5/10 | 630首次上线 |
| 2024/9/29 | hostUid字段变更为TEXT类型 |


### TASK

格式：

| 字段名          | 类型      | 索引        | 含义                         |
|--------------|---------|-----------|----------------------------|
| startNs      | INTEGER | TaskIndex | 算子任务开始时间，单位 ns             |
| endNs        | INTEGER |           | 算子任务结束时间，单位 ns             |
| deviceId     | INTEGER |           | 算子任务对应的deviceId            |
| connectionId | INTEGER |           | 生成host-device连线            |
| globalTaskId | INTEGER | TaskIndex | 用于唯一标识全局算子任务               |
| globalPid    | INTEGER |           | 算子任务执行时的pid                |
| taskType     | INTEGER |           | device执行该算子的加速器类型          |
| contextId    | INTEGER |           | 用于区分子图小算子，常见于MIX算子和FFTS+任务 |
| streamId     | INTEGER |           | 算子任务对应的streamId            |
| taskId       | INTEGER |           | 算子任务对应的taskId              |
| modelId      | INTEGER |           | 算子任务对应的modelId             |


变更记录：

| 日期        | 内容                                                           |
|-----------|--------------------------------------------------------------|
| 2024/3/7  | 330首次上线                                                      |
| 2024/3/26 | 删除globalTaskId主键设置，设置startNs， globalTaskId为联合索引，命名为TaskIndex |


### COMPUTE_TASK_INFO

格式：

| 字段名             | 类型      | 索引  | 含义                                                   |
|-----------------|---------|-----|------------------------------------------------------|
| name            | INTEGER |     | 算子名，STRING_IDS(name)                                 |
| globalTaskId    | INTEGER | 主键  | 全局算子任务id，用于关联TASK表                                   |
| blockDim        | INTEGER |     | 算子运行切分数量，对应算子运行时核数                                   |
| mixBlockDim     | INTEGER |     | mix算子从加速器的block_dim值                                 |
| taskType        | INTEGER |     | host执行该算子的加速器类型，STRING_IDS(taskType)                 |
| opType          | INTEGER |     | 算子类型，STRING_IDS(opType)                              |
| inputFormats    | INTEGER |     | 算子输入数据格式，STRING_IDS(inputFormats)                    |
| inputDataTypes  | INTEGER |     | 算子输入数据类型，STRING_IDS(inputDataTypes)                  |
| inputShapes     | INTEGER |     | 算子的输入维度，STRING_IDS(inputShapes)                      |
| outputFormats   | INTEGER |     | 算子输出数据格式，STRING_IDS(outputFormats)                   |
| outputDataTypes | INTEGER |     | 算子输出数据类型，STRING_IDS(outputDataTypes)                 |
| outputShapes    | INTEGER |     | 算子输出维度，STRING_IDS(outputShapes)                      |
| attrInfo        | INTEGER |     | 算子的attr信息，用来映射算子shape，算子自定义的参数等，STRING_IDS(attrInfo) |


变更记录：

| 日期        | 内容                                |
|-----------|-----------------------------------|
| 2024/3/7  | 330首次上线                           |
| 2024/5/19 | 修改原有block_dim为blockDim，统一命名风格为小驼峰 |


### COMMUNICATION_TASK_INFO

格式：

| 字段名           | 类型      | 索引                     | 含义                                                                     |
|---------------|---------|------------------------|------------------------------------------------------------------------|
| name          | INTEGER |                        | 算子名，STRING_IDS(name)                                                   |
| globalTaskId  | INTEGER | CommunicationTaskIndex | 全局算子任务id，用于关联TASK表                                                     |
| taskType      | INTEGER |                        | 算子类型，STRING_IDS(taskType)                                              |
| planeId       | INTEGER |                        | 网络平面ID                                                                 |
| groupName     | INTEGER |                        | 通信域，STRING_IDS(groupName)                                              |
| notifyId      | INTEGER |                        | notify唯一ID                                                             |
| rdmaType      | INTEGER |                        | rdma类型，包含：RDMASendNotify、RDMASendPayload，ENUM_HCCL_RDMA_TYPE(rdmaType) |
| srcRank       | INTEGER |                        | 源Rank                                                                  |
| dstRank       | INTEGER |                        | 目的Rank                                                                 |
| transportType | INTEGER |                        | 传输类型，包含：LOCAL、SDMA、RDMA，ENUM_HCCL_TRANSPORT_TYPE(transportType)        |
| size          | INTEGER |                        | 数据量，单位Byte                                                             |
| dataType      | INTEGER |                        | 数据格式，ENUM_HCCL_DATA_TYPE(dataType)                                     |
| linkType      | INTEGER |                        | 链路类型，包含：HCCS、PCIe、RoCE，ENUM_HCCL_LINK_TYPE(linkType)                   |
| opId          | INTEGER |                        | 对应的大算子Id，用于关联COMMUNICATION_OP表                                         |


变更记录：

| 日期        | 内容                                                                     |
|-----------|------------------------------------------------------------------------|
| 2024/3/7  | 330首次上线                                                                |
| 2024/3/26 | globalTaskId修改为索引，命名为CommunicationTaskIndex                            |
| 2024/5/19 | dataType，linkType，transportType，rdmaType字段不再从StringIds里取，而从对应的enum表中获取 |

### COMMUNICATION_OP

格式：

| 字段名          | 类型      | 索引  | 含义                                                                          |
|--------------|---------|-----|-----------------------------------------------------------------------------|
| opName       | INTEGER |     | 算子名，STRING_IDS(opName)，例：hcom_allReduce__428_0_1                            |
| startNs      | INTEGER |     | 通信大算子的开始时间，单位 ns                                                            |
| endNs        | INTEGER |     | 通信大算子的结束时间，单位 ns                                                            |
| connectionId | INTEGER |     | 生成host-device连线                                                             |
| groupName    | INTEGER |     | 通信域，STRING_IDS(groupName)，例：10.170.22.98%enp67s0f5_60000_0_1708156014257149 |
| opId         | INTEGER | 主键  | 通信大算子Id，用于关联COMMUNICATION_TASK_INFO表                                        |
| relay        | INTEGER |     | 借轨通信标识，暂未开发                                                                 |
| retry        | INTEGER |     | 重传标识，暂未开发                                                                   |
| dataType     | INTEGER |     | 大算子传输的数据类型，如（INT8，FP32），ENUM_HCCL_DATA_TYPE(dataType)                       |
| algType      | INTEGER |     | 通信算子使用的算法，可分为多个阶段，如（HD-MESH）                                                |
| count        | NUMERIC |     | 算子传输的dataType类型的数据量                                                         |
| opType       | INTEGER |     | 算子类型，STRING_IDS(opType), 例：hcom_broadcast_                                  |


变更记录：

| 日期        | 内容                                     |
|-----------|----------------------------------------|
| 2024/3/7  | 330首次上线                                |
| 2024/5/10 | 添加opType字段                             |
| 2024/5/19 | dataType字段不再从StringIds里取，而从对应的enum表中获取 |

### CANN_API

格式：

| 字段名          | 类型      | 索引  | 含义                             |
|--------------|---------|-----|--------------------------------|
| startNs      | INTEGER |     | api的开始时间，单位 ns                 |
| endNs        | INTEGER |     | api的结束时间，单位 ns                 |
| type         | INTEGER |     | api所属层级                        |
| globalTid    | INTEGER |     | 该api所属的全局tid。高32位：pid，低32位：tid |
| connectionId | INTEGER | 主键  | 用于关联TASK表和COMMUNICATION_OP表    |
| name         | INTEGER |     | 该api的名字，STRING_IDS(name)       |


变更记录：

| 日期        | 内容                                               |
|-----------|--------------------------------------------------|
| 2024/3/7  | 330首次上线                                          |
| 2024/3/13 | 区分框架API表，更名为CANN_API。删除apiId，重新设置connectionId为主键 |

### ACC_PMU

格式：

| 字段名           | 类型      | 索引  | 含义                |
|---------------|---------|-----|-------------------|
| accId         | INTEGER |     | 加速器id             |
| readBwLevel   | INTEGER |     | DVPP和DSA加速器读带宽的等级 |
| writeBwLevel  | INTEGER |     | DVPP和DSA加速器写带宽的等级 |
| readOstLevel  | INTEGER |     | DVPP和DSA加速器读并发的等级 |
| writeOstLevel | INTEGER |     | DVPP和DSA加速器写并发的等级 |
| timestampNs   | NUMERIC |     | 本地时间，单位 ns        |
| deviceId      | INTEGER |     | deviceId          |


变更记录：

| 日期        | 内容              |
|-----------|-----------------|
| 2024/3/7  | 330首次上线         |
| 2024/3/13 | 表字段更名，突出level含义 |


### SOC_BANDWIDTH_LEVEL

格式：

| 字段名             | 类型      | 索引  | 含义            |
|-----------------|---------|-----|---------------|
| l2BufferBwLevel | INTEGER |     | L2 Buffer带宽等级 |
| mataBwLevel     | INTEGER |     | Mata带宽等级      |
| timestampNs     | NUMERIC |     | 本地时间，单位 ns    |
| deviceId        | INTEGER |     | deviceId      |


变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### NIC

格式：

| 字段名          | 类型      | 索引  | 含义                  |
|--------------|---------|-----|---------------------|
| deviceId     | INTEGER |     | deviceId            |
| timestampNs  | INTEGER |     | 本地时间，单位 ns          |
| bandwidth    | INTEGER |     | 带宽，单位 B/s           |
| rxPacketRate | NUMERIC |     | 收包速率，单位 packet/s    |
| rxByteRate   | NUMERIC |     | 接收字节速率，单位 B/s       |
| rxPackets    | INTEGER |     | 累计收包数量，单位 packet    |
| rxBytes      | INTEGER |     | 累计接收字节数量，单位 Byte    |
| rxErrors     | INTEGER |     | 累计接收错误包数量，单位 packet |
| rxDropped    | INTEGER |     | 累计接收丢包数量，单位 packet  |
| txPacketRate | NUMERIC |     | 发包速率，单位 packet/s    |
| txByteRate   | NUMERIC |     | 发送字节速率，单位 B/s       |
| txPackets    | INTEGER |     | 累计发包数量，单位 packet    |
| txBytes      | INTEGER |     | 累计发送字节数量，单位 Byte    |
| txErrors     | INTEGER |     | 累计发送错误包数量，单位 packet |
| txDropped    | INTEGER |     | 累计发送丢包数量，单位 packet  |
| funcId       | INTEGER |     | 端口号                 |

公式：
summary计算公式：
字段均为原summary中呈现字段
公式中使用的变量为当前表字段

| 字段            | 公式                                                            |
|---------------|---------------------------------------------------------------|
| duration      | max(timestamp) - min(timestamp)                               |
| bandwidth     | bandwidth中的第一个的值                                              |
| rxbandwidth   | ((max(rxBytes) - min(rxBytes)) * 8) / (duration * bandwidth)  |
| txbandwidth   | ((max(txBytes) - min(txBytes)) * 8) / (duration * bandwidth)  |
| rxpacket      | sum(rxPacketRate) / (max(timestamp) - min(timestamp))         |
| rxerrorrate   | sum(rxErrors) / count(rxErrors) / sum(rxPacketRate) * 100%    |
| rxdroppedrate | sum(rxDropped) / count(rxDropped)  / sum(rxPacketRate) * 100% |
| txpacket      | sum(txPacketRate) / (max(timestamp) - min(timestamp))         |
| txerrorrate   | sum(txErrors) / count(txErrors) / sum(txPacketRate) * 100%    |
| txdroppedrate | sum(txDropped) / sum(txPacketRate) * 100%                     |

timeline计算公式：
字段均为原timeline中呈现字段
公式中使用的变量为当前表字段

| 字段                     | 公式                          |
|------------------------|-----------------------------|
| rx_bandwidth_effciency | rxByteRate * 8  / bandwidth |
| rx_packets             | rxPacketRate                |
| rx_error_rate          | rxErrors / rxPackets        |
| rx_dropped_rate        | rxDropped   / rxPackets     |
| tx_bandwidth_effciency | txByteRate * 8  / bandwidth |
| tx_packets             | txPacketRate                |
| tx_error_rate          | txErrors / txPackets        |
| tx_dropped_rate        | txDropped   / txPackets     |


变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### RoCE

格式：

| 字段名          | 类型      | 索引  | 含义                  |
|--------------|---------|-----|---------------------|
| deviceId     | INTEGER |     | deviceId            |
| timestampNs  | INTEGER |     | 本地时间，单位 ns          |
| bandwidth    | INTEGER |     | 带宽，单位 B/s           |
| rxPacketRate | NUMERIC |     | 收包速率，单位 packet/s    |
| rxByteRate   | NUMERIC |     | 接收字节速率，单位 B/s       |
| rxPackets    | INTEGER |     | 累计收包数量，单位 packet    |
| rxBytes      | INTEGER |     | 累计接收字节数量，单位 Byte    |
| rxErrors     | INTEGER |     | 累计接收错误包数量，单位 packet |
| rxDropped    | INTEGER |     | 累计接收丢包数量，单位 packet  |
| txPacketRate | NUMERIC |     | 发包速率，单位 packet/s    |
| txByteRate   | NUMERIC |     | 发送字节速率，单位 B/s       |
| txPackets    | INTEGER |     | 累计发包数量，单位 packet    |
| txBytes      | INTEGER |     | 累计发送字节数量，单位 Byte    |
| txErrors     | INTEGER |     | 累计发送错误包数量，单位 packet |
| txDropped    | INTEGER |     | 累计发送丢包数量，单位 packet  |
| funcId       | INTEGER |     | 端口号                 |

公式参考NIC

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### RoH
变更记录：

| 日期        | 内容                          |
|-----------|-----------------------------|
| 2024/3/13 | 计划新增                        |
| 2024/5/19 | RoH交付件和原有RoCE交付件命名一致不再新增，删去 |


### LLC
(仅支持非310)

格式：

| 字段名         | 类型      | 索引  | 含义                           |
|-------------|---------|-----|------------------------------|
| deviceId    | INTEGER |     | deviceId                     |
| llcId       | INTEGER |     |                              |
| timestampNs | INTEGER |     | 本地时间，单位 ns                   |
| hitRate     | NUMERIC |     | 三级缓存命中率(100%)                |
| throughput  | NUMERIC |     | 三级缓存吞吐量，单位 B/s               |
| mode        | INTEGER |     | 模式，用于区分是读或写，STRING_IDS(mode) |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### TASK_PMU_INFO
(仅支持stars芯片：310B + 910B)

格式：

| 字段名          | 类型      | 索引  | 含义                              |
|--------------|---------|-----|---------------------------------|
| globalTaskId | INTEGER |     | 全局算子任务id，用于关联TASK表              |
| name         | INTEGER |     | pmu metric 指标名，STRING_IDS(name) |
| value        | NUMERIC |     | 对应指标名的数值                        |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### SAMPLE_PMU_TIMELINE

格式：

| 字段名         | 类型      | 索引  | 含义                                     |
|-------------|---------|-----|----------------------------------------|
| deviceId    | INTEGER |     | deviceId                               |
| timestampNs | INTEGER |     | 本地时间，单位 ns                             |
| totalCycle  | INTEGER |     | 对应core在时间片上的cycle数                     |
| usage       | NUMERIC |     | 对应core在时间片上的利用率（100%）                  |
| freq        | NUMERIC |     | 对应core在时间片上的频率，单位 MHz                  |
| coreId      | INTEGER |     | coreId                                 |
| coreType    | INTEGER |     | core类型(AIC 或 AIV)，STRING_IDS(coreType) |

变更记录：

| 日期        | 内容                         |
|-----------|----------------------------|
| 2024/3/7  | 330首次上线                    |
| 2024/3/13 | 新增coreType用以区分AIC或AIV的core |



### SAMPLE_PMU_SUMMARY

格式：

| 字段名      | 类型      | 索引  | 含义                                     |
|----------|---------|-----|----------------------------------------|
| deviceId | INTEGER |     | deviceId                               |
| metric   | INTEGER |     | pmu metric 指标名，STRING_IDS(metric)      |
| value    | NUMERIC |     | 对应指标名的数值                               |
| coreId   | INTEGER |     | coreId                                 |
| coreType | INTEGER |     | core类型(AIC 或 AIV)，STRING_IDS(coreType) |

变更记录：

| 日期        | 内容                         |
|-----------|----------------------------|
| 2024/3/7  | 330首次上线                    |
| 2024/3/13 | 新增coreType用以区分AIC或AIV的core |


### NPU_MEM

格式：

| 字段名         | 类型      | 索引  | 含义                                  |
|-------------|---------|-----|-------------------------------------|
| type        | INTEGER |     | event类型，app或device，STRING_IDS(type) |
| ddr         | NUMERIC |     | ddr占用大小，单位 B                        |
| hbm         | NUMERIC |     | hbm占用大小，单位 B                        |
| timestampNs | INTEGER |     | 本地时间，单位 ns                          |
| deviceId    | INTEGER |     | deviceId                            |

变更记录：

| 日期        | 内容                               |
|-----------|----------------------------------|
| 2024/3/7  | 330首次上线                          |
| 2024/3/13 | ddrUsage和hbmUsage更名为ddr和hbm，计划变更 |
| 2024/5/19 | ddrUsage和hbmUsage更名为ddr和hbm      |


### NPU_MODULE_MEM

格式：

| 字段名           | 类型      | 索引  | 含义                         |
|---------------|---------|-----|----------------------------|
| moduleId      | INTEGER |     | 组件类型，ENUM_MODULE(moduleId) |
| timestampNs   | INTEGER |     | 本地时间，单位 ns                 |
| totalReserved | NUMERIC |     | 内存占用大小，单位 B                |
| deviceId      | INTEGER |     | deviceId                   |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### NPU_OP_MEM

格式：

| 字段名           | 类型      | 索引  | 含义                                 |
|---------------|---------|-----|------------------------------------|
| operatorName  | INTEGER |     | 算子名字，STRING_IDS(operatorName)      |
| addr          | INTEGER |     | 内存申请释放首地址                          |
| type          | INTEGER |     | 用于区分申请或是释放，STRING_IDS(type)        |
| size          | INTEGER |     | 申请的内存大小，单位 B                       |
| timestampNs   | INTEGER |     | 本地时间，单位 ns                         |
| globalTid     | INTEGER |     | 该条记录的全局tid。高32位：pid，低32位：tid       |
| totalAllocate | NUMERIC |     | 总体已分配的内存大小，单位 B                    |
| totalReserve  | NUMERIC |     | 总体持有的内存大小，单位 B                     |
| component     | INTEGER |     | 组件名，命令行侧写死GE，STRING_IDS(component) |
| deviceId      | INTEGER |     | deviceId                           |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### HBM

格式：

| 字段名         | 类型      | 索引  | 含义                       |
|-------------|---------|-----|--------------------------|
| deviceId    | INTEGER |     | deviceId                 |
| timestampNs | INTEGER |     | 本地时间，单位 ns               |
| bandwidth   | NUMERIC |     | 带宽，单位 B/s                |
| hbmId       | INTEGER |     | 内存访问单元id                 |
| type        | INTEGER |     | 用于区分读或写，STRING_IDS(type) |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### DDR

格式：

| 字段名         | 类型      | 索引  | 含义            |
|-------------|---------|-----|---------------|
| deviceId    | INTEGER |     | deviceId      |
| timestampNs | INTEGER |     | 本地时间，单位 ns    |
| read        | NUMERIC |     | 内存读取带宽，单位 B/s |
| write       | NUMERIC |     | 内存写入带宽，单位 B/s |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |


### HCCS

格式：

| 字段名          | 类型      | 索引  | 含义          |
|--------------|---------|-----|-------------|
| deviceId     | INTEGER |     | deviceId    |
| timestampNs  | INTEGER |     | 本地时间，单位 ns  |
| txThroughput | NUMERIC |     | 发送带宽，单位 B/s |
| rxThroughput | NUMERIC |     | 接收带宽，单位 B/s |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |



### PCIE

格式：

| 字段名                 | 类型      | 索引  | 含义         |
|---------------------|---------|-----|------------|
| deviceId            | INTEGER |     | deviceId   |
| timestampNs         | INTEGER |     | 本地时间，单位 ns |
| txPostMin           | NUMERIC |     |            |
| txPostMax           | NUMERIC |     |            |
| txPostAvg           | NUMERIC |     |            |
| txNonpostMin        | NUMERIC |     |            |
| txNonpostMax        | NUMERIC |     |            |
| txNonpostAvg        | NUMERIC |     |            |
| txCplMin            | NUMERIC |     |            |
| txCplMax            | NUMERIC |     |            |
| txCplAvg            | NUMERIC |     |            |
| txNonpostLatencyMin | NUMERIC |     |            |
| txNonpostLatencyMax | NUMERIC |     |            |
| txNonpostLatencyAvg | NUMERIC |     |            |
| rxPostMin           | NUMERIC |     |            |
| rxPostMax           | NUMERIC |     |            |
| rxPostAvg           | NUMERIC |     |            |
| rxNonpostMin        | NUMERIC |     |            |
| rxNonpostMax        | NUMERIC |     |            |
| rxNonpostAvg        | NUMERIC |     |            |
| rxCplMin            | NUMERIC |     |            |
| rxCplMax            | NUMERIC |     |            |
| rxCplAvg            | NUMERIC |     |            |

1、Tx表示发送端，Rx表示接收端。

2、PCIe_post：PCIe Posted数据传输带宽，单位B/s。

3、PCIe_nonpost：PCIe Non-Posted数据传输带宽，单位B/s。

4、PCIe_cpl：接收写请求的完成数据包，单位B/s。

5、PCIe_nonpost_latency：PCIe Non-Posted模式下的传输时延，单位ns


变更记录：

| 日期       | 内容                           |
|----------|------------------------------|
| 2024/3/7 | 330首次上线                      |
| 2024/9/2 | PCIe_nonpost_latency 单位校准为ns |


### META_DATA

格式：

| 字段名          | 类型      | 索引  | 含义  |
|--------------|---------|-----|-----|
| name         | TEXT    |     | 字段名 |
| value        | TEXT    |     | 数值  |

内容：

| name                 | 含义                           |
|----------------------|------------------------------|
| SCHEMA_VERSION       | 总版本号，如 1.0.2，整体变更参照竞品        |
| SCHEMA_VERSION_MAJOR | 大版本号，如 1， 仅当数据库格式存在重写或重构时更改  |
| SCHEMA_VERSION_MINOR | 中版本号，如 0， 当更改列或类型时更改，存在兼容性问题 |
| SCHEMA_VERSION_MICRO | 小版本号，如 2， 当更新表时都会更改，不具有兼容性问题 |


变更记录：

| 日期        | 内容                      |
|-----------|-------------------------|
| 2024/5/15 | 新增表，用于存放msprof导出的相关配置信息 |


### MSTX_EVENTS

格式

| 字段名          | 类型      | 索引  | 含义  |
|--------------|---------|-----|------------------------|
| startNs      | INTEGER |     | host侧tx打点数据开始时间  |
| endNs        | INTEGER |     | host侧tx打点数据结束时间  |
| eventType    | INTEGER |     | host侧tx打点数据类型在ENUM_MSTX_EVENT_TYPE表里对应的id|
| rangeId      | INTEGER |     | host侧range类型tx数据对应的range id(预留)|
| category     | INTEGER |     | host侧tx数据所属的分类id(预留)|
| message      | INTEGER |     | host侧tx打点数据携带信息在STRING_IDS表里对应的id|
| globalTid    | INTEGER |     | host侧tx打点数据开始线程的全局tid|
| endGlobalTid | INTEGER |     | host侧tx打点数据结束线程的全局tid|
| domainId     | INTEGER |     | host侧tx打点数据所属域的域id(预留)|
| connectionId | INTEGER |     | host侧tx打点数据与TASK表里npu打点task的关联id |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/5/31 | 630首次上线，用于保存host侧tx打点数据 |

### ENUM_MSTX_EVENT_TYPE

格式

| 字段名          | 类型      | 索引  | 含义  |
|--------------|---------|-----|------------------------|
| id           | INTEGER | 主键 | host侧tx打点数据event类型对应的id  |
| name         | INTEGER |     | host侧tx打点数据event类型  |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/5/31 | 630首次上线，用于保存host侧tx打点数据event 类型 |

### MEMCPY_INFO

格式

| 字段名          | 类型      | 索引  | 含义  |
|--------------|---------|-----|------------------------|
| globalTaskId | NUMERIC | 主键 | 全局算子任务id，用于关联TASK  |
| size     | NUMERIC |     | 拷贝的数据量  |
| memcpyOperation| NUMERIC |    | 拷贝类型，ENUM_MEMCPY_OPERATION(memcpyOperation)|

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/12/17 | 1230首次上线，为MEMCPY_ASYNC类型的task提供数据量、拷贝方向等数据 |


### AICORE_FREQ

格式

| 字段名          | 类型      | 索引  | 含义              |
|--------------|---------|-----|-----------------|
| deviceId | INTEGER |  | deviceId        |
| timestampNs     | NUMERIC |     | 频率变化时的本地时间      |
| freq| INTEGER |    | aicore频率值，单位MHz |

变更记录：

| 日期         | 内容                         |
|------------|----------------------------|
| 2024/12/25 | 630已上线，1225补充AICORE_FREQ内容 |

## 2. pytorch框架数据db格式表结构
db命名：ascend_pytorch_profiler_{rankId}.db
（无rankId时：ascend_pytorch_profiler.db）

### STRING_IDS

格式:

| 字段名   | 类型      | 索引  | 含义           |
|-------|---------|-----|--------------|
| id    | INTEGER | 主键  | string 对应的id |
| value | TEXT    |     | string内容     |


变更记录:

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### PYTORCH_API

格式:

| 字段名          | 类型      | 索引  | 含义       |
|--------------  |---------|-----|---------------------------------------------|
| startNs        | INTEGER |     | op api开始时间                                  |
| endNs          | INTEGER |     | op api结束时间                                  |
| globalTid      | INTEGER |     | 该api所属的全局tid。高32位：pid，低32位：tid     |
| connectionId   | INTEGER |     | 索引，用于在CONNECTION_IDS表查询对应的connection_id;如果无connection_id，此处为空    |
| name           | INTEGER |     | 该op api名在STRING_IDS表中对应的id    |
| sequenceNumber | INTEGER |     | op序号    |
| fwdThreadId    | INTEGER |     | op前向线程id|
| inputDtypes    | INTEGER |     | 输入数据类型在STRING_IDS表中对应的id |
| inputShapes    | INTEGER |     | 输入shape在STRING_IDS表中对应的id |
| callchainId    | INTEGER |     | 索引，用于在PYTORCH_CALLCHAINS表查询对应的call stack信息;如果无stack信息，此处为空 |
| type           | INTEGER |     | 标记数据类型，op、queue、mstx还是python_trace，数据类型存于枚举表ENUM _API_TYPE中 |

变更记录:

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |
| 2024/8/5  | PYTORCH_API新增type字段 |

### CONNECTION_IDS

格式:

| 字段名          | 类型      | 索引  | 含义              |
|--------------|---------|-----|---------------------------------------------|
| id           | INTEGER |     | 索引，对应PYTORCH_API表的connectionId  |
| connectionId | INTEGER |     | 用于表示关联关系的id，当前包括task_queue、fwd_bwd、torch-cann-task三种关联关系   |

变更记录:

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### PYTORCH_CALLCHAINS

格式:

| 字段名          | 类型      | 索引  | 含义              |
|--------------|---------|-----|---------------------------------------------|
| id           | INTEGER |     | 索引，对应PYTORCH_API表的callchainId  |
| stack        | INTEGER |     | 当前栈的字符串内容在STRING_IDS表中对应的id   |
| stackDepth   | INTEGER |     | 当前栈所在深度|

变更记录:

| 日期       | 内容      |
|----------|---------|
| 2024/3/7 | 330首次上线 |

### MEMORY_RECORD

格式:

| 字段名            | 类型      | 索引  | 含义                                   |
|----------------|---------|-----|--------------------------------------|
| component      | INTEGER |     | 组件名(GE、PTA、PTA+GE)在STRING_IDS表中对应的id |
| timestamp      | INTEGER |     | 时间戳                                  |
| totalAllocated | INTEGER |     | 内存分配总额                               |
| totalReserved  | INTEGER |     | 内存预留总额                               |
| totalActive    | INTEGER |     | PTA流申请的总内存                           |
| streamPtr      | INTEGER |     | ascendcl流地址                          |
| deviceId       | INTEGER |     | device id                            |


变更记录:

| 日期        | 内容                                                                                |
|-----------|-----------------------------------------------------------------------------------|
| 2024/3/7  | 330首次上线                                                                           |
| 2024/5/19 | 修改total_allocated，total_reserved，total_active，stream_ptr， device_id等字段为小驼峰，统一命名规则 |
| 2024/7/11 | timestamp资料中字段修改，5.19漏改                                                           |

### OP_MEMORY

格式:

| 字段名                      | 类型      | 索引  | 含义                               |
|--------------------------|---------|-----|----------------------------------|
| name                     | INTEGER |     | torch op/ge op名在STRING_IDS中对应的id |
| size                     | INTEGER |     | 算子占用内存大小                         |
| acllocationTime          | INTEGER |     | 算子内存申请时间                         |
| releaseTime              | INTEGER |     | 算子内存释放时间                         |
| activeReleaseTime        | INTEGER |     | 内存实际归还内存池时间                      |
| duration                 | INTEGER |     | 内存占用时间                           |
| activeDuration           | INTEGER |     | 内存实际占用时间                         |
| allocationTotalAllocated | INTEGER |     | 算子内存分配时PTA和GE内存分配总额              |
| allocationTotalReserved  | INTEGER |     | 算子内存分配时PTA和GE内存占用总额              |
| allocationTotalActive    | INTEGER |     | 算子内存分配时当前流申请的内存总额                |
| releaseTotalAllocated    | INTEGER |     | 算子内存释放时PTA和GE内存分配总额              |
| releaseTotalReserved     | INTEGER |     | 算子内存释放时PTA和GE内存占用总额              |
| releaseTotalActive       | INTEGER |     | 算子内存释放时当前流申请的内存总额                |
| streamPtr                | INTEGER |     | ascendcl流地址                      |
| deviceId                 | INTEGER |     | device id                        |


变更记录:

| 日期        | 内容              |
|-----------|-----------------|
| 2024/3/7  | 330首次上线         |
| 2024/5/19 | 表中各字段统一修改为小驼峰命名 |


### RANK_DEVICE_MAP
无rank_id场景不生成该表

格式:

| 字段名      | 类型      | 索引  | 含义           |
|----------|---------|-----|--------------|
| rankId   | INTEGER |     | rank id值     |
| deviceId | INTEGER |     | 当前device id值 |

变更记录:

| 日期        | 内容              |
|-----------|-----------------|
| 2024/3/7  | 330首次上线         |
| 2024/5/19 | 表中各字段统一修改为小驼峰命名 |

### STEP_TIME
保存profiler采集step起始时间

格式:

| 字段名      | 类型      | 索引  | 含义           |
|----------|---------|-----|--------------|
| id       | INTEGER |     | step id值    |
| startNs  | INTEGER |     | step开始时间  |
| endNs    | INTEGER |     | step结束时间  |

变更记录:

| 日期        | 内容              |
|-----------|-----------------|
| 2024/5/31  | 630首次上线 |

### GC_RECORD
保存profiler采集的GC事件

格式:

| 字段名      | 类型    | 索引  | 含义           |
|----------- |---------|-------|--------------- |
| startNs    | INTEGER |       | GC事件开始时间  |
| endNs      | INTEGER |       | GC事件结束时间  |
| globalTid  | INTEGER |       | GC事件的全局tid |

变更记录:

| 日期        | 内容              |
|-----------|-----------------|
| 2024/7/23  | 930首次上线 |

### COMMUNICATION_SCHEDULE_TASK_INFO
保存通信调度的task info

格式:

| 字段名             | 类型      | 索引  | 含义                                                  |
|-----------------|---------|-----|-----------------------------------------------------|
| name            | INTEGER |     | 算子名，STRING_IDS(name)                                |
| globalTaskId    | INTEGER | 主键  | 全局算子任务id，用于关联TASK表                                  |
| taskType        | INTEGER |     | host执行该算子的加速器类型，STRING_IDS(taskType)                |
| opType          | INTEGER |     | 算子类型，STRING_IDS(opType)                             |


变更记录:

| 日期         | 内容       |
|------------|----------|
| 2024/12/16 | 1230首次上线 |

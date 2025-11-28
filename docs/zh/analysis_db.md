## 1. 通信性能数据communication_analyzer.db格式表结构
db命名：communication_analyzer.db

### 单位相关
1、时间相关，统一使用毫秒（ms）

2、通信数据量，统一使用MB

3、通信带宽，统一使用GB/s


### CommAnalyzerTime

格式:

|字段名|类型|索引|含义|
|------|------|------|------|
|hccl_op_name|TEXT||HCCL通信算子名称|
|group_name|TEXT||通信算子的分组|
|start_timestamp|INTEGER||通信开始时间戳|
|elapse_time|INTEGER||算子的通信总耗时|
|transit_time|INTEGER||通信时长。表示通信算子的通信耗时，如果通信耗时过长，可能是某条链路存在问题|
|wait_time|INTEGER||通信时长。表示通信算子的通信耗时，如果通信耗时过长，可能是某条链路存在问题|
|synchronization_time|INTEGER||同步时长。节点之间进行同步需要的时长|
|idle_time|INTEGER||通信算子下发耗时。通信算子下发耗时（idle_time） = 算子的通信总耗时（elapse_time） - 通信时长（transit_time） - 等待时长（wait_time）|



变更记录:

|日期|内容|
|------|------|
|2024/2/23|330首次上线|

### CommAnalyzerBandwidth

格式:

|字段名|类型|索引|含义|
|------|------|------|------|
|hccl_op_name|TEXT||HCCL通信算子名称|
|group_name|TEXT||通信算子的分组|
|transport_type|TEXT||通信传输类型，包含：LOCAL、SDMA、RDMA、PCIE、HCCS、SIO|
|transit_size|INTEGER||通信数据量|
|transit_time|INTEGER||通信时长。表示通信算子的通信耗时，如果通信耗时过长，可能是某条链路存在问题|
|bandwidth|INTEGER||通信带宽大小|
|large_packet_ratio|INTEGER||通信数据大包占比|
|package_size|INTEGER||一次传输的通信数据包大小，单位MB|
|count|INTEGER||通信传输次数|
|total_duration|INTEGER||数据传输总耗时|



变更记录:

|日期|内容|
|------|------|
|2024/2/23|330首次上线|

### CommAnalyzerMatrix

格式:

|字段名|类型|索引|含义|
|------|------|------|------|
|hccl_op_name|TEXT||HCCL通信算子名称|
|group_name|TEXT||通信算子的分组|
|src_rank|TEXT||通信源Rank|
|dst_rank|TEXT||通信目的Rank|
|transport_type|TEXT||通信传输类型，包含：LOCAL、SDMA、RDMA、PCIE、HCCS、SIO|
|transit_size|INTEGER||通信数据量|
|transit_time|INTEGER||通信时长。表示通信算子的通信耗时，如果通信耗时过长，可能是某条链路存在问题|
|bandwidth|INTEGER||通信带宽大小|




变更记录:

|日期|内容|
|------|------|
|2024/2/23|330首次上线|

## 2. pytorch analysis.db格式表结构

### CommAnalyzerBandwidth

格式：

| 字段名             | 类型     | 索引 |  含义 |
| ------------      | --------| ---- | ------------ |
| hccl_op_name      | TEXT    |     | HCCL大算子名，例：hcom_broadcast__303_1_1  |
| group_name        | TEXT    |     | 通信域hashId，例：3915571125887837303  |
| transport_type    | TEXT    |     | 传输类型，包含：LOCAL、SDMA、RDMA  |
| transit_size      | NUMERIC |     | 传输的数据量，单位：MB  |
| transit_time      | NUMERIC |     | 传输耗时，单位：ms  |
| bandwidth         | NUMERIC |     | 带宽，单位：GB/s  |
| large_packet_ratio| NUMERIC |     | 大数据包的比例  |
| package_size      | NUMERIC |     | 一次传输的通信数据包大小，单位：MB|
| count             | NUMERIC |     | 通信传输次数  |
| total_duration    | NUMERIC |     | 数据传输总耗时  |
| step              | TEXT    |     | 算子所属的step，例：step12  |
| type              | TEXT    |     | 算子类型，包含：collective，p2p  |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/11 | 330首次上线 |

### CommAnalyzerTime
| 字段名  | 类型  | 索引  | 含义 |
| --------------------- | ------------ | ------------ | ------------ |
| hccl_op_name          | TEXT |   | HCCL大算子名，例：hcom_broadcast__303_1_1  |
| group_name            | TEXT  |   | 通信域hashId，例：3915571125887837303  |
| start_timestamp       | NUMERIC  |   | 开始时间，单位：us  |
| elapse_time           | NUMERIC  |   | 经过的时间，单位：ms  |
| transit_time          | NUMERIC |   | 传输时间，单位：ms  |
| wait_time             | NUMERIC  |   | 等待时间，单位：ms  |
| synchronization_time  | NUMERIC   |   | 同步时间，单位：ms  |
| idle_time             | NUMERIC  |   | 空闲时间，单位：ms  |
| step                  | TEXT  |   | 算子所属的step，例：step12  |
| type                  | TEXT  |   | 算子类型，包含：collective，p2p  |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/11 | 330首次上线 |

### CommAnalyzerMatrix
| 字段名          | 类型     | 索引  | 含义 |
| -------------- | --------| ------------ | ------------ |
| hccl_op_name   | TEXT    |   | 矩阵分析后的精简算子名，例：send-top1  |
| group_name     | TEXT    |   | 通信域hashId，例：3915571125887837303  |
| src_rank       | TEXT    |   | 发送数据的rankId，例：0  |
| dst_rank       | TEXT    |   | 接受数据的rankId，例：1  |
| transport_type | TEXT    |   | 传输类型，包含：LOCAL、SDMA、RDMA  |
| transit_size   | NUMERIC |   | 传输的数据量，单位：MB  |
| transit_time   | NUMERIC |   | 传输耗时，单位：ms  |
| bandwidth      | NUMERIC |   | 带宽，单位：GB/s  |
| step           | TEXT    |   | 算子所属的step，例：step12  |
| type           | TEXT    |   | 算子类型，包含：collective，p2p  |
| op_name        | TEXT    |   | 算子的原始名字，例：hcom_broadcast__303_1_1  |

变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/11 | 330首次上线 |

### StepTraceTime
| 字段名          | 类型     | 索引  | 含义 |
| -------------- | ------------ | ------------ | ------------ |
| deviceId       | NUMERIC  |   | deviceId，例：0  |
| step           | TEXT    |    | step编号，例：12  |
| computing      | NUMERIC  |   | 计算的时间，单位：ms  |
| communication  | NUMERIC  |   | 通信的时间，单位：ms  |
| overlapped  | NUMERIC  |   | 同时进行计算和通信的时间，单位：ms  |
| communication_not_overlapped  | NUMERIC  |   | 纯用于通信的时间，单位：ms  |
| free  | NUMERIC  |   | 空闲的时间，单位：ms  |
| stage  | NUMERIC   |   | step内除去接收数据的时间，单位：ms  |
| bubble  | NUMERIC  |   | step内用于接收数据的时间，单位：ms  |
| communication_not_overlapped_and_exclude_receive  | NUMERIC  |   | 纯用于通信的时间减去用于接收数据的时间，单位：ms  |


变更记录：

| 日期       | 内容      |
|----------|---------|
| 2024/3/11 | 330首次上线 |
| 2025/6/4  | 增加deviceId |

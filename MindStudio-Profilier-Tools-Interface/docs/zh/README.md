# msPTI工具用户指南

## 简介
提供msPTI各种接口的使用用例，各个目录对应不同用例，供用户理解使用msPTI接口。

## 使用前准备

**环境准备**

详情请参见[环境和依赖](../../README.md#环境和依赖)。

**约束**

msPTI工具不可与任何其他性能数据采集工具同时使用，否则会导致采集的数据丢失。

## msPTI接口用例

**功能说明**

msPTI当前提供使用C开发的一套API以及将C API的功能作为底层逻辑封装的一套Python的API。 

- Callback Api

| 样例                                              | 说明                                                                                                                                              | 支持产品型号            |
|-------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|-----------------|
| [callback_domain](../../samples/callback_domain) | 1. 展示Callback API功能，可以通过msptiEnableDomain，在runtime API的前后执行Callback操作。                                                                          | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [callback_mstx](../../samples/callback_mstx)                 | 1. 展示Callback与mstx接口相结合功能, 使用Callback API和mstx打点功能，在runtime的Launch Kernel前后打点，采集算子数据。<br/> 2. 演示Callback中userdata用法，用户可以通过userdata透传配置或者部分运行参数。 | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |

- Activity API

| 样例                                                         | 说明                                                                                                                                                              | 支持产品型号            |
|------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------|
| [mspti_activity](../../samples/mspti_activity)                         | 1. 展示Activity API接口的基本功能，样例展示如何采集Kernel和Memory等数据。<br/> 2. 演示Activity API的基本运行，讲述Activity API的基本使用，包括Activity Buffer内存分配，Buffer消费等逻辑。                           | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [mspti_correlation](../../samples/mspti_correlation)                   | 1. 展示Activity API接口的基本功能，展示如何通过correlationId字段将API和Kernel数据做关联。<br/> 2. 演示runtime API下发与Kernel实际执行数据的关联，关联后可以将算子的下发和执行一一对应，方便分析性能瓶颈。                            | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [mspti_external_correlation](../../samples/mspti_external_correlation) | 1. 展示MSPTI External Correlation功能。<br/>2. 演示msptiActivityPopExternalCorrelationId和msptiActivityPushExternalCorrelationId两接口使用方法，用户可以通过接口将各种API关联到一起，方便回溯函数的调用栈。 | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [mspti_hccl_activity](../../samples/mspti_hccl_activity)               | 1. 展示Activity API接口的基本功能，样例展示如何通过Hccl开关采集通信数据。                                                                                                                  | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [mspti_mstx_activity_domain](../../samples/mspti_mstx_activity_domain) | 1. 展示MSPTI控制mstxDomain功能，通过开关控制打点数据是否采集。<br/> 2. 用户可以通过MSPTI开关实时开关采集打点，减小性能损耗。                                                                                  | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |

- Python API

| 样例                                           | 说明                                                           | 支持产品型号            |
|----------------------------------------------|--------------------------------------------------------------|-----------------|
| [python_monitor](../../samples/python_monitor)           | 1.展示Monitor基本使用方式，通过KernelMonitor、HcclMonitor获取计算算子和通信算子的耗时。 | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |
| [python_mstx_monitor](../../samples/python_mstx_monitor) | 1. 展示MstxMonitor基本使用方式，用户可以通过Mstx打点采集对应算子（如matmul）耗时。        | **Atlas A2 训练系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件Atlas A3 训练系列产品/Atlas A3 推理系列产品** |

**注意事项**

- Python用例依赖于torch以及torch_npu，使用前请确认已安装。

**使用示例**
1. 将目录切换到对应用例下。
2. 请在使用前执行`source <ASCEND_HOME_PATH>/set_env.sh`以保证用例正常执行，ASCEND_HOME_PATH是CANN包的实际路径。命令示例如下：
    ```shell
    source /usr/local/Ascend/ascend-toolkit/set_env.sh
    ```
3. 执行对应用例目录下的sample_run.sh即可。

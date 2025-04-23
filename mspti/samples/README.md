## 介绍
本目录包含有mspti各种接口的使用用例, 各个文件夹对应不同用例, 供用户理解使用mspti接口. 目录以及用例具体说明如下

| 样例                                                         | 说明                                                                                                                                 | 支持产品型号            |
|------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|-----------------|
| [callback_domain_mstx](./callback_domain_mstx)             | 1. 用例展示callback api功能，可以通过enableCallbackDomain，在一批接口的前后执行callback操作, <br/>2. 使用enableCallbackDomain后可以在一类接口前后进行用户自定义操作，便于打点或者调用栈采集 | Atlas A2,A3系列产品 |
| [callback_mstx](./callback_mstx)                           | 1. 展示callback与mstx接口相结合功能, 使用callback api和mstx打点功能，在launchKernel前后打点，采集算子数据。<br>2. 演示callback中userData用法，用户可以通过userData透传配置或者部分运行参数 | Atlas A2,A3系列产品 |
| [mspti_activity](./mspti_activity)                         | 1. 展示msptiActivity接口的基本功能，用例展示如何采集kernel和mem等数据。<br>2. 演示activity api的基本运行，讲述activity接口的基本使用，包括buffer内存分配，buffer消费等逻辑              | Atlas A2,A3系列产品 |
| [mspti_correlation](./mspti_correlation)                   | 1. 展示msptiActivity接口的基本功能，展示如何通过correlationId字段将API和kernel数据做关联 <br>2. 用例演示基本api下发与kernel实际执行数据的关联，关联后可以讲算子的下发和执行一一对应，方便分析性能瓶颈     | Atlas A2,A3系列产品 |
| [mspti_external_correlation](./mspti_external_correlation) | 1. 展示mspti external correlation功能。<br>2. 用例演示pop和push两接口使用方法，用户可以通过接口将各种api关联到一起，方便回溯函数的调用栈                                        | Atlas A2,A3系列产品 |
| [mspti_hccl_activity](./mspti_hccl_activity)               | 1.展示msptiActivity接口的基本功能，用例展示如何通过hccl开关采集通信数据。                                                                                     | Atlas A2,A3系列产品 |
| [mspti_mstx_activity_domain](./mspti_mstx_activity_domain)             | 1. 展示mspti控制mstxDomain功能，通过开关控制打点数据是否采集，<br> 2.用户可以通过pti开关实时开关采集打点, 减小性能损耗。                                                        | Atlas A2,A3系列产品 |
| [python_monitor](./python_monitor)                         | 1.展示Monitor基本使用方式，通过msptiMonitor获取计算算子和通信算子的耗时                                                                                     | Atlas A2,A3系列产品 |
| [python_mstx_monitor](./python_mstx_monitor)                    | 展示mstxMonitor基本使用方式，用户可以通过mstx打点采集对应算子（如matmul）耗时                                                                                  | Atlas A2,A3系列产品 |

## 构建用例执行
1. 将目录切换到对应用例下
2. 请在使用前执行source {ASCEND_HOME}/set_env.sh以保证用例正常执行, ascend-toolkit的默认安装路径为/usr/local/Ascend/ascend-tookit
3. 执行对应用例目录下的sample_run.sh即可

## 用例约束
1. 本用例依赖于ascend-toolkit, 使用前请确认已安装。
2. python用例依赖于torch以及torch_npu，使用前请确认已安装
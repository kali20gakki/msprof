# msPTI工具

## 简介
本文通过提供msPTI样例集的方式，供用户理解使用msPTI工具。

## AI处理器支持情况

**AI处理器支持情况**

>![](./figures/icon-note.gif) **说明：** 
>AI处理器与昇腾产品的对应关系，请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

| AI处理器类型 | 是否支持 |
| ------------ | :------: |
| Ascend 910C  |    √     |
| Ascend 910B  |    √     |
| Ascend 310B  |    √     |
| Ascend 310P  |    ×     |
| Ascend 910   |    ×     |

## 使用前准备

**环境准备**

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 软件环境请参见《[CANN 软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_quick.html?Mode=PmIns&InstallType=local&OS=openEuler&Software=cannToolKit)》安装昇腾设备开发或运行环境，即toolkit软件包。

以上环境依赖请根据实际环境选择适配的版本。

**约束**

msPTI工具不可与任何其他性能数据采集工具同时使用，否则会导致采集的数据丢失。

## msPTI样例集

本节提供msPTI各种接口的使用样例，供用户理解使用msPTI接口，样例具体说明及目录如下。

**注意事项<a name="zh-cn_topic_0000002257378834_section9293111211496"></a>**

- 请确保安装Ascend-cann-toolkit包。

  参见《CANN 软件安装指南》。

- msPTI Python API部分的样例依赖于PyTorch框架和torch\_npu插件，请确保安装。

  参见《Ascend Extension for PyTorch 软件安装指南》中的“安装PyTorch”。

- Python用例依赖于torch以及torch_npu，使用前请确认已安装。

**构建样例执行<a name="zh-cn_topic_0000002257378834_section6122533557"></a>**

1. 安装CANN软件后，使用CANN运行用户进行编译、运行时，需要以CANN运行用户登录环境，执行source $_\{install\_path\}_/set\_env.sh命令设置环境变量。其中$\{install\_path\}为CANN软件的安装目录，例如：/usr/local/Ascend/ascend-toolkit。

2. 进入样例目录。

   msPTI样例代码集成在Ascend-cann-toolkit包中，路径为$\{INSTALL\_DIR\}/tools/mspti/samples。

   $\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。若安装的Ascend-cann-toolkit软件包，以root安装举例，则安装后文件存储路径为：/usr/local/Ascend/ascend-toolkit/latest。

   示例如下：

   ```
   cd ${INSTALL_DIR}/tools/mspti/samples/callback_domain
   ```

3. 执行对应样例目录下的sample\_run.sh。

   ```
   bash sample_run.sh
   ```

下表为当前提供的样例介绍：

- Callback API

  | 样例                                             | 说明                                                         | AI处理器支持情况            |
  | ------------------------------------------------ | ------------------------------------------------------------ | --------------------------- |
  | [callback_domain](../../samples/callback_domain) | 1. 展示Callback API功能，可以通过msptiEnableDomain，在runtime API的前后执行Callback操作。 | Ascend 910B<br/>Ascend 910C |
  | [callback_mstx](../../samples/callback_mstx)     | 1. 展示Callback与mstx接口相结合功能, 使用Callback API和mstx打点功能，在runtime的Launch Kernel前后打点，采集算子数据。<br/> 2. 演示Callback中userdata用法，用户可以通过userdata透传配置或者部分运行参数。 | Ascend 910B<br/>Ascend 910C |

- Activity API

  | 样例                                                         | 说明                                                         | AI处理器支持情况            |
  | ------------------------------------------------------------ | ------------------------------------------------------------ | --------------------------- |
  | [mspti_activity](../../samples/mspti_activity)               | 1. 展示Activity API接口的基本功能，样例展示如何采集Kernel和Memory等数据。<br/> 2. 演示Activity API的基本运行，讲述Activity API的基本使用，包括Activity Buffer内存分配，Buffer消费等逻辑。 | Ascend 910B<br/>Ascend 910C |
  | [mspti_correlation](../../samples/mspti_correlation)         | 1. 展示Activity API接口的基本功能，展示如何通过correlationId字段将API和Kernel数据做关联。<br/> 2. 演示runtime API下发与Kernel实际执行数据的关联，关联后可以将算子的下发和执行一一对应，方便分析性能瓶颈。 | Ascend 910B<br/>Ascend 910C |
  | [mspti_external_correlation](../../samples/mspti_external_correlation) | 1. 展示MSPTI External Correlation功能。<br/>2. 演示msptiActivityPopExternalCorrelationId和msptiActivityPushExternalCorrelationId两接口使用方法，用户可以通过接口将各种API关联到一起，方便回溯函数的调用栈。 | Ascend 910B<br/>Ascend 910C |
  | [mspti_hccl_activity](../../samples/mspti_hccl_activity)     | 1. 展示Activity API接口的基本功能，样例展示如何通过Hccl开关采集通信数据。 | Ascend 910B<br/>Ascend 910C |
  | [mspti_mstx_activity_domain](../../samples/mspti_mstx_activity_domain) | 1. 展示MSPTI控制mstxDomain功能，通过开关控制打点数据是否采集。<br/> 2. 用户可以通过MSPTI开关实时开关采集打点，减小性能损耗。 | Ascend 910B<br/>Ascend 910C |

- Python API

  | 样例                                                     | 说明                                                         | AI处理器支持情况            |
  | -------------------------------------------------------- | ------------------------------------------------------------ | --------------------------- |
  | [python_monitor](../../samples/python_monitor)           | 1.展示Monitor基本使用方式，通过KernelMonitor、HcclMonitor获取计算算子和通信算子的耗时。 | Ascend 910B<br/>Ascend 910C |
  | [python_mstx_monitor](../../samples/python_mstx_monitor) | 1. 展示MstxMonitor基本使用方式，用户可以通过Mstx打点采集对应算子（如matmul）耗时。 | Ascend 910B<br/>Ascend 910C |


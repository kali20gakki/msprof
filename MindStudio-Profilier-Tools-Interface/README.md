# 📖MindStudio-Profilier-Tools-Interface

![python](https://img.shields.io/badge/python-3.8|3.9|3.10-blue)
![platform](https://img.shields.io/badge/platform-Linux-yellow)

## 📖简介

msPTI工具（msPTI，MindStudio Profiling Tool Interface）是MindStudio针对Ascend设备提出的一套Profiling API，用户可以通过msPTI构建针对NPU应用程序的工具，用于分析应用程序的性能。

msPTI为通用场景接口，使用msPTI API开发的Profiling分析工具可以在各种框架的推理训练场景生效。

msPTI主要包括以下功能：
- Tracing：在msPTI中Tracing是指CANN应用程序执行启动CANN活动的时间戳和附加信息的收集，如CANN API、Kernel、内存拷贝等。通过了解程序运行耗时，识别CANN代码的性能问题。可以使用Activity API和Callback API收集Tracing信息。
- Profiling：在msPTI中Profiling是指单独收集一个或一组Kernel的NPU性能指标。

## 🗂️目录结构

目录如下：

```sh
└── docs                      // 文档
	└── zh                    // 中文文档
└── csrc                      // 使用C开发的一套API
└── mspti                     // 将C API的功能作为底层逻辑封装的一套Python的API
└── samples                   // 工具样例存放目录
└── scripts                   // 存放whl包构建脚本，run包编译、安装相关脚本，UT运行、覆盖率脚本等
└── test                      // 测试部分，存放UT代码
└── README.md                 // 整体仓说明文档
```

## 🏷️版本说明

包含msPTI的软件版本配套关系和软件包下载以及每个版本的特性变更说明，详情请参见《[版本说明](docs/zh/release_notes.md)》。

## ⚙️环境部署

### 环境和依赖

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 软件环境请参见《[CANN 软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_quick.html?Mode=PmIns&InstallType=local&OS=openEuler&Software=cannToolKit)》安装昇腾设备开发或运行环境，即toolkit软件包。

以上环境依赖请根据实际环境选择适配的版本。

### 🛠️工具安装

安装msPTI工具，详情请参见《[msPTI工具安装指南](docs/zh/mspti_install_guide.md)》。

## 🧰功能介绍

使用msPTI工具，详情请参见《[msPTI工具用户指南](docs/zh/README.md)》。

## ❗免责声明

- 本工具仅供调试和开发之用，使用者需自行承担使用风险，并理解以下内容：

  - [X] 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防泄露或不必要的信息泄露。
  - [X] 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
  - [X] 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于由于输入命令行不当所导致的问题，本工具及其开发者概不负责。
- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
- 在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

## 🔒安全声明

描述msPTI产品的安全加固信息、公网地址信息及通信矩阵等内容。详情请参见《[msPTI工具安全声明](./docs/zh/security_statement.md)》。

## 🔑License
msPTI产品许可证。详见[LICENSE](./LICENSE.md)文件。

## 💬建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交issues，我们会尽快回复。感谢您的支持。

## ❤️致谢

msPTI由华为公司的下列部门联合贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献msPTI！
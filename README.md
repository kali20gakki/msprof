# 📖MindStudio Profiler

![python](https://img.shields.io/badge/python-3.8|3.9|3.10-blue)
![platform](https://img.shields.io/badge/platform-Linux-yellow)

## 📖简介

MindStudio Profiler（简称msProf）是性能调试命令行工具。msProf提供了AI任务运行性能数据、昇腾AI处理器系统数据等性能数据的采集和解析功能，这些功能侧重不同的训练或推理场景，可以定位模型训练或推理中的性能问题。

## 🗂️目录结构

关键目录如下，详细目录介绍参见[项目目录](docs/zh/dir_structure.md)。

```sh
└── .gitsubmodule             // 声明依赖的submodule文件
└── docs                      // 文档
    └── zh                    // 中文文档
└── thirdparty                // 三方依赖存放目录
└── example                   // 工具样例存放目录
    ├── README.md             // 工具样例说明
└── src                       // 基础能力
    ├── parse                 // 数据解析目录
└── scripts                   // 存放run包安装、升级相关脚本
└── test                      // 测试部分，存放覆盖率统计脚本
└── build                     // 构建目录
    ├── build.sh              // 构建脚本
└── README.md                 // 整体仓说明文档

```
## 🏷️版本说明

包含msProf的软件版本配套关系和软件包下载以及每个版本的特性变更说明。

## ⚙️环境部署

### 环境和依赖

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 软件环境请参见《[CANN 软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_quick.html?Mode=PmIns&InstallType=local&OS=openEuler&Software=cannToolKit)》安装昇腾设备开发或运行环境，即toolkit软件包。

以上环境依赖请根据实际环境选择适配的版本。

###  🛠️工具安装

安装msProf工具，详情请参见《[msProf工具安装指南](docs/zh/msprof_install_guide.md)》。

## 🚀 [快速入门](docs/zh/quick_start.md)

离线推理场景推荐使用msProf命令采集，请参见[离线推理场景性能分析快速入门](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/devaids/Profiling/atlasprofiling_16_0005.html)。如果当前环境未安装Ascend-cann-toolkit包，则无法使用msProf命令。


## 🧰 功能介绍

### [msProf性能数据采集]()

通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行采集。

### [msProf性能数据解析](docs/zh/msprof_profile_data_parsing_instruct.md)

通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行解析。

## ❗免责声明

- 本工具仅供调试和开发之用，使用者需自行承担使用风险，并理解以下内容：

  - [X] 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防泄露或不必要的信息泄露。
  - [X] 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
  - [X] 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于由于输入命令行不当所导致的问题，本工具及其开发者概不负责。
- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
- 在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

## 🔒 安全声明

描述msProf产品的安全加固信息、公网地址信息及通信矩阵等内容。详情请参见[msProf工具安全声明](./docs/zh/security_statement.md)。

## 💬建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交issues，我们会尽快回复。感谢您的支持。

## ❤️致谢

msProf由华为公司的下列部门联合贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献msProf！
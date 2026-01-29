# MindStudio Profiler

## 简介

MindStudio Profiler（模型调优工具，msProf）提供了AI任务运行性能数据、昇腾AI处理器系统数据等性能数据的采集和解析功能，这些功能侧重不同的训练或推理场景，可以定位模型训练或推理中的性能问题。

## 目录结构

关键目录如下，详细目录介绍参见[项目目录](docs/zh/dir_structure.md)。

```sh
└── .gitcode                  // 存放仓库中的元数据
└── analysis                  // 数据解析目录
└── build                     // 构建目录
    ├── build.sh              // 构建脚本
└── cmake                     // 存放解析C化部分cmake文件
└── docs                      // 文档
    └── zh                    // 中文文档
└── samples                   // 工具样例存放目录
    ├── README.md             // 工具样例说明
└── scripts                   // 存放run包安装、升级相关脚本
└── test                      // 测试部分，存放覆盖率统计脚本
└── README.md                 // 整体仓说明文档

```
## 环境部署

### 环境和依赖

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 工具的运行需要提前获取并安装CANN开源版本，当前CANN开源版本正在发布中，敬请期待。

以上环境依赖请根据实际环境选择适配的版本。

###  工具安装

安装msProf工具，详情请参见《[msProf工具安装指南](docs/zh/msprof_install_guide.md)》。


## 快速开始


## 功能介绍

1. [msProf性能数据采集](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/850alpha002/devaids/Profiling/atlasprofiling_16_0010.html)

   通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行采集。

2. [msProf性能数据解析](docs/zh/msprof_parsing_instruct.md)

   通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行解析。

## 贡献指导
介绍如何向msProf反馈问题、需求以及为msProf贡献的代码开发流程，具体请参见[为MindStudio Profiler贡献](CONTRIBUTING.md)。

## 联系我们

<div>
  <a href="https://raw.gitcode.com/kali20gakki1/Imageshack/raw/main/CDC0BEE2-8F11-477D-BD55-77A15417D7D1_4_5005_c.jpeg">
    <img src="https://img.shields.io/badge/WeChat-07C160?style=for-the-badge&logo=wechat&logoColor=white"></a>
</div>

## 安全声明

描述msProf产品的安全加固信息、公网地址信息及通信矩阵等内容。详情请参见[msProf安全声明](./docs/zh/security_statement.md)。

## 免责声明

### 致msProf使用者

- 本工具仅供调试和开发使用，使用者需自行承担使用风险，并理解以下内容：
  - 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防信息泄露。
  - 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
  - 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于输入命令行不当所导致的问题，本工具及其开发者概不负责。
- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
- 在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

### 致数据所有者

如果您不希望您的模型或数据集等信息在msProf中被提及，或希望更新msProf中有关的描述，请在GitCode提交Issue，我们将根据您的Issue要求删除或更新您相关描述。衷心感谢您对msProf的理解和贡献。

## License

msProf产品的使用许可证，具体请参见[LICENSE](LICENSE)文件。

msProf工具docs目录下的文档适用CC-BY 4.0许可证，具体请参见[LICENSE](./docs/LICENSE)。

## 致谢

msProf由华为公司的下列部门联合贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献msProf！ 
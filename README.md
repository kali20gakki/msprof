# 📖MindStudio Profiler

## 📖简介

MindStudio Profiler（简称msProf）是性能调试命令行工具。msProf提供了AI任务运行性能数据、昇腾AI处理器系统数据等性能数据的采集和解析功能，这些功能侧重不同的训练或推理场景，可以定位模型训练或推理中的性能问题。

## 🗂️目录结构

关键目录如下，详细目录介绍参见[项目目录](docs/zh/dir_structure.md)。

```sh
└── .gitmodules             // 声明依赖的modules文件
└── docs                      // 文档
    └── zh                    // 中文文档
└── thirdparty                // 第三方依赖存放目录
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

msProf的版本说明包含msProf的软件版本配套关系和软件包下载以及每个版本的特性变更说明，详情请参见《[版本说明](docs/zh/release_notes.md)》。

## ⚙️环境部署

### 环境和依赖

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 软件环境请参见《[CANN 软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_quick.html?Mode=PmIns&InstallType=local&OS=openEuler&Software=cannToolKit)》安装昇腾设备开发或运行环境，即Toolkit开发套件包和ops算子包。

以上环境依赖请根据实际环境选择适配的版本。

###  🛠️工具安装

安装msProf工具，详情请参见《[msProf工具安装指南](docs/zh/msprof_install_guide.md)》。

## 🚀 快速入门

离线推理场景推荐使用msProf命令采集，请参见[离线推理场景性能分析快速入门](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/devaids/Profiling/atlasprofiling_16_0005.html)。如果当前环境未安装Ascend-cann-toolkit包，则无法使用msProf命令。


## 🧰 功能介绍

1. msProf性能数据采集

   通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行采集。

2. [msProf性能数据解析](docs/zh/msprof_parsing_instruct.md)

   通过msProf命令对AI任务运行性能数据、昇腾AI处理器系统数据进行解析。


## 🔒 安全声明

描述msProf产品的安全加固信息、公网地址信息及通信矩阵等内容。详情请参见[msProf工具安全声明](./docs/zh/security_statement.md)。

## ❗免责声明

### 致msProf使用者

- 本工具仅供调试和开发之用，使用者需自行承担使用风险，并理解以下内容：
  - 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防信息泄露。
  - 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
  - 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于由于输入命令行不当所导致的问题，本工具及其开发者概不负责。
- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
- 在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

### 致数据所有者

如果您不希望您的模型或数据集等信息在msProf中被提及，或希望更新msProf中有关的描述，请在GitCode提交issue，我们将根据您的issue要求删除或更新您相关描述。衷心感谢您对msProf的理解和贡献。

## License

msProf产品的使用许可证，具体请参见[License](License)文件。

msProf工具docs目录下的文档适用CC-BY 4.0许可证，具体请参见[License](./docs/LICENSE)。

## 贡献声明

1. 提交错误报告：如果您在msProf中发现了一个不存在安全问题的漏洞，请在msProf仓库中的Issues中搜索，以防该漏洞已被提交，如果找不到漏洞可以创建一个新的Issues。如果发现了一个安全问题请不要将其公开，请参阅安全问题处理方式。提交错误报告时应该包含完整信息。
2. 安全问题处理：本项目中对安全问题处理的形式，请通过邮箱通知项目核心人员确认编辑.
3. 解决现有问题：通过查看仓库的Issues列表可以发现需要处理的问题信息, 可以尝试解决其中的某个问题
4. 如何提出新功能：请使用Issues的Feature标签进行标记，我们会定期处理和确认开发。
5. 开始贡献：
   1. Fork本项目的仓库
   2. Clone到本地
   3. 创建开发分支
   4. 本地自测，提交前请通过所有的已经单元测试，以及为您要解决的问题新增单元测试。
   5. 提交代码
   6. 新建Pull Request
   7. 代码检视，您需要根据评审意见修改代码，并再次推送更新。此过程可能会有多轮。
   8. 当您的PR获得足够数量的检视者批准后，Committer会进行最终审核。
   9. 审核和测试通过后，CI会将您的PR合并入到项目的主干分支。

## 💬建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交issues，我们会尽快回复。感谢您的支持。

## ❤️致谢

msProf由华为公司的下列部门联合贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献msProf！ 
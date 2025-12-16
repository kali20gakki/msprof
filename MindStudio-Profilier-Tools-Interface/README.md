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

## 🔗API参考

- [msPTI C API参考](docs/zh/c_api/README.md)

  介绍msPTI工具C语言的API。

- [msPTI Python API参考](docs/zh/python_api/README.md)

  介绍msPTI工具Python语言的API。

## 📍免责声明
**致msPTI使用者**
1. msPTI提供的所有内容仅供您用于非商业目的。
2. 对于msPTI测试用例以及示例文件中所涉及的各模型和数据集，平台仅用于功能测试，华为不提供任何模型权重和数据集，如您使用这些数据进行训练，请您特别注意应遵守对应模型和数据集的License，如您因使用这些模型和数据集而产生侵权纠纷，华为不承担任何责任。
3. 如您在使用msPTI过程中，发现任何问题（包括但不限于功能问题、合规问题），请在GitCode提交issue，我们将及时审视并解决。
4. msPTI功能依赖的第三方开源软件，均由第三方社区提供和维护，因第三方开源软件导致的问题的修复依赖相关社区的贡献和反馈。您应理解，msPTI仓库不保证对第三方开源软件本身的问题进行修复，也不保证会测试、纠正所有第三方开源软件的漏洞和错误。

**致数据所有者**

如果您不希望您的模型或数据集在msPTI中被提及，或希望更新msPTI中有关的描述，请在GitCode提交issue，我们将根据您的issue要求删除或更新您相关描述。衷心感谢您对msPTI的理解和贡献。

## 🔒安全声明

描述msPTI产品的安全加固信息、公网地址信息及通信矩阵等内容。详情请参见《[msPTI工具安全声明](./docs/zh/security_statement.md)》。

## ✒️License

msPTI产品许可证，详见[LICENSE](./LICENSE.md)文件。

## 📣贡献声明
1. **提交错误报告**：如果您在msPTI中发现了一个不存在安全问题的漏洞，请在msPTI仓库中的Issues中搜索，以防该漏洞已被提交，如果找不到漏洞可以创建一个新的Issues。如果发现了一个安全问题请不要将其公开，请参阅安全问题处理方式。提交错误报告时应该包含完整信息。
2. **安全问题处理**：本项目中对安全问题处理的形式，请通过邮箱通知项目核心人员确认编辑。
3. **解决现有问题**：通过查看仓库的Issues列表可以发现需要处理的问题信息，可以尝试解决其中的某个问题。
4. **如何提出新功能**：请使用Issues的Feature标签进行标记，我们会定期处理和确认开发。
5. **开始贡献：**
   1. Fork 本项目的仓库。
   2. Clone到本地。
   3. 创建开发分支。
   4. 本地自测，提交前请通过所有的已经单元测试，以及为您要解决的问题新增单元测试。
   5. 提交代码。
   6. 新建 Pull Request。
   7. 代码检视，您需要根据评审意见修改代码，并再次推送更新。此过程可能会有多轮。
   8. 当您的PR获得足够数量的检视者批准后，Committer会进行最终审核。
   9. 审核和测试通过后，CI会将您的PR合并入项目的主干分支。

## 💬建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交issues，我们会尽快回复。感谢您的支持。

## 🤝致谢

msPTI由华为公司的下列部门联合贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献msPTI！
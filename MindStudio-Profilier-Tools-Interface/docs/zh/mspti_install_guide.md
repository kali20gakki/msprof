# msPTI工具安装指南
## 📝安装说明
本文主要介绍msPTI工具的安装方式。

## ⚠️安装前准备
- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 软件环境请参见《[CANN 软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_quick.html?Mode=PmIns&InstallType=local&OS=openEuler&Software=cannToolKit)》安装配套版本的CANN Toolkit开发套件包和ops算子包并配置CANN环境变量。

## ✅安装run包

### 源码编译
执行如下命令编译run包：
```bash
git clone https://gitcode.com/Ascend/mspti.git
cd mspti
bash scripts/build.sh [<version>]
```
编译完成后，会在mspti/output目录下生成msPTI工具的run包，run包名称格式为`Ascend-mindstudio-mspti_<version>_linux-<arch>.run`。
上述编译命令中的version参数即为软件包名称中的version，表示该run包的版本号，默认为“none”。
run包中的arch表示系统架构，根据实际运行系统自动适配。

### 安装步骤

1. 增加对run包的可执行权限。
    ```shell
    chmod +x Ascend-mindstudio-mspti_<version>_linux-<arch>.run
    ```
2. 安装run包。
    ```shell
    ./Ascend-mindstudio-mspti_<version>_linux-<arch>.run --install
    ```
    安装命令支持`--install-path=<path>`等参数，具体使用方式请参见[参数说明](#参数说明)。

    执行安装命令时，会自动执行--check参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。
    ```text
    Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
    ```
    安装完成后，若显示如下信息，则说明软件安装成功：
    ```text
    MindStudio-Profiler-Tools-Interface package install success.
    ```

## 附录
### 参数说明

msPTI工具run包的安装命令可配置如下参数：

| 参数     | 可选/必选 | 说明 |
| --------| -------  |------|
| --install | 必选 | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。 |
| --install-path | 可选 | 安装路径。须和CANN包安装时指定路径保持一致，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br> - root用户：“/usr/local/Ascend”。<br>- 非root用户：“\${HOME}/Ascend”，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选 | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msPTI运行业务，但该参数存在安全风险，请谨慎使用。 |

安装run包还可指定其他参数，具体可通过./xxx.run --help命令查看。

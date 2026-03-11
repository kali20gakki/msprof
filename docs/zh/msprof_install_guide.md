# msProf工具安装指南

## 安装说明

本文档主要介绍msProf工具的安装方式。

## 安装前准备

完成安装配套版本的CANN Toolkit开发套件包和ops算子包并配置CANN环境变量，具体请参见《[CANN软件安装指南](https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst/instg/instg_0000.html?Mode=PmIns&InstallType=netconda&OS=openEuler)》。

## 安装run包

### 源码编译

#### 编译采集、解析run包

采集、解析run包包含msProf的采集和解析功能。

1. 安装依赖。

   msProf的采集源码在[runtime](https://gitcode.com/cann/runtime)仓和[oam-tools](https://gitcode.com/cann/oam-tools)仓中，msProf编采集、解析包会先编译runtime和oam-tools两个仓的代码，runtime仓在编译时需要指定一定版本的依赖，具体如下。
   - python >= 3.9.0
   - pip3
   - gcc >= 7.3.0, <= 13
   - cmake >= 3.16.0
   - ccache
   - autoconf
   - gperf
   - libtool

   runtime仓编译前置依赖详细介绍请参见[runtime仓编译依赖](https://gitcode.com/cann/runtime/blob/master/README.md#%E5%89%8D%E6%8F%90%E6%9D%A1%E4%BB%B6)，可直接复制[install_deps.sh](https://gitcode.com/cann/runtime/blob/master/install_deps.sh)脚本到编包环境执行。
   
2. 下载源码。

    ```shell
    git clone https://gitcode.com/Ascend/msprof.git
    ```

3. 编译run包。
   
   操作方式如下：

      ```shell
       cd msprof
       # 下载三方依赖包
       bash scripts/download_thirdparty.sh
       # 编译采集包
       bash build/build.sh --mode=all --version=<version>
      ```
   
   编译命令支持通过--mode参数，分别编译包含msProf采集和解析功能的软件包或仅包含msProf解析功能的软件包，更多编译参数详细介绍请参见[编译run包参数说明](#编译run包参数说明)。
   
   编译完成后，会在msprof/output目录下生成msProf工具的run包，run包名称格式为`mindstudio-profiler_<version>_<arch>.run`。
    
   上述编译命令中的version参数即为软件包名称中的version，表示该run包的版本号，默认为“none”。
   
   run包中的arch表示系统架构，根据实际运行系统自动适配。
   
#### 编译解析run包

解析run包包含msProf的解析功能。

1. 安装依赖。

   编译前需要下载一定的依赖，具体如下。

   - sqlite3
   
      Ubuntu系统上安装SQLite3：
      
      ```shell
      sudo apt update
      sudo apt install sqlite3 libsqlite3-dev
      ```
      
      openEuler/CentOS系统上安装SQLite3：
      
      ```shell
      sudo yum install sqlite sqlite-devel
      ```

2. 下载源码。

    ```shell
    git clone https://gitcode.com/Ascend/msprof.git
    ```

3. 编译run包。

   操作方式如下：

      ```shell
       cd msprof
       # 下载三方依赖包
       bash scripts/download_thirdparty.sh
       # 编译解析包
       bash build/build.sh --mode=analysis --version=<version>
      ```

   编译命令支持通过--mode参数，分别编译包含msProf采集和解析功能的软件包或仅包含msProf采集功能以及仅包含msProf解析功能的软件包，更多编译参数详细介绍请参见[编译run包参数说明](#编译run包参数说明)。
   
   编译完成后，会在msprof/output目录下生成msProf工具的run包，run包名称格式为`mindstudio-profiler_<version>_<arch>.run`。
   
   上述编译命令中的version参数即为软件包名称中的version，表示该run包的版本号，默认为“none”。
   
   run包中的arch表示系统架构，根据实际运行系统自动适配。

### 安装步骤

1. 增加对软件包的可执行权限。

    ```shell
    chmod +x mindstudio-profiler_<version>_<arch>.run
    ```

2. 安装run包。
   
    ```shell
    ./mindstudio-profiler_<version>_<arch>.run --install
    ```
   
    安装命令支持--install-path等参数，详细介绍请参见[安装run包参数说明](#安装run包参数说明)。

    执行安装命令时，会自动执行--check参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。
    
    ```text
    Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
    ```

    安装完成后，若显示如下信息，则说明软件安装成功。

    ```text
    mindstudio-profiler package install success.
    ```

## 附录

### 编译run包参数说明

msProf工具run包的编译命令可配置如下参数。

| 参数                | 可选/必选 | 说明                                                                                                                                            |
|-------------------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| --build_type      | 可选    | 编包类型，可取值: <br/>&#8226; Release：编译出用于生产环境部署的软件包。<br/>&#8226; Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br/> 默认值为Release。               |
| --mode            | 可选    | 编包方式。可取值: <br/>&#8226; all：编译出包含msProf采集和解析功能的软件包。<br/>&#8226; collector：编译出仅包含msProf采集功能的软件包。<br/>&#8226; analysis：编译出仅包含msProf解析功能的软件包。<br/>默认值为analysis。 |
| --version | 可选    | 配置run包的版本号。<br/>默认值为none。                                                                                                                     |

### 安装run包参数说明

msProf工具run包的安装命令可配置如下参数。

| 参数     | 可选/必选 | 说明                                                                                                                                             |
| --------| -------  |------------------------------------------------------------------------------------------------------------------------------------------------|
| --install | 可选 | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。                                                                           |
| --uninstall | 可选 | 卸载软件包。可配置--install-path参数指定软件安装时的路径；不配置--install-path参数时，则直接卸载默认路径下的msprof。                                                                    |
| --install-path | 可选 | 安装路径。路径须指定到cann目录，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br>- root用户：`/usr/local/Ascend/cann`<br>- 非root用户：`${HOME}/Ascend/cann`，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选 | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。                                                                            |

安装run包还可指定其他参数，具体可通过./xxx.run --help命令查看。

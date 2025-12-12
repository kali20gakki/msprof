# msProf工具安装指南

## 安装说明
本文档主要介绍msProf工具的安装方式。

## 安装前准备
安装配套版本的CANN Toolkit开发套件包并配置CANN环境变量，具体请参见《CANN软件安装指南》中“选择安装场景”章节的“训练&推理&开发调试”场景。

## 安装run包

### 源码编译

当前源码编译仅支持linux版本为ubuntu20.04

1. 安装依赖

    下载并安装cmake、autoconf、gperf等编译依赖，命令如下。

    ```shell
      apt install -y cmake python3 python3-pip ccache autoconf gperf libtool libssl-dev
   ```
   
2. 下载源码。
    ```shell
    git clone https://gitcode.com/Ascend/msprof.git
    ```

3. 编译run包。
 
   编译命令支持通过--mode参数，分别编译包含msProf采集和解析功能的软件包或仅包含msProf采集功能以及仅包含msProf解析功能的软件包，更多编译参数详细介绍请参见[编译run包参数说明](#编译run包参数说明)。

   其中msProf采集功能采集的是原始性能数据（不可直接查看，需通过msProf解析工具解析出交付件查看）；msProf解析功能将原始性能数据进行解析，并导出可查看的交付件。
    
   操作方式如下：

   - 编译采集和解析包
       ```shell
       cd msprof
       # 下载三方依赖包
       bash scripts/download_thirdparty.sh
       # 编译采集、解析包
       bash build/build.sh --mode=all --version=[<version>]
       ```
        
   - 编译采集包
      ```shell
       cd msprof
       # 下载三方依赖包
       bash scripts/download_thirdparty.sh
       # 编译解析包
       bash build/build.sh --mode=collector --version=[<version>]
     ```
   - 编译解析包
      ```shell
       cd msprof
       # 下载三方依赖包
       bash scripts/download_thirdparty.sh
       # 编译解析包
       bash build/build.sh --version=[<version>]
       ```

     编译完成后，会在msprof/output目录下生成msProf工具的run包，run包名称格式为`Ascend-mindstudio-msprof_<version>_linux-<arch>.run`。
   
     上述编译命令中的version参数即为软包名称中的version，表示该run包的版本号，默认为“none”。
   
      run包中的arch表示系统架构，根据实际运行系统自动适配。

### 安装步骤

1. 增加对软件包的可执行权限。

    ```shell
    chmod +x Ascend-mindstudio-msprof_<version>_linux-<arch>.run
    ```

2. 安装run包。
    
    ```shell
    ./Ascend-mindstudio-msprof_<version>_linux-<arch>.run --install
    ```
   
    安装命令支持--install-path等参数，详细介绍请参见[安装run包参数说明](#安装run包参数说明)。

    执行安装命令时，会自动执行--check参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。
    
    ```text
    Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
    ```

    安装完成后，若显示如下信息，则说明软件安装成功。

    ```text
    MindStudio-Profilier package install success.
    ```
 
## 附录
### 编译run包参数说明
 
msProf工具run包的编译命令可配置如下参数。

| 参数                | 可选/必选 | 说明                                                                                                                                            |
|-------------------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| --build_type      | 可选    | 编包类型，可取值: <br/> - Release：编译出用于生产环境部署的软件包。<br/> - Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br/> 默认值为Release。                              |
| --mode            | 可选    | 编包方式。可取值: <br/> - all：编译出包含msProf采集和解析功能的软件包。<br/> - collector：编译出仅包含msProf采集功能的软件包。<br/> - analysis：编译出仅包含msProf解析功能的软件包。<br/> 默认值为analysis。 |
| --version | 可选    | 配置run包的版本号。<br/>默认值为none。                                                                                                                     |


### 安装run包参数说明
 
msProf工具run包的安装命令可配置如下参数。

| 参数     | 可选/必选 | 说明                                                                                                                                            |
| --------| -------  |-----------------------------------------------------------------------------------------------------------------------------------------------|
| --install | 必选 | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。                                                                          |
| --install-path | 可选 | 安装路径。须和CANN包安装时指定路径保持一致，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br> - root用户：“/usr/local/Ascend”。<br>- 非root用户：“\${HOME}/Ascend”，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选 | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。                                                                           |
 
安装run包还可指定其他参数，具体可通过./xxx.run --help命令查看。
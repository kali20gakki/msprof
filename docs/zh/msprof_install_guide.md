# msProf工具安装指南

本文档主要介绍msProf工具的两种安装方式，发布包安装和源码编译、安装。

## 1. 发布包安装 

msProf 工具已集成在 CANN 包中发布，可通过以下方式完成安装：

### 方式一：依据 CANN 官方文档安装

请参考<a href="https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst" target="_blank">《CANN 安装官方文档》</a>，
按文档逐步完成安装与配置。

### 方式二：使用 CANN 官方容器镜像

请访问<a href="https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884" target="_blank">《CANN 官方镜像仓库》</a>，
按仓库中的指引完成镜像拉取及容器启动。

## 2. 源码编译、安装

如需使用最新代码的功能，可下载本仓库代码，自行编译、打包并完成安装

### 2.1 编译环境准备

msProf工具源码编译依赖SQLite3，请执行以下命令完成安装，或确保当前环境已满足该依赖。

* Ubuntu系统上安装SQLite3：
      
   ```shell
   sudo apt update
   sudo apt install sqlite3 libsqlite3-dev
   ```
      
* openEuler/CentOS系统上安装SQLite3：

   ```shell
   sudo yum install sqlite sqlite-devel
   ```

### 2.2 克隆本仓库

```shell
git clone https://gitcode.com/Ascend/msprof.git
```

### 2.3 下载三方依赖

```shell
cd msprof
# 下载三方依赖包
bash scripts/download_thirdparty.sh
```

### 2.4 执行编译打包

`build/build.sh` 编译脚本支持通过 --mode 参数指定编译类型：

* all：编译全量 run 包（包含采集与解析功能） 
* analysis：编译解析 run 包（仅包含解析功能）

更多参数说明请参见[编译run包参数说明](#编译run包参数说明)。  

编译完成后，会在当前路径 `output` 目录下生成 run 包，名称格式为 `mindstudio-profiler_<version>_<arch>.run`。其中，`version` 为版本号，`arch` 为系统架构（根据实际运行系统自动适配）。

#### 方式一：编译msProf全量run包（推荐）

```shell
# 编译全量run包，包含msProf的采集和解析功能
bash build/build.sh --mode=all --version=<version>
```
   
#### 方式二：编译msProf解析run包

```shell
# 单独编译解析包
bash build/build.sh --mode=analysis --version=<version>
```

### 2.5 安装run包

run 包将生成在 `output` 目录下，执行以下命令为其添加可执行权限：

```shell
cd output
chmod +x mindstudio-profiler_<version>_<arch>.run
```
 
执行安装命令：

```shell
./mindstudio-profiler_<version>_<arch>.run --install
```
   
安装命令支持`--install-path`等参数，具体请参见[安装run包参数说明](#安装run包参数说明)。

执行安装命令时，会自动执行`--check`参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。
    
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
| --build_type      | 可选    | 编包类型，可取值: <br/>&#8226; Release：编译出用于生产环境部署的软件包。<br/>&#8226; Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br/> 默认值为Release。               |
| --mode            | 可选    | 编包方式。可取值: <br/>&#8226; all：编译出包含msProf采集和解析功能的软件包。<br/>&#8226; collector：编译出仅包含msProf采集功能的软件包。<br/>&#8226; analysis：编译出仅包含msProf解析功能的软件包。<br/>默认值为analysis。 |
| --version | 可选    | 配置run包的版本号。<br/>默认值为none。                                                                                                                     |

### 安装run包参数说明

msProf工具run包的安装命令可配置如下参数。

| 参数     | 可选/必选 | 说明                                                                                                                                             |
| --------| -------  |------------------------------------------------------------------------------------------------------------------------------------------------|
| --install | 可选 | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。                                                                           |
| --uninstall | 可选 | 卸载软件包。可配置--install-path参数指定软件安装时的路径；不配置--install-path参数时，则直接卸载默认路径下的msProf。                                                                    |
| --install-path | 可选 | 安装路径。路径须指定到cann目录，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br>- root用户：`/usr/local/Ascend/cann`<br>- 非root用户：`${HOME}/Ascend/cann`，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选 | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。                                                                            |
| --help | 可选 | 查看帮助信息。                                                                                                                                        |

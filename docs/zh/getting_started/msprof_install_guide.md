# msProf工具安装指南

msProf工具的安装方式包括：

- 使用CANN包安装：msProf工具完整功能已集成在CANN包中发布，可直接安装CANN包，具体请参见[CANN快速安装](https://www.hiascend.com/cann/download)。
- 使用run包安装：msProf工具完整功能集成在CANN包中且msProf依赖CANN包，因此使用msProf工具需要**先完成CANN包的安装**，若需要升级安装本工具代码仓中的最新功能，可以[下载run包安装](#下载run包安装)或[编译run包安装](#编译run包安装)，在已安装CANN包的环境下覆盖安装msProf包。

## 下载run包安装

使用msProf run包安装需要单独获取msProf发布包，可访问[msProf Release](https://gitcode.com/Ascend/msprof/releases)页面，进入目标版本后下载与当前系统架构匹配的`mindstudio-profiler_<version>_<arch>.run`安装包。若发布页同时提供SHA256校验信息，建议一并获取，用于安装前的完整性校验。

> [!note] 说明
>
> 下载的msProf run包需要在已安装CANN的环境中进行覆盖安装才能使用。

1. 下载msProf run包完成后，执行以下命令为run包添加可执行权限。

   ```shell
   chmod +x mindstudio-profiler_<version>_<arch>.run
   ```

2. 校验安装包完整性。

   ```shell
   # 若发布页提供SHA256校验文件，可直接执行校验
   sha256sum -c <sha256_file>
   
   # 若发布页提供SHA256摘要值，可执行如下命令并与发布页摘要值比对
   sha256sum mindstudio-profiler_<version>_<arch>.run
   
   # run包也支持通过--check执行完整性校验
   ./mindstudio-profiler_<version>_<arch>.run --check
   ```

   打印如下信息，则说明软件包完整性校验成功。

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

3. 校验通过后，执行以下命令完成安装。

   ```shell
   ./mindstudio-profiler_<version>_<arch>.run --install
   ```

4. 如需指定安装路径，可附加 `--install-path=<cann_path>` 参数。安装路径须指向`cann`目录，具体请参见[安装run包参数说明](#安装run包参数说明)。

## 编译run包安装

如需使用最新代码的功能，可下载本仓库代码，自行编译、打包并完成安装。

> [!note] 说明
> 
> 编译出的msProf run包需要在已安装CANN的环境中进行覆盖安装才能使用。

### 编译环境准备

1. 安装依赖。

   msProf工具源码编译依赖SQLite3，请执行以下命令完成安装，或确保当前环境已满足该依赖。

   - Ubuntu系统上安装SQLite3：

     ```shell
      sudo apt update
      sudo apt install sqlite3 libsqlite3-dev
     ```

   - openEuler/CentOS系统上安装SQLite3：

     ```shell
     sudo yum install sqlite sqlite-devel
     ```

2. 克隆本仓库。

   ```shell
   git clone https://gitcode.com/Ascend/msprof.git
   ```

3. 下载第三方依赖。

   ```shell
   cd msprof
   # 下载三方依赖包
   bash scripts/download_thirdparty.sh
   ```

### 执行编译打包

`build/build.sh`编译脚本支持通过--mode参数指定编译类型：

- all：编译全量run包（包含采集与解析功能）
- analysis：编译解析run包（仅包含解析功能）

更多参数说明请参见[编译run包参数说明](#编译run包参数说明)。

编译完成后，会在当前路径`output`目录下生成run包，名称格式为`mindstudio-profiler_<version>_<arch>.run`。其中，`version`为版本号，`arch`为系统架构（根据实际运行系统自动适配）。

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

### 安装run包

1. run包将生成在`output`目录下，执行以下命令为其添加可执行权限：

   ```shell
   cd output
   chmod +x mindstudio-profiler_<version>_<arch>.run
   ```

2. 执行安装命令。

   ```shell
   ./mindstudio-profiler_<version>_<arch>.run --install
   ```

   安装命令支持`--install-path`等参数，具体请参见[安装run包参数说明](https://gitcode.com/Ascend/msprof/blob/37781908bfb29a6686a6a37130d141bff08203bf/docs/zh/msprof_install_guide.md#安装run包参数说明)。

   执行安装命令时，会自动执行`--check`参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

   安装完成后，若显示如下信息，则说明软件安装成功。

   ```ColdFusion
   mindstudio-profiler package install success.
   ```

## 附录

### 编译run包参数说明

msProf工具run包的编译命令可配置如下参数。

| 参数         | 可选/必选 | 说明                                                         |
| ------------ | --------- | ------------------------------------------------------------ |
| --build_type | 可选      | 编包类型，可取值：<br/>&#8226; Release：编译出用于生产环境部署的软件包。<br/>&#8226; Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br/>默认值为Release。 |
| --mode       | 可选      | 编包方式。可取值：<br/>&#8226; all：编译出包含msProf采集和解析功能的软件包。<br/>&#8226; analysis：编译出仅包含msProf解析功能的软件包。<br/>默认值为analysis。 |
| --version    | 可选      | 配置run包的版本号。<br/>默认值为none。                       |

### 安装run包参数说明

msProf工具run包的安装命令可配置如下参数。

| 参数              | 可选/必选 | 说明                                                         |
| ----------------- | --------- | ------------------------------------------------------------ |
| --install         | 可选      | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。 |
| --uninstall       | 可选      | 卸载软件包。可配置--install-path参数指定软件安装时的路径；不配置--install-path参数时，则直接卸载默认路径下的msProf。 |
| --install-path    | 可选      | 安装路径。路径须指定到cann目录，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br>&#8226; root用户：`/usr/local/Ascend/cann`<br>&#8226; 非root用户：`${HOME}/Ascend/cann`，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选      | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。 |
| --help            | 可选      | 查看帮助信息。                                               |

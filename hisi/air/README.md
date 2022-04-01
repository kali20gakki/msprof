# air

#### 介绍
{**以下是 Gitee 平台说明，您可以替换此简介**
Gitee 是 OSCHINA 推出的基于 Git 的代码托管平台（同时支持 SVN）。专为开发者提供稳定、高效、安全的云端软件开发协作平台
无论是个人、团队、或是企业，都能够用 Gitee 实现代码托管、项目管理、协作开发。企业项目请看 [https://gitee.com/enterprises](https://gitee.com/enterprises)}

#### 软件架构

软件架构说明

#### 安装教程

##### 源码安装

`air`也支持由源码编译，进行源码编译前，首先确保系统满足以下要求：

- `GCC >= 7.3.0`
- `CMake >= 3.14.0`

注：`air`的编译**依赖昇腾软件包**（开发者套件中的`ATC`和`FWK`包）,请先在社区下载。

1. 下载`air`源码。

    `air`源码托管在码云平台，可由此下载。需要注意的是，需要同时下载依赖的子模块，指定`--recursive`选项。
    ```
    git clone --recursive https://gitee.com/cann/air.git
    cd air
    git checkout master
    git submodule init
    git submodule update
    ```

2. 在`air`根目录下执行下列命令即可进行编译。
    ```
    bash build.sh
    ```

    - 开始编译之前，请确保正确设置相关的环境变量。
      ```
      export ASCEND_CUSTOM_PATH="/path/to/ascend";
      ```
      1. 如通过安装方式部署，则默认的路径是`/usr/local/Ascend`。
      2. 如通过解压安装包方式，则`ascend`目录下需包含`atc`，`fwkacllib`目录。

    - 在`build.sh`的脚本中，会调用`cmake`下载一些依赖的库，如：`Google Protobuf`、`Google Test`、`Json`等，请确保网络连接正常。
    - 在`build.sh`的脚本中，默认会8线程编译，如果机器性能较差，可能会编译失败。可以通过`-j{线程数}`来控制线程数，如`bash build.sh -j4`。

3. 完成编译后，相应的动态库文件会生成在`output`文件夹中。

4. ut的编译方式
    ```
    bash build.sh -u 编译所有的ut
    bash build.sh -f 编译nn engine的ut
    bash build.sh -a 编译cpu engine的ut
    ```

更多指令帮助，可以使用：
```
bash build.sh -h
```

如果想清除历史编译记录，可以如下操作：
```
rm -rf build/ output/
bash build.sh
```

#### 参与贡献

1.  `Fork` 本仓库
2.  新建 `Feat_xxx` 分支
3.  提交代码
4.  新建 `Pull Request`


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)

# 项目目录

项目全量目录层级介绍如下：

```sh
    └── .gitcode                                  // 存放仓库中的元数据
    └── analysis                                  // 数据解析目录
        └── analyzer                              // 通信数据解析目录
        └── common_func                           // 公共方法目录
        └── csrc                                  // 解析C化代码目录
        └── framework                             // 解析大流程目录
        └── host_prof                             // 解析host侧系统调用数据目录
        └── interface                             // 解析接口
        └── mscalculate                           // 解析数据计算目录
        └── msconfig                              // 存放stars、aicore等配置类
        └── msinterface                           // 存放命令行参数解析类
        └── msmodel                               // 存储DB的处理类
        └── msparser                              // 二进制数据解析流程管理
        └── msprof                                // 数据解析入口
        └── profiling_bean                        // 二进制数据解析处理类
        └── tuning                                // 集群数据管理
        └── viewer                                // 导出交付件
    └── build                                     // 构建目录
        ├── build.sh                              // 构建脚本
        ├── setup.py                              // 构建解析python代码脚本
    └── cmake                                     // 存放解析C化部分cmake文件
    └── docs                                      // 文档
        └── zh                                    // 中文文档
    └── samples                                   // 工具样例存放目录
        ├── README.md                             // 工具样例说明
    └── scripts                                   // 存放run包安装、升级相关脚本
        └── run_script                            // 存放安装等脚本
            ├── install.sh                        // 安装脚本
        ├── create_run_package.sh                 // 打包run包脚本
        ├── download_thirdparty.sh                // 下载第三方依赖脚本
    └── test                                      // 测试部分，存放覆盖率统计脚本
        └── msprof_cpp                            // 数据解析c++代码测试用例
        └── msprof_python                         // 数据解析python代码测试用例
    └── README.md                                 // 整体仓说明文档

```
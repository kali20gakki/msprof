# 为 MindStudio Profiler 贡献

感谢您考虑为 MindStudio Profiler（msProf）做出贡献！我们欢迎任何形式的贡献——错误修复、功能增强、文档改进，甚至只是反馈。无论您是经验丰富的开发者还是第一次参与开源项目，您的帮助都是非常宝贵的。

您可以通过多种方式支持本项目：
- 报告问题或意外行为。
- 建议或实现新功能。
- 改进或扩展文档。
- 审查 Pull Request 并协助其他贡献者。
- 传播项目：在博客文章、社交媒体上分享 msProf，或给仓库点个 ⭐。

## 寻找可贡献的问题

想要开始贡献？可以查看以下类型的问题：
- [Good first issues](https://gitcode.com/Ascend/msprof/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22good%20first%20issue%22)
- [Call for contribution](https://gitcode.com/Ascend/msprof/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22help%20wanted%22)

此外，您可以通过查看 [Issues 列表](https://gitcode.com/Ascend/msprof/issues) 了解项目的发展计划和路线图。

## 开发环境搭建

### 环境要求

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》
- 需要提前安装 CANN 开源版本
- Python 3.7.5 及以上版本
- CMake 3.14 及以上版本

### 安装步骤

1. **Fork 并克隆仓库**
   ```bash
   git clone https://gitcode.com/<your-username>/msprof.git
   cd msprof
   ```

2. **安装开发依赖**
   
   详细安装步骤请参见《[msProf工具安装指南](docs/zh/msprof_install_guide.md)》。

3. **编译项目**
   ```bash
   cd build
   bash build.sh
   ```

### 目录结构

了解项目结构有助于您快速定位代码位置，详细目录介绍请参见《[项目目录说明](docs/zh/dir_structure.md)》。

关键目录：
- `analysis/` - 数据解析核心代码
- `docs/` - 项目文档
- `scripts/` - 构建和安装脚本
- `test/` - 测试用例
- `samples/` - 使用示例

## 代码规范

### Python 代码规范

- 遵循 PEP 8 编码规范
- 使用 4 个空格进行缩进
- 类名使用大驼峰命名法（如 `DataManager`）
- 函数和变量使用小写加下划线命名法（如 `parse_data`）
- 添加必要的类型注解和文档字符串

### C++ 代码规范

- 遵循项目现有的编码风格
- 使用 4 个空格进行缩进
- 类名使用大驼峰命名法
- 函数名使用小驼峰命名法
- 添加必要的注释说明复杂逻辑

### 提交信息规范

提交信息应该清晰地描述更改的内容和原因：

```
<type>: <subject>

<body>

<footer>
```

类型（type）包括：
- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

示例：
```
feat: 添加内存使用分析功能

- 实现内存数据采集模块
- 添加内存使用趋势分析算法
- 更新相关文档

Closes #123
```

## 测试

### 运行测试

在提交代码前，请确保所有测试通过：

```bash
# Python 单元测试
cd test/msprof_python/ut
python -m pytest

# C++ 单元测试
cd test/msprof_cpp/analysis_ut
# 根据具体测试框架运行测试
```

### 添加测试

- 为新功能添加相应的单元测试
- 确保测试覆盖主要逻辑分支
- 测试用例应该具有良好的可读性和维护性
- 测试数据应该放在 `test/` 目录下的相应位置

### 代码覆盖率

可以使用以下脚本生成覆盖率报告：

```bash
# Python 覆盖率
bash scripts/generate_coverage_py.sh

# C++ 覆盖率
bash scripts/generate_coverage_cpp.sh
```

## 文档

### 更新文档

如果您的更改影响用户使用方式，请更新相关文档：

- 用户指南：`docs/zh/`
- API 文档：代码注释中的文档字符串
- 示例代码：`samples/`

### 文档规范

- 使用简洁明了的中文表述
- 提供完整的示例代码
- 包含必要的截图或图表说明
- 确保链接的有效性

## Pull Request 流程

### 提交前检查清单

在提交 Pull Request 之前，请确保：

- [ ] 代码遵循项目的编码规范
- [ ] 添加了必要的测试用例
- [ ] 所有测试通过
- [ ] 更新了相关文档
- [ ] 提交信息清晰明确
- [ ] 代码已经过自我审查

### 提交流程

1. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **提交更改**
   ```bash
   git add .
   git commit -m "feat: your feature description"
   ```

3. **推送到远程仓库**
   ```bash
   git push origin feature/your-feature-name
   ```

4. **创建 Pull Request**
   
   在 GitCode 上创建 Pull Request，并填写：
   - 清晰的标题（遵循提交信息规范）
   - 详细的描述（包括更改内容、原因、测试情况等）
   - 关联相关的 Issue

5. **代码审查**
   
   您需要根据评审意见修改代码，并重新提交更新。此流程可能涉及多轮迭代。请保持积极响应和沟通。

6. **合并**
   
   当您的 PR 获得足够数量的检视者批准后，维护者会进行最终审核。审核和测试通过后，您的 PR 将被合并到主干分支。

### Pull Request 最佳实践

- 保持 PR 的大小适中，便于审查
- 一个 PR 只解决一个问题或实现一个功能
- 及时响应审查意见
- 保持与主分支同步，及时解决冲突

## 报告问题

### 错误报告

如果您发现了 Bug，请创建一个 Issue 并包含以下信息：

- **问题描述**：清晰简洁地描述问题
- **复现步骤**：详细的复现步骤
- **预期行为**：描述您期望的正确行为
- **实际行为**：描述实际发生的情况
- **环境信息**：
  - 操作系统及版本
  - Python 版本
  - CANN 版本
  - msProf 版本
- **日志和截图**：相关的错误日志和截图

### 安全问题

如果您发现了安全漏洞，**请不要公开披露**。请通过邮件直接联系项目维护者，我们会尽快处理。

### 功能建议

如果您有新功能建议，请创建一个 Issue 并使用 `enhancement` 标签，包含：

- **功能描述**：清晰描述建议的功能
- **使用场景**：说明该功能的应用场景
- **实现思路**（可选）：如果有想法可以简单说明
- **相关资料**（可选）：相关的参考资料或示例

## 社区准则

### 行为准则

我们致力于为所有参与者提供一个友好、安全和包容的环境。参与本项目即表示您同意：

- 尊重不同的观点和经验
- 接受建设性的批评
- 关注对社区最有利的事情
- 对其他社区成员表示同理心

### 沟通渠道

- **Issues**：用于报告 Bug、提出功能建议和讨论技术问题
- **Pull Requests**：用于代码审查和讨论具体实现
- **微信群**：日常交流和快速问答（参见 README.md）

## 许可证

通过向本项目贡献代码，您同意您的贡献将按照项目的许可证进行授权。详见 [LICENSE](LICENSE) 文件。

msProf 工具 docs 目录下的文档适用 CC-BY 4.0 许可证，详见 [docs/LICENSE](./docs/LICENSE)。

## 致谢

感谢您为 msProf 做出的贡献。您的努力使这个项目变得更加强大和用户友好。期待您的参与！

---

如有任何疑问或需要帮助，请随时在 [Issues](https://gitcode.com/Ascend/msprof/issues) 中提问，或通过其他社区渠道与我们联系。

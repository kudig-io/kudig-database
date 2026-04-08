# 贡献指南

感谢您对 KUDIG-DATABASE 的关注！以下是参与贡献的完整指南。

---

## 1. 快速开始

```bash
# Fork 并克隆仓库
git clone https://github.com/<your-username>/kudig-database.git
cd kudig-database

# 创建功能分支
git checkout -b feature/your-feature-name

# 编辑文档后提交
git add .
git commit -m "docs: 简要描述你的改动"
git push origin feature/your-feature-name

# 在 GitHub 上创建 Pull Request
```

---

## 2. 目录命名规范

### 顶层目录分类

| 前缀 | 用途 | 示例 |
|:---|:---|:---|
| `domain-{N}-` | 知识域目录，按序号递增 | `domain-1-architecture-fundamentals/` |
| `topic-` | 专题目录，横切多个知识域 | `topic-fta/`, `topic-skills/` |
| 无前缀 | 工具/基础设施目录 | `scripts/`, `man/`, `gitbook/`, `reports/` |

### 文件命名规则

```
✅ 正确格式：
  01-kubernetes-architecture-overview.md    （连字符 + 双位数编号）
  fta-methodology-and-agentic-practices.md （非编号文件用连字符）
  README.md                                 （目录索引文件，大写）

❌ 错误格式：
  1_fta_origin_and_evolution.md             （不使用下划线）
  kubernetes architecture.md                （不使用空格）
  01-K8S-Architecture.md                    （不使用大写）
```

### 命名规则总结

1. **编号**：统一使用两位数（`01-`, `02-`, ...），超过 99 篇用三位数
2. **分隔符**：文件名统一使用连字符（`-`），不使用下划线（`_`）
3. **语言**：文件名使用英文小写
4. **特殊文件**：`README.md`、`CHANGELOG.md`、`STATS.md` 使用大写

---

## 3. 文档结构规范

### 每篇文档必须包含

```markdown
# 标题（一级标题，全文唯一）

> 一句话摘要说明

## 目录（如果文档超过 200 行）

## 正文章节
...

## 参考资料（如有）
- [链接文本](URL)

## 相关文档（推荐）
- 前置阅读：[文档名](相对路径)
- 深入阅读：[文档名](相对路径)
- 实践指南：[文档名](相对路径)
```

### 每个目录必须包含

- `README.md`：目录索引和概述，包含：
  - 目录主题说明
  - 文档列表（表格形式，含序号、标题、关键内容）
  - 与其他模块的关系
  - 学习路径建议（可选）

---

## 4. 内容质量标准

### 基本要求

| 维度 | 要求 |
|:---|:---|
| 专业深度 | 面向生产环境，非入门级 toy 示例 |
| 准确性 | 所有命令/配置经过验证，标注适用 K8s 版本 |
| 完整性 | 包含原理说明 + 配置示例 + 故障排查 |
| 时效性 | 标注适用版本范围，定期检查更新 |

### YAML/代码示例要求

```yaml
# ✅ 正确：包含详细中文注释、版本标注
apiVersion: apps/v1    # K8s v1.9+ GA
kind: Deployment
metadata:
  name: nginx-deployment
  labels:
    app: nginx          # 标签用于 Service 选择器匹配
spec:
  replicas: 3           # 生产环境建议 ≥3 副本
```

```yaml
# ❌ 错误：无注释、无上下文
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
```

### 格式规范

- Markdown 标题层级不超过 4 级（`####`）
- 表格对齐使用 `:---`（左对齐）或 `:---:`（居中）
- 代码块标注语言类型（```yaml, ```bash, ```go 等）
- 中文与英文/数字之间添加空格：`Kubernetes 集群` 而非 `Kubernetes集群`

---

## 5. 交叉引用规范

### 链接格式

```markdown
# 同目录引用
[文档名](./02-filename.md)

# 跨目录引用
[文档名](../domain-5-networking/11-service-concepts-types.md)

# 不使用绝对路径
❌ [文档名](/Users/xxx/kudig-database/domain-5/xxx.md)
```

### 推荐的文档关联

每篇文档末尾建议添加：

```markdown
## 相关文档

| 类型 | 文档 | 说明 |
|:---|:---|:---|
| 前置阅读 | [xxx](路径) | 建议先阅读此文档 |
| 深入阅读 | [xxx](路径) | 更深入的专题内容 |
| 速查参考 | [xxx](路径) | 相关速查卡 |
| 故障排查 | [xxx](路径) | 相关故障排查指南 |
```

---

## 6. 提交规范

### Commit Message 格式

```
<type>: <description>

# 类型说明：
docs:     文档内容新增或修改
fix:      修复文档错误（链接、拼写、格式）
refactor: 目录结构调整、文件重命名
feat:     新增知识域或专题
chore:    脚本、配置等非文档改动
```

### Pull Request 要求

1. PR 标题清晰描述改动范围
2. 说明涉及的知识域/专题
3. 如有文件重命名，列出旧→新映射
4. 确认所有内部链接有效

---

## 7. 贡献类型

| 类型 | 说明 | 难度 |
|:---|:---|:---:|
| 📚 新增文档 | 补充新的知识点/专题 | ⭐⭐⭐ |
| 🔧 修正错误 | 修复拼写、链接、格式问题 | ⭐ |
| 🌍 翻译 | 提供英文版本 | ⭐⭐ |
| 📊 质量提升 | 增强现有文档深度和示例 | ⭐⭐⭐ |
| 🛠️ 工具改进 | 改进脚本和自动化工具 | ⭐⭐ |
| 🐛 报告问题 | 通过 Issue 报告内容问题 | ⭐ |

---

## 8. 联系方式

- 📧 邮箱: [your-email@example.com](mailto:your-email@example.com)
- 💬 Issues: [GitHub Issues](../../issues)
- 💭 Discussions: [GitHub Discussions](../../discussions)

感谢每一位贡献者！🙏

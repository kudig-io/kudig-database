# scripts/ - 项目工具脚本索引

> 本目录存放 KUDIG-DATABASE 项目级自动化脚本，用于**统计、质量检查、可视化**三类场景。

## 脚本清单

| # | 脚本 | 语言 | 用途 | 保留理由 |
|:---:|:---|:---:|:---|:---|
| 1 | `generate-readme-stats.sh` | Bash | README 数字指标统计 | **核心脚本**。自动计算 README 中引用的所有数字（文档数、字符数、知识域、FTA/FEBM/CNCF 等），支持 JSON/表格/徽章三种输出格式，确保每次内容更新后 README 数字与实际一致 |
| 2 | `comprehensive-quality-check.sh` | Bash | 知识库全面质量检查 | 检查目录结构完整性、README 链接有效性、文档长度、头部信息完整性、专家级内容标识等，是 CI/手动审查的入口 |
| 3 | `code-example-validation.sh` | Bash | 文档中 YAML/Bash 代码块语法校验 | 自动提取所有 Markdown 中的 `yaml` 和 `bash` 代码块，分别用 Python yaml 解析和 `bash -n` 做语法校验，防止文档中出现错误示例 |
| 4 | `fta_tree_visualization.py` | Python | FTA 故障树可视化 | 基于 matplotlib 生成高质量 FTA（故障树分析）示意图 PNG，用于文档和演示场景，依赖 matplotlib + numpy |

## 快速使用

```bash
# ── 统计指标（最常用）──────────────────────────
./scripts/generate-readme-stats.sh            # 表格输出
./scripts/generate-readme-stats.sh --json     # JSON 格式（可被其他脚本消费）
./scripts/generate-readme-stats.sh --badges   # 生成 README 徽章 HTML
./scripts/generate-readme-stats.sh --diff     # 比对 README 当前数字

# ── 质量检查 ──────────────────────────────────
./scripts/comprehensive-quality-check.sh      # 全面质量扫描
./scripts/code-example-validation.sh          # YAML/Bash 语法校验

# ── 可视化 ────────────────────────────────────
python3 scripts/fta_tree_visualization.py     # 生成 FTA 故障树图片
```

## 已清理脚本

| 脚本 | 清理原因 |
|:---|:---|
| `count-stats.sh` | 功能已被 `generate-readme-stats.sh` 完全覆盖并大幅增强（支持 JSON/徽章/差异比对，覆盖 README 全部指标） |
| `check-concepts.ps1` | PowerShell 脚本，仅限 Windows；引用的 `reference/concept.md` 路径在项目中不存在，实际不可用 |

## 其他工具

Domain 级别的专用工具存放在各自目录下：

- `domain-12-troubleshooting/tools/` — K8s 故障排查工具套件

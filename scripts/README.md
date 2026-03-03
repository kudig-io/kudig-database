# scripts - 项目工具脚本

项目维护和质量检查相关的自动化脚本。

## 脚本列表

| 脚本 | 说明 |
|:---|:---|
| `count-stats.sh` | 项目统计（文件数、字数、产品数、领域数、知识点数） |
| `comprehensive-quality-check.sh` | 知识库全面质量检查（目录完整性、链接有效性、文档质量） |
| `code-example-validation.sh` | 文档中 YAML/Bash 代码块语法校验 |
| `check-concepts.ps1` | 技术术语完整性检查（PowerShell） |

## 使用方法

```bash
# 运行项目统计
./scripts/count-stats.sh

# 运行质量检查
./scripts/comprehensive-quality-check.sh

# 运行代码示例校验
./scripts/code-example-validation.sh
```

## 其他工具

Domain 级别的专用工具存放在各自目录下：

- `domain-12-troubleshooting/tools/` - K8s 故障排查工具套件

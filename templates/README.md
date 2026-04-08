# 文档模板 (Templates)

> 用于创建新文档的标准化模板，确保全库质量一致性

## 可用模板

| 模板 | 用途 | 适用目录 |
|:---|:---|:---|
| [domain-article-template.md](./domain-article-template.md) | Domain 知识域文档 | `domain-*/` |
| [fta-template.md](./fta-template.md) | FTA 故障树分析文档 | `topic-fta/list/` |
| [skill-template.md](./skill-template.md) | Skill 运维技能文档 | `topic-skills/` |
| [cheat-sheet-template.md](./cheat-sheet-template.md) | 速查卡 | `topic-cheat-sheet/` |

## 使用方法

```bash
# 复制模板到目标目录
cp templates/domain-article-template.md domain-5-networking/42-new-topic.md

# 编辑文档，替换模板占位符
# 搜索 {{PLACEHOLDER}} 替换为实际内容
```

## 模板规范

所有模板遵循 [CONTRIBUTING.md](../CONTRIBUTING.md) 中定义的文档规范：
- 连字符 + 双位数编号命名
- 包含一级标题、摘要、正文、相关文档
- YAML/代码示例包含中文注释

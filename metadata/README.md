# 元数据索引 (Metadata)

> 文档标签、难度分级和知识图谱索引，提升检索和 RAG 分块质量

## 目录

| 文件 | 用途 |
|:---|:---|
| [tags-index.md](./tags-index.md) | 标签索引 - 按标签聚合文档 |
| [difficulty-index.md](./difficulty-index.md) | 难度分级索引 |
| [knowledge-map.md](./knowledge-map.md) | 知识图谱 - 模块间关系 |

## 用途

### 对 RAG 应用的价值
- **标签索引**：帮助 RAG 系统按主题精准检索相关文档
- **难度分级**：根据用户水平推荐合适的文档
- **知识图谱**：构建文档间的语义关联，增强上下文理解

### 对人类读者的价值
- 按主题快速定位相关文档
- 了解学习路径和文档间依赖关系
- 评估自身水平，选择合适难度的内容

## Frontmatter 规范

建议每篇文档逐步添加 YAML frontmatter：

```yaml
---
title: "文档标题"
domain: architecture    # 所属知识域
difficulty: intermediate  # beginner / intermediate / advanced / expert
tags: [kubernetes, architecture, high-availability]
k8s_versions: [v1.28, v1.29, v1.30, v1.31, v1.32]
last_updated: 2026-04-01
---
```

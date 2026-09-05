---
title: Readme
summary: 本目录用于存放专题研究（Topic Research）材料，每个文件对应一个独立的研究课题。
category: research
tags:
- research
- topic-study
- deep-dive
tier: supporting
created: 2026-06-25
updated: 2026-06-25
last_updated: 2026-06-25
---



# Research — 专题研究

本目录用于存放**专题研究（Topic Research）**材料，每个文件对应一个独立的研究课题。

## 目录约定

```
research/
├── README.md                      ← 本文件
├── <topic-slug>.md                ← 单个课题研究文档
└── ...
```

## 文件命名规范

- 全小写，单词间用连字符 `-` 分隔
- 格式：`<主题关键词>.md`
- 示例：`ebpf-production-adoption.md`、`gateway-api-vs-ingress.md`

## 课题文档模板

每个课题文件建议使用以下结构：

```markdown
---
category: research
tags:
  - <tag1>
  - <tag2>
created: YYYY-MM-DD
updated: YYYY-MM-DD
status: draft | in-progress | done
---

# 课题标题

## 研究背景
> 为什么研究这个课题？解决什么问题？

## 核心问题
1. 问题一
2. 问题二

## 调研发现
### 发现一
### 发现二

## 关键对比（可选）
| 维度 | 方案 A | 方案 B |
|------|--------|--------|

## 结论与建议

## 参考资料
```

## 与 concepts/ 的关系

| 位置 | 定位 | 状态 |
|------|------|------|
| `concepts/Research: *.md` | 宏观趋势调研，2025-2026 方向性综述 | 已完成初稿 |
| `research/*.md` | 微观专题深研，针对具体问题/技术点 | 进行中 |

> 课题研究成果成熟后，可提炼为 `concepts/` 或 `entities/` 下的正式知识条目。

---

*本目录为专题研究区，欢迎随时新建课题文件。*

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub

- research/ — tag hub
- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]

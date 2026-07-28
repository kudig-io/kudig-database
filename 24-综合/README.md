---
title: 综合（Synthesis）目录说明
summary: 本目录存放跨域综合文章，将两个及以上知识域的技术交叉融合分析，共 7 个主题分区 40+ 篇文档，是 RAG 语料中的"关联推理"层。
category: synthesis
tags:
- synthesis
- cross-domain
- readme
tier: supporting
created: 2026-07-27
last_updated: 2026-07-27
---

# 综合（Synthesis）— 跨域交叉分析

本目录存放**跨域综合文章**：每篇文档探讨两个或多个技术/概念的交叉点、协同与张力（如 `Kubernetes × etcd`、`Cilium × Service Mesh`）。区别于单一域目录的纵向深度，本目录提供横向关联视角，是 AI 语料库中的"关联推理"层。

完整文章索引见 [[24-综合/index|综合目录索引]]。

## 目录结构

```
24-综合/
├── README.md                ← 本文件（目录说明）
├── index.md                 ← 全量文章索引（按主题分组）
├── 01-AI与机器学习/          ← GPU 调度、FinOps、RAG、训练推理生命周期
├── 02-交付与GitOps/          ← ArgoCD、Rollouts、Crossplane、多集群联邦
├── 03-网络与服务网格/        ← Service、Cilium、mTLS、零信任分段
├── 04-安全与合规/            ← RBAC、供应链、策略即代码、SOC2/HIPAA
├── 05-可观测性/              ← Prometheus、OTel、eBPF、SLO
├── 06-可靠性与成本/          ← 弹性伸缩、备份容灾、混沌工程、多租户
└── 07-平台与数据/            ← etcd、CDC 流处理、平台工程、有状态存储
```

每个子目录含 `index.md` 子索引。

## 文档定位与写作约定

| 约定 | 说明 |
|------|------|
| 命名 | 全小写连字符，体现交叉主体：`<主体A>-<主体B>[-<视角>].md` |
| 选题标准 | 至少涉及 2 个知识域；单域内容应归入对应域目录（01-21） |
| frontmatter | `category: synthesis`；tier 按重要性取 core/supporting |
| 内部链接 | 交叉主体应回链到 [[22-概念/MOC|22-概念]] / 23-实体 的规范条目 |

## 与其他提炼层目录的关系

| 目录 | 定位 | 与本目录的边界 |
|------|------|---------------|
| 22-概念 | 单一概念的规范定义 | 概念本体在 22，概念间关系分析在 24 |
| 23-实体 | 单一项目/产品条目 | 实体档案在 23，实体组合实践在 24 |
| 25-研究 | 开放性专题调研 | 研究成熟后可提炼为 24 的综合文章 |
| 20-最佳实践 | 单域生产实践清单 | 跨域协同的实践模式归入 24 |

## 维护说明

- 新增文章后需同步更新本目录 `index.md` 与所属子目录 `index.md`
- 断链校验由 `.github/workflows/quality.yml` 的 wikilink gate 强制保证

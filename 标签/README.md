---
title: 标签体系
description: KUDIG 知识库标签体系 — 知识图谱导航层，通过标签枢纽页实现跨域知识聚合与发现
category: readme
tags:
- tag-system
- knowledge-graph
- navigation
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
---

# 标签体系 (Tag System)

## 概述

标签目录是 KUDIG 知识库的**知识图谱导航层**。每个标签文件是一个"Tag Hub"（标签枢纽），将分散在各知识域（集群基础、网络、存储、安全、可观测性等）中的相关内容聚合为可发现的知识网络。

## 目录结构

```
标签/
├── README.md                    ← 本文件
├── index.md                     ← 标签总索引
├── k8s.md                       ← Kubernetes 核心知识枢纽
├── networking.md                ← 网络技术枢纽
├── security.md                  ← 安全治理枢纽
├── storage.md                   ← 存储体系枢纽
├── observability.md             ← 可观测性枢纽
├── reliability.md               ← 可靠性工程枢纽
├── production.md                ← 生产运营枢纽
├── troubleshooting.md           ← 故障诊断枢纽
├── platform-engineering.md      ← 平台工程枢纽
├── sre.md                       ← 站点可靠性工程枢纽
├── gitops.md                    ← GitOps 交付枢纽
├── helm.md                      ← Helm 包管理枢纽
├── operator.md                  ← Operator 模式枢纽
├── containerd.md                ← 容器运行时枢纽
├── multi-cluster.md             ← 多集群管理枢纽
├── gpu.md                       ← GPU 调度枢纽
├── ai-ml-infra.md               ← AI/ML 基础设施枢纽
├── best-practices.md            ← 最佳实践枢纽
├── deep-dive.md                 ← 深度解析枢纽
├── papers.md                    ← 论文研究枢纽
├── reference.md                 ← 参考资料枢纽
├── research.md                  ← 研究专题枢纽
└── visibility-public.md         ← 公开可见枢纽
```

## 标签分类体系

### 一级分类

| 分类 | 标签 | 说明 |
|------|------|------|
| 核心领域 | k8s, networking, security, storage, observability, reliability, gpu, ai-ml-infra | 知识域维度 |
| 工程实践 | gitops, helm, operator, containerd, multi-cluster, production, best-practices | 实践维度 |
| 方法论 | troubleshooting, platform-engineering, sre | 方法论维度 |
| 内容类型 | deep-dive, papers, reference, research, visibility-public | 内容形式维度 |

### 标签文件结构规范

每个标签枢纽文件包含：

1. **YAML Frontmatter** — 元数据（title, description, tags, tier, difficulty, domain）
2. **核心定义** — 该标签领域的概念定义与能力矩阵
3. **知识索引** — 按子域分组的相关文档链接
4. **生产实践** — 关键指标、工具生态、最佳实践
5. **Related Tags** — 关联标签导航

## 使用方式

- **知识发现**：从标签枢纽出发，发现某领域的全部相关知识
- **跨域导航**：通过 Related Tags 在不同知识域间跳转
- **学习路径**：按标签组织学习顺序（k8s → networking → security → ...）
- **AI 检索**：标签作为 RAG 检索的元数据过滤条件

## 维护规范

- 新增知识域时，同步创建对应标签枢纽文件
- 标签文件中的链接使用 Obsidian wiki-link 格式：`[[路径|显示名]]`
- 每个标签枢纽至少包含 5 个分类、20 个链接
- 定期审计链接有效性，移除失效引用

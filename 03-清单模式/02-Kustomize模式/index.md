---
title: Kustomize Patterns
description: Kustomize 模式知识域 — Base/Overlay 结构、Transformers、远程构建、GitOps 集成、多环境管理
category: subdomain
tags:
- kustomize
- base-overlay
- transformers
- gitops
- multi-environment
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# Kustomize 模式 Kustomize Patterns

> 无模板的 Kubernetes 配置管理，通过叠加实现多环境差异化。

## 核心概念

| 概念 | 说明 | 用途 |
|------|------|------|
| Base | 基础配置层 | 通用资源定义 |
| Overlay | 叠加层 | 环境差异化配置 |
| Patches | 补丁 | 修改特定字段 |
| Transformers | 转换器 | 批量修改资源 |
| Generators | 生成器 | ConfigMap/Secret 生成 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[03-清单模式/02-Kustomize模式/01-kustomize-base-overlay-structure.md\|Base/Overlay 结构]] | 目录组织与多环境管理 | intermediate |
| [[03-清单模式/02-Kustomize模式/02-kustomize-transformers-reference.md\|Transformers 参考]] | 内置/自定义转换器 | advanced |
| [[03-清单模式/02-Kustomize模式/03-kustomize-remote-build-gitops.md\|远程构建 & GitOps]] | 远程 Base + ArgoCD 集成 | advanced |

## Kustomize vs Helm

| 维度 | Kustomize | Helm |
|------|-----------|------|
| 模板 | 无（纯 YAML 叠加） | Go Template |
| 学习曲线 | 低 | 中 |
| 多环境 | Overlay 叠加 | values 文件 |
| 包管理 | 无 | Chart Repository |
| K8s 集成 | kubectl 原生支持 | 需安装 Helm CLI |

## Related

- [[03-清单模式/index.md|清单模式总索引]]
- [[11-发布变更/index.md|发布变更]]
- [[10-平台工程/index.md|平台工程]]

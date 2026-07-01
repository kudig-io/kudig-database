---
title: k9s
description: k9s 是一个基于终端的 Kubernetes 集群管理 UI 工具。它提供了实时的资源浏览、日志查看、Shell 进入和交互式操作能力，是
  kubectl 的...
summary: k9s 是一个基于终端的 Kubernetes 集群管理 UI 工具。它提供了实时的资源浏览、日志查看、Shell 进入和交互式操作能力，是 kubectl
  的...
category: dictionary
tags:
- k8s
- glossary
- tooling
- ui
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- k9s 是什么
- k9s 详解
trigger_keywords:
- k9s
- dictionary
prerequisites:
- kubectl-basics
---



# k9s

> **英文名**: k9s

## 概述

k9s 是一个基于终端的 Kubernetes 集群管理 UI 工具。它提供了实时的资源浏览、日志查看、Shell 进入和交互式操作能力，是 kubectl 的强力补充。

## 核心概念/原理

### 核心功能

- **资源浏览**：实时查看所有 Kubernetes 资源（Pod、Service、Deployment 等）。
- **日志查看**：实时流式查看 Pod/容器日志。
- **Shell 进入**：直接进入容器 Shell。
- **编辑/删除**：交互式编辑和删除资源。
- **端口转发**：一键设置端口转发。
- **资源使用**：实时显示 CPU/内存使用率。

### 常用快捷键

| 快捷键 | 功能 |
|--------|------|
| `:pods` | 查看 Pod 列表 |
| `:svc` | 查看 Service 列表 |
| `l` | 查看日志 |
| `s` | 进入 Shell |
| `d` | Describe 资源 |
| `e` | 编辑资源 |
| `ctrl-a` | 查看所有资源 |

## 关键机制或特性

- k9s 使用 kubeconfig 连接集群。
- 支持插件和别名自定义。
- 配置文件位于 `~/.config/k9s/`。
- 支持只读模式（`--readonly`）。

## 使用场景与最佳实践

- 日常运维和调试的必备工具。
- 使用 `--readonly` 模式防止误操作。
- 配置自定义皮肤和布局。
- 配合 stern 工具进行高级日志跟踪。

## 参考链接

- [k9s - Official Documentation](https://k9scli.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm.md|Kubeadm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubectx.md|Kubectx]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubens.md|Kubens]]
- [[domain-17-system-foundation/topic-dictionary/tooling/stern.md|Stern]]

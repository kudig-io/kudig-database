---
title: containerd 升级迁移
description: '## 概述'
summary: 'containerd 是 CNCF Graduated 项目，Kubernetes 的默认容器运行时。本页涵盖 containerd 版本升级路径、配置迁移和兼容性验证。从 1.x 到 2.x 涉及 CRI、镜像存储和配置格式的重大变更。'
category: entities
tags:
- k8s
- cncf
- runtime
- 04-containerd-upgrade-migration
- kubelet
- prometheus
- grafana
- containerd
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 升级迁移 是什么
- 如何 containerd 升级迁移
trigger_keywords:
- containerd
- 升级迁移
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 升级迁移

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

containerd 是由 Docker 公司开发并于 2017 年捐赠给 CNCF 的容器运行时，2019 年成为 Graduated 项目。它是 Kubernetes 的默认 CRI（Container Runtime Interface）实现，负责镜像拉取、容器生命周期管理和存储管理。containerd 的版本升级（特别是从 1.x 到 2.x）涉及多项**破坏性变更**：移除了已废弃的配置参数、更改了默认镜像存储格式（从 `containerd.io/oci/pause` 迁移）、CRI 版本升级等。

本页指导如何在生产 Kubernetes 集群中安全地升级 containerd，包括升级前检查、滚动升级策略、配置迁移和回滚方案。

## 升级关键事项

- **CRI 版本兼容性**：containerd 2.0 要求 Kubernetes 1.27+，确保 kubelet 版本匹配
- **配置迁移**：`config.toml` 中部分 v1 字段在 v2 中被移除（如 `cri.plugin` 路径变更）
- **镜像存储格式**：v2 默认使用新的 snapshotter（如 `overlayfs` 配置变更）
- **NRI（Node Resource Interface）**：v2 中 NRI 成为内置插件
- **沙箱（Sandbox）镜像**：Pause 镜像路径和版本需要验证

## Architecture

containerd 运行在每个 Kubernetes 节点上，通过 gRPC 与 kubelet 通信（CRI 接口）。核心组件包括：**containerd-shim**（管理单个容器的生命周期）、**containerd-shim-runc-v2**（OCI runtime shim）、**ctr**（CLI 工具）、**nerdctl**（Docker 兼容 CLI）。升级时需要停止 kubelet → 升级 containerd 二进制 → 迁移配置 → 重启 kubelet。

## K8s 集成

containerd 是 Kubernetes 节点的核心组件。kubelet 通过 CRI gRPC 接口（`/var/run/containerd/containerd.sock`）管理容器。升级 containerd 会影响该节点上所有 Pod 的运行状态，因此必须**逐节点滚动升级**（cordon → drain → upgrade → uncordon）。

## 生产部署要点

- **升级前备份**：备份 `config.toml` 和 containerd 数据目录（`/var/lib/containerd`）
- **滚动升级**：逐节点升级，每节点 drain 后操作，避免批量中断
- **兼容性检查**：升级前验证 `crictl version` 与目标 containerd 版本的 CRI 兼容性
- **配置迁移工具**：使用 `containerd config migrate` 自动迁移旧配置
- **验证脚本**：升级后运行 Pod 创建/销毁测试验证功能正常

## 生产场景

1. **1.7 → 2.0 大版本升级**：核心集群的 containerd 版本升级，需要全面兼容性验证
2. **安全补丁升级**：CVE 修复的小版本升级（如 1.6.x → 1.6.y）
3. **配置参数迁移**：启用新的 snapshotter 或 CNI 配置变更
4. **多架构集群升级**：ARM 和 x86 节点的混合升级策略

## 操作命令

```bash
# 🟢 只读：检查当前版本
containerd --version
crictl version

# 🟡 升级前：cordon 节点
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 🟡 升级 containerd（以 Debian/Ubuntu 为例）
systemctl stop kubelet
systemctl stop containerd
apt-get update && apt-get install containerd=1.7.20*
# 迁移配置（如需要）
containerd config migrate
systemctl start containerd
systemctl start kubelet

# 🟢 验证
kubectl uncordon <node-name>
crictl ps  # 确认容器正常运行
```

## 对比

| 特性 | containerd 1.7 | containerd 2.0 |
|------|---------------|----------------|
| CRI 版本 | v1alpha2/v1 | v1（移除 alpha2） |
| 配置格式 | v1 TOML | v2 TOML |
| NRI | 外部插件 | 内置 |
| 沙箱镜像 | `k8s.gcr.io/pause` | `registry.k8s.io/pause` |

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[pod-lifecycle]]
- [[23-实体/kubelet.md|[[kubelet|kubelet]]]]

## Related

- [[spinkube]] — SpinKube
- [[wasmedge]] — WasmEdge
- [[23-实体/15-参考与索引/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[37-归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- RELEASE-NOTES-1.6
- [[37-归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- [[37-归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- RELEASE-NOTES-1.1
- RELEASE-NOTES-0.0
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- 04-containerd-upgrade-migration

<!-- risk-assessed -->

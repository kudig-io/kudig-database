---
title: 发布说明索引 — 网络
description: '# 发布说明索引 — 网络'
summary: '# 发布说明索引 — 网络'
category: references
tags:
- k8s
- release-notes
- networking
- calico
- cilium
- cni-plugins
- envoy
- istio
- linkerd
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 网络 是什么
- 如何 发布说明索引 — 网络
trigger_keywords:
- 发布说明索引
- 网络
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — 网络

> 本文档汇总网络领域 6 个核心项目的发布说明索引，共覆盖 **157 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Calico | 35 | v3.31 | v3.27 | 网络策略与 CNI |
| Cilium | 24 | v1.19 | v1.3 | eBPF 网络与安全 |
| CNI Plugins | 14 | v1.9 | v1.0 | 标准 CNI 插件集 |
| Envoy | 38 | v1.37 | — | 边缘与服务代理 |
| Istio | 38 | v1.29 | — | 服务网格平台 |
| Linkerd | 8 | v18.9 | — | 轻量服务网格 |

---

## 项目详情

### Calico

- **最新版本**: v3.31
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/calico/`
- **版本覆盖**: v0.1 → v3.31（35 个版本）
- **Breaking Changes 提醒**:
  - v3.27: Felix 配置参数和 BGP 默认行为变更
- **升级要点**: v3.x 持续优化 eBPF 数据平面和 WireGuard 加密

### Cilium

- **实体页面**: [[cilium|Cilium]]
- **最新版本**: v1.19
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/cilium/`
- **版本覆盖**: v0.1 → v1.19（24 个版本）
- **Breaking Changes 提醒**:
  - v1.3: Hubble 和 Gateway API 配置格式变更
- **升级要点**: v1.x 引入 Gateway API 原生支持和 Tetragon 集成

### CNI Plugins

- **实体页面**: [[entities/cni-plugins.md|CNI Plugins]]
- **最新版本**: v1.9
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/cni-plugins/`
- **版本覆盖**: v0.1 → v1.9（14 个版本）
- **Breaking Changes 提醒**:
  - v1.0: CNI 规范 1.0，插件接口标准化
- **升级要点**: 基础网络插件集，保持与 CNI 规范同步

### Envoy

- **实体页面**: [[envoy|Envoy]]
- **最新版本**: v1.37
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/envoy/`
- **版本覆盖**: v0.1 → v1.37（38 个版本）
- **升级要点**: 持续优化 xDS API 和 WASM 扩展能力

### Istio

- **实体页面**: [[istio|Istio]]
- **最新版本**: v1.29
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/istio/`
- **版本覆盖**: v0.1 → v1.29（38 个版本）
- **升级要点**: Ambient Mesh 模式逐步成熟，sidecar-less 架构演进

### Linkerd

- **实体页面**: [[linkerd|Linkerd]]
- **最新版本**: v18.9
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/networking/linkerd/`
- **版本覆盖**: v0.1 → v18.9（8 个版本）
- **升级要点**: 轻量级服务网格，专注 mTLS 和流量管理

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v3.27 | Calico | Felix 配置和 BGP 默认行为变更 |
| v1.3 | Cilium | Hubble 和 Gateway API 配置格式变更 |
| v1.0 | CNI Plugins | CNI 规范 1.0 接口标准化 |

---

## 相关导航

- [[concepts/service-mesh-evolution.md|服务网格演进]]
- [[domain-19-landscape-references/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[entities/tetragon.md|tetragon]] — Tetragon
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[cni]] — CNI (Container Network Interface)
- [[envoy]] — Envoy

- [[README]]

<!-- risk-assessed -->

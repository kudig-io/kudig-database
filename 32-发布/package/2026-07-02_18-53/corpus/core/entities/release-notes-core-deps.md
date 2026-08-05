---
title: 发布说明索引 — 核心依赖
description: '| etcd | 15 | v3.6 | v3.6 | 分布式键值存储 |'
summary: '| etcd | 15 | v3.6 | v3.6 | 分布式键值存储 |'
category: references
tags:
- k8s
- release-notes
- core-deps
- containerd
- coredns
- cri-o
- etcd
- runc
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 核心依赖 是什么
- 如何 发布说明索引 — 核心依赖
trigger_keywords:
- 发布说明索引
- 核心依赖
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — 核心依赖

> 本文档汇总 Kubernetes 核心依赖领域 5 个组件的发布说明索引，共覆盖 **83 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| containerd | 13 | v2.2 | v1.3 | 容器运行时 |
| CoreDNS | 16 | v1.14 | v1.13 | 集群 DNS 服务 |
| CRI-O | 32 | v1.35 | — | OCI 容器运行时 |
| etcd | 15 | v3.6 | v3.6 | 分布式键值存储 |
| runc | 7 | v1.4 | — | OCI 运行时规范实现 |

---

## 项目详情

### containerd

- **实体页面**: [[containerd|containerd]]
- **最新版本**: v2.2
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/core-deps/containerd/`
- **版本覆盖**: v0.1 → v2.2（13 个版本）
- **Breaking Changes 提醒**:
  - v1.3: 插件接口和配置文件格式变更
  - v2.0: 主版本升级，API 重构
- **升级要点**: v2.x 为新的主版本线，与 Kubernetes 1.30+ 搭配推荐

### CoreDNS

- **实体页面**: [[coredns|CoreDNS]]
- **最新版本**: v1.14
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/core-deps/coredns/`
- **版本覆盖**: v0.1 → v1.14（16 个版本）
- **Breaking Changes 提醒**:
  - v1.13: 部分插件默认配置变更
- **升级要点**: Kubernetes 默认 DNS 实现，保持与 K8s 版本同步

### CRI-O

- **实体页面**: [[cri-o|CRI-O]]
- **最新版本**: v1.35
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/core-deps/cri-o/`
- **版本覆盖**: v0.1 → v1.35（32 个版本）
- **升级要点**: 版本号与 Kubernetes 版本对齐（v1.35 → K8s 1.35）

### etcd

- **实体页面**: [[etcd|etcd]]
- **最新版本**: v3.6
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/core-deps/etcd/`
- **版本覆盖**: v0.1 → v3.6（15 个版本）
- **Breaking Changes 提醒**:
  - v3.6: gRPC API 和存储引擎优化
- **升级要点**: v3.x 系列持续优化 MVCC 性能和快照恢复

### runc

- **最新版本**: v1.4
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/core-deps/runc/`
- **版本覆盖**: v0.1 → v1.4（7 个版本）
- **升级要点**: OCI 运行时规范参考实现，安全修复优先

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v2.0 | containerd | 主版本 API 重构 |
| v1.13 | CoreDNS | 插件默认配置变更 |
| v3.6 | etcd | gRPC API 和存储引擎优化 |

---

## 版本兼容矩阵

核心依赖版本与 Kubernetes 版本的对应关系详见：
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]]
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]]

---

## 相关导航

- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]]
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/skills/training-lecturer/11-workloads/index|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->

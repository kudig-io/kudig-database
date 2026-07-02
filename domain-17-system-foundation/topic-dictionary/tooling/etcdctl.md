---
title: etcdctl
description: etcdctl 是 etcd 的官方命令行客户端工具，用于直接与 etcd 集群交互。在 Kubernetes 运维中，etcdctl 常用于集群健康检查、数据...
summary: etcdctl 是 etcd 的官方命令行客户端工具，用于直接与 etcd 集群交互。在 Kubernetes 运维中，etcdctl 常用于集群健康检查、数据...
category: dictionary
tags:
- k8s
- glossary
- tooling
- etcd
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcdctl 是什么
- etcdctl 详解
trigger_keywords:
- etcdctl
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# etcdctl

> **英文名**: etcdctl

## 概述

etcdctl 是 etcd 的官方命令行客户端工具，用于直接与 etcd 集群交互。在 Kubernetes 运维中，etcdctl 常用于集群健康检查、数据备份和恢复操作。

## 核心概念/原理

### 核心命令

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查集群健康状态
etcdctl endpoint health --cluster

# 查看集群成员
etcdctl member list --write-out=table

# 查看集群状态
etcdctl endpoint status --cluster --write-out=table

# 备份数据
etcdctl snapshot save /backup/etcd-snapshot.db

# 恢复数据
etcdctl snapshot restore /backup/etcd-snapshot.db  # ⚠️ 覆盖 etcd 数据，集群状态回退

# 查看 key（仅用于调试）
etcdctl get /registry/pods --prefix --keys-only
```
### 环境变量

```bash
export ETCDCTL_API=3
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/peer.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/peer.key
```

## 关键机制或特性

- etcdctl v3 是推荐版本（`ETCDCTL_API=3`）。
- 访问 Kubernetes 的 etcd 需要 TLS 证书认证。
- `snapshot save` 是备份 etcd 数据的标准方法。
- 不建议直接修改 etcd 中的 Kubernetes 数据。

## 使用场景与最佳实践

- 定期使用 etcdctl 检查集群健康状态。
- 实施自动化 etcd 备份策略（每天至少一次）。
- 升级或迁移前必须执行 `snapshot save`。
- 监控 etcd 的 WAL fsync 延迟和 DB size。

## 参考链接

- [etcdctl - Official Documentation](https://etcd.io/docs/latest/op-guide/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm.md|Kubeadm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubectx.md|Kubectx]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubens.md|Kubens]]
- [[domain-17-system-foundation/topic-dictionary/tooling/k9s.md|K9S]]


<!-- risk-assessed -->

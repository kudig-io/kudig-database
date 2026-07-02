---
title: 版本升级指南
description: '# 版本升级指南'
summary: 'kubectl api-resources | grep -E "DEPRECATED|removed"'
category: references
tags:
- k8s
- release-notes
- upgrade
- version-compatibility
- maintenance
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 版本升级指南 是什么
- 如何 版本升级指南
trigger_keywords:
- 版本升级指南
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
- backup-basics
---



# 版本升级指南

> 本文档基于 `domain-19-landscape-references/_archived-release-notes/` 目录下全部 1321 个发布说明文件提炼而成，为 K8s 集群运维人员提供版本升级的参考指南 ^[inferred]

## Kubernetes 升级策略

### 升级路径规则

- **相邻版本升级**：仅支持相邻小版本升级（v1.28 -> v1.29）
- **跳过版本**：不允许跳过多个版本直接升级
- **组件升级顺序**：控制面先升级，然后节点

### 升级前检查

1. **API 弃用检查**
   ```bash
   kubectl api-resources | grep -E "DEPRECATED|removed"
   ```

2. **组件兼容性**
   - 确认 etcd 版本与目标 K8s 版本兼容
   - 确认 containerd/CRI-O 版本支持目标 CRI API
   - 确认网络插件兼容新版本

3. **备份**
   - etcd 快照备份
   - 关键 CRD 数据备份
   - Velero 集群备份

### 控制面升级步骤

```bash
# 1. 升级 kube-apiserver
# 2. 升级 kube-controller-manager
# 3. 升级 kube-scheduler
# 4. 升级 etcd（如需要）
```

### 节点升级步骤

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 1. 驱逐节点上的 Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 2. 升级 kubelet 和 kube-proxy

# 3. 重启节点

# 4. 取消驱逐
kubectl uncordon <node-name>
```

## 核心组件升级

### etcd 升级

1. 执行 etcd 快照备份
2. 逐个升级 etcd 成员（rolling upgrade）
3. 验证集群健康状态
4. 检查 API 版本兼容性

### containerd 升级

1. 检查 CRI API 兼容性
2. 节点级滚动升级
3. 验证 Pod 运行状态

## 生态组件升级

### Argo CD 升级

- 参考 [[concepts/gitops-tool-evolution.md|GitOps 工具演进]] 了解版本变更
- 备份 etcd 中 Argo CD 数据
- 按官方升级指南操作

### Prometheus 升级

- 注意 v2.0 的破坏性变更（存储层完全重写）
- 备份 TSDB 数据
- 检查规则和告警兼容性

### Istio 升级

- v1.5 的架构重构（合并为 istiod）需特别注意
- 使用 `istioctl upgrade` 命令
- 验证 Sidecar 注入

## 版本兼容性矩阵

### 当前推荐组合

| K8s | etcd | containerd | CoreDNS | CNI |
|---|---|---|---|---|
| v1.28 | v3.5.x | v1.6.x | v1.10.x | Calico/Cilium 最新稳定 |
| v1.29 | v3.5.x | v1.7.x | v1.11.x | Calico/Cilium 最新稳定 |
| v1.30 | v3.5.x | v1.7.x | v1.11.x | Calico/Cilium 最新稳定 |
| v1.31 | v3.5.x | v1.7.x | v1.11.x | Calico/Cilium 最新稳定 |
| v1.32 | v3.5.x | v1.7.x | v1.11.x | Calico/Cilium 最新稳定 |

## 回退策略

1. **控制面回退**：恢复 etcd 快照到升级前状态
2. **节点回退**：安装旧版本 kubelet 并重启
3. **应用回退**：使用 Velero 恢复或 GitOps 回滚

## 来源文档

domain-19-landscape-references/_archived-release-notes/ 目录下全部 1321 个文件。

## Related

- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows

- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- 07-upgrade-paths-strategy
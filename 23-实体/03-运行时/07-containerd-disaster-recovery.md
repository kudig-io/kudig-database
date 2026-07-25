---
title: containerd 灾难恢复
description: '## 概述'
summary: 'containerd 是 CNCF Graduated 项目，Kubernetes 的默认容器运行时。本页涵盖 containerd 节点级灾难恢复场景：数据目录损坏、镜像丢失、运行时故障和节点重建流程。'
category: entities
tags:
- k8s
- cncf
- runtime
- 07-containerd-disaster-recovery
- containerd
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 灾难恢复 是什么
- 如何 containerd 灾难恢复
trigger_keywords:
- containerd
- 灾难恢复
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 灾难恢复

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

containerd 作为 Kubernetes 节点的容器运行时，其稳定性直接关系到该节点上所有工作负载的可用性。本页总结生产环境中常见的 containerd 灾难场景及其恢复策略，包括：容器运行时进程崩溃、镜像数据损坏、磁盘空间耗尽、节点状态异常和完整节点重建。

containerd 的数据存储在两个主要位置：`/var/lib/containerd`（镜像层和容器状态）和 `/var/lib/containerd/io.containerd.content.v1.content`（镜像内容 blobs）。恢复策略取决于故障的具体表现和数据损坏程度。

## 常见故障场景

- **containerd 进程崩溃**：OOM Kill、配置错误导致启动失败、NRI 插件冲突
- **镜像数据损坏**：`/var/lib/containerd` 文件系统损坏、意外的 `rm -rf` 操作
- **磁盘空间耗尽**：镜像层堆积、容器日志膨胀导致 `/var` 满载
- **快照器（Snapshotter）故障**：overlayfs 挂载残留、inotify watch 耗尽
- **CNI 配置丢失**：`/etc/cni/net.d/` 被误删导致 Pod 网络异常

## Architecture

containerd 灾难恢复的核心原则是"**Pod 可重建，状态在别处**"。有状态应用的持久数据通过 PVC 存储在外部存储系统中，containerd 仅管理容器运行时状态。因此，大部分灾难场景的恢复策略是：清理损坏的运行时数据 → 重启 containerd → 让 Kubernetes 重新调度和创建 Pod。

## K8s 集成

Kubernetes 的声明式模型是灾难恢复的基础。当 containerd 数据损坏时，删除 `/var/lib/containerd` 内容并重启 containerd，kubelet 会检测到容器缺失并自动重建 Pod。对于 Deployment/StatefulSet 管理的 Pod，控制器会确保期望副本数。StatefulSet 的 PVC 数据独立存储，不受 containerd 数据目录影响。

## 生产部署要点

- **磁盘监控**：监控 `/var` 磁盘使用率，设置 80% 告警阈值
- **镜像定期清理**：配置 `crictl rmi --prune` 或 containerd GC 定期清理未使用镜像
- **配置备份**：`/etc/containerd/config.toml` 纳入版本控制
- **节点冗余**：确保集群有足够冗余节点，单节点故障不影响业务
- **PodDisruptionBudget**：为关键服务配置 PDB，确保 drain 时最小可用副本数

## 生产场景

1. **containerd 数据目录损坏**：文件系统错误导致镜像无法拉取，需要清理重建
2. **磁盘空间耗尽**：日志和镜像堆积导致 containerd 无法写入，紧急清理恢复
3. **节点无法恢复**：节点彻底损坏，需要替换节点并等待 Pod 重调度
4. **容器卡死在 ContainerCreating**：快照残留导致新容器无法创建，需要清理残留

## 操作命令

```bash
# 🟢 诊断：检查 containerd 状态
systemctl status containerd
journalctl -u containerd --since "1 hour ago" | tail -50
crictl ps -a | head -20
df -h /var/lib/containerd

# 🟡 清理未使用镜像释放空间
crictl rmi --prune
nerdctl system prune -f

# 🔴 完全重建 containerd 数据（高风险！会删除所有本地镜像和容器状态）
systemctl stop kubelet
systemctl stop containerd
mv /var/lib/containerd /var/lib/containerd.bak.$(date +%s)
mkdir -p /var/lib/containerd
systemctl start containerd
systemctl start kubelet
# 等待 kubelet 重新拉取镜像并重建 Pod

# 🟡 重启 containerd 进程（不丢数据）
systemctl restart containerd
# 验证容器恢复
crictl ps
```

## 对比

| 恢复策略 | 影响范围 | 恢复时间 | 数据安全 |
|----------|---------|---------|---------|
| 重启 containerd | 本节点 Pod 短暂中断 | < 1min | ✅ 无数据丢失 |
| 清理镜像缓存 | 镜像需重新拉取 | 3-10min | ✅ PVC 数据安全 |
| 完全重建数据目录 | 本节点所有 Pod 重建 | 5-15min | ✅ PVC 数据安全 |
| 节点替换 | Pod 重调度到其他节点 | 5-20min | ✅ PVC 数据安全 |

## 参考链接

- [[containerd]]
- [[deployment]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[flatcar]] — Flatcar Container Linuxux 生产环境速查卡|Linux]]
- [[kcp]] — kcp
- [[23-实体/15-参考与索引/cncf-security.md|cncf-security]] — CNCF 安全与合规项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- 07-containerd-disaster-recovery
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
- [[23-实体/03-运行时/hyperlight.md|Hyperlight]]
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->

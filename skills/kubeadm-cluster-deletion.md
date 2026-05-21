---
title: kubeadm 集群删除操作
description: '## 概述'
category: skills
tags:
- k8s
- kubeadm
- cluster-deletion
- reset
- drain
- cleanup
- etcd-cleanup
- etcd
- kubelet
- flannel
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 集群删除操作 是什么
- 如何 kubeadm 集群删除操作
trigger_keywords:
- kubeadm
- 集群删除操作
prerequisites:
- kubectl-basics
- etcd-basics
---

# kubeadm 集群删除操作

## 概述

集群删除是 Kubernetes 生命周期管理中不可忽视的环节。`kubeadm reset` 负责清理当前节点上的 Kubernetes 相关配置，但不会自动清理 CNI、iptables 等网络配置。完整的删除流程需要多个步骤配合。

## 完整节点移除流程

```
步骤 1: 驱逐 Pod     → kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
步骤 2: 删除 Node    → kubectl delete node <node>
步骤 3: reset 节点   → kubeadm reset --force
步骤 4: 手动清理     → iptables/ipvs/CNI/证书/数据目录
```

## kubeadm reset 的三个阶段

| Phase | 说明 | 关键操作 |
|-------|------|---------|
| `preflight` | root 权限检查、用户确认 | 跳过确认提示（`--force`） |
| `remove-etcd-member` | 从 etcd 集群移除本地成员 | `etcdctl member remove` |
| `cleanup-node` | 停止服务、清理目录、删除容器 | 卸载挂载点、清理 manifest/pki |

## reset 的容错设计

`kubeadm reset` 采用 **best-effort** 策略：每个步骤失败后仅打印 Warning，不中断后续步骤。

| 失败场景 | reset 行为 |
|---------|-----------|
| kubelet 停止失败 | 提示手动停止 |
| 卸载挂载点失败 | 继续清理目录 |
| 容器移除失败 | 继续清理配置 |
| etcd 成员移除失败 | 提示手动 `etcdctl member remove` |
| 目录清理失败 | 继续清理下一个目录 |

## reset 不清理的内容

kubeadm reset 不会自动清理以下内容，需要手动处理：

| 内容 | 清理命令 |
|------|---------|
| iptables 规则 | `iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X` |
| ipvs 规则 | `ipvsadm -C` |
| CNI 配置 | `rm -rf /etc/cni/net.d` |
| CNI 插件 | `rm -rf /opt/cni` |
| kubeconfig | `rm -rf $HOME/.kube/config` |
| 容器数据 | `rm -rf /var/lib/containerd/*` |
| 网络接口 | `ip link delete cni0`、`ip link delete flannel.1` |

## 异常场景处理

### 节点不可达

```bash
# 在可达的控制面节点上
kubectl delete node <unreachable-node>

# 手动移除 etcd 成员
etcdctl member remove <member-id>

# 节点恢复后执行
kubeadm reset -f
```

### etcd 仲裁丢失

```bash
# 3 节点 etcd 中 2 个节点丢失
kubeadm reset -f --skip-phases=remove-etcd-member
rm -rf /var/lib/etcd
```

### 卸载挂载点卡住

```bash
# 懒卸载
umount -l $(mount | grep kubelet | awk '{print $3}')

# 或在 ResetConfiguration 中设置
# unmountFlags: ["MNT_DETACH"]
```

## 删除前备份检查清单

在执行集群删除前，确认以下内容已备份：

- [ ] etcd 快照：`etcdctl snapshot save`
- [ ] PKI 证书目录：`/etc/kubernetes/pki/`
- [ ] kubeconfig 文件
- [ ] 关键应用数据（PVC 后端存储）
- [ ] kubeadm 配置文件
- [ ] 网络配置（CNI 配置文件）

## 灾难恢复删除脚本

```bash
#!/bin/bash
NODE_NAME=$(hostname)

# 1. 尝试正常 reset
kubeadm reset -f || true

# 2. 停止所有服务
systemctl stop kubelet 2>/dev/null || true
systemctl stop containerd 2>/dev/null || true

# 3. 强制卸载 kubelet 挂载点
mount | grep '/var/lib/kubelet' | awk '{print $3}' | while read mp; do
    umount -l "$mp" 2>/dev/null || true
done

# 4. 清理数据
rm -rf /etc/kubernetes/ /var/lib/kubelet/ /var/lib/etcd/
rm -rf /etc/cni/net.d/ /opt/cni/ $HOME/.kube/

# 5. 清理网络规则
iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X
ipvsadm -C 2>/dev/null || true
ip link delete cni0 2>/dev/null || true
ip link delete flannel.1 2>/dev/null || true
```

## 相关技能

- [[skills/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]]
- [[skills/node-drain-and-maintenance.md|节点驱逐与维护]]
- [[skills/backup-restore-etcd.md|备份和恢复 etcd]]
- [[concepts/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[etcd|etcd]]

## Related

- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系

- [[domain-07-platform-engineering/topic-code-analysis/cluster-delete/README.md|Cluster Delete — Kubernetes 集群删除源码分析]]
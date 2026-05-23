---
title: HA 集群删除注意事项 (topic-code-analysis)
description: '## 概述'
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- HA 集群删除注意事项 是什么
- 如何 HA 集群删除注意事项
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- HA
- 集群删除注意事项
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
created: "2026-05-23"
---

title: HA 集群删除注意事项
category: cluster-delete
tags:
- ha
- high-availability
- etcd
- quorum
- load-balancer
- control-plane
- kubeadm
- cluster-delete
last_updated: 2026-05-18
description: 深入分析高可用（HA）Kubernetes 集群删除的关键注意事项，涵盖 etcd 仲裁维护、删除顺序要求、负载均衡器处理、控制面组件顺序退出、External
  etcd HA 集群删除以及部分删除（缩小控制面规模）等场景。
difficulty: advanced
intent_queries:
- kubernetes HA cluster deletion注意事项
- etcd quorum maintenance cluster deletion
- HA cluster control plane removal sequence
- kubernetes HA cluster teardown etcd member
- load balancer update HA cluster deletion
trigger_keywords:
- HA cluster deletion
- etcd quorum
- etcd member removal
- stacked etcd
- external etcd
- kube-vip
- load balancer
- control plane
- upload-certs
- kubeadm-config
reading_level: advanced
audience:
- platform-engineer
- kubernetes-administrator
- sre
estimated_read_time: 5min
related_domains:
- domain-01-cluster-fundamentals
- domain-01-cluster-fundamentals
related_topics:
- cluster-delete
- etcd-cleanup
- reset
- force-delete
- cleanup
- cloud-delete
domain_link: '[Control Plane](../domain-01-cluster-fundamentals/README.md)'
topic_link: '[Cluster Delete Overview](./01-overview.md)'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# HA 集群删除注意事项

## 概述

高可用（HA）Kubernetes 集群的删除比单节点集群复杂得多：需要维护 etcd 仲裁、处理负载均衡器、确保控制面组件正常退出。本文档分析 HA 集群删除的关键注意事项。

---

## HA 架构回顾

```
┌──────────────────────────────────────────────────────────────────┐
│  HA 集群架构（Stacked etcd）                                      │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │  CP Node 1       │  │  CP Node 2       │  │  CP Node 3       │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────┐  │  │
│  │  │ API Server │  │  │  │ API Server │  │  │  │ API Server │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │  Scheduler │  │  │  │  Scheduler │  │  │  │  Scheduler │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │    CCM     │  │  │  │    CCM     │  │  │  │    CCM     │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │   etcd-1   │  │  │  │   etcd-2   │  │  │  │   etcd-3   │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────┘  │  │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘  │
│           │                    │                    │              │
│           └────────────────────┼────────────────────┘              │
│                                │                                    │
│                     ┌──────────▼──────────┐                        │
│                     │   Load Balancer      │                        │
│                     │   (kube-vip/HAProxy) │                        │
│                     └──────────┬──────────┘                        │
│                                │                                    │
│                     ┌──────────▼──────────┐                        │
│                     │    Worker Nodes      │                        │
│                     └─────────────────────┘                        │
└──────────────────────────────────────────────────────────────────┘
```

---

## 1. etcd 仲裁维护

### 1.1 删除顺序要求

```
┌──────────────────────────────────────────────────────────────┐
│  3 节点 HA 集群的安全删除顺序                                  │
├──────────────────────────────────────────────────────────────┤
│                                                                │
│  Step 1: 删除第 1 个控制面节点                                  │
│    成员数: 3 → 2  (仲裁: 2, 仍可用 ✅)                        │
│    ├─ drain + delete node                                      │
│    ├─ kubeadm reset (自动移除 etcd 成员)                       │
│    └─ 确认 etcd 健康后继续                                     │
│                                                                │
│  Step 2: 删除第 2 个控制面节点                                  │
│    成员数: 2 → 1  (仲裁: 1, 勉强可用 ⚠️)                      │
│    ├─ drain + delete node                                      │
│    ├─ kubeadm reset                                            │
│    └─ 此时集群仍有 1 个 etcd 成员                              │
│                                                                │
│  Step 3: 删除第 3 个控制面节点                                  │
│    成员数: 1 → 0  (集群销毁)                                   │
│    └─ kubeadm reset -f                                         │
│                                                                │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 禁忌操作

| 操作 | 后果 |
|------|------|
| 同时 reset 2 个控制面节点 | etcd 仲裁丢失，集群不可恢复 |
| 不移除 etcd 成员直接清理数据 | etcd 集群永远无法达到健康状态 |
| 先删 etcd leader | 触发 leader election，短暂不可用 |

### 1.3 etcd Leader 处理

```bash
# 查看当前 leader
etcdctl endpoint status --write-out=table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key

# 如果要删除的节点是 leader，先移动 leader
etcdctl move-leader <target-member-id> \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  --endpoints=https://<non-leader-ip>:2379
```

---

## 2. 负载均衡器处理

### 2.1 kube-vip

```bash
# 查看 kube-vip 状态
ip addr show | grep <vip-address>

# kube-vip 作为静态 Pod 运行
# kubeadm reset 会清理 /etc/kubernetes/manifests/kube-vip.yaml

# 手动清理
ip addr del <vip-address>/32 dev eth0
```

### 2.2 外部负载均衡器（HAProxy/Nginx）

```
┌──────────────────────────────────────────────────────────┐
│  外部 LB 更新流程                                         │
├──────────────────────────────────────────────────────────┤
│  1. 从 LB 后端池中移除要删除的节点                         │
│  2. 验证 LB 健康检查更新                                  │
│  3. 确认剩余后端正常                                      │
│  4. 执行节点删除                                          │
│                                                            │
│  ⚠️ 删除最后一个控制面后，LB 配置可完全移除                │
└──────────────────────────────────────────────────────────┘
```

---

## 3. 控制面组件顺序退出

### 3.1 组件依赖关系

```
┌───────────────────────────────────────────────────┐
│  组件关闭依赖                                       │
├───────────────────────────────────────────────────┤
│  kube-scheduler                                    │
│       ↓ (无依赖)                                   │
│  kube-controller-manager                           │
│       ↓ (依赖 API Server)                          │
│  kube-apiserver                                    │
│       ↓ (依赖 etcd)                                │
│  etcd                                              │
│       ↓ (最底层)                                   │
└───────────────────────────────────────────────────┘
```

### 3.2 kubeadm reset 的处理

`kubeadm reset` 通过停止 kubelet 来间接停止所有组件：

```go
if err := initSystem.ServiceStop("kubelet"); err != nil {
    klog.Warningf("[reset] The kubelet service could not be stopped: [%v]\n", err)
}
```

**过程**:
1. 停止 kubelet 服务
2. kubelet 停止后，静态 Pod（API Server、Scheduler、CM、etcd）自动终止
3. 删除静态 Pod manifests 确保组件不会重启

**静态 Pod 终止流程**:
```
kubelet 停止
  → 不再监控 /etc/kubernetes/manifests/
  → 已运行的静态 Pod 容器成为孤儿进程
  → reset 删除 manifests 目录内容
  → 删除所有容器（包括静态 Pod 容器）
```

---

## 4. External etcd HA 集群删除

### 4.1 架构差异

```
┌──────────────────────────────────────────────────────┐
│  External etcd HA 集群                                │
├──────────────────────────────────────────────────────┤
│  控制面节点: 无 etcd 静态 Pod                          │
│  etcd 集群: 独立部署（不在 Kubernetes 管理范围内）     │
│                                                        │
│  kubeadm reset:                                       │
│  ├─ 检测不到 etcd.yaml → "Assuming external etcd"     │
│  ├─ 不执行 etcd 成员移除                               │
│  └─ 仅清理控制面节点                                   │
└──────────────────────────────────────────────────────┘
```

### 4.2 手动处理 etcd

```bash
# 1. 列出所有 etcd 成员
ETCDCTL_API=3 etcdctl member list \
  --endpoints=https://etcd1:2379,https://etcd2:2379,https://etcd3:2379 \
  --cacert=/etc/etcd/pki/ca.crt \
  --cert=/etc/etcd/pki/client.crt \
  --key=/etc/etcd/pki/client.key

# 2. 逐个删除控制面节点
for node in cp1 cp2 cp3; do
    ssh $node "kubeadm reset -f"
    kubectl delete node $node
done

# 3. 清理 etcd 数据
for etcd in etcd1 etcd2 etcd3; do
    ssh $etcd "systemctl stop etcd && rm -rf /var/lib/etcd"
done
```

---

## 5. 部分删除（缩小控制面规模）

### 5.1 从 5 节点缩减到 3 节点

```bash
# 1. 确认当前 etcd 健康
etcdctl endpoint health --cluster

# 2. 逐个删除多余的控制面节点
for node in cp4 cp5; do
    # drain 节点
    kubectl drain $node --ignore-daemonsets --delete-emptydir-data

    # 从 API 删除
    kubectl delete node $node

    # 在目标节点上 reset
    ssh $node "kubeadm reset -f"

    # 确认 etcd 成员已移除
    etcdctl member list

    # 确认 etcd 健康
    etcdctl endpoint health --cluster
done
```

### 5.2 更新 Load Balancer

```
删除节点后更新 LB 配置:
├─ HAProxy: 更新 backend server 列表
├─ Nginx: 更新 upstream 块
├─ kube-vip: 自动适应（通过 leader election）
└─ Cloud LB: 更新 target pool
```

---

## 6. 完整 HA 集群销毁流程

```bash
#!/bin/bash
set -e

WORKER_NODES="worker1 worker2 worker3"
CP_NODES="cp1 cp2 cp3"
VIP="192.168.1.100"

echo "=== 开始销毁 HA 集群 ==="

# Step 1: 驱逐并删除所有工作节点
for node in $WORKER_NODES; do
    echo ">>> 处理工作节点: $node"
    kubectl drain $node --ignore-daemonsets --delete-emptydir-data --timeout=60s || true
    kubectl delete node $node || true
done

# Step 2: 逐个删除控制面节点
for node in $CP_NODES; do
    echo ">>> 处理控制面节点: $node"
    kubectl drain $node --ignore-daemonsets --delete-emptydir-data --timeout=60s || true
    kubectl delete node $node || true
    ssh $node "kubeadm reset -f" || true

    # 等待 etcd 稳定
    sleep 10
done

# Step 3: 在最后一个控制面节点上强制 reset
echo ">>> 强制重置最后一个控制面节点"
ssh ${CP_NODES[-1]} "kubeadm reset -f --cleanup-tmp-dir" || true

# Step 4: 清理所有节点
for node in $CP_NODES $WORKER_NODES; do
    echo ">>> 清理节点: $node"
    ssh $node "bash -s" <<'EOF'
        rm -rf /etc/kubernetes/
        rm -rf /var/lib/kubelet/
        rm -rf /var/lib/etcd/
        rm -rf /etc/cni/net.d/
        rm -rf $HOME/.kube/
        iptables -F 2>/dev/null || true
        iptables -t nat -F 2>/dev/null || true
        iptables -t mangle -F 2>/dev/null || true
        ipvsadm -C 2>/dev/null || true
EOF
done

# Step 5: 清理负载均衡器
echo ">>> 清理 Load Balancer"
# (根据实际 LB 类型执行清理)

echo "=== HA 集群已完全销毁 ==="
```

---

## 7. 常见问题

### 7.1 删除控制面后 API Server 不可用

**原因**: etcd 仲裁丢失或 LB 后端池配置不正确。

**处理**: 在剩余控制面节点上直接操作 etcd：
```bash
# 使用 etcdctl 绕过 API Server
etcdctl member list
etcdctl endpoint health --cluster
```

### 7.2 upload-certs 清理

HA 初始化时使用 `kubeadm init --upload-certs` 上传证书到 Secret。reset 不会自动清理这些 Secret：

```bash
# 手动清理（在集群仍可用时）
kubectl delete secret -n kube-system kubeadm-certs
```

### 7.3 kubeadm-config ConfigMap

reset 不删除 `kubeadm-config` ConfigMap，需要手动清理：

```bash
kubectl delete configmap -n kube-system kubeadm-config
```

---

## 参考

- [kubeadm HA 文档](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/high-availability/)
- [etcd 运维指南](https://etcd.io/docs/latest/op-guide/)
- [kubeadm reset 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/reset.go)

## Related

- [[README.md|README]]
- [[man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]

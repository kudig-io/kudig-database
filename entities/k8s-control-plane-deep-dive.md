---
title: 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
description: '# 控制平面深度剖析'
summary: '1. **认证（Authentication）**：x509 证书、Bearer Token、OIDC 等'
category: reference
tags:
- k8s
- control-plane
- apiserver
- scheduler
- kube-controller-manager
- cri
- csi
- cni
- etcd
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
- 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI 是什么
- 如何 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
trigger_keywords:
- 控制平面深度剖析：API
- Server
- Scheduler
- KCM
- CRI
- CSI
- CNI
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制平面深度剖析

## API Server 请求处理链路

每个 API 请求经过四阶段处理：

1. **认证（Authentication）**：x509 证书、Bearer Token、OIDC 等
2. **授权（Authorization）**：RBAC（推荐）、ABAC、Webhook
3. **准入控制（Admission Control）**：
   - Mutating Webhooks：修改请求对象（如注入 sidecar）
   - Validating Webhooks：验证请求合法性（如镜像策略）
4. **持久化**：写入 etcd 并返回响应

## Scheduler 调度算法

两阶段调度流程：

**阶段一：过滤（Filtering）**
- NodeResourcesFit：资源是否满足
- NodeAffinity：节点亲和性
- TaintToleration：污点容忍
- PodTopologySpread：拓扑分布约束

**阶段二：打分（Scoring）**
- LeastRequestedPriority：资源使用率最低
- BalancedResourceAllocation：CPU/Memory 均衡
- ImageLocality：镜像已存在优先

## KCM 控制器清单

kube-controller-manager 内含 30+ 独立控制器：
- Deployment Controller：管理 ReplicaSet 滚动更新
- Node Controller：节点健康检测与驱逐
- Service Controller：云厂商 LB 集成
- Endpoint Controller：维护 Endpoint 对象
- Job Controller：批处理任务生命周期

## CRI/CSI/CNI 三大接口

| 接口 | 职责 | 主流实现 |
|------|------|----------|
| CRI（Container Runtime Interface） | 容器生命周期管理 | containerd, CRI-O |
| CSI（Container Storage Interface） | 存储卷挂载/卸载 | Ceph CSI, EBS CSI, NFS CSI |
| CNI（Container Network Interface） | Pod 网络配置 | Calico, Cilium, Flannel |

三大接口的解耦设计使得 K8s 能够适配不同基础设施，无需修改核心代码。

---

> 来源：.zread/wiki/drafts/7-kong-zhi-ping-mian-shen-du-pou-xi-api-server-scheduler-kcm-yu-cri-csi-cni.md

## Related

- [[deployment]] — Deployment
- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd


<!-- risk-assessed -->

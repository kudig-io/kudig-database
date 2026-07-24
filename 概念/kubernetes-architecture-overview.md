---
title: Kubernetes Architecture Overview
description: '- Kubernetes 架构全景图'
summary: 'Kubernetes follows a layered architecture with seven distinct layers:'
category: concepts
tags:
- k8s
- architecture
- control-plane
- data-plane
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Architecture Overview 是什么
- 如何 Kubernetes Architecture Overview
trigger_keywords:
- Kubernetes
- Architecture
- Overview
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[Kubernetes|Kubernetes]] Architecture Overview

## Layered Architecture

Kubernetes follows a layered architecture with seven distinct layers:

| Layer | Name | Responsibility | Key Components |
|-------|------|----------------|----------------|
| Layer 1 | Orchestration | Scheduling, automation | Scheduler, Controllers |
| Layer 2 | API | Unified entry, auth, admission | API Server, Admission |
| Layer 3 | Data | Persistent state | etcd |
| Layer 4 | Runtime | Container execution | [[kubelet|kubelet]], Container Runtime |
| Layer 5 | Network | Pod networking, load balancing | CNI, kube-proxy |
| Layer 6 | Storage | Persistent volume management | CSI, Volume Plugin |
| Layer 7 | Extension | Custom functionality | CRD, Operator, Webhook |

## Control Plane vs Data Plane

The **control plane** manages cluster state through four core components:
- **kube-apiserver**: Central REST API gateway, handles authentication, authorization, admission control, and persistence to [[etcd|etcd]]
- **etcd**: Distributed key-value store using Raft consensus and MVCC for state persistence
- **kube-scheduler**: Assigns Pods to nodes through a two-phase Filter+Score scheduling algorithm
- **kube-controller-manager**: Runs 40+ built-in controllers maintaining desired state via [[概念/controller-pattern.md|Controller Pattern]]

The **data plane** executes workloads on each node:
- **kubelet**: Node agent managing Pod lifecycle, communicates with API Server via Watch mechanism
- **kube-proxy**: Network proxy implementing Service load balancing (iptables/IPVS/eBPF)
- **Container Runtime**: Executes containers via CRI (containerd or CRI-O since v1.24)

## Communication Pattern

All components communicate exclusively through the API Server -- no direct component-to-component calls. This loose coupling enables independent upgrades and fault isolation. The Watch mechanism (HTTP chunked streaming based on etcd revisions) provides real-time state synchronization.

## Design Principles

Kubernetes is built on core principles: declarative API, controller reconciliation loop, loose coupling, extensibility (CRI/CNI/CSI/Device Plugin), self-healing, horizontal scaling, immutable infrastructure, and eventual consistency.

## 源码实现分析

### 组件通信架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Control Plane                            │
│  ┌─────────────┐    ┌───────────┐    ┌─────────────────┐  │
│  │ kube-       │◄──►│   etcd    │    │ kube-scheduler  │  │
│  │ apiserver   │    │ (Raft)    │    │ (Watch+Bind)    │  │
│  │             │◄───────────────────►│                 │  │
│  │             │◄───────────────────►├─────────────────┤  │
│  │  Admission  │    └───────────┘    │ controller-mgr  │  │
│  │  Webhooks   │                     │ (40+ controllers)│  │
│  └──────┬──────┘                     └─────────────────┘  │
└─────────┼─────────────────────────────────────────────────┘
          │ HTTPS (Watch/List/Update)
          ▼
┌─────────────────────────────────────────────────────────────┐
│                    Data Plane (per Node)                    │
│  ┌─────────┐    ┌───────────┐    ┌─────────────────────┐  │
│  │ kubelet  │───►│ containerd│───►│ runc/crun (OCI)     │  │
│  │ (CRI)    │    │ (CRI)     │    │ Pod sandbox+ctrs    │  │
│  └─────────┘    └───────────┘    └─────────────────────┘  │
│  ┌─────────┐    ┌───────────────────────────────────────┐  │
│  │kube-proxy│    │ CNI plugin (Calico/Cilium/Flannel)    │  │
│  │(iptables)│    │ veth pair + bridge/eBPF               │  │
│  └─────────┘    └───────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### API Server 请求处理链

```go
// kubernetes/pkg/kubeapiserver 简化请求处理流程
func (s *APIServer) HandleRequest(req *http.Request) {
    // 1. Authentication: X.509 / Token / OIDC / Webhook
    user := s.authenticator.Authenticate(req)
    // 2. Authorization: RBAC / ABAC / Webhook
    s.authorizer.Authorize(user, req.Resource, req.Verb)
    // 3. Admission Control: Mutating → Validating
    obj = s.mutatingAdmission.Admit(obj)   // 修改对象（如注入 sidecar）
    s.validatingAdmission.Admit(obj)       // 拒绝不合规对象
    // 4. Validation + Persist to etcd
    s.storage.Create(ctx, key, obj)        // etcd Put with revision
    // 5. Notify watchers (HTTP chunked streaming)
    s.watchCache.NotifyAll(obj, revision)
}
```

## 使用场景

### 场景一：集群健康检查

```bash
# 🟢 低风险 - 控制平面组件状态
kubectl get componentstatuses
kubectl get --raw='/readyz?verbose' | grep -v ok

# 🟢 低风险 - etcd 集群健康
kubectl -n kube-system exec etcd-master-0 -- etcdctl endpoint health

# 🟢 低风险 - 节点状态总览
kubectl get nodes -o wide
kubectl top nodes
```

### 场景二：查看组件间通信

```bash
# 🟢 低风险 - API Server 审计日志查看请求来源
kubectl logs -n kube-system kube-apiserver-master-0 | grep "user-agent"

# 🟢 低风险 - 查看 Watch 连接数
kubectl get --raw='/metrics' | grep apiserver_watch_events
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 组件间直接通信 | 所有组件仅通过 API Server 通信，无直接连接 |
| etcd 存储所有数据 | etcd 只存 K8s 对象状态，容器日志/镜像不在其中 |
| kubelet 主动拉取配置 | kubelet 通过 Watch 机制被动接收 API Server 推送 |
| 控制平面不能调度 Pod | 通过去除 Taint 可在控制平面节点调度工作负载 |
| kube-proxy 是真正的代理 | kube-proxy 只维护 iptables/IPVS 规则，流量不经过它 |
| 单节点 etcd 可用于生产 | 生产必须 3/5 节点 etcd，单节点无容错能力 |

## 面试要点

1. **K8s 为什么所有组件都通过 API Server 通信？** — 解耦设计：任何组件可独立升级/重启而不影响其他组件；统一认证授权审计；Watch 机制提供实时状态同步；etcd 只被 API Server 访问，减少并发压力。

2. **控制平面客单点故障如何处理？** — API Server 无状态可多副本 + LB；etcd 用 Raft 共识（3节点容1故障）；Scheduler/Controller-Manager 用 Lease 选举（leader election），故障后 15s 内切换。

3. **一个 kubectl apply 的完整链路？** — kubectl → API Server（Auth→Authz→Admission→Validate）→ etcd 写入 → Watch 通知 Scheduler → Bind 到节点 → kubelet Watch 到新 Pod → CRI 创建容器 → CNI 配置网络 → CSI 挂载存储 → 状态回报。

4. **K8s 扩展点有哪些？** — CRI（容器运行时）、CNI（网络）、CSI（存储）、Device Plugin（GPU/FPGA）、Admission Webhook（准入控制）、CRD + Operator（自定义控制器）、Scheduler Framework（调度插件）。

## Related

- [[pod-lifecycle]] — Pod Lifecycle
- [[概念/declarative-api.md|declarative-api]] — Declarative API
- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[etcd]] — etcd
- [[概念/controller-pattern.md|Controller Pattern]]
- [[概念/declarative-api.md|Declarative API]]
- [[概念/watch-mechanism.md|Watch Mechanism]]
- [[etcd|etcd]]
- [[实体/kube-apiserver.md|kube-apiserver]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[实体/kubelet.md|kubelet]]
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]

- Kubernetes 架构全景图
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
- [[实体/metal3-io.md|Metal3]] — Cross-reference
- [[实体/clusterpedia.md|Clusterpedia]] — Cross-reference


<!-- risk-assessed -->

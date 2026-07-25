---
title: Kubernetes 组件交互关系与数据流向源码分析
description: 基于 kubernetes-1.36.2 源码的组件通信矩阵、Pod 创建全链路数据流、节点侧 kubelet/kube-proxy 协作与跨域机制串联
summary: 以「创建一个 Deployment」为主线串联 apiserver/KCM/scheduler/kubelet/kube-proxy 的完整数据流，给出组件通信矩阵、hub-and-spoke 架构的源码依据与各技术域的机制衔接点。
category: source-analysis
tags:
- k8s
- source-code
- dataflow
- kubelet
- kube-proxy
- architecture
- interaction
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 组件之间如何通信
- 创建 Pod 的完整数据流源码
- kubelet syncLoop 工作原理
- kube-proxy 如何感知 Service 变化
trigger_keywords:
- 数据流
- 组件交互
- syncLoop
- hub-and-spoke
- Pod 创建流程
- PLEG
related_domains:
- 集群基础
- 工作负载
- 网络
- 存储
- 可观测性
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# Kubernetes 组件交互关系与数据流向源码分析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/`
> 本篇是 kubernetes-core 系列的收束篇：前六篇拆解单组件，本篇把它们接成一张图。

## 一、Hub-and-Spoke：唯一的架构不变量

Kubernetes 组件间**没有任何直接调用**——所有交互都经过 apiserver 以「写状态 / 观察状态」间接完成：

```
                          ┌────────────┐
      watch Pods(未调度)   │            │   watch Pods(本节点)
  scheduler ◀────────────▶│ apiserver  │◀────────────▶ kubelet ──CRI──▶ containerd
      bind(写)             │  (etcd)    │   status(写)          └─CNI/CSI 插件
                          │            │
  KCM 40+控制器 ◀────────▶ │            │ ◀────────────▶ kube-proxy
   watch/写各资源          └────────────┘   watch Service/EndpointSlice
```

这个设计的源码级依据：

- scheduler 不通知 kubelet：它只写 Binding（`schedule_one.go` bindingCycle:397），kubelet 靠 watch `spec.nodeName=自己` 的 Pod 发现新任务
- KCM 不调用 scheduler：ReplicaSet 控制器创建的 Pod `nodeName` 为空（`replica_set.go` manageReplicas:649），scheduler 靠 watch 未调度 Pod 接手
- 组件死活互不影响正确性：任何组件重启后从 List-Watch 重建视图（[[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]），继续调谐——**状态在 etcd，组件皆无状态**

### 组件通信矩阵

| 发起方 → 接收方 | 协议/机制 | 端口 | 认证 |
|----------------|----------|------|------|
| 一切组件 → apiserver | HTTPS REST + Watch | 6443 | 证书/SA Token |
| apiserver → etcd | gRPC | 2379 | 双向 TLS |
| apiserver → kubelet | HTTPS（exec/logs/port-forward 反向通道） | 10250 | apiserver 客户端证书 |
| apiserver → webhook | HTTPS（准入/转换回调） | 用户定义 | CA bundle |
| kubelet → 运行时 | CRI gRPC | unix socket | 本地 |
| kubelet → CNI 插件 | 二进制 exec + stdin JSON | — | 本地 |
| kubelet → CSI | gRPC unix socket | — | 本地 |

唯二的「apiserver 主动外呼」是 kubelet 反向通道与 webhook——这两条也是网络策略/防火墙配置最容易遗漏的路径（表现为 `kubectl exec` 超时、webhook 超时导致资源无法创建）。

---

## 二、主线数据流：kubectl create deployment 之后的 8 步

每一步都标注源码落点（行号均已实测，详见对应分篇）：

```
① kubectl → apiserver: POST Deployment
   Filter链(config.go:1036) → Store.Create(store.go:454) → etcd(:274)          [02篇]
② Deployment 控制器: watch 到新 Deployment
   syncDeployment(deployment_controller.go:589) → 创建 ReplicaSet              [03篇]
③ ReplicaSet 控制器: syncReplicaSet(:755) → manageReplicas(:649)
   → 慢启动批量 POST Pod (nodeName="")                                        [03篇]
④ scheduler: activeQ 弹出 Pod → schedulingCycle(schedule_one.go:175)
   → Filter/Score → assume(:1108) → 异步 bind(:397) 写 nodeName                [04篇]
⑤ kubelet(目标节点): watch 到 spec.nodeName=自己 的 Pod
   syncLoop(kubelet.go:2620) → syncLoopIteration(:2695) → podWorkers
   → SyncPod(:2019): 建 sandbox(CNI 分配 IP) → 挂卷(CSI) → 起容器(CRI)
⑥ kubelet 状态回写: PATCH pods/status (PodIP、containerStatuses、Ready)
⑦ EndpointSlice 控制器(KCM): watch Ready Pod → 更新 EndpointSlice              [03篇]
⑧ kube-proxy(全部节点): watch Service/EndpointSlice
   → syncProxyRules(经 BoundedFrequencyRunner, iptables/proxier.go:312)
   → 更新 iptables/ipvs/nftables 规则 → 流量可达
```

**排障即定位断点**：Pod 卡在哪一步，就用哪一篇的排障表。②③不动看 KCM（选主丢失？队列积压?），④看调度（Pending 原因），⑤看 kubelet/运行时/CNI/CSI，⑧看 kube-proxy 与内核规则。这一主线与 [[19-故障诊断/README.md|故障诊断域]] 的症状树互为映照。

---

## 三、节点侧协作细节

### 3.1 kubelet 事件源与 syncLoop

```go
// pkg/kubelet/kubelet.go:2695（实测行号）
func (kl *Kubelet) syncLoopIteration(ctx, configCh, handler, syncCh, housekeepingCh, plegCh) bool {
    select {
    case u := <-configCh:        // 三大 Pod 来源合流: apiserver watch / 静态Pod文件 / http
    case e := <-plegCh:          // PLEG: 运行时容器状态变化 (relist 对比生成事件)
    case <-syncCh:               // 1s 周期兜底同步
    case <-housekeepingCh:       // 2s 周期清理 (孤儿Pod/卷/目录)
    }
}
```

静态 Pod（控制面组件自身的部署方式）走 configCh 的 file 来源，再由 kubelet 向 apiserver 注册 mirror pod——这解释了「删除 mirror pod 会立即重建、必须改 manifest 文件才能真正变更控制面」。`PLEG is not healthy` 则是 plegCh 生产端（运行时 relist）超时的直接表现，矛头指向容器运行时而非 kubelet 本身（详见 [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|Kubelet Deep Dive]] 与 [[14-容器运行时/README.md|容器运行时域]]）。

### 3.2 kube-proxy 的防抖同步

```go
// pkg/proxy/iptables/proxier.go:312（实测行号）
proxier.syncRunner = runner.NewBoundedFrequencyRunner(
    "sync-runner", proxier.syncProxyRules, minSyncPeriod, syncPeriod, ...)
```

Service/EndpointSlice 事件不直接触发规则重写，而是经 BoundedFrequencyRunner 合并防抖（默认 minSync 1s）——大规模 Endpoint 抖动时规则更新是批量的，`sync_proxy_rules_duration_seconds` 与端点收敛延迟由此解释。三种模式（iptables/ipvs/nftables）同构，仅规则生成不同（`pkg/proxy/{iptables,ipvs,nftables}/`），横向对比见 [[05-网络/01-K8s网络核心/index.md|网络域：K8s 网络核心]] 与 [[01-集群基础/03-控制平面/16-kube-proxy-deep-dive.md|Kube Proxy Deep Dive]]。

---

## 四、跨技术域机制衔接总表

kubernetes-core 系列与知识库各域的接口点，按域归类：

| 技术域 | 源码机制衔接点 | 本系列出处 |
|--------|---------------|-----------|
| [[01-集群基础/README.md|集群基础]] | 三层 apiserver、选主、调度框架 | 02/03/04 篇 |
| [[02-工作负载/README.md|工作负载]] | Deployment/RS/StatefulSet 调谐、Pod 删除排序 | 03 篇 |
| [[05-网络/README.md|网络]] | kube-proxy 同步模型、CNI 调用点(SyncPod sandbox 阶段)、EndpointSlice 流水线 | 本篇 |
| [[06-存储/README.md|存储]] | 卷挂载在 SyncPod 的次序(卷先于容器)、CSI gRPC 边界、nodevolumelimits 调度插件 | 本篇/04 篇 |
| [[08-安全/README.md|安全]] | 认证授权 Filter 顺序、NodeRestriction、RBAC 授权器位置 | 02 篇 |
| [[09-可观测性/README.md|可观测性]] | workqueue_*/scheduler_*/etcd_* 指标的源码含义 | 03/04/05 篇 |
| [[12-可靠性/README.md|可靠性]] | 选主空窗计算、etcd 仲裁与 NOSPACE、抢占与 PDB | 03/04/05 篇 |
| [[14-容器运行时/README.md|容器运行时]] | CRI 协议(k8s.io/cri-api)、PLEG | 本篇 |
| [[15-AI基础设施/README.md|AI基础设施]] | DRA/gangscheduling 调度插件 | 04 篇 |
| [[19-故障诊断/README.md|故障诊断]] | 8 步主线 = 排障断点定位法 | 本篇 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/README.md|kubernetes-core 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/01-source-tree-architecture.md|01 - 源码整体架构与目录结构]]
- [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 - kube-apiserver 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md|03 - KCM 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/04-kube-scheduler-deep-dive.md|04 - kube-scheduler 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/05-etcd-storage-deep-dive.md|05 - etcd 与存储链路源码剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 - 声明式 API 与 Informer 机制源码剖析]]
- [[01-集群基础/03-控制平面/02-plane-components-interaction.md|控制平面组件交互]]（运维视角）
- [[01-集群基础/01-架构总览/01-kubernetes-architecture-overview.md|Kubernetes 架构总览]]

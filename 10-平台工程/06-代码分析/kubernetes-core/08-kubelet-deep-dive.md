---
title: kubelet 源码深度剖析
description: 基于 kubernetes-1.36.2 源码的 kubelet 事件循环、podWorkers、kuberuntime SyncPod、PLEG、卷管理、探针、驱逐与状态回写完整剖析
summary: 逐层拆解 kubelet 从 syncLoop 事件源到 CRI 调用的完整执行链：podWorkers 每 Pod 串行化、computePodActions 决策表、PLEG relist、volumemanager 调谐、prober/eviction/status 各子管理器，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- kubelet
- pleg
- podworkers
- cri
- eviction
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 35min
intent_queries:
- kubelet 源码执行流程
- podWorkers 工作原理
- PLEG is not healthy 源码定位
- kubelet 如何调用 CRI 创建容器
- kubelet 驱逐机制源码
trigger_keywords:
- kubelet
- syncLoop
- podWorkers
- PLEG
- computePodActions
- volumemanager
- eviction
- 探针
related_domains:
- 集群基础
- 工作负载
- 容器运行时
- 存储
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kubelet 源码深度剖析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/pkg/kubelet/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|控制平面：Kubelet Deep Dive]]（运维视角）
> 系列上下文：[[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 篇]]已给出 syncLoop 四事件源骨架，本篇向下展开每一层。

## 概述

kubelet 是唯一「既 watch apiserver 又操作本机内核/运行时」的组件，其内部是十余个子管理器围绕一条主事件循环的协作体：

```
配置源(apiserver/file/http) ─┐
PLEG(运行时事件)            ─┼─▶ syncLoop(:2620) ─▶ podWorkers(每 Pod 一 goroutine)
周期同步(1s)/清理(2s)       ─┘         │                    │
                                      │              kubelet.SyncPod(:2019)
   statusManager ◀── 状态回写 ────────┘                    │
   volumeManager / probeManager / evictionManager    kuberuntime.SyncPod(:1450)
   containerManager(cgroup) / deviceManager                │ CRI gRPC
                                                     containerd / CRI-O
```

---

## 一、podWorkers：每 Pod 串行化状态机

```go
// pkg/kubelet/pod_workers.go（实测行号）
func (p *podWorkers) UpdatePod(ctx context.Context, options UpdatePodOptions)      // :751
func (p *podWorkers) podWorkerLoop(parentCtx, podUID, podUpdates <-chan struct{})  // :1231
```

- **每个 Pod UID 一个 goroutine + 单槽位待处理更新**：同一 Pod 的多次事件在 worker 忙碌时合并为「最新一次」，与 client-go WorkQueue 去重语义同构（[[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]）
- 状态机三阶段 `syncing → terminating → terminated`：保证「先停容器、再卸卷、最后清理 cgroup」的顺序不可逆——Pod 删除卡住时 `kubectl get pod -o json` 里的 phase 与该状态机阶段一一对应
- 不同 Pod 完全并行：单个 Pod 的慢操作（如镜像拉取）不阻塞其他 Pod

## 二、kuberuntime：SyncPod 的真正执行者

kubelet.SyncPod（`kubelet.go:2019`）做准入与环境准备（cgroup、目录、卷等待），然后把「让容器达到期望态」全权委托给运行时管理器：

```go
// pkg/kubelet/kuberuntime/kuberuntime_manager.go（实测行号）
func (m *kubeGenericRuntimeManager) computePodActions(...) podActions   // :1175 决策
func (m *kubeGenericRuntimeManager) SyncPod(...)                        // :1450 执行
```

`computePodActions` 是一张纯函数决策表——对比 Pod Spec 与 CRI 实际状态，产出：是否重建 sandbox、杀哪些容器、起哪些容器。几个高频行为的源码解释：

| 现象 | 决策依据 |
|------|---------|
| 改 Pod 网络相关字段后容器全重启 | sandbox 变更 ⇒ `CreateSandbox=true`，全部容器随之重建 |
| 容器 hash 变化即重启 | 容器 spec 的 hash 与运行中容器 annotation 不一致 ⇒ 加入 kill 列表 |
| initContainer 失败反复重跑 | init 未完成 ⇒ 只调度下一个 init，主容器列表为空 |

`SyncPod`(:1450) 按固定顺序执行：**杀 sandbox → 杀多余容器 → 建 sandbox（CNI 在此发生）→ 起 ephemeral → 起 init → 起主容器**，每步经 CRI gRPC（`staging/src/k8s.io/cri-client/pkg/remote_runtime.go` RunPodSandbox:220）落到 containerd/CRI-O，协议细节见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|生态篇：容器运行时与 CRI]]。

## 三、PLEG：运行时状态的事件化

```go
// pkg/kubelet/pleg/generic.go（实测行号）
func (g *GenericPLEG) Start()   // :161  每 1s 触发一次 Relist
func (g *GenericPLEG) Relist()  // :292  全量列出 CRI 容器 → 与上次快照 diff → 生成事件
```

- Relist 对比新旧快照产出 `ContainerStarted/ContainerDied/...` 事件推入 plegCh，驱动 syncLoop
- **`PLEG is not healthy: pleg was last seen active Xs ago`**：Relist 超过 3 分钟未完成——每次 Relist 都要调 CRI `ListPodSandbox/ListContainers`，运行时 hang、节点容器数过多、磁盘 IO 卡顿都会命中。该报错矛头永远指向运行时/内核而非 kubelet 逻辑
- Evented PLEG（beta）用 CRI 事件流替代轮询，将容器状态感知延迟从秒级降到毫秒级

## 四、volumeManager：卷的独立调谐循环

```go
// pkg/kubelet/volumemanager/reconciler/reconciler.go（实测行号）
func (rc *reconciler) Run(ctx, stopCh)   // :26  100ms 周期
func (rc *reconciler) reconcile(ctx)     // :33  对比 desired/actual 两个世界状态
```

desiredStateOfWorld（从 podManager 算出该挂什么）与 actualStateOfWorld（实际挂了什么）双缓存对比，差异触发 attach/mount/unmount/detach。**SyncPod 只是等待者**：`WaitForAttachAndMount` 超时报 `Unable to attach or mount volumes` 时，真正的失败在本调谐循环（或外部 CSI 链路，见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/03-csi-storage-drivers.md|生态篇：CSI 存储驱动]]）。

## 五、探针、状态与驱逐

### 5.1 probeManager

```go
// pkg/kubelet/prober/worker.go:215（实测行号）
func (w *worker) doProbe(ctx) (keepGoing bool)
```

每个容器×每种探针（liveness/readiness/startup）一个独立 worker goroutine。结果只改内部缓存：liveness 失败 → 通知 podWorkers 重启容器；readiness 变化 → statusManager 更新 Ready condition → EndpointSlice 摘除流量。**探针执行在 kubelet 进程内**（exec 经 CRI、http/tcp 由 kubelet 直连 PodIP），节点级网络策略拦截 kubelet→Pod 流量会造成「应用正常但探针失败」。

### 5.2 statusManager：唯一的状态出口

```go
// pkg/kubelet/status/status_manager.go（实测行号）
func (m *manager) SetPodStatus(logger, pod, status)  // :464 各子系统写入
func (m *manager) syncPod(ctx, uid, status)          // :1151 PATCH pods/status 回 apiserver
```

所有子管理器的状态变更汇聚于此、去重后按序回写——这就是 8 步主线（07 篇）第⑥步的实现体。节点状态则由 `kubelet_node_status.go` syncNodeStatus:452 / tryUpdateNodeStatus:489 以 10s 周期（配合 Lease 心跳）独立上报。

### 5.3 evictionManager：节点自保

```go
// pkg/kubelet/eviction/eviction_manager.go:248（实测行号）
func (m *managerImpl) synchronize(ctx, diskInfoProvider, podFunc) ([]*v1.Pod, error)
```

10s 周期对比信号（memory.available、nodefs、imagefs、pid）与阈值：软阈值走优雅驱逐，硬阈值立即杀。排序规则：**先看是否超 request，再看 Priority，最后看超出量**——这解释了为什么 Guaranteed Pod 最后被驱逐，以及为何「request 设得准」本身就是抗驱逐能力。与 API 发起的 Eviction（PDB 保护）是两条完全独立的路径：节点压力驱逐**不经过 PDB**。

## 六、containerManager 与 deviceManager

- `pkg/kubelet/cm/container_manager_linux.go` Start:640：cgroup 树管理（node → kubepods → QoS 级 → Pod 级）、系统预留（system/kube-reserved）落地为 cgroup limit
- `pkg/kubelet/cm/devicemanager/manager.go` Allocate:366 / AllocatePod:1126：Device Plugin（GPU 等）的分配入口，插件经 unix socket 注册，调度器只看数量、实际设备绑定在此完成——GPU Pod 卡 `ContainerCreating` 且事件含 device 字样时查此链路，联动 [[15-AI基础设施/README.md|AI 基础设施域]]

## 七、生产排障速查

| 症状 | 源码定位 | 检查手段 |
|------|---------|---------|
| PLEG is not healthy | Relist (generic.go:292) 超时 | 运行时响应（`crictl ps` 耗时）、容器数、磁盘 IO |
| Pod 卡 Terminating | podWorkerLoop (:1231) terminating 阶段阻塞 | 容器停不掉（unmount 失败/进程 D 状态）、finalizer |
| 卷挂载超时 | volumemanager reconcile (:33) | `kubectl describe pod` 事件 + CSI 驱动日志 |
| 探针失败但应用正常 | doProbe (:215) 网络路径 | kubelet→PodIP 连通性、超时参数 |
| 频繁 OOM 前驱逐 | eviction synchronize (:248) | 阈值配置、`kubectl describe node` 压力条件 |
| 状态长时间不更新 | statusManager syncPod (:1151) 积压 | kubelet→apiserver QPS 限制、节点心跳 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/README.md|kubernetes-core 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]（syncLoop 骨架）
- [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|生态篇 01 - 容器运行时与 CRI 集成]]
- [[10-平台工程/06-代码分析/kubernetes-ecosystem/03-csi-storage-drivers.md|生态篇 03 - CSI 存储驱动集成]]
- [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|控制平面：Kubelet Deep Dive]]
- [[14-容器运行时/README.md|容器运行时域]]
- [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|Pod 生命周期与事件]]

---
title: 38 - Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism)
description: '# 38 - Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism)'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- kubelet
- prometheus
- statefulset
- daemonset
- crd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism) 是什么
- 如何 Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism)
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Terway
- GC
- 垃圾回收
- 机制详解
- Terway
- Garbage
- Collection
- Mechanism
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md
  label: '故障树: terway'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
created: "2026-05-23"
---

# 38 - Terway GC (垃圾回收) 机制详解 (Terway [[domain-17-system-foundation/topic-dictionary/fundamentals/garbage-collection.md|Garbage Collection]] Mechanism)

> **适用版本**: 阿里云 ACK v1.26 - v1.32 | **Terway 版本**: v1.5+ | **最后更新**: 2026-04

---

<!-- chunk: 1. GC 机制设计思想 -->
## 1. GC 机制设计思想

### 1.1 为什么需要 GC

在 Terway CNI 的 ENIIP 模式下，Pod 与 VPC 内的 ENI 辅助 IP 直接绑定。当 Pod 被删除、驱逐或异常退出时，理论上其占用的 IP 应当及时归还 IP 池。但在以下场景中，正常的 IP 释放流程可能失效：

| 场景 | 原因 | 后果 |
|:---|:---|:---|
| [[kubelet|kubelet]] 强制驱逐 | 节点压力大，跳过 CNI DEL 回调 | IP 残留在 ENI 辅助 IP 列表 |
| 节点异常重启 | Terway Agent 进程未优雅退出 | 本地 IPAM 状态丢失 |
| Terway Agent 重启/升级 | 内存状态与持久化状态不一致 | 孤儿 IP 无法追踪 |
| CRD Finalizer 阻塞 | PodENI/IPInstance 删除卡住 | IP 永久占用 |
| 阿里云 API 超时/失败 | 网络抖动导致辅助 IP 释放失败 | 云平台记录与本地不一致 |

GC 机制的核心目标：**周期性对账，发现并清理孤儿资源，确保 IP 池与 ENI 资源的最终一致性**。

### 1.2 设计原则

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Terway GC 设计原则                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   1. 最终一致性 (Eventual Consistency)                                       │
│      ┌──────────────────────────────────────────────────────────────┐       │
│      │ Kubernetes Pod 状态 ←── 对账 ──→ VPC ENI/IP 实际状态        │       │
│      │ 两侧数据源定期对比，差异部分作为 GC 候选                      │       │
│      └──────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   2. 安全优先 (Safety First)                                                │
│      ┌──────────────────────────────────────────────────────────────┐       │
│      │ - 多轮确认: IP 必须连续 N 个 GC 周期被标记为孤儿才触发清理   │       │
│      │ - 宽限期: 新分配 IP 在 grace period 内不参与 GC 判定          │       │
│      │ - 白名单: ReservedIP / 固定 IP 跳过 GC                       │       │
│      └──────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   3. 最小影响 (Minimal Impact)                                              │
│      ┌──────────────────────────────────────────────────────────────┐       │
│      │ - GC 操作在 Terway Agent 后台 goroutine 异步执行             │       │
│      │ - 限速清理: 单次 GC 周期最多清理 N 个资源，避免云 API 雪崩   │       │
│      │ - 退避策略: 清理失败后指数退避重试                            │       │
│      └──────────────────────────────────────────────────────────────┘       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. GC 架构与核心组件 -->
## 2. GC 架构与核心组件

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         Terway GC 整体架构                                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                      Terway Control Plane                                │   │
│   │                                                                          │   │
│   │   ┌──────────────────┐    ┌──────────────────┐    ┌─────────────────┐   │   │
│   │   │  GC Controller   │    │ ENI Reconciler   │    │ IP Reconciler   │   │   │
│   │   │ (周期性对账)      │    │ (ENI 生命周期)   │    │ (IP 生命周期)   │   │   │
│   │   └────────┬─────────┘    └────────┬─────────┘    └────────┬────────┘   │   │
│   │            │                       │                       │            │   │
│   │            └───────────────────────┼───────────────────────┘            │   │
│   │                                    │                                    │   │
│   │                          ┌─────────▼─────────┐                          │   │
│   │                          │   Resource Store   │                          │   │
│   │                          │  (CRD 状态缓存)    │                          │   │
│   │                          └─────────┬─────────┘                          │   │
│   └────────────────────────────────────┼────────────────────────────────────┘   │
│                                        │                                        │
│   ┌────────────────────────────────────┼────────────────────────────────────┐   │
│   │                      Terway Agent (每节点)                               │   │
│   │                                    │                                    │   │
│   │   ┌──────────────────┐    ┌───────▼────────┐    ┌──────────────────┐   │   │
│   │   │  Local IPAM      │    │  GC Worker     │    │  ENI Manager     │   │   │
│   │   │ (本地 IP 分配表) │◄──►│ (本地 GC 执行) │◄──►│ (ENI 辅助 IP)   │   │   │
│   │   └────────┬─────────┘    └───────┬────────┘    └────────┬─────────┘   │   │
│   │            │                      │                      │             │   │
│   └────────────┼──────────────────────┼──────────────────────┼─────────────┘   │
│                │                      │                      │                  │
│                ▼                      ▼                      ▼                  │
│   ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────────┐     │
│   │  Kubernetes API  │    │  CRI Runtime     │    │  Alibaba Cloud API  │     │
│   │  (Pod/CRD 状态)  │    │  (容器真实状态)  │    │  (ENI/IP 云端状态)  │     │
│   └──────────────────┘    └──────────────────┘    └──────────────────────┘     │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 GC 涉及的资源类型

| 资源类型 | 位置 | GC 对象 | 清理动作 |
|:---|:---|:---|:---|
| **ENI 辅助 IP** | 阿里云 VPC | 未关联任何 Pod 的辅助 IP | 调用 `UnassignPrivateIpAddresses` 释放 |
| **ENI** | 阿里云 VPC | 无任何辅助 IP 且未被节点使用的 ENI | 调用 `DetachNetworkInterface` + `DeleteNetworkInterface` |
| **PodENI CRD** | Kubernetes | 对应 Pod 已不存在的 PodENI | 删除 CRD 对象 |
| **IPInstance CRD** | Kubernetes | 对应 Pod 已不存在的 IPInstance | 删除 CRD 对象，释放关联 IP |
| **ReservedIP CRD** | Kubernetes | 超过保留时长的 ReservedIP | 根据 `reclaimPolicy` 释放或保留 |
| **本地 IPAM 缓存** | Terway Agent 内存 | 与 Pod/CRI 不一致的分配记录 | 清除本地记录 |

---

<!-- chunk: 3. GC 相关配置详解 -->
## 3. GC 相关配置详解

### 3.1 eni-config ConfigMap — GC 关键参数

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",

      # ============ IP 池管理 (直接影响 GC 行为) ============

      # ENI 辅助 IP 池最大值
      # 当池中空闲 IP 超过此值时，GC 会主动释放多余 IP 回 VPC
      # 生产建议: 根据节点 Pod 密度设置，通常 20-30
      "max_pool_size": 25,

      # ENI 辅助 IP 池最小值
      # GC 清理后池中保留的最少空闲 IP，确保新 Pod 快速分配
      # 生产建议: max_pool_size 的 30%-50%
      "min_pool_size": 10,

      # ============ vSwitch 与安全组 ============

      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx1"],
        "cn-hangzhou-i": ["vsw-xxx2"]
      },
      "security_groups": ["sg-xxxxxxxxxx"],
      "service_cidr": "172.21.0.0/20",

      # ============ GC 相关高级参数 ============

      # IP GC 最小间隔 (秒)
      # 两次 GC 扫描之间的最小时间间隔，防止频繁 GC 导致 API 压力
      # 默认值: 300 (5 分钟)
      # 生产建议: 大规模集群 (>100 节点) 可调大至 600
      "gc_min_interval": 300,

      # GC 宽限期 (秒)
      # IP 分配后在此时间内不参与 GC 判定，避免误回收正在启动的 Pod IP
      # 默认值: 120 (2 分钟)
      # 生产建议: 如果集群有大镜像/Init 容器较慢，可调大至 300
      "gc_grace_period": 120,

      # 单次 GC 最大清理数量
      # 单个 GC 周期内最多清理的 IP/ENI 数量
      # 默认值: 5
      # 生产建议: 小集群可保持默认; 大集群 IP 泄漏严重时可临时调大
      "gc_max_cleanup_per_cycle": 5,

      # ENI 空闲超时 (秒)
      # ENI 上所有辅助 IP 释放后，ENI 保持挂载的时间
      # 超过此时间后 GC 会 Detach 并删除该 ENI
      # 默认值: 600 (10 分钟)
      # 生产建议: 频繁扩缩容场景可调大至 1800 (30 分钟) 减少 ENI 反复创删
      "eni_idle_timeout": 600,

      # 是否启用热插拔
      "hot_plug": true
    }
```

### 3.2 terway-controlplane ConfigMap — 控制面 GC 参数

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-controlplane
  namespace: kube-system
data:
  terway-controlplane: |
    {
      # ============ CRD 级别 GC 配置 ============

      # IPInstance GC 间隔 (秒)
      # 控制面扫描 IPInstance CRD 检查孤儿资源的周期
      # 默认值: 600 (10 分钟)
      "ipinstance_gc_interval": 600,

      # PodENI GC 间隔 (秒)
      # 控制面扫描 PodENI CRD 检查孤儿资源的周期
      # 默认值: 600 (10 分钟)
      "podeni_gc_interval": 600,

      # ReservedIP 过期检查间隔 (秒)
      # 默认值: 3600 (1 小时)
      "reservedip_expiry_check_interval": 3600,

      # GC 并行度
      # 控制面同时处理 GC 清理任务的并发数
      # 默认值: 2
      "gc_concurrency": 2
    }
```

### 3.3 Terway [[DaemonSet|DaemonSet]] 启动参数 — GC 相关 Flag

```bash
# 查看 Terway DaemonSet 的 GC 相关启动参数
kubectl get ds terway -n kube-system -o jsonpath='{.spec.template.spec.containers[?(@.name=="terway")].args}' | jq .
```

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--gc-min-interval` | `300s` | GC 扫描最小间隔 |
| `--gc-grace-period` | `120s` | 新分配 IP 的 GC 豁免期 |
| `--gc-max-backoff` | `600s` | GC 失败后最大退避时间 |
| `--enable-ip-gc` | `true` | 是否启用 IP 级别 GC |
| `--enable-eni-gc` | `true` | 是否启用 ENI 级别 GC |
| `--gc-stale-threshold` | `2` | 孤儿资源需连续被标记的次数才触发清理 |

### 3.4 配置参数关系图

```
                    ┌───────────────────────────────────────────┐
                    │            GC 参数协作关系                 │
                    └───────────────────────────────────────────┘

  gc_min_interval ──► 控制扫描频率 ──┐
         (300s)                      │
                                     ▼
  gc_grace_period ──► 新 IP 豁免 ──► GC 扫描判定 ──► 标记孤儿
         (120s)                      ▲                  │
                                     │              gc_stale_threshold (2次)
  max_pool_size  ──► 池上限判定 ─────┘                  │
  min_pool_size  ──► 池下限保护                         ▼
                                                  触发清理动作
                                                        │
  gc_max_cleanup_per_cycle ──► 单次限额 ────────────────┤
         (5个)                                          │
                                                        ▼
  eni_idle_timeout ──► ENI 空闲判定 ──► ENI 回收 ──► 完成
         (600s)
```

---

<!-- chunk: 4. GC 触发链路与执行流程 -->
## 4. GC 触发链路与执行流程

### 4.1 源码默认配置 (Source Code Defaults)

> **源码位置**: [`daemon/daemon.go`](https://github.com/AliyunContainerService/terway/blob/main/daemon/daemon.go)

Terway GC 的核心时长配置定义在源码 `daemon/daemon.go` 的常量块中：

```go
// 源码: github.com/AliyunContainerService/terway/daemon/daemon.go
const (
    gcPeriod    = 5 * time.Minute   // GC 扫描周期，每 5 分钟执行一次
    listTimeout = 60 * time.Second  // List 操作超时时间，防止 API 调用挂起
)
```

**GC 循环启动入口** (源码: `daemon/builder.go`)：

```go
// builder.go 中启动 GC 循环
if b.config.IPAMType != types.IPAMTypeCRD {
    go b.service.startGarbageCollectionLoop(b.ctx)
}

// startGarbageCollectionLoop 使用 wait.PollUntilContextCancel 实现周期循环
func (n *networkService) startGarbageCollectionLoop(ctx context.Context) {
    _ = wait.PollUntilContextCancel(ctx, gcPeriod, true, func(ctx context.Context) (done bool, err error) {
        err = n.gcPods(ctx)
        if err != nil {
            serviceLog.Error(err, "error garbage collection")
        }
        return false, nil
    })
}
```

**IP 保留时长** (源码: `pkg/k8s/k8s.go`)：

```go
const (
    defaultStickTimeForSts = 5 * time.Minute  // StatefulSet Pod 删除后 IP 保留时长
)
```

**完整默认值汇总**：

| 常量/变量 | 源码位置 | 默认值 | 说明 |
|:---|:---|:---|:---|
| `gcPeriod` | `daemon/daemon.go` | **5 分钟** | GC 扫描周期，`wait.PollUntilContextCancel` 的 interval 参数 |
| `listTimeout` | `daemon/daemon.go` | **60 秒** | List Pod/资源时的超时保护 |
| `defaultStickTimeForSts` | `pkg/k8s/k8s.go` | **5 分钟** | 有状态工作负载 Pod 删除后 IP 保留时间 |
| 新 Pod 跳过阈值 | `daemon/daemon.go` gcPods() | **2 分钟** | `time.Since(createTime) < 2*time.Minute` 的 Pod 跳过规则同步 |
| resourceDB 路径 | `daemon/daemon.go` | `/var/lib/cni/terway/ResRelation.db` | BoltDB 持久化的资源关系数据库 |
| 泄漏规则清理开关 | `daemon/daemon.go` gcPods() | `TERWAY_GC_RULES=true` | 环境变量控制，默认关闭，仅首次 GC 执行一次 |

> **关键实现细节**：
> - `gcPeriod` 是**硬编码常量**，不可通过 ConfigMap 动态调整，修改需重新编译或等待上游更新
> - `wait.PollUntilContextCancel` 的第三个参数 `immediate=true`，意味着 **Agent 启动后立即执行第一次 GC**
> - GC 函数 `gcPods()` 执行时持有 **写锁 (n.Lock())**，会阻塞同时段的 AllocIP/ReleaseIP 请求
> - 泄漏规则清理 `gcLeakedRules()` 使用 `sync.Once` 确保仅在首次 GC 时执行，清理 iptables 规则和 TC 过滤器

### 4.2 GC 触发时机

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     GC 触发时机矩阵                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  触发方式 1: 周期性定时器 (Primary)                               │      │
│   │  ┌────────────┐     ┌────────────┐     ┌────────────┐           │      │
│   │  │ GC Timer   │────►│ 检查间隔   │────►│ 执行 GC    │           │      │
│   │  │ 启动       │     │ ≥ gc_min_  │     │ Scan       │           │      │
│   │  │            │     │  interval  │     │            │           │      │
│   │  └────────────┘     └────────────┘     └────────────┘           │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  触发方式 2: 事件驱动 (Reactive)                                  │      │
│   │  ┌────────────┐     ┌────────────┐     ┌────────────┐           │      │
│   │  │ Pod Delete │────►│ CNI DEL    │────►│ 即时释放   │           │      │
│   │  │ Event      │     │ 回调       │     │ IP         │           │      │
│   │  └────────────┘     └────────────┘     └──────┬─────┘           │      │
│   │                                               │ 失败            │      │
│   │                                               ▼                 │      │
│   │                                        ┌────────────┐           │      │
│   │                                        │ 标记待 GC  │           │      │
│   │                                        └────────────┘           │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  触发方式 3: Terway Agent 启动对账 (Startup)                      │      │
│   │  ┌────────────┐     ┌────────────┐     ┌────────────┐           │      │
│   │  │ Agent 启动 │────►│ 全量对账   │────►│ 清理残留   │           │      │
│   │  │ /重启      │     │ Pod vs IP  │     │ 孤儿 IP    │           │      │
│   │  └────────────┘     └────────────┘     └────────────┘           │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  触发方式 4: IP 池水位告警 (Pool-driven)                          │      │
│   │  ┌────────────┐     ┌────────────┐     ┌────────────┐           │      │
│   │  │ 空闲 IP >  │────►│ 触发缩容   │────►│ 释放多余   │           │      │
│   │  │ max_pool   │     │ GC         │     │ 辅助 IP    │           │      │
│   │  └────────────┘     └────────────┘     └────────────┘           │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 IP GC 完整执行流程 (gcPods 函数)

> **源码**: `daemon/daemon.go` → `gcPods()` | `daemon/daemon_linux.go` → `gcPolicyRoutes()`, `gcLeakedRules()`

```
┌─────────────────────────────────────────────────────────────────────────────┐
│           gcPods() 完整执行流程 (源码实现)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   Phase 1: 获取写锁 + 数据采集                                              │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │                                                                  │       │
│   │   n.Lock()                   // 持有写锁，阻塞 AllocIP/ReleaseIP │       │
│   │                                                                  │       │
│   │   ① k8s.GetLocalPods()        ② resourceDB.List()               │       │
│   │      (从本节点 kubelet 获取)     (从 BoltDB 获取)                │       │
│   │      ↓                            ↓                              │       │
│   │   exist = map[podID]bool       podResources = [...]              │       │
│   │   existIPs = sets.Set[string]  (包含 Resources, PodInfo, NetConf)│       │
│   │                                                                  │       │
│   │   // 只统计未退出 sandbox 的 Pod                                 │       │
│   │   for pod in localPods:                                          │       │
│   │     if !pod.SandboxExited:                                       │       │
│   │       exist[podID] = true                                        │       │
│   │       existIPs.Insert(pod.IPv4)                                  │       │
│   │                                                                  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 2: 遍历对账 (for podRes in podResources)                            │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │                                                                  │       │
│   │   ① Pod 仍在运行 (exist[podID] == true):                        │       │
│   │     if createTime < 2min ago → skip (新 Pod 不同步规则)          │       │
│   │     else → ruleSync(ctx, podRes)  // 同步策略路由规则            │       │
│   │                                                                  │       │
│   │   ② Pod 本节点不存在 (exist[podID] == false):                    │       │
│   │     k8s.PodExist() → 再次通过 API Server 确认                    │       │
│   │       │                                                          │       │
│   │       ├─ Pod 仍存在 → skip (可能在其他节点)                      │       │
│   │       │                                                          │       │
│   │       └─ Pod 真正不存在 → 触发清理                               │       │
│   │                                                                  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 3: 执行清理 (对已确认不存在的 Pod)                                  │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │                                                                  │       │
│   │   for resource in podRes.Resources:                              │       │
│   │     1. gcPolicyRoutes(mac, containerIP)  → 清理策略路由          │       │
│   │        (通过 datapath.PolicyRoute.Teardown 清除 veth/路由规则)   │       │
│   │     2. eniMgr.Release(cni, resource)     → 释放网络资源          │       │
│   │        (归还 IP 到 ENI 空闲池 或调用云 API 释放)                 │       │
│   │     3. deletePodResource(podID)           → 从 resourceDB 删除   │       │
│   │                                                                  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
│   Phase 4: 可选泄漏规则清理                                                 │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │                                                                  │       │
│   │   if os.Getenv("TERWAY_GC_RULES") == "true":                    │       │
│   │     gcRulesOnce.Do(func() {                                      │       │
│   │       gcLeakedRules(existIPs)    // 仅执行一次 (sync.Once)       │       │
│   │     })                                                           │       │
│   │     // 清理 IPVLAN 路由 + TC 过滤器中引用不存在 IP 的规则       │       │
│   │                                                                  │       │
│   │   cleanRuntimeNode(ctx, uidInLocal)  // 清理节点运行时记录       │       │
│   │   n.Unlock()                         // 释放写锁                 │       │
│   │                                                                  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.4 ENI GC 执行流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     ENI GC 执行流程                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌────────────────┐                                                        │
│   │ 扫描节点上所有  │                                                        │
│   │ 挂载的 ENI      │                                                        │
│   └───────┬────────┘                                                        │
│           │                                                                  │
│           ▼                                                                  │
│   ┌────────────────────┐     Yes     ┌────────────────────┐                 │
│   │ ENI 有辅助 IP ?    │────────────►│ 跳过，交给 IP GC   │                 │
│   └───────┬────────────┘             └────────────────────┘                 │
│           │ No                                                               │
│           ▼                                                                  │
│   ┌────────────────────┐     No      ┌────────────────────┐                 │
│   │ 空闲时间 >         │────────────►│ 保留，下次再检查   │                 │
│   │ eni_idle_timeout ? │             └────────────────────┘                 │
│   └───────┬────────────┘                                                    │
│           │ Yes                                                              │
│           ▼                                                                  │
│   ┌────────────────────┐     No      ┌────────────────────┐                 │
│   │ 剩余 ENI > 1 ?    │────────────►│ 保留最后一个 ENI   │                 │
│   │ (保留主 ENI)       │             │ 确保节点可用       │                 │
│   └───────┬────────────┘             └────────────────────┘                 │
│           │ Yes                                                              │
│           ▼                                                                  │
│   ┌────────────────────┐                                                    │
│   │ DetachNetworkInterface                                                  │
│   │ → 等待 Detach 完成                                                      │
│   │ → DeleteNetworkInterface                                                │
│   │ → 清理 NodeNetworking CRD                                              │
│   │ → 记录审计日志                                                          │
│   └────────────────────┘                                                    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.5 CRD GC 执行流程 (控制面)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CRD GC 执行流程 (Terway Controller)                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  IPInstance GC                                                    │      │
│   │                                                                   │      │
│   │  1. List 所有 IPInstance CRD                                     │      │
│   │  2. 对每个 IPInstance:                                           │      │
│   │     a. 检查 spec.pod.name + spec.pod.namespace → 查询 Pod       │      │
│   │     b. 如果 Pod 不存在且 IPInstance 超过 grace period:           │      │
│   │        - 移除 Finalizer                                          │      │
│   │        - 调用 UnassignPrivateIpAddresses 释放 IP                 │      │
│   │        - 删除 IPInstance CRD                                     │      │
│   │     c. 如果 Pod 存在但 UID 不匹配 (Pod 被重建):                 │      │
│   │        - 视为孤儿，执行清理                                      │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  PodENI GC                                                        │      │
│   │                                                                   │      │
│   │  1. List 所有 PodENI CRD                                        │      │
│   │  2. 对每个 PodENI:                                               │      │
│   │     a. 检查 owner Pod 是否存在                                   │      │
│   │     b. 如果 Pod 不存在:                                          │      │
│   │        - 释放关联的 ENI 资源                                     │      │
│   │        - 移除 Finalizer，删除 PodENI CRD                        │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────┐      │
│   │  ReservedIP GC                                                    │      │
│   │                                                                   │      │
│   │  1. List 所有 ReservedIP CRD                                    │      │
│   │  2. 对每个 ReservedIP:                                           │      │
│   │     a. 如果 retention.enabled == true:                           │      │
│   │        - 检查 Pod 是否存在                                       │      │
│   │        - Pod 不存在时检查 expirationTimestamp                    │      │
│   │        - 超过 retention.duration → 根据 reclaimPolicy 处理      │      │
│   │           Retain  → 保留 IP 不释放                               │      │
│   │           Delete  → 释放 IP 并删除 CRD                          │      │
│   │     b. 如果 retention.enabled == false:                          │      │
│   │        - Pod 不存在时立即释放                                    │      │
│   └──────────────────────────────────────────────────────────────────┘      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 5. GC 时机判断与清理策略 -->
## 5. GC 时机判断与清理策略

### 5.1 IP 是否为孤儿的判定条件

```
IP 被判定为孤儿需同时满足以下所有条件:

  ┌─ 条件 1: 分配时间 > gc_grace_period (已过宽限期)
  │
  ├─ 条件 2: 无 Running Pod 引用该 IP
  │           (通过 Kubernetes API + CRI 双重确认)
  │
  ├─ 条件 3: 不在 ReservedIP 白名单中
  │
  ├─ 条件 4: 连续 gc_stale_threshold 次 GC 周期被标记
  │           (防止瞬态不一致导致误清理)
  │
  └─ 条件 5: IP 不属于正在创建中的 Pod
              (检查 Pod phase != Pending with scheduled)
```

### 5.2 IP 池水位调控策略

| 场景 | 当前空闲 IP | 动作 | 说明 |
|:---|:---|:---|:---|
| 空闲不足 | < `min_pool_size` | 向 VPC 申请新辅助 IP | 预热扩容 |
| 空闲正常 | `min_pool_size` ~ `max_pool_size` | 不操作 | 正常水位 |
| 空闲过多 | > `max_pool_size` | GC 释放多余 IP 至 `max_pool_size` | 缩容回收 |
| 零空闲且满额 | 0，且总 IP = 节点上限 | 告警，等待 Pod 删除释放 | 容量上限 |

### 5.3 清理优先级

```
高 ← ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ → 低

  1. 标记次数最多的          2. 分配时间最早的
     孤儿 IP                    孤儿 IP
     (最大 gc_mark_count)       (最久未使用)

  3. 已无辅助 IP 的          4. 池中超限的
     空闲 ENI                   多余空闲 IP
     (资源浪费最大)             (缩容回收)
```

---

<!-- chunk: 6. 运维操作手册 -->
## 6. 运维操作手册

### 6.1 GC 状态监控

```bash
# ============================================
# 查看 Terway GC 日志
# ============================================

# 查看最近的 GC 执行日志
kubectl logs -n kube-system -l app=terway -c terway --tail=200 | \
  grep -E 'GC|gc|garbage|orphan|stale|cleanup|reclaim'

# 查看特定节点的 GC 日志
NODE="cn-hangzhou.192.168.1.10"
TERWAY_POD=$(kubectl get pods -n kube-system -l app=terway \
  --field-selector spec.nodeName=${NODE} -o jsonpath='{.items[0].metadata.name}')
kubectl logs -n kube-system ${TERWAY_POD} -c terway --tail=200 | \
  grep -E 'GC|gc|orphan|stale'

# 查看 GC 相关事件
kubectl get events -n kube-system --field-selector \
  reason=GCSucceeded,reason=GCFailed --sort-by='.lastTimestamp'
```

### 6.2 IP 泄漏检测脚本

```bash
#!/bin/bash
# detect-ip-leak.sh - 检测 Terway IP 泄漏
# 用法: ./detect-ip-leak.sh [node-name]

set -euo pipefail

NODE_NAME="${1:-}"
echo "=== Terway IP Leak Detection ==="
echo "Timestamp: $(date)"
echo "Target Node: ${NODE_NAME:-ALL}"
echo ""

# Step 1: 收集已分配 IP (从 IPInstance CRD)
echo "[Step 1] 收集 IPInstance CRD 中的 IP 分配记录..."
if [ -n "${NODE_NAME}" ]; then
  ALLOCATED_IPS=$(kubectl get ipinstances -A -o json | \
    jq -r --arg node "${NODE_NAME}" \
    '.items[] | select(.status.nodeName == $node) | 
     "\(.spec.ip.ipv4)\t\(.spec.pod.namespace)/\(.spec.pod.name)\t\(.spec.pod.uid)"')
else
  ALLOCATED_IPS=$(kubectl get ipinstances -A -o json | \
    jq -r '.items[] | 
     "\(.spec.ip.ipv4)\t\(.spec.pod.namespace)/\(.spec.pod.name)\t\(.status.nodeName)"')
fi

ALLOCATED_COUNT=$(echo "${ALLOCATED_IPS}" | grep -c '.' || echo 0)
echo "  已分配 IP 数量: ${ALLOCATED_COUNT}"

# Step 2: 收集运行中 Pod 的 IP
echo ""
echo "[Step 2] 收集运行中 Pod 的 IP..."
if [ -n "${NODE_NAME}" ]; then
  RUNNING_POD_IPS=$(kubectl get pods -A --field-selector \
    spec.nodeName=${NODE_NAME},status.phase=Running \
    -o jsonpath='{range .items[*]}{.status.podIP}{"\n"}{end}' | sort -u)
else
  RUNNING_POD_IPS=$(kubectl get pods -A --field-selector \
    status.phase=Running \
    -o jsonpath='{range .items[*]}{.status.podIP}{"\n"}{end}' | sort -u)
fi

RUNNING_COUNT=$(echo "${RUNNING_POD_IPS}" | grep -c '.' || echo 0)
echo "  运行中 Pod IP 数量: ${RUNNING_COUNT}"

# Step 3: 对比找出孤儿 IP
echo ""
echo "[Step 3] 检测孤儿 IP (已分配但无运行中 Pod)..."
echo "${ALLOCATED_IPS}" | while IFS=$'\t' read -r ip pod_info extra; do
  if [ -n "${ip}" ] && ! echo "${RUNNING_POD_IPS}" | grep -q "^${ip}$"; then
    echo "  ⚠️  孤儿 IP: ${ip} | 原 Pod: ${pod_info} | ${extra}"
  fi
done

# Step 4: 检查 ReservedIP 状态
echo ""
echo "[Step 4] 检查 ReservedIP 保留情况..."
kubectl get reservedips -A -o json | jq -r '
  .items[] | 
  "  保留 IP: \(.spec.ip.ipv4) | Pod: \(.spec.association.podName // "未关联") | 保留时长: \(.spec.retention.duration // "无限") | 策略: \(.spec.reclaimPolicy)"'

echo ""
echo "=== 检测完成 ==="
```

### 6.3 手动触发 GC

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ============================================
# 方法 1: 重启特定节点的 Terway Pod (触发启动对账)
# ============================================
NODE="cn-hangzhou.192.168.1.10"
TERWAY_POD=$(kubectl get pods -n kube-system -l app=terway \
  --field-selector spec.nodeName=${NODE} -o jsonpath='{.items[0].metadata.name}')

# 优雅重启 (推荐)
kubectl delete pod -n kube-system ${TERWAY_POD}
# Terway 重启后会执行一次全量对账 GC

# ============================================
# 方法 2: 通过 terway-cli 手动清理 (精确控制)
# ============================================

# 查看当前 IP 分配状态
kubectl exec -n kube-system ${TERWAY_POD} -c terway -- terway-cli show

# 查看疑似泄漏的 IP
kubectl exec -n kube-system ${TERWAY_POD} -c terway -- terway-cli show | \
  grep -E 'orphan|stale|unassigned'

# ============================================
# 方法 3: 手动清理孤儿 IPInstance CRD
# ============================================

# 列出所有没有对应 Pod 的 IPInstance
kubectl get ipinstances -A -o json | jq -r '
  .items[] | 
  select(.spec.pod.name != null) |
  "\(.metadata.name)\t\(.spec.pod.namespace)\t\(.spec.pod.name)"' | \
while IFS=$'\t' read -r name ns pod; do
  if ! kubectl get pod ${pod} -n ${ns} &>/dev/null; then
    echo "孤儿 IPInstance: ${name} (原 Pod: ${ns}/${pod})"
    # 取消注释下行执行清理 (谨慎操作)
    # kubectl delete ipinstance ${name}
  fi
done
```

### 6.4 调整 GC 参数

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
# ============================================
# 场景 1: 加速 GC (IP 泄漏严重时临时调整)
# ============================================
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson | 
    .gc_min_interval = 60 |
    .gc_max_cleanup_per_cycle = 20 |
    .gc_grace_period = 60 |
    tostring)' | kubectl apply -f -

# 重启 Terway 使配置生效
kubectl rollout restart ds/terway -n kube-system

# ⚠️ 注意: 问题解决后务必恢复默认值，避免 API 压力

# ============================================
# 场景 2: 大规模集群优化 GC (减少 API 压力)
# ============================================
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson | 
    .gc_min_interval = 600 |
    .gc_max_cleanup_per_cycle = 3 |
    .eni_idle_timeout = 1800 |
    tostring)' | kubectl apply -f -

kubectl rollout restart ds/terway -n kube-system
```

---

<!-- chunk: 7. 常见问题与处理方案 -->
## 7. 常见问题与处理方案

### 7.1 问题速查表

| 问题 | 现象 | 根因 | 处理方案 |
|:---|:---|:---|:---|
| IP 泄漏累积 | 节点可用 IP 持续减少，Pod Pending | GC 未正常执行或阈值过高 | 检查 GC 日志，降低 `gc_min_interval`，手动触发 GC |
| GC 误回收 | 正在启动的 Pod IP 被回收 | `gc_grace_period` 过短 | 增大 `gc_grace_period` 至 300s |
| ENI 反复创删 | 频繁的 ENI Attach/Detach 操作 | `eni_idle_timeout` 过短 | 增大 `eni_idle_timeout` 至 1800s |
| GC 执行失败 | 日志出现 `GC.*failed` | 阿里云 API 限流或权限不足 | 检查 RAM 权限，增大 `gc_max_backoff` |
| CRD Finalizer 阻塞 | IPInstance/PodENI 无法删除 | Terway Controller 异常 | 重启 Controller，必要时手动移除 Finalizer |
| 固定 IP 被意外回收 | StatefulSet Pod 重建后 IP 变化 | ReservedIP 过期或策略错误 | 检查 `retention.duration`，确认 `reclaimPolicy: Retain` |
| 阿里云 API 超时 | GC 清理操作长时间无响应 | VPC API 限流或网络问题 | 检查节点到 VPC API 连通性，增大重试间隔 |

### 7.2 IP 泄漏紧急处理流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
#!/bin/bash
# emergency-ip-gc.sh - IP 泄漏紧急处理
# ⚠️ 仅在 IP 泄漏导致 Pod 大面积 Pending 时使用

echo "=== Terway IP 泄漏紧急处理 ==="
echo "时间: $(date)"
echo ""

# Step 1: 确认问题规模
echo "[Step 1] 确认 Pod Pending 规模..."
PENDING_COUNT=$(kubectl get pods -A --field-selector status.phase=Pending \
  -o json | jq '[.items[] | select(.status.conditions[]? | 
  select(.reason == "Unschedulable" and (.message | test("IP|ip|ENI"))))] | length')
echo "  IP 不足导致 Pending 的 Pod 数: ${PENDING_COUNT}"

# Step 2: 检查各节点 IP 使用率
echo ""
echo "[Step 2] 各节点 IP 使用情况..."
kubectl get nodes -o json | jq -r '
  .items[] | 
  "\(.metadata.name)\t\(.metadata.annotations["k8s.aliyun.com/allocated-eniips"] // "N/A")"' | \
  column -t -s $'\t'

# Step 3: 识别 IP 泄漏最严重的节点
echo ""
echo "[Step 3] 识别泄漏节点..."
kubectl get ipinstances -A -o json | jq -r '
  [.items[] | {node: .status.nodeName, ip: .spec.ip.ipv4, pod: .spec.pod.name, ns: .spec.pod.namespace}] |
  group_by(.node) | .[] | 
  {node: .[0].node, total_ips: length, 
   orphans: [.[] | select(.pod != null) | 
   select(.pod as $p | .ns as $n | 
   ([$p, $n] | join("/")))] | length} |
  "\(.node)\tTotal: \(.total_ips)\tOrphans: \(.orphans)"' | \
  sort -t$'\t' -k3 -rn | column -t -s $'\t'

# Step 4: 临时加速 GC
echo ""
echo "[Step 4] 临时加速 GC 配置..."
echo "  正在调低 gc_min_interval 至 60s..."
echo "  正在调高 gc_max_cleanup_per_cycle 至 20..."
echo "  ⚠️ 请执行以下命令应用配置:"
echo ""
cat << 'CMDS'
# 临时加速 GC (问题解决后恢复)
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson | 
    .gc_min_interval = 60 | .gc_max_cleanup_per_cycle = 20 | tostring)' | \
  kubectl apply -f -
kubectl rollout restart ds/terway -n kube-system

# 恢复默认 GC 配置
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson | 
    .gc_min_interval = 300 | .gc_max_cleanup_per_cycle = 5 | tostring)' | \
  kubectl apply -f -
kubectl rollout restart ds/terway -n kube-system
CMDS
```

### 7.3 CRD Finalizer 阻塞处理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# ============================================
# 检查被 Finalizer 阻塞的 CRD
# ============================================

# 查找有 Finalizer 但对应 Pod 不存在的 IPInstance
kubectl get ipinstances -A -o json | jq -r '
  .items[] | select(.metadata.finalizers != null and (.metadata.finalizers | length > 0)) |
  select(.metadata.deletionTimestamp != null) |
  "\(.metadata.name)\t\(.metadata.finalizers | join(","))\t\(.metadata.deletionTimestamp)"' | \
  column -t -s $'\t'

# 查找被阻塞的 PodENI
kubectl get podenis -A -o json | jq -r '
  .items[] | select(.metadata.finalizers != null and (.metadata.finalizers | length > 0)) |
  select(.metadata.deletionTimestamp != null) |
  "\(.metadata.namespace)/\(.metadata.name)\t\(.metadata.finalizers | join(","))"' | \
  column -t -s $'\t'

# ============================================
# 手动移除 Finalizer (⚠️ 最后手段，确认 IP 已释放后操作)
# ============================================

# 移除 IPInstance Finalizer
kubectl patch ipinstance <name> --type='json' \
  -p='[{"op": "remove", "path": "/metadata/finalizers"}]'

# 移除 PodENI Finalizer
kubectl patch podeni <name> -n <namespace> --type='json' \
  -p='[{"op": "remove", "path": "/metadata/finalizers"}]'
```

---

<!-- chunk: 8. 监控与告警 -->
## 8. 监控与告警

### 8.1 GC 关键指标

| 指标名称 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| `terway_gc_total` | Counter | GC 执行总次数 | - |
| `terway_gc_duration_seconds` | Histogram | GC 执行耗时 | P99 > 30s 告警 |
| `terway_gc_orphan_ips` | Gauge | 当前检测到的孤儿 IP 数 | > 10 告警 |
| `terway_gc_cleaned_ips` | Counter | GC 已清理的 IP 总数 | - |
| `terway_gc_errors_total` | Counter | GC 失败次数 | 5 分钟内 > 3 告警 |
| `terway_ip_pool_available` | Gauge | IP 池当前可用 IP 数 | < min_pool_size 告警 |
| `terway_eni_count` | Gauge | 节点当前 ENI 数量 | 接近实例上限告警 |

### 8.2 Prometheus 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-gc-alerts
  namespace: monitoring
spec:
  groups:
    - name: terway-gc
      rules:
        # IP 池枯竭告警
        - alert: TerwayIPPoolExhausted
          expr: terway_ip_pool_available == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "节点 {{ $labels.node }} Terway IP 池已耗尽"
            description: "IP 池可用 IP 为 0，持续 5 分钟。新 Pod 将无法调度。"

        # 孤儿 IP 累积告警
        - alert: TerwayOrphanIPsAccumulating
          expr: terway_gc_orphan_ips > 10
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} 存在 {{ $value }} 个孤儿 IP"
            description: "孤儿 IP 持续累积，GC 可能未正常工作。"

        # GC 持续失败告警
        - alert: TerwayGCFailure
          expr: increase(terway_gc_errors_total[10m]) > 5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} Terway GC 持续失败"
            description: "10 分钟内 GC 失败 {{ $value }} 次，请检查阿里云 API 权限和网络。"

        # GC 执行耗时过长
        - alert: TerwayGCSlowExecution
          expr: histogram_quantile(0.99, terway_gc_duration_seconds_bucket) > 30
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} GC 执行耗时过长"
            description: "GC P99 延迟 {{ $value }}s，可能受 API 限流影响。"
```

---

<!-- chunk: 9. 生产最佳实践 -->
## 9. 生产最佳实践

| 类别 | 建议 | 说明 |
|:---|:---|:---|
| **GC 间隔** | 生产环境保持 300s 默认值 | 过短增加 API 压力，过长延迟清理 |
| **宽限期** | 大镜像/慢启动场景调至 300s | 避免正在拉镜像的 Pod 被误回收 |
| **ENI 空闲** | 频繁扩缩容设为 1800s | 减少 ENI 反复创建/删除的 API 开销 |
| **池大小** | min=Pod密度*30%, max=Pod密度*60% | 平衡预热速度与资源浪费 |
| **清理限额** | 默认 5，紧急时可临时调至 20 | 控制单次 GC 的 API 调用量 |
| **监控** | 配置 orphan IP 和 pool 水位告警 | 第一时间发现 GC 异常 |
| **固定 IP** | 使用 ReservedIP + reclaimPolicy:Retain | 防止 StatefulSet IP 被 GC 回收 |
| **审计** | 定期执行 IP 泄漏检测脚本 | 配合自动化巡检任务运行 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 36-api-gateway-patterns
- 37-terway-resources-crud-operations
- 39-csi-cni-version-matrix
- 40-terway-product-overview

## Related

- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]

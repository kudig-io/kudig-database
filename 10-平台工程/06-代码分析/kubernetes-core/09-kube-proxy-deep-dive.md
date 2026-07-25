---
title: kube-proxy 源码深度剖析
description: 基于 kubernetes-1.36.2 源码的 kube-proxy 三模式（iptables/ipvs/nftables）规则同步、Service/EndpointSlice 变更追踪与防抖机制完整剖析
summary: 剖析 kube-proxy 的事件驱动架构：ServiceChangeTracker/EndpointsChangeTracker 增量缓存、BoundedFrequencyRunner 防抖、三种模式 syncProxyRules 的规则生成差异与规模化瓶颈，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- kube-proxy
- iptables
- ipvs
- nftables
- endpointslice
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 25min
intent_queries:
- kube-proxy 源码工作原理
- iptables 与 ipvs 模式源码差异
- syncProxyRules 什么时候触发
- Service 流量转发规则如何生成
trigger_keywords:
- kube-proxy
- syncProxyRules
- iptables
- ipvs
- nftables
- conntrack
- EndpointSlice
related_domains:
- 集群基础
- 网络
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kube-proxy 源码深度剖析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/pkg/proxy/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/16-kube-proxy-deep-dive.md|控制平面：Kube Proxy Deep Dive]] · [[05-网络/01-K8s网络核心/index.md|网络域：K8s 网络核心]]

## 概述

kube-proxy 是「Service 抽象」的节点侧执行者：watch Service/EndpointSlice/Node，把虚拟 VIP 翻译成内核转发规则。它**不在数据路径上**——规则写入内核后，流量转发完全由 netfilter/IPVS 完成，kube-proxy 挂掉只影响规则更新、不影响已有转发。

```
apiserver ──watch──▶ ServiceConfig/EndpointSliceConfig
                          │ OnServiceAdd(:461) / OnEndpointSliceAdd(:494)
                          ▼
            ServiceChangeTracker / EndpointsChangeTracker（增量缓存）
                          │ 触发
                          ▼
            BoundedFrequencyRunner（防抖, proxier.go:312）
                          │ ≥minSyncPeriod 才执行
                          ▼
            syncProxyRules（全量重算 → 原子写内核）
       iptables(:638) │ ipvs(:674) │ nftables(:1062)
```

---

## 一、事件接入与增量缓存

```go
// pkg/proxy/iptables/proxier.go（实测行号）
func (proxier *Proxier) OnServiceAdd(service *v1.Service)                      // :461
func (proxier *Proxier) OnEndpointSliceAdd(endpointSlice *discovery.EndpointSlice) // :494
```

事件处理只做一件事：把变更记入 ChangeTracker（`pkg/proxy/servicechangetracker.go` / `endpointschangetracker.go`）然后请求一次 Sync。**规则永远全量重算**——ChangeTracker 的作用是把「apiserver 对象模型」预翻译为「proxy 内部模型」（ServicePortName → 端点列表），并合并未消费的连续变更。这与 Informer「事件只是触发器」的哲学一致（[[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]）。

## 二、BoundedFrequencyRunner：两侧夹逼的防抖

```go
// pkg/proxy/iptables/proxier.go:312（实测行号）
proxier.syncRunner = runner.NewBoundedFrequencyRunner(
    "sync-runner", proxier.syncProxyRules, minSyncPeriod, syncPeriod, ...)
```

- **上界** `syncPeriod`（默认 30s）：即使无事件也周期全量同步，兜底修复被外部篡改的规则
- **下界** `minSyncPeriod`（默认 1s）：事件再密集也不高于此频率执行——大规模滚动更新时端点抖动被批量合并
- 生产调参含义：`sync_proxy_rules_duration_seconds` 高且节点 Service 多 → 调大 minSyncPeriod 换吞吐；端点收敛慢 → 检查是否被防抖合并所致

## 三、三种模式的 syncProxyRules

```go
// 实测行号
pkg/proxy/iptables/proxier.go:638   func (proxier *Proxier) syncProxyRules() (retryError error)
pkg/proxy/ipvs/proxier.go:674      func (proxier *Proxier) syncProxyRules() (retryError error)
pkg/proxy/nftables/proxier.go:1062 func (proxier *Proxier) syncProxyRules() (retryError error)
```

三者骨架完全同构（读 ChangeTracker 快照 → 生成规则 → 原子提交 → 清理 conntrack），差异只在规则表达：

| | iptables | ipvs | nftables |
|---|---------|------|----------|
| 数据结构 | 线性规则链（KUBE-SERVICES → KUBE-SVC-* → KUBE-SEP-*） | 内核哈希表（virtual server → real server） | verdict map（O(1) 查找） |
| 匹配复杂度 | O(Service 数) 线性匹配 | O(1) | O(1) |
| 更新方式 | `iptables-restore` 全量原子替换 | 增量 netlink 调用 + 少量 iptables（SNAT 标记） | `nft -f` 原子替换，仅重写变更部分 |
| 负载均衡算法 | random（statistic 模块概率分流） | rr/lc/sh 等可选 | random |
| 规模瓶颈 | 数千 Service 后规则生成/匹配开销显著 | 端点数十万级仍稳定 | 设计目标即大规模，1.33+ GA |

**iptables 模式的经典生产事故链**：Service 数过多 → `iptables-restore` 输入巨大 → 同步耗时超过 syncPeriod → 规则长期滞后 → 流量打到已死端点。迁移 ipvs/nftables 或启用 `minimizeIPTablesRestore`（仅写变更链）是标准解法。

## 四、conntrack 清理与会话陷阱

规则写完后 syncProxyRules 末尾会清理失效端点的 conntrack 表项——**UDP Service 端点变更后旧连接黑洞**（DNS 最常见）正是此逻辑覆盖不到的场景之一：已建立的 conntrack 条目指向死端点，若清理遗漏则客户端持续超时。排查手段：`conntrack -L -p udp --dport 53` 对照当前 EndpointSlice。

其他源码级行为要点：

- `externalTrafficPolicy: Local`：规则只指向本节点端点，无端点则丢弃（NodePort 健康检查端口由 kube-proxy 起 HTTP server 供 LB 探测）——保源 IP 的代价是节点间负载不均
- `sessionAffinity: ClientIP`：iptables 用 recent 模块、ipvs 用 persistence timeout 实现
- kernelspace（Windows）与三种 Linux 模式并列，本篇不展开

## 五、生产排障速查

| 症状 | 源码定位 | 检查手段 |
|------|---------|---------|
| Service 不通但 Pod 直连通 | syncProxyRules 未生成规则 | `iptables-save \| grep <svc>` / `ipvsadm -Ln`、kube-proxy 日志 |
| 规则更新滞后 | 同步耗时 > syncPeriod | `sync_proxy_rules_duration_seconds`、Service/端点规模 |
| 端点收敛慢 | BoundedFrequencyRunner 防抖 (:312) | minSyncPeriod 配置、`sync_proxy_rules_last_queued_timestamp` |
| UDP/DNS 间歇超时 | conntrack 残留 | `conntrack -L` 对照 EndpointSlice |
| 保源 IP 后负载不均 | externalTrafficPolicy=Local 语义 | 端点分布、LB 健康检查 |
| NodePort 部分节点不通 | Local 策略无本地端点 | healthCheckNodePort 探测结果 |

kube-proxy 之外的替代数据面（Cilium eBPF kube-proxy replacement、服务网格 sidecar 拦截）见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/02-cni-network-plugins.md|生态篇：CNI 网络插件]] 与 [[10-平台工程/06-代码分析/kubernetes-ecosystem/04-service-mesh-integration.md|生态篇：服务网格集成]]。

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/README.md|kubernetes-core 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]（8 步主线第⑧步）
- [[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md|03 - KCM 源码深度剖析]]（EndpointSlice 控制器一侧）
- [[01-集群基础/03-控制平面/16-kube-proxy-deep-dive.md|控制平面：Kube Proxy Deep Dive]]
- [[05-网络/01-K8s网络核心/index.md|网络域：K8s 网络核心]]
- [[19-故障诊断/README.md|故障诊断域]]

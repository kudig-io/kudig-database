---
title: 大规模集群总览与规模基线
description: 大规模 Kubernetes 集群的规模分级、官方上限、架构设计原则与容量基线，为集群规划提供决策框架
summary: 定义大规模集群的分级标准与架构原则，给出单集群上限、拆分时机与容量基线参考
category: references
tags:
- k8s
- large-scale
- capacity-planning
- architecture
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 15min
---

# 大规模集群总览与规模基线

> 本文是大规模 Kubernetes 生产最佳实践专题的开篇：先定义"多大算大"，再给出架构原则与容量基线，作为后续四大领域最佳实践的决策框架。

## 1. 规模分级与官方上限

### 1.1 Kubernetes 官方单集群上限（现行官方文档口径，v1.36）

| 维度 | 官方支持上限 | 备注 |
|---|---|---|
| 节点数 | 5,000 | 超出后调度器/控制面性能不可保证 |
| Pod 总数 | 150,000 | — |
| 容器总数 | 300,000 | — |
| 单节点 Pod 数 | 110 | kubelet `maxPods` 默认值，可调但需评估 |
| Namespace 数 | 10,000 | 大量 NS 会放大 LIST 开销 |

官方同时给出两条**可伸缩性 SLO**（超出上限的集群可能无法满足）：API 响应性 P99 < 1s；Pod 启动（镜像已预热）P99 < 5s。可作为平台 SLO 基线（见 [[15-slo-chaos-engineering]]）。

### 1.2 实践中的规模分级

| 档位 | 节点数 | 关键关注点 |
|---|---|---|
| 中型 | 100–500 | 标准 HA 三件套（多 Master + etcd 集群 + LB）即可，重点是规范化 |
| 大型 | 500–2,000 | APF 限流、NodeLocal DNS、etcd 独立 SSD、组件专属节点池 |
| 超大型 | 2,000–5,000 | 调度器吞吐、watch 风暴治理、APIServer 横向扩展、etcd 分片评估 |
| 巨型 | > 5,000 | **不要再堆单集群**：按业务/地域/故障域拆分为多集群，用联邦或多集群管理平面统一治理 |

### 1.3 单集群 vs 多集群的决策点

出现以下任一信号时，应优先考虑**拆分集群**而不是继续扩容：

- 单集群节点数超过 3,000，且业务有明显地域或 BU 边界
- 需要跨可用区/跨地域容灾，单集群控制面无法满足 RTO
- 多租户隔离要求高（强隔离需求靠 Namespace 不够）
- 升级爆炸半径不可接受：一次控制面故障影响全部业务
- 监管合规要求物理/逻辑隔离

## 2. 大规模集群架构原则

### 2.1 控制面与数据面分离

- Master 节点**独立部署**，与业务节点物理/资源隔离
- 系统组件（CoreDNS、metrics-server、ingress-controller、monitoring）放入**专属系统节点池**，用 taint/toleration 隔离业务负载
- etcd 优先使用**独立节点 + 本地 NVMe SSD**，不与 APIServer 混部

### 2.2 控制面横向扩展

- APIServer 无状态，前置 LB 后可水平扩展到 3/5/7 实例
- 大规模集群中 APIServer 的瓶颈通常在 **watch 连接数与 LIST 请求**，而非 CPU；用 APF（API Priority and Fairness）分级限流
- Controller-manager 与 scheduler 通过 leader election 保证单活，必要时调大 QPS/Burst

### 2.3 故障域设计

- 节点按可用区（AZ）打标签，业务用 `topologySpreadConstraints` 跨 AZ 均匀分布
- 控制面跨 3 个 AZ 部署，容忍单 AZ 故障
- etcd 3/5 节点跨 AZ，保证 (n-1)/2 节点故障仍可用

### 2.4 可观测性先行

- 集群上线前必须先有：控制面指标（apiserver/etcd/scheduler/kubelet）、节点指标、事件采集、审计日志
- 大规模集群中**事件量**会成为 etcd 压力来源，需要单独治理（见 [[02-cluster-configuration#4. 事件与审计治理]]）

## 3. 容量基线参考

### 3.1 控制面组件资源基线（1,000–2,000 节点规模）

| 组件 | CPU Request | 内存 Request | 备注 |
|---|---|---|---|
| kube-apiserver（每实例） | 4–8C | 16–32 Gi | 随 watch 连接数线性增长 |
| etcd（每节点） | 2–4C | 8–16 Gi | 内存随对象数量增长，磁盘必须 SSD |
| kube-scheduler | 1–2C | 2–4 Gi | 大集群调大 QPS 后 CPU 上升 |
| kube-controller-manager | 1–2C | 2–4 Gi | — |
| CoreDNS（集群级） | 每实例 0.5–1C | 512 Mi–1 Gi | 按 QPS 水平扩展 + NodeLocal DNSCache |

### 3.2 节点资源预留基线

kubelet 必须为系统与 K8s 守护进程预留资源，否则节点满载时会 OOM 系统进程：

```yaml
# kubelet 配置示例（32C/128G 节点）
systemReserved:
  cpu: "500m"
  memory: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "2Gi"
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
```

### 3.3 密度基线

| 指标 | 保守值 | 激进值 | 风险 |
|---|---|---|---|
| 单节点 Pod 数 | 60 | 110 | 密度越高，CNI IP 消耗、iptables/ipvs 规则、镜像拉取并发压力越大 |
| 单节点容器启动并发 | 20 | 50 | 受磁盘 IO 与镜像仓库带宽限制 |
| 单集群 Service 数 | 5,000 | 10,000 | 直接影响 kube-proxy 规则同步延迟 |

## 4. 建设路线速查

```text
规划期   → 确定规模档位 → 单集群/多集群决策 → 控制面拓扑设计
建设期   → 控制面 HA + etcd 优化 → 网络(CNI/IP 规划) → 存储(StorageClass)
         → 系统组件节点池 → 可观测性基座 → 执行 [[06-initialization-checklist]]
上线期   → 接入规范([[03-workload]]) → 容量压测 → 执行 [[07-pre-production-checklist]]
运营期   → 季度清单回归 → 升级演练 → 护网前执行 [[08-security-defense-checklist]]
```

## 5. 常见反模式

| 反模式 | 后果 | 正确做法 |
|---|---|---|
| 单集群无限扩容到 5,000+ 节点 | 控制面抖动、升级爆炸半径过大 | 按故障域拆多集群 |
| Master 与业务混部 | 业务突发流量打挂控制面 | 独立节点 + taint 隔离 |
| etcd 用云盘普通型 | fsync 延迟高导致 Leader 频繁切换 | 本地 NVMe 或 ESSD PL1+ |
| 所有组件默认配置跑大集群 | APIServer OOM、DNS 超时、调度慢 | 按规模档位调优（见 [[02-cluster-configuration]]） |
| 没有 NodeLocal DNSCache | CoreDNS 被打爆，全集群解析超时 | 每个节点本地缓存 |

## Related

- [[02-cluster-configuration|集群配置最佳实践]]
- [[06-initialization-checklist|初始化配置检查项]]
- [[20-最佳实践/07-scenarios/capacity-planning|容量规划场景]]
- [[20-最佳实践/07-scenarios/multi-cluster|多集群场景]]

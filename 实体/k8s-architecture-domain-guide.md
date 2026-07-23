---
title: Kubernetes Architecture Domain Guide
description: Kubernetes Architecture Domain Guide — Kubernetes 生产运维知识库
summary: Kubernetes Architecture Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- architecture
- 集群基础
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Architecture Domain Guide 是什么
- 如何 Kubernetes Architecture Domain Guide
trigger_keywords:
- Kubernetes
- Architecture
- Domain
- Guide
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Architecture Domain Guide

## Source

Distilled from 集群基础 (25 documents, Kubernetes v1.29-v1.33).

## Layered Architecture

| Layer | Name | Components |
|-------|------|-----------|
| Layer 1 | Orchestration | Scheduler, Controllers |
| Layer 2 | API | API Server, Admission |
| Layer 3 | Data | etcd |
| Layer 4 | Runtime | kubelet, Container Runtime |
| Layer 5 | Network | CNI, kube-proxy |
| Layer 6 | Storage | CSI, Volume Plugin |
| Layer 7 | Extension | CRD, Operator, Webhook |

## Control Plane Components

- **kube-apiserver**: Port 6443 HTTPS. Stateless. Authentication + Authorization + Admission + etcd persistence.
- **etcd**: Ports 2379/2380. Raft consensus, MVCC storage. 3-node (tolerates 1 failure) or 5-node (tolerates 2).
- **kube-scheduler**: Port 10259. Filter+Score algorithm, leader election for HA.
- **kube-controller-manager**: Port 10257. 40+ controllers, leader election for HA.
- **cloud-controller-manager**: Port 10258. Cloud-specific Node/Service/Route controllers.

## Node Components

- **kubelet**: Port 10250. Pod lifecycle, CRI, CSI, probes, eviction. Max 110 Pods default.
- **kube-proxy**: Port 10249. iptables/IPVS/eBPF Service load balancing.
- **Container Runtime**: CRI via Unix socket (containerd or CRI-O).

## HA Sizing

| Cluster Size | Nodes | Pods | Master Config | etcd Config |
|-------------|-------|------|---------------|-------------|
| Small | <50 | <1500 | 2C4G | 3-node, 2C4G, SSD |
| Medium | 50-250 | 1500-7500 | 4C8G | 3-node, 4C8G, SSD |
| Large | 250-1000 | 7500-30000 | 8C16G | 5-node, 8C16G, NVMe |
| XLarge | >1000 | >30000 | 16C32G | 5-node, 16C32G, NVMe |

## 运维操作

```bash
# 🟢 检查控制平面组件状态
kubectl get --raw /healthz?verbose
kubectl get pods -n kube-system -l component=kube-apiserver
kubectl get pods -n kube-system -l component=etcd
kubectl get pods -n kube-system -l component=kube-scheduler
kubectl get pods -n kube-system -l component=kube-controller-manager

# 🟢 检查 etcd 集群健康
kubectl exec -n kube-system etcd-master-0 -- etcdctl endpoint health --cluster
kubectl exec -n kube-system etcd-master-0 -- etcdctl endpoint status --cluster -w table
kubectl exec -n kube-system etcd-master-0 -- etcdctl member list -w table

# 🟢 检查 API Server 指标
curl -sk https://localhost:6443/metrics | grep apiserver_request_duration_seconds
kubectl get --raw /metrics | grep apiserver_current_inflight_requests

# 🟢 检查调度器状态
kubectl get events -A --field-selector reason=FailedScheduling --sort-by=.lastTimestamp
kubectl logs -n kube-system -l component=kube-scheduler --tail=20

# 🟢 检查节点状态
kubectl get nodes -o wide
kubectl describe node <node> | grep -A10 "Conditions"
kubectl top nodes

# 🟢 检查集群资源使用
kubectl get pods -A --no-headers | wc -l  # 总 Pod 数
kubectl api-resources --verbs=list --namespaced -o name | xargs -n1 kubectl get --all-namespaces --no-headers 2>/dev/null | wc -l
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| API Server 无响应 | 证书过期/etcd断开 | `curl -k https://localhost:6443/healthz` | 检查证书/etcd连接 |
| etcd 延迟高 | 磁盘IO慢/成员失联 | `etcdctl endpoint status` | 升级SSD/检查网络 |
| Pod 调度失败 | 资源不足/约束冲突 | `kubectl describe pod` Events | 扩容/调整约束 |
| 节点 NotReady | kubelet/运行时异常 | `kubectl describe node` | 检查 kubelet 日志 |
| 控制器不工作 | Leader 选举失败 | `kubectl logs -n kube-system -l component=kube-controller-manager` | 检查选举/重启 |
| 集群升级失败 | 版本不兼容/插件冲突 | 检查升级日志 | 逐版本升级 |

### 排查流程

```
控制平面异常
├── API Server 不可用？
│   ├── 证书有效？→ openssl x509 -dates
│   ├── etcd 可达？→ etcdctl endpoint health
│   └── 资源充足？→ 检查 CPU/内存
├── etcd 性能问题？
│   ├── 磁盘延迟？→ iostat -x 1
│   ├── 成员健康？→ etcdctl member list
│   └── 数据库大小？→ etcdctl endpoint status
├── 调度异常？
│   ├── 资源不足？→ kubectl describe nodes | grep Allocated
│   ├── 约束冲突？→ 检查 affinity/taint/toleration
│   └── 调度器运行？→ kubectl logs scheduler
└── 节点问题？
    ├── kubelet 运行？→ systemctl status kubelet
    ├── 运行时正常？→ crictl info
    └── 网络可达？→ ping 节点 IP
```

## 生产案例

### 案例1：etcd 磁盘延迟导致集群不稳定

- **场景**：API Server 延迟周期性飙升，所有 kubectl 操作卡顿 5-10s
- **排查**：`etcdctl endpoint status` 显示 WAL fsync 延迟 500ms+；`iostat` 显示磁盘 %util 100%
- **方案**：将 etcd 数据目录迁移到独立 NVMe SSD；设置 etcd quota-backend-bytes=8GiB；定期 compact+defrag
- **效果**：API 延迟稳定在 50ms 以内

### 案例2：大规模集群调度器性能优化

- **场景**：1000 节点集群，Pod 调度延迟从 100ms 增长到 5s
- **排查**：调度器队列积压；大量 Pod 使用复杂的 anti-affinity 规则
- **方案**：启用调度器并行化（percentageOfNodesToScore=10）；简化 affinity 规则；升级调度器版本
- **效果**：调度延迟回落到 200ms

## 检查清单

- [ ] 控制平面组件全部健康（healthz verbose）
- [ ] etcd 集群健康且延迟 < 10ms
- [ ] API Server 证书有效期 > 30 天
- [ ] 调度器无长期积压队列
- [ ] 节点状态全部 Ready
- [ ] 控制平面监控告警已配置
- [ ] etcd 备份策略已执行
- [ ] 集群版本在支持范围内

## Related

- [[reference|#reference Hub]] — tag hub

- [[概念/observability-pillars.md|observability-pillars]] — 01-observability-architecture-overview Pillars
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[概念/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[概念/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]
- [[概念/observability-pillars.md|Observability Pillars]]
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]

- 01-plane-architecture-overview
- [[平台工程/代码分析/cluster-cert/01-pki-architecture.md|01-pki-architecture]]

<!-- risk-assessed -->

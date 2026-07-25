---
title: kube-controller-manager
description: kube-controller-manager — Kubernetes 生产运维知识库
summary: kube-controller-manager 运行 Kubernetes 所有内置控制器，通过控制循环持续将集群状态收敛到期望状态。
category: entities
tags:
- k8s
- controller-manager
- control-plane
- controllers
- leader-election
- reconcile
- etcd
- apiserver
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-controller-manager 是什么
- 如何 kube-controller-manager
trigger_keywords:
- kube-controller-manager
prerequisites:
- kubectl-basics
- kubernetes-concepts
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-controller-manager

## Role

kube-controller-manager (KCM) runs all built-in controllers in Kubernetes. Each controller watches the API Server for state changes and continuously reconciles current state with desired state.

Key controllers include:
- **Node Controller**: Monitors node health, marks nodes NotReady/Unknown, evicts Pods after timeout
- **Replication Controller / ReplicaSet Controller**: Maintains desired Pod replicas
- **Deployment Controller**: Manages rolling updates and rollbacks
- **Endpoint / EndpointSlice Controller**: Populates Service backends
- **Job / CronJob Controller**: Manages batch workloads
- **PV / PVC Controller**: Binds volumes and handles provisioning/deletion
- **Namespace Controller**: Cleans up resources when Namespace is deleted
- **ServiceAccount / Token Controller**: Manages SA credentials

## Architecture

```
API Server
    ↑↓
kube-controller-manager (single leader via Leader Election)
    ├── Node Controller
    ├── ReplicaSet Controller
    ├── Deployment Controller
    ├── EndpointSlice Controller
    ├── Job Controller
    ├── PV Binder Controller
    └── ... (40+ controllers)
```

### Leader Election

Only one KCM instance is active at a time. Standby instances hold a Lease object in `kube-system` and take over when the leader fails.

| Lease Object | Command |
|-------------|---------|
| `kube-controller-manager` | `kubectl get lease kube-controller-manager -n kube-system` |

## Key Configuration

| Parameter | Purpose | Production Default |
|-----------|---------|-------------------|
| `--leader-elect` | Enable leader election | `true` |
| `--leader-elect-lease-duration` | Lease duration | `15s` |
| `--leader-elect-renew-deadline` | Renew deadline | `10s` |
| `--leader-elect-retry-period` | Retry period | `2s` |
| `--node-monitor-grace-period` | Time before marking node Unknown | `40s` |
| `--pod-eviction-timeout` | Timeout before evicting Pods from failed node | `5m0s` |
| `--controllers` | Enable/disable specific controllers | `*` (all) |
| `--concurrent-deployment-syncs` | Parallel sync goroutines | `5` |
| `--concurrent-replicaset-syncs` | Parallel RS sync goroutines | `5` |

## 运维操作

```bash
# 🟢 查看 KCM Pod 状态
kubectl get pods -n kube-system -l component=kube-controller-manager

# 🟢 查看 Leader Lease
kubectl get lease kube-controller-manager -n kube-system -o yaml

# 🟢 查看 KCM 日志
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100

# 🟢 查看控制器指标
kubectl get --raw /metrics | grep controller_manager

# 🟢 检查节点状态变化
kubectl get nodes -o wide
kubectl describe node <node>

# 🟡 调整节点监控宽限期（修改静态 Pod manifest）
# /etc/kubernetes/manifests/kube-controller-manager.yaml:
#   --node-monitor-grace-period=60s
#   --pod-eviction-timeout=2m
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Deployment 不滚动更新 | KCM 未运行 / Leader 丢失 | `kubectl get lease kube-controller-manager -n kube-system` | 重启 KCM / 检查 API Server 连通性 |
| 节点长期 NotReady 但 Pod 未驱逐 | `pod-eviction-timeout` 过大或 KCM 异常 | `kubectl get pods -n kube-system -l component=kube-controller-manager` | 调整 eviction 参数 / 恢复 KCM |
| ReplicaSet 不创建 Pod | RS Controller 异常 / 配额不足 | `kubectl describe rs <name>` | 检查事件 / 资源配额 |
| Service Endpoint 为空 | EndpointSlice Controller 异常 | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` | 检查 KCM 日志 |
| PVC 长期 Pending | PV Controller / 存储类异常 | `kubectl describe pvc <name>` | 检查 StorageClass / CSI driver |
| 证书过期导致 KCM 无法启动 | PKI 证书过期 | `openssl x509 -in /etc/kubernetes/pki/controller-manager.crt -noout -dates` | 轮换证书 |

```bash
# 排查流程
# 1. 检查 KCM 进程/静态 Pod
ps aux | grep kube-controller-manager
crictl ps | grep kube-controller-manager

# 2. 检查 Leader Lease
kubectl get lease kube-controller-manager -n kube-system

# 3. 检查 KCM 到 API Server 的连通性
kubectl logs -n kube-system -l component=kube-controller-manager | grep -i error

# 4. 检查证书有效期
for cert in /etc/kubernetes/pki/*.crt; do
  echo "$cert: $(openssl x509 -in $cert -noout -enddate)"
done
```

## 生产案例

### 案例1：Leader 选举异常导致控制循环停滞
- **场景**：Deployment 更新后 Pod 数量不变化，节点状态不更新
- **排查**：`kubectl get lease kube-controller-manager -n kube-system` 显示 holder 为空且不断切换
- **方案**：检查 API Server 网络延迟与 etcd 健康；降低 `--leader-elect-renew-deadline` 适应高延迟环境；确保多个 KCM 副本时钟同步
- **效果**：Leader 稳定，控制循环恢复

### 案例2：Pod 驱逐延迟导致故障域扩大
- **场景**：节点断电后 5 分钟内 Pod 仍未被驱逐
- **排查**：`pod-eviction-timeout` 配置为 5m，`node-monitor-grace-period` 为 40s
- **方案**：根据 RTO 要求调整为 `--node-monitor-grace-period=20s --pod-eviction-timeout=1m`
- **效果**：故障恢复时间从 5 分钟缩短到 1 分钟

## 检查清单

- [ ] KCM 高可用部署（>= 3 控制平面节点）
- [ ] Leader Election Lease 正常且唯一
- [ ] 关键控制器未被禁用
- [ ] 节点监控与驱逐参数符合 RTO/RPO 要求
- [ ] 证书有效期 > 30 天
- [ ] KCM 日志已接入日志系统
- [ ] 关键控制器指标已接入监控（workqueue depth、rate limiter latency）

## Related

- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|kube-controller-manager 深度解析]]
- [[19-故障诊断/06-FTA故障树/list/controller-manager-fta.md|Controller Manager 异常故障树分析]]


<!-- risk-assessed -->

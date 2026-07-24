---
title: Kubernetes Workloads Domain Guide
description: '- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
summary: '- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
category: references
tags:
- k8s
- workloads
- 工作负载
- pod
- deployment
- statefulset
- reference
- daemonset
- job
- cronjob
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Workloads Domain Guide 是什么
- 如何 Kubernetes Workloads Domain Guide
trigger_keywords:
- Kubernetes
- Workloads
- Domain
- Guide
prerequisites:
- kubectl-basics
- pod-lifecycle
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Workloads Domain Guide

## Source

Distilled from 工作负载 (24 documents, Kubernetes v1.28-v1.32).

## Workload Controllers

| Controller | Manages | Update Strategy | Use Case |
|-----------|---------|----------------|----------|
| **[[deployment]]** | ReplicaSet | RollingUpdate, Recreate | Stateless microservices |
| **StatefulSet** | Pod (ordered) | RollingUpdate (reverse), Partition | Databases, message brokers |
| **DaemonSet** | Pod (per node) | RollingUpdate | Logging, monitoring, CNI agents |
| **Job** | Pod (run-to-completion) | - | Batch processing, data migration |
| **CronJob** | Job (scheduled) | - | Backup, cleanup, periodic tasks |

## Pod Lifecycle Phases

Pending -> Running -> Succeeded/Failed -> Terminating

Conditions: PodScheduled, Initialized, ContainersReady, Ready.

## Production Patterns

- Set `revisionHistoryLimit: 10` for rollback capability
- Use `maxSurge: 1, maxUnavailable: 0` for zero-downtime updates
- Configure PodAntiAffinity for replica distribution
- Always set resource requests/limits
- Use three probes: startup, liveness, readiness

## Sidecar Pattern

v1.28+ native sidecar containers: init containers with `restartPolicy: Always` run alongside main containers, enabling service mesh proxies, log shippers, and config watchers without external injection.

## 运维操作

```bash
# 🟢 查看工作负载状态
kubectl get deploy,sts,ds,job,cronjob -A

# 🟢 查看 Deployment 滚动更新状态
kubectl rollout status deploy/<name> -n <ns>
kubectl rollout history deploy/<name> -n <ns>

# 🟢 查看 Pod 生命周期状态
kubectl get pods -n <ns> -o wide
kubectl get pods -n <ns> -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,RESTARTS:.status.containerStatuses[0].restartCount

# 🟡 回滚 Deployment
kubectl rollout undo deploy/<name> -n <ns>
kubectl rollout undo deploy/<name> -n <ns> --to-revision=3

# 🟡 暂停/恢复滚动更新
kubectl rollout pause deploy/<name> -n <ns>
kubectl rollout resume deploy/<name> -n <ns>

# 🟡 手动扩缩容
kubectl scale deploy/<name> --replicas=5 -n <ns>

# 🔴 强制删除卡住的 Pod
kubectl delete pod <name> -n <ns> --grace-period=0 --force
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Pod Pending | 资源不足/调度失败 | `kubectl describe pod <name>` | 检查节点资源/Taint/亲和性 |
| ImagePullBackOff | 镜像不存在/认证失败 | `kubectl get events` | 检查镜像名和 imagePullSecrets |
| CrashLoopBackOff | 应用启动崩溃 | `kubectl logs --previous` | 检查应用日志和配置 |
| Deployment 卡住 | 滚动更新失败 | `kubectl rollout status` | 检查新 Pod 健康检查 |
| StatefulSet 卡住 | PVC 未绑定 | `kubectl get pvc` | 检查 StorageClass 和 PV |

```bash
# 排查流程
# 1. 检查工作负载状态
kubectl get deploy <name> -n <ns> -o yaml | grep -A10 status

# 2. 检查 Pod 事件
kubectl get events -n <ns> --sort-by='.lastTimestamp' | tail -20

# 3. 检查容器日志
kubectl logs <pod> -n <ns> -c <container> --tail=100
kubectl logs <pod> -n <ns> --previous  # 上次崩溃日志

# 4. 检查资源使用
kubectl top pod <pod> -n <ns>
kubectl describe node <node> | grep -A10 "Allocated resources"
```

## 生产案例

### 案例1：零停机滚动更新
- **场景**：生产服务需要无感知更新，不允许任何请求失败
- **方案**：配置 maxSurge=1, maxUnavailable=0；配置 readinessProbe 确保新 Pod 就绪后才接收流量；配置 preStop hook 优雅关闭
- **效果**：更新过程零请求失败，用户无感知

### 案例2：StatefulSet 数据库升级
- **场景**：PostgreSQL StatefulSet 需要从 v14 升级到 v15
- **方案**：使用 partition 策略逐个升级；先备份 PVC；按逆序更新 Pod；验证数据一致性
- **效果**：数据库升级零数据丢失，主从切换时间 < 10s

## 检查清单

- [ ] 所有工作负载已配置 resource requests/limits
- [ ] 健康检查已配置（startup/liveness/readiness）
- [ ] revisionHistoryLimit 已设置（建议 10）
- [ ] PodAntiAffinity 已配置（副本分散）
- [ ] PDB 已配置（关键服务）
- [ ] preStop hook 已配置（优雅关闭）
- [ ] 滚动更新策略已配置（maxSurge/maxUnavailable）

## Related

- [[reference|#reference Hub]] — tag hub

- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[pod-lifecycle]] — Pod Lifecycle
- [[pod-lifecycle|Pod Lifecycle]]
- [[deployment|Deployment]]
- [[实体/statefulset.md|StatefulSet]]
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]

- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]

<!-- risk-assessed -->

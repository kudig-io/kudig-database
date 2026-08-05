---
title: 技能总览
type: moc
created: '2026-08-05'
description: 运维技能库总览 — 诊断排障、最佳实践、培训体系、FTA 方法，Agent 优先读取
tags:
- skills
- troubleshooting
- best-practices
- overview
- agent-entry
tier: core
---

# 技能 (Skills) — 总览

> **运维技能库入口**。按 8 大技术专题组织，覆盖诊断排障、工单案例、最佳实践、运维操作、培训体系。
> 高频标记：🔴 工单TOP | 🔵 最佳实践TOP | 🟢 产品高频

## 子域导航

- [[26-技能/01-集群运维/index.md|01-集群运维]] — 集群升级、扩缩容、GitOps、Helm、迁移
- [[26-技能/02-控制面/index.md|02-控制面]] — API Server、Scheduler、Controller Manager、etcd
- [[26-技能/03-节点/index.md|03-节点]] — Node NotReady、节点池、GPU
- [[26-技能/04-工作负载/index.md|04-工作负载]] — Pod/Deployment/StatefulSet/DaemonSet/Job/HPA 🔴 工单TOP
- [[26-技能/05-网络/index.md|05-网络]] — DNS/Service/Ingress/NetworkPolicy/CNI 🔴 工单TOP
- [[26-技能/06-存储/index.md|06-存储]] — CSI/PV/PVC 存储排障 🔴 工单TOP
- [[26-技能/07-安全/index.md|07-安全]] — RBAC/证书/Webhook/Pod 安全
- [[26-技能/08-可观测性/index.md|08-可观测性]] — 监控/日志/链路追踪 🔵 最佳实践TOP

## 快速诊断入口

```bash
# Pod 异常
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded
kubectl describe pod <name> -n <ns> | tail -30

# 节点异常
kubectl get nodes -o wide
kubectl describe node <name>

# 网络诊断
kubectl exec <pod> -- nslookup kubernetes.default
```

## 学习路径

| 阶段 | 内容 |
| L1 新人 | K8s 基础 + OnCall 培训 |
| L2 进阶 | 诊断排障 + 工单案例 |
| L3 高级 | FTA 方法论 + 最佳实践 |
| L4 专家 | Agent 编排 + 技能建设 |

## 使用指引

- **Agent 检索**: 从本索引出发，按子域定位具体技能文档
- **人类阅读**: 配合 [[26-技能/index.md|主索引]] 使用

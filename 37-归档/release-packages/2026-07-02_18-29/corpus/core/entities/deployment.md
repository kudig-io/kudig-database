---
title: Deployment
description: '- [[concepts/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
summary: Deployment is the primary workload controller for stateless applications.
  It manages ReplicaSets, which in turn manage [[Pods|Pods]].
category: entities
tags:
- k8s
- deployment
- workload
- replica-set
- rolling-update
- stateless
- prometheus
- hpa
- statefulset
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployment 是什么
- 如何 Deployment
trigger_keywords:
- Deployment
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Deployment

## Role

Deployment is the primary workload controller for stateless applications. It manages ReplicaSets, which in turn manage [[Pods|Pods]].

## Key Features

| Feature | Description |
|---------|-------------|
| **Declarative management** | Define desired replica count, update strategy, Pod template |
| **Rolling updates** | Replace old Pods with new Pods gradually (maxSurge, maxUnavailable) |
| **Rollback** | Revert to any previous [[ReplicaSet|ReplicaSet]] via `kubectl rollout undo` |
| **Scaling** | Change replica count with `kubectl scale` or HPA |
| **Revision history** | Track changes via `revisionHistoryLimit` (default 10) |

## Update Strategy

| Strategy | Behavior | Use Case |
|----------|----------|----------|
| **RollingUpdate** | Gradually replace Pods (default) | Most production workloads |
| **Recreate** | Kill all old Pods, then create new | Stateful or incompatible versions |

RollingUpdate parameters:
- `maxSurge`: Extra Pods above desired count during update (default 25%)
- `maxUnavailable`: Pods below desired count during update (default 25%)

For zero-downtime updates: set `maxUnavailable: 0` with `maxSurge: 1`.

## Conditions

[[Deployments|Deployments]] report these conditions:
- **Available**: Minimum replicas are ready
- **Progressing**: Deployment is making progress (new Pods created or old Pods terminated)

## Related
- [[concepts/Pod 生命周期 × Secret 管理.md|[[Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]]]] — 综合
- [[concepts/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 综合

- [[concepts/Deployment × Secret 管理.md|Deployment × Secret 管理]]

- [[skills/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[skills/k8s-deployment-strategies-guide.md|k8s-deployment-strategies-guide]] — Kubernetes 部署策略最佳实践
- [[skills/deployment-canary-and-bluegreen.md|deployment-canary-and-bluegreen]] — 金丝雀与蓝绿发布
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[entities/statefulset.md|StatefulSet]]
- ReplicaSet
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]

- 04-kubernetes-multi-cloud-hybrid-deployment
- 99-kubernetes-deployment-patterns-architecture
- 07-deployment-replicaset-events
- 03-prometheus-ha-deployment
- 03-backstage-deployment
- 04-deployment-replicaset
- 25-multi-cloud-hybrid-deployment
- 24-production-deployment-best-practices
- 11-vercel-frontend-deployment-platform
- 22-agentscope-production-deployment
- 09-production-deployment-guide
- 02-multi-cloud-hybrid-deployment-strategy
- 03-edge-computing-production-deployment
- 02-deployment-production-patterns
- 09-edge-computing-deployment
- 03-kubeedge-architecture-deployment
- 12-cluster-deployment-patterns
- 10-model-deployment-management
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/03-deployment-comprehensive-troubleshooting|11-deployment-comprehensive-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/deployment-fta.md|Deployment 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/02-deployment-troubleshooting.md|02-deployment-troubleshooting]]
- [[entities/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[entities/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[entities/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[entities/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[entities/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]] — Cross-reference
- [[entities/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[entities/kubectl-quick-reference.md|Kubectl Quick Reference]] — Cross-reference
- [[entities/k8s-ai-agent-engineering.md|AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署]] — Cross-reference
- [[entities/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]] — Cross-reference
- [[entities/k8s-supply-chain-yaml-cheatsheet.md|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[entities/kubernetes-api-versions-reference.md|Kubernetes API Versions Reference]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-02-workloads-applications/00-core-workloads/04-scheduler-configuration|调度器配置与优化]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-02-workloads-applications/00-core-workloads/03-workload-controllers-overview|工作负载控制器详解]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-02-workloads-applications/00-core-workloads/03-pod-lifecycle-events|Pod 生命周期事件表]] — Cross-reference
- [[concepts/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/bp-infrastructure.md|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/bp-operations.md|最佳实践：Operations]] — Cross-reference
- [[concepts/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[skills/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-scaling-guide.md|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[skills/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/assessment-k8s-fundamentals-quiz.md|K8S Fundamentals Quiz]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes.md|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[skills/learn-06-configmap-secret.md|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[skills/learn-10-health-check.md|第八课：健康检查 - Probe 详解]] — Cross-reference
- [[skills/learn-lecturer-persona.md|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[skills/learn-15-scheduling-basics.md|第15课：调度与亲和性]] — Cross-reference
- [[skills/learn-07-namespace-resource-quota.md|第七课：Namespace 与资源隔离]] — Cross-reference
- [[skills/learn-09-hpa-basics.md|第九课：HPA - 自动伸缩]] — Cross-reference
- [[skills/learn-11-job-cronjob.md|第九课：Job 和 CronJob - 任务调度]] — Cross-reference
- [[skills/learn-root.md|Kubernetes 培训：Root]] — Cross-reference
- [[skills/learn-14-statefulset-basics.md|第14课：StatefulSet - 有状态应用管理]] — Cross-reference
- [[skills/learn-02-pod-basics.md|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[skills/learn-04-service-basics.md|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/deployment-rolling-update.md|Deployment 滚动更新策略]] — Cross-reference
- [[skills/skill-MOC.md|topic-skills MOC]] — Cross-reference
- [[skills/learn-03-deployment-basics.md|第三课：Deployment - 应用部署管理器]] — Cross-reference
- [[skills/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/learn-analogy-dictionary.md|K8S 概念类比词典]] — Cross-reference
- [[entities/metal3-io.md|Metal3]] — Cross-reference
- [[entities/clusterpedia.md|Clusterpedia]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference


<!-- risk-assessed -->

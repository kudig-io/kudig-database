---
title: Deployment
description: '- [[synthesis/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
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

# Deployment

## Role

Deployment is the primary workload controller for stateless applications. It manages ReplicaSets, which in turn manage Pods.

## Key Features

| Feature | Description |
|---------|-------------|
| **Declarative management** | Define desired replica count, update strategy, Pod template |
| **Rolling updates** | Replace old Pods with new Pods gradually (maxSurge, maxUnavailable) |
| **Rollback** | Revert to any previous ReplicaSet via `kubectl rollout undo` |
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

Deployments report these conditions:
- **Available**: Minimum replicas are ready
- **Progressing**: Deployment is making progress (new Pods created or old Pods terminated)

## Related
- [[synthesis/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合
- [[synthesis/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 综合

- [[synthesis/Deployment × Secret 管理]]

- [[skills/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[skills/k8s-deployment-strategies-guide.md|k8s-deployment-strategies-guide]] — Kubernetes 部署策略最佳实践
- [[skills/deployment-canary-and-bluegreen.md|deployment-canary-and-bluegreen]] — 金丝雀与蓝绿发布
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[entities/statefulset.md|StatefulSet]]
- ReplicaSet
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]

- [[domain-19-landscape-references/04-kubernetes-multi-cloud-hybrid-deployment.md|04-kubernetes-multi-cloud-hybrid-deployment]]
- [[domain-01-cluster-fundamentals/99-kubernetes-deployment-patterns-architecture.md|99-kubernetes-deployment-patterns-architecture]]
- [[domain-17-system-foundation/07-deployment-replicaset-events.md|07-deployment-replicaset-events]]
- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
- [[domain-07-platform-engineering/03-backstage-deployment.md|03-backstage-deployment]]
- [[domain-18-manifests-patterns/04-deployment-replicaset.md|04-deployment-replicaset]]
- [[domain-01-cluster-fundamentals/25-multi-cloud-hybrid-deployment.md|25-multi-cloud-hybrid-deployment]]
- [[domain-01-cluster-fundamentals/24-production-deployment-best-practices.md|24-production-deployment-best-practices]]
- [[domain-07-platform-engineering/11-vercel-frontend-deployment-platform.md|11-vercel-frontend-deployment-platform]]
- [[domain-14-ai-ml-infra/22-agentscope-production-deployment.md|22-agentscope-production-deployment]]
- [[domain-14-ai-ml-infra/09-production-deployment-guide.md|09-production-deployment-guide]]
- [[domain-01-cluster-fundamentals/02-multi-cloud-hybrid-deployment-strategy.md|02-multi-cloud-hybrid-deployment-strategy]]
- [[domain-01-cluster-fundamentals/03-edge-computing-production-deployment.md|03-edge-computing-production-deployment]]
- [[domain-02-workloads-applications/02-deployment-production-patterns.md|02-deployment-production-patterns]]
- [[domain-02-workloads-applications/09-edge-computing-deployment.md|09-edge-computing-deployment]]
- [[domain-15-specialized-tech/03-kubeedge-architecture-deployment.md|03-kubeedge-architecture-deployment]]
- [[domain-01-cluster-fundamentals/12-cluster-deployment-patterns.md|12-cluster-deployment-patterns]]
- [[domain-14-ai-ml-infra/10-model-deployment-management.md|10-model-deployment-management]]
- [[domain-10-troubleshooting-diagnostics/11-deployment-comprehensive-troubleshooting.md|11-deployment-comprehensive-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md|Deployment 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting.md|02-deployment-troubleshooting]]
- [[references/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/release-notes-kubernetes|发布说明索引 — Kubernetes]] — Cross-reference
- [[references/kubectl Scenario Quick Reference|kubectl Scenario Quick Reference]] — Cross-reference
- [[references/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[references/kubectl-quick-reference|Kubectl Quick Reference]] — Cross-reference
- [[references/k8s-ai-agent-engineering|AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署]] — Cross-reference
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]] — Cross-reference
- [[references/k8s-supply-chain-yaml-cheatsheet|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[references/kubernetes-api-versions-reference|Kubernetes API Versions Reference]] — Cross-reference
- [[domain-02-workloads-applications/00-core-workloads/19-scheduler-configuration|调度器配置与优化]] — Cross-reference
- [[domain-02-workloads-applications/00-core-workloads/10-workload-controllers-overview|工作负载控制器详解]] — Cross-reference
- [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events|Pod 生命周期事件表]] — Cross-reference
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/bp-infrastructure|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/bp-operations|最佳实践：Operations]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/k8s-production-best-practices|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-scaling-guide|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[skills/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/assessment-k8s-fundamentals-quiz|K8S Fundamentals Quiz]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/deployment-fta|Deployment 异常故障树分析]] — Cross-reference
- [[skills/learn-06-configmap-secret|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[skills/learn-10-health-check|第八课：健康检查 - Probe 详解]] — Cross-reference
- [[skills/learn-lecturer-persona|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[skills/learn-15-scheduling-basics|第15课：调度与亲和性]] — Cross-reference
- [[skills/learn-07-namespace-resource-quota|第七课：Namespace 与资源隔离]] — Cross-reference
- [[skills/learn-09-hpa-basics|第九课：HPA - 自动伸缩]] — Cross-reference
- [[skills/learn-11-job-cronjob|第九课：Job 和 CronJob - 任务调度]] — Cross-reference
- [[skills/learn-root|Kubernetes 培训：Root]] — Cross-reference
- [[skills/learn-14-statefulset-basics|第14课：StatefulSet - 有状态应用管理]] — Cross-reference
- [[skills/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[skills/learn-04-service-basics|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/deployment-rolling-update|Deployment 滚动更新策略]] — Cross-reference
- [[skills/skill-MOC|topic-skills MOC]] — Cross-reference
- [[skills/learn-03-deployment-basics|第三课：Deployment - 应用部署管理器]] — Cross-reference
- [[skills/learn-12-common-problems|第十课：常见问题排查]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/learn-analogy-dictionary|K8S 概念类比词典]] — Cross-reference
- [[entities/metal3-io|Metal3]] — Cross-reference
- [[entities/clusterpedia|Clusterpedia]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference

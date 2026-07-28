---
title: Deployment
description: '- [[22-概念/11-交叉分析/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
summary: Deployment is the primary workload controller for stateless applications.
  It manages ReplicaSets, which in turn manage [[pods|Pods]].
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

Deployment is the primary workload controller for stateless applications. It manages ReplicaSets, which in turn manage [[pods|Pods]].

## Key Features

| Feature | Description |
|---------|-------------|
| **Declarative management** | Define desired replica count, update strategy, Pod template |
| **Rolling updates** | Replace old Pods with new Pods gradually (maxSurge, maxUnavailable) |
| **Rollback** | Revert to any previous [[replicaset\|ReplicaSet]] via `kubectl rollout undo` |
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

[[deployments|Deployments]] report these conditions:
- **Available**: Minimum replicas are ready
- **Progressing**: Deployment is making progress (new Pods created or old Pods terminated)

## Related
- [[22-概念/Pod 生命周期 × Secret 管理.md|[[Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]]]] — 综合
- [[22-概念/11-交叉分析/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 综合

- [[22-概念/11-交叉分析/Deployment × Secret 管理.md|Deployment × Secret 管理]]

- [[26-技能/04-工作负载/deployment/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[26-技能/04-工作负载/deployment/最佳实践/k8s-deployment-strategies-guide.md|k8s-deployment-strategies-guide]] — Kubernetes 部署策略最佳实践
- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen.md|deployment-canary-and-bluegreen]] — 金丝雀与蓝绿发布
- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[22-概念/07-调度与资源/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[23-实体/02-K8s核心组件/statefulset.md|StatefulSet]]
- ReplicaSet
- [[pod-lifecycle|Pod Lifecycle]]
- [[22-概念/07-调度与资源/autoscaling-strategies.md|Autoscaling Strategies]]

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
- [[19-故障诊断/02-资源排障/11-deployment-comprehensive-troubleshooting.md|11-deployment-comprehensive-troubleshooting]]
- [[19-故障诊断/06-FTA故障树/list/deployment-fta.md|Deployment 异常故障树分析]]
- [[19-故障诊断/04-高级排障/structural-05-workloads/02-deployment-troubleshooting.md|02-deployment-troubleshooting]]
- [[23-实体/15-参考与索引/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[23-实体/15-参考与索引/kubectl-quick-reference.md|Kubectl Quick Reference]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-ai-agent-engineering.md|AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署]] — Cross-reference
- [[23-实体/15-参考与索引/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-supply-chain-yaml-cheatsheet.md|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[23-实体/15-参考与索引/kubernetes-api-versions-reference.md|Kubernetes API Versions Reference]] — Cross-reference
- [[02-工作负载/01-核心工作负载/19-scheduler-configuration.md|调度器配置与优化]] — Cross-reference
- [[02-工作负载/01-核心工作负载/10-workload-controllers-overview.md|工作负载控制器详解]] — Cross-reference
- [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|Pod 生命周期事件表]] — Cross-reference
- [[19-故障诊断/01-核心排障/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[22-概念/08-可靠性与运维/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[22-概念/11-交叉分析/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[22-概念/02-工作负载/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[22-概念/10-最佳实践/bp-infrastructure.md|最佳实践：Infrastructure]] — Cross-reference
- [[22-概念/10-最佳实践/bp-operations.md|最佳实践：Operations]] — Cross-reference
- [[22-概念/12-研究/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[26-技能/04-工作负载/hpa-vpa/最佳实践/k8s-scaling-guide.md|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/培训/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz.md|K8S Fundamentals Quiz]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-what-is-kubernetes.md|第一课：Kubernetes 入门]] — Cross-reference
- [[26-技能/04-工作负载/deployment/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-06-configmap-secret.md|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-10-health-check.md|第八课：健康检查 - Probe 详解]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-lecturer-persona.md|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics.md|第15课：调度与亲和性]] — Cross-reference
- [[26-技能/07-安全/resource-quota/培训/learn-07-namespace-resource-quota.md|第七课：Namespace 与资源隔离]] — Cross-reference
- [[26-技能/04-工作负载/hpa-vpa/培训/learn-09-hpa-basics.md|第九课：HPA - 自动伸缩]] — Cross-reference
- [[26-技能/04-工作负载/job-cronjob/培训/learn-11-job-cronjob.md|第九课：Job 和 CronJob - 任务调度]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-root.md|Kubernetes 培训：Root]] — Cross-reference
- [[26-技能/04-工作负载/statefulset/培训/learn-14-statefulset-basics.md|第14课：StatefulSet - 有状态应用管理]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-02-pod-basics.md|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[26-技能/05-网络/service/培训/learn-04-service-basics.md|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[26-技能/04-工作负载/deployment/deployment-rolling-update.md|Deployment 滚动更新策略]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-MOC.md|topic-skills MOC]] — Cross-reference
- [[26-技能/04-工作负载/deployment/培训/learn-03-deployment-basics.md|第三课：Deployment - 应用部署管理器]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-analogy-dictionary.md|K8S 概念类比词典]] — Cross-reference
- [[23-实体/09-编排调度/metal3-io.md|Metal3]] — Cross-reference
- [[23-实体/09-编排调度/clusterpedia.md|Clusterpedia]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference


<!-- risk-assessed -->

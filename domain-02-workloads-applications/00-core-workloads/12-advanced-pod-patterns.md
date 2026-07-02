---
title: 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)
description: '# 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)'
summary: 'requiredDuringSchedulingIgnoredDuringExecution:'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- operator
- daemonset
- job
- cronjob
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 容器与 Pod 高级运维模式 (Advanced Pod Patterns) 是什么
- 如何 容器与 Pod 高级运维模式 (Advanced Pod Patterns)
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 容器与
- Pod
- 高级运维模式
- Advanced
- Pod
- Patterns
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
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
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---



# 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[concepts/pod-lifecycle.md|Pod Lifecycle]]](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/)

<!-- chunk: 1. 探针深度调优 (Probes Tuning) -->
## 1. 探针深度调优 (Probes Tuning)

| 探针 (Probe) | 职责 (Responsibility) | 生产注意 (Production Tips) |
|-------------|---------------------|--------------------------|
| **Startup** | 延时探测启动, 保护大模型加载 | 必须配置, 防止容器启动中被 Liveness 杀掉 |
| **Liveness** | 检测僵死, 触发重启 | *不要* 检测依赖服务, 仅检测进程自身 |
| **Readiness** | 控制流量切入 | 对接边缘情况, 确保从 LB 摘除后再退出 |

<!-- chunk: 2. 调度策略: 亲和性与互斥 (Affinity & Anti-affinity) -->
## 2. 调度策略: 亲和性与互斥 (Affinity & Anti-affinity)

```yaml
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: node.kubernetes.io/instance-type
            operator: In
            values: ["ecs.g7.xlarge"]
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: high-availability-svc
        topologyKey: "kubernetes.io/hostname"
```

<!-- chunk: 3. 安全加固 (Pod Security) -->
## 3. 安全加固 (Pod Security)

- **只读根文件系统**: `readOnlyRootFilesystem: true` 防止被植入病毒。
- **能力限制**: `capabilities: { drop: ["ALL"] }` 遵循最小权限原则。
- **PSA (Pod Security Admission)**: 命名空间级强制执行 `privileged`, `baseline`, `restricted` 策略。

<!-- chunk: 4. 生命周期 Hook (Lifecycle Hooks) -->
## 4. 生命周期 Hook (Lifecycle Hooks)

- **preStop**: 生产必配，执行自律下线（如 Nginx 关闭, Java 优雅退出）。
- **postStart**: 用于初始化配置，但不保证在 EntryPoint 之后执行。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-02-workloads-applications KUDIG Database — Global MOC
- [[domain-02-workloads-applications/README.md|Domain-4: Kubernetes工作负载管理]]
- Domain-4 工作负载 — 开源项目索引
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 10-workload-controllers-overview
- 11-pod-lifecycle-events
- 13-container-lifecycle-hooks
- 14-sidecar-containers-patterns

## Related

- [[domain-19-landscape-references/topic-index/pod-index.md|Pod 知识图谱索引]]

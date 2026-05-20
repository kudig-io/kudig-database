---
title: 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)
description: '# 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- operator
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
cross_refs:
- type: domain
  path: ../domain-3-control-plane/
  label: '相关知识域: domain-3-control-plane'
- type: domain
  path: ../domain-8-observability/
  label: '相关知识域: domain-8-observability'
- type: fta
  path: ../topic-fta/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

# 111 - 容器与 Pod 高级运维模式 (Advanced Pod Patterns)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Pod Lifecycle](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/)

## 1. 探针深度调优 (Probes Tuning)

| 探针 (Probe) | 职责 (Responsibility) | 生产注意 (Production Tips) |
|-------------|---------------------|--------------------------|
| **Startup** | 延时探测启动, 保护大模型加载 | 必须配置, 防止容器启动中被 Liveness 杀掉 |
| **Liveness** | 检测僵死, 触发重启 | *不要* 检测依赖服务, 仅检测进程自身 |
| **Readiness** | 控制流量切入 | 对接边缘情况, 确保从 LB 摘除后再退出 |

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

## 3. 安全加固 (Pod Security)

- **只读根文件系统**: `readOnlyRootFilesystem: true` 防止被植入病毒。
- **能力限制**: `capabilities: { drop: ["ALL"] }` 遵循最小权限原则。
- **PSA (Pod Security Admission)**: 命名空间级强制执行 `privileged`, `baseline`, `restricted` 策略。

## 4. 生命周期 Hook (Lifecycle Hooks)

- **preStop**: 生产必配，执行自律下线（如 Nginx 关闭, Java 优雅退出）。
- **postStart**: 用于初始化配置，但不保证在 EntryPoint 之后执行。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

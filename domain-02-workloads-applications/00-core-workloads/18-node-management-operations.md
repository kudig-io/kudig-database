---
title: 27 - 节点与节点池管理 (Node & NodePool Management)
description: '# 27 - 节点与节点池管理 (Node & NodePool Management)'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- kubelet
- hpa
- vpa
- pdb
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 节点与节点池管理 (Node & NodePool Management) 是什么
- 如何 节点与节点池管理 (Node & NodePool Management)
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 节点与节点池管理
- Node
- NodePool
- Management
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
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
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md
  label: '故障树: node'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
created: "2026-05-23"
---

# 27 - 节点与节点池管理 (Node & NodePool Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [ACK NodePool](https://help.aliyun.com/document_detail/160490.html)

<!-- chunk: 节点池 (NodePool) 核心架构 -->
## 节点池 (NodePool) 核心架构

| 功能 (Feature) | 描述 (Description) | 生产建议 (Best Practice) |
|---------------|-------------------|--------------------------|
| **弹性伸缩 (ASG)** | 自动增加/减少 ECS 实例 | 开启 `cluster-autoscaler` 配合 HPA/VPA, 为不同业务建独立 NodePool |
| **多规格混合** | 选定多种 ECS 规格 | 将 Spot 与按量/包年实例混用, 设置合适 `expander` 策略(如 `least-waste`) |
| **自定义 OS** | 支持 Alibaba Cloud Linux / ContainerOS | 生产环境推荐 ContainerOS, 统一 OS 版本, 禁止手工登录改配置 |
| **自动修复** | 节点 NotReady 时自动重启或替换 | 关键生产环境必须开启, 同时结合 `PodDisruptionBudget` 控制重启节奏 |
| **分级隔离** | 不同安全等级/环境的节点池 | 通过 Node 标签/污点严格区分 `prod/staging/dev` 与 `internet/intranet` |

<!-- chunk: 节点生命周期与运维流程 (Node Lifecycle) -->
## 节点生命周期与运维流程 (Node Lifecycle)

| 阶段 | 关键操作 | 建议命令 | 注意事项 |
|------|----------|----------|----------|
| **准备 (Provision)** | 通过 NodePool 创建/扩容节点 | ACK 控制台或 `cluster-autoscaler` | 统一镜像与 [[kubelet|kubelet]] 配置, 预置监控/日志 [[DaemonSet|DaemonSet]] |
| **接入 (Join)** | 节点加入集群并打标签 | `kubectl label nodes` / `kubectl taint nodes` | 加入后立刻补齐 `env=prod`、`zone=xxx` 等业务标签 |
| **维护 (Maintain)** | 打补丁/升级内核/重启宿主机 | `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` | 搭配 PDB, 控制同时维护的节点数量 |
| **下线 (Decommission)** | 永久移除节点 | `kubectl drain` → `kubectl delete node` | 先确认无绑定本地盘/本地日志, 相关 Pod 已在其他节点稳定运行 |

```bash
# 查看节点 & 池
kubectl get nodes -o wide
kubectl get nodepool -A 2>/dev/null || echo "在 ACK 控制台查看 NodePool 配置"

# 按标签筛选节点
kubectl get nodes -l env=prod
```

<!-- chunk: 调度与隔离: 标签与污点 (Label & Taint) -->
## 调度与隔离: 标签与污点 (Label & Taint)

| 能力 | 示例 | 作用 |
|------|------|------|
| **节点标签 (Label)** | `node.[[Kubernetes|kubernetes]].io/instance-type=ecs.g7.xlarge` | 匹配大规格计算节点, 用于 CPU 密集型服务 |
| | `zone=cn-hangzhou-h` | 控制跨 AZ 分布, 与 PV 拓扑、SLB 匹配 |
| **节点污点 (Taint)** | `kubectl taint nodes node1 role=system:NoSchedule` | 仅允许带对应容忍 (Toleration) 的系统 Pod 调度上去 |
| **Pod 亲和/反亲和** | `topologyKey: kubernetes.io/hostname` | 同一业务副本分散到不同节点/机架, 提升高可用 |

```yaml
# 专用 GPU 节点池示例
apiVersion: v1
kind: Pod
metadata:
  name: gpu-job
spec:
  nodeSelector:
    aliyun.accelerator/nvidia_name: "V100"
  tolerations:
  - key: "gpu-only"
    operator: "Equal"
    value: "true"
    effect: "NoSchedule"
```

<!-- chunk: 资源预留 (Resource Reservation) -->
## 资源预留 (Resource Reservation)

通过 `kubelet` 参数控制系统稳定性：
- `--system-reserved`: CPU/Memory 为 OS 进程预留。
- `--kube-reserved`: 为 K8s 组件 (Kubelet, Proxy) 预留。
- `--eviction-hard`: 设置硬驱逐阈值 (如 `memory<500Mi`) 防范宿主机崩溃。

> **生产建议**: 将系统/组件预留总和控制在节点容量的 10%~20%, 对大节点适当上浮。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-02-workloads-applications MOC
- [[domain-02-workloads-applications/README|Domain-4: Kubernetes工作负载管理]]
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

- 16-runtime-class-configuration
- 17-container-images-registry
- 19-scheduler-configuration
- 20-kubelet-configuration

## Related

- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]

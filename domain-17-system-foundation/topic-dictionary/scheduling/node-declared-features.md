---
title: Node Declared Features
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- scheduler
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Node Declared Features 是什么
- 如何 Node Declared Features
trigger_keywords:
- Node
- Declared
- Features
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# Node Declared Features

## 概述

节点声明特性（Node Declared Features）是 Kubernetes v1.35 中引入的 alpha 特性。Kubernetes 节点使用声明特性来报告特定新特性或特性门控功能的可用性。控制平面组件利用这些信息做出更好的决策。

## 核心概念/原理

该机制通过以下三个主要组件协同工作：

1. **Kubelet 特性报告**：每个节点启动时，kubelet 检测当前启用的托管 Kubernetes 特性，并在 Node 对象的 `.status.declaredFeatures` 字段中报告它们。只有处于活跃开发中的特性才会包含在此字段中。

2. **调度器过滤**：默认的 kube-scheduler 使用 `NodeDeclaredFeatures` 插件：
   - 在 `PreFilter` 阶段，检查 `PodSpec` 推断出 Pod 所需的节点特性集合。
   - 在 `Filter` 阶段，检查节点的 `.status.declaredFeatures` 是否满足 Pod 推断出的需求。缺少所需特性的节点不会调度该 Pod。
   - 自定义调度器也可以利用 `.status.declaredFeatures` 字段实施类似的约束。

3. **准入控制**：`nodedeclaredfeaturevalidator` 准入控制器可以拒绝那些需要节点未声明特性的 Pod，防止在 Pod 更新时出现问题。

## 关键机制或特性

- **管理版本偏差（version skew）**：在集群升级或混合版本环境中，不同节点可能启用了不同的特性，该机制有助于管理版本偏差并提高集群稳定性。
- **目标用户**：该机制主要为 Kubernetes 特性开发者引入新的节点级特性而设计，在后台工作；部署 Pod 的应用开发者不需要直接与此框架交互。
- **特性门控**：要使用节点声明特性，必须在 `kube-apiserver`、`kube-scheduler` 和 `kubelet` 组件上启用 `NodeDeclaredFeatures` 特性门控。

## 使用场景

- 集群升级期间，确保依赖新节点特性的 Pod 不会被调度到尚未升级或不支持该特性的旧节点上。
- 混合版本环境中，防止因节点特性不一致导致的调度失败或运行时错误。
- 特性开发者在引入新的节点级功能时，确保控制平面和调度器能够正确处理特性可用性差异。

## 最佳实践/注意事项

- 这是一个 alpha 特性，默认禁用，需要在 apiserver、scheduler 和 kubelet 上显式启用 `NodeDeclaredFeatures` 特性门控。
- 应用开发者通常不需要直接与此框架交互。
- 自定义调度器可以通过读取 Node 的 `.status.declaredFeatures` 来实现自己的特性匹配逻辑。

## 生产 YAML 示例

### 查看节点声明特性

```bash
# 查看节点的 declaredFeatures
kubectl get node worker-01 -o jsonpath='{.status.declaredFeatures}' | jq .
```

### 启用 NodeDeclaredFeatures 特性门控

```yaml
# kube-apiserver 启动参数
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
    - name: kube-apiserver
      command:
        - kube-apiserver
        - --feature-gates=NodeDeclaredFeatures=true
        # ... 其他参数
---
# kubelet 配置（/var/lib/kubelet/config.yaml）
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  NodeDeclaredFeatures: true
---
# kube-scheduler 配置
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: default-scheduler
    plugins:
      preFilter:
        enabled:
          - name: NodeDeclaredFeatures
      filter:
        enabled:
          - name: NodeDeclaredFeatures
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 节点 `.status.declaredFeatures` 为空 | kubelet 未启用特性门控 | 确认 kubelet 配置中 `NodeDeclaredFeatures=true` |
| Pod 调度失败，提示特性不匹配 | 目标节点版本较旧，不支持所需特性 | `kubectl get node -o yaml` 对比不同节点的 declaredFeatures |
| 升级后新旧节点行为不一致 | 版本偏差导致特性集差异 | 先升级 control plane，再逐步升级 worker 节点 |
| 准入控制器拒绝 Pod | Pod 需要节点未声明的特性 | 检查 `nodedeclaredfeaturevalidator` 准入控制器日志 |

## 生产检查清单

- [ ] 在 apiserver、scheduler、kubelet 上统一启用 `NodeDeclaredFeatures` 特性门控
- [ ] 配置调度器启用 `NodeDeclaredFeatures` 插件（PreFilter + Filter）
- [ ] 在集群升级期间制定节点滚动升级策略，避免版本偏差过大
- [ ] 启用 `nodedeclaredfeaturevalidator` 准入控制器
- [ ] 监控不同节点的 declaredFeatures 差异

## 命令快速参考

```bash
# 查看所有节点的 declaredFeatures
kubectl get nodes -o custom-columns='NAME:.metadata.name,FEATURES:.status.declaredFeatures'

# 查看特定节点的详细特性
kubectl get node <node-name> -o jsonpath='{.status.declaredFeatures}' | jq .

# 检查 kubelet 特性门控配置
ssh <node> cat /var/lib/kubelet/config.yaml | grep -A 5 featureGates

# 查看调度器插件状态
kubectl logs -n kube-system -l component=kube-scheduler | grep NodeDeclaredFeatures
```

## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度器 Filter 阶段如何使用 NodeDeclaredFeatures
- [调度框架](./scheduling-framework.md) — PreFilter / Filter 扩展点说明
- [将 Pod 分配给节点](./assigning-pods-to-nodes.md) — nodeSelector 与 NodeDeclaredFeatures 互补

## 参考链接

- [Kubernetes 官方文档 - Node Declared Features](https://kubernetes.io/docs/concepts/scheduling-eviction/node-declared-features/)

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]

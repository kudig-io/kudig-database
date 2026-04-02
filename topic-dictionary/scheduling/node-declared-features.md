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

## 参考链接

- [Kubernetes 官方文档 - Node Declared Features](https://kubernetes.io/docs/concepts/scheduling-eviction/node-declared-features/)

---
title: Operator 模式
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- helm
- statefulset
- rbac
- crd
- operator
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator 模式 是什么
- 如何 Operator 模式
trigger_keywords:
- Operator
- 模式
- dictionary
title_en: Operator Pattern
---


# Operator 模式

## 概述

Operator 是 Kubernetes 的软件扩展，它利用自定义资源（Custom Resources）来管理应用程序及其组件。Operator 遵循 Kubernetes 的设计原则，尤其是控制循环（Control Loop）模式。其核心目标是捕获人类运维专家管理服务的知识和行为，并通过代码实现自动化。

## 核心概念/原理

- **Operator 定义**：Operator 是 Kubernetes API 的客户端，充当自定义资源的控制器。它将一个或多个自定义资源与控制器关联起来，从而扩展集群的行为，而无需修改 Kubernetes 本身的代码。
- **控制循环**：Operator 持续观察自定义资源的实际状态，并通过调谐（reconcile）使其向期望状态靠拢。
- **声明式 API**：用户声明期望状态（如数据库副本数、版本），Operator 负责执行复杂的运维操作来实现该状态。

## 关键机制或特性

- **自定义资源 + 控制器**：Operator 通常由两部分组成：
  1. **CustomResourceDefinition（CRD）**：定义新的资源类型（如 `SampleDB`）。
  2. **控制器（Controller）**：运行在 Deployment 中的 Pod 内，持续监听 CR 的变化并执行相应的运维逻辑。
- **典型自动化能力**：
  - 按需部署应用
  - 执行应用状态的备份与恢复
  - 处理应用升级及关联变更（如数据库 schema 迁移、配置更新）
  - 为不支持 Kubernetes API 的应用发布 Service 以供发现
  - 模拟集群故障以测试弹性
  - 为分布式应用选举领导者
- **部署方式**：最常见的方式是将 CRD 和对应的控制器一起部署到集群中。控制器通常作为 Deployment 运行在控制平面之外。

## 使用场景

- 管理有状态应用（如数据库、消息队列、缓存集群），需要复杂的生命周期管理（部署、扩容、备份、恢复、升级）。
- 需要将人类运维专家的经验（如故障处理、配置优化）编码为自动化逻辑。
- 希望在 Kubernetes 中通过声明式方式管理第三方中间件或自定义应用。

## 最佳实践/注意事项

- 在生态中寻找已有的 Operator（如 OperatorHub.io），避免重复造轮子。
- 如果没有现成的 Operator，可以使用多种语言和框架自行开发，如 Go（kubebuilder、Operator SDK）、Python（Kopf）、Java（Java Operator SDK）、Rust（kube-rs）、.NET（KubeOps）等。
- Operator 控制器通常以 Deployment 形式运行，需要为其配置适当的 RBAC 权限。
- 设计 Operator 时，应充分考虑故障恢复、幂等性、升级兼容性以及与现有 Kubernetes 原生资源（如 StatefulSet、PersistentVolumeClaim）的协作。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| CR 创建后无响应 | Operator Pod 未运行或未 watch 该 namespace | `kubectl get pods -n <operator-ns>`；检查 Operator 日志 |
| Operator 频繁重启 | RBAC 权限不足或代码 panic | 查看 Operator Pod 日志和事件 |
| 调协循环不收敛 | Reconcile 逻辑有 bug 导致无限更新 | 检查 Operator 日志中的 reconcile 频率 |
| CRD 升级后 Operator 不兼容 | API 版本不匹配 | 确认 Operator 版本与 CRD 版本匹配 |

## 生产检查清单

- [ ] Operator 使用 leader election 确保单实例运行
- [ ] 配置最小 RBAC 权限
- [ ] 实现 Reconcile 幂等逻辑
- [ ] 设置合理的 Reconcile requeue 间隔
- [ ] 使用 OLM 或 Helm 管理 Operator 生命周期
- [ ] 监控 Operator 自身的健康和性能

## 命令快速参考

```bash
# 查看 Operator Pod
kubectl get pods -n <operator-namespace> -l app=<operator>

# 查看 Operator 日志
kubectl logs -n <operator-namespace> -l app=<operator> --tail=100

# 查看 Operator 管理的 CR
kubectl get <cr-type> -A
```

## 交叉引用

- [Custom Resources](./custom-resources.md) — CRD 定义
- [扩展 Kubernetes API](./extending-the-kubernetes-api.md) — API 扩展方式对比
- [Admission Webhook](./admission-webhook-good-practices.md) — Webhook 作为 Operator 辅助

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/operator/

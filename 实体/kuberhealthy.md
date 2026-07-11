---
title: Kuberhealthy (entities)
description: '## 概述'
summary: 'Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控工具。'
category: entities
tags:
- k8s
- cncf
- observability
- kuberhealthy
- prometheus
- grafana
- daemonset
- job
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuberhealthy 是什么
- 如何 Kuberhealthy
trigger_keywords:
- Kuberhealthy
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuberhealthy

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控（Synthetic Monitoring）工具，由 Comcast 开发，2020 年加入 CNCF 沙箱。它通过运行 Kubernetes Job 来执行主动健康检查，将检查结果以 Prometheus 指标格式输出。与传统的被动监控不同，Kuberhealthy 采用合成监控方法——主动模拟用户行为来测试集群功能是否正常，如"创建一个 Deployment 并验证 Pod 是否 Running"、"解析一个 DNS 名称"、"挂载一个 PVC"等。这些检查以 khcheck CRD 声明式定义，支持自定义检查镜像，可以验证 DNS、部署、存储、网络等各方面的集群健康状态。

## 核心能力

- **合成监控**: 通过 Kubernetes Job 执行主动健康检查，模拟真实工作负载
- **丰富检查项**: 内置 DNS 解析、Deployment 创建、DaemonSet 部署、Pod 重启、PodStatus 等检查
- **自定义检查**: 使用任何容器镜像编写自定义检查逻辑
- **Prometheus 集成**: 检查结果直接导出为 Prometheus 指标（kuberhealthy_check）
- **CRD 配置**: 使用 khcheck/khstate CRD 声明式定义和管理检查
- **多命名空间**: 支持跨命名空间和集群范围的健康检查

## 架构

Kuberhealthy 采用 Controller + Check Job 模式：

- **Kuberhealthy Controller**: 核心控制器，管理所有 khcheck 资源的生命周期
- **khcheck CRD**: 声明式健康检查定义（检查镜像、运行频率、超时时间）
- **Check Pod (Job)**: Kuberhealthy Controller 根据 khcheck 创建的临时 Pod 执行检查
- **Check Protocol**: 检查 Pod 通过特定退出码（0=OK，1=Failure）和 stdout JSON 报告结果
- **State Storage**: khstate CRD 存储每个检查的当前状态（OK/Error/运行中）
- **Metrics Exporter**: 暴露 Prometheus 格式指标供 scrape

检查流程：`khcheck → Controller → Check Job (Pod) → 执行检查 → Exit Code → khstate → Prometheus`

## K8s 集成

Kuberhealthy 以 Helm Chart 部署在 Kubernetes 集群中。Controller 以 Deployment 运行，监听 khcheck CRD。每个 khcheck 定义了检查镜像和运行频率，Controller 定期创建 Check Pod（通过 Kubernetes Job）执行检查。Check Pod 执行完毕后通过退出码报告检查结果，Controller 更新对应的 khstate CRD。Prometheus 通过 scrape Kuberhealthy 的 metrics endpoint 获取所有检查的状态指标。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Job、CronJob、ConfigMap 等原生资源深度集成。

## 生产场景

1. **集群功能验证**: 定期验证 DNS、网络、存储等关键集群功能是否正常
2. **SLA 合成监控**: 从用户视角主动测试"部署一个应用"是否成功，验证服务可用性
3. **自定义业务检查**: 编写自定义检查镜像验证特定业务逻辑（如"数据库连接是否正常"）
4. **多集群健康对比**: 在多个集群部署 Kuberhealthy，对比各集群的健康指标

## 安装

```bash
# Helm 安装 Kuberhealthy
helm repo add kuberhealthy https://kuberhealthy.github.io/kuberhealthy/helm-repos
helm install kuberhealthy kuberhealthy/kuberhealthy -n kuberhealthy --create-namespace

# 部署 DNS 检查
kubectl apply -f https://raw.githubusercontent.com/kuberhealthy/kuberhealthy/master/cmd/dns-resolution-check/dns-check.yaml

# 部署 Deployment 检查
kubectl apply -f https://raw.githubusercontent.com/kuberhealthy/kuberhealthy/master/cmd/deployment-check/deployment-check.yaml

# 查看检查状态
kubectl get khstate -A

# 查看检查指标
kubectl port-forward svc/kuberhealthy -n kuberhealthy 8080:80
curl http://localhost:8080/metrics | grep kuberhealthy
```

## 对比

| 特性 | Kuberhealthy | Prometheus | Blackbox Exporter | Synthetic Monitoring |
|------|-------------|-----------|-------------------|---------------------|
| 合成监控 | ✅ K8s 原生 | ❌ 被动 | ✅ 外部探测 | ✅ |
| K8s 资源检查 | ✅ | ⚠️ | ❌ | ❌ |
| 自定义检查 | ✅ 任意镜像 | ⚠️ | ❌ | ⚠️ |
| CNCF 状态 | Sandbox | Graduated | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Kuberhealthy 属于 **Observability** 类别，为云原生应用提供合成监控和综合健康检查能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/observability-pillars.md|observability-pillars]]
- [[pod-lifecycle]]

## Related

- [[kubefleet]] — KubeFleet
- [[kuma]] — Kuma
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kuberhealthy
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

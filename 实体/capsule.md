---
title: Capsule (entities)
description: '## 概述'
summary: 'Capsule 是一个 Kubernetes 多租户框架，允许在单个集群中实现多租户隔离。'
category: entities
tags:
- k8s
- cncf
- policy
- capsule
- prometheus
- ingress
- rbac
- networkpolicy
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
- Capsule 是什么
- 如何 Capsule
trigger_keywords:
- Capsule
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Capsule

> **CNCF 状态**: Sandbox | **类别**: Policy/Multi-tenancy | **主要语言**: Go

## 概述

Capsule 是一个 Kubernetes 多租户框架，由 Clastix 开发，2021 年加入 CNCF 沙箱。它允许在单个 Kubernetes 集群中实现多租户隔离，通过 Tenant CRD 将多个命名空间（Namespace）组织为逻辑单元，为每个租户提供隔离的资源配额、网络策略和 RBAC 控制。与传统的"每租户一集群"方案相比，Capsule 显著降低了运维复杂度和成本——所有租户共享同一个控制面，但各自拥有隔离的命名空间集合。租户所有者（Tenant Owner）可以在自己的租户内自助创建命名空间，而无需集群管理员权限。Capsule 还内置了 Capsule Proxy 组件，为租户提供 kubectl 的自定义视图。

## 核心能力

- **多租户隔离**: 单集群内实现 Tenant 级别的资源隔离
- **命名空间聚合**: 将多个命名空间归属到单个 Tenant
- **资源配额**: Tenant 级别的 ResourceQuota 和 LimitRange 控制
- **网络隔离**: 自动应用 NetworkPolicy 实现租户间网络隔离
- **RBAC 管理**: 租户所有者自助管理命名空间和资源
- **自定义策略**: 限制 NodePort、Ingress Class、StorageClass、镜像来源等

## 架构

Capsule 采用 CRD + Webhook 的控制模式：

- **Capsule Controller**: 监听 Tenant CRD，为每个租户创建和维护关联资源
- **Tenant CRD**: 定义租户的属主、命名空间限制、资源配额、策略规则
- **Capsule Proxy**: 可选的反向代理，为租户提供定制化的 Kubernetes API 视图
- **Validating/Mutating Webhook**: 在命名空间创建和资源操作时执行策略验证
- **Tenant Resource Quota**: 自动在租户的命名空间中创建 ResourceQuota
- **NetworkPolicy 自动注入**: 为每个命名空间自动创建 deny-all 的 NetworkPolicy

管理模式：`集群管理员 → Tenant CRD → Tenant Owner → 自助创建 Namespace → Pod/Service`

## K8s 集成

Capsule 以 Kubernetes Operator 方式运行，通过 Tenant CRD 管理多租户。集群管理员创建 Tenant CRD 指定租户属主和策略限制，租户属主获得在其 Tenant 下创建 Namespace 的权限（通过 Kubernetes RBAC 的 RoleBinding 实现）。Capsule Controller 监听 Namespace 创建，验证是否归属有效的 Tenant，并自动注入 NetworkPolicy、ResourceQuota 等策略。Capsule Proxy 是可选组件，通过 kube-rbac-proxy 提供租户视角的 API 代理。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 RBAC、ResourceQuota、NetworkPolicy 机制配合使用。

## 生产场景

1. **SaaS 多租户平台**: 在单个集群中为不同客户提供隔离的命名空间和资源配额
2. **企业多团队共享**: 多个开发团队共享一个集群，各自管理自己的命名空间
3. **成本优化**: 用单集群替代多集群方案，减少控制面开销和运维成本
4. **合规隔离**: 满足不同业务线的网络和资源隔离要求

## 安装

```bash
# Helm 安装 Capsule
helm repo add capsule https://clastix.github.io/charts
helm install capsule capsule/capsule \
  --namespace capsule-system --create-namespace \
  --set manager.options.forceTenantPrefix=true

# 创建租户
kubectl apply -f - <<EOF
apiVersion: capsule.clastix.io/v1beta2
kind: Tenant
metadata:
  name: gas-station
spec:
  owners:
  - name: alice
    kind: User
  resourceQuotas:
    scope: Tenant
    items:
    - hard:
        limits.cpu: "8"
        limits.memory: 16Gi
        persistentvolumeclaims: "10"
  networkPolicies:
    items:
    - egress:
      - {}
      ingress:
      - from:
        - podSelector: {}
      podSelector: {}
EOF

# 租户属主自助创建命名空间
kubectl create namespace gas-prod --as=alice
```

## 对比

| 特性 | Capsule | vCluster | Kiosk | Multi-tenancy Bench |
|------|---------|----------|-------|---------------------|
| 隔离方式 | Namespace 聚合 | 虚拟集群 | Namespace | 多种方案 |
| 自助 Namespace | ✅ | ✅ | ✅ | ⚠️ |
| 无额外开销 | ✅ | ❌ 控制面 | ✅ | ✅ |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | WG |

## 架构定位

在 CNCF 生态中，Capsule 属于 **Policy/Multi-tenancy** 类别，为云原生应用提供轻量级多租户隔离能力。

## 参考链接

- [[实体/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[buildpacks]] — Cloud Native Buildpacks
- [[kube-rs]] — kube-rs
- [[02-prometheus-promql-advanced]] — PromQL 高级查询
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- capsule
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

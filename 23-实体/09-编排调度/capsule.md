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

Capsule 以 Kubernetes Operator 方式运行，通过 Tenant CRD 管理多租户。集群管理员创建 Tenant CRD 指定租户属主和策略限制，租户属主获得在其 Tenant 下创建 Namespace 的权限（通过 Kubernetes RBAC 的 RoleBinding 实现）。Capsule Controller 监听 Namespace 创建，验证是否归属有效的 Tenant，并自动注入 NetworkPolicy、ResourceQuota 等策略。Capsule Proxy 是可选组件，通过 kube-rbac-proxy 提供租户视角的 API 代理。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 RBAC、ResourceQuota、NetworkPolicy 机制配合使用。

## 生产场景

1. **SaaS 多租户平台**: 在单个集群中为不同客户提供隔离的命名空间和资源配额
2. **企业多团队共享**: 多个开发团队共享一个集群，各自管理自己的命名空间
3. **成本优化**: 用单集群替代多集群方案，减少控制面开销和运维成本
4. **合规隔离**: 满足不同业务线的网络和资源隔离要求

## 安装与配置

```bash
# Helm 安装 Capsule
helm repo add capsule https://clastix.github.io/charts
helm install capsule capsule/capsule \
  --namespace capsule-system --create-namespace \
  --set manager.options.forceTenantPrefix=true

kubectl get pods -n capsule-system
```

### 租户配置

```yaml
apiVersion: capsule.clastix.io/v1beta2
kind: Tenant
metadata:
  name: gas-station
spec:
  owners:
  - name: alice
    kind: User
  - name: team-gas
    kind: Group
  resourceQuotas:
    scope: Tenant
    items:
    - hard:
        limits.cpu: "8"
        limits.memory: 16Gi
        persistentvolumeclaims: "10"
        services.loadbalancers: "2"
  networkPolicies:
    items:
    - egress:
      - {}
      ingress:
      - from:
        - podSelector: {}
      podSelector: {}
  limitRanges:
    items:
    - limits:
      - default:
          cpu: 500m
          memory: 512Mi
        defaultRequest:
          cpu: 100m
          memory: 128Mi
        type: Container
  imagePullPolicies:
    - Always
  containerRegistries:
    allowed:
      - registry.internal.com
```

```bash
# 租户属主自助创建命名空间
kubectl create namespace gas-prod --as=alice
kubectl create namespace gas-staging --as=alice
```

## 运维操作

```bash
# 🟢 查看租户状态
kubectl get tenants
kubectl describe tenant gas-station

# 🟢 查看租户命名空间
kubectl get namespaces -l capsule.clastix.io/tenant=gas-station

# 🟡 修改租户配额
kubectl patch tenant gas-station --type=merge -p '{"spec":{"resourceQuotas":{"items":[{"hard":{"limits.cpu":"16"}}]}}}'

# 🟡 添加租户属主
kubectl patch tenant gas-station --type=json -p '[{"op":"add","path":"/spec/owners/-","value":{"name":"bob","kind":"User"}}]'

# 🔴 删除租户（级联删除所有命名空间）
kubectl delete tenant gas-station
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 无法创建 Namespace | 非租户属主 | `kubectl auth can-i create ns --as=alice` | 确认 owners 配置 |
| 配额未生效 | ResourceQuota 未同步 | `kubectl get resourcequota -n gas-prod` | 检查 Tenant spec |
| 网络隔离失效 | NetworkPolicy 未应用 | `kubectl get networkpolicy -n gas-prod` | 确认 CNI 支持 NP |
| 镜像拉取被拒绝 | registry 不在白名单 | `kubectl describe pod -n gas-prod` | 添加到 containerRegistries |
| 租户前缀强制失败 | forceTenantPrefix 未启用 | 检查 Helm values | 重新配置 --set |

```
排查流程:
├── 租户创建失败
│   ├── kubectl get tenants → 检查状态
│   ├── kubectl logs -n capsule-system → controller 日志
│   └── 确认 CRD 版本匹配
├── 权限问题
│   ├── kubectl auth can-i --as=<user> → 检查权限
│   └── 确认 RBAC 和 owners 配置
└── 隔离失效
    ├── kubectl get networkpolicy → 确认 NP 存在
    └── 确认 CNI 插件支持 NetworkPolicy
```

## 生产案例

### 案例 1: 多团队单集群整合

- **场景**: 5 个业务团队各自维护独立集群，资源利用率低、运维成本高
- **方案**: 部署 Capsule 实现单集群多租户；每个团队一个 Tenant，自助管理 Namespace；统一配额和网络策略
- **效果**: 集群数量从 5 减少到 1，资源利用率提升 40%，运维成本降低 60%

### 案例 2: 租户资源超卖防护

- **场景**: 某团队突发负载占满集群资源，影响其他团队
- **方案**: 配置 Tenant 级别 ResourceQuota 和 LimitRange；设置 Pod 优先级和抢占策略
- **效果**: 资源隔离有效，单团队突发不再影响其他租户

## 对比

| 特性 | Capsule | vCluster | Kiosk | Multi-tenancy Bench | 适用场景 |
|------|---------|----------|-------|---------------------|----------|
| 隔离方式 | Namespace 聚合 | 虚拟集群 | Namespace | 多种方案 | 架构选择 |
| 自助 Namespace | ✅ | ✅ | ✅ | ⚠️ | 开发者自助 |
| 无额外开销 | ✅ | ❌ 控制面 | ✅ | ✅ | 性能敏感 |
| 网络隔离 | ✅ NP | ✅ 完全 | ⚠️ | ✅ | 安全合规 |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | WG | 生态成熟度 |

## 架构定位

在 CNCF 生态中，Capsule 属于 **Policy/Multi-tenancy** 类别，为云原生应用提供轻量级多租户隔离能力。

## 参考链接

- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]

## Related

- [[buildpacks]] — Cloud Native Buildpacks
- [[kube-rs]] — kube-rs
- [[23-实体/07-可观测性/01-prometheus-promql-advanced]] — PromQL 高级查询
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- capsule
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

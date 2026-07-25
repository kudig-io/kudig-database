---
title: Cozystack (entities)
description: '## 概述'
summary: 'Cozystack 是一个开源的 PaaS 平台，基于 Kubernetes 构建，旨在提供类似云厂商的托管服务体验。它允许平台工程师在裸金属或任何基础设施上快速搭建一个完整的云平台，提供托管 Kubernetes 集群、数据库（PostgreSQL、MySQL、Redis）、消息队列、监控等服务。'
category: entities
tags:
- k8s
- cncf
- platform
- cozystack
- etcd
- prometheus
- grafana
- helm
- argocd
- flux
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cozystack 是什么
- 如何 Cozystack
trigger_keywords:
- Cozystack
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cozystack

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

Cozystack 是由 Flant 开发的开源 PaaS（Platform-as-a-Service）平台，2024 年进入 CNCF Sandbox。它在 Kubernetes 之上构建了一层抽象，让平台工程师可以在裸金属或任何基础设施上快速搭建一个**完整的云平台**——提供类似 AWS/GCP/阿里云的托管服务体验。租户通过简单的 CRD 创建托管 Kubernetes 集群、PostgreSQL/MySQL/Redis 数据库、Kafka 消息队列和监控服务，无需了解底层 K8s 配置。

Cozystack 使用 **FluxCD** 作为 GitOps 引擎管理平台和应用配置，通过 **Talos Linux** 作为不可变节点操作系统。它提供**多租户隔离**——每个租户获得独立的命名空间和资源配额，通过 OPA/Gatekeeper 策略确保隔离性。平台通过 Helm Charts 或 Krew 管理托管服务的生命周期（安装、升级、备份、监控）。

## Key Features

- **托管服务目录**：一键创建 PostgreSQL、MySQL、Redis、Kafka、MongoDB 等托管服务
- **托管 K8s 集群**：通过 CRD 创建和管理子 Kubernetes 集群（基于 Cluster API）
- **多租户隔离**：租户级命名空间隔离、资源配额和网络策略
- **GitOps 管理**：基于 FluxCD 的声明式平台配置，Git 作为唯一来源
- **不可变 OS**：使用 Talos Linux 作为节点 OS，提升安全性和一致性
- **内置监控**：开箱即用的 Prometheus + Grafana 监控平台和租户服务

## Architecture

Cozystack 由 **Cozystack Controller**（平台级控制器，管理租户、托管服务和子集群）、**FluxCD**（GitOps 引擎，同步 Git 仓库中的配置到集群）、**Cluster API**（子 Kubernetes 集群生命周期管理）、**Talos Linux**（节点 OS）和 **Helm Charts 仓库**（托管服务的安装模板）组成。租户通过创建 CR（如 `PostgreSQL`、`ManagedKubernetes`）触发控制器安装对应服务。

## K8s 集成

Cozystack 本身运行在 Kubernetes 之上（作为管理集群）。租户通过 CRD 创建托管服务——这些 CR 被 Cozystack Controller 处理，通过 Helm Chart 或 Operator 安装实际服务到租户命名空间。托管 Kubernetes 子集群通过 Cluster API 创建，管理集群负责子集群的控制平面和节点生命周期。

## 生产部署要点

- **基础设施规划**：预先规划存储（Ceph）和网络拓扑
- **租户隔离**：为每个团队创建独立的租户命名空间和资源配额
- **GitOps 管理**：将所有平台配置纳入 Git 仓库管理
- **监控**：利用内置 Prometheus/Grafana 监控平台和租户服务
- **备份策略**：为所有有状态服务配置定期备份

## 生产场景

1. **私有云 PaaS**：企业在裸金属上搭建私有云平台，内部团队按需创建服务
2. **托管数据库服务**：租户通过 CR 一键创建 PostgreSQL/Redis，自动配置备份和监控
3. **多团队平台**：多个开发团队共享一个 Cozystack 平台，资源隔离且自服务
4. **边缘 PaaS**：在边缘数据中心运行 Cozystack，为本地应用提供托管服务

## 安装与配置

```bash
# 前提：需要一个运行 Talos Linux 的 Kubernetes 管理集群
helm repo add cozystack https://cozystack.github.io/charts
helm repo update
helm install cozystack cozystack/cozystack \
  -n cozy-system --create-namespace \
  --set flux.enabled=true \
  --set monitoring.enabled=true

kubectl get pods -n cozy-system
```

### 租户配置

```yaml
apiVersion: cozy.io/v1alpha1
kind: Tenant
metadata:
  name: team-alpha
spec:
  namespace: tenant-alpha
  resourceQuota:
    requests.cpu: "20"
    requests.memory: 40Gi
  networkPolicy: enabled
```

### 托管 PostgreSQL

```yaml
apiVersion: helm.cozystack.io/v1alpha1
kind: PostgreSQL
metadata:
  name: mydb
  namespace: tenant-alpha
spec:
  version: "15"
  replicas: 3
  storage: 100Gi
  backup:
    enabled: true
    schedule: "0 2 * * *"
    retention: 7
  resources:
    requests:
      cpu: "1"
      memory: 2Gi
```

## 运维操作

```bash
# 🟢 查看租户状态
kubectl get tenants -A
kubectl describe tenant team-alpha

# 🟢 查看托管服务
kubectl get postgresql,redis,kafka -A

# 🟡 创建托管服务
kubectl apply -f postgresql.yaml

# 🟡 扩容存储
kubectl patch postgresql mydb -n tenant-alpha --type=merge -p '{"spec":{"storage":"200Gi"}}'

# 🔴 删除托管服务（数据不可恢复）
kubectl delete postgresql mydb -n tenant-alpha
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 租户创建失败 | 配额超限 | `kubectl describe tenant` | 调整 resourceQuota |
| DB 创建失败 | 存储不足 | `kubectl get pvc -n tenant-alpha` | 扩容存储/检查 SC |
| 备份失败 | S3 凭据错误 | `kubectl logs backup-pod` | 更新备份凭据 |
| 监控无数据 | Prometheus 未启用 | `kubectl get pods -n monitoring` | 检查 monitoring 配置 |
| Flux 同步失败 | Git 仓库不可达 | `kubectl logs -n flux-system` | 检查 Git 凭据 |

```
排查流程:
├── 平台组件异常
│   ├── kubectl get pods -n cozy-system → 控制平面状态
│   ├── kubectl logs -n cozy-system → 查看错误
│   └── 确认 Talos 集群健康
├── 托管服务异常
│   ├── kubectl describe postgresql <name> → 查看状态
│   ├── kubectl get pods -n tenant-alpha → Pod 状态
│   └── 检查 PVC 和存储状态
└── 租户问题
    ├── kubectl describe tenant → 查看配额使用
    └── 确认 NetworkPolicy 配置
```

## 生产案例

### 案例 1: 私有云 PaaS 建设

- **场景**: 企业需要内部 PaaS，团队自助创建数据库和中间件
- **方案**: 部署 Cozystack，定义租户模板；开发者通过 CR 一键创建 PG/Redis；自动配置备份和监控
- **效果**: 服务交付从工单 3 天缩短到自助 5min，运维团队工作量减少 70%

### 案例 2: 多团队资源隔离

- **场景**: 多团队共享集群，资源争抢严重
- **方案**: 每个团队一个 Tenant，配置独立配额和网络策略；团队内自服务，跨团队隔离
- **效果**: 资源争抢问题消除，各团队独立运维互不影响

## 对比

| 特性 | Cozystack | KubeVista | CapRover | Kamaji | 适用场景 |
|------|-----------|-----------|---------|--------|----------|
| 托管 DB | ✅ PG/MySQL/Redis | ✅ | ⚠️ | ❌ | 数据库服务 |
| 托管 K8s | ✅ Cluster API | ❌ | ❌ | ✅ | 多集群 |
| GitOps | ✅ FluxCD | ⚠️ | ❌ | ⚠️ | 持续交付 |
| 多租户 | ✅ | ⚠️ | ⚠️ | ⚠️ | 平台工程 |
| 监控内置 | ✅ | ✅ | ⚠️ | ❌ | 可观测性 |

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[23-实体/08-交付与制品/argocd.md|argocd]]
- [[operator-pattern]]

## Related

- [[helm]] — Helm
- [[cloudevents]] — CloudEvents
- [[keda]] — KEDA
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cozystack
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

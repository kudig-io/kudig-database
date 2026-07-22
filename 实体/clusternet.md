---
title: Clusternet (entities)
description: '## 概述'
summary: 'Clusternet 是一个多集群管理和应用分发平台，专为管理跨云、跨区域的 Kubernetes 集群而设计。它采用 Hub-Agent 架构，支持 Pull 和 Push 两种模式进行集群注册，能够将应用资源（Deployment、[[Service|Service]]、Helm Release 等）智能分发到多个子集群。'
category: entities
tags:
- k8s
- cncf
- orchestration
- clusternet
- prometheus
- grafana
- helm
- crd
- operator
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Clusternet 是什么
- 如何 Clusternet
trigger_keywords:
- Clusternet
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Clusternet

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Clusternet 是由华为开源的多集群管理和应用分发平台，专为管理跨云、跨区域的 Kubernetes 集群而设计，2021 年加入 CNCF Sandbox。它采用 Hub-Agent 架构，支持 Pull 和 Push 两种模式进行集群注册，能够将应用资源（Deployment、[[Service|Service]]、Helm Release 等）智能分发到多个子集群。Clusternet 特别适合边缘计算和混合云场景，即使子集群位于 NAT 或防火墙后面也能通过 Pull 模式注册连接。

## 核心特性

- **Hub-Agent 架构**: Hub 集群集中管理，Agent 部署在子集群主动连接
- **Pull/Push 双模式**: Pull 模式适配 NAT 后的边缘集群，Push 模式适配云上集群
- **Subscription 模型**: 类似 Kubernetes 的 Label Selector，按标签选择目标集群
- **Helm 分发**: 支持将 Helm Chart 分发到多个集群并管理 Release
- **Override 策略**: 为不同集群定制差异化的配置覆盖
- **多集群调度**: 按权重或策略将工作负载分发到多个集群

## 架构

Clusternet 由 Hub 和 Agent 两部分组成。Hub 集群运行 Clusternet Hub（API 聚合层、Webhook、Controller），通过 `clusters.clusternet.io` CRD 管理注册的子集群。Agent（clusternet-agent）以 Deployment 运行在子集群中，通过 Pull 模式注册到 Hub 并建立 WebSocket 连接。用户创建 Subscription（定义分发内容和目标集群选择器）后，Controller 生成 Base 和 Localization（Override）资源，Agent 通过 FeedIn（安全隧道）将资源应用到本地集群。

## Kubernetes 集成

Clusternet 通过 CRD（ManagedCluster、Subscription、Base、Localization、FeedIn、HelmChart）实现多集群管理。Hub 聚合了所有子集群的 API 访问能力，通过 FeedIn（基于 WebSocket 的反向代理）安全访问 NAT 后的子集群。Subscription CRD 类似 Deployment 的多集群扩展，定义 What（分发什么）和 Where（分发到哪里）。Override 策略支持 JSON Patch 实现差异化配置。

## 生产使用场景

1. **边缘集群管理**: 管理大量位于 NAT 后的边缘 Kubernetes 集群
2. **混合云分发**: 将应用统一分发到公有云和私有数据中心集群
3. **多环境部署**: 按标签选择开发/测试/生产集群，差异化配置
4. **渐进式发布**: 先分发到灰度集群验证，再扩大到全量集群

## 安装与配置

```bash
# Hub 集群
helm repo add clusternet https://clusternet.github.io/charts
helm install clusternet-hub clusternet/clusternet-hub \
  -n clusternet-system --create-namespace
# 注册子集群
helm install clusternet-agent clusternet/clusternet-agent \
  -n clusternet-system --create-namespace \
  --set hubURL=https://hub.example.com \
  --set parentAPIServerToken=<token>
```

### Subscription 分发示例

```yaml
apiVersion: apps.clusternet.io/v1alpha1
kind: Subscription
metadata:
  name: nginx-multi-cluster
  namespace: default
spec:
  subscribers:
    - clusterAffinity:
        matchLabels:
          env: production
      schedulerStrategy:
        dividingScheduling:
          dividingType: Dynamic
          preferredClusters:
            - cluster-1
  feeds:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
      namespace: default
    - apiVersion: v1
      kind: Service
      name: nginx-svc
      namespace: default
```

### Override 差异化配置

```yaml
apiVersion: apps.clusternet.io/v1alpha1
kind: Localization
metadata:
  name: nginx-override-edge
spec:
  clusterID: edge-cluster-01
  overrides:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
      jsonPatch:
        - op: replace
          path: /spec/replicas
          value: 1
        - op: add
          path: /spec/template/spec/tolerations
          value:
            - key: edge
              operator: Exists
```

## 运维操作

```bash
# 🟢 查看已注册子集群
kubectl get managedclusters -n clusternet-system

# 🟢 查看集群连接状态
kubectl get managedclusters -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[0].type

# 🟢 查看 Subscription 分发状态
kubectl get subscriptions -A
kubectl describe subscription nginx-multi-cluster

# 🟢 查看 FeedIn 资源状态
kubectl get feedin -A

# 🟡 创建新的 Subscription 分发应用
kubectl apply -f subscription.yaml

# 🟡 更新 Override 策略
kubectl apply -f localization.yaml

# 🔴 注销子集群
kubectl delete managedcluster <cluster-name> -n clusternet-system

# 🟢 查看 Helm Release 分发状态
kubectl get helmreleases -A
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 子集群未注册 | Agent 无法连接 Hub | `kubectl logs -n clusternet-system deploy/clusternet-agent` | 检查 Hub URL 和 Token |
| 分发失败 | Subscription 选择器无匹配 | `kubectl describe subscription <name>` | 检查 clusterAffinity 标签 |
| FeedIn 超时 | WebSocket 连接中断 | `kubectl get feedin -A` | 检查网络连通性 |
| Override 未生效 | JSON Patch 路径错误 | `kubectl describe localization <name>` | 验证 Patch 路径与资源结构匹配 |
| Helm Release 失败 | Chart 仓库不可达 | `kubectl logs deploy/clusternet-agent \| grep helm` | 检查 Chart 仓库 URL 和认证 |

### 排查流程

```
Clusternet 分发异常
├─ 子集群未注册？
│  ├─ Agent Pod 未运行 → 检查资源/RBAC
│  ├─ Hub URL 不可达 → 检查 DNS/防火墙
│  └─ Token 无效 → 重新生成 ServiceAccount Token
├─ 分发未触发？
│  ├─ Subscription 无匹配集群 → 检查标签选择器
│  └─ Controller 异常 → 检查 Hub 组件日志
└─ 资源应用失败？
   ├─ FeedIn 报错 → 检查子集群 RBAC 权限
   └─ 资源冲突 → 检查命名空间/名称冲突
```

## 生产案例

### 案例 1: 运营商 5G 边缘集群管理

**场景**: 某电信运营商需管理 200+ 位于基站侧的边缘 K8s 集群，均在 NAT 后。

**方案**:
1. 中心机房部署 Clusternet Hub
2. 边缘集群通过 Pull 模式注册（Agent 主动连接 Hub）
3. 使用 Subscription 统一分发 5G UPF 工作负载
4. Override 策略为不同区域配置差异化参数

**效果**: 单团队管理 200+ 集群，应用发布时间从 2 天缩短到 10 分钟。

### 案例 2: 混合云多集群渐进式发布

**场景**: 企业应用需同时部署到 AWS、阿里云和私有数据中心。

**方案**:
1. 按环境标签分组集群（canary/stable）
2. 先分发到 canary 集群验证
3. 确认无问题后扩大到 stable 集群
4. 使用 Override 为不同云配置不同的 StorageClass 和 Ingress

**效果**: 多云发布全流程自动化，回滚时间 < 1 分钟。

## 对比与替代方案

| 维度 | Clusternet | OCM | Karmada | ArgoCD AppSet |
|------|------------|-----|---------|---------------|
| 边缘适配 | ✅ Pull 模式 | ❌ Push | 部分 | ❌ |
| 调度能力 | 中 | 低 | 强 | 低 |
| Helm 分发 | ✅ | ✅ | ✅ | ✅ |
| Override | JSON Patch | 有限 | 强 | 有限 |
| CNCF 状态 | Sandbox | Incubating | Incubating | Graduated |
| 架构复杂度 | 中 | 低 | 高 | 低 |

## 检查清单

- [ ] Hub 集群高可用部署（多副本）
- [ ] 子集群 Agent 使用 Pull 模式（NAT 环境）
- [ ] ServiceAccount Token 定期轮换
- [ ] Subscription 标签选择器已测试验证
- [ ] Override 策略已在非生产环境验证
- [ ] WebSocket 连接监控告警已配置
- [ ] 分发失败自动重试策略已配置
- [ ] 集群注册/注销审计日志已开启

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Clusternet** | 边缘场景适配好、Pull 模式 | 社区较小 |
| OCM (Open Cluster Management) | Red Hat 支持、API 清晰 | 不支持边缘 Pull |
| Karmada | CNCF Incubating、调度强 | 架构较重 |
| ArgoCD + ApplicationSet | GitOps 原生 | 非专门的多集群平台 |

## 架构定位

在 CNCF 生态中，Clusternet 属于 **Orchestration / Multi-Cluster** 类别，专注于边缘计算和混合云场景的多集群管理。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[runme-notebooks]] — Runme
- [[operator-framework]] — Operator Framework
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- clusternet
- [[概念/etcd x 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

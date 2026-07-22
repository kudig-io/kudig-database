---
title: KubeFleet [entities]
description: '## 概述'
summary: 'KubeFleet 是一个多集群资源编排平台，提供跨 Kubernetes 集群的工作负载分发、配置管理和策略驱动的资源放置能力。它通过 Hub-Member 架构和声明式 Placement 策略，实现将 Kubernetes 资源（Deployment、[[Service|Service]]、ConfigMap 等）自动分发到多个成员集群，'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubefleet
- cri-o
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
- KubeFleet 是什么
- 如何 KubeFleet
trigger_keywords:
- KubeFleet
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeFleet

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeFleet 是一个 CNCF 沙箱项目，由 Microsoft 开源，专注于 Kubernetes 多集群应用编排和资源调度。它提供统一的管理平面，将应用工作负载智能分发到多个集群，支持基于资源可用性、标签策略和地理位置的调度决策。KubeFleet 特别关注大规模边缘和混合云场景，解决多集群环境下的应用部署、配置管理和生命周期协调问题。项目是 Azure Kubernetes Fleet Manager 的开源核心。

## Key Features（核心能力）

- **多集群调度**：基于资源容量、标签约束和亲和性的智能工作负载调度
- **资源预留**：在目标集群预留资源确保部署成功
- **渐进式部署**：支持跨集群的滚动更新和金丝雀发布
- **配置传播**：跨集群的 ConfigMap、Secret、RBAC 等配置同步
- **集群分组**：通过集群属性（Property）和分组（ClusterGroup）管理集群
- **冲突解决**：自动处理多集群资源冲突和覆盖

## 架构与工作原理

KubeFleet 采用 Hub-Spoke 架构：Hub Cluster 运行 Fleet Manager 控制器，管理工作负载分发策略和集群状态；Member Clusters 运行 Fleet Agent，接收并执行分发指令。核心 CRD 包括 ClusterProperty（集群属性）、ClusterGroup（集群分组）、MemberCluster（成员集群注册）。调度引擎通过 Resource Distribution Controller 将工作负载按策略分发到目标集群，并跟踪各集群的部署状态。

## K8s 集成

KubeFleet 通过丰富的 CRD 与 Kubernetes 集成：MemberCluster CRD 注册成员集群；ClusterResourcePlacement CRD 定义资源分发策略（目标集群、调度约束、部署策略）；ClusterGroup CRD 定义集群分组。Hub Controller 通过各成员集群的 kubeconfig 连接到远程 API Server，推送配置和监控状态。Agent 在成员集群中协调实际资源创建。

## 生产用例

- **多集群应用部署**：将应用统一部署到多个生产集群
- **边缘计算编排**：将工作负载分发到地理分布的边缘集群
- **灾难恢复**：跨集群的工作负载快速迁移和恢复
- **多环境管理**：统一管理 dev/staging/prod 的应用部署

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add kubefleet https://azure.github.io/fleet/charts
helm repo update

# 🟢 安装 Fleet Manager (Hub Cluster)
helm install fleet kubefleet/fleet-manager \
  -n fleet-system --create-namespace \
  --set enableV1Alpha1CRDs=true \
  --set enableV1Beta1CRDs=true

# 🟢 验证安装
kubectl get pods -n fleet-system
kubectl get crd | grep fleet.azure.com

# 🟢 注册成员集群
# 在 Hub 集群创建 MemberCluster
kubectl apply -f member-cluster.yaml

# 🟢 在成员集群安装 Fleet Agent
helm install fleet-agent kubefleet/fleet-agent \
  -n fleet-system --create-namespace \
  --set config.hubKubeConfigSecret=hub-kubeconfig
```

### MemberCluster CRD 示例

```yaml
apiVersion: cluster.kubernetes-fleet.io/v1beta1
kind: MemberCluster
metadata:
  name: prod-east
spec:
  identity:
    name: fleet-agent-prod-east
    kind: ServiceAccount
    namespace: fleet-system
    apiGroup: ""
  labels:
    region: east
    tier: production
    cloud: azure
  taints:
    - key: maintenance
      value: "true"
      effect: NoSchedule
---
apiVersion: cluster.kubernetes-fleet.io/v1beta1
kind: ClusterResourcePlacement
metadata:
  name: nginx-deployment
spec:
  resourceSelectors:
    - group: apps
      version: v1
      kind: Deployment
      name: nginx
      labelSelector:
        matchLabels:
          app: nginx
  policy:
    placementType: PickN
    numberOfClusters: 3
    affinity:
      clusterAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
          clusterSelectorTerms:
            - labelSelector:
                matchLabels:
                  tier: production
    tolerations:
      - key: maintenance
        operator: Exists
        effect: NoSchedule
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
```

## 运维操作

```bash
# 🟢 查看成员集群状态
kubectl get membercluster -o wide

# 🟢 查看资源放置状态
kubectl get clusterresourceplacement -o wide
kubectl describe clusterresourceplacement nginx-deployment

# 🟢 查看集群属性和分组
kubectl get clusterproperty
kubectl get clustergroup

# 🟡 强制重新调度（修改 Placement 触发）
kubectl annotate clusterresourceplacement nginx-deployment \
  fleet.azure.com/force-reschedule=$(date +%s) --overwrite

# 🟡 排除成员集群（添加 Taint）
kubectl patch membercluster prod-east --type=merge -p \
  '{"spec":{"taints":[{"key":"maintenance","value":"true","effect":"NoSchedule"}]}}'

# 🔴 删除成员集群注册（会清理该集群上所有 Fleet 管理的资源）
kubectl delete membercluster prod-east
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| MemberCluster 状态 Unknown | Agent 断连 | `kubectl get membercluster -o wide` | 检查 Agent Pod 日志和网络 |
| Placement 卡在 Scheduling | 无满足条件的集群 | `kubectl describe crp <name>` | 检查集群标签和策略约束 |
| 资源未同步到成员集群 | 网络/权限问题 | 成员集群 `kubectl get deploy` | 检查 Agent RBAC 和网络连通 |
| 滚动更新卡住 | 集群不可用 | `kubectl get crp <name> -o yaml` | 检查 maxUnavailable 和集群健康 |

```bash
# 排查流程
# 1. 检查 Hub Controller 状态
kubectl logs -n fleet-system -l app=fleet-controller --tail=100

# 2. 检查成员集群连接
kubectl get membercluster -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[0].reason}{"\n"}{end}'

# 3. 检查 Placement 调度决策
kubectl get crp nginx-deployment -o jsonpath='{.status.conditions}' | jq .

# 4. 检查成员集群 Agent
kubectl logs -n fleet-system -l app=fleet-agent --tail=100
```

## 生产案例

### 案例1：跨地域多集群应用部署
- **场景**：金融企业需要将交易系统部署到3个地域集群，确保就近访问和容灾
- **方案**：使用 ClusterResourcePlacement + PickN 策略，按地域标签选择目标集群；配置 RollingUpdate 策略确保零停机更新；通过 ClusterProperty 标记集群容量和地域属性
- **效果**：部署时间从 30min 缩短到 3min，跨集群配置一致性达到 100%

### 案例2：边缘集群批量管理
- **场景**：零售企业 200+ 边缘集群需要统一更新 POS 应用
- **方案**：使用 ClusterGroup 按区域分组，分批滚动更新；通过 Taint 机制排除维护中的集群；配置 maxUnavailable=10% 控制更新速度
- **效果**：200+ 集群全量更新从 2天 缩短到 4小时，零业务中断

## 对比替代方案

| 维度 | KubeFleet | Karmada | KubeStellar | Open Cluster Management |
|------|-----------|---------|-------------|------------------------|
| CNCF 状态 | Sandbox | Incubating | Sandbox | Incubating |
| 调度策略 | 丰富(标签/容量/亲和) | 丰富 | 基础 | 中等 |
| Azure 集成 | 原生 | 无 | 无 | 无 |
| 社区规模 | 中 | 大 | 小 | 大 |
| 边缘场景 | 强 | 中 | 强 | 中 |
| 学习曲线 | 中 | 高 | 低 | 中 |

## 检查清单

- [ ] Hub Cluster 已部署 Fleet Manager 且 Pod Running
- [ ] 成员集群已注册且 MemberCluster 状态为 Ready
- [ ] Fleet Agent 在成员集群正常运行
- [ ] ClusterResourcePlacement 策略已验证（先在测试环境）
- [ ] 滚动更新参数已配置（maxUnavailable/maxSurge）
- [ ] 集群标签和分组已正确设置
- [ ] 网络连通性已验证（Hub ↔ Member 双向）
- [ ] RBAC 权限已正确配置

## Related

- [[cedar]] — Cedar
- [[cri-o]] — CRI-O
- [[shipwright]] — Shipwright
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubefleet
- [[实体/cncf-orchestration.md|[[CNCF 编排与应用管理项目全景|CNCF 编排与应用管理项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

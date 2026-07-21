---
title: 多云与多集群管理知识词典
description: 涵盖 Kubernetes 多云、多集群、联邦调度、边缘计算等分布式集群管理领域的完整术语体系与技术参考
summary: 多云多集群管理领域词典，覆盖 Federation、Cluster API、Crossplane、Karmada、边缘计算等核心概念
category: dictionary
tags:
- dictionary
- multi-cloud
- multi-cluster
- federation
- edge-computing
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- 平台工程师
- SRE
- 架构师
---

# 多云与多集群管理知识词典（Multi-Cloud & Multi-Cluster）

> 本词典覆盖 Kubernetes 多云多集群管理领域的核心术语、技术组件及工程实践，是平台工程师和 SRE 管理分布式集群的权威参考。

## 领域概述

多云与多集群管理是云原生架构演进的高级阶段，解决的核心问题包括：

- **单集群局限性**：单个 K8s 集群存在节点数上限（推荐 ≤5000 节点）、故障域集中、地域延迟等问题
- **业务连续性**：跨地域/跨云部署实现容灾和高可用
- **合规要求**：数据主权、属地化部署等法规约束
- **成本优化**：利用多云竞价实例、区域价格差异降低 TCO
- **供应商锁定规避**：保持架构可移植性

## 核心术语定义

### 集群联邦（Federation）

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Federation v2 (KubeFed) | K8s 原生多集群联邦标准，通过联邦控制平面统一管理跨集群资源 | kubefed |
| FederatedTypeConfig | 定义哪些资源类型参与联邦同步的 CRD | KubeFed |
| Placement Policy | 定义资源分发到哪些成员集群的策略 | KubeFed/Clusternet |
| Override Policy | 针对特定集群的资源差异化覆盖规则 | KubeFed/Karmada |
| Member Cluster | 加入联邦的被管理集群 | 通用 |
| Host Cluster | 运行联邦控制平面的宿主集群 | KubeFed |

### 多集群编排

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Cluster API (CAPI) | 声明式集群生命周期管理框架，支持多云 Provider | cluster-api |
| Machine/MachineSet | CAPI 中管理节点生命周期的抽象资源 | CAPI |
| ClusterClass | CAPI 集群模板，定义标准化集群拓扑 | CAPI v1beta1 |
| Karmada | CNCF 多集群编排引擎，兼容 K8s API 的联邦方案 | Karmada |
| Open Cluster Management (OCM) | Red Hat 主导的轻量级多集群管理框架 | OCM |
| Clusternet | 腾讯云开源的多集群管理框架，支持影子集群模式 | Clusternet |
| Fleet | Rancher 的 GitOps 多集群持续交付方案 | Rancher Fleet |

### 基础设施即代码（IaC）与云资源编排

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Crossplane | 将云基础设施抽象为 K8s CRD 的控制平面框架 | Crossplane |
| Composition | Crossplane 中组合多个云资源为自定义 API 的机制 | Crossplane |
| Provider (Crossplane) | Crossplane 对接特定云 API 的插件 | provider-aws/gcp/azure |
| Cloud Credential Operator | 管理多云凭据分发与轮换的 Operator | CCO |
| Terraform Controller | 在 K8s 中运行 Terraform 的 Operator 方案 | tf-controller |

### 边缘计算与轻量集群

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| K3s | Rancher 轻量级 K8s 发行版（<100MB），适用于边缘/IoT | K3s |
| KubeEdge | 华为开源的云边协同框架，扩展 K8s 到边缘节点 | KubeEdge |
| OpenYurt | 阿里云开源的云边一体平台，支持节点池管理 | OpenYurt |
| SuperEdge | 腾讯开源的边缘计算框架，支持边缘自治 | SuperEdge |
| MicroK8s | Canonical 的零运维轻量 K8s，适用于开发/边缘 | MicroK8s |
| Spaceborne Computing | 天基计算概念，将 K8s 调度延伸到卫星/太空节点 | 前沿研究 |

### 多集群服务与流量管理

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Multi-Cluster Service (MCS) | K8s SIG 多集群服务发现标准 API | mcs-api |
| ServiceImport/ServiceExport | MCS API 中跨集群服务注册与发现的核心资源 | mcs-api |
| 联邦 Ingress | 跨集群统一入口流量管理 | KubeFed Ingress |
| 全局负载均衡 (GSLB) | 基于 DNS/Anycast 的跨集群流量调度 | CoreDNS/ExternalDNS |
| Cluster Gateway | 多集群间安全通信的网关代理 | OCM ClusterGateway |

## 技术组件索引

### 联邦与编排类

- [[系统基础/知识字典/multi-cloud/federation.md|Federation（集群联邦）]]
- [[系统基础/知识字典/multi-cloud/cluster-api.md|Cluster API（声明式集群管理）]]
- [[系统基础/知识字典/multi-cloud/multi-cluster-service.md|Multi-Cluster Service（多集群服务发现）]]
- [[系统基础/知识字典/multi-cloud/multi-cloud-operations.md|Multi-Cloud Operations（多云运维）]]

### 基础设施编排类

- [[系统基础/知识字典/multi-cloud/crossplane-composition.md|Crossplane Composition（云资源组合）]]
- [[系统基础/知识字典/multi-cloud/cloud-credential-operator.md|Cloud Credential Operator（凭据管理）]]

### 边缘计算类

- [[系统基础/知识字典/multi-cloud/edge-computing-and-k3s.md|Edge Computing & K3s（边缘计算）]]
- [[系统基础/知识字典/multi-cloud/spaceborne-computing.md|Spaceborne Computing（天基计算）]]

## 架构模式对比

| 模式 | 适用场景 | 复杂度 | 代表方案 |
|------|----------|--------|----------|
| 联邦模式 | 统一策略下发、全局视图 | 高 | KubeFed, Karmada |
| GitOps 分发 | 配置一致性、审计追踪 | 中 | Fleet, ArgoCD App-of-Apps |
| 服务网格联邦 | 跨集群流量管理、mTLS | 高 | Istio Multi-Primary |
| IaC 编排 | 基础设施生命周期 | 中 | Crossplane, CAPI |
| 边缘自治 | 弱网/离线边缘节点 | 中 | KubeEdge, OpenYurt |

## 生产最佳实践

### 多集群架构设计

1. **渐进式联邦**：从 2-3 个集群开始验证，逐步扩展
2. **Hub-Spoke 拓扑**：中心集群运行控制平面，成员集群保持独立运行能力
3. **故障域隔离**：每个集群独立可用区，联邦控制平面 3 副本跨 AZ
4. **版本策略**：成员集群版本差异不超过 2 个小版本

### 安全与合规

1. **凭据管理**：使用 Cloud Credential Operator 或 External Secrets，避免静态凭据
2. **网络隔离**：集群间通信走 mTLS 加密隧道，禁止明文 API 暴露
3. **RBAC 联邦**：统一身份认证（OIDC），集群级细粒度授权
4. **数据主权**：通过 Placement 约束确保数据类工作负载在合规区域运行

### 运维与可观测性

1. **统一监控**：Thanos/Mimir 聚合多集群指标，Grafana 全局视图
2. **日志聚合**：每集群 Loki/Fluentd → 中心日志平台
3. **集群健康巡检**：定期验证联邦控制平面与成员集群的连通性
4. **灾难恢复**：联邦 etcd 独立备份，成员集群可脱离联邦独立运行

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| 资源未同步到成员集群 | Placement 不匹配/成员集群 Unreachable | 检查 KubeFed 控制器日志、集群 Ready 状态 |
| 跨集群服务发现失败 | MCS API 未启用/DNS 配置错误 | 验证 ServiceExport/Import、CoreDNS 配置 |
| Cluster API 创建集群超时 | 云 Provider 配额不足/网络不通 | 检查 CAPI Controller 日志、云控制台配额 |
| 边缘节点失联后数据不一致 | 边缘自治模式下的状态同步延迟 | 检查 EdgeCore 连接、手动触发状态同步 |

## 学习路径

```
基础: K8s 单集群管理 → 多集群概念理解
进阶: Cluster API 实践 → Karmada/KubeFed 部署
高级: Crossplane IaC → 多集群服务网格 → 全球多活架构
前沿: 边缘自治 → 天基计算 → AI 驱动的多集群调度
```

## 深度技术解析

### KubeFed 架构与工作原理

KubeFed v2 的核心架构由以下组件构成：

```
联邦控制平面 (Host Cluster)
├── kubefed-controller-manager    # 核心控制器
│   ├── FederatedTypeConfig Controller  # 管理联邦资源类型
│   ├── Placement Controller           # 计算资源分发目标
│   ├── Override Controller            # 应用集群级覆盖
│   └── Sync Controller                # 执行资源同步
├── kubefed-admission-webhook     # 准入控制
└── etcd (Host Cluster)           # 联邦状态存储

成员集群 (Member Clusters)
├── cluster1 (us-east-1)
├── cluster2 (eu-west-1)
└── cluster3 (ap-southeast-1)
```

**资源同步流程：**

1. 用户创建 Federated 资源（如 FederatedDeployment）
2. Placement Controller 解析 Placement 策略，确定目标集群列表
3. Override Controller 应用集群级差异化配置
4. Sync Controller 将渲染后的资源分发到各成员集群
5. 各成员集群的本地控制器接管实际工作负载管理
6. Sync Controller 持续监控状态，确保一致性

### Cluster API 生命周期管理

Cluster API 提供声明式集群生命周期管理：

```yaml
# ClusterClass 定义标准化集群模板
apiVersion: cluster.x-k8s.io/v1beta1
kind: ClusterClass
metadata:
  name: production-cluster
spec:
  controlPlane:
    ref:
      apiVersion: controlplane.cluster.x-k8s.io/v1beta1
      kind: KubeadmControlPlaneTemplate
      name: prod-cp
    machineInfrastructure:
      ref:
        kind: AWSMachineTemplate
        name: prod-cp-infra
  workers:
    machineDeployments:
    - class: default-worker
      template:
        spec:
          clusterName: "${cluster.name}"
          version: "v1.30.0"
---
# 使用 ClusterClass 创建集群
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: prod-us-east
spec:
  topology:
    class: production-cluster
    version: v1.30.0
    controlPlane:
      replicas: 3
    workers:
      machineDeployments:
      - class: default-worker
        name: md-0
        replicas: 5
```

### Karmada 与 KubeFed 对比

| 维度 | Karmada | KubeFed v2 |
|------|---------|------------|
| 社区活跃度 | CNCF 孵化，华为主导 | 维护模式，社区不活跃 |
| API 兼容性 | 原生 K8s API，无需 Federated 前缀 | 需要 FederatedXxx 包装 |
| 调度能力 | 内置多集群调度器，支持权重/亲和 | 简单 Placement 策略 |
| 流量管理 | 集成多集群流量调度 | 需额外组件 |
| 生态集成 | 支持 Helm/Kustomize/ArgoCD | 有限 |
| 生产就绪度 | 华为/工行/携程等生产使用 | 逐渐被替代 |

### Crossplane Composition 工作原理

```yaml
# Composition 定义：将云资源组合为自定义 API
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: compositepostgresqlinstance
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: CompositePostgreSQLInstance
  resources:
  - name: rdsinstance
    base:
      apiVersion: rds.aws.crossplane.io/v1beta2
      kind: Instance
      spec:
        forProvider:
          region: us-east-1
          dbInstanceClass: db.t3.medium
          engine: postgres
          engineVersion: "15"
    patches:
    - fromFieldPath: spec.parameters.storageGB
      toFieldPath: spec.forProvider.allocatedStorage
  - name: securitygroup
    base:
      apiVersion: ec2.aws.crossplane.io/v1beta1
      kind: SecurityGroup
```

## 生产案例研究

### 案例一：全球电商平台多集群架构

**背景：** 某电商平台需要在北美、欧洲、亚太三个区域部署，要求：
- 各区域独立运行，区域间故障不影响其他区域
- 统一配置管理，确保一致性
- 数据合规，用户数据不跨境

**架构方案：**
- 使用 Karmada 作为多集群编排引擎
- 每个区域 3 个 K8s 集群（跨 AZ）
- GitOps (ArgoCD) 管理应用配置
- Istio Multi-Primary 实现区域内服务网格
- 全局 DNS (Route53 + ExternalDNS) 实现流量调度

**关键决策：**
- 选择 Karmada 而非 KubeFed：社区活跃、原生 API 兼容
- 数据层不联邦：各区域独立数据库，通过应用层同步
- 渐进式上线：先 2 个区域验证，再扩展第 3 个

### 案例二：边缘计算 IoT 平台

**背景：** 某制造企业在全国 200+ 工厂部署边缘计算节点，管理产线传感器数据。

**架构方案：**
- 云端：Karmada 控制平面 + 中心集群
- 边缘：K3s 轻量集群（每工厂 3 节点）
- 使用 OpenYurt 实现边缘自治（断网后本地继续运行）
- 节点池管理：按工厂/产线分组

**关键挑战与解决：**
- 弱网环境：边缘自治 + 异步状态同步
- 大规模管理：节点池批量操作 + 模板化部署
- 资源受限：K3s 轻量运行时 + 精简组件集

## 多云成本优化策略

| 策略 | 实现方式 | 预期节省 |
|------|----------|----------|
| 竞价实例混合 | CAPI MachinePool 配置 Spot 比例 | 40-70% 计算成本 |
| 跨区域调度 | Karmada 权重调度到低价区域 | 15-30% |
| 资源右调 | 多集群统一 VPA/推荐引擎 | 20-40% |
| 预留实例池 | 基线负载用 RI，弹性用 Spot | 30-50% |
| 存储分层 | 热数据本地，冷数据对象存储 | 50-80% 存储成本 |

## 安全架构设计

### 多集群零信任架构

```
身份层: OIDC Provider (Keycloak/Dex)
    │
    ├── 联邦控制平面: mTLS + RBAC
    │
    ├── 集群间通信: Service Mesh mTLS / WireGuard
    │
    ├── 工作负载: Pod Identity / IRSA / Workload Identity
    │
    └── 凭据管理: External Secrets + Vault
```

### 网络隔离模型

1. **管理网络**：联邦控制平面 ↔ 成员集群 API Server（专用 VPC Peering）
2. **业务网络**：集群间业务流量（Service Mesh 加密隧道）
3. **数据网络**：数据库复制/同步流量（独立子网 + ACL）

## 监控与可观测性

### 多集群监控架构

```
各成员集群:
  Prometheus (local) → Thanos Sidecar
                         │
中心集群:                ▼
  Thanos Query ← Thanos Store Gateway
       │              │
       ▼              ▼
  Grafana (Global Dashboard)
       │
       ▼
  Alertmanager (Global Routing)
```

### 关键监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| cluster_api_cluster_status | CAPI 集群状态 | phase != Provisioned |
| kubefed_cluster_ready | 联邦成员集群就绪 | Ready = False 持续 5min |
| karmada_cluster_ready | Karmada 集群状态 | Ready = False |
| crossplane_managed_resource_ready | Crossplane 资源状态 | Synced = False |
| edge_node_connectivity | 边缘节点连接 | 断连 > 10min |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| Karmada | v1.8+ | v1.9+ | v1.10+ | v1.11+ |
| Cluster API | v1.6+ | v1.7+ | v1.8+ | v1.9+ |
| Crossplane | v1.14+ | v1.15+ | v1.16+ | v1.17+ |
| KubeEdge | v1.15+ | v1.16+ | v1.17+ | v1.18+ |
| OpenYurt | v1.3+ | v1.4+ | v1.5+ | v1.6+ |

## 多集群迁移策略

### 单集群 → 多集群迁移路径

```
阶段1: 评估与规划
  └─ 工作负载分类（无状态/有状态/数据密集）
  └─ 依赖关系梳理（服务间调用、共享存储）
  └─ 网络拓扑设计（跨集群通信方案）

阶段2: 基础设施准备
  └─ 目标集群部署（CAPI 或云托管）
  └─ 多集群管理平面部署（Karmada/OCM）
  └─ 网络打通（VPC Peering / Transit Gateway）

阶段3: 渐进式迁移
  └─ 无状态服务先行（Deployment 类）
  └─ 有状态服务跟进（StatefulSet + 数据同步）
  └─ 流量切换（DNS 权重 / GSLB）

阶段4: 优化与固化
  └─ 多集群调度策略调优
  └─ 统一可观测性建设
  └─ 运维自动化（Day-2 Operations）
```

### 跨云迁移检查清单

| 检查项 | 说明 | 状态 |
|--------|------|------|
| 应用无云厂商 API 依赖 | 移除 AWS SDK/GCP Client 等硬编码 | ☐ |
| 存储抽象化 | 使用 CSI 而非云特定 Volume 插件 | ☐ |
| 网络策略可移植 | NetworkPolicy 而非云 SG 规则 | ☐ |
| 密钥管理统一 | External Secrets / Vault 而非云 KMS 直连 | ☐ |
| DNS 解耦 | ExternalDNS 而非云 DNS 服务绑定 | ☐ |
| 镜像仓库可访问 | 多区域镜像同步（Harbor Replication） | ☐ |

## 常见问题 FAQ

**Q1: KubeFed 和 Karmada 应该选哪个？**

A: 新项目强烈建议 Karmada。KubeFed 已进入维护模式，社区不再活跃开发。Karmada 是 CNCF 孵化项目，兼容原生 K8s API，无需 Federated 前缀包装，且内置多集群调度器。已有 KubeFed 的用户建议规划迁移到 Karmada。

**Q2: 多集群和单集群大规模哪个更好？**

A: 取决于场景。单集群上限约 5000 节点，超过后 API Server 延迟显著增加。如果业务有跨地域、跨云、故障域隔离需求，多集群是必然选择。如果仅是规模问题，先考虑单集群优化（API Priority and Fairness、etcd 调优）。

**Q3: 边缘节点断网后如何保证业务连续？**

A: 使用边缘自治方案（KubeEdge/OpenYurt/SuperEdge）。核心机制：边缘节点本地缓存所有工作负载 spec，断网后本地 kubelet 继续运行已有 Pod，网络恢复后自动同步状态。注意：断网期间无法创建新 Pod 或扩缩容。

**Q4: 多集群环境如何处理有状态服务？**

A: 有状态服务（数据库、消息队列）通常不联邦，而是每集群独立部署 + 应用层数据同步。原因：联邦有状态服务的复杂度和数据一致性风险极高。推荐模式：每区域独立数据库 + 异步复制（如 MySQL Group Replication、CockroachDB 多区域）。

**Q5: 多集群环境的服务发现怎么做？**

A: 三种主流方案：
1. MCS API（ServiceExport/ServiceImport）：K8s 原生标准，适合集群间服务调用
2. Service Mesh 多集群（Istio Multi-Primary）：适合细粒度流量管理
3. 全局 DNS（ExternalDNS + Route53）：适合客户端直连场景

## 参考链接

- https://github.com/kubernetes-sigs/kubefed
- https://karmada.io/
- https://cluster-api.sigs.k8s.io/
- https://www.crossplane.io/
- https://open-cluster-management.io/
- https://k3s.io/
- https://kubeedge.io/
- https://openyurt.io/
- https://github.com/clusternet/clusternet

## Related

- [[系统基础/知识字典/platform-engineering/karmada.md|Karmada]]
- [[系统基础/知识字典/platform-engineering/open-cluster-management.md|OCM]]
- [[系统基础/知识字典/scheduling/kubefleet.md|KubeFleet]]
- [[系统基础/知识字典/networking/cilium.md|Cilium ClusterMesh]]
- [[系统基础/知识字典/operations/argo.md|ArgoCD 多集群 GitOps]]
- [[系统基础/知识字典/security/zero-trust.md|零信任安全架构]]

## 常用运维命令速查

```bash
# === Karmada 多集群管理 ===
# 查看成员集群状态
kubectl --kubeconfig $KARMADA_CONFIG get clusters
# 查看资源分发状态
kubectl --kubeconfig $KARMADA_CONFIG get resourcebindings -A
# 查看 Override 策略
kubectl --kubeconfig $KARMADA_CONFIG get overridepolicies -A

# === Cluster API ===
# 查看集群生命周期状态
clusterctl get clusters -A
# 查看 Machine 状态
kubectl get machines -A -o wide
# 扩容工作节点
kubectl scale machinedeployment md-0 --replicas=10
# 升级集群版本
kubectl patch cluster prod --type merge -p '{"spec":{"topology":{"version":"v1.31.0"}}}'

# === Crossplane ===
# 查看托管资源状态
kubectl get managed -A
# 查看 Provider 健康状态
kubectl get providerrevisions
# 查看 Composition 定义
kubectl get compositions

# === 多集群诊断 ===
# 检查跨集群网络连通性
kubectl --context cluster1 run test --image=nicolaka/netshoot --rm -it -- bash
# 检查 MCS 服务导出
kubectl get serviceexports -A
kubectl get serviceimports -A
```

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| CAPI | Cluster API | 声明式集群管理框架 |
| MCS | Multi-Cluster Service | 多集群服务发现标准 |
| GSLB | Global Server Load Balancing | 全局服务器负载均衡 |
| IaC | Infrastructure as Code | 基础设施即代码 |
| CCO | Cloud Credential Operator | 云凭据操作员 |
| OCM | Open Cluster Management | 开放集群管理 |
| mTLS | Mutual TLS | 双向 TLS 认证 |
| AZ | Availability Zone | 可用区 |
| TCO | Total Cost of Ownership | 总拥有成本 |
| IRSA | IAM Roles for Service Accounts | AWS Pod 身份认证 |
| VPC | Virtual Private Cloud | 虚拟私有云 |
| OIDC | OpenID Connect | 开放身份认证协议 |


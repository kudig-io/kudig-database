---
title: 'Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)'
description: '## 概述'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- flannel
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7) 是什么'
- '如何 Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)'
trigger_keywords:
- Week
- '1:'
- ACK
- ACR
- 基础与集群生命周期
- Days
- 1-7
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- etcd-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

---
title: Week 1: ACK/ACR 基础与集群生命周期
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - ACK ACR cluster lifecycle week 1 curriculum
  - [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes cluster]] lifecycle management
  - ACK week 1 learning path guide
  - ACK cluster type selection
  - Kubernetes cluster certificate management
trigger_keywords:
  - week 1
  - ACK ACR
  - cluster lifecycle
  - 集群生命周期
  - cluster type
  - 集群类型
  - SDK
  - API
  - console
  - 控制台
  - upgrade
  - 升级
  - certificate
  - 证书
reading_level: beginner
audience:
  - All week 1 learners
  - ACK beginners
  - New joiners
estimated_read_time: 30min
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - day-1-ack-acr-sr
  - day-2-ack-sdk-api
  - day-3-ack-acr-console
  - day-4-cluster-creation
  - day-5-cluster-deletion
  - day-6-cluster-upgrade
  - day-7-cluster-certificate
---

# Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)

## 概述

第一周是整个培训计划的基础阶段，聚焦于阿里云容器服务 ACK（Alibaba Cloud Kubernetes）和容器镜像服务 ACR（Alibaba Cloud Container Registry）的核心概念与操作。本周将带你从理解 ACK/ACR 的服务架构开始，逐步掌握 SDK/API 调用、控制台操作，最终能够独立完成集群的创建、删除、升级和证书管理全流程。

ACK 是阿里云提供的托管 Kubernetes 服务，它将 K8s 的复杂性封装起来，让你专注于业务应用的部署和管理。ACR 是企业级的容器镜像仓库服务，支持镜像的安全存储、高效分发和漏洞扫描。作为 K8s 运维工程师，深入理解这两个服务的架构和操作是所有后续工作的基础。

### 学习目标

- 理解 ACK 托管版、专有版、Serverless 三种集群类型的架构差异与选型依据
- 了解 ACR 企业版与个人版的区别及典型使用场景
- 掌握 ACK SDK 的使用方式与核心 API 调用
- 熟悉 ACK/ACR 控制台的功能布局与核心操作
- 能够独立完成集群的创建、删除、升级和证书管理
- **产出**: 能够独立完成集群创建、升级、删除全流程操作

---

## 核心概念详解

### ACK 集群架构与类型选择

阿里云 ACK 提供三种集群类型，适用于不同的业务场景：

**ACK 托管版（Managed Cluster）** 是最常用的集群类型。在这种模式下，Kubernetes 控制平面（包括 kube-apiserver、[[etcd|etcd]]、kube-scheduler、kube-controller-manager）由阿里云托管，用户只需要管理 Worker 节点。托管版的优势包括：无需维护控制平面组件的高可用、自动进行控制平面升级和安全补丁、控制平面的费用已经包含在集群管理费中。托管版适合绝大多数生产场景，特别是没有特殊控制平面定制需求的团队。

**ACK 专有版（Dedicated Cluster）** 提供了对控制平面的完全控制。用户需要自己提供和管理 Master 节点的 ECS 实例，自行维护 etcd 的备份和高可用。专有版的优势在于：可以对控制平面进行深度定制、满足特殊的合规要求（如数据不能存储在共享基础设施上）、可以自定义控制平面的网络和存储配置。专有版适合有特殊合规需求或需要对控制平面进行深度定制的大型企业。

**ACK Serverless（ASK）** 是一种无服务器的 K8s 运行方式。你不需要创建和管理任何 Worker 节点，只需要创建 Pod，底层由阿里云的弹性容器实例（ECI）来运行。ASK 的优势：按 Pod 实际运行时间计费、无需进行节点容量规划、适合突发性或间歇性的工作负载。ASK 适合 CI/CD 构建、离线任务处理、弹性扩容等场景。

选型建议：初创公司和中小团队首选托管版；大型企业或有特殊合规需求选择专有版；突发性工作负载或不想管理节点选择 Serverless。

| 集群类型 | 控制平面管理 | 节点管理 | 计费方式 | 适用场景 |
|----------|------------|---------|---------|---------|
| 托管版 | 阿里云托管 | 用户管理 | 集群管理费 + ECS | 大多数生产场景 |
| 专有版 | 用户管理 | 用户管理 | 仅 ECS 费用 | 合规定制需求 |
| Serverless | 阿里云托管 | 无需管理 | 按 Pod 计费 | 突发/间歇性负载 |

### ACR 容器镜像服务

ACR 是阿里云提供的企业级容器镜像仓库服务。它分为两个版本：

**ACR 个人版** 提供基本的镜像存储和分发功能，适合个人开发者和小型团队。个人版支持公网和 VPC 网络访问，支持镜像的推送和拉取，但缺乏高级功能如镜像签名、漏洞扫描等。

**ACR 企业版** 面向企业用户，提供了丰富的安全和运维功能：

- **镜像安全扫描**: 自动检测镜像中的已知漏洞（CVE），支持阻断包含高危漏洞的镜像部署
- **镜像签名**: 通过公钥加密验证镜像的完整性和来源可信度，防止被篡改的镜像运行
- **同步复制**: 支持跨地域的镜像同步，确保在全球范围内都能快速拉取镜像
- **命名空间级别的权限控制**: 通过 RAM 策略精细控制不同团队对不同命名空间的访问权限
- **Helm Chart 仓库**: 除了容器镜像，还支持 Helm Chart 的存储和版本管理

在 ACK 集群中使用 ACR 的典型流程：开发者在本地或 CI 环境构建镜像 → 推送到 ACR → 在 ACK 集群中通过 Deployment 引用 ACR 中的镜像 → kubelet 从 ACR 拉取镜像并运行容器。

| 特性 | 个人版 | 企业版 |
|------|--------|--------|
| 镜像存储 | 支持 | 支持 |
| VPC 拉取 | 支持 | 支持 |
| 安全扫描 | 基础 | 深度 (CVE 库) |
| 镜像签名 | 不支持 | 支持 |
| 跨地域同步 | 不支持 | 支持 |
| Helm Chart | 不支持 | 支持 |
| SLA | 无 | 99.99% |

### ACK SDK 与 API

ACK 提供了完整的 OpenAPI 接口，可以通过 SDK（支持 Java、Python、Go、Node.js 等）或 aliyun CLI 调用。核心 API 接口包括：

| API 接口 | 用途 | HTTP 方法 |
|----------|------|----------|
| DescribeClustersV1 | 查询集群列表 | GET |
| CreateCluster | 创建集群 | POST |
| DeleteCluster | 删除集群 | DELETE |
| UpgradeCluster | 升级集群版本 | POST |
| DescribeClusterUserKubeconfig | 获取 kubeconfig | GET |
| DescribeClusterDetail | 查看集群详情 | GET |
| DescribeClusterNodes | 查看节点列表 | GET |
| CreateClusterNodePool | 创建节点池 | POST |

使用 SDK 的基本流程：创建 Alibaba Cloud SDK 客户端 → 配置 AccessKey 和 Region → 调用 API → 处理返回结果。在实际开发中，建议使用 SDK 而非直接调用 HTTP API，因为 SDK 封装了签名、重试、错误处理等通用逻辑。

### 集群生命周期管理

**集群创建** 是最基础也最重要的操作。创建集群时需要配置的核心参数包括：

- 集群类型和 K8s 版本
- 网络配置：VPC、vSwitch、Pod CIDR、Service CIDR
- 节点池配置：实例规格、系统盘、数据盘、节点数量
- 插件配置：网络插件（Terway/Flannel）、存储插件、日志插件、监控插件

网络规划是集群创建中最容易出错的地方。VPC 的网段必须足够大以容纳所有节点和 Pod 的 IP 地址。Pod CIDR 和 Service CIDR 不能与 VPC 网段重叠。在多集群场景中，不同集群的 Pod CIDR 也不能重叠，否则无法实现跨集群通信。

| 参数 | 说明 | 注意事项 |
|------|------|---------|
| VPC CIDR | 底层网络基础 | 需包含所有 vSwitch |
| Pod CIDR | Pod IP 范围 (Flannel) | 不能与 VPC/Service CIDR 重叠 |
| Service CIDR | ClusterIP 范围 | 创建后不可修改 |
| vSwitch | 节点 IP 来源 | 多可用区部署至少 2 个 |

**集群删除** 需要注意资源的完整清理。删除集群时，ACK 会自动释放关联的 ECS 实例、SLB 实例等资源。但有些资源需要手动处理：PersistentVolume 关联的云盘、External IP、ACR 中的镜像等。建议在删除集群前先确认所有相关资源的处理方式。

**集群升级** 是生产环境中最关键也最危险的运维操作。ACK 支持原地升级（In-place Upgrade），升级过程中控制平面先升级，然后逐个升级 Worker 节点。升级前必须确认：所有工作负载的健康检查已正确配置（确保节点重启后 Pod 能够恢复）、etcd 数据已备份、应用兼容目标 K8s 版本的 API 变化。

升级流程:

```
升级前检查
  ├── API 废弃检查 (kubent)
  ├── 组件兼容性检查
  ├── 备份集群资源
  └── webhook 检查

管控面升级 (托管版自动)
  └── API Server / Scheduler / Controller Manager / etcd

节点升级 (推荐替换方式)
  ├── 扩容新版本节点
  ├── Drain 旧版本节点
  ├── 确认 Pod 迁移完成
  └── 移除旧版本节点

升级后验证
  ├── 版本确认
  ├── 组件状态检查
  └── 业务可用性验证
```

**集群证书管理** 包括：kubeconfig 证书（用于 kubectl 连接集群）、etcd 证书、API Server 证书等。证书有过期时间，过期后集群将不可用。ACK 支持证书自动轮换，建议在集群创建时就启用此功能。

| 证书类型 | 用途 | 有效期 | 轮换方式 |
|----------|------|--------|---------|
| CA 根证书 | 签发其他证书 | 10 年 | 手动 |
| API Server 证书 | 服务端认证 | 1 年 | 自动 |
| kubelet 证书 | 节点身份认证 | 1 年 | 自动 |
| kubeconfig | 用户访问凭证 | 3 年 | API 重新获取 |

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 1 | ACK/ACR 管控 SR | [day-1-ack-acr-sr.md](./day-1-ack-acr-sr.md) |
| Day 2 | ACK SDK & API | [day-2-ack-sdk-api.md](./day-2-ack-sdk-api.md) |
| Day 3 | ACK/ACR 控制台 & 功能 | [day-3-ack-acr-console.md](./day-3-ack-acr-console.md) |
| Day 4 | K8S 新建集群 | [day-4-cluster-creation.md](./day-4-cluster-creation.md) |
| Day 5 | K8S 集群删除 | [day-5-cluster-deletion.md](./day-5-cluster-deletion.md) |
| Day 6 | K8S 集群升级 | [day-6-cluster-upgrade.md](./day-6-cluster-upgrade.md) |
| Day 7 | K8S 集群证书 | [day-7-cluster-certificate.md](./day-7-cluster-certificate.md) |

### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P1**: [ACK 集群全生命周期管理](../projects/p1-ack-cluster-lifecycle.md)

---

## 常见问题

### Q1: 托管版和专有版可以互相转换吗？

不可以。集群类型在创建时确定，无法后续变更。如果需要从托管版迁移到专有版（或反向），需要创建新集群并迁移工作负载。建议在创建集群前充分评估需求，选择合适的类型。

### Q2: 集群创建失败如何排查？

集群创建失败通常与网络配置或资源配额有关。排查步骤：1) 查看 ACK 控制台的集群创建任务日志；2) 检查 VPC 和 vSwitch 是否存在且状态正常；3) 检查账户的 ECS 实例配额是否充足；4) 检查账户余额是否充足。

```bash
# 查看创建日志
aliyun cs GET /clusters/<cluster_id>/logs

# 常见失败原因:
# - vSwitch IP 耗尽
# - ECS 实例库存不足
# - CIDR 与已有网络冲突
# - RAM 权限不足
# - 账户余额不足
```

### Q3: 集群升级过程中业务会中断吗？

如果应用配置了正确的健康检查（readinessProbe）和多副本（replicas >= 2），升级过程中不会中断。节点升级时会先将 Pod 调度到其他节点，再重启当前节点。但如果是单副本应用且没有配置健康检查，可能会出现短暂不可用。

### Q4: kubeconfig 证书过期了怎么办？

可以通过 ACK 控制台重新生成 kubeconfig 文件。如果控制台也无法访问（极少见的情况），可以通过阿里云 CLI 执行 `aliyun cs GET /clusters/<cluster_id>/user_config` 获取新的 kubeconfig。

```bash
# 检查证书过期时间
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates

# 重新获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config
```

---

## 要点总结

| 主题 | 关键知识点 | 学习日 |
|------|-----------|--------|
| 服务架构 | ACK/ACR 架构、集群类型 | Day 1 |
| SDK/API | OpenAPI 调用、参数配置 | Day 2 |
| 控制台 | 功能布局、核心操作 | Day 3 |
| 集群创建 | 网络规划、参数配置 | Day 4 |
| 集群删除 | 资源清理、依赖处理 | Day 5 |
| 集群升级 | 升级策略、兼容性检查 | Day 6 |
| 证书管理 | 证书类型、轮换机制 | Day 7 |

---

## 延伸阅读

- [ACK 服务总览](../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md)
- [K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [K8s 核心组件深入](../../domain-01-cluster-fundamentals/02-core-components-deep-dive.md)
- [kubectl 命令参考](../../domain-01-cluster-fundamentals/05-kubectl-commands-reference.md)
- [集群生命周期管理](../../domain-07-platform-engineering/02-cluster-lifecycle-management.md)
- [ECS 计算资源](../../domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]

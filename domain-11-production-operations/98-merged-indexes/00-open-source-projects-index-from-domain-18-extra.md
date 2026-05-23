---
title: Domain-18 生产运维 — 开源项目索引
description: '# Domain-18 生产运维 — 开源项目索引'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- scheduler
- prometheus
- grafana
- docker
- opa
- kafka
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Domain-18 生产运维 — 开源项目索引 是什么
- 如何 Domain-18 生产运维 — 开源项目索引
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- Domain-18
- 生产运维
- 开源项目索引
- production
- operations
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- kafka-basics
- policy-basics
- logging-basics
created: "2026-05-23"
---

# Domain-18 生产运维 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: KEDA v2.17 / Cluster API v1.9 / OpenCost v1.114

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、自动伸缩](#二自动伸缩)
- [三、多集群管理](#三多集群管理)
- [四、成本优化](#四成本优化)
- [五、集群生命周期管理](#五集群生命周期管理)
- [六、节点与容量管理](#六节点与容量管理)
- [七、混沌工程](#七混沌工程)
- [八、版本兼容矩阵](#八版本兼容矩阵)
- [九、生产运维架构选型](#九生产运维架构选型)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **KEDA** | 事件驱动自动伸缩 | Graduated | v2.17.0 | 8.5k+ | Apache-2.0 |
| **Cluster API** | 声明式集群生命周期 | K8s SIG | v1.9.0 | 3.5k+ | Apache-2.0 |
| **OpenCost** | K8s 成本可视化 | Incubating | v1.114.0 | 5.5k+ | Apache-2.0 |
| **Karpenter** | 智能节点自动伸缩 | AWS 开源 | v1.3.0 | 6.5k+ | Apache-2.0 |
| **Descheduler** | Pod 重调度优化 | K8s SIG | v0.32.0 | 4k+ | Apache-2.0 |
| **VPA** | 垂直 Pod 自动伸缩 | K8s SIG | v1.3.0 | 5.5k+ | Apache-2.0 |
| **Karmada** | 多云多集群调度 | Incubating | v1.13.0 | 4.5k+ | Apache-2.0 |
| **Rancher** | 多集群管理平台 | SUSE | v2.10.0 | 23k+ | Apache-2.0 |
| **vCluster** | 虚拟集群 | Loft | v0.24.0 | 7k+ | Apache-2.0 |
| **Chaos Mesh** | 混沌工程 | Incubating | v2.7.0 | 6.5k+ | Apache-2.0 |
| **Litmus** | 混沌工程 | Incubating | v3.12.0 | 4k+ | Apache-2.0 |
| **Keptn** | 应用生命周期编排 | 非 CNCF | v2.4.0 | 1.5k+ | Apache-2.0 |
| **Kueue** | K8s 作业队列管理 | K8s SIG | v0.11.0 | 1.5k+ | Apache-2.0 |
| **Kamaji** | 托管 K8s 控制平面 | Clastix | v1.0.0 | 1k+ | Apache-2.0 |
| **Kubecost** | K8s 成本管理与优化 | Kubecost | v2.7.0 | 12k+ | Apache-2.0 |
| **Infracost** | 云成本估算 (Terraform) | Infracost | v0.11.0 | 11k+ | Apache-2.0 |
| **Capsule** | K8s 多租户框架 | Clastix | v0.7.0 | 2k+ | Apache-2.0 |
| **HNC** | 层级命名空间 | K8s SIG | v1.2.0 | 1k+ | Apache-2.0 |
| **Reloader** | ConfigMap/Secret 变更自动重启 | Stakater | v1.3.0 | 7k+ | Apache-2.0 |

---

## 二、自动伸缩

### 2.1 KEDA (CNCF Graduated)

```yaml
# 核心能力
- 基于外部事件源的 HPA
- 50+ 内置 Scaler (Kafka, RabbitMQ, SQS, Azure Queue, Prometheus, etc.)
- 支持 scale-to-zero
- 与 HPA 无缝集成
- 多触发器组合 (min replica 取最大值)
```

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: kafka-scaledobject
spec:
  scaleTargetRef:
    name: my-app
  pollingInterval: 30
  cooldownPeriod: 300
  minReplicaCount: 0
  maxReplicaCount: 50
  triggers:
  - type: kafka
    metadata:
      bootstrapServers: kafka:9092
      consumerGroup: my-group
      topic: my-topic
      lagThreshold: "100"
```

**GitHub**: https://github.com/kedacore/keda
**文档**: https://keda.sh/docs/

### 2.2 VPA (Vertical Pod Autoscaler)

- 自动调整 CPU/Memory requests/limits
- 四种模式: Off / Initial / Recreate / Auto (in-place 正在开发)
- 与 HPA 同时使用需谨慎 (建议 VPA 管资源，HPA 管副本)

### 2.3 Cluster Autoscaler vs Karpenter

| 维度 | Cluster Autoscaler | Karpenter |
|:---|:---|:---|
| 策略 | 基于节点组 | 基于实例类型直接启动 |
| 扩容速度 | 中等 (需节点初始化) | 快 (直接启动最优实例) |
| 节点多样性 | 受限于节点组配置 | 自动选择最优实例 |
| 缩容 | 优雅缩容 | 优雅缩容 + 整合 |
| 云厂商 | 多云支持 | 目前 AWS 为主，Azure/GCP 扩展中 |
| 与 KEDA 配合 | 完美配合 | 完美配合 |

---

## 三、多集群管理

### 3.1 Karmada (CNCF Incubating)

```yaml
# 核心能力
- PropagationPolicy: 多集群资源分发策略
- OverridePolicy: 集群级覆盖
- ResourceBinding: 自动调度绑定
- 故障转移 (Failover)
- 多集群 Service 发现
```

```yaml
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
spec:
  resourceSelectors:
  - apiVersion: apps/v1
    kind: Deployment
    name: nginx
  placement:
    clusterAffinity:
      clusterNames:
      - member1
      - member2
    replicaScheduling:
      replicaDivisionPreference: Weighted
      replicaSchedulingType: Divided
      weightPreference:
        staticWeightList:
        - targetCluster:
            clusterNames: [member1]
          weight: 1
        - targetCluster:
            clusterNames: [member2]
          weight: 1
```

**GitHub**: https://github.com/karmada-io/karmada

### 3.2 Rancher

- 多集群统一管理平台
- 集成监控 (Prometheus/Grafana)、日志 ([[domain-19-landscape-references/01-cncf-landscape/graduated/fluentd/fluentd|Fluentd]])、CI/CD (Fleet)
- 支持导入任意 K8s 发行版
- v2.10 增强安全与边缘支持

### 3.3 vCluster

- 在命名空间内运行虚拟集群
- 每个 vCluster 独立控制平面 (API Server + Controller Manager)
- 共享宿主集群工作节点
- 适用于多租户隔离、开发测试环境

---

## 四、成本优化

### 4.1 OpenCost (CNCF Incubating)

```yaml
# 核心能力
- 实时 K8s 成本分配
- 按 Namespace/Deployment/Service/Pod 维度
- 支持 AWS/GCP/Azure/On-Prem 定价
- Prometheus 指标导出
- Web UI 与 API
```

**集成**
```yaml
# 与 Prometheus + Grafana 集成
# OpenCost 提供官方 Grafana Dashboard
# 支持自定义告警规则 (如某 Namespace 成本突增)
```

**GitHub**: https://github.com/opencost/opencost
**文档**: https://www.opencost.io/docs/

---

## 五、集群生命周期管理

### 5.1 Cluster API (CAPI)

```yaml
# 核心概念
- Cluster: 目标集群抽象
- Machine / MachineDeployment: 节点生命周期
- KubeadmControlPlane: 控制平面管理
- 基础设施 Provider: AWS/Azure/GCP/vSphere/Docker/etc.
```

**部署示例**
```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: my-cluster
spec:
  clusterNetwork:
    pods:
      cidrBlocks: ["192.168.0.0/16"]
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: my-cp
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta2
    kind: AWSCluster
    name: my-aws-cluster
```

**GitHub**: https://github.com/kubernetes-sigs/cluster-api

---

## 六、节点与容量管理

### 6.1 Descheduler

- 基于策略的 Pod 重调度
- 内置策略: RemoveDuplicates, LowNodeUtilization, PodLifeTime, etc.
- 解决调度器短视问题 (随时间推移负载不均衡)

### 6.2 Kueue (K8s SIG)

- K8s 原生作业队列管理
- 资源配额与公平调度 (Quota / Queue / Workload)
- 支持 Job、RayJob、MPIJob、TFJob 等
- 与 Cluster Autoscaler / Karpenter 集成

---

## 七、混沌工程

### 7.1 Chaos Mesh (CNCF Incubating)

```yaml
# 故障注入类型
- PodChaos: 杀 Pod、容器故障
- NetworkChaos: 延迟、丢包、分区、带宽限制
- StressChaos: CPU/Memory 压力
- IOChaos: 文件系统延迟/错误
- DNSChaos: DNS 故障
- HTTPChaos: HTTP  abort/delay
- TimeChaos: 时间偏移
- KernelChaos: 内核故障 (使用 bpf)
```

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay
spec:
  action: delay
  mode: one
  selector:
    namespaces:
    - default
    labelSelectors:
      app: web-show
  delay:
    latency: "10ms"
    correlation: "100"
    jitter: "0ms"
```

**GitHub**: https://github.com/chaos-mesh/chaos-mesh

### 7.2 Litmus (CNCF Incubating)

- 混沌工作流编排
- ChaosHub: 预定义实验库
- 与 Argo Workflows 集成
- 故障分类与韧性评分

---

## 八、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| KEDA v2.17 | ✅ | ✅ | ✅ | metrics-server 依赖 |
| Cluster API v1.9 | ✅ | ✅ | ✅ | Provider 独立版本 |
| OpenCost v1.114 | ✅ | ✅ | ✅ | Prometheus 依赖 |
| Karpenter v1.3 | ✅ | ✅ | ✅ | AWS 为主 |
| Descheduler v0.32 | ✅ | ✅ | ✅ | 驱逐策略需调优 |
| VPA v1.3 | ✅ | ✅ | ✅ | 与 HPA 同用需谨慎 |
| Karmada v1.13 | ✅ | ✅ | ✅ | 控制平面独立 |
| vCluster v0.24 | ✅ | ✅ | ✅ | 宿主集群兼容 |
| Chaos Mesh v2.7 | ✅ | ✅ | ✅ | 特权容器 |
| Kueue v0.11 | ✅ | ✅ | ✅ | 原生集成 |

---

## 九、生产运维架构选型

```
┌─────────────────────────────────────────────────────────────┐
│                生产运维技术栈参考架构                          │
└─────────────────────────────────────────────────────────────┘

容量管理
  ├── HPA (CPU/Memory) ──► 基础水平伸缩
  ├── KEDA ──► 事件驱动伸缩 (scale-to-zero)
  ├── VPA ──► 资源请求优化
  ├── Cluster Autoscaler / Karpenter ──► 节点弹性
  └── Descheduler ──► 负载再平衡

多集群管理
  ├── Karmada ──► 应用级多集群编排
  ├── Rancher ──► 统一运维管理平面
  ├── Cluster API ──► 集群生命周期自动化
  └── vCluster ──► 虚拟集群多租户

成本治理
  ├── OpenCost ──► 实时成本归因
  ├── Kubecost (商业) ──► 企业级成本优化建议
  └── 资源配额 (ResourceQuota/LimitRange)

韧性验证
  ├── Chaos Mesh ──► 全面故障注入
  ├── Litmus ──► 混沌工作流编排
  └── 定期 Game Day ──► 人工演练

作业调度
  ├── Kueue ──► 队列与公平调度
  └── Volcano ──► 批处理 Gang Scheduling
```

---

## 参考链接

- [KEDA 官方文档](https://keda.sh/docs/)
- [Cluster API 文档](https://cluster-api.sigs.k8s.io/)
- [OpenCost 文档](https://www.opencost.io/docs/)
- [Karpenter 文档](https://karpenter.sh/docs/)
- [Chaos Mesh 文档](https://chaos-mesh.org/docs/)
- [Karmada 文档](https://karmada.io/docs/)
- [Kueue 文档](https://kueue.sigs.k8s.io/docs/)

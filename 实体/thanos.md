---
title: Thanos [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- thanos
- prometheus
- grafana
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Thanos 是什么
- 如何 Thanos
trigger_keywords:
- Thanos
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Thanos

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Thanos 是由 Improbable 开源的高可用、长期存储的 Prometheus 集群解决方案，2019 年加入 CNCF 孵化。它通过 Sidecar 模式与现有 Prometheus 实例集成，提供全局查询视图、无限数据保留和跨集群联邦能力。Thanos 的核心价值在于无需替换现有 Prometheus 即可实现水平扩展和长期存储，是 Prometheus 生态中最流行的扩展方案之一。

## 核心能力

- **全局查询**: Querier 聚合多个 Prometheus/Sidecar 的数据，提供跨集群全局视图
- **无限存储**: 通过对象存储（S3/GCS/Azure）实现无限数据保留
- **降采样**: 自动对历史数据进行降采样，减少存储和查询开销
- **高可用**: 多副本 Prometheus 去重，避免单点故障
- **Sidecar 模式**: 无需修改 Prometheus，通过 Sidecar 无缝集成
- **兼容 PromQL**: 完全兼容 Prometheus 查询语言

## 架构

Thanos 由多个组件构成：

- **Sidecar**: 与 Prometheus 并行运行，上传数据块到对象存储，提供 StoreAPI
- **Store Gateway**: 从对象存储读取历史数据，提供 StoreAPI
- **Querier**: 聚合所有 StoreAPI（Sidecar + Store Gateway），提供全局 PromQL 查询
- **Compactor**: 压缩和降采样对象存储中的数据块
- **Ruler**: 评估告警和记录规则，将结果写入对象存储
- **Receive**: 接收 remote_write 数据（替代 Sidecar 模式）

数据流：`Prometheus + Sidecar → 对象存储；查询 → Querier → Sidecar + Store Gateway`

## K8s 集成

Thanos 通过 Helm Chart 或 Thanos Operator 部署在 Kubernetes 中。Sidecar 以容器形式添加到 Prometheus StatefulSet Pod 中。对象存储凭据通过 Kubernetes Secret 管理。Querier、Store Gateway、Compactor 以独立 Deployment 运行。与 Prometheus Operator 深度集成，支持 ServiceMonitor 和 ThanosRuler CRD。

## 生产场景

1. **多集群监控联邦**: 聚合多个 K8s 集群的 Prometheus 数据
2. **长期数据保留**: 将监控数据保留数月/数年用于趋势分析
3. **高可用 Prometheus**: 多副本 Prometheus + Thanos 去重
4. **成本优化**: 降采样减少存储成本，同时保留长期趋势

## 安装与配置

```bash
# Helm 安装 Thanos
helm repo add bitnami https://charts.bitnami.com/bitnami
helm install thanos bitnami/thanos -n thanos --create-namespace \
  --set objstoreConfig.type=S3 \
  --set objstoreConfig.config.bucket=thanos-metrics \
  --set objstoreConfig.config.endpoint=minio.default.svc:9000

# 配置 Prometheus Sidecar
kubectl apply -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus
spec:
  thanos:
    image: quay.io/thanos/thanos:v0.36.0
    objectStorageConfig:
      name: thanos-objstore
      key: objstore.yml
  serviceAccountName: prometheus
EOF
```

### 对象存储配置

```yaml
# objstore.yml
apiVersion: v1
kind: Secret
metadata:
  name: thanos-objstore
stringData:
  objstore.yml: |
    type: S3
    config:
      bucket: thanos-metrics
      endpoint: minio.default.svc:9000
      access_key: minioadmin
      secret_key: minioadmin
      insecure: true
```

## 运维操作

```bash
# 🟢 查看 Thanos 组件状态
kubectl get pods -n thanos

# 🟢 检查 Querier Store 连接
kubectl exec -n thanos deploy/thanos-querier -- \
  wget -qO- "http://localhost:9090/api/v1/stores"

# 🟢 查看对象存储数据块
kubectl exec -n thanos deploy/thanos-store-gateway -- \
  wget -qO- "http://localhost:9090/api/v1/blocks"

# 🟢 执行全局查询
kubectl exec -n thanos deploy/thanos-querier -- \
  wget -qO- "http://localhost:9090/api/v1/query?query=up"

# 🟡 触发压缩
kubectl exec -n thanos deploy/thanos-compactor -- \
  wget -qO- "http://localhost:9090/api/v1/compact"

# 🟢 查看降采样状态
kubectl logs -n thanos deploy/thanos-compactor | grep -i downsample
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Sidecar 未上传 | 对象存储不可达 | `kubectl logs prometheus-0 -c thanos-sidecar` | 检查 S3 凭据和网络 |
| 查询无数据 | Store 未连接 | 检查 Querier /stores 端点 | 检查 Sidecar/Store Gateway |
| 数据重复 | 去重未启用 | 检查 Querier 配置 | 启用 replica-label 去重 |
| 查询慢 | 数据块过多 | 检查 Compactor 状态 | 执行压缩和降采样 |
| 存储成本高 | 降采样未配置 | 检查 Compactor 日志 | 配置降采样策略 |

### 排查流程

```
Thanos 异常
├─ 数据未上传？
│  ├─ Sidecar 错误 → 检查对象存储凭据
│  ├─ 网络问题 → 检查 S3/GCS 连通性
│  └─ Prometheus 未运行 → 检查 Prometheus 状态
├─ 查询异常？
│  ├─ 无数据 → 检查 Store 连接
│  ├─ 重复数据 → 配置 replica-label 去重
│  └─ 超时 → 扩展 Querier 或执行压缩
└─ 存储问题？
   ├─ 空间不足 → 配置数据保留策略
   └─ 成本高 → 启用降采样
```

## 生产案例

### 案例 1: 多集群监控联邦

**场景**: 企业 30+ K8s 集群需统一监控视图。

**方案**:
1. 每个集群 Prometheus 添加 Thanos Sidecar
2. 中央部署 Querier + Store Gateway
3. 数据上传到中央 S3 存储
4. Grafana 通过 Querier 提供全局查询

**效果**: 单一入口查询所有集群，数据保留 2 年。

### 案例 2: 高可用 Prometheus

**场景**: 生产环境需要 Prometheus 高可用。

**方案**:
1. 部署 2 副本 Prometheus（相同配置）
2. Thanos Querier 配置 replica-label 去重
3. 任一 Prometheus 故障不影响查询

**效果**: 监控可用性 99.99%，无单点故障。

## 对比与替代方案

| 维度 | Thanos | Cortex | Mimir | VictoriaMetrics |
|------|--------|--------|-------|------------------|
| 集成方式 | Sidecar | remote_write | remote_write | remote_write |
| 多租户 | ❌ | ✅ | ✅ | ⚠️ |
| 部署复杂度 | 中 | 高 | 高 | 低 |
| 对象存储 | ✅ | ✅ | ✅ | ✅ |
| 降采样 | ✅ | ✅ | ✅ | ❌ |
| CNCF 状态 | Incubating | Incubating | 非 CNCF | 非 CNCF |

## 检查清单

- [ ] 对象存储已配置并有足够容量
- [ ] Sidecar 已添加到 Prometheus Pod
- [ ] Querier 已配置 replica-label 去重
- [ ] Store Gateway 已部署并可访问对象存储
- [ ] Compactor 定期运行压缩和降采样
- [ ] Grafana 数据源已指向 Querier
- [ ] 监控告警：组件健康/存储使用率
- [ ] 数据保留策略已配置

## 架构定位

在 CNCF 生态中，Thanos 属于 **Observability** 类别，为 Prometheus 提供高可用、长期存储和全局查询能力。它是 Prometheus 生态中最流行的扩展方案之一。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/observability-pillars.md|observability-pillars]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[submariner]] — Submariner
- [[03-prometheus-ha-deployment]] — [[Prometheus|Prometheus]]us 高可用部署|Prometheus 高可用部署]]
- [[inclavare-containers]] — Inclavare Containers
- [[bank-vaults]] — Bank-Vaults
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 04-thanos-enterprise-metrics-federation
- thanos
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.28
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.31
- [[实体/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[实体/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[概念/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[技能/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->

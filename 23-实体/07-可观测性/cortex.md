---
title: Cortex (entities)
description: '## 概述'
summary: 'Cortex 是多租户、水平可扩展的 Prometheus 即服务解决方案。'
category: entities
tags:
- k8s
- cncf
- observability
- cortex
- prometheus
- grafana
- containerd
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cortex 是什么
- 如何 Cortex
trigger_keywords:
- Cortex
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cortex

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Cortex 是多租户、水平可扩展的 Prometheus 即服务（Prometheus-as-a-Service）解决方案，由 Weaveworks 开发，2018 年加入 CNCF 孵化。它为 Prometheus 提供长期存储、高可用性和全局视图能力，适合大规模 Kubernetes 监控场景。Cortex 的核心价值在于将多个 Prometheus 实例的数据聚合到一个全局存储后端（S3、GCS、Azure Blob 等对象存储），支持多租户隔离和水平扩展。与原生 Prometheus（单机 TSDB）相比，Cortex 通过分布式架构解决了长期数据保留（数年）、高可用查询和跨集群全局聚合的问题。Cortex 的设计理念后来催生了 Mimir（Grafana Labs 的 Cortex 分支）和 Thanos 等项目。

## 核心能力

- **多租户**: 完全隔离的租户数据和查询（通过 X-Scope-OrgID Header）
- **水平扩展**: Distributor、Ingester、Querier、Compactor 等组件可独立水平扩展
- **长期存储**: 支持 S3、GCS、Azure Blob、Aliyun OSS 等对象存储后端
- **高可用**: 数据复制（Replication Factor）和组件自动故障转移
- **兼容 Prometheus**: 完全兼容 PromQL 和 remote_write，现有 Prometheus 无缝接入
- **全局查询**: 聚合多个 Prometheus 实例的数据，提供集群/跨集群全局视图

## 架构

Cortex 采用微服务化架构（也支持单进程模式）：

- **Distributor**: 接收 remote_write 数据，验证并分发到 Ingester
- **Ingester**: 在内存中接收和索引时序数据，定期刷入对象存储
- **Querier**: 处理查询请求，同时从 Ingester 和对象存储获取数据
- **Query Frontend**: 查询前置组件，提供缓存、分片和请求合并
- **Compactor**: 合并和压缩对象存储中的数据块，减少查询开销
- **Store Gateway**: 从对象存储读取历史数据的网关
- **Ruler/Alertmanager (可选)**: 内置告警规则评估和通知

数据流：`Prometheus (remote_write) → Distributor → Ingester → 对象存储；查询 → Querier → Ingester + Store Gateway`

## K8s 集成

Cortex 通过 Helm Chart 或 Jsonnet 以微服务模式部署在 Kubernetes 集群中。各组件（Distributor、Ingester、Querier 等）以独立 Deployment 运行，通过 Kubernetes Service 互相发现。Prometheus 实例配置 `remote_write` 将指标推送到 Cortex。对象存储凭据通过 Kubernetes Secret 管理。生产环境推荐部署多副本 Ingester 和 Distributor 实现高可用。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Deployment/Service 和 Prometheus Operator 生态集成。

## 生产场景

1. **大规模多集群监控**: 聚合数百个集群的 Prometheus 数据到统一后端
2. **长期数据保留**: 将监控数据保留数月/数年用于趋势分析和容量规划
3. **多租户 SaaS 监控**: 为不同租户提供隔离的监控数据存储和查询
4. **高可用 Prometheus**: 通过 Cortex 提供高可用的查询入口，避免单点 Prometheus 故障

## 安装

```bash
# Helm 安装 Cortex
helm repo add cortex https://cortexproject.github.io/cortex-helm-chart
helm install cortex cortex/cortex -n cortex --create-namespace \
  --set config.storage.backend=s3 \
  --set config.storage.s3.endpoint=minio.default.svc:9000 \
  --set config.storage.s3.bucket_name=cortex-metrics

# 配置 Prometheus remote_write 到 Cortex
kubectl apply -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus
spec:
  remoteWrite:
  - url: http://cortex-distributor.cortex.svc:8080/api/v1/push
    headers:
      X-Scope-OrgID: tenant-1
  serviceAccountName: prometheus
EOF

# 查询 Cortex（带租户 ID）
kubectl exec -it deployment/cortex-querier -n cortex -- \
  wget -qO- --header="X-Scope-OrgID: tenant-1" \
  "http://localhost:9009/api/v1/query?query=up"

# 配置 Grafana 数据源指向 Cortex
# URL: http://cortex-query-frontend.cortex.svc:8080
# Header: X-Scope-OrgID: tenant-1
```

## 运维操作

```bash
# 🟢 查看 Cortex 组件状态
kubectl get pods -n cortex

# 🟢 检查 Ingester 状态
kubectl exec -n cortex deploy/cortex-ingester -- curl -s localhost:8080/ingester/ring

# 🟢 查看租户数据
kubectl exec -n cortex deploy/cortex-querier -- \
  wget -qO- --header="X-Scope-OrgID: tenant-1" \
  "http://localhost:9009/api/v1/label/__name__/values"

# 🟢 检查对象存储连接
kubectl exec -n cortex deploy/cortex-compactor -- \
  curl -s localhost:8080/compactor/ring

# 🟡 强制压缩
kubectl exec -n cortex deploy/cortex-compactor -- \
  curl -X POST localhost:8080/compactor/force

# 🟢 查看查询性能
kubectl logs -n cortex deploy/cortex-query-frontend | grep -i "slow query"
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| remote_write 失败 | Distributor 不可用 | `kubectl logs deploy/cortex-distributor` | 检查 Distributor 副本数 |
| 查询超时 | Ingester 压力大 | `kubectl top pod -n cortex` | 扩展 Ingester 副本 |
| 数据丢失 | 对象存储故障 | 检查 S3/GCS 连接 | 检查存储凭据和网络 |
| 租户隔离失效 | Header 未设置 | 检查请求 Header | 确保 X-Scope-OrgID 存在 |
| 内存溢出 | Ingester 缓存过大 | `kubectl describe pod` | 调整内存 limits |

### 排查流程

```
Cortex 异常
├─ 数据写入失败？
│  ├─ Distributor 错误 → 检查 remote_write URL
│  ├─ Ingester 拒绝 → 检查 Ingester 状态和磁盘
│  └─ 对象存储错误 → 检查 S3/GCS 凭据
├─ 查询异常？
│  ├─ 无数据 → 检查租户 ID 和时间范围
│  ├─ 超时 → 扩展 Querier/Query Frontend
│  └─ 错误结果 → 检查 Compactor 状态
└─ 组件崩溃？
   ├─ OOM → 增加内存 limits
   └─ 存储连接失败 → 检查网络和凭据
```

## 生产案例

### 案例 1: 多集群监控聚合

**场景**: 企业 50+ K8s 集群需统一监控视图。

**方案**:
1. 每个集群 Prometheus 配置 remote_write 到中央 Cortex
2. 按集群/环境划分租户
3. Grafana 通过 Cortex 提供全局查询

**效果**: 单一入口查询所有集群指标，数据保留 1 年。

### 案例 2: SaaS 多租户监控

**场景**: SaaS 平台需为每个客户提供隔离的监控数据。

**方案**:
1. 每个客户分配独立的 X-Scope-OrgID
2. Cortex 多租户隔离保证数据安全
3. 客户通过 Grafana 查询自己的数据

**效果**: 完全租户隔离，无数据泄露风险。

## 对比与替代方案

| 维度 | Cortex | Mimir | Thanos | VictoriaMetrics |
|------|--------|-------|--------|------------------|
| 多租户 | ✅ | ✅ | ❌ | ⚠️ |
| 水平扩展 | ✅ 微服务 | ✅ 微服务 | ⚠️ Sidecar | 单节点/集群 |
| 对象存储 | ✅ | ✅ | ✅ | ✅ |
| 部署复杂度 | 高 | 高 | 中 | 低 |
| CNCF 状态 | Incubating | 非 CNCF | Incubating | 非 CNCF |

## 检查清单

- [ ] 对象存储已配置并有足够容量
- [ ] Ingester 多副本部署（至少 3）
- [ ] 租户隔离已配置（X-Scope-OrgID）
- [ ] remote_write 已配置并测试
- [ ] Grafana 数据源已指向 Cortex
- [ ] 监控告警：组件健康/存储使用率
- [ ] Compactor 定期运行压缩数据
- [ ] 查询性能已优化（缓存/分片）

## 对比

| 特性 | Cortex | Mimir | Thanos | VictoriaMetrics |
|------|--------|-------|--------|-----------------|
| 多租户 | ✅ | ✅ | ❌ | ⚠️ |
| 水平扩展 | ✅ 微服务 | ✅ 微服务 | ⚠️ Sidecar | ✡ 单节点/集群 |
| 对象存储 | ✅ | ✅ | ✅ | ✅ |
| CNCF 状态 | Incubating | 非 CNCF | Incubating | 非 CNCF |

## 架构定位

在 CNCF 生态中，Cortex 属于 **Observability** 类别，为云原生应用提供多租户、水平可扩展的 Prometheus 长期存储能力。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[keylime]] — Keylime
- [[openebs]] — OpenEBS
- [[04-containerd-windows-support]] — containerd Windows 支持
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cortex
- [[23-实体/15-参考与索引/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

---
title: Prometheus 高可用部署 (entities)
description: '# Prometheus 高可用部署'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- 03-prometheus-ha-deployment
- prometheus
- grafana
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
- Prometheus 高可用部署 是什么
- 如何 Prometheus 高可用部署
trigger_keywords:
- Prometheus
- 高可用部署
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Prometheus 高可用部署

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Prometheus 高可用部署是关于在生产环境中部署和运维高可用 Prometheus 监控系统的最佳实践。由于 Prometheus 本身不提供原生的集群和高可用能力，需要通过双实例冗余、Thanos/Cortex/Mimir 联邦、AlertManager 集群等组合方案实现监控系统的高可用性。该实践涵盖 Prometheus 实例冗余、长期存储、查询联邦、告警高可用等多个维度。

## Key Features（核心能力）

- **双实例冗余**：部署两个相同配置的 Prometheus 实例独立采集和告警
- **Thanos/Cortex 集成**：通过 Thanos Sidecar 或 Cortex 实现长期存储和全局查询
- **AlertManager 集群**：部署多节点 AlertManager 集群实现告警高可用
- **远程写入**：通过 Remote Write 将指标实时转发到中央存储
- **分区采集**：通过分片（Sharding）将采集目标分配到多个 Prometheus 实例
- **自动故障转移**：通过 Prometheus Operator 实现实例自动恢复

## 架构与工作原理

高可用架构有三种模式：1) 简单冗余模式——两个 Prometheus 独立运行，AlertManager 去重告警；2) Thanos 联邦模式——Thanos Sidecar 上传 Block 到对象存储，Thanos Query 提供全局查询；3) Cortex/Mimir 模式——多副本写入、分布式存储，提供真正的水平扩展。AlertManager 集群通过 Gossip 协议同步告警状态，避免重复通知。

## K8s 集成

通过 Prometheus Operator 部署和管理 Prometheus 实例，StatefulSet 保证实例标识。Thanos 通过 Sidecar 模式注入到 Prometheus Pod，Query Store Gateway 通过 Deployment 部署。使用 K8s PodDisruptionBudget 保证最小可用实例数。Cortex/Mimir 通过微服务模式部署，利用 Ingester、Querier、Compactor 等组件分散负载。

## 生产用例

- **大规模集群监控**：数千节点集群的指标采集和存储
- **长期指标存储**：超过 Prometheus 默认 15 天的长期数据保留需求
- **多集群监控联邦**：统一查询多个集群的监控指标
- **高可用告警**：确保告警系统不因单点故障中断

## 安装与配置

### Prometheus Operator + Thanos 部署

```bash
# 🟢 安装 kube-prometheus-stack（含 Thanos Sidecar）
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update
helm install kube-prometheus prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  --set prometheus.thanos.create=true \
  --set prometheus.prometheusSpec.replicas=2 \
  --set prometheus.prometheusSpec.retention=6h \
  --set prometheus.prometheusSpec.retentionSize=50GB \
  --set alertmanager.alertmanagerSpec.replicas=3

# 🟢 安装 Thanos Query + Store Gateway
helm install thanos prometheus-community/thanos \
  -n monitoring \
  --set query.enabled=true \
  --set storeGateway.enabled=true \
  --set compactor.enabled=true \
  --set bucket.type=S3 \
  --set bucket.config.bucket=thanos-metrics \
  --set bucket.config.endpoint=s3.internal:9000

# 🟢 验证部署
kubectl get pods -n monitoring
kubectl get prometheus -n monitoring
```

### Prometheus CRD 配置（双副本 + Thanos）

```yaml
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: k8s-ha
  namespace: monitoring
spec:
  replicas: 2  # 双实例冗余
  replicaExternalLabelName: prometheus_replica
  externalLabels:
    cluster: prod-east
    environment: production
  retention: 6h  # 本地短期存储，长期由 Thanos 管理
  retentionSize: 50GB
  thanos:
    image: quay.io/thanos/thanos:v0.35.0
    objectStorageConfig:
      name: thanos-objstore-secret
      key: objstore.yml
    version: v0.35.0
  resources:
    requests:
      cpu: "2"
      memory: 8Gi
    limits:
      cpu: "4"
      memory: 16Gi
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
  serviceMonitorSelector:
    matchLabels:
      team: platform
  podDisruptionBudget:
    minAvailable: 1
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app.kubernetes.io/name: prometheus
        topologyKey: kubernetes.io/hostname
```

### AlertManager 集群配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: Alertmanager
metadata:
  name: ha-cluster
  namespace: monitoring
spec:
  replicas: 3  # Gossip 集群
  configSecret: alertmanager-config
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
  storage:
    volumeClaimTemplate:
      spec:
        resources:
          requests:
            storage: 10Gi
---
# AlertManager 路由配置
apiVersion: v1
kind: Secret
metadata:
  name: alertmanager-config
  namespace: monitoring
stringData:
  alertmanager.yaml: |
    global:
      resolve_timeout: 5m
    route:
      group_by: ['alertname', 'namespace']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      receiver: default
      routes:
      - match:
          severity: critical
        receiver: pagerduty-critical
        repeat_interval: 1h
    receivers:
    - name: default
      webhook_configs:
      - url: http://alertmanager-webhook:8080/alert
    - name: pagerduty-critical
      pagerduty_configs:
      - service_key: <pd-key>
```

### Thanos 对象存储配置

```yaml
# thanos-objstore-secret
apiVersion: v1
kind: Secret
metadata:
  name: thanos-objstore-secret
  namespace: monitoring
stringData:
  objstore.yml: |
    type: S3
    config:
      bucket: thanos-metrics
      endpoint: minio.storage.svc:9000
      access_key: ${MINIO_ACCESS_KEY}
      secret_key: ${MINIO_SECRET_KEY}
      insecure: true
```

## 运维操作

```bash
# 🟢 检查 Prometheus 实例状态
kubectl get prometheus -n monitoring
kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus

# 🟢 检查 Thanos 组件
kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos-query
kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos-store-gateway

# 🟢 检查 AlertManager 集群状态
curl -s http://alertmanager:9093/api/v2/status | jq '.cluster'

# 🟢 查询指标（通过 Thanos Query 全局视图）
curl -s 'http://thanos-query:9090/api/v1/query?query=up'

# 🟡 强制刷新 Prometheus 配置
kubectl rollout restart statefulset/prometheus-k8s-ha -n monitoring

# 🟢 检查存储使用情况
kubectl exec -n monitoring prometheus-k8s-ha-0 -- du -sh /prometheus/
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 指标查询无数据 | Target 发现失败 | `curl :9090/api/v1/targets` | 检查 ServiceMonitor/label |
| Thanos Store 不可用 | 对象存储连接失败 | `kubectl logs thanos-store-*` | 检查 S3 凭据/网络 |
| 告警重复发送 | AlertManager 集群分裂 | `curl :9093/api/v2/status` | 检查 Gossip 端口 9094 |
| Prometheus OOM | 高基数指标/内存不足 | `kubectl top pod`; `tsdb status` | 调整 retentionSize/增加内存 |
| 指标延迟 > 5min | 采集负载过高 | `prometheus_tsdb_head_samples_appended_total` | 分片采集/减少 target |

### 排查流程

```
监控数据异常
├── 指标缺失？
│   ├── 检查 Target: curl :9090/api/v1/targets
│   ├── 检查 ServiceMonitor: kubectl get servicemonitor -A
│   └── 检查 RBAC: kubectl auth can-i list pods --as=system:serviceaccount:monitoring:prometheus-k8s
├── Thanos 查询无历史数据？
│   ├── 检查 Sidecar: kubectl logs prometheus-*-thanos-sidecar
│   ├── 检查对象存储: mc ls minio/thanos-metrics/
│   └── 检查 Store Gateway: kubectl logs thanos-store-*
└── 告警异常？
    ├── 告警未触发 → 检查 AlertRule/PrometheusRule
    ├── 告警重复 → 检查 AlertManager 集群状态
    └── 告警丢失 → 检查 AlertManager 副本数/PDB
```

## 生产案例

### 案例1：Prometheus 单实例宕机导致监控盲区

- **场景**：Prometheus Pod 因节点故障宕机，30分钟内无监控数据和告警
- **排查**：单实例部署无冗余；PVC 在故障节点上无法重新挂载
- **方案**：部署双副本 Prometheus + Thanos；AlertManager 3 节点集群；PDB 保证最小可用
- **效果**：单实例故障时另一个继续采集和告警，无监控盲区

### 案例2：高基数指标导致 Prometheus OOM

- **场景**：新上线服务暴露了带 user_id label 的指标，基数爆炸导致 OOM
- **排查**：`prometheus_tsdb_head_series` 从 50万飙升到 5000万；`tsdb status` 显示 top series
- **方案**：通过 metric_relabel_configs 丢弃高基数 label；设置 `--storage.tsdb.max-block-duration=2h`；增加内存限制
- **效果**：series 回落到 100万，内存稳定在 8GB 以内

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| 双副本 + Thanos | 简单可靠、长期存储、全局查询 | 写入重复、Thanos组件运维 | 中大规模集群 |
| Cortex/Mimir | 真正水平扩展、多租户 | 架构复杂、组件多 | 超大规模/多租户 |
| VictoriaMetrics | 高性能、单二进制部署简单 | 生态较新、社区较小 | 追求性能/简单部署 |
| Datadog/New Relic | 全托管、零运维 | 成本高、数据出境 | 小团队/合规允许 |

## 检查清单

- [ ] Prometheus 副本数 >= 2，配置 PodAntiAffinity
- [ ] Thanos Sidecar/Query/StoreGateway/Compactor 已部署
- [ ] 对象存储已配置且可访问
- [ ] AlertManager 副本数 >= 3，Gossip 端口互通
- [ ] PodDisruptionBudget 已配置
- [ ] 指标保留策略已定义（本地 + 远程）
- [ ] 高基数指标防护已配置（relabel/drop）
- [ ] 监控自身的监控（meta-monitoring）已配置

## Related

- [[atlantis]] — Atlantis
- [[submariner]] — Submariner
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-prometheus-ha-deployment
- [[23-实体/cncf-observability.md|[[23-实体/15-参考与索引/cncf-observability|CNCF 可观测性项目全景]]]] — Cross-reference


<!-- risk-assessed -->

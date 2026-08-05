---
title: VictoriaMetrics 集群版 on Kubernetes
description: 在阿里云 ACK / 专有云 ASO 环境中部署 VictoriaMetrics 集群版，覆盖架构设计、存储规划、扩缩容、监控告警与故障排查
summary: 在阿里云 ACK / 专有云 ASO 环境中部署 VictoriaMetrics 集群版，覆盖架构设计、存储规划、扩缩容、监控告警与故障排查
category: domain
tags:
- victoriametrics
- prometheus
- tsdb
- kubernetes
- ack
- aso
- observability
- storage
- scaling
- monitoring
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 可观测性架构师
estimated_read_time: 18min
intent_queries:
- VictoriaMetrics 集群版 on Kubernetes 是什么
- 如何在 ACK 上部署 VictoriaMetrics 集群版
- VictoriaMetrics 与 Prometheus 在生产环境如何选型
trigger_keywords:
- VictoriaMetrics
- vmcluster
- vminsert
- vmselect
- vmstorage
- 时序数据库
- Prometheus 替代
prerequisites:
- kubectl-basics
- prometheus-basics
- pvc-basics
- ack-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# VictoriaMetrics 集群版 on Kubernetes

## 目录

- [1. 架构与组件](#1-架构与组件)
- [2. 与 Prometheus 的对比选型](#2-与-prometheus-的对比选型)
- [3. 在 ACK 上部署集群版](#3-在-ack-上部署集群版)
- [4. 阿里云 ACK 特有配置](#4-阿里云-ack-特有配置)
- [5. 存储规划与性能调优](#5-存储规划与性能调优)
- [6. 扩缩容策略](#6-扩缩容策略)
- [7. 高可用与多可用区](#7-高可用与多可用区)
- [8. 监控告警与可视化](#8-监控告警与可视化)
- [9. 日志收集与成本优化](#9-日志收集与成本优化)
- [10. 常见故障排查](#10-常见故障排查)
- [11. 生产检查清单](#11-生产检查清单)
- [12. 阿里云 OSS 远程存储与成本控制](#12-阿里云-oss-远程存储与成本控制)
- [13. 相关文档](#13-相关文档)
## 1. 架构与组件

VictoriaMetrics 集群版通过水平拆分写入、查询与存储，解决单机 Prometheus 在大规模指标场景下的扩展瓶颈。核心组件如下：

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  vminsert   │────→│  vmstorage  │←────│  vmselect   │
│  (写入路由)  │     │  (存储节点)  │     │  (查询聚合)  │
└─────────────┘     └─────────────┘     └─────────────┘
       ↑                                     │
       └────────  vmagent / Prometheus ──────┘
```

| 组件 | 职责 | 关键参数 |
|------|------|---------|
| `vminsert` | 接收 remote write 请求，按 metric name + label 一致性哈希分发到 vmstorage | `-replicationFactor`, `-storageNode` |
| `vmstorage` | 持久化时序数据，执行压缩与保留策略 | `-retentionPeriod`, `-storageDataPath` |
| `vmselect` | 聚合多个 vmstorage 的查询结果，支持 PromQL | `-storageNode`, `-dedup.minScrapeInterval` |
| `vmagent` | 轻量级采集代理，替代 Prometheus server 抓取 exporter | `-promscrape.config`, `-remoteWrite.url` |

vminsert 与 vmselect 都是无状态的，可以任意水平扩展；vmstorage 是有状态节点，扩容时需要特别小心一致性哈希环的重新平衡。在阿里云 ACK 环境中，建议将 vminsert 与 vmselect 部署为 Deployment，vmstorage 部署为 StatefulSet，以便分别利用无状态弹性与有状态持久化能力。vmagent 作为采集代理，可以 DaemonSet 或 Sidecar 方式部署到业务集群，将指标 remote write 到中心 VictoriaMetrics 集群，实现联邦采集架构。

## 2. 与 Prometheus 的对比选型

在阿里云 ACK 或专有云 ASO 中，当集群规模超过 500 节点或时间序列超过 500 万条时，VictoriaMetrics 集群版通常是更优选择。

| 维度 | Prometheus | VictoriaMetrics 集群版 |
|------|------------|------------------------|
| 扩展方式 | 垂直扩展 + federation / Thanos | 原生水平扩展 |
| 高基数 | 高基数 label 容易导致 OOM | 对高基数容忍度更高 |
| 查询语言 | PromQL | PromQL 兼容 |
| 存储效率 | 2h block，本地存储 | 高压缩比，对象存储/云盘均可 |
| 多租户 | 不支持原生 | 支持 `-tenantID` 隔离 |
| 部署复杂度 | 低 | 中 |
| 资源占用 | Head block 常驻内存 | 查询时按需加载 |

> 建议：保留 Prometheus agent 模式作为采集端，将 remote write 目标指向 VictoriaMetrics，实现无侵入迁移。对于已有 Prometheus server，可逐步将 remote write 地址切换到 vminsert，避免一次性改造带来的风险。

## 3. 在 ACK 上部署集群版

### 3.1 通过 Helm 安装 vmoperator

vmoperator 是 VictoriaMetrics 官方 Kubernetes Operator，推荐使用 Helm 在 ACK 集群中部署：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 VictoriaMetrics Helm 仓库并更新，确保使用官方最新 chart
helm repo add vm https://victoriametrics.github.io/helm-charts/
helm repo update

# 在可观测性命名空间安装 operator
kubectl create namespace vm
helm install vmoperator vm/victoria-metrics-operator -n vm \
  --set operator.enable_converter_ownership=false
```
### 3.2 创建 VMCluster 自定义资源

VMCluster CR 声明了 vminsert、vmselect、vmstorage 的副本数与资源规格。以下示例适用于日均写入 100 万样本的生产环境：

```yaml
apiVersion: operator.victoriametrics.com/v1beta1
kind: VMCluster
metadata:
  name: prod-vmcluster
  namespace: vm
spec:
  retentionPeriod: "30d"
  replicationFactor: 2
  vmstorage:
    replicaCount: 3
    storageDataPath: /vmstorage-data
    storage:
      volumeClaimTemplate:
        spec:
          storageClassName: alicloud-disk-ssd   # ACK SSD 云盘
          resources:
            requests:
              storage: 500Gi
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "4"
        memory: 16Gi
  vmselect:
    replicaCount: 2
    resources:
      requests:
        cpu: "1"
        memory: 4Gi
      limits:
        cpu: "2"
        memory: 8Gi
  vminsert:
    replicaCount: 2
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
```

创建后通过以下命令确认组件状态：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VMCluster 状态，确保 phase 为 Running
kubectl get vmcluster -n vm prod-vmcluster -o jsonpath='{.status.status}'

# 检查 vminsert、vmselect、vmstorage Pod 是否全部就绪
kubectl get pods -n vm -l app.kubernetes.io/name=vmcluster
```
### 3.3 配置 remote write

将 Prometheus 或 vmagent 的 remote write 指向 vminsert 的 Service：

```yaml
remote_write:
  - url: "http://vminsert-prod-vmcluster.vm.svc.cluster.local:8480/insert/0/prometheus/api/v1/write"
    queue_config:
      max_samples_per_send: 10000
      max_shards: 30
```

其中 `/insert/0/prometheus/api/v1/write` 的 `0` 为 tenantID，多租户场景可按团队或环境拆分。例如生产环境使用 tenantID `prod`，测试环境使用 `test`，实现数据隔离与权限控制。

## 4. 阿里云 ACK 特有配置

### 4.1 使用阿里云 OSS 作为长期冷存

VictoriaMetrics 的本地存储成本随 retentionPeriod 线性增长。对于 30 天以上的历史数据，可配置 vmbackup 将 snapshot 定期上传至阿里云 OSS，降低 ESSD 云盘成本。该策略尤其适用于审计与合规场景，需要保留数月甚至数年的历史指标，但日常查询主要集中在最近 7 到 15 天。

```bash
# 使用 vmbackup 将 vmstorage 数据备份到 OSS
vmbackup \
  -storageDataPath=/vmstorage-data \
  -snapshot.createURL=http://localhost:8428/snapshot/create \
  -dst=oss://my-vmbackup-bucket/vmcluster/prod/ \
  -credsFilePath=/etc/secrets/oss-credentials
```

### 4.2 网络与 DNS 优化

在 ACK 专有网络环境中，vminsert 与 vmstorage 之间建议通过 Headless Service 直接通信，避免 kube-proxy 带来的额外跳数：

```yaml
# 确保 vmstorage 使用 StatefulSet + Headless Service
apiVersion: v1
kind: Service
metadata:
  name: vmstorage-prod-vmcluster
  namespace: vm
spec:
  clusterIP: None
  selector:
    app.kubernetes.io/name: vmstorage
  ports:
    - port: 8401
      name: vmselect
    - port: 8400
      name: vminsert
```

## 5. 存储规划与性能调优

### 5.1 磁盘选型

vmstorage 对磁盘延迟与吞吐量均敏感，磁盘选型直接影响写入吞吐与查询性能。在阿里云 ACK 中，应根据写入负载与查询并发度选择合适的 StorageClass。

| 磁盘类型 | 适用场景 | 备注 |
|---------|---------|------|
| `alicloud-disk-ssd` | 生产默认 | IOPS 与吞吐量均衡，支持在线扩容 |
| `alicloud-disk-essd` | 高写入吞吐 | 适合 vmstorage 写入密集型场景 |
| `alicloud-disk-efficiency` | 测试环境 | 成本低，但延迟较高 |

### 5.2 保留策略与压缩

通过 retentionPeriod 控制数据保留时长，建议结合成本与合规要求设置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前 vmstorage 数据目录大小，评估保留周期是否需要调整
kubectl exec -n vm vmstorage-prod-vmcluster-0 -- du -sh /vmstorage-data
```
### 5.3 vminsert 写入并发调优

当采集端数量超过 5000 时，需调整 vminsert 的并发参数：

```yaml
spec:
  vminsert:
    extraArgs:
      maxConcurrentInserts: "64"
      maxQueueDuration: "1m"
```

### 5.4 vmstorage 内存限制

vmstorage 默认会占用节点大部分内存进行缓存。在 ACK 节点上运行多个 workload 时，应显式设置 `-memory.allowedPercent`：

```yaml
spec:
  vmstorage:
    extraArgs:
      memory.allowedPercent: "60"
```

## 6. 扩缩容策略

### 6.1 水平扩容 vmstorage

当 vmstorage 节点磁盘使用率超过 70% 或查询延迟 P99 超过 2s 时，应扩容 vmstorage：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 编辑 VMCluster，将 vmstorage.replicaCount 从 3 改为 5
kubectl patch vmcluster prod-vmcluster -n vm --type merge \
  -p '{"spec":{"vmstorage":{"replicaCount":5}}}'
```
扩容后 vminsert 会自动感知新节点，但建议重启 vminsert 以刷新一致性哈希环，避免数据分布不均：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 滚动重启 vminsert，重新加载 storage node 列表
kubectl rollout restart deployment vminsert-prod-vmcluster -n vm
```
### 6.2 垂直扩容 vmselect

查询高峰期 CPU 利用率持续高于 80% 时，优先增加 vmselect 副本数：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 临时扩容 vmselect 应对查询高峰
kubectl scale deployment vmselect-prod-vmcluster -n vm --replicas=4
```
## 7. 高可用与多可用区

在 ACK 多可用区集群中，应通过 Pod 反亲和性将 vmstorage 副本分散到不同可用区。多可用区部署虽然能提升可用性，但也会引入跨可用区网络延迟，可能影响 vminsert 到 vmstorage 的写入延迟以及 vmselect 的查询聚合效率。因此，在延迟敏感场景下，可以选择将 vmstorage 集中在同一可用区，同时通过 replicationFactor 与跨可用区备份策略保证数据安全。

```yaml
spec:
  vmstorage:
    affinity:
      podAntiAffinity:
        preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app.kubernetes.io/name: vmstorage
              topologyKey: topology.kubernetes.io/zone
```

同时建议 replicationFactor 设置为 2 或 3，确保单个可用区故障时数据仍可查询。

## 8. 监控告警与可视化

### 8.1 暴露自身指标

VictoriaMetrics 组件默认在 `http://<pod>:8428/metrics` 暴露自身指标，可通过 ServiceMonitor 被 Prometheus / vmagent 采集：

```yaml
apiVersion: operator.victoriametrics.com/v1beta1
kind: VMServiceScrape
metadata:
  name: vmcluster-self-monitor
  namespace: vm
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: vmcluster
  endpoints:
    - port: http
      path: /metrics
```

### 8.2 关键告警规则

| 告警名 | 表达式 | 含义 |
|--------|--------|------|
| VMStorageDiskHigh | `vm_free_disk_space_bytes / vm_total_disk_space_bytes < 0.2` | 磁盘空间不足 |
| VMInsertSlow | `histogram_quantile(0.99, rate(vm_http_request_duration_seconds_bucket[5m])) > 2` | 写入延迟高 |
| VMSelectErrors | `rate(vm_http_request_errors_total[5m]) > 0.05` | 查询错误率高 |
| VMStorageOOMRisk | `container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.85` | vmstorage 内存接近上限 |

## 9. 日志收集与成本优化

### 9.1 日志收集

建议将 vminsert、vmselect、vmstorage 的日志统一采集到阿里云 SLS 或自研日志平台，便于排查慢查询与写入失败：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 vmstorage 最近错误日志
kubectl logs -n vm statefulset/vmstorage-prod-vmcluster --tail=200 | grep -iE "error|warn|oom"
```
### 9.2 成本优化

| 优化手段 | 效果 | 风险 |
|---------|------|------|
| 缩短 retentionPeriod | 降低存储成本 | 历史查询受限 |
| 使用 recording rules | 减少原始样本保留 | 需要预定义查询 |
| 冷热分层存储 | 历史数据转 OSS | 查询延迟增加 |
| 限制高基数 label | 降低存储与内存占用 | 需要改造采集端 |

## 10. 常见故障排查

### 10.1 vmstorage OOM

vmstorage 在数据合并（merge）时会消耗大量内存。若频繁 OOM，可先检查内存使用趋势：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 vmstorage Pod 的内存使用与重启次数
kubectl top pod -n vm -l app.kubernetes.io/component=vmstorage
kubectl get pods -n vm -l app.kubernetes.io/component=vmstorage
```
常见缓解措施：

1. 增加 `-memory.allowedPercent` 限制，避免占用全部节点内存。
2. 提升 vmstorage 内存 limit。
3. 缩短 retentionPeriod 减少数据量。
4. 增加 vmstorage 副本数，分散单节点数据量。

### 10.2 查询返回慢或超时

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 vmselect 日志，定位慢查询来源
kubectl logs -n vm deployment/vmselect-prod-vmcluster --tail=200 | grep -i "slow"
```
优先排查：

- 查询时间窗口是否过大（> 7d）。
- 是否缺少 recording rules 预聚合。
- vmselect 与 vmstorage 之间网络延迟是否过高。
- 是否存在高基数指标拉低查询性能。

### 10.3 vminsert 返回 4xx/5xx

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 vminsert 实时错误统计
kubectl logs -n vm deployment/vminsert-prod-vmcluster --tail=500 | grep -i error
```
常见原因包括：tenantID 格式错误、标签值超长、样本乱序（out-of-order）。

## 11. 生产检查清单

- [ ] VMCluster 的 `replicationFactor` 至少为 2，避免单点数据丢失。
- [ ] vmstorage 使用 SSD 或 ESSD 云盘，避免使用 efficiency 盘。
- [ ] 已配置 retentionPeriod 并评估长期存储成本。
- [ ] vminsert、vmselect、vmstorage 的资源 limit ≥ request 的 2 倍。
- [ ] 已配置自身指标采集与关键告警规则。
- [ ] 已制定 vmstorage 扩容 SOP 并验证过重启 vminsert 刷新哈希环。
- [ ] remote write URL 中的 tenantID 已按团队/环境规划。
- [ ] 已测试跨可用区部署对查询延迟的影响。
- [ ] 已配置日志收集与 OSS 冷备策略。
- [ ] 已对高基数 label 进行治理，避免内存爆炸。

## 12. 阿里云 OSS 远程存储与成本控制

在阿里云环境中，VictoriaMetrics 的 Checkpoint（此处指标快照/备份概念）与长期归档可结合 OSS 实现低成本存储。

### OSS 作为远程备份目标

虽然 VictoriaMetrics 本身不依赖对象存储运行，但可将关键配置、快照与历史数据归档到 OSS：

```bash
# 创建 vmstorage 快照
curl -s http://vmstorage-0:8482/snapshot/create | jq .

# 将快照目录上传到 OSS 归档
ossutil cp -r /vm-data/snapshots/20240629 oss://victoria-backups/snapshots/
```

> 快照上传前应停止写入或利用 vmstorage 的在线快照能力，确保数据一致性。

### 成本控制策略

| 策略 | 说明 |
|---|---|
| 降低保留周期 | 将热存储保留 15-30 天，历史数据归档 OSS |
| 降采样 | 对超过 7 天的指标使用 recording rule 聚合 |
| 标签控制 | 限制高基数 label，减少 active series |
| 冷热分离 | 热数据用 SSD，温冷数据用高效云盘 |

### 典型工单诊断

1. 查询变慢：检查 vmselect 资源、高基数指标与查询时间范围。
2. 写入失败：检查 vminsert 与 vmstorage 网络连通性、磁盘空间。
3. 数据丢失：确认 retentionPeriod 与备份策略。

## 13. 相关文档

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-16-database-middleware/05-time-series-db/01-prometheus-tsdb-deep-dive|Prometheus TSDB 深度解析]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-16-database-middleware/05-time-series-db/02-influxdb-vs-timescaledb|InfluxDB 与 TimescaleDB 对比]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/01-monitoring-metrics-system|监控指标体系]]

```

<!-- risk-assessed -->

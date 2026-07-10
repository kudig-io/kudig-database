---
title: RocketMQ on Kubernetes 生产指南
description: 在阿里云专有云 ACK 集群中部署与运维 Apache RocketMQ：NameServer/Broker 架构、持久化存储、扩缩容、监控告警与常见问题排查。
summary: 在阿里云专有云 ACK 集群中部署与运维 Apache RocketMQ：NameServer/Broker 架构、持久化存储、扩缩容、监控告警与常见问题排查。
category: database-middleware
tags:
- rocketmq
- message-queue
- name-server
- broker
- statefulset
- storage
- kubernetes
- alibaba-cloud
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
reading_level: advanced
audience:
- 中间件工程师
- SRE
- 专有云运维
estimated_read_time: 18min
intent_queries:
- RocketMQ Kubernetes 部署
- RocketMQ NameServer Broker 扩缩容
- RocketMQ on K8s 监控告警
trigger_keywords:
- rocketmq
- name-server
- broker
- controller
- message-queue
- aliyun mq
prerequisites:
- 数据库中间件/03-message-queues/03-message-queue-comparison.md
- 数据库中间件/01-databases/08-kafka-kubernetes-strimzi.md
- 存储/01-k8s-storage/README.md
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




# RocketMQ on Kubernetes 生产指南

> 适用场景：在阿里云专有云或公有云 ACK 集群中自托管 Apache RocketMQ，满足金融、电商、物联网等高吞吐、低延迟消息场景。

## 目录

- [1. 架构概览](#1-架构概览)
- [2. 部署方式选择](#2-部署方式选择)
- [3. 使用 Helm 部署 RocketMQ 集群](#3-使用-helm-部署-rocketmq-集群)
- [4. 存储规划](#4-存储规划)
- [5. NameServer 与 Broker 扩缩容](#5-nameserver-与-broker-扩缩容)
- [6. 监控与告警](#6-监控与告警)
- [7. 常见问题排查](#7-常见问题排查)
- [8. 生产检查清单](#8-生产检查清单)
- [9. 典型工单诊断决策树](#9-典型工单诊断决策树)
- [10. 相关文档](#10-相关文档)
## 1. 架构概览

Apache RocketMQ 的核心组件包括：

- **NameServer**：轻量级路由注册中心，Broker 启动后向所有 NameServer 注册 Topic 路由信息。Producer 和 Consumer 从 NameServer 拉取路由。
- **Broker**：消息存储与转发节点。支持 Master-Slave 架构（异步/同步复制）以及 4.5+ 版本的 Controller 模式（DLedger）。
- **Controller（可选）**：在 4.5+ 中提供自动主备切换能力，避免手动切换带来的 RTO。
- **Dashboard / Exporter**：可视化运维与 Prometheus 指标暴露。

在 Kubernetes 中，NameServer 通常以 StatefulSet + Headless Service 部署；Broker 以 StatefulSet 部署，每个 Pod 挂载独立持久卷保存 commitlog 与 consumequeue。

## 2. 部署方式选择

| 部署方式 | 适用场景 | 特点 |
| --- | --- | --- |
| Helm Chart | 中小规模、快速上线 | 配置灵活，社区 chart 较多 |
| RocketMQ Operator | 大规模、多集群、自动化运维 | 自动扩缩容、故障恢复、配置管理 |
| 阿里云 MQ（ONS） | 不愿自托管、需要 SLA | 全托管，按量付费，但非 K8s 内自管 |

在专有云 ASO 中，如果客户要求数据不出集群，推荐 Helm 或 Operator 自托管；若接受云服务，可直接使用阿里云消息队列 RocketMQ 版。

## 3. 使用 Helm 部署 RocketMQ 集群

### 3.1 添加社区 Chart

以下命令使用 bitnami/apache-rocketmq chart（或社区 chart）在 `middleware` 命名空间部署一个最小可用集群。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 添加社区 Helm 仓库并拉取默认 values 文件
helm repo add rocketmq-repo https://charts.apacherocketmq.ai/
helm repo update
helm show values rocketmq-repo/rocketmq-cluster > values.yaml
```
### 3.2 最小生产 values 示例

```yaml
# values.yaml 片段
nameServer:
  replicaCount: 3
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
  persistence:
    enabled: true
    storageClass: alicloud-disk-ssd
    size: 10Gi

broker:
  replicaCount: 2
  size:
    master: 2
    replica: 1
  persistence:
    enabled: true
    storageClass: alicloud-disk-essd
    size: 100Gi
  config:
    flushDiskType: ASYNC_FLUSH
    brokerRole: ASYNC_MASTER
```

### 3.3 安装并验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 RocketMQ 集群并等待 Pod 就绪
helm install rocketmq rocketmq-repo/rocketmq-cluster -n middleware --create-namespace -f values.yaml
kubectl rollout status statefulset/rocketmq-nameserver -n middleware
kubectl rollout status statefulset/rocketmq-broker-master -n middleware
```
> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 NameServer 与 Broker 是否成功注册
kubectl exec -it rocketmq-nameserver-0 -n middleware -- sh -c \
  "sh mqadmin clusterList -n 'rocketmq-nameserver-0.rocketmq-nameserver.middleware.svc.cluster.local:9876'"
```
## 4. 存储规划

Broker 的 commitlog、consumequeue、index 文件对 IOPS 与延迟敏感，存储选型直接影响吞吐与稳定性。

| 存储类型 | 适用场景 | 注意事项 |
| --- | --- | --- |
| emptyDir | 测试/开发 | 节点故障数据丢失 |
| hostPath | 单节点验证 | 无法跨节点迁移 |
| 阿里云 NAS | 多可用区共享 | 性能中等，适合异步复制 |
| 阿里云 ESSD | 生产首选 | 高 IOPS、低延迟，建议单盘挂载 |
| CPFS | 超大规模 | 需要专有存储网络 |

生产环境建议为每个 Broker Pod 分配独立的 ESSD 云盘，通过 StorageClass 动态供给。

```yaml
# storageclass-essd.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  regionId: cn-hangzhou
  zoneId: cn-hangzhou-b
  diskType: cloud_essd
  provisionedIops: "30000"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

## 5. NameServer 与 Broker 扩缩容

### 5.1 水平扩展 NameServer

NameServer 无状态，增加副本可提升路由查询可用性。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 将 NameServer 副本数从 3 扩到 5
kubectl patch statefulset rocketmq-nameserver -n middleware \
  --type='json' -p='[{"op": "replace", "path": "/spec/replicas", "value": 5}]'
```
扩缩容后，需要更新 Producer/Consumer 的 NameServer 地址列表，建议通过 Kubernetes Service 的 DNS 名（`rocketmq-nameserver.middleware.svc.cluster.local:9876`）或配置中心统一分发。

### 5.2 Broker 扩缩容

Broker 扩缩容涉及数据分片与 Topic 队列重新分配，不能简单修改副本数。推荐流程：

1. 新增 Broker 加入集群；
2. 对新 Topic 使用新的 Broker 写入；
3. 对老 Topic 逐步迁移队列（通过 `mqadmin updateTopic` 调整 write/read queue 数）。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查询当前 Topic 队列分布
kubectl exec -it rocketmq-broker-master-0 -n middleware -- \
  sh mqadmin topicStatus -n rocketmq-nameserver.middleware.svc.cluster.local:9876 -t ORDER_TOPIC
```
### 5.3 Controller 模式自动切换

开启 DLedger Controller 后，当 Master 故障时，可自动从 Slave 中选举新 Master。

```yaml
# values.yaml 片段：启用 controller 模式
broker:
  controller:
    enabled: true
    replicaCount: 3
  config:
    enableDLegerCommitLog: true
    dLegerGroup: broker-a
    dLegerPeers: n0-broker-a-0:40911;n1-broker-a-1:40911;n2-broker-a-2:40911
```

## 6. 监控与告警

### 6.1 部署 rocketmq-exporter

```yaml
# rocketmq-exporter.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rocketmq-exporter
  namespace: middleware
spec:
  replicas: 1
  selector:
    matchLabels:
      app: rocketmq-exporter
  template:
    metadata:
      labels:
        app: rocketmq-exporter
    spec:
      containers:
        - name: exporter
          image: apache/rocketmq-exporter:latest
          args:
            - --rocketmq.config.namesrvAddr=rocketmq-nameserver.middleware.svc.cluster.local:9876
          ports:
            - containerPort: 5557
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 暴露 exporter 服务并创建 ServiceMonitor
kubectl expose deployment rocketmq-exporter --port=5557 -n middleware
```
### 6.2 关键告警指标

| 指标 | 含义 | 告警阈值建议 |
| --- | --- | --- |
| `rocketmq_broker_tps` | Broker 每秒消息数 | 持续低于预期或突增 |
| `rocketmq_broker_qps` | Broker 每秒查询数 | 超过容量 80% |
| `rocketmq_consumer_lag` | 消费积压 | lag > 10000 且持续增长 |
| `rocketmq_broker_runtime_commitlog_disk_ratio` | commitlog 磁盘使用率 | > 80% |
| `rocketmq_nameserver_rt` | NameServer 响应时间 | P99 > 100 ms |

## 7. 常见问题排查

### 7.1 Producer 报 NO_ROUTE

通常意味着 Topic 未创建或 NameServer 路由信息未同步。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Topic 是否存在以及路由信息
kubectl exec -it rocketmq-broker-master-0 -n middleware -- \
  sh mqadmin topicList -n rocketmq-nameserver.middleware.svc.cluster.local:9876
```
### 7.2 消费 lag 持续增长

步骤一：检查 Consumer 实例数与消费线程数。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看消费者组连接情况与消费进度
kubectl exec -it rocketmq-broker-master-0 -n middleware -- \
  sh mqadmin consumerProgress -n rocketmq-nameserver.middleware.svc.cluster.local:9876 -g order-consumer-group
```
步骤二：检查 Consumer Pod CPU/内存是否受限，是否存在 Full GC 或网络抖动。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Consumer Pod 资源使用与重启次数
kubectl top pod -l app=order-consumer -n middleware
kubectl get pod -l app=order-consumer -n middleware
```
### 7.3 Broker 磁盘占满

commitlog 默认 72 小时或磁盘 75% 触发清理。若业务消息量大，应：

- 扩容磁盘；
- 缩短消息保留时间（`fileReservedTime`）；
- 对非关键 Topic 启用定时删除。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Broker 磁盘使用情况
kubectl exec -it rocketmq-broker-master-0 -n middleware -- df -h /root/store
```
## 8. 生产检查清单

- [ ] NameServer 副本数 >= 3，跨可用区分布；
- [ ] Broker 使用 ESSD 云盘，单 Pod 单盘；
- [ ] Topic 已提前创建或开启自动创建策略；
- [ ] Producer/Consumer 使用 Service DNS 而非固定 Pod IP；
- [ ] 已部署 rocketmq-exporter 与 Grafana dashboard；
- [ ] 消费 lag、磁盘使用率、Broker TPS 已配置告警；
- [ ] 备份了 NameServer/Broker 配置与关键 Topic 元数据；
- [ ] 已验证 Controller 模式下的 Master 自动切换。

## 9. 典型工单诊断决策树

RocketMQ 在 Kubernetes 中的工单主要集中在消息发送失败、消费延迟、Broker 宕机与磁盘空间不足四类。

### 消息发送失败

1. `kubectl get pod` 检查 NameServer 与 Broker Pod 是否全部 Running。
2. 在 Pod 内执行 `sh mqadmin clusterList -n <nameserver>` 确认 Broker 已注册。
3. 检查生产者配置的 NameServer 地址是否正确。
4. 查看 Broker 日志，确认是否因磁盘满或权限问题拒绝写入。

### 消费延迟高

1. `sh mqadmin consumerProgress -g <group>` 查看消费组积压。
2. 检查消费者 Pod 资源是否充足，CPU 或内存是否受限。
3. 评估 Topic 队列数是否过少，必要时扩容队列。
4. 查看消费端日志，确认是否有异常重试或网络抖动。

### Broker 宕机或重启

1. 检查 Pod 事件与 StatefulSet 重启原因。
2. 查看存储是否满或 PVC 是否丢失。
3. 检查 JVM GC 日志，确认是否因堆内存不足导致 OOM。
4. 必要时按主从切换 SOP 恢复服务。

### 磁盘空间不足

1. `df -h` 检查 Broker 数据目录使用率。
2. 调整 `fileReservedTime` 与删除策略。
3. 扩容 PVC 或增加 Broker 节点。

## 10. 相关文档

- [[数据库中间件/03-message-queues/03-message-queue-comparison.md|消息队列选型对比]]
- [[数据库中间件/01-databases/08-kafka-kubernetes-strimzi.md|Kafka on Kubernetes Strimzi]]
- Kubernetes 存储架构概述


<!-- risk-assessed -->

---
title: Kafka StatefulSet 生产部署指南
description: 面向阿里云/专有云 K8s 的 Kafka 生产部署方案，涵盖 KRaft 模式、分区副本设计、扩容缩容、监控告警与性能调优。
summary: 面向阿里云/专有云 K8s 的 Kafka 生产部署方案，涵盖 KRaft 模式、分区副本设计、扩容缩容、监控告警与性能调优。
category: storage
tags:
- k8s
- statefulset
- kafka
- kraft
- streaming
- alicloud
- apsara-stack
- production
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 消息中间件运维
- 数据平台工程师
estimated_read_time: 25min
intent_queries:
- Kafka StatefulSet 生产部署
- K8s 上 Kafka KRaft 模式部署
- 阿里云 K8s Kafka 扩容与监控
trigger_keywords:
- Kafka
- KRaft
- StatefulSet
- partition
- replica
- 扩容
- 监控
prerequisites:
- kubectl-basics
- statefulset-basics
- kafka-basics
- storage-basics
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



# Kafka StatefulSet 生产部署指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，系统讲解 Kafka 在 K8s 上的 KRaft 生产部署、Topic 设计、扩容与监控。

## 目录

1. [架构选型](#架构选型)
2. [KRaft 模式 StatefulSet 部署](#kraft-模式-statefulset-部署)
3. [Topic 分区与副本设计](#topic-分区与副本设计)
4. [扩容与缩容](#扩容与缩容)
5. [数据迁移与再平衡](#数据迁移与再平衡)
6. [监控告警](#监控告警)
7. [性能调优](#性能调优)
8. [常见问题与排错](#常见问题与排错)
9. [阿里云/专有云生产实践](#阿里云专有云生产实践)
10. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 架构选型

### 1.1 部署模式对比

Kafka 在 Kubernetes 上有多种部署方式，选择合适的模式需要综合考虑运维复杂度、性能需求和团队熟悉度。对于阿里云/专有云环境，建议优先评估托管消息队列服务，其次才是自建 Kafka 集群。

| 模式 | 组件 | 复杂度 | 推荐度 | 适用场景 |
|:---|:---|:---:|:---:|:---|
| KRaft | 仅 Kafka Broker | 低 | 高 | K8s 生产首选 |
| ZooKeeper | Kafka + ZK | 中 | 中 | 历史兼容 |
| 阿里云 Kafka / 消息队列 | 托管 | 最低 | 最高 | 核心生产优先 |
| Strimzi Operator | Kafka + ZK/KRaft | 中 | 高 | 需要 Operator 管理 |

### 1.2 节点规划

Kafka 对磁盘 IO 和网络吞吐非常敏感，生产部署需要为 Broker 预留充足的计算和存储资源。

| 节点类型 | 数量 | 存储 | CPU/内存 |
|:---|:---:|:---|:---|
| Broker | 3+ | 本地 SSD / ESSD | 8C / 32Gi |
| Controller (KRaft) | 3 | ESSD | 4C / 8Gi |

对于阿里云/专有云环境，建议优先使用 **阿里云消息队列 Kafka 版**。如果必须自建，则推荐 Kafka 3.x 的 KRaft 模式，以减少 ZooKeeper 的运维负担。KRaft 模式将元数据管理内置到 Kafka 自身，简化了部署架构，同时降低了外部依赖故障带来的风险。

---

## 2. KRaft 模式 StatefulSet 部署

### 2.1 为什么使用 KRaft

Kafka 3.x 引入 KRaft 模式，移除 ZooKeeper 依赖：
- 减少外部依赖，部署更简单
- Controller 元数据自我管理
- 更适合容器化与云原生环境

相比传统 ZooKeeper 模式，KRaft 模式减少了运维组件数量，降低了学习和维护成本。在 Kubernetes 环境中，KRaft 模式可以更好地利用 StatefulSet 的稳定网络标识和持久存储特性。

### 2.2 配置 Kraft 集群元数据

在部署之前，需要生成一个 cluster ID，用于标识整个 Kafka 集群。cluster ID 只需生成一次，所有 Broker 和 Controller 节点共享同一个 ID。

```bash
# 1. 生成 cluster ID
CLUSTER_ID=$(kubectl run kafka-kraft-init --rm -i --restart=Never --image=bitnami/kafka:3.6 -- kafka-storage.sh random-uuid)
echo "Cluster ID: $CLUSTER_ID"
```

### 2.3 StatefulSet 示例

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: kafka-kraft
  namespace: production
spec:
  serviceName: kafka-headless
  replicas: 3
  selector:
    matchLabels:
      app: kafka-kraft
  template:
    metadata:
      labels:
        app: kafka-kraft
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values:
                        - kafka-kraft
                topologyKey: topology.kubernetes.io/zone
      containers:
        - name: kafka
          image: bitnami/kafka:3.6
          env:
            - name: KAFKA_ENABLE_KRAFT
              value: "yes"
            - name: KAFKA_CFG_PROCESS_ROLES
              value: "broker,controller"
            - name: KAFKA_CFG_NODE_ID
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: KAFKA_CFG_CONTROLLER_LISTENER_NAMES
              value: "CONTROLLER"
            - name: KAFKA_CFG_LISTENERS
              value: "PLAINTEXT://:9092,CONTROLLER://:9093"
            - name: KAFKA_CFG_ADVERTISED_LISTENERS
              value: "PLAINTEXT://$(POD_NAME).kafka-headless.production.svc.cluster.local:9092"
            - name: KAFKA_CFG_CONTROLLER_QUORUM_VOTERS
              value: "0@kafka-kraft-0.kafka-headless.production.svc.cluster.local:9093,1@kafka-kraft-1.kafka-headless.production.svc.cluster.local:9093,2@kafka-kraft-2.kafka-headless.production.svc.cluster.local:9093"
            - name: KAFKA_KRAFT_CLUSTER_ID
              value: "CLUSTER_ID_PLACEHOLDER"
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: ALLOW_PLAINTEXT_LISTENER
              value: "yes"
          ports:
            - containerPort: 9092
              name: kafka
            - containerPort: 9093
              name: controller
          resources:
            requests:
              cpu: "4"
              memory: "16Gi"
            limits:
              cpu: "8"
              memory: "32Gi"
          volumeMounts:
            - name: data
              mountPath: /bitnami/kafka
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: openebs-lvmpv-local-ssd
        resources:
          requests:
            storage: 1Ti
```

> **注意**: `KAFKA_KRAFT_CLUSTER_ID` 需要替换为前面生成的真实 cluster ID。`KAFKA_CFG_NODE_ID` 通过 `metadata.name` 注入，需要保证 Pod 名称为纯数字后缀。在生产环境中，建议为 Controller 和 Broker 分别部署独立的 StatefulSet，以隔离元数据管理和数据流处理。

---

## 3. Topic 分区与副本设计

### 3.1 设计原则

合理的 Topic 设计是 Kafka 性能和稳定性的基础。分区数决定了并行度，副本数决定了可用性，而 `min.insync.replicas` 则平衡了一致性与可用性。

| 原则 | 说明 |
|:---|:---|
| 分区数 | 根据消费者并发度设置，建议初始 6-24 |
| 副本因子 | 生产至少 3，保证高可用 |
| min.insync.replicas | 建议 2，平衡可用性与一致性 |
| retention | 按业务需求设置时间与大小 |

### 3.2 创建 Topic

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 创建一个高可用 Topic：3 分区、3 副本
kubectl exec -it kafka-kraft-0 -n production -- kafka-topics.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --create --topic order-events \
  --partitions 6 --replication-factor 3 \
  --config min.insync.replicas=2 \
  --config retention.ms=604800000
```

### 3.3 查看 Topic 分布

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看每个分区的 leader、replica、ISR
kubectl exec -it kafka-kraft-0 -n production -- kafka-topics.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --describe --topic order-events
```

---

## 4. 扩容与缩容

### 4.1 Broker 扩容

修改 StatefulSet replicas 即可扩容 Broker：

```bash
# 将副本数从 3 扩容到 5
kubectl scale sts kafka-kraft -n production --replicas=5
```

扩容后新 Broker 没有数据，需要通过分区重分配平衡负载。

### 4.2 缩容前必须迁移数据

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 生成重分配 JSON，将待下线 broker 的分区迁出
kubectl exec -it kafka-kraft-0 -n production -- kafka-reassign-partitions.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --generate \
  --topics-to-move-json-file /tmp/topics.json \
  --broker-list "0,1,2,3"
```

---

## 5. 数据迁移与再平衡

### 5.1 执行分区重分配

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 先生成重分配计划
kubectl exec -it kafka-kraft-0 -n production -- kafka-reassign-partitions.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --execute \
  --reassignment-json-file /tmp/reassign.json

# 2. 查看进度
kubectl exec -it kafka-kraft-0 -n production -- kafka-reassign-partitions.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --verify \
  --reassignment-json-file /tmp/reassign.json
```

### 5.2 再平衡注意事项

- 重分配期间会占用网络与磁盘 IO，建议在低峰期执行。
- 大集群可分批迁移，避免一次性重分配过多分区。
- 监控 `UnderReplicatedPartitions` 指标，确保数据同步正常。

---

## 6. 监控告警

### 6.1 关键指标

| 指标 | 告警阈值 | 说明 |
|:---|:---|:---|
| UnderReplicatedPartitions | > 0 | 存在未同步分区 |
| OfflinePartitions | > 0 | 离线分区 |
| ActiveControllerCount | != 1 | Controller 数量异常 |
| LeaderElectionRateAndTimeMs | 突增 | leader 选举频繁 |
| BytesInPerSec / BytesOutPerSec | 突增 | 流量异常 |
| RequestQueueTimeMs | > 500ms | 请求队列等待过长 |

### 6.2 PrometheusRule

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kafka-alerts
  namespace: monitoring
spec:
  groups:
    - name: kafka.rules
      rules:
        - alert: KafkaUnderReplicatedPartitions
          expr: kafka_server_replica_manager_under_replicated_partitions > 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Kafka 存在未同步分区"
        - alert: KafkaOfflinePartitions
          expr: kafka_controller_kafkacontroller_offline_partitions_count > 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Kafka 存在离线分区"
        - alert: KafkaNoActiveController
          expr: kafka_controller_kafkacontroller_active_controller_count != 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Kafka Controller 数量异常"
```

---

## 7. 性能调优

### 7.1 JVM 与 Kafka 参数

```properties
# server.properties 关键参数
log.dirs=/var/lib/kafka/data
log.retention.hours=168
log.segment.bytes=1073741824
num.io.threads=16
num.network.threads=8
socket.send.buffer.bytes=102400
socket.receive.buffer.bytes=102400
socket.request.max.bytes=104857600
offsets.topic.replication.factor=3
default.replication.factor=3
min.insync.replicas=2
num.replica.fetchers=4
replica.socket.timeout.ms=30000
```

### 7.2 本地盘优化

使用本地 SSD 时建议：
- 每个 Broker 独占磁盘
- 禁用 swap
- 磁盘挂载使用 `noatime`
- 单盘容量预留 20% 以上

---

## 8. 常见问题与排错

### 8.1 KRaft 启动失败

可能原因包括 cluster ID 不正确、controller 配置不一致或端口冲突。排查方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 日志
kubectl logs kafka-kraft-0 -n production

# 检查 Kraft 元数据初始化
kubectl exec -it kafka-kraft-0 -n production -- kafka-storage.sh info /bitnami/kafka/data
```

### 8.2 消息写入延迟高

常见原因包括：
- `min.insync.replicas` 设置过高，导致写入需要等待更多副本确认。
- 磁盘 IO 瓶颈，尤其是使用网络存储时。
- 网络带宽不足，副本同步延迟。

优化建议：
- 使用本地 SSD 或 ESSD PL3。
- 调整 `num.network.threads` 和 `num.io.threads`。
- 增加分区数以分散写入压力。

### 8.3 消费延迟

消费延迟通常与消费者组配置或消费能力不足有关。排查方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看消费者组延迟
kubectl exec -it kafka-kraft-0 -n production -- kafka-consumer-groups.sh \
  --bootstrap-server kafka-kraft-0.kafka-headless.production.svc.cluster.local:9092 \
  --describe --group order-consumer-group
```

---

## 9. 阿里云/专有云生产实践

### 9.1 存储选择

在阿里云/专有云环境中，Kafka 对存储性能要求极高。建议：
- 优先使用本地 SSD（Local PV）作为 Kafka 数据盘，以获得最佳的顺序写入性能。
- 如果无法使用本地盘，应选择 ESSD PL2 或 PL3 云盘，并确保云盘容量足够大以维持高 IOPS。
- 避免使用 NAS 或 OSS 作为 Kafka 数据存储，因为共享存储的延迟和吞吐无法满足 Kafka 的需求。

### 9.2 网络规划

Kafka Broker 之间、Broker 与客户端之间存在大量网络通信。建议：
- 为 Kafka 集群规划独立的 VPC 子网或安全组，避免与其他业务流量争抢带宽。
- 在阿里云环境中，启用 Terway 网络模式可以提高 Pod 网络性能。
- 对于跨可用区部署，确保可用区之间的网络延迟低于 5ms，否则副本同步延迟会显著增加。

### 9.3 备份与灾备

Kafka 的数据备份通常不采用传统数据库的定期全量备份方式，而是采用双活集群或 MirrorMaker 2 进行实时复制。建议：
- 关键 Topic 配置 `replication.factor=3` 和 `min.insync.replicas=2`。
- 对于需要长期保留的消息，配置合理的 retention 策略，并归档到对象存储。
- 在异地部署灾备集群，使用 MirrorMaker 2 同步关键 Topic。

---

---

## 11. 阿里云/专有云生产实践

### 11.1 存储选择

在阿里云/专有云环境中，Kafka 对存储性能要求极高。建议优先使用本地 SSD（Local PV）作为 Kafka 数据盘，以获得最佳的顺序写入性能。如果无法使用本地盘，应选择 ESSD PL2 或 PL3 云盘，并确保云盘容量足够大以维持高 IOPS。避免使用 NAS 或 OSS 作为 Kafka 数据存储，因为共享存储的延迟和吞吐无法满足 Kafka 的需求。

### 11.2 网络规划

Kafka Broker 之间、Broker 与客户端之间存在大量网络通信。建议为 Kafka 集群规划独立的 VPC 子网或安全组，避免与其他业务流量争抢带宽。在阿里云环境中，启用 Terway 网络模式可以提高 Pod 网络性能。对于跨可用区部署，确保可用区之间的网络延迟低于 5ms，否则副本同步延迟会显著增加。

### 11.3 备份与灾备

Kafka 的数据备份通常不采用传统数据库的定期全量备份方式，而是采用双活集群或 MirrorMaker 2 进行实时复制。建议关键 Topic 配置 replication.factor=3 和 min.insync.replicas=2。对于需要长期保留的消息，配置合理的 retention 策略，并归档到对象存储。在异地部署灾备集群，使用 MirrorMaker 2 同步关键 Topic。

---

## 10. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| KRaft 模式 | 3.0+ 使用 KRaft | 查看环境变量 |
| Broker 数量 | >= 3 | `kubectl get sts kafka-kraft -n production` |
| 副本因子 | >= 3 | `kafka-topics.sh --describe` |
| 跨可用区 | Pod 反亲和性 zone | `kubectl get pod -o wide` |
| 本地 SSD 使用 | Broker 使用 Local PV | `kubectl get pvc` |
| Topic 分布均衡 | 无单 broker 热点 | `kafka-topics.sh --describe` |
| 监控告警 | UnderReplicated、Offline | PrometheusRule |
| 备份策略 | MirrorMaker 2 双活 | 检查目标集群 |

---

## Related

- [[domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns.md|有状态应用 Kubernetes 存储模式]]
- [[domain-04-storage-data/01-k8s-storage/08-storage-performance-tuning.md|存储性能调优]]

## See Also

- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring.md|Prometheus 企业监控]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/33-performance-bottleneck-troubleshooting.md|性能瓶颈故障诊断]]

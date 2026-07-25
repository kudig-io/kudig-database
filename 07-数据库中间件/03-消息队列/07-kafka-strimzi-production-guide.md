---
title: Kafka on Kubernetes with Strimzi Operator — Production Guide
description: K8s 上 Kafka 生产部署 — Strimzi Operator、Kafka 集群配置、Topic 管理、性能调优、监控告警、灾难恢复
summary: 使用 Strimzi Operator 在 Kubernetes 上运行生产级 Kafka 集群的完整实践
category: practice
tags:
- kafka
- strimzi
- operator
- streaming
- production
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# Kafka on Kubernetes — Strimzi 生产部署指南

> 使用 Strimzi Operator 在 K8s 上运行生产级 Kafka 的完整实践。

## 架构概览

```
┌─────────────────────────────────────────────────────┐
│  Strimzi Cluster Operator                           │
│  (监听 Kafka/KafkaTopic/KafkaUser CR)              │
└──────────────────────┬──────────────────────────────┘
                       │ 管理
┌──────────────────────▼──────────────────────────────┐
│  Kafka Cluster (Kafka CR)                           │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐            │
│  │Broker-0 │  │Broker-1 │  │Broker-2 │            │
│  │+ ZK/    │  │+ ZK/    │  │+ ZK/    │  (KRaft)   │
│  │ KRaft   │  │ KRaft   │  │ KRaft   │            │
│  └────┬────┘  └────┬────┘  └────┬────┘            │
│       │             │             │                 │
│  ┌────▼─────────────▼─────────────▼────┐           │
│  │  Persistent Volumes (JBOD/SSD)      │           │
│  └─────────────────────────────────────┘           │
└─────────────────────────────────────────────────────┘
         ▲                    ▲
    Producers            Consumers
```

## Strimzi 安装

```bash
# 安装 Strimzi Operator（Helm）
helm repo add strimzi https://strimzi.io/charts/
helm install strimzi-operator strimzi/strimzi-kafka-operator \
  --namespace kafka-system --create-namespace \
  --set watchNamespaces="{production,staging}" \
  --set resources.requests.memory=384Mi \
  --set resources.limits.memory=512Mi
```

## Kafka 集群部署（KRaft 模式）

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: production-kafka
  namespace: production
  annotations:
    strimzi.io/kraft: enabled
    strimzi.io/node-pools: enabled
spec:
  kafka:
    version: "3.7.0"
    metadataVersion: "3.7-IV4"
    replicas: 3
    listeners:
      - name: plain
        port: 9092
        type: internal
        tls: false
      - name: tls
        port: 9093
        type: internal
        tls: true
        authentication:
          type: scram-sha-512
      - name: external
        port: 9094
        type: loadbalancer
        tls: true
        authentication:
          type: scram-sha-512
        configuration:
          brokerCertChainAndKey:
            secretName: kafka-tls-cert
            certificate: tls.crt
            key: tls.key
    config:
      # 副本与可靠性
      default.replication.factor: 3
      min.insync.replicas: 2
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      transaction.state.log.min.isr: 2
      # 日志保留
      log.retention.hours: 168  # 7 天
      log.retention.bytes: 107374182400  # 100GB per partition
      log.segment.bytes: 1073741824  # 1GB segment
      log.cleanup.policy: delete
      # 性能
      num.io.threads: 8
      num.network.threads: 4
      num.replica.fetchers: 4
      socket.send.buffer.bytes: 1048576
      socket.receive.buffer.bytes: 1048576
      replica.fetch.max.bytes: 10485760
      # 压缩
      compression.type: lz4
      log.compression.type: lz4
    storage:
      type: jbod
      volumes:
        - id: 0
          type: persistent-claim
          size: 500Gi
          class: gp3-encrypted
          deleteClaim: false
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "4"
        memory: 12Gi
    jvmOptions:
      -Xms: 4g
      -Xmx: 4g
      gcLoggingEnabled: false
    template:
      pod:
        affinity:
          podAntiAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
              - labelSelector:
                  matchLabels:
                    strimzi.io/name: production-kafka-kafka
                topologyKey: kubernetes.io/hostname
        priorityClassName: high-priority
    metricsConfig:
      type: jmxPrometheusExporter
      valueFrom:
        configMapKeyRef:
          name: kafka-metrics
          key: kafka-metrics-config.yml
  entityOperator:
    topicOperator:
      resources:
        requests:
          cpu: 200m
          memory: 256Mi
    userOperator:
      resources:
        requests:
          cpu: 200m
          memory: 256Mi
  cruiseControl:
    replicas: 1
    config:
      default.goals: >
        com.linkedin.kafka.cruisecontrol.analyzer.goals.RackAwareGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.MinTopicLeadersPerBrokerGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.ReplicaCapacityGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.DiskCapacityGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.NetworkInboundCapacityGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.NetworkOutboundCapacityGoal
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
```

## Topic 管理（声明式）

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: orders
  namespace: production
  labels:
    strimzi.io/cluster: production-kafka
spec:
  partitions: 12
  replicas: 3
  config:
    retention.ms: "604800000"  # 7 天
    retention.bytes: "53687091200"  # 50GB
    segment.bytes: "1073741824"
    min.insync.replicas: "2"
    cleanup.policy: delete
    compression.type: lz4
    max.message.bytes: "10485760"  # 10MB
---
# 压缩 Topic（变更日志模式）
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: user-events-compacted
  namespace: production
  labels:
    strimzi.io/cluster: production-kafka
spec:
  partitions: 6
  replicas: 3
  config:
    cleanup.policy: compact
    min.cleanable.dirty.ratio: "0.5"
    delete.retention.ms: "86400000"
    segment.bytes: "536870912"
```

## 用户与 ACL

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaUser
metadata:
  name: order-service
  namespace: production
  labels:
    strimzi.io/cluster: production-kafka
spec:
  authentication:
    type: scram-sha-512
  authorization:
    type: simple
    acls:
      - resource:
          type: topic
          name: orders
          patternType: literal
        operations: ["Read", "Write", "Describe"]
      - resource:
          type: group
          name: order-processor
          patternType: prefix
        operations: ["Read"]
      - resource:
          type: topic
          name: order-results
          patternType: literal
        operations: ["Write", "Describe"]
```

## 监控告警

### Prometheus 规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kafka-alerts
  namespace: monitoring
spec:
  groups:
    - name: kafka
      rules:
        - alert: KafkaBrokerDown
          expr: kafka_server_replicamanager_leadercount < 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Kafka Broker {{ $labels.kubernetes_pod_name }} 不可用"
        - alert: KafkaUnderReplicatedPartitions
          expr: kafka_server_replicamanager_underreplicatedpartitions > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "存在未同步副本分区"
        - alert: KafkaConsumerLagHigh
          expr: kafka_consumergroup_lag_sum > 100000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "消费者组 {{ $labels.consumergroup }} 延迟过高"
        - alert: KafkaDiskUsageHigh
          expr: kafka_server_kafkaserver_broker_state / kafka_log_log_size * 100 > 85
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "Kafka 磁盘使用率 > 85%"
        - alert: KafkaControllerNotAvailable
          expr: kafka_controller_kafkacontroller_activecontrollercount != 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Kafka Controller 不可用"
```

## 性能调优

### Producer 配置

```properties
# 高吞吐 Producer
acks=all
retries=3
retry.backoff.ms=100
batch.size=65536
linger.ms=10
buffer.memory=67108864
compression.type=lz4
max.in.flight.requests.per.connection=5
enable.idempotence=true
```

### Consumer 配置

```properties
# 高吞吐 Consumer
fetch.min.bytes=1048576
fetch.max.wait.ms=500
max.partition.fetch.bytes=10485760
max.poll.records=500
max.poll.interval.ms=300000
session.timeout.ms=45000
heartbeat.interval.ms=15000
auto.offset.reset=earliest
enable.auto.commit=false
```

### 容量规划

| 指标 | 计算公式 | 示例 |
|------|----------|------|
| 磁盘 | 日消息量 × 保留天数 × 副本数 × 1.2 | 100GB/天 × 7 × 3 × 1.2 = 2.5TB |
| 网络 | 峰值吞吐 × 3（生产+副本+消费） | 200MB/s × 3 = 600MB/s |
| 分区数 | 目标吞吐 / 单分区吞吐 | 1GB/s / 50MB/s = 20 分区 |
| Broker 数 | max(3, 磁盘总量/单节点容量) | 2.5TB / 1TB = 3 |

## 故障排查

```bash
# 集群状态
kubectl get kafka -n production
kubectl describe kafka production-kafka -n production

# Broker 日志
kubectl logs production-kafka-kafka-0 -n production -c kafka --tail=100

# Topic 状态
kubectl get kafkatopic -n production
kubectl describe kafkatopic orders -n production

# 消费者组延迟
kubectl exec -it production-kafka-kafka-0 -n production -- \
  bin/kafka-consumer-groups.sh --bootstrap-server localhost:9092 \
  --describe --group order-processor

# 分区 leader 分布
kubectl exec -it production-kafka-kafka-0 -n production -- \
  bin/kafka-topics.sh --bootstrap-server localhost:9092 \
  --describe --topic orders
```

## 灾难恢复

| 场景 | 恢复方式 | RTO |
|------|----------|-----|
| 单 Broker 故障 | 自动副本切换 + Strimzi 重建 Pod | < 1min |
| 整个 AZ 故障 | 跨 AZ 副本自动恢复 | < 5min |
| 集群数据损坏 | MirrorMaker 2 从 DR 集群同步 | < 30min |
| etcd/PV 丢失 | 从快照恢复 + 副本重建 | < 1h |

```yaml
# MirrorMaker 2 跨集群复制（DR）
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaMirrorMaker2
metadata:
  name: dr-replication
  namespace: production
spec:
  version: "3.7.0"
  replicas: 2
  connectCluster: target
  clusters:
    - alias: source
      bootstrapServers: source-kafka:9093
      tls:
        trustedCertificates:
          - secretName: source-ca-cert
            certificate: ca.crt
    - alias: target
      bootstrapServers: target-kafka:9093
      tls:
        trustedCertificates:
          - secretName: target-ca-cert
            certificate: ca.crt
  mirrors:
    - sourceCluster: source
      targetCluster: target
      sourceConnector:
        config:
          replication.factor: 3
          offset-syncs.topic.replication.factor: 3
      checkpointConnector:
        config:
          checkpoints.topic.replication.factor: 3
      topicsPattern: ".*"
      groupsPattern: ".*"
```

## Related

- [[07-数据库中间件/03-消息队列/index.md|消息队列]]
- [[07-数据库中间件/03-消息队列/06-kafka-kubernetes-production-guide.md|Kafka 生产指南]]
- [[07-数据库中间件/03-消息队列/03-message-queue-comparison.md|消息队列对比]]

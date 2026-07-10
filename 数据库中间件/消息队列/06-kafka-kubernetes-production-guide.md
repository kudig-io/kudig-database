---
title: Kafka Kubernetes 生产指南
description: 面向生产环境的 Kafka on Kubernetes 运维手册，覆盖 KRaft 与 ZooKeeper 选型、Strimzi Operator 部署、Topic/Partition/Replica 设计、吞吐调优、监控告警、升级与灾难恢复。
summary: 面向生产环境的 Kafka on Kubernetes 运维手册，覆盖 KRaft 与 ZooKeeper 选型、Strimzi Operator 部署、Topic/Partition/Replica 设计、吞吐调优、监控告警、升级与灾难恢复。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- kafka
- strimzi
- kraft
- messaging
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- Kafka on Kubernetes 生产环境如何部署
- KRaft 与 ZooKeeper 在 Kafka on K8s 中如何选择
- Strimzi Operator 生产级配置与调优
- Kafka Topic 分区副本设计与吞吐量优化
trigger_keywords:
- kafka kubernetes
- strimzi
- kraft
- zookeeper
- topic partition replica
- kafka 生产指南
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- kafka-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# Kafka Kubernetes 生产指南

本指南面向需要在 Kubernetes 生产环境中运行 Apache Kafka 的 SRE 与平台工程师，提供从架构选型、Operator 部署、Topic 设计、性能调优到监控告警与灾难恢复的完整运行手册。内容基于 Strimzi 0.45+ 与 Kafka 3.9，并默认推荐 KRaft 模式以简化元数据管理。Kafka 作为事件驱动架构的核心枢纽，其稳定性直接影响微服务间的异步通信、日志聚合、CDC 数据管道以及实时流处理链路，因此生产部署必须从第一天就关注高可用、数据持久化、安全与可观测性。

## 1. 适用场景与范围

本指南适用于以下场景：

- 事件驱动微服务、日志聚合、CDC（变更数据捕获）、实时流处理等场景的 Kafka 集群运维。
- 使用 Strimzi Operator 进行声明式管理的 Kafka on Kubernetes 部署。
- 需要为 Kafka 集群建立生产就绪基线、监控告警、备份恢复与升级流程的团队。

本指南不覆盖自研 Helm Chart 或裸 Pod 部署的 Kafka 集群；如需手动部署，应额外评估控制面复杂度与长期维护成本。同时，本指南重点关注 Kafka Broker 与 Topic 的运维，对于 Kafka Connect、Kafka Streams 与 ksqlDB 的详细配置请参考官方文档与相关专题指南。

## 2. 前置条件与工具

在开始部署前，请确认以下前置条件已经满足：

- Kubernetes 1.28–1.33，节点具备跨可用区（AZ）分布，控制面节点与工作节点之间网络稳定。
- StorageClass 支持块存储或 SSD backing，建议 `volumeBindingMode: WaitForFirstConsumer`，以确保 PVC 与 Pod 调度到同一 AZ。
- 已安装 Helm 3、kubectl，具备管理 CRD 的权限。
- 已部署 Prometheus + Grafana，用于指标采集与可视化。
- 可选但强烈建议：cert-manager 用于 TLS 证书自动轮换；Velero 用于命名空间级备份。
- 建议预先完成内部镜像仓库与 Kafka 镜像同步，避免生产环境直接拉取公网镜像，以降低供应链风险并提升拉取速度。

## 3. 核心概念与架构

### 3.1 KRaft vs ZooKeeper

Kafka 从 2.8 开始引入 KRaft（Kafka Raft）模式，从 3.3 起标记为生产可用。KRaft 用 Kafka 内部的 Raft 仲裁取代了外部 ZooKeeper，从而将元数据管理统一在 Kafka 内部。这一架构变革显著降低了 Kafka on Kubernetes 的运维复杂度，因为不再需要为 ZooKeeper 维护独立的 StatefulSet、存储与升级流程。

| 维度 | KRaft（推荐） | ZooKeeper（传统） |
|---|---|---|
| 元数据管理 | Kafka 内部 Raft 仲裁 | 外部 ZooKeeper 集群 |
| 部署复杂度 | 低，无需额外 ZK Pod | 高，需维护 ZK 集群 |
| 故障转移 | 毫秒级 Raft 选举 | 秒级 ZK 选举 |
| Partition 上限 | 百万级 | 十万级 |
| 控制器性能 | 更高，元数据变更更高效 | 受 ZK 写入能力限制 |
| Strimzi 支持 | 0.35+ | 全版本 |
| 迁移成本 | 一次性、不可逆 | 无需迁移 |

生产环境建议优先使用 KRaft 模式，仅在遗留系统或依赖 ZooKeeper 的周边工具链无法迁移时保留 ZK。迁移到 KRaft 前，必须在 staging 环境完整演练，并准备元数据快照回退方案。需要特别注意的是，KRaft 模式下的 Controller 与 Broker 可以合并部署，也可以分离部署；对于生产环境，建议分离部署以获得更好的故障隔离能力。

### 3.2 Strimzi 架构要点

Strimzi 通过以下 CRD 管理 Kafka 生命周期：

- `Kafka`：定义 Broker/Controller 数量、存储、监听、认证、授权。
- `KafkaTopic`：声明式 Topic 管理，包含分区数、副本因子、配置覆盖。
- `KafkaUser`：声明式用户与 ACL。
- `KafkaConnect` / `KafkaMirrorMaker2`：流处理与跨集群复制。

Topic Operator 与 User Operator 可采用实体 Operator（Entity Operator）或 Unidirectional 模式，生产环境建议启用并配置副本与反亲和性。实体 Operator 将 Topic 和 User 的管理集中在同一 Deployment，适合中小规模；Unidirectional 模式将两个 Operator 分离，便于独立扩缩容与故障隔离。在生产环境中，Operator 本身也应被视为关键服务，配置至少两个副本并跨 AZ 分布。

### 3.3 Topic、Partition、Replica 设计

Topic、Partition 与 Replica 是 Kafka 容量与可靠性的核心杠杆。合理的设计能够在吞吐、延迟与可用性之间取得平衡，而不当的设计则会导致热点、 rebalance 风暴或数据丢失风险。

- **Topic 命名**：采用 `<domain>.<event-name>.<version>` 结构，便于权限治理与版本管理。例如 `payment.order.v1`、`log.app.v2`。清晰的命名规范有助于在多团队共享集群时快速识别数据所有者与用途。
- **Partition 数量**：
  - 计算方式：`max(目标吞吐 / 单 Partition 吞吐, 消费者实例数)`。
  - 单 Broker 建议不超过 1000–2000 个 Partition（含副本）。
  - 避免过度分区，过多 Partition 会增加 Controller 负担、Leader 选举时间与文件句柄消耗。
  - 初始分区数建议为预期峰值消费者实例数的 2–3 倍，预留扩容空间。扩容 Partition 会改变消息顺序保证，因此应在设计阶段尽量预留。
- **Replica Factor**：生产环境至少 `3`，关键 Topic 可配置 `min.insync.replicas=2` 与 `acks=all`，确保写入多数确认。副本因子为 2 时，单节点故障可能导致部分 Partition 不可用；副本因子为 1 仅适用于开发测试环境。
- **Retention**：按业务与合规要求设置 `retention.ms` / `retention.bytes`，时序类数据优先使用压缩与降采样。金融类流水建议保留 7–30 天，日志类可缩短至 1–3 天。Retention 过小可能导致消费者故障时数据被过早删除，过大则会增加存储成本。

### 3.4 高可用与拓扑分布

Kafka 集群必须跨可用区部署，避免单 AZ 故障导致多数副本不可用。Strimzi 支持通过 `rack` 配置启用机架感知，使 Partition 副本自动分布到不同 AZ。

```yaml
spec:
  kafka:
    rack:
      topologyKey: topology.kubernetes.io/zone
```

同时应配置 Pod 反亲和性，确保同一 Broker 的多个 Pod 不会调度到同一节点。对于控制面节点上的 Kafka Controller，也应避免与 etcd 或 API Server 竞争磁盘与网络资源。

### 3.5 存储规划

Kafka 是磁盘密集型服务，存储性能直接决定吞吐与延迟上限。生产环境应优先选择 SSD 或 NVMe  backing 的 StorageClass，并满足以下要求：

- **StorageClass 参数**：使用 `volumeBindingMode: WaitForFirstConsumer`，确保 PVC 与 Pod 调度到同一可用区；启用 `AllowVolumeExpansion: true`，支持在线扩容。
- **磁盘类型**：SSD 是最低要求，NVMe 可显著降低 fsync 延迟；避免使用网络附加存储（如 NFS）作为 Kafka 数据目录。
- **容量估算**：按 `日写入量 × 副本因子 × 保留天数 × 1.2` 估算，预留 20% 突发余量。例如日写入 1 TB、副本因子 3、保留 7 天，则单 Broker 约需 `(1 TB × 3 × 7 / 3) × 1.2 = 8.4 TB`。
- **独立分区**：将 Kafka 日志目录放在独立分区或独立磁盘，避免与系统日志、容器运行时数据竞争 IO。对于 etcd 共存节点，必须物理隔离。
- **快照与备份**：配置 CSI 快照计划，至少每日一次；关键 Topic 同时使用 MirrorMaker 2 复制到异地集群，避免单点存储故障。

定期检查 `LogFlushLatency` 与节点 `iostat -x` 的 `await` 指标，若 `await` 持续高于 50 ms，应考虑升级到更高性能存储或拆分热点 Topic。

## 4. 标准操作流程

### 4.1 安装 Strimzi Operator

```bash
helm repo add strimzi https://strimzi.io/charts/
helm repo update

kubectl create namespace kafka

helm install strimzi-kafka strimzi/strimzi-kafka-operator \
  --namespace kafka \
  --set image.tag=0.45.0 \
  --set watchAnyNamespace=false \
  --set replicas=2
```

验证：

```bash
kubectl get deployment strimzi-cluster-operator -n kafka
kubectl get crd | grep strimzi.io
```

建议将 Operator 的镜像 tag 固定，避免自动升级引入未经测试的 CRD 变更。在多租户场景中，可以部署多个 Strimzi Operator 分别 watch 不同命名空间，以降低单一 Operator 故障的影响范围。

### 4.2 部署 KRaft Kafka 集群

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: prod-kafka
  namespace: kafka
spec:
  kafka:
    version: 3.9.0
    replicas: 3
    roleNodes:
      controller:
        replicas: 3
      broker:
        replicas: 3
    listeners:
      - name: tls
        port: 9093
        type: internal
        tls: true
        authentication:
          type: tls
    authorization:
      type: simple
    config:
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      transaction.state.log.min.isr: 2
      default.replication.factor: 3
      min.insync.replicas: 2
      num.partitions: 6
      log.retention.hours: 168
      log.segment.bytes: 1073741824
    storage:
      type: persistent-claim
      size: 500Gi
      class: fast-ssd
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "4"
        memory: 16Gi
  entityOperator:
    topicOperator: {}
    userOperator: {}
```

应用并观察：

```bash
kubectl apply -f kafka-kraft.yaml
kubectl wait kafka/prod-kafka --for=condition=Ready --timeout=300s -n kafka
kubectl get pods -n kafka -o wide
```

### 4.3 创建 Topic

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: orders.v1
  namespace: kafka
  labels:
    strimzi.io/cluster: prod-kafka
spec:
  partitions: 12
  replicas: 3
  config:
    retention.ms: 604800000
    min.insync.replicas: 2
    cleanup.policy: delete
    max.message.bytes: 1048576
```

验证：

```bash
kubectl get kafkatopic orders.v1 -n kafka
kubectl exec -n kafka prod-kafka-broker-0 -- \
  bin/kafka-topics.sh --bootstrap-server localhost:9092 --describe --topic orders.v1
```

### 4.4 用户与 ACL 管理

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaUser
metadata:
  name: order-service
  namespace: kafka
  labels:
    strimzi.io/cluster: prod-kafka
spec:
  authentication:
    type: tls
  authorization:
    type: simple
    acls:
      - resource:
          type: topic
          name: orders.v1
        operation: Write
      - resource:
          type: topic
          name: orders.v1
        operation: Read
      - resource:
          type: group
          name: order-service-group
        operation: Read
```

### 4.5 吞吐量调优

关键参数：

| 参数 | 推荐值 | 说明 |
|---|---|---|
| `log.flush.interval.messages` | 100000 | 避免过于频繁的刷盘 |
| `log.flush.interval.ms` | 10000 | 与上项配合，平衡持久化与性能 |
| `num.network.threads` | CPU 核数 | 处理网络请求 |
| `num.io.threads` | CPU 核数 × 2 | 处理磁盘 IO |
| `socket.send.buffer.bytes` | 102400 | 高吞吐网络 |
| `socket.receive.buffer.bytes` | 102400 | 高吞吐网络 |
| `linger.ms` | 5–100 | 生产者批量发送 |
| `batch.size` | 32768–131072 | 批量大小 |
| `acks` | 1 / all | 按一致性要求选择 |
| `compression.type` | lz4 / zstd | 降低网络与磁盘开销 |

通过 Strimzi `spec.kafka.config` 注入 Broker 级配置；生产者/消费者配置在应用侧生效。对于高吞吐场景，建议优先调整 `batch.size` 与 `linger.ms`，而非盲目增加 Partition 数量。同时，应避免在生产环境中使用 `acks=0`，因为这会牺牲持久化保证。

### 4.6 滚动升级

升级前检查：

```bash
kubectl get kafka prod-kafka -n kafka -o jsonpath='{.status.conditions}'
kubectl get kafkatopic -n kafka
```

升级 Kafka 版本：

```yaml
spec:
  kafka:
    version: "3.9.1"
    metadataVersion: "3.9"
```

应用后观察滚动更新进度与 Under-Replicated Partitions：

```bash
kubectl exec -n kafka prod-kafka-broker-0 -- \
  bin/kafka-topics.sh --bootstrap-server localhost:9092 --describe | grep -c "Isr"
```

升级过程中应密切关注 `UnderReplicatedPartitions` 与 `ActiveControllerCount` 指标，确保元数据一致性与副本同步正常。

## 5. 关键检查点与验证命令

| 检查项 | 命令/配置 |
|---|---|
| 集群状态 | `kubectl get kafka prod-kafka -n kafka -o jsonpath='{.status.conditions}'` |
| Broker 日志 | `kubectl logs -n kafka prod-kafka-broker-0 --tail=200` |
| Topic 分布 | `kubectl exec ... -- bin/kafka-topics.sh --describe` |
| 消费延迟 | `kubectl exec ... -- bin/kafka-consumer-groups.sh --describe --group <g>` |
| 磁盘使用 | `kubectl top pvc -n kafka` / `df -h`（登录节点） |
| 证书有效期 | `kubectl get secret -n kafka prod-kafka-cluster-ca-cert -o jsonpath='{.data.ca\.crt}' \| base64 -d \| openssl x509 -noout -dates` |
| 控制器健康 | `kubectl exec ... -- bin/kafka-metadata-quorum.sh --bootstrap-server localhost:9092 describe --status` |

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 处置 |
|---|---|---|
| Topic 写入延迟高 | 磁盘 IO 饱和 / network threads 不足 | `iostat -x 1`；扩容磁盘或调整 `num.network.threads` |
| Consumer Lag 持续增长 | 分区数不足 / 消费端 GC / Rebalance 频繁 | 扩容分区或消费者；检查 `max.poll.interval.ms` |
| Broker Pod 重启 | 内存不足 / JVM OOM / 磁盘满 | `kubectl describe pod`；调整 resources.limits；扩容 PVC |
| Under-Replicated Partitions | 节点网络分区 / Broker 离线 | `kubectl get pods -n kafka -o wide`；修复节点后等待自动同步 |
| Controller 选举失败 | KRaft 仲裁节点数不足 | 确保 Controller Pod 跨 AZ 且至少 3 个副本健康 |
| 证书过期导致连接失败 | Strimzi CA 未轮换 / Secret 未注入 | 检查 `StrimziPodSet` 与 `Certificate`；必要时重启 Broker Pod |
| Rebalance 风暴 | 消费者实例频繁扩缩容 | 调整 `session.timeout.ms` 与 `heartbeat.interval.ms` |
| 磁盘 inode 耗尽 | 小文件过多 / log segment 过小 | 调整 `log.segment.bytes`；清理过期日志 |

## 7. 风险与注意事项

- **数据持久化**：Kafka Broker 必须使用持久化存储，禁止 EmptyDir 或本地无冗余路径。
- **跨 AZ 部署**：Broker/Controller 必须配置 Pod 反亲和性，避免单 AZ 故障导致多数副本不可用。
- **升级风险**：跨 Kafka 小版本升级前，先在 staging 验证 `Kafka` CR 的 `version` 与 `inter.broker.protocol.version`。
- **ZooKeeper 迁移**：从 ZK 迁移到 KRaft 是一次性、不可逆操作，需在低峰期执行并提前做快照备份。
- **监控盲区**：Kafka Exporter 与 JMX Exporter 必须同时部署，避免只监控集群级指标而遗漏客户端消费延迟。
- **Topic 删除策略**：生产环境谨慎开启 `delete.topic.enable`，避免误删导致数据丢失。
- **配额治理**：为不同业务团队配置 Producer/Consumer 配额，防止单一应用拖垮集群。

## 8. 相关 Runbook / 推荐阅读

- [[数据库中间件/数据库/08-kafka-kubernetes-strimzi.md|Kafka Kubernetes 企业级实践 — Strimzi Operator 深度指南]]
- [[数据库中间件/99-production-readiness-operations-guide.md|Database & Middleware 生产就绪运维指南]]
- [[可靠性/99-production-readiness-operations-guide.md|可靠性工程生产就绪运维指南]]
- [[生产运维/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- Kafka MirrorMaker2 跨集群复制运维（待补充）
- Kafka 安全加固与 mTLS 最佳实践（待补充）

---

*本指南基于 Strimzi 0.45 / Kafka 3.9 编写，实际部署前请结合具体版本 release notes 与组织安全策略进行裁剪。*

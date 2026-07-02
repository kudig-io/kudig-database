---
title: Flink on Kubernetes：部署、Checkpoint、Savepoint 与自动扩缩容
description: 面向阿里云 ACK / 专有云 ASO 的 Apache Flink on Kubernetes 生产指南，涵盖部署模式、Checkpoint/Savepoint
  管理、自动扩缩容与故障排查
summary: 面向阿里云 ACK / 专有云 ASO 的 Apache Flink on Kubernetes 生产指南，涵盖部署模式、Checkpoint/Savepoint
  管理、自动扩缩容与故障排查
category: domain
tags:
- flink
- kubernetes
- stream-processing
- checkpoint
- savepoint
- autoscaling
- ack
- aso
- stateful
- jobmanager
- taskmanager
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 数据平台工程师
- 流计算开发工程师
estimated_read_time: 22min
intent_queries:
- Flink on Kubernetes 是什么
- 如何在 ACK 上部署 Flink 集群
- Flink Checkpoint Savepoint 自动扩缩容最佳实践
trigger_keywords:
- Flink
- 流处理
- Checkpoint
- Savepoint
- 自动扩缩容
- JobManager
- TaskManager
prerequisites:
- kubectl-basics
- kubernetes-basics
- stream-processing-basics
- flink-basics
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



# Flink on Kubernetes：部署、Checkpoint、Savepoint 与自动扩缩容

## 目录

- [1. 部署模式选型](#1-部署模式选型)
- [2. Flink Kubernetes Operator](#2-flink-kubernetes-operator)
- [3. 应用模式部署](#3-应用模式部署)
- [4. 资源规划与容量估算](#4-资源规划与容量估算)
- [5. Checkpoint 与 Savepoint 管理](#5-checkpoint-与-savepoint-管理)
- [6. 自动扩缩容](#6-自动扩缩容)
- [7. 状态后端与存储](#7-状态后端与存储)
- [8. 与 Kafka 集成](#8-与-kafka-集成)
- [9. 监控告警与日志](#9-监控告警与日志)
- [10. 升级与变更管理](#10-升级与变更管理)
- [11. 常见故障排查](#11-常见故障排查)
- [12. 生产检查清单](#12-生产检查清单)
- [13. 阿里云 OSS Checkpoint 与反压治理](#13-阿里云-oss-checkpoint-与反压治理)
- [14. 相关文档](#14-相关文档)
## 1. 部署模式选型

Flink on Kubernetes 支持三种部署模式，其中 `per-job` 模式已在 Flink 1.15 后废弃，生产环境推荐使用 `application` 模式。

| 模式 | 特点 | 适用场景 |
|------|------|---------|
| Application | 每个 Job 一个专用集群，JobManager 与作业同生命周期 | 生产主推，资源隔离好 |
| Session | 共享 JobManager，多作业提交到同一集群 | 开发测试、多小作业共享 |
| Per-Job（已废弃）| 每个 Job 独立集群，但资源管理由 Flink 负责 | 不推荐 |

Application 模式的优势在于：Job 的 main 方法在 JobManager 中运行，用户代码不会直接暴露在客户端，安全性更高；同时每个作业独立启停，避免了 Session 模式下多个作业相互影响的问题。在阿里云 ACK 生产环境中，Application 模式也更容易与命名空间级别的资源配额、网络策略以及审计日志集成，符合多租户安全合规要求。

## 2. Flink Kubernetes Operator

在阿里云 ACK 中，推荐使用 [Flink Kubernetes Operator](https://nightlies.apache.org/flink/flink-kubernetes-operator-docs-stable/) 管理 Flink 应用生命周期。

### 2.1 安装 Operator

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 添加 Flink Operator Helm 仓库
helm repo add flink-operator-repo https://downloads.apache.org/flink/flink-kubernetes-operator-1.8.0/
helm repo update

# 创建命名空间并安装 operator
kubectl create namespace flink
helm install flink-kubernetes-operator flink-operator-repo/flink-kubernetes-operator -n flink
```

安装后检查 operator 与 webhook：

```bash
# 确认 operator Pod 与 webhook 服务正常
kubectl get pods -n flink
kubectl get svc -n flink
```

## 3. 应用模式部署

### 3.1 提交 Flink 应用

以下示例部署一个基于 Flink 1.18 的流处理作业，使用 `FlinkDeployment` CR：

```yaml
apiVersion: flink.apache.org/v1beta1
kind: FlinkDeployment
metadata:
  name: realtime-etl
  namespace: flink
spec:
  image: registry-vpc.cn-beijing.aliyuncs.com/myrepo/flink-realtime-etl:1.2.0
  flinkVersion: v1.18
  jobManager:
    resource:
      memory: "2Gi"
      cpu: 1
    replicas: 1
  taskManager:
    resource:
      memory: "4Gi"
      cpu: 2
    replicas: 2
  job:
    jarURI: local:///opt/flink/usrlib/realtime-etl.jar
    parallelism: 4
    upgradeMode: savepoint
    state: running
```

创建后查看作业状态：

```bash
# 查看 FlinkDeployment 状态
kubectl get flinkdeployment realtime-etl -n flink -o jsonpath='{.status.jobManagerDeploymentStatus}{"\n"}{.status.jobStatus.state}'

# 查看 JobManager 与 TaskManager Pod
kubectl get pods -n flink -l app=realtime-etl
```

### 3.2 使用阿里云 OSS 作为依赖存储

在专有云 ASO 或 ACK 中，可将作业 JAR 与依赖存放在阿里云 OSS，并通过 `jarURI` 引用 OSS 路径。需在 Flink 镜像中集成 OSS Hadoop 依赖并配置 `fs.oss.endpoint`。

```yaml
spec:
  flinkConfiguration:
    fs.oss.endpoint: oss-cn-beijing-internal.aliyuncs.com
    fs.oss.accessKeyId: ${OSS_ACCESS_KEY_ID}
    fs.oss.accessKeySecret: ${OSS_ACCESS_KEY_SECRET}
  job:
    jarURI: oss://my-flink-jars/realtime-etl-1.2.0.jar
```

## 4. 资源规划与容量估算

在 ACK 中部署 Flink 前，需要根据数据流量估算资源需求。核心公式如下：

| 指标 | 估算方法 | 备注 |
|------|---------|------|
| TaskManager 数量 | `parallelism / slots-per-taskmanager` | 每个 TaskManager 通常配置 2-4 slots |
| 总内存 | `JM memory + TM count × TM memory` | 需预留 20% 缓冲 |
| Checkpoint 存储 | `每秒状态变更量 × Checkpoint 间隔 × 保留数` | 建议使用 OSS |
| 网络带宽 | `输入吞吐量 × 副本因子` | 跨可用区部署需额外考虑 |

例如：parallelism=16，每个 TaskManager 4 slots，则需要 4 个 TaskManager；若每个 TaskManager 8Gi，则总内存约为 32Gi + JobManager 4Gi = 36Gi。实际部署时，还需为 JVM 元空间、RocksDB 缓存、网络缓冲区等预留额外内存，通常建议 TaskManager 内存比估算值增加 20% 到 30%。

## 5. Checkpoint 与 Savepoint 管理

### 5.1 Checkpoint 配置

Checkpoint 是 Flink 的自动容错机制，建议开启增量 Checkpoint 并配置外部化 Checkpoint：

```yaml
spec:
  flinkConfiguration:
    execution.checkpointing.interval: 30s
    execution.checkpointing.min-pause: 30s
    execution.checkpointing.max-concurrent-checkpoints: "1"
    execution.checkpointing.externalized-checkpoint-retention: RETAIN_ON_CANCELLATION
    state.backend.incremental: "true"
    state.backend: rocksdb
    state.checkpoints.dir: oss://my-flink-checkpoints/realtime-etl
```

### 5.2 Savepoint 触发与恢复

Savepoint 用于有计划地停止并恢复作业，例如版本升级或逻辑变更。通过 Flink Kubernetes Operator 触发 Savepoint：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 触发 Savepoint，Flink Operator 会协调创建并记录路径
kubectl annotate flinkdeployment realtime-etl -n flink \
  flink.apache.org/savepoint-trigger-id="upgrade-$(date +%s)"
```

升级时通过 `initialSavepointPath` 从 Savepoint 恢复：

```yaml
spec:
  job:
    initialSavepointPath: oss://my-flink-checkpoints/savepoints/realtime-etl/savepoint-xxx
    upgradeMode: savepoint
    state: running
```

### 5.3 Checkpoint 与 Savepoint 对比

| 特性 | Checkpoint | Savepoint |
|------|------------|-----------|
| 触发方式 | 自动周期性触发 | 手动触发 |
| 保留策略 | 根据配置自动清理 | 长期保留，需手动删除 |
| 用途 | 故障恢复 | 升级、迁移、A/B 测试 |
| 元数据格式 | 内部优化格式 | 稳定格式，跨版本兼容 |
| 对作业影响 | 亚秒级暂停 | 需要暂停作业 |

## 6. 自动扩缩容

### 6.1 基于负载的自动扩缩容

Flink Kubernetes Operator 支持 Autoscaler，可根据 backlog、CPU、延迟等指标自动调整 parallelism：

```yaml
spec:
  flinkConfiguration:
    kubernetes.operator.job.autoscaler.enabled: "true"
    kubernetes.operator.job.autoscaler.stabilization.interval: 5m
    kubernetes.operator.job.autoscaler.metrics.window: 10m
    kubernetes.operator.job.autoscaler.target.utilization: "0.7"
    kubernetes.operator.job.autoscaler.scaleUp.gracePeriod: 5m
```

> 注意：自动扩缩容触发时，作业会先执行 Savepoint，然后以新的 parallelism 重启，期间会有秒级到分钟级中断。对于 SLA 要求极高的场景，建议手动在低峰期执行扩缩容。

### 6.2 手动调整并行度

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 编辑 FlinkDeployment，修改 job.parallelism 与 taskManager.replicas
kubectl patch flinkdeployment realtime-etl -n flink --type merge \
  -p '{"spec":{"job":{"parallelism":8},"taskManager":{"replicas":4}}}'
```

## 7. 状态后端与存储

### 7.1 RocksDB 调优

RocksDB 是生产环境推荐的状态后端，适用于大状态场景。在 ACK 中，建议将 RocksDB 增量数据与 Checkpoint 目录分离：

```yaml
spec:
  flinkConfiguration:
    state.backend: rocksdb
    state.backend.incremental: "true"
    state.backend.rocksdb.memory.managed: "true"
    state.backend.rocksdb.predefined-options: FLASH_SSD_OPTIMIZED
```

### 7.2 存储选型

| 存储 | 适用场景 | 备注 |
|------|---------|------|
| 阿里云 OSS | Checkpoint/Savepoint | 成本低、容量大、与 ACK 集成好 |
| 阿里云 NAS | 本地状态兜底 | 不建议作为 RocksDB 主存储 |
| ESSD 云盘 | RocksDB 本地状态 | 延迟低，但需考虑节点故障恢复 |

对于 TB 级大状态作业，建议将 RocksDB 本地目录挂载为 emptyDir 或 hostPath，并确保 TaskManager 调度到具有 ESSD 的节点上。当节点发生故障时，Flink 会从 Checkpoint 自动恢复状态，因此本地状态目录的临时性不会影响数据一致性，但会延长故障恢复时间。

## 8. 与 Kafka 集成

Flink 与 Kafka 是流处理场景最常见的组合。在 ACK 中，建议 Kafka 与 Flink 部署在同一 VPC 内，减少公网访问带来的延迟与费用。

```yaml
spec:
  flinkConfiguration:
    # Kafka 消费者配置
    security.protocol: SASL_SSL
    sasl.mechanism: PLAIN
    sasl.jaas.config: org.apache.kafka.common.security.plain.PlainLoginModule required username="flink" password="${KAFKA_PASSWORD}";
```

消费延迟增长时，优先通过扩容 parallelism 提升消费能力，同时检查下游算子是否存在反压。在 ACK 中，若 Kafka 与 Flink 部署在不同可用区，还需关注跨可用区网络流量费用与延迟。建议将 Kafka 集群与 Flink 作业部署在同一 VPC 和相近可用区，必要时使用阿里云消息队列 Kafka 版，减少自建 Kafka 的运维负担。此外，建议为 Kafka consumer group 设置合理的 `auto.offset.reset` 与 commit 间隔，避免作业重启后重复消费或丢数。

## 9. 监控告警与日志

### 9.1 暴露 Flink Metrics

Flink 支持通过 Prometheus reporter 暴露指标。启用后，每个 TaskManager 与 JobManager 都会在指定端口暴露 Prometheus 格式的指标，可被集群内的 Prometheus 或 VictoriaMetrics 采集。在 `flinkConfiguration` 中启用：

```yaml
spec:
  flinkConfiguration:
    metrics.reporters: prom
    metrics.reporter.prom.class: org.apache.flink.metrics.prometheus.PrometheusReporter
    metrics.reporter.prom.port: "9249"
```

### 9.2 关键告警规则

| 告警名 | 触发条件 | 处理建议 |
|--------|---------|---------|
| FlinkCheckpointFailed | 连续 Checkpoint 失败 | 检查网络、存储可用性与反压 |
| FlinkJobManagerRestart | JobManager 重启次数增加 | 检查内存与 GC |
| FlinkBackpressureHigh | 反压持续高 | 增加 TaskManager 或优化算子 |
| FlinkKafkaLagGrowing | Kafka 消费延迟增长 | 扩容 parallelism 或排查下游 |

### 9.3 日志收集

建议将 Flink Pod 日志统一采集到阿里云 SLS 或 ELK，便于快速定位 Checkpoint 失败、反压与 OOM 等生产问题：

```bash
# 查看 JobManager 最近异常日志
kubectl logs -n flink deployment/realtime-etl --tail=200 | grep -iE "error|exception|failed"
```

## 10. 升级与变更管理

Flink 作业升级是高风险操作，任何镜像或配置变更都可能导致状态不兼容。升级前应先在测试环境使用相同数据量验证 Savepoint 恢复流程，确认业务指标无异常后再上生产。Flink 作业升级应遵循以下流程：

1. 触发 Savepoint 并确认路径。
2. 更新 `FlinkDeployment` 镜像或配置。
3. Operator 自动从 Savepoint 恢复作业。
4. 验证新作业状态与数据一致性。
5. 保留旧版本 Savepoint，至少 24 小时后再清理。

```yaml
spec:
  job:
    initialSavepointPath: oss://my-flink-checkpoints/savepoints/realtime-etl/savepoint-xxx
    upgradeMode: savepoint
    state: running
```

## 11. 常见故障排查

### 11.1 Checkpoint 超时

```bash
# 查看 Flink UI 或 Pod 日志定位具体失败原因
kubectl logs -n flink deployment/realtime-etl --tail=500 | grep -i checkpoint
```

常见原因：

- 网络带宽不足，导致快照上传慢。
- 反压严重，barrier 无法对齐。
- OSS/RocksDB 存储性能瓶颈。

### 11.2 TaskManager OOM

```bash
# 查看 TaskManager 资源使用与重启次数
kubectl top pod -n flink -l app=realtime-etl,component=taskmanager
kubectl get pods -n flink -l app=realtime-etl,component=taskmanager
```

处理措施：

1. 增加 TaskManager 内存。
2. 调整 RocksDB managed memory 比例。
3. 减少每个 TaskManager 的 slot 数量。

### 11.3 作业卡在 RECONCILING

```bash
# 查看 FlinkDeployment 事件与状态
kubectl describe flinkdeployment realtime-etl -n flink
```

常见原因：CR 配置与现有状态冲突、Savepoint 路径不可达、资源配额不足。

### 11.4 Kafka 消费延迟持续增加

优先检查：

1. 下游算子是否存在反压。
2. parallelism 是否足够。
3. Kafka 分区数是否大于 parallelism。
4. 是否存在数据倾斜导致部分 subtask 过载。

## 12. 生产检查清单

- [ ] 生产环境使用 Application 模式部署，避免 Session 模式资源争抢。
- [ ] 已配置 Checkpoint 间隔与增量 Checkpoint，并启用外部化保留。
- [ ] Checkpoint/Savepoint 存储在阿里云 OSS，且已配置生命周期策略。
- [ ] 已根据业务 SLA 评估自动扩缩容策略，避免高峰期频繁重启。
- [ ] RocksDB 状态后端已启用 managed memory 与 SSD 优化选项。
- [ ] 已配置 Prometheus metrics reporter 与关键告警规则。
- [ ] 已制定升级 SOP：Savepoint → 停作业 → 更新镜像 → 从 Savepoint 恢复。
- [ ] 已限制 Flink 命名空间资源配额，防止单个作业耗尽集群资源。
- [ ] 已配置日志收集并设置关键错误告警。
- [ ] 已在测试环境验证升级回滚流程。

## 13. 阿里云 OSS Checkpoint 与反压治理

在阿里云环境中，Flink 的状态持久化与恢复高度依赖 Checkpoint/Savepoint 存储。OSS 作为高可用对象存储，是专有云 Flink 的首选后端。

### OSS Checkpoint 配置要点

1. 为 Flink 作业配置独立的 OSS bucket，避免与业务数据混用。
2. 启用 OSS 服务端加密，满足数据合规要求。
3. 配置合理的生命周期策略，自动清理过期 Checkpoint。
4. 为 JobManager/TaskManager 配置 RAM 角色，避免 AccessKey 泄露。

### 反压治理

反压是 Flink 作业最常见的性能问题，表现为上游算子等待下游消费。治理思路：

| 现象 | 根因 | 处理 |
|---|---|---|
| 单一算子反压 | 该算子并行度不足 | 增加并行度或优化算子逻辑 |
| 全链路反压 | Sink 吞吐不足 | 扩容 Sink 资源或批量写入 |
| 周期性反压 | Checkpoint 期间资源抢占 | 增加 Checkpoint 间隔或异步 Checkpoint |
| 网络反压 | 序列化开销大 | 使用 Avro/Protobuf 替代 JSON |

### 排查命令

```bash
# 查看 Flink Web UI 反压状态
kubectl port-forward svc/wordcount-app-rest 8081:8081 -n flink

# 查看 JobManager 日志
kubectl logs -f deployment/wordcount-app -n flink

# 查看 TaskManager 资源使用
kubectl top pod -l app=wordcount-app -n flink
```

## 14. 相关文档

- [[domain-16-database-middleware/06-data-streaming/01-cdc-change-data-capture.md|CDC 变更数据捕获]]
- [[domain-16-database-middleware/06-data-streaming/02-stream-processing-overview.md|流处理概述]]

```
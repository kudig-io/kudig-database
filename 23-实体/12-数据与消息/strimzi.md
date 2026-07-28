---
title: Strimzi [entities]
description: '## 概述'
summary: 'Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源项目，通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理。它提供了声明式配置、自动化运维和无缝扩展能力。'
category: entities
tags:
- k8s
- cncf
- streaming
- strimzi
- prometheus
- grafana
- kafka
- crd
- operator
- serverless
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Strimzi 是什么
- 如何 Strimzi
trigger_keywords:
- Strimzi
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Strimzi

> **CNCF 状态**: Incubating | **类别**: Streaming | **主要语言**: Java

## 概述

Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源 Operator，由 Red Hat 开发并开源，2022 年加入 CNCF Incubating。它通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理，提供了声明式配置、自动化运维和无缝扩展能力。Strimzi 覆盖了 Kafka 全生态组件（Broker、ZooKeeper/KRaft、Connect、MirrorMaker、Bridge），是目前 Kafka on Kubernetes 最成熟的开源方案。

## 核心特性

- **全组件管理**: Kafka Broker、ZooKeeper/KRaft、Kafka Connect、MirrorMaker 2、Kafka Bridge
- **CRD 声明式**: Kafka、KafkaTopic、KafkaUser、KafkaConnect 等 CRD 管理
- **滚动升级**: 零停机的 Kafka 版本升级和配置变更
- **安全集成**: TLS 加密、SASL/SCRAM 认证、OAuth 2.0、ACL 授权
- **监控集成**: 内置 Prometheus 指标和 Grafana 仪表盘
- **Topic/User 管理**: 通过 CRD 声明式管理 Kafka Topic 和用户

## 架构

Strimzi 的核心是 Cluster Operator，监听 Kafka CRD，管理 Kafka 集群的全生命周期。架构包含：Cluster Operator（管理 Kafka/ZooKeeper/Connect 集群）、Topic Operator（管理 KafkaTopic CRD 到 Topic 的同步）、User Operator（管理 KafkaUser CRD 到用户/ACL 的同步）。Kafka Broker 以 StatefulSet 运行，数据存储在 PVC 上。每个 Pod 包含 Kafka 进程和 Stunnel（TLS 代理）、Cruise Control（分区重平衡）。Entity Operator（Topic + User Operator）作为单独的 Deployment 运行。

## Kubernetes 集成

Strimzi 完全基于 Kubernetes CRD。Kafka CRD 定义集群规格（Broker 数、存储、网络、安全）。KafkaTopic CRD 声明式创建和管理 Topic（分区数、副本数、配置）。KafkaUser CRD 管理用户认证和 ACL 权限。Operator 通过 Kubernetes API Server 管理资源，无需外部工具。StorageClass 配置决定数据持久化方式。支持 PodAntiAffinity 实现跨可用区分布。

## 生产使用场景

1. **事件流平台**: 在 Kubernetes 上运行 Kafka 作为微服务的事件流基础设施
2. **CDC 数据管道**: 使用 Kafka Connect 连接数据库变更数据
3. **跨集群复制**: 使用 MirrorMaker 2 实现灾备和多区域复制
4. **Kafka 即服务**: 为多团队提供 Kafka 实例的自服务平台

## 安装与配置

```bash
# Helm 安装 Strimzi Operator
helm repo add strimzi https://strimzi.io/charts/
helm install strimzi strimzi/strimzi-kafka-operator \
  --namespace kafka --create-namespace \
  --set watchAnyNamespace=true
# 等待 Operator 就绪
kubectl wait --for=condition=available deployment/strimzi-cluster-operator -n kafka --timeout=120s
```

```yaml
# Kafka 集群 CRD（KRaft 模式，无 ZooKeeper）
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: production-cluster
  namespace: kafka
spec:
  kafka:
    version: 3.7.0
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
    config:
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      min.insync.replicas: 2
      default.replication.factor: 3
      log.retention.hours: 168
      log.segment.bytes: 1073741824
    storage:
      type: jbod
      volumes:
      - id: 0
        type: persistent-claim
        size: 500Gi
        class: fast-ssd
        deleteClaim: false
    resources:
      requests:
        cpu: "2"
        memory: 4Gi
      limits:
        cpu: "4"
        memory: 8Gi
  entityOperator:
    topicOperator:
      reconciliationIntervalSeconds: 60
    userOperator:
      reconciliationIntervalSeconds: 60
  cruiseControl:
    config:
      default.goals: >
        com.linkedin.kafka.cruisecontrol.analyzer.goals.RackAwareGoal,
        com.linkedin.kafka.cruisecontrol.analyzer.goals.MinTopicLeadersPerBrokerGoal
---
# KafkaTopic CRD
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: orders
  namespace: kafka
  labels:
    strimzi.io/cluster: production-cluster
spec:
  partitions: 12
  replicas: 3
  config:
    retention.ms: 604800000
    min.insync.replicas: 2
---
# KafkaUser CRD（SCRAM 认证 + ACL）
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaUser
metadata:
  name: order-service
  namespace: kafka
  labels:
    strimzi.io/cluster: production-cluster
spec:
  authentication:
    type: scram-sha-512
  authorization:
    type: simple
    acls:
    - resource:
        type: topic
        name: orders
      operations: [Read, Write, Describe]
    - resource:
        type: group
        name: order-service-group
      operations: [Read]
```

## 运维操作

```bash
# 🟢 低风险：查看 Kafka 集群状态
kubectl get kafka -A
kubectl describe kafka production-cluster -n kafka
kubectl get kafkatopics -n kafka
kubectl get kafkausers -n kafka

# 🟢 低风险：查看 Broker 日志
kubectl logs production-cluster-kafka-0 -n kafka -c kafka --tail=50

# 🟡 中风险：滚动重启 Broker
kubectl annotate kafka production-cluster -n kafka strimzi.io/manual-rolling-update=true

# 🟡 中风险：扩容 Broker
kubectl patch kafka production-cluster -n kafka --type merge -p '{"spec":{"kafka":{"replicas":5}}}'

# 🟡 中风险：触发 Topic 重平衡（Cruise Control）
kubectl annotate kafka production-cluster -n kafka strimzi.io/rebalance=true

# 🔴 高风险：删除 Kafka 集群（数据丢失）
kubectl delete kafka production-cluster -n kafka
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Kafka 集群未就绪 | PVC 未绑定/存储不足 | `kubectl get pvc -n kafka` | 检查 StorageClass 和容量 |
| Broker Pod CrashLoop | 内存不足/JVM OOM | `kubectl logs <broker-pod> -c kafka` | 增加 resources.limits.memory |
| Topic 未创建 | Entity Operator 未运行 | `kubectl get pods -l app.kubernetes.io/name=entity-operator` | 检查 entityOperator 配置 |
| 客户端连接失败 | TLS/SCRAM 配置错误 | `kubectl get secret <user-name> -n kafka` | 检查证书和凭据 |
| 消息延迟高 | 分区不均衡 | `kubectl get kafka production-cluster -o yaml` | 触发 Cruise Control rebalance |

```
排查流程：
├── 集群未就绪？
│   ├── kubectl describe kafka → 查看 Conditions
│   ├── kubectl get pvc → 检查存储
│   └── kubectl get pods -n kafka → 检查 Pod 状态
├── 客户端连接失败？
│   ├── 检查 Listener 配置和端口
│   ├── 验证 TLS 证书和 SCRAM 凭据
│   └── 检查 NetworkPolicy 是否阻止
└── 性能问题？
    ├── 检查 Broker 日志中的 GC 停顿
    ├── 查看 Cruise Control 指标
    └── 考虑增加 Broker 或调整分区数
```

## 生产案例

### 案例 1：Kafka 零停机升级

- **场景**：生产 Kafka 集群需要从 3.5 升级到 3.7，业务不允许停机
- **排查**：手动升级需要逐个 Broker 操作，风险高且耗时
- **方案**：修改 Kafka CRD 的 version 字段，Strimzi 自动执行滚动升级（逐个 Broker 重启，确保 ISR 满足）
- **效果**：升级全程零停机，耗时 15 分钟，无需人工干预

### 案例 2：多团队 Kafka 即服务

- **场景**：20+ 团队需要独立的 Kafka Topic 和用户，传统方式需要运维团队手动创建
- **排查**：每次新 Topic 申请需要 1-2 天等待，且权限管理混乱
- **方案**：团队通过 KafkaTopic/KafkaUser CRD 自助申请，GitOps 自动同步，ACL 自动配置
- **效果**：Topic 创建从 2 天缩短至 5 分钟，权限 100% 自动化管理

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Strimzi** | CNCF Incubating、Red Hat 支持 | 资源开销大 |
| Confluent for K8s | Confluent 官方、功能丰富 | 商业许可 |
| Koperator (Banzaicloud) | 轻量级 | 社区较小 |
| Bitnami Kafka Chart | 简单快速 | 运维自动化能力弱 |

## 架构定位

在 CNCF 生态中，Strimzi 属于 **Streaming** 类别，是 Kafka on Kubernetes 的标杆项目。它将复杂的 Kafka 运维转化为声明式的 K8s 资源管理。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-kafka-kubernetes-strimzi
- strimzi
- [[23-实体/tremor.md|[[tremor|Tremor]]]]
- [[23-实体/cncf-infrastructure.md|[[23-实体/15-参考与索引/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

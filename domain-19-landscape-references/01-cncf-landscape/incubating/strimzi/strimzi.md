---
title: Strimzi
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- jaeger
- helm
- kafka
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Strimzi 是什么
- 如何 Strimzi
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Strimzi
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- tracing-basics
---

title: Strimzi
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- jaeger
- helm
- kafka
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Strimzi 是什么
- 如何 Strimzi
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Strimzi
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Strimzi

> **成熟度**: Incubating | **加入时间**: 2019-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://strimzi.io |
| **GitHub** | https://github.com/strimzi/strimzi-kafka-operator |
| **许可证** | Apache-2.0 |
| **主要语言** | Java |
| **CNCF 分类** | Streaming & Messaging |

---

## 项目概述

Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源项目，通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理。它提供了声明式配置、自动化运维和无缝扩展能力。

## 核心特性

- **Kubernetes 原生**: 使用 CRD 声明式管理 Kafka 集群
- **全组件覆盖**: Kafka Broker、ZooKeeper/KRaft、Connect、MirrorMaker、Bridge
- **自动化运维**: 滚动更新、自动恢复、证书轮换
- **安全集成**: TLS 加密、SASL 认证、OAuth 2.0、ACL 授权
- **监控集成**: Prometheus 指标导出、Grafana 仪表盘
- **多租户支持**: 命名空间隔离、资源配额管理

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubernetes Cluster                            │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Strimzi Operators                            │   │
│  │  ┌─────────────┐ ┌──────────────┐ ┌─────────────────┐   │   │
│  │  │   Cluster   │ │    Entity    │ │     Topic       │   │   │
│  │  │  Operator   │ │   Operator   │ │    Operator     │   │   │
│  │  └─────────────┘ └──────────────┘ └─────────────────┘   │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                    │
│                    watches/manages                                │
│                              ▼                                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Custom Resources (CRDs)                      │   │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌─────────────────┐   │   │
│  │  │ Kafka  │ │ Topic  │ │ User   │ │ KafkaConnect    │   │   │
│  │  └────────┘ └────────┘ └────────┘ └─────────────────┘   │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                    │
│                    creates/manages                                │
│                              ▼                                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Kafka Ecosystem Pods                         │   │
│  │  ┌────────────────┐  ┌──────────────┐  ┌──────────┐     │   │
│  │  │ Kafka Brokers  │  │  ZooKeeper/  │  │  Kafka   │     │   │
│  │  │   (3+ pods)    │  │    KRaft     │  │ Connect  │     │   │
│  │  └────────────────┘  └──────────────┘  └──────────┘     │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 | 管理的资源 |
|------|------|------------|
| Cluster Operator | 管理 Kafka 集群生命周期 | Kafka, KafkaConnect, KafkaMirrorMaker |
| Entity Operator | 管理 Topic 和 User | KafkaTopic, KafkaUser |
| Topic Operator | Topic 的 CRUD 操作 | 自动同步 Topic 配置 |
| User Operator | 用户认证授权 | ACL、SCRAM 凭证 |

---

## 快速开始

### 安装 Strimzi Operator

```bash
# 创建命名空间
kubectl create namespace kafka

# 安装 Strimzi Operator (使用 Helm)
helm repo add strimzi https://strimzi.io/charts/
helm install strimzi-kafka-operator strimzi/strimzi-kafka-operator \
  --namespace kafka \
  --set watchNamespaces="{kafka}"
```

### 部署 Kafka 集群

```yaml
# kafka-cluster.yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: my-cluster
  namespace: kafka
spec:
  kafka:
    version: 3.6.0
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
      - name: external
        port: 9094
        type: nodeport
        tls: false
    config:
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      transaction.state.log.min.isr: 2
      default.replication.factor: 3
      min.insync.replicas: 2
    storage:
      type: jbod
      volumes:
        - id: 0
          type: persistent-claim
          size: 100Gi
          class: standard
          deleteClaim: false
    resources:
      requests:
        memory: 2Gi
        cpu: 500m
      limits:
        memory: 4Gi
        cpu: 2000m
  zookeeper:
    replicas: 3
    storage:
      type: persistent-claim
      size: 20Gi
      class: standard
  entityOperator:
    topicOperator: {}
    userOperator: {}
```

### 创建 Topic

```yaml
# kafka-topic.yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: my-topic
  namespace: kafka
  labels:
    strimzi.io/cluster: my-cluster
spec:
  partitions: 12
  replicas: 3
  config:
    retention.ms: 604800000
    segment.bytes: 1073741824
    cleanup.policy: delete
```

### 创建 Kafka 用户（SCRAM-SHA-512）

```yaml
# kafka-user.yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaUser
metadata:
  name: my-user
  namespace: kafka
  labels:
    strimzi.io/cluster: my-cluster
spec:
  authentication:
    type: scram-sha-512
  authorization:
    type: simple
    acls:
      - resource:
          type: topic
          name: my-topic
          patternType: literal
        operations:
          - Read
          - Write
          - Describe
        host: "*"
      - resource:
          type: group
          name: my-group
          patternType: literal
        operations:
          - Read
        host: "*"
```

---

## 生产配置

### 高可用配置

```yaml
spec:
  kafka:
    replicas: 5
    rack:
      topologyKey: topology.kubernetes.io/zone
    template:
      pod:
        affinity:
          podAntiAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
              - labelSelector:
                  matchExpressions:
                    - key: strimzi.io/name
                      operator: In
                      values:
                        - my-cluster-kafka
                topologyKey: kubernetes.io/hostname
```

### TLS + OAuth 2.0 认证

```yaml
spec:
  kafka:
    listeners:
      - name: oauth
        port: 9095
        type: internal
        tls: true
        authentication:
          type: oauth
          validIssuerUri: https://keycloak.example.com/realms/kafka
          jwksEndpointUri: https://keycloak.example.com/realms/kafka/protocol/openid-connect/certs
          userNameClaim: preferred_username
```

---

## 监控与运维

### Prometheus 指标

```yaml
spec:
  kafka:
    metricsConfig:
      type: jmxPrometheusExporter
      valueFrom:
        configMapKeyRef:
          name: kafka-metrics
          key: kafka-metrics-config.yml
```

### 关键监控指标

| 指标 | 说明 | 告警阈值 |
|------|------|----------|
| kafka_server_brokertopicmetrics_messagesin_total | 消息入站速率 | 根据业务设定 |
| kafka_controller_kafkacontroller_offlinepartitionscount | 离线分区数 | > 0 |
| kafka_server_replicamanager_underreplicatedpartitions | 副本不足分区 | > 0 |
| kafka_consumer_consumer_fetch_manager_metrics_lag | 消费者延迟 | 根据 SLA |

---

## 与云原生生态集成

```
┌─────────────────────────────────────────────────────────────┐
│                   Strimzi Ecosystem                          │
├─────────────────────────────────────────────────────────────┤
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    │
│   │  Debezium   │    │    Flink    │    │   Spark     │    │
│   │   (CDC)     │───▶│  Streaming  │◀───│  Streaming  │    │
│   └─────────────┘    └─────────────┘    └─────────────┘    │
│          │                  │                  │            │
│          ▼                  ▼                  ▼            │
│   ┌────────────────────────────────────────────────────┐   │
│   │               Strimzi Kafka Cluster                │   │
│   └────────────────────────────────────────────────────┘   │
│          │                  │                  │            │
│          ▼                  ▼                  ▼            │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    │
│   │ Prometheus  │    │   Grafana   │    │   Jaeger    │    │
│   │  (Metrics)  │    │ (Dashboard) │    │  (Tracing)  │    │
│   └─────────────┘    └─────────────┘    └─────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

---

## 最佳实践

1. **存储配置**: 使用高性能 SSD 存储，配置合适的 IOPS
2. **资源隔离**: 为 Kafka 和 ZooKeeper 配置专用节点池
3. **网络策略**: 限制 Kafka 集群的网络访问
4. **备份策略**: 使用 MirrorMaker 2 进行跨集群复制
5. **版本升级**: 使用 Strimzi 的滚动升级能力，零停机更新

---

## 参考资源

- [官方文档](https://strimzi.io/documentation)
- [GitHub Repo](https://github.com/strimzi/strimzi-kafka-operator)
- [Helm Charts](https://github.com/strimzi/strimzi-kafka-operator/tree/main/helm-charts)
- [Strimzi Blog](https://strimzi.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]

---
title: NATS (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- streaming
- nats
- istio
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NATS 是什么
- 如何 NATS
trigger_keywords:
- NATS
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# NATS

> **CNCF 状态**: Incubating | **类别**: Streaming | **主要语言**: Go

## 概述

NATS 是一个 CNCF 毕业项目，由 Synadia 公司主导开发，是一个高性能、轻量级的云原生消息系统。它提供发布-订阅（Pub/Sub）、请求-响应（Request-Reply）和队列组（Queue Group）等消息通信模式。NATS 以极低的延迟（亚毫秒级）和极高的吞吐量（每秒数百万消息）著称，是微服务通信和事件驱动架构的理想选择。项目于 2010 年由 Derek Collison 创建，2018 年加入 CNCF，2023 年正式毕业。

## Key Features（核心能力）

- **高性能消息传输**：单节点支持每秒数百万消息，亚毫秒级延迟
- **多通信模式**：支持 Pub/Sub、Request/Reply、Queue Groups 和 JetStream 持久化
- **JetStream 持久化**：内置持久化引擎，支持 At-Least-Once 和 Exactly-Once 语义
- **多租户支持**：通过 Account 隔离实现多租户安全通信
- **Leaf Nodes**：支持边缘节点连接到核心集群，实现混合部署
- **多语言客户端**：提供 Go, Java, Python, JavaScript, Rust, C 等 20+ 语言客户端

## 架构与工作原理

NATS 架构简洁高效：NATS Server 是核心组件，采用类似 router 的设计，不持久化消息（除非启用 JetStream）。客户端通过 TCP 连接 Server，使用简单的文本协议通信。集群模式通过 gossip 协议实现服务发现和路由。JetStream 作为内嵌的持久化层，支持 Streams（消息流）和 Consumers（消费者），提供文件系统或内存存储后端。Leaf Node 允许远程节点以低延迟方式连接到主集群。

## K8s 集成

NATS 可通过 Helm Chart 部署到 Kubernetes，支持 StatefulSet 部署模式实现集群高可用。JetStream 使用 PVC 提供持久化存储。K8s Service 提供 ClusterIP 或 Headless Service 实现服务发现。NATS 与 K8s 的集成包括：通过 ConfigMap 配置集群参数，通过 Secret 管理 TLS 证书和认证凭据，通过 PodDisruptionBudget 保证可用性。

## 生产用例

- **微服务异步通信**：为微服务架构提供高性能的消息总线
- **事件驱动架构**：作为 Event Backbone 支撑事件溯源和 CQRS 模式
- **IoT 数据接入**：利用 Leaf Node 在边缘收集 IoT 设备数据
- **实时数据流**：金融交易、实时分析等低延迟数据流场景

## 安装与配置

```bash
# 🟢 Helm 安装 (启用 JetStream)
helm repo add nats https://nats-io.github.io/k8s/helm/charts/
helm install nats nats/nats -n nats --create-namespace \
  --set jetstream.enabled=true \
  --set jetstream.fileStorage.size=10Gi

# 🟢 验证安装
kubectl get pods -n nats
kubectl get svc -n nats

# 🟢 测试连接
kubectl run nats-box --image=natsio/nats-box -n nats --rm -it -- sh
nats pub test.subject "hello"
nats sub test.subject

# 🟢 查看 JetStream 状态
nats server info
nats stream ls
nats consumer ls <stream-name>
```

### NATS 集群配置

```yaml
# nats-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: nats-config
  namespace: nats
data:
  nats.conf: |
    server_name: $POD_NAME
    listen: 0.0.0.0:4222
    http: 0.0.0.0:8222
    
    jetstream {
      store_dir: /data/jetstream
      max_mem: 1Gi
      max_file: 10Gi
    }
    
    cluster {
      name: nats-cluster
      port: 6222
      routes [
        nats://nats-0.nats-headless.nats.svc:6222
        nats://nats-1.nats-headless.nats.svc:6222
        nats://nats-2.nats-headless.nats.svc:6222
      ]
    }
    
    authorization {
      users [
        { user: admin, password: $ADMIN_PASS, permissions: { publish: ">", subscribe: ">" } }
        { user: app, password: $APP_PASS, permissions: { publish: "app.>", subscribe: "app.>" } }
      ]
    }
```

### JetStream Stream 配置

```yaml
# 使用 nats CLI 创建 Stream
# nats stream add ORDERS --subjects "orders.>" --storage file --replicas 3 --retention limits --max-msgs 1000000 --max-age 72h

# 或使用 NATS Operator CRD
apiVersion: jetstream.nats.io/v1beta2
kind: Stream
metadata:
  name: orders-stream
  namespace: nats
spec:
  name: ORDERS
  subjects:
  - "orders.>"
  storage: file
  replicas: 3
  retention: limits
  maxMsgs: 1000000
  maxAge: 72h
  discard: old
---
apiVersion: jetstream.nats.io/v1beta2
kind: Consumer
metadata:
  name: orders-consumer
  namespace: nats
spec:
  streamName: ORDERS
  durableName: order-processor
  deliverPolicy: all
  ackPolicy: explicit
  ackWait: 30s
  maxDeliver: 5
  filterSubject: "orders.created"
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 NATS Pod
kubectl get pods -n nats -o wide

# 🟢 查看 NATS 日志
kubectl logs -n nats -l app=nats --tail=50

# 🟢 监控端点 (Prometheus)
curl http://nats.nats.svc:8222/vars

# 🟢 查看连接数
curl http://nats.nats.svc:8222/connz

# 🟢 查看路由状态
curl http://nats.nats.svc:8222/routez

# 🟢 JetStream 信息
nats server info
nats account info

# 🟢 查看 Stream 状态
nats stream ls
nats stream info ORDERS
nats stream report

# 🟢 查看 Consumer 状态
nats consumer ls ORDERS
nats consumer info ORDERS order-processor

# 🟡 删除 Stream (数据丢失)
nats stream rm ORDERS

# 🟢 查看消息
nats stream view ORDERS --count=10
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 客户端连接失败 | 认证错误/网络不通 | `nats server check connection` | 检查用户凭据和 NetworkPolicy |
| 集群路由断开 | 节点间网络问题 | `curl :8222/routez` | 检查 6222 端口连通性 |
| JetStream 写入失败 | 磁盘空间不足 | `nats stream report` | 扩展 PVC 或清理旧消息 |
| Consumer 消息积压 | 处理速度不足 | `nats consumer info <stream> <consumer>` | 增加消费者实例/优化处理逻辑 |
| 消息丢失 | 未启用 JetStream/副本不足 | `nats stream info <name>` | 启用 JetStream + replicas>=3 |
| 内存 OOM | max_mem 配置过高 | `kubectl top pod -n nats` | 调整 max_mem 和 Pod limits |

### 排查流程

```
1. kubectl get pods -n nats → 确认 Pod 状态
2. curl :8222/healthz → 健康检查
3. curl :8222/connz → 查看连接状态
4. nats server info → JetStream 状态
5. nats stream report → 存储使用情况
6. kubectl logs -l app=nats → 查看服务日志
```

## 生产案例

### 案例1: 微服务事件总线
- **场景**: 50+ 微服务需要异步通信，替代 RabbitMQ
- **方案**: NATS JetStream 作为事件总线，按业务域划分 Stream
- **效果**: 延迟从 5ms 降至 0.5ms，运维复杂度大幅降低

### 案例2: IoT 边缘数据汇聚
- **场景**: 100+ 边缘节点采集 IoT 数据需汇聚到中心
- **方案**: 边缘部署 NATS Leaf Node，中心集群 JetStream 持久化
- **效果**: 边缘断网时本地缓存，重连后自动同步，零数据丢失

## 对比替代方案

| 维度 | NATS | Kafka | RabbitMQ | Redis Streams |
|------|------|-------|----------|---------------|
| 延迟 | 亚毫秒 | 毫秒级 | 毫秒级 | 亚毫秒 |
| 吞吐量 | 极高 | 极高 | 中 | 高 |
| 持久化 | JetStream | 原生 | 原生 | 有限 |
| 运维复杂度 | 低 | 高 | 中 | 低 |
| 协议支持 | NATS/STOMP | Kafka | AMQP/MQTT | Redis |
| 多租户 | Account 隔离 | 无 | VHost | 无 |
| 边缘支持 | Leaf Node | MirrorMaker | Federation | 无 |

## 检查清单

- [ ] 集群副本数 >= 3，配置 PDB
- [ ] JetStream 启用且 replicas >= 3
- [ ] 认证和授权已配置 (非开放访问)
- [ ] TLS 加密已启用 (生产环境)
- [ ] 监控端点 (8222) 已接入 Prometheus
- [ ] PVC 大小充足且有扩容计划
- [ ] Consumer ackPolicy 设置为 explicit
- [ ] 配置了消息保留策略 (maxAge/maxMsgs)

## Related

- [[vineyard]] — Vineyard
- [[distribution]] — Distribution
- [[03-istio-security-hardening]] — [[istio|Istio]]io 安全加固|Istio 安全加固]]
- [[copa]] — Copa (Copacetic)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- nats
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

---
title: NATS 深度解析
description: '# NATS 深度解析'
category: domain
tags:
- nats
- message-queue
- jetstream
- cloud-native
- statefulset
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NATS 深度解析 是什么
- 如何 NATS 深度解析
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- NATS
- 深度解析
- database
- middleware
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# [[NATS|NATS]] 深度解析

## 架构演进

```
NATS Server:
├── Core NATS: 发布/订阅、请求/回复、队列组
├── JetStream: 持久化、流、消费者、KV Store
└── Leaf Nodes: 边缘计算、桥接
```

## Core NATS vs JetStream

| 特性 | Core NATS | JetStream |
|------|-----------|-----------|
| 持久化 | ❌ | ✅ |
| 至少一次交付 | ❌ | ✅ |
| 消息重放 | ❌ | ✅ |
| 流处理 | ❌ | ✅ |
| KV Store | ❌ | ✅ |
| 性能 | 极高（10M+/sec） | 高（1M+/sec） |

## [[Kubernetes|Kubernetes]] 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: nats
spec:
  serviceName: nats
  replicas: 3
  template:
    spec:
      containers:
      - name: nats
        image: nats:2.10-alpine
        args:
          - --cluster_name
          - nats-cluster
          - --js
          - --store_dir
          - /data/jetstream
          - --cluster
          - nats://0.0.0.0:6222
          - --routes
          - nats://nats-0.nats:6222,nats://nats-1.nats:6222,nats://nats-2.nats:6222
        volumeMounts:
        - name: js-data
          mountPath: /data/jetstream
  volumeClaimTemplates:
  - metadata:
      name: js-data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```

## 相关

- [[domain-16-database-middleware/03-message-queues/02-pulsar-architecture]]

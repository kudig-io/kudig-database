# NATS

> **成熟度**: Incubating | **加入时间**: 2018-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://nats.io |
| **GitHub** | https://github.com/nats-io/nats-server |
| **文档** | https://docs.nats.io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Streaming & Messaging |

---

## 项目概述

### 简介
NATS 是一个简单、安全、高性能的云原生消息系统。它提供发布订阅、请求响应和持久化消息队列功能，以极简的设计实现超高性能。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2010 | Derek Collison 创建 |
| 2018-03 | 加入 CNCF Sandbox |
| 2020-04 | 晋升为 CNCF Incubating |
| 2021 | JetStream 持久化功能发布 |

### 核心定位
NATS 是云原生应用的神经系统，以零依赖、超低延迟著称，适合需要高性能消息传递的微服务架构。

---

## 架构设计

### 消息模式

```
┌─────────────────────────────────────────────────────────────────┐
│                    NATS 消息模式                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Publish-Subscribe (发布订阅)                                 │
│     ┌──────────┐     subject: orders.*                          │
│     │Publisher │─────────────────────────────────┐              │
│     └──────────┘                                 │              │
│                        ┌─────────────────────────┼──────┐       │
│                        ▼                         ▼      ▼       │
│                   ┌────────┐  ┌────────┐  ┌────────┐           │
│                   │ Sub A  │  │ Sub B  │  │ Sub C  │           │
│                   └────────┘  └────────┘  └────────┘           │
│                                                                  │
│  2. Request-Reply (请求响应)                                     │
│     ┌──────────┐    Request    ┌──────────┐                     │
│     │ Client   │──────────────►│ Service  │                     │
│     │          │◄──────────────│          │                     │
│     └──────────┘    Reply      └──────────┘                     │
│                                                                  │
│  3. Queue Groups (负载均衡)                                      │
│     ┌──────────┐                                                │
│     │Publisher │─────────────────────────────┐                  │
│     └──────────┘                             │                  │
│                        Queue Group: workers  │                  │
│                   ┌──────────────────────────┼──────┐          │
│                   ▼          ▼               ▼      │          │
│                ┌────────┐ ┌────────┐ ┌────────┐    │          │
│                │Worker 1│ │Worker 2│ │Worker 3│    │          │
│                └────────┘ └────────┘ └────────┘    │          │
│                   只有一个 Worker 收到消息 ────────┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### JetStream 持久化

```
┌─────────────────────────────────────────────────────────────────┐
│                   JetStream 架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                      Stream                                  ││
│  │  ┌─────────────────────────────────────────────────────┐   ││
│  │  │  Messages: [msg1] [msg2] [msg3] [msg4] [msg5] ...   │   ││
│  │  │  Subjects: orders.*, payments.*                      │   ││
│  │  │  Retention: Limits / Interest / WorkQueue           │   ││
│  │  └─────────────────────────────────────────────────────┘   ││
│  │            │                    │                           ││
│  │            ▼                    ▼                           ││
│  │  ┌─────────────────┐  ┌─────────────────┐                  ││
│  │  │   Consumer A    │  │   Consumer B    │                  ││
│  │  │  (Push/Pull)    │  │  (Push/Pull)    │                  ││
│  │  │  AckPolicy:     │  │  AckPolicy:     │                  ││
│  │  │  Explicit/All   │  │  None           │                  ││
│  │  └─────────────────┘  └─────────────────┘                  ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用示例

### Go 客户端

```go
package main

import (
    "github.com/nats-io/nats.go"
    "log"
)

func main() {
    // 连接
    nc, _ := nats.Connect(nats.DefaultURL)
    defer nc.Close()
    
    // 发布订阅
    nc.Subscribe("orders.*", func(m *nats.Msg) {
        log.Printf("Received: %s", string(m.Data))
    })
    
    nc.Publish("orders.new", []byte(`{"id": "123"}`))
    
    // 请求响应
    msg, _ := nc.Request("api.users", []byte("get"), time.Second)
    log.Printf("Response: %s", string(msg.Data))
    
    // JetStream
    js, _ := nc.JetStream()
    
    // 创建 Stream
    js.AddStream(&nats.StreamConfig{
        Name:     "ORDERS",
        Subjects: []string{"orders.*"},
    })
    
    // 发布持久化消息
    js.Publish("orders.new", []byte(`{"id": "456"}`))
    
    // 创建 Consumer
    sub, _ := js.PullSubscribe("orders.*", "worker")
    msgs, _ := sub.Fetch(10)
    for _, msg := range msgs {
        msg.Ack()
    }
}
```

### Kubernetes 部署

```yaml
# NATS Helm 安装
# helm install nats nats/nats --set jetstream.enabled=true
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
          image: nats:2.10
          args:
            - -js  # 启用 JetStream
            - -m 8222  # 监控端口
          ports:
            - containerPort: 4222  # 客户端
            - containerPort: 6222  # 集群
            - containerPort: 8222  # 监控
```

---

## 性能特点

| 指标 | 数值 |
|:---|:---|
| **延迟** | < 1ms (P99) |
| **吞吐量** | 10M+ msg/sec |
| **二进制大小** | ~10MB |
| **内存占用** | ~10MB 起步 |

---

## 参考资源

- [官方文档](https://docs.nats.io)
- [GitHub Repo](https://github.com/nats-io/nats-server)
- [CNCF 项目页面](https://www.cncf.io/projects/nats/)
- [NATS by Example](https://natsbyexample.com/)

---

**维护者**: Kudig Team | **许可证**: MIT

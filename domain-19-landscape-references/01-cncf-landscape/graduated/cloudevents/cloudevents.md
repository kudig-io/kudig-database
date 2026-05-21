---
title: CloudEvents
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- kafka
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- CloudEvents 是什么
- 如何 CloudEvents
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- CloudEvents
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
---

title: CloudEvents
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kafka
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- CloudEvents 是什么
- 如何 CloudEvents
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CloudEvents
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

# CloudEvents

> **成熟度**: Graduated | **加入时间**: 2018-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cloudevents.io |
| **GitHub** | https://github.com/cloudevents/spec |
| **文档** | https://cloudevents.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | 规范/多语言 SDK |
| **CNCF 分类** | Serverless |

---

## 项目概述

### 简介
CloudEvents 是一个描述事件数据的开放规范，提供跨服务、平台和系统的事件互操作性。它定义了事件的通用格式、必需属性和协议绑定，解决了事件驱动架构中的数据格式碎片化问题。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2018-05 | 加入 CNCF Sandbox |
| 2019-10 | CloudEvents 1.0 规范发布 |
| 2019-10 | 晋升为 CNCF Incubating |
| 2024-01 | 晋升为 CNCF Graduated |

### 核心定位
CloudEvents 是事件驱动架构的"通用语言"，被 Knative、Azure Event Grid、AWS EventBridge 等平台广泛采用，是实现事件互操作的行业标准。

---

## 规范结构

### 核心属性

```
┌─────────────────────────────────────────────────────────────────┐
│                    CloudEvents 核心属性                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  必需属性 (Required)                                             │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ specversion │ 规范版本，如 "1.0"                            ││
│  │ id          │ 事件唯一标识符                                 ││
│  │ source      │ 事件来源，URI 格式                             ││
│  │ type        │ 事件类型，如 "com.example.order.created"       ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  可选属性 (Optional)                                             │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ datacontenttype │ 数据格式，如 "application/json"           ││
│  │ dataschema      │ 数据 Schema URI                           ││
│  │ subject         │ 事件主题，细化事件来源                    ││
│  │ time            │ 事件发生时间，RFC 3339 格式               ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  数据 (Data)                                                     │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ data            │ 事件负载，任意格式                        ││
│  │ data_base64     │ Base64 编码的二进制数据                   ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### JSON 格式示例

```json
{
  "specversion": "1.0",
  "type": "com.example.order.created",
  "source": "/orders/service",
  "id": "A234-1234-1234",
  "time": "2024-01-15T10:30:00Z",
  "datacontenttype": "application/json",
  "subject": "order-12345",
  "data": {
    "orderId": "12345",
    "customerId": "customer-789",
    "items": [
      {"productId": "prod-001", "quantity": 2},
      {"productId": "prod-002", "quantity": 1}
    ],
    "totalAmount": 199.99,
    "currency": "USD"
  }
}
```

---

## 协议绑定

### HTTP 绑定

```http
# 结构化模式 (Structured Mode)
POST /events HTTP/1.1
Host: event-receiver.example.com
Content-Type: application/cloudevents+json

{
  "specversion": "1.0",
  "type": "com.example.order.created",
  "source": "/orders",
  "id": "event-001",
  "data": {"orderId": "12345"}
}
```

```http
# 二进制模式 (Binary Mode) - 属性在 Header 中
POST /events HTTP/1.1
Host: event-receiver.example.com
Content-Type: application/json
ce-specversion: 1.0
ce-type: com.example.order.created
ce-source: /orders
ce-id: event-001

{"orderId": "12345"}
```

### Kafka 绑定

```
┌─────────────────────────────────────────────────────────────────┐
│                   Kafka CloudEvents                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Kafka Record                                                    │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │ Headers:                                                     ││
│  │   ce_specversion: "1.0"                                      ││
│  │   ce_type: "com.example.order.created"                       ││
│  │   ce_source: "/orders"                                       ││
│  │   ce_id: "event-001"                                         ││
│  │   content-type: "application/json"                           ││
│  │                                                              ││
│  │ Key: "order-12345"                                           ││
│  │                                                              ││
│  │ Value: {"orderId": "12345", "amount": 99.99}                 ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## SDK 使用

### Go SDK

```go
package main

import (
    "context"
    "log"
    
    cloudevents "github.com/cloudevents/sdk-go/v2"
)

// 创建事件
func createEvent() cloudevents.Event {
    event := cloudevents.NewEvent()
    event.SetSource("/orders/service")
    event.SetType("com.example.order.created")
    event.SetID("event-001")
    event.SetData(cloudevents.ApplicationJSON, map[string]interface{}{
        "orderId": "12345",
        "amount":  99.99,
    })
    return event
}

// 发送事件
func sendEvent() {
    client, _ := cloudevents.NewClientHTTP()
    ctx := cloudevents.ContextWithTarget(context.Background(), 
        "http://event-receiver:8080/events")
    
    event := createEvent()
    if result := client.Send(ctx, event); cloudevents.IsUndelivered(result) {
        log.Printf("Failed to send: %v", result)
    }
}

// 接收事件
func receiveEvents() {
    client, _ := cloudevents.NewClientHTTP()
    
    client.StartReceiver(context.Background(), func(event cloudevents.Event) {
        log.Printf("Received event: %s", event.Type())
        log.Printf("Data: %s", event.Data())
    })
}
```

### Python SDK

```python
from cloudevents.http import CloudEvent, to_structured
from cloudevents.conversion import to_json
import requests

# 创建事件
def create_event():
    attributes = {
        "type": "com.example.order.created",
        "source": "/orders/service",
    }
    data = {
        "orderId": "12345",
        "amount": 99.99
    }
    return CloudEvent(attributes, data)

# 发送事件 (结构化模式)
def send_event():
    event = create_event()
    headers, body = to_structured(event)
    requests.post(
        "http://event-receiver:8080/events",
        headers=headers,
        data=body
    )

# Flask 接收事件
from flask import Flask, request
from cloudevents.http import from_http

app = Flask(__name__)

@app.route("/events", methods=["POST"])
def receive_event():
    event = from_http(request.headers, request.get_data())
    print(f"Received: {event['type']} - {event.data}")
    return "", 200
```

### JavaScript SDK

```javascript
const { CloudEvent, httpTransport } = require('cloudevents');

// 创建事件
const event = new CloudEvent({
  type: 'com.example.order.created',
  source: '/orders/service',
  data: {
    orderId: '12345',
    amount: 99.99
  }
});

// 发送事件
async function sendEvent() {
  const response = await httpTransport.binary(event)
    .target('http://event-receiver:8080/events')
    .send();
  console.log('Event sent:', response.status);
}

// Express 接收事件
const express = require('express');
const { HTTP } = require('cloudevents');

const app = express();
app.use(express.json());

app.post('/events', (req, res) => {
  const event = HTTP.toEvent({ headers: req.headers, body: req.body });
  console.log(`Received: ${event.type}`, event.data);
  res.status(200).send();
});
```

---

## 生态集成

```
┌─────────────────────────────────────────────────────────────────┐
│                    CloudEvents 生态系统                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  云平台                                                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Azure Event │ │AWS EventBridge│ │ Google     │               │
│  │ Grid        │ │             │ │ Eventarc   │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
│                                                                  │
│  CNCF 项目                                                       │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │  Knative    │ │   Dapr      │ │  Argo       │               │
│  │  Eventing   │ │   Pub/Sub   │ │  Events     │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
│                                                                  │
│  消息系统                                                        │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │   Kafka     │ │  RabbitMQ   │ │   NATS      │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

| 项目/平台 | 集成方式 |
|:---|:---|
| **Knative Eventing** | 原生 CloudEvents 支持 |
| **Dapr** | Pub/Sub 使用 CloudEvents 格式 |
| **Azure Event Grid** | 原生支持，Schema Registry |
| **AWS EventBridge** | 通过 Schema Registry 集成 |
| **Kafka** | CloudEvents 协议绑定 |

---

## 使用场景

### 1. 微服务事件通信

```yaml
# Knative Eventing 示例
apiVersion: sources.knative.dev/v1
kind: PingSource
metadata:
  name: heartbeat
spec:
  schedule: "*/1 * * * *"
  contentType: "application/json"
  data: '{"message": "heartbeat"}'
  sink:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: event-display
```

### 2. 多云事件路由

```
┌─────────────────────────────────────────────────────────────────┐
│               多云事件路由架构                                    │
│                                                                  │
│   AWS                    Azure                    GCP           │
│   ┌────────────┐        ┌────────────┐        ┌────────────┐   │
│   │ Lambda     │        │ Functions  │        │ Cloud Run  │   │
│   │ (Producer) │        │ (Producer) │        │ (Producer) │   │
│   └─────┬──────┘        └─────┬──────┘        └─────┬──────┘   │
│         │ CloudEvents        │ CloudEvents        │ CloudEvents│
│         └────────────────────┼────────────────────┘            │
│                              ▼                                  │
│                    ┌─────────────────┐                          │
│                    │  Event Router   │                          │
│                    │  (统一格式)     │                          │
│                    └────────┬────────┘                          │
│                             │                                   │
│              ┌──────────────┼──────────────┐                   │
│              ▼              ▼              ▼                   │
│         ┌────────┐    ┌────────┐    ┌────────┐                 │
│         │Analytics│    │Archive │    │Alerting│                 │
│         └────────┘    └────────┘    └────────┘                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 参考资源

- [CloudEvents 规范](https://github.com/cloudevents/spec)
- [官方网站](https://cloudevents.io)
- [CNCF 项目页面](https://www.cncf.io/projects/cloudevents/)
- [Go SDK](https://github.com/cloudevents/sdk-go)
- [Python SDK](https://github.com/cloudevents/sdk-python)
- [JavaScript SDK](https://github.com/cloudevents/sdk-javascript)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/dapr.md|dapr]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference

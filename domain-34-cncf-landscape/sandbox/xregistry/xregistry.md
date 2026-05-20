---
title: xRegistry
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- redis
- mysql
- postgresql
- kafka
- job
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- xRegistry 是什么
- 如何 xRegistry
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- xRegistry
- cncf
- landscape
---


# xRegistry

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/xregistry/spec |
| **官网** | https://xregistry.io/ |
| **许可证** | Apache-2.0 |
| **规范版本** | 0.5 |
| **CNCF 分类** | Event-Driven / Schema Registry |
| **相关标准** | CloudEvents / AsyncAPI / OpenAPI |

---

## 项目概述

xRegistry 是一个通用的元数据注册中心规范，用于管理和发现事件驱动架构中的各类资源。它定义了一种标准化的 API 来注册、存储和查询消息定义、模式（Schema）、端点等元数据，支持 CloudEvents、AsyncAPI、OpenAPI 等多种规范，是构建可互操作事件驱动系统的基础设施。

### 核心价值

- **统一注册**: 集中管理事件类型、消息模式、服务端点
- **多格式支持**: CloudEvents、AsyncAPI、OpenAPI、JSON Schema
- **版本管理**: 完整的版本控制和兼容性检查
- **服务发现**: 动态发现可用的事件源和消费者
- **互操作性**: 跨平台、跨语言的元数据共享

---

## 核心概念

### 注册中心模型

```
┌─────────────────────────────────────────────────────────────────┐
│                      xRegistry Model                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                     Registry                               │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │                    Groups                            │  │  │
│  │  │                                                      │  │  │
│  │  │  ┌──────────────┐  ┌──────────────┐                 │  │  │
│  │  │  │   Messages   │  │   Schemas    │                 │  │  │
│  │  │  │    Group     │  │    Group     │                 │  │  │
│  │  │  │              │  │              │                 │  │  │
│  │  │  │  ┌────────┐  │  │  ┌────────┐  │                 │  │  │
│  │  │  │  │Resource│  │  │  │Resource│  │                 │  │  │
│  │  │  │  ├────────┤  │  │  ├────────┤  │                 │  │  │
│  │  │  │  │Version │  │  │  │Version │  │                 │  │  │
│  │  │  │  │Version │  │  │  │Version │  │                 │  │  │
│  │  │  │  └────────┘  │  │  └────────┘  │                 │  │  │
│  │  │  │              │  │              │                 │  │  │
│  │  │  └──────────────┘  └──────────────┘                 │  │  │
│  │  │                                                      │  │  │
│  │  │  ┌──────────────┐  ┌──────────────┐                 │  │  │
│  │  │  │  Endpoints   │  │  Subscriptions│                │  │  │
│  │  │  │    Group     │  │    Group     │                 │  │  │
│  │  │  └──────────────┘  └──────────────┘                 │  │  │
│  │  │                                                      │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 资源层次

| 层级 | 描述 | 示例 |
|:---|:---|:---|
| **Registry** | 注册中心根节点 | 整个元数据存储 |
| **Group** | 资源分组 | messages, schemas, endpoints |
| **Resource** | 具体资源 | OrderCreatedEvent, UserSchema |
| **Version** | 资源版本 | v1.0.0, v1.1.0 |

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                      xRegistry Architecture                        │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                     Producers & Consumers                     │ │
│  │                                                                │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐     │ │
│  │  │ Producer │  │ Producer │  │ Consumer │  │ Consumer │     │ │
│  │  │    A     │  │    B     │  │    X     │  │    Y     │     │ │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘     │ │
│  │       │             │             │             │            │ │
│  └───────│─────────────│─────────────│─────────────│────────────┘ │
│          │             │             │             │              │
│          └─────────────┴──────┬──────┴─────────────┘              │
│                               │                                    │
│  ┌────────────────────────────▼─────────────────────────────────┐ │
│  │                    xRegistry Server                           │ │
│  │                                                                │ │
│  │  ┌────────────────────────────────────────────────────────┐  │ │
│  │  │                    REST API                             │  │ │
│  │  │                                                          │  │ │
│  │  │  GET  /groups/{group}/resources                          │  │ │
│  │  │  POST /groups/{group}/resources                          │  │ │
│  │  │  GET  /groups/{group}/resources/{id}/versions            │  │ │
│  │  │                                                          │  │ │
│  │  └────────────────────────────────────────────────────────┘  │ │
│  │                              │                                │ │
│  │  ┌────────────────────────────────────────────────────────┐  │ │
│  │  │                  Registry Core                          │  │ │
│  │  │                                                          │  │ │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐              │  │ │
│  │  │  │  Group   │  │ Resource │  │ Version  │              │  │ │
│  │  │  │ Manager  │  │ Manager  │  │ Manager  │              │  │ │
│  │  │  └──────────┘  └──────────┘  └──────────┘              │  │ │
│  │  │                                                          │  │ │
│  │  └────────────────────────────────────────────────────────┘  │ │
│  │                              │                                │ │
│  │  ┌────────────────────────────────────────────────────────┐  │ │
│  │  │                   Storage Backend                       │  │ │
│  │  │                                                          │  │ │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐   │  │ │
│  │  │  │PostgreSQL│  │  MySQL  │  │  Redis  │  │  S3     │   │  │ │
│  │  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘   │  │ │
│  │  │                                                          │  │ │
│  │  └────────────────────────────────────────────────────────┘  │ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                                                                    │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 注册中心模型定义

```yaml
# registry-model.yaml
$schema: https://xregistry.io/schema/registry-model
specversion: "0.5"

groups:
  # 消息定义组
  messages:
    singular: message
    plural: messages
    resourceSchema:
      type: object
      properties:
        format:
          type: string
          enum: [CloudEvents, AsyncAPI, Custom]
        schemaUrl:
          type: string
          format: uri
        schemaFormat:
          type: string
    
  # Schema 定义组  
  schemas:
    singular: schema
    plural: schemas
    resourceSchema:
      type: object
      properties:
        format:
          type: string
          enum: [JsonSchema, Avro, Protobuf, XSD]
        content:
          type: string
          
  # 端点定义组
  endpoints:
    singular: endpoint
    plural: endpoints
    resourceSchema:
      type: object
      properties:
        protocol:
          type: string
          enum: [HTTP, AMQP, MQTT, Kafka, NATS]
        url:
          type: string
          format: uri
        usage:
          type: string
          enum: [producer, consumer, both]
```

### 注册消息类型

```bash
# 创建消息定义
curl -X POST https://registry.example.com/messages \
  -H "Content-Type: application/json" \
  -d '{
    "id": "order.created",
    "name": "Order Created Event",
    "description": "Emitted when a new order is created",
    "format": "CloudEvents",
    "schemaFormat": "application/schema+json",
    "schema": {
      "type": "object",
      "properties": {
        "orderId": {"type": "string"},
        "customerId": {"type": "string"},
        "items": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "productId": {"type": "string"},
              "quantity": {"type": "integer"}
            }
          }
        },
        "total": {"type": "number"},
        "createdAt": {"type": "string", "format": "date-time"}
      },
      "required": ["orderId", "customerId", "items", "total"]
    }
  }'
```

### 查询消息定义

```bash
# 列出所有消息
curl https://registry.example.com/messages

# 获取特定消息
curl https://registry.example.com/messages/order.created

# 获取特定版本
curl https://registry.example.com/messages/order.created/versions/1.0.0

# 搜索消息
curl "https://registry.example.com/messages?filter=name.contains('order')"
```

---

## 高级功能

### CloudEvents 消息注册

```json
// POST /messages
{
  "id": "com.example.orders.created",
  "name": "Order Created",
  "description": "Event emitted when an order is successfully created",
  "format": "CloudEvents",
  
  "cloudevents": {
    "type": "com.example.orders.created",
    "source": "/orders/service",
    "datacontenttype": "application/json",
    "dataschema": "https://registry.example.com/schemas/order-created-data/versions/1.0.0"
  },
  
  "schemaFormat": "application/cloudevents+json; charset=utf-8",
  "schema": {
    "specversion": "1.0",
    "type": "com.example.orders.created",
    "datacontenttype": "application/json",
    "data": {
      "$ref": "#/definitions/OrderCreatedData"
    },
    "definitions": {
      "OrderCreatedData": {
        "type": "object",
        "properties": {
          "orderId": {"type": "string"},
          "total": {"type": "number"}
        }
      }
    }
  }
}
```

### AsyncAPI 集成

```yaml
# POST /messages (AsyncAPI 格式)
id: user-signed-up
name: User Signed Up
format: AsyncAPI
schemaFormat: application/vnd.aai.asyncapi+yaml;version=2.6.0
schema: |
  asyncapi: '2.6.0'
  info:
    title: User Service
    version: '1.0.0'
  channels:
    user/signedup:
      subscribe:
        message:
          payload:
            type: object
            properties:
              userId:
                type: string
              email:
                type: string
                format: email
              signedUpAt:
                type: string
                format: date-time
```

### Schema 注册

```json
// POST /schemas
{
  "id": "order-created-data",
  "name": "Order Created Data Schema",
  "format": "JsonSchema",
  "content": {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "https://registry.example.com/schemas/order-created-data",
    "title": "Order Created Data",
    "type": "object",
    "properties": {
      "orderId": {
        "type": "string",
        "description": "Unique order identifier"
      },
      "customerId": {
        "type": "string",
        "description": "Customer who placed the order"
      },
      "items": {
        "type": "array",
        "items": {
          "$ref": "#/definitions/OrderItem"
        }
      },
      "total": {
        "type": "number",
        "minimum": 0
      },
      "currency": {
        "type": "string",
        "pattern": "^[A-Z]{3}$"
      }
    },
    "required": ["orderId", "customerId", "items", "total"],
    "definitions": {
      "OrderItem": {
        "type": "object",
        "properties": {
          "productId": {"type": "string"},
          "quantity": {"type": "integer", "minimum": 1},
          "price": {"type": "number", "minimum": 0}
        },
        "required": ["productId", "quantity", "price"]
      }
    }
  }
}
```

### 端点注册

```json
// POST /endpoints
{
  "id": "orders-kafka-producer",
  "name": "Orders Service Kafka Producer",
  "description": "Kafka endpoint for order events",
  "protocol": "Kafka",
  "url": "kafka://kafka.example.com:9092/orders-topic",
  "usage": "producer",
  
  "config": {
    "topic": "orders",
    "partitions": 12,
    "replicationFactor": 3
  },
  
  "messages": [
    "order.created",
    "order.updated", 
    "order.cancelled"
  ],
  
  "authentication": {
    "type": "SASL_SSL",
    "mechanism": "SCRAM-SHA-512"
  }
}
```

---

## 版本管理

### 创建新版本

```bash
# 创建消息的新版本
curl -X POST https://registry.example.com/messages/order.created/versions \
  -H "Content-Type: application/json" \
  -d '{
    "id": "2.0.0",
    "schema": {
      "type": "object",
      "properties": {
        "orderId": {"type": "string"},
        "customerId": {"type": "string"},
        "items": {"type": "array"},
        "total": {"type": "number"},
        "discount": {"type": "number"},
        "createdAt": {"type": "string", "format": "date-time"},
        "metadata": {"type": "object"}
      },
      "required": ["orderId", "customerId", "items", "total"]
    }
  }'
```

### 版本兼容性检查

```bash
# 检查新版本与旧版本的兼容性
curl -X POST https://registry.example.com/messages/order.created/versions/2.0.0/compatibility \
  -H "Content-Type: application/json" \
  -d '{
    "targetVersion": "1.0.0",
    "compatibilityMode": "BACKWARD"
  }'

# 响应:
{
  "compatible": true,
  "issues": [],
  "mode": "BACKWARD"
}
```

### 版本策略

```json
// 设置版本兼容性策略
// PUT /messages/order.created/config
{
  "compatibility": {
    "mode": "BACKWARD_TRANSITIVE",
    "allowIncompatible": false
  },
  "retention": {
    "maxVersions": 10,
    "maxAge": "P365D"
  },
  "validation": {
    "required": true,
    "strict": false
  }
}
```

---

## API 参考

### 核心端点

```
Registry API
├── Groups
│   ├── GET    /groups                    - 列出所有组
│   ├── GET    /groups/{group}            - 获取组详情
│   └── PUT    /groups/{group}            - 更新组配置
│
├── Resources
│   ├── GET    /groups/{group}/resources              - 列出资源
│   ├── POST   /groups/{group}/resources              - 创建资源
│   ├── GET    /groups/{group}/resources/{id}         - 获取资源
│   ├── PUT    /groups/{group}/resources/{id}         - 更新资源
│   └── DELETE /groups/{group}/resources/{id}         - 删除资源
│
├── Versions
│   ├── GET    /{group}/{id}/versions                 - 列出版本
│   ├── POST   /{group}/{id}/versions                 - 创建版本
│   ├── GET    /{group}/{id}/versions/{version}       - 获取版本
│   └── DELETE /{group}/{id}/versions/{version}       - 删除版本
│
└── Discovery
    ├── GET    /discovery                             - 服务发现
    └── GET    /export                                - 导出注册数据
```

### 查询过滤

```bash
# 分页
GET /messages?offset=0&limit=100

# 过滤
GET /messages?filter=format=='CloudEvents'
GET /messages?filter=name.contains('order')
GET /messages?filter=createdAt>='2026-01-01'

# 排序
GET /messages?orderby=name,asc
GET /messages?orderby=createdAt,desc

# 字段选择
GET /messages?select=id,name,format
GET /messages?expand=versions
```

---

## 集成示例

### 生产者集成

```java
// Java 示例 - 使用 xRegistry 获取 Schema
public class OrderProducer {
    private final XRegistryClient registry;
    private final KafkaProducer<String, byte[]> producer;
    
    public void sendOrderCreated(Order order) {
        // 从注册中心获取 Schema
        MessageDefinition messageDef = registry
            .getMessage("order.created")
            .getLatestVersion();
        
        // 使用 Schema 验证数据
        JsonSchema schema = messageDef.getSchema();
        ValidationResult result = schema.validate(order);
        
        if (!result.isValid()) {
            throw new ValidationException(result.getErrors());
        }
        
        // 构建 CloudEvent
        CloudEvent event = CloudEventBuilder.v1()
            .withId(UUID.randomUUID().toString())
            .withType("com.example.orders.created")
            .withSource(URI.create("/orders/service"))
            .withDataSchema(messageDef.getSchemaUrl())
            .withData("application/json", serialize(order))
            .build();
        
        // 发送消息
        producer.send(new ProducerRecord<>("orders", event));
    }
}
```

### 消费者集成

```python
# Python 示例 - 使用 xRegistry 验证消息
from xregistry import XRegistryClient
from cloudevents.http import from_http
import jsonschema

class OrderConsumer:
    def __init__(self, registry_url: str):
        self.registry = XRegistryClient(registry_url)
        self.schemas = {}
    
    def get_schema(self, event_type: str):
        if event_type not in self.schemas:
            message = self.registry.get_message(event_type)
            self.schemas[event_type] = message.schema
        return self.schemas[event_type]
    
    def handle_event(self, event: CloudEvent):
        # 获取对应的 Schema
        schema = self.get_schema(event['type'])
        
        # 验证数据
        try:
            jsonschema.validate(event.data, schema)
        except jsonschema.ValidationError as e:
            logger.error(f"Invalid event data: {e}")
            return
        
        # 处理事件
        if event['type'] == 'com.example.orders.created':
            self.process_order_created(event.data)
```

### CI/CD 集成

```yaml
# GitHub Actions - Schema 兼容性检查
name: Schema Compatibility Check

on:
  pull_request:
    paths:
      - 'schemas/**'

jobs:
  check-compatibility:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Check Schema Compatibility
        run: |
          for schema in schemas/*.json; do
            SCHEMA_ID=$(basename "$schema" .json)
            
            # 检查与生产版本的兼容性
            curl -X POST "$REGISTRY_URL/schemas/$SCHEMA_ID/compatibility" \
              -H "Content-Type: application/json" \
              -d "{\"schema\": $(cat $schema)}" \
              --fail
          done
        env:
          REGISTRY_URL: ${{ secrets.XREGISTRY_URL }}
```

---

## 最佳实践

### 命名约定

```yaml
# 消息命名: {domain}.{entity}.{action}
messages:
  - com.example.orders.created
  - com.example.orders.updated
  - com.example.orders.cancelled
  - com.example.inventory.reserved
  - com.example.payments.completed

# Schema 命名: {entity}-{context}-{version}
schemas:
  - order-created-data-v1
  - order-item-schema
  - customer-profile-schema

# 端点命名: {service}-{protocol}-{role}
endpoints:
  - orders-kafka-producer
  - payments-http-consumer
  - inventory-amqp-both
```

### 组织结构

```
Registry Organization
├── Domain: com.example.orders
│   ├── Messages
│   │   ├── orders.created
│   │   ├── orders.updated
│   │   └── orders.cancelled
│   ├── Schemas
│   │   ├── order-data
│   │   └── order-item
│   └── Endpoints
│       ├── orders-kafka
│       └── orders-http
│
├── Domain: com.example.payments
│   ├── Messages
│   │   ├── payments.initiated
│   │   └── payments.completed
│   └── Schemas
│       └── payment-data
│
└── Domain: com.example.inventory
    ├── Messages
    │   ├── inventory.reserved
    │   └── inventory.released
    └── Schemas
        └── inventory-item
```

---

## 参考资源

- [GitHub 仓库](https://github.com/xregistry/spec)
- [规范文档](https://xregistry.io/spec/)
- [CloudEvents 规范](https://cloudevents.io/)
- [AsyncAPI 规范](https://www.asyncapi.com/)
- [JSON Schema](https://json-schema.org/)
- [CNCF Sandbox](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

---
title: Tremor
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- kafka
- elasticsearch
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Tremor 是什么
- 如何 Tremor
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Tremor
- cncf
- landscape
---

# Tremor

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.tremor.rs/ |
| **GitHub** | https://github.com/tremor-rs/tremor-runtime |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Tremor 是一个高性能的事件处理引擎，专为处理大规模数据流（日志、指标、追踪数据）而设计。它由 Wayfair 开源，用 Rust 实现，通过自定义的查询语言（Troy/Trickle）定义数据管道，支持背压处理、有保证的交付和复杂事件处理。

### 核心特性

- **高性能**: Rust 实现，单实例处理 10GB+/s 的数据吞吐
- **自定义查询语言**: Troy (管道定义) 和 Trickle (数据转换) DSL
- **背压管理**: 内置背压传播机制，防止下游过载
- **有保证的交付**: 支持 at-least-once 投递语义
- **多协议连接器**: Kafka, Elasticsearch, S3, TCP, UDP, WebSocket, gRPC
- **流量整形**: 批处理、速率限制和负载均衡
- **低延迟**: 微秒级事件处理延迟

---

## 快速开始

### 安装

```bash
# Docker 运行
docker run -d --name tremor \
  -v $(pwd)/config:/etc/tremor \
  tremorproject/tremor:latest

# 从源码编译
cargo install tremor-cli
```

### 数据管道定义

```troy
# config.troy - 日志聚合管道
define flow main
flow
  use std::connectors;
  use std::pipeline;

  # 定义 Kafka 输入
  define connector kafka_in from connectors::kafka_consumer
  with
    codec = "json",
    config = {
      "brokers": ["kafka:9092"],
      "group_id": "tremor-consumer",
      "topics": ["application-logs"]
    }
  end;

  # 定义 Elasticsearch 输出
  define connector elastic_out from connectors::elastic
  with
    config = {
      "nodes": ["http://elasticsearch:9200"],
      "index": "logs-{YYYY}.{MM}.{DD}"
    }
  end;

  # 定义处理管道
  define pipeline process
  pipeline
    # 过滤 debug 日志
    select event from in
    where event.level != "debug"
    into out;

    # 添加处理时间戳
    select {
      ...event,
      "processed_at": system::nanotime()
    }
    from out into out;
  end;

  # 连接组件
  create connector kafka_in;
  create connector elastic_out;
  create pipeline process;

  connect /connector/kafka_in to /pipeline/process;
  connect /pipeline/process to /connector/elastic_out;
end;

deploy flow main;
```

---

## 高级功能

### 窗口聚合

```trickle
# 5 分钟窗口内的指标聚合
define tumbling window five_min
with
  interval = datetime::with_minutes(5)
end;

select {
  "metric": event.name,
  "avg": aggr::stats::mean(event.value),
  "max": aggr::stats::max(event.value),
  "min": aggr::stats::min(event.value),
  "count": aggr::stats::count()
}
from in[five_min]
group by event.name
into out;
```

### 流量控制

```troy
# 速率限制和批处理
define pipeline rate_limited
pipeline
  # 批处理 - 每 1000 条或每 5 秒
  define operator batch from std::operator::batch
  with
    count = 1000,
    timeout = 5000000000  # 5 seconds in nanoseconds
  end;

  create operator batch;
  connect /in to /operator/batch;
  connect /operator/batch to /out;
end;
```

---

## 最佳实践

1. **背压设计**: 利用 Tremor 的背压机制保护下游系统
2. **窗口聚合**: 使用窗口函数在 Tremor 层做预聚合，减少下游负载
3. **批处理**: 对 Elasticsearch/S3 等输出配置批处理提升吞吐
4. **过滤前移**: 尽早在管道中过滤不需要的数据减少处理量
5. **资源监控**: 监控 Tremor 的内存和 CPU 使用，调整管道并发度

---

## 参考资源

- [Tremor 官方文档](https://www.tremor.rs/docs/)
- [Tremor GitHub](https://github.com/tremor-rs/tremor-runtime)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

---
title: openGemini
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- flux
- postgresql
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
- openGemini 是什么
- 如何 openGemini
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- openGemini
- cncf
- landscape
---

# openGemini

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://opengemini.org/ |
| **GitHub** | https://github.com/openGemini/openGemini |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

openGemini 是一个高性能、分布式时序数据库，专为物联网 (IoT)、可观测性和工业互联网场景设计。它基于 InfluxDB 协议兼容，提供高速写入、低延迟查询和高效压缩，支持每秒千万级数据点的写入和 PB 级数据存储。openGemini 采用存算分离架构，可独立扩展计算和存储资源。

### 核心特性

- **高性能写入**: 单节点支持百万级 TPS，集群支持千万级写入
- **高效压缩**: 针对时序数据优化的压缩算法，压缩比高达 10:1
- **InfluxDB 兼容**: 兼容 InfluxDB Line Protocol 和 InfluxQL
- **存算分离**: 计算节点和存储节点独立扩展
- **高可用**: 数据多副本，支持节点故障自动恢复
- **丰富聚合**: 内置大量时序分析函数（移动平均、导数、积分等）

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                 openGemini Cluster                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │                ts-meta (元数据)                │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐     │    │
│  │  │ Meta 1   │ │ Meta 2   │ │ Meta 3   │     │    │
│  │  │ (Raft)   │ │ (Raft)   │ │ (Raft)   │     │    │
│  │  └──────────┘ └──────────┘ └──────────┘     │    │
│  └──────────────────────────────────────────────┘    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │                ts-sql (查询层)                 │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐     │    │
│  │  │ SQL 1    │ │ SQL 2    │ │ SQL 3    │     │    │
│  │  │(无状态)  │ │(无状态)  │ │(无状态)  │     │    │
│  │  └──────────┘ └──────────┘ └──────────┘     │    │
│  └──────────────────────────────────────────────┘    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │               ts-store (存储层)               │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐     │    │
│  │  │ Store 1  │ │ Store 2  │ │ Store 3  │     │    │
│  │  │ (Shard)  │ │ (Shard)  │ │ (Shard)  │     │    │
│  │  └──────────┘ └──────────┘ └──────────┘     │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 单机部署

```bash
# 下载
wget https://github.com/openGemini/openGemini/releases/latest/download/openGemini-linux-amd64.tar.gz
tar -xzf openGemini-linux-amd64.tar.gz
cd openGemini

# 启动单机版
./ts-server --config openGemini.singlenode.conf

# 验证服务
curl -i "http://localhost:8086/ping"
```

### Kubernetes 部署

```bash
# 使用 Helm 安装
helm repo add opengemini https://opengemini.github.io/helm-charts
helm install opengemini opengemini/opengemini \
  --namespace opengemini \
  --create-namespace \
  --set meta.replicaCount=3 \
  --set store.replicaCount=3
```

### 创建数据库

```bash
# 使用 CLI
./ts-cli

> CREATE DATABASE iot_metrics
> USE iot_metrics
> SHOW DATABASES
```

### 写入数据

```bash
# Line Protocol 写入
curl -i -XPOST "http://localhost:8086/write?db=iot_metrics" \
  --data-binary '
temperature,device=sensor01,location=room1 value=25.3 1699000000000000000
temperature,device=sensor02,location=room2 value=24.8 1699000000000000000
humidity,device=sensor01,location=room1 value=65.5 1699000000000000000
'
```

### 查询数据

```sql
-- 查询最近 1 小时的温度
SELECT mean(value) 
FROM temperature 
WHERE time > now() - 1h 
GROUP BY time(5m), device

-- 查询特定设备
SELECT * 
FROM temperature 
WHERE device = 'sensor01' 
ORDER BY time DESC 
LIMIT 100
```

---

## 高级功能

### 连续查询 (Continuous Query)

```sql
-- 创建连续查询，每分钟聚合数据
CREATE CONTINUOUS QUERY cq_temperature_1m ON iot_metrics
BEGIN
  SELECT mean(value) AS mean_temp, max(value) AS max_temp
  INTO temperature_1m
  FROM temperature
  GROUP BY time(1m), device
END
```

### 保留策略 (Retention Policy)

```sql
-- 创建保留策略
CREATE RETENTION POLICY "7days" ON iot_metrics 
  DURATION 7d REPLICATION 2 DEFAULT

-- 创建长期存储策略
CREATE RETENTION POLICY "1year" ON iot_metrics 
  DURATION 365d REPLICATION 2

-- 将聚合数据写入长期存储
SELECT mean(value) INTO "1year".temperature_hourly
FROM temperature
GROUP BY time(1h), device
```

### 高级聚合函数

```sql
-- 移动平均
SELECT moving_average(value, 5) 
FROM temperature 
WHERE time > now() - 1h

-- 导数计算
SELECT derivative(value, 1m) 
FROM counter_metric 
WHERE time > now() - 1h

-- 差值计算
SELECT difference(value) 
FROM cumulative_metric 
WHERE time > now() - 1h

-- 累计求和
SELECT cumulative_sum(value) 
FROM rate_metric 
WHERE time > now() - 1d
```

### 分布式集群配置

```toml
# openGemini.conf
[meta]
  bind-address = ":8091"
  http-bind-address = ":8091"
  join = ["meta1:8091", "meta2:8091", "meta3:8091"]

[data]
  store-ingest-addr = ":8400"
  store-select-addr = ":8401"
  
[coordinator]
  write-timeout = "10s"
  shard-writer-timeout = "10s"

[retention]
  enabled = true
  check-interval = "30m"
```

---

## 与其他方案对比

| 特性 | openGemini | InfluxDB | TimescaleDB | QuestDB |
|:---|:---|:---|:---|:---|
| 协议 | InfluxDB 兼容 | 原生 | PostgreSQL | InfluxDB/PG |
| 架构 | 分布式 | 单机/商业集群 | PostgreSQL 扩展 | 单机/集群 |
| 写入性能 | 千万 TPS | 百万 TPS | 百万 TPS | 百万 TPS |
| 压缩比 | 10:1 | 8:1 | 5:1 | 10:1 |
| 存算分离 | 原生支持 | 不支持 | 不支持 | 不支持 |
| 开源 | 完全开源 | 核心开源 | 完全开源 | 核心开源 |

---

## 最佳实践

1. **合理分片**: 根据数据量设置合适的 Shard Duration，避免单 Shard 过大
2. **标签设计**: Tag 用于高基数维度，Field 用于数值，避免高基数 Tag
3. **保留策略**: 为不同精度的数据设置不同保留策略，自动降采样
4. **批量写入**: 使用批量写入而非单点写入，提高写入效率
5. **查询优化**: 查询时指定时间范围和 Tag 过滤，避免全表扫描

---

## 参考资源

- [openGemini 官方文档](https://docs.opengemini.org/)
- [openGemini GitHub](https://github.com/openGemini/openGemini)
- [InfluxDB Line Protocol](https://docs.influxdata.com/influxdb/v1/write_protocols/line_protocol_tutorial/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

# TiKV

> **成熟度**: Graduated | **加入时间**: 2018-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://tikv.org |
| **GitHub** | https://github.com/tikv/tikv |
| **文档** | https://tikv.org/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Rust |
| **CNCF 分类** | Database |

---

## 项目概述

### 简介
TiKV 是一个分布式事务键值数据库，由 PingCAP 开发，是 TiDB 的存储层。

### 核心定位
TiKV 提供分布式事务、水平扩展、强一致性，适合需要事务支持的分布式键值存储场景。是构建分布式数据库系统的理想存储引擎。

### 发展历程
- **2016**: PingCAP 开始开发 TiKV
- **2017**: 开源发布
- **2018-08**: 加入 CNCF 孵化项目
- **2020-09**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **分布式事务**: 完整的 ACID 事务支持，基于 Percolator 模型
- **水平扩展**: 自动数据分片（Region）和负载均衡
- **高可用**: Multi-Raft 共识协议，自动故障转移
- **强一致性**: 线性一致读写，支持快照隔离
- **协处理器**: 下推计算能力，减少数据传输
- **RawKV/TxnKV**: 支持原始 KV 和事务 KV 两种 API

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                        TiKV Cluster                         │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │   TiKV Node 1   │ │   TiKV Node 2   │ │  TiKV Node 3  │ │
│  │  ┌───────────┐  │ │  ┌───────────┐  │ │ ┌───────────┐ │ │
│  │  │  Region   │  │ │  │  Region   │  │ │ │  Region   │ │ │
│  │  │  (Raft)   │◄─┼─┼─►│  (Raft)   │◄─┼─┼►│  (Raft)   │ │ │
│  │  └───────────┘  │ │  └───────────┘  │ │ └───────────┘ │ │
│  │  ┌───────────┐  │ │  ┌───────────┐  │ │ ┌───────────┐ │ │
│  │  │  RocksDB  │  │ │  │  RocksDB  │  │ │ │  RocksDB  │ │ │
│  │  └───────────┘  │ │  └───────────┘  │ │ └───────────┘ │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     Placement Driver (PD)                    │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │  Metadata   │ │  Scheduler  │ │    Timestamp Oracle     ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| TiKV Server | 存储节点 | 处理读写请求，管理 Region |
| PD (Placement Driver) | 集群管理 | 元数据管理、调度、TSO |
| Region | 数据分片 | 数据的基本单位，默认 96MB |
| Raft | 共识协议 | 保证数据一致性和高可用 |
| RocksDB | 存储引擎 | 底层键值存储 |

### 工作原理
1. 数据按 Key 范围划分为 Region
2. 每个 Region 由多个副本组成 Raft Group
3. PD 负责 Region 的调度和负载均衡
4. 事务通过两阶段提交（2PC）保证原子性
5. 使用 MVCC 实现快照隔离

---

## 使用场景

### 典型应用
- **分布式数据库后端**: TiDB 的存储层
- **元数据存储**: 存储海量元数据
- **消息队列**: 持久化消息存储
- **缓存系统**: 替代 Redis 作为持久化缓存

### 适用条件
- 需要分布式事务支持
- 需要水平扩展能力
- 需要强一致性保证
- 数据量大，单机无法承载

### 不适用场景
- 对延迟极其敏感的场景
- 简单的缓存需求
- 不需要事务的场景

---

## 快速开始

### 安装部署
```bash
# 使用 TiUP 部署（推荐）
curl --proto '=https' --tlsv1.2 -sSf https://tiup-mirrors.pingcap.com/install.sh | sh
tiup playground --mode tikv-slim

# Docker 运行
docker run -d --name tikv -p 20160:20160 pingcap/tikv:latest

# Kubernetes 部署（使用 TiDB Operator）
kubectl apply -f https://raw.githubusercontent.com/pingcap/tidb-operator/master/manifests/crd.yaml
```

### 基础配置
```toml
# tikv.toml
[server]
addr = "0.0.0.0:20160"
grpc-concurrency = 4

[storage]
data-dir = "/var/lib/tikv"
reserve-space = "0"

[raftstore]
region-split-size = "96MB"
region-max-size = "144MB"

[rocksdb]
max-background-jobs = 8
```

### 验证测试
```bash
# 使用 tikv-ctl
tikv-ctl --host 127.0.0.1:20160 metrics

# 使用客户端 SDK
# Go 示例
import "github.com/tikv/client-go/v2/txnkv"
```

---

## 最佳实践

### 生产环境建议
- 至少部署 3 个 TiKV 节点
- PD 部署 3 个节点保证高可用
- 使用 SSD 存储
- 配置合理的 Region 大小

### 性能优化
- 开启 Titan（大 Value 优化）
- 调整 RocksDB 参数
- 配置合适的 gRPC 并发数
- 监控热点 Region

### 安全加固
- 启用 TLS 加密
- 配置认证授权
- 审计日志
- 网络隔离

---

## 生态集成

### 相关 CNCF 项目
- **TiDB**: 分布式 SQL 数据库（使用 TiKV 作为存储）
- **Prometheus**: 监控指标
- **Grafana**: 可视化面板

### 常见集成方案
- TiDB + TiKV 分布式数据库
- TiKV + Prometheus 监控
- TiKV + Kubernetes 云原生部署

---

## 社区与支持

### 社区资源
- Slack: https://slack.tidb.io
- 论坛: https://ask.pingcap.com
- GitHub Discussions

### 贡献指南
访问 https://github.com/tikv/tikv/blob/master/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://tikv.org/docs)
- [GitHub Repo](https://github.com/tikv/tikv)
- [CNCF 项目页面](https://www.cncf.io/projects/tikv/)
- [TiKV 深度解析](https://tikv.org/deep-dive/)

---

**维护者**: Kudig Team | **许可证**: MIT

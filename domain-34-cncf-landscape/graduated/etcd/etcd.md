# etcd

> **成熟度**: Graduated | **加入时间**: 2018-12 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://etcd.io |
| **GitHub** | https://github.com/etcd-io/etcd |
| **文档** | https://etcd.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Database |

---

## 项目概述

### 简介
etcd 是一个分布式、高可用的键值存储系统，用于共享配置和服务发现，是 Kubernetes 的核心数据存储。

### 核心定位
etcd 提供强一致性的分布式键值存储，通过 Raft 共识算法保证数据可靠性，是 Kubernetes 集群状态的唯一真实来源（Source of Truth）。

### 发展历程
- **2013-08**: CoreOS 发布 etcd
- **2014**: Kubernetes 选择 etcd 作为数据存储
- **2018-12**: 加入 CNCF 作为孵化项目
- **2020-11**: 成为 CNCF 毕业项目
- **2024**: etcd v3.5+ 持续演进

---

## 核心功能

### 主要特性
- **强一致性**: 基于 Raft 共识算法
- **高可用**: 支持多节点集群
- **Watch 机制**: 监听键值变化
- **事务支持**: 原子性的多键操作
- **租约机制**: 带 TTL 的键值
- **版本控制**: MVCC 多版本并发控制

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                       etcd Cluster                          │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │    etcd Node 1  │ │    etcd Node 2  │ │  etcd Node 3  │ │
│  │    (Leader)     │ │   (Follower)    │ │  (Follower)   │ │
│  │  ┌───────────┐  │ │  ┌───────────┐  │ │ ┌───────────┐ │ │
│  │  │   Raft    │◄─┼─┼─►│   Raft    │◄─┼─┼►│   Raft    │ │ │
│  │  └───────────┘  │ │  └───────────┘  │ │ └───────────┘ │ │
│  │  ┌───────────┐  │ │  ┌───────────┐  │ │ ┌───────────┐ │ │
│  │  │   Store   │  │ │  │   Store   │  │ │ │   Store   │ │ │
│  │  │  (bbolt)  │  │ │  │  (bbolt)  │  │ │ │  (bbolt)  │ │ │
│  │  └───────────┘  │ │  └───────────┘  │ │ └───────────┘ │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
etcd 采用 Leader-Follower 架构，通过 Raft 协议选举 Leader 处理写请求，所有节点可处理读请求，使用 bbolt 作为底层存储引擎。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Raft | 共识模块 | 实现分布式一致性 |
| Store | 存储引擎 | bbolt 键值存储 |
| gRPC Server | API 服务 | 客户端通信接口 |
| WAL | 预写日志 | 保证数据持久性 |
| Snapshot | 快照 | 数据备份和恢复 |

### 工作原理
1. 客户端向 Leader 发送写请求
2. Leader 将日志条目复制到 Follower
3. 多数节点确认后提交日志
4. Leader 响应客户端写入成功
5. 读请求可由任意节点处理

---

## 使用场景

### 典型应用
- **Kubernetes 数据存储**: 集群状态的唯一存储
- **服务发现**: 服务注册和发现
- **配置中心**: 分布式配置管理
- **分布式锁**: 实现分布式协调
- **Leader 选举**: 分布式系统选主

### 适用条件
- 需要强一致性的键值存储
- Kubernetes 集群数据存储
- 分布式系统协调场景
- 需要 Watch 机制的配置管理

### 不适用场景
- 大数据量存储（单集群 < 8GB）
- 高写入吞吐量场景
- 需要复杂查询的数据存储

---

## 快速开始

### 安装部署
```bash
# Docker 运行单节点
docker run -d --name etcd \
  -p 2379:2379 -p 2380:2380 \
  quay.io/coreos/etcd:v3.5.12 \
  /usr/local/bin/etcd \
  --advertise-client-urls http://0.0.0.0:2379 \
  --listen-client-urls http://0.0.0.0:2379

# 二进制安装
ETCD_VER=v3.5.12
curl -L https://github.com/etcd-io/etcd/releases/download/${ETCD_VER}/etcd-${ETCD_VER}-linux-amd64.tar.gz -o etcd.tar.gz
tar xzvf etcd.tar.gz
./etcd
```

### 基础配置
```yaml
# etcd.yaml (3节点集群)
name: etcd-1
data-dir: /var/lib/etcd
initial-advertise-peer-urls: http://etcd-1:2380
listen-peer-urls: http://0.0.0.0:2380
advertise-client-urls: http://etcd-1:2379
listen-client-urls: http://0.0.0.0:2379
initial-cluster: etcd-1=http://etcd-1:2380,etcd-2=http://etcd-2:2380,etcd-3=http://etcd-3:2380
initial-cluster-state: new
initial-cluster-token: etcd-cluster-token
```

### 验证测试
```bash
# 健康检查
etcdctl endpoint health

# 写入数据
etcdctl put /key1 value1

# 读取数据
etcdctl get /key1

# 监听变化
etcdctl watch /key1

# 查看成员
etcdctl member list
```

---

## 最佳实践

### 生产环境建议
- 使用奇数节点（3 或 5 节点）
- 配置专用 SSD 存储
- 定期备份数据
- 监控集群健康状态

### 性能优化
- 使用 SSD 存储 WAL 和数据
- 合理设置配额限制
- 定期压缩历史版本
- 网络延迟优化

### 安全加固
- 启用 TLS 加密通信
- 配置客户端证书认证
- 加密静态数据
- 限制网络访问

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: 使用 etcd 存储集群状态
- **CoreDNS**: 可选 etcd 后端
- **Rook**: etcd Operator 管理

### 常见集成方案
- Kubernetes + etcd 集群
- etcd + Prometheus 监控
- etcd + Grafana 可视化
- etcd Operator 自动化管理

---

## 社区与支持

### 社区资源
- Slack: https://slack.k8s.io #etcd
- 邮件列表: etcd-dev@googlegroups.com
- Google Groups: https://groups.google.com/g/etcd-dev

### 贡献指南
访问 https://github.com/etcd-io/etcd/blob/main/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://etcd.io/docs)
- [GitHub Repo](https://github.com/etcd-io/etcd)
- [CNCF 项目页面](https://www.cncf.io/projects/etcd/)
- [etcd 运维指南](https://etcd.io/docs/v3.5/op-guide/)

---

**维护者**: Kudig Team | **许可证**: MIT

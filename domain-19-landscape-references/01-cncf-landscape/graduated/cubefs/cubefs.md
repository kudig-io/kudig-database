---
title: CubeFS
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- helm
- docker
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CubeFS 是什么
- 如何 CubeFS
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- CubeFS
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
---

title: CubeFS
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- docker
- gateway
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
- CubeFS 是什么
- 如何 CubeFS
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CubeFS
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
# CubeFS

> **成熟度**: Graduated | **加入时间**: 2022-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cubefs.io |
| **GitHub** | https://github.com/cubefs/cubefs |
| **文档** | https://cubefs.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Storage |

---

## 项目概述

### 简介
CubeFS（原 ChubaoFS）是云原生分布式存储系统，支持文件、对象和块存储，由京东开发并开源。

### 核心定位
CubeFS 为云原生应用提供高性能、可扩展的统一存储解决方案，特别适合大规模数据密集型应用和 AI/ML 工作负载。

### 发展历程
- **2018**: 京东内部开发 ChubaoFS
- **2019**: 开源发布
- **2021**: 更名为 CubeFS
- **2022-07**: 加入 CNCF 孵化项目
- **2024-07**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **多协议支持**: POSIX、S3、HDFS 接口兼容
- **弹性扩展**: 元数据和数据节点独立扩展
- **多租户**: 资源隔离、配额管理
- **纠删码**: 高效存储空间利用
- **多级缓存**: 本地缓存加速
- **AI/ML 优化**: 大规模数据集处理优化

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                         Clients                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │   POSIX     │ │     S3      │ │         HDFS            ││
│  │   Client    │ │   Gateway   │ │        Client           ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                        CubeFS Cluster                       │
│  ┌─────────────────┐ ┌─────────────────┐                   │
│  │   Master Node   │ │   Master Node   │  (HA Cluster)     │
│  └─────────────────┘ └─────────────────┘                   │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │ Meta Node       │ │ Meta Node       │ │  Meta Node    │ │
│  │ (Metadata)      │ │ (Metadata)      │ │  (Metadata)   │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │ Data Node       │ │ Data Node       │ │  Data Node    │ │
│  │ (Replica/EC)    │ │ (Replica/EC)    │ │  (Replica/EC) │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Master | 集群管理 | 管理元数据节点和数据节点 |
| MetaNode | 元数据存储 | 存储文件系统元数据 |
| DataNode | 数据存储 | 存储文件数据（副本/纠删码） |
| ObjectNode | S3 网关 | 提供 S3 兼容接口 |
| Client | 客户端 | POSIX/FUSE 客户端 |

### 工作原理
1. Client 向 Master 请求 Volume 信息
2. Master 返回 MetaNode 和 DataNode 位置
3. Client 直接与 MetaNode 交互元数据操作
4. Client 直接与 DataNode 进行数据读写
5. 支持副本和纠删码两种数据保护方式

---

## 使用场景

### 典型应用
- **AI/ML 训练**: 大规模数据集存储
- **大数据分析**: Spark/Flink 数据湖
- **容器持久化**: Kubernetes CSI 存储
- **日志存储**: 海量日志数据存储
- **混合云存储**: 多云数据共享

### 适用条件
- 需要统一存储接口
- 大规模数据存储需求
- 需要弹性扩展能力
- AI/ML 工作负载

### 不适用场景
- 小规模简单存储需求
- 对延迟极其敏感的 OLTP 场景

---

## 快速开始

### 安装部署
```bash
# Docker Compose 快速体验
git clone https://github.com/cubefs/cubefs.git
cd cubefs/docker
docker-compose up -d

# Kubernetes 部署（使用 Helm）
helm repo add cubefs https://cubefs.github.io/helm-charts
helm install cubefs cubefs/cubefs
```

### 基础配置
```json
{
  "clusterName": "cubefs-cluster",
  "id": "1",
  "role": "master",
  "ip": "192.168.1.1",
  "listen": "17010",
  "prof": "17020",
  "retainLogs": "2000",
  "peers": "1:192.168.1.1:17010,2:192.168.1.2:17010,3:192.168.1.3:17010"
}
```

### 验证测试
```bash
# 创建 Volume
cfs-cli volume create test-vol

# 挂载 Volume
cfs-client -c client.json

# 测试读写
dd if=/dev/zero of=/mnt/cubefs/testfile bs=1M count=100
```

---

## 最佳实践

### 生产环境建议
- 至少 3 个 Master 节点
- MetaNode 使用 SSD
- DataNode 可使用 HDD
- 配置合理的副本数或纠删码策略

### 性能优化
- 启用本地缓存
- 调整 Block Size
- 配置预读策略
- 监控热点分区

### 安全加固
- 启用认证授权
- 配置网络隔离
- 数据加密
- 审计日志

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: CSI 驱动
- **Prometheus**: 监控指标
- **Fluid**: 数据集编排

### 常见集成方案
- CubeFS + Kubernetes CSI
- CubeFS + Spark/Flink 大数据
- CubeFS + AI 训练平台

---

## 社区与支持

### 社区资源
- Slack: cubefs.slack.com
- 微信群: CubeFS 社区
- GitHub Discussions

### 贡献指南
访问 https://github.com/cubefs/cubefs/blob/master/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://cubefs.io/docs)
- [GitHub Repo](https://github.com/cubefs/cubefs)
- [CNCF 项目页面](https://www.cncf.io/projects/cubefs/)
- [架构设计文档](https://cubefs.io/docs/master/design/architecture.html)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]

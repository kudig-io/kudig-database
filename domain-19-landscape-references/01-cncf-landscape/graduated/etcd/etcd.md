---
title: etcd
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- prometheus
- grafana
- coredns
- docker
- rook
- operator
- apiserver
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- 如何 etcd
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- etcd
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
- ebpf-basics
- etcd-basics
---

title: etcd
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- grafana
- coredns
- docker
- rook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- 如何 etcd
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- etcd
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta.md
  label: '故障树: etcd'
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

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[hot.md|hot]]
- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[journal/digest-2026-05-21|Wiki Digest — Daily (2026-05-21)]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[references/kubectl-quick-reference|Kubectl Quick Reference]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[references/k8s-node-create|Kubernetes 节点管理操作指南]] — Cross-reference
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/etcd x 高可用模式|etcd × 高可用模式]] — Cross-reference
- [[synthesis/etcd × Operator 模式|etcd × Operator 模式]] — Cross-reference
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-infrastructure|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/declarative-api|Declarative API]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution|Kubernetes 版本演进]] — Cross-reference
- [[concepts/etcd Operational Reference|etcd Operational Reference]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/infrastructure-as-code|Infrastructure as Code]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/eventual-consistency|Eventual Consistency in Kubernetes]] — Cross-reference
- [[concepts/k8s-production-best-practices|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[concepts/Kubernetes Core Concepts|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/watch-mechanism|Watch Mechanism (List-Watch)]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-disaster-recovery-guide|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/node-drain-and-maintenance|节点驱逐与维护]] — Cross-reference
- [[skills/statefulset-fta|StatefulSet 异常故障树分析]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/skill-MOC|topic-skills MOC]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/11-etcd-deep-dive|etcd 深度解析]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/12-apiserver-deep-dive|kube-apiserver 深度解析]] — Cross-reference
- [[domain-01-cluster-fundamentals/98-merged-indexes/README-from-domain-01-cluster-fundamentals|Domain-3: Kubernetes控制平面]] — Cross-reference
- [[entities/kube-apiserver|kube-apiserver]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/backup-dr-index|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.2|etcd v0.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.5|etcd v3.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.0|etcd v2.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.1|etcd v3.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.1|etcd v2.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.0|etcd v3.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.3|etcd v0.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.4|etcd v3.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.2|etcd v2.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.3|etcd v3.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.4|etcd v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.1|etcd v0.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.6|etcd v3.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.3|etcd v2.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.2|etcd v3.2 Release Notes]]

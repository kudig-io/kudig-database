---
title: 阅读顺序指南
description: '# 阅读顺序指南'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- flannel
- ingress
- rbac
- rag
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- All kudig-database users
- ACK learners
- New joiners
estimated_read_time: 5min
intent_queries:
- kudig-database documentation reading order
- ACK learning path week by week
- Kubernetes knowledge learning sequence
- Inner training curriculum structure
- kudig-database document relationship
trigger_keywords:
- reading order
- learning path
- curriculum
- week
- day
- document relationship
- knowledge graph
- prerequisite
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- knowledge-map
- commands-cheatsheet
---

# 阅读顺序指南

> 按天排列的 kudig-database 文档阅读顺序，配合每日教案使用

---

## 使用说明

- 每天在开始教案前，先按顺序阅读对应的参考文档
- 文件路径均相对于 `inner-training/` 目录
- 标注 ⭐ 为核心必读，标注 📖 为补充阅读
- 建议每篇文档阅读时间 15-30 分钟

---

## Week 1: ACK/ACR 基础与集群生命周期

### Day 1: ACK ACR 管控 SR

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/200-ack-overview.md` | ACK 产品概览与架构 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/205-ack-cluster-types.md` | 集群类型对比 |
| 📖3 | `../../domain-12-cloud-providers/04-alicloud-ack/280-ack-acr-integration.md` | ACR 镜像服务集成 |

### Day 2: ACK SDK & API

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/290-ack-openapi.md` | ACK OpenAPI 规范 |
| 📖2 | `../../domain-12-cloud-providers/04-alicloud-ack/200-ack-overview.md` | API 端点与认证 |

### Day 3: ACK ACR 控制台 & 功能

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/200-ack-overview.md` | 控制台功能入口 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/280-ack-acr-integration.md` | ACR 控制台与镜像管理 |

### Day 4: K8S 新建集群

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md` | 集群创建流程 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md` | 网络 CIDR 规划 |

### Day 5: K8S 集群删除

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md` | 集群删除与资源清理 |

### Day 6: K8S 集群升级

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/220-ack-upgrade.md` | 集群升级策略 |
| 📖2 | `../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md` | 升级兼容性 |

### Day 7: K8S 集群证书

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/230-ack-certificate.md` | 证书管理与轮转 |
| 📖2 | `../../domain-05-security/01-authentication.md` | K8S 认证机制 |

---

## Week 2: 安全认证与监控运维

### Day 8: K8S 集群 RBAC

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-05-security/02-rbac.md` | RBAC 权限模型 |
| ⭐2 | `../../domain-05-security/01-authentication.md` | 认证与授权 |

### Day 9: RAM 账号管理

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md` | RAM 集成方案 |
| 📖2 | `../../domain-05-security/02-rbac.md` | RBAC 与 RAM 映射 |

### Day 10: ACK ACR K8S 漏洞

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-05-security/05-vulnerability.md` | 漏洞类型与防护 |
| 📖2 | `../../domain-05-security/04-pod-security.md` | Pod 安全标准 |

### Day 11: 风险点识别与防范

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-05-security/04-pod-security.md` | Pod Security Standards |
| ⭐2 | `../../domain-05-security/03-network-policy.md` | 网络安全策略 |

### Day 12: K8S 集群审计

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-05-security/06-audit.md` | 审计日志配置 |
| 📖2 | `../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md` | SLS 日志集成 |

### Day 13: K8S 集群监控

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-11-observability/01-monitoring-overview.md` | 监控体系概览 |
| ⭐2 | `../../domain-11-observability/02-prometheus.md` | Prometheus 部署与 PromQL |

### Day 14: K8S 集群配额 & License

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-08-resource-management/01-resource-quota.md` | ResourceQuota 配置 |
| ⭐2 | `../../domain-08-resource-management/02-limit-range.md` | LimitRange 配置 |

---

## Week 3: 节点与工作负载管理

### Day 15: Node 节点基础

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-03-node/01-node-overview.md` | 节点概念与状态 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md` | ACK 节点与 ECS |

### Day 16: Node 节点进阶

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-03-node/02-node-management.md` | 标签、污点、维护 |
| 📖2 | `../../domain-10-troubleshooting-diagnostics/05-node-troubleshooting.md` | 节点故障排查 |

### Day 17: 节点池基础

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md` | 节点池概念与创建 |

### Day 18: 节点池进阶

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-10-troubleshooting-diagnostics/28-cluster-autoscaler-troubleshooting.md` | 自动伸缩排障 |
| 📖2 | `../../domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md` | 伸缩组配置 |

### Day 19: Pod 容器组基础

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-09-workload/01-pod-overview.md` | Pod 概念与定义 |
| ⭐2 | `../../domain-09-workload/02-pod-lifecycle.md` | Pod 生命周期 |

### Day 20: Pod 容器组进阶

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-09-workload/05-pod-scheduling.md` | 调度策略 |
| ⭐2 | `../../domain-09-workload/06-pod-probes.md` | 健康探针 |

### Day 21: K8S 组件运维

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-04-control-plane/01-api-server.md` | 控制面组件 |
| ⭐2 | `../../domain-10-troubleshooting-diagnostics/01-troubleshooting-overview.md` | 故障排查思路 |

---

## Week 4: 网络与存储

### Day 22: Service 基础

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-06-service-networking/01-service-overview.md` | Service 类型与机制 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md` | ACK SLB 集成 |

### Day 23: Ingress

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-06-service-networking/03-ingress.md` | Ingress 规则与控制器 |
| 📖2 | `../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md` | ALB Ingress |

### Day 24: Terway 网络

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md` | Terway 架构与模式 |
| ⭐2 | `../../domain-06-service-networking/04-cni.md` | CNI 规范 |

### Day 25: Flannel 网络

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-06-service-networking/04-cni.md` | Flannel VxLAN |
| 📖2 | `../../domain-10-troubleshooting-diagnostics/10-network-troubleshooting.md` | 网络排查 |

### Day 26: 存储卷创建 & 删除

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-07-storage/01-storage-overview.md` | PV/PVC 概念 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/270-ack-storage.md` | ACK 存储集成 |

### Day 27: 存储卷挂载

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-07-storage/03-volume-types.md` | Volume 挂载类型 |
| 📖2 | `../../domain-07-storage/02-storage-class.md` | StorageClass 配置 |

### Day 28: 综合复习与实践

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | 回顾以上所有 ⭐ 文档 | 查漏补缺 |
| 📖2 | `../../domain-10-troubleshooting-diagnostics/01-troubleshooting-overview.md` | 综合排障 |

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]

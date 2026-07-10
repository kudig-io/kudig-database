---
title: Synthesis 跨域合成索引
summary: Synthesis 跨域合成索引
category: synthesis
tags:
- synthesis
- cross-domain
- moc
- index
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
relationships:
- target: '[[平台工程/运维/13-multi-cluster-management.md]]'
  type: uses
- target: '[[概念/Deployment × Secret 管理.md]]'
  type: uses
- target: '[[实体/etcd.md]]'
  type: uses
- target: '[[实体/kubernetes.md]]'
  type: uses
---



# Synthesis 跨域合成索引

> Synthesis 页面连接两个或多个 Domain 的核心概念，形成跨域知识网络。

---

## 按连接域分类

### 可观测性 × 其他域

- Cilium eBPF × 可观测性 — 网络可观测性
- CRD × 可观测性 — 自定义资源监控
- [[概念/observability-finops.md|可观测性 × FinOps]] — 成本驱动监控
- [[概念/multi-cluster-observability-federation.md|多集群可观测性联邦]] — 跨集群指标聚合
- [[概念/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 监控栈集成
- [[概念/chaos-drill-integration.md|混沌演练 × 可观测性]] — 故障注入监控
- [[概念/security-observability-correlation.md|安全 × 可观测性关联]] — 安全事件监控

### 平台工程 × 其他域

- GitOps × 平台工程 — 发布管理 × 平台工程
- [[概念/platform-engineering-sre.md|平台工程 × SRE]] — 平台可靠性
- [[概念/backstage-platform-catalog.md|Backstage × 平台目录]] — 开发者门户
- [[概念/finops-resource-governance.md|FinOps × 资源治理]] — 成本预算与配额
- [[概念/gitops-sre-release-gate.md|GitOps × SRE 发布门控]] — SLO 驱动发布

### 网络 × 安全

- CNI 插件 × NetworkPolicy — 网络安全策略
- eBPF × 运行时安全 — 容器安全监控
- [[概念/multi-cluster-security.md|多集群安全]] — 跨集群安全策略
- [[概念/service-mesh-security-governance.md|服务网格 × 安全治理]] — Istio 安全策略
- 纵深防御 × 供应链安全 — 供应链安全
- 服务网格 × 零信任安全 — mTLS + 零信任

### 存储 × 灾备

- [[概念/velero-disaster-recovery.md|Velero × 灾难恢复]] — 备份策略与恢复
- [[概念/data-protection-k8s.md|K8s 数据保护]] — 数据备份策略
- [[概念/cross-cloud-migration-playbook.md|跨云迁移手册]] — 多云数据迁移
- [[概念/edge-cloud-continuum.md|边缘-云连续体]] — 边缘存储

### AI/ML × 运维

- [[概念/ai-agent-ops-patterns.md|AI Agent × 运维自动化]] — 智能运维模式
- [[概念/ai-ml-observability.md|AI/ML × 可观测性]] — ML 模型监控

### 控制器模式 × 其他域

- [[概念/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 控制器与部署
- 控制器模式 × Operator 模式 — 控制器演进
- 控制器模式 × 可观测性 — 控制器监控
- 声明式 API × 控制器模式 — 声明式系统
- Operator 模式 × Pod 生命周期 — 生命周期管理
- Operator 模式 × 可观测性 — Operator 监控
- [[实体/etcd.md|etcd]] × Operator 模式 — etcd Operator

### 资源管理

- [[概念/cost-optimization-multi-cluster.md|多集群成本优化]] — 跨集群成本
- Pod 生命周期 × 存储模型 — 存储生命周期
- Pod 生命周期 × Secret 管理 — Secret 注入
- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]] — 部署与 Secret
- Secret 管理 × 存储模型 — Secret 存储
- IaC × [[平台工程/运维/13-multi-cluster-management.md|多集群管理]] — 基础设施即代码

### SRE/可靠性

- [[概念/slo-monitoring-integration.md|SLO × 监控集成]] — SLO 监控
- K8s 问题分布与 MTTR 基准 — 问题统计
- [[实体/kubernetes.md|Kubernetes]] Fault Distribution and MTTR.md|K8s 问题分布 (EN)]] — 英文版
- [[系统基础/知识字典/operations/production-troubleshooting-playbook.md|生产故障排查手册]] — 综合排查
- [[概念/Structural Troubleshooting Framework.md|结构化故障排查框架]] — 方法论

---

## 生产工单 Case Study

> 详见 Case Study 索引

---

## 统计

- **Synthesis 页面总数**: 50
- **Case Study 总数**: 9
- **覆盖 Domain 对**: 36+
- **平均页面长度**: 4,000+ 字
## Related

- [[概念/etcd × Operator 模式.md|etcd × Operator 模式]]

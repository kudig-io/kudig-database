---
title: Synthesis 跨域合成索引
category: synthesis
tags: [synthesis, cross-domain, moc, index]
created: "2026-05-23"
updated: "2026-05-23"
relationships:
  - target: "[[domain-07-platform-engineering/operate/13-multi-cluster-management]]"
    type: uses
  - target: "[[synthesis/Deployment × Secret 管理]]"
    type: uses
  - target: "[[entities/etcd]]"
    type: uses
  - target: "[[entities/kubernetes]]"
    type: uses
---

# Synthesis 跨域合成索引

> Synthesis 页面连接两个或多个 Domain 的核心概念，形成跨域知识网络。

---

## 按连接域分类

### 可观测性 × 其他域

- Cilium eBPF × 可观测性 — 网络可观测性
- CRD × 可观测性 — 自定义资源监控
- [[synthesis/observability-finops|可观测性 × FinOps]] — 成本驱动监控
- [[synthesis/multi-cluster-observability-federation|多集群可观测性联邦]] — 跨集群指标聚合
- [[synthesis/可观测性支柱 × Prometheus-Grafana|可观测性支柱 × Prometheus-Grafana]] — 监控栈集成
- [[synthesis/chaos-drill-integration|混沌演练 × 可观测性]] — 故障注入监控
- [[synthesis/security-observability-correlation|安全 × 可观测性关联]] — 安全事件监控

### 平台工程 × 其他域

- GitOps × 平台工程 — 发布管理 × 平台工程
- [[synthesis/platform-engineering-sre|平台工程 × SRE]] — 平台可靠性
- [[synthesis/backstage-platform-catalog|Backstage × 平台目录]] — 开发者门户
- [[synthesis/finops-resource-governance|FinOps × 资源治理]] — 成本预算与配额
- [[synthesis/gitops-sre-release-gate|GitOps × SRE 发布门控]] — SLO 驱动发布

### 网络 × 安全

- CNI 插件 × NetworkPolicy — 网络安全策略
- eBPF × 运行时安全 — 容器安全监控
- [[synthesis/multi-cluster-security|多集群安全]] — 跨集群安全策略
- [[synthesis/service-mesh-security-governance|服务网格 × 安全治理]] — Istio 安全策略
- 纵深防御 × 供应链安全 — 供应链安全
- 服务网格 × 零信任安全 — mTLS + 零信任

### 存储 × 灾备

- [[synthesis/velero-disaster-recovery|Velero × 灾难恢复]] — 备份策略与恢复
- [[synthesis/data-protection-k8s|K8s 数据保护]] — 数据备份策略
- [[synthesis/cross-cloud-migration-playbook|跨云迁移手册]] — 多云数据迁移
- [[synthesis/edge-cloud-continuum|边缘-云连续体]] — 边缘存储

### AI/ML × 运维

- [[synthesis/ai-agent-ops-patterns|AI Agent × 运维自动化]] — 智能运维模式
- [[synthesis/ai-ml-observability|AI/ML × 可观测性]] — ML 模型监控

### 控制器模式 × 其他域

- [[synthesis/控制器模式 × Deployment|控制器模式 × Deployment]] — 控制器与部署
- 控制器模式 × Operator 模式 — 控制器演进
- 控制器模式 × 可观测性 — 控制器监控
- 声明式 API × 控制器模式 — 声明式系统
- Operator 模式 × Pod 生命周期 — 生命周期管理
- Operator 模式 × 可观测性 — Operator 监控
- [[entities/etcd|etcd]] × Operator 模式 — etcd Operator

### 资源管理

- [[synthesis/cost-optimization-multi-cluster|多集群成本优化]] — 跨集群成本
- Pod 生命周期 × 存储模型 — 存储生命周期
- Pod 生命周期 × Secret 管理 — Secret 注入
- [[synthesis/Deployment × Secret 管理|Deployment × Secret 管理]] — 部署与 Secret
- Secret 管理 × 存储模型 — Secret 存储
- IaC × [[domain-07-platform-engineering/operate/13-multi-cluster-management|多集群管理]] — 基础设施即代码

### SRE/可靠性

- [[synthesis/slo-monitoring-integration|SLO × 监控集成]] — SLO 监控
- K8s 问题分布与 MTTR 基准 — 问题统计
- [[entities/kubernetes|Kubernetes]] Fault Distribution and MTTR.md|K8s 问题分布 (EN)]] — 英文版
- [[domain-17-system-foundation/topic-dictionary/operations/production-troubleshooting-playbook|生产故障排查手册]] — 综合排查
- [[synthesis/Structural Troubleshooting Framework|结构化故障排查框架]] — 方法论

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

- [[synthesis/etcd × Operator 模式|etcd × Operator 模式]]

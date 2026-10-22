---
title: 分布式云 ACK One
description: ACK One 目录索引 — 多集群舰队 Fleet、注册集群混合云、多集群网关/Service、分布式工作流与容灾
category: index
tags:
- index
- alibaba-cloud
- ack-one
- multi-cluster
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-20'
---

# 分布式云 ACK One

> 阿里云分布式云容器平台 ACK One（Distributed Cloud Container Platform for Kubernetes）：面向混合云、多集群、分布式计算、容灾场景的企业级云原生平台。

## 模式定位一句话

**ACK One = 托管版 "OCM + Karmada + ArgoCD"，再用托管网关（MSE/ALB）与多集群 Service 补齐跨集群数据面** —— Hub-Spoke 骨架 + Federation 调度内核 + 云产品数据面的三层组合。

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[18-云厂商/01-阿里云/分布式云-ACKOne/01-ack-one-multi-cluster-architecture.md\|ACK One 多集群架构深度剖析]] | Fleet 架构 / Karmada 分发链路 / 多集群流量 / 容灾 / 选型对比 | advanced |
| [[18-云厂商/01-阿里云/分布式云-ACKOne/02-karmada-deployment-distribution.md\|一个普通 Deployment 的多集群之旅 —— Karmada 分发全链路]] | PropagationPolicy→ResourceBinding→Work 全链路 / 最小 YAML / Duplicated vs Divided / 开源与 ACK One 差异 / 面试问答卡 | intermediate |
| [[18-云厂商/01-阿里云/分布式云-ACKOne/03-ack-one-ocm-selection-rationale.md\|ACK One 为何选型 OCM —— 纳管层选型逻辑深剖]] | 分层选型框架 / OCM 架构与边界（klusterlet pull 模式 / Placement 只选集群）/ 五条选型逻辑 / 备选反事实推演 / CNCF 成熟度反差 / 云厂商路线对照 / 决策树 / 面试口径 | advanced |

## Related

- [[18-云厂商/01-阿里云/index.md|阿里云知识域]]
- [[28-资产/architecture-diagrams/architecture-multi-cluster.md|多集群架构模式]]
- [[20-最佳实践/09-large-scale-production/10-multi-cluster.md|多集群与联邦管理最佳实践]]

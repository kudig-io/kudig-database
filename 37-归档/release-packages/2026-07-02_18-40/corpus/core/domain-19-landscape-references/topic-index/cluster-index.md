---
title: Cluster 集群知识图谱索引
description: '## Cluster 知识图谱'
summary: '## Cluster 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- cluster
- lifecycle
- kubeadm
- etcd
- apiserver
- kubelet
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Cluster 知识图谱 是什么
- Kubernetes 集群 相关文档
trigger_keywords:
- Cluster
- 知识图谱
- index
- kubeadm
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cluster 集群知识图谱索引

> 知识图谱：按关键字 **cluster** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 集群生命周期

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/operate/01-cluster-lifecycle-management|集群生命周期管理 (Cluster Lifecycle Management)]]
- 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)
- 集群容量规划

### 集群创建

- [[entities/kubernetes.md|Kubernetes]] 集群新建逻辑 — 基于官方代码分析](domain-02-workloads-applications/topic-functions/cluster-create/01-overview.md)
- [预检阶段 (Preflight Checks)](domain-02-workloads-applications/topic-functions/cluster-create/02-preflight.md)
- [证书阶段 (Certificate Generation)](domain-02-workloads-applications/topic-functions/cluster-create/03-certs.md)
- [kubeconfig 阶段 (Kubeconfig Generation)](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-create/01-kubeconfig.md)
- [控制面阶段 (Control Plane & Static [[domain-17-system-foundation/知识字典/workloads/pods.md|Pods]])](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-create/04-control-plane.md)
- 节点加入流程 (kubeadm join)](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-create/05-join.md)
- [etcd 集群初始化细节](32-发布/package/2026-07-02_18-40/corpus/core/domain-02-workloads-applications/topic-functions/cluster-create/01-etcd.md)
- [高可用控制面搭建](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/07-ha.md)
- [集群升级流程](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/08-upgrade.md)
- [CNI 网络插件与集群网络](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/16-cni-networking.md)
- [Node 注册与 kubeadm token 详解](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/17-node-registration.md)
- Cluster Create — Kubernetes 集群新建源码分析](domain-02-workloads-applications/topic-functions/cluster-create/README.md)

### 集群证书

- [Kubernetes 集群 PKI 架构总览](domain-02-workloads-applications/topic-functions/cluster-cert/01-pki-architecture.md)
- [CA 证书生成源码分析](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/01-ca-generation.md)
- [API Server 证书生成源码分析](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/02-apiserver-cert.md)
- [etcd 证书体系源码分析](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/02-etcd-cert.md)
- [kubelet 证书与 CSR 机制源码分析](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/03-kubelet-cert.md)
- [证书轮换机制源码分析](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/04-cert-rotation.md)
- [证书身份到 RBAC 的映射关系](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/03-rbac-mapping.md)
- Cluster Cert — Kubernetes 集群证书体系源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/README.md)

### 集群删除

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-delete/03-etcd-cleanup|etcd 数据清理与成员移除 — 源码分析]]
- [[domain-02-workloads-applications/topic-functions/cluster-delete/README.md|Cluster Delete — Kubernetes 集群删除源码分析]]

## 关联文档 (K8s 集成)

### 故障排查

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/07-upgrade-migration-troubleshooting|34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/02-large-scale-cluster-operations|40 - 大规模集群运维 (Large Scale Cluster Operations)]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/01-cluster-maintenance-troubleshooting.md|集群运维与升级故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md|集群高可用与灾备故障排查指南]]

### 多集群管理

- 多集群管理
- 55 - 虚拟集群与多租户

### YAML 配置参考

- 33 - kubeadm 集群引导配置 YAML 参考

## 扩展参考

### 集群生态项目

- Karmada
- Clusterpedia
- Kubean
- kcp (Kubernetes-like Control Plane)

### 平台运维

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/governance/02-large-scale-cluster-optimization|14 large scale cluster optimization]]
- Karpenter 节点自动扩展实践指南


<!-- risk-assessed -->

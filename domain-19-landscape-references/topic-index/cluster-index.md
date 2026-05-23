---
title: Cluster 集群知识图谱索引
description: '## Cluster 知识图谱'
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
created: "2026-05-23"
---

# Cluster 集群知识图谱索引

> 知识图谱：按关键字 **cluster** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 集群生命周期

- [[domain-07-platform-engineering/operate/02-cluster-lifecycle-management|集群生命周期管理 (Cluster Lifecycle Management)]]]]
- 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)
- 集群容量规划

### 集群创建

- [[entities/kubernetes|Kubernetes]] 集群新建逻辑 — 基于官方代码分析](domain-02-workloads-applications/topic-functions/cluster-create/01-overview.md)
- [预检阶段 (Preflight Checks)](domain-02-workloads-applications/topic-functions/cluster-create/02-preflight.md)
- [证书阶段 (Certificate Generation)](domain-02-workloads-applications/topic-functions/cluster-create/03-certs.md)
- [kubeconfig 阶段 (Kubeconfig Generation)](domain-02-workloads-applications/topic-functions/cluster-create/04-kubeconfig.md)
- [控制面阶段 (Control Plane & Static [[Pods|Pods]])](domain-02-workloads-applications/topic-functions/cluster-create/05-control-plane.md)
- 节点加入流程 (kubeadm join)](domain-02-workloads-applications/topic-functions/cluster-create/06-join.md)
- [etcd 集群初始化细节](domain-02-workloads-applications/topic-functions/cluster-create/07-etcd.md)
- [高可用控制面搭建](domain-02-workloads-applications/topic-functions/cluster-create/08-ha.md)
- [集群升级流程](domain-02-workloads-applications/topic-functions/cluster-create/09-upgrade.md)
- [CNI 网络插件与集群网络](domain-02-workloads-applications/topic-functions/cluster-create/19-cni-networking.md)
- [Node 注册与 kubeadm token 详解](domain-02-workloads-applications/topic-functions/cluster-create/20-node-registration.md)
- Cluster Create — Kubernetes 集群新建源码分析](domain-02-workloads-applications/topic-functions/cluster-create/README.md)

### 集群证书

- [Kubernetes 集群 PKI 架构总览](domain-02-workloads-applications/topic-functions/cluster-cert/01-pki-architecture.md)
- [CA 证书生成源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/02-ca-generation.md)
- [API Server 证书生成源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/03-apiserver-cert.md)
- [etcd 证书体系源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/04-etcd-cert.md)
- [kubelet 证书与 CSR 机制源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/05-kubelet-cert.md)
- [证书轮换机制源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/06-cert-rotation.md)
- [证书身份到 RBAC 的映射关系](domain-02-workloads-applications/topic-functions/cluster-cert/08-rbac-mapping.md)
- Cluster Cert — Kubernetes 集群证书体系源码分析](domain-02-workloads-applications/topic-functions/cluster-cert/README.md)

### 集群删除

- [[domain-02-workloads-applications/topic-functions/cluster-delete/05-etcd-cleanup|etcd 数据清理与成员移除 — 源码分析]]
- [[domain-02-workloads-applications/topic-functions/cluster-delete/README|[[Cluster Delete — Kubernetes 集群删除源码分析|Cluster Delete — Kubernetes 集群删除源码分析]]]]

## 关联文档 (K8s 集成)

### 故障排查

- [[domain-10-troubleshooting-diagnostics/34-upgrade-migration-troubleshooting|34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/40-large-scale-cluster-operations|40 - 大规模集群运维 (Large Scale Cluster Operations)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/01-cluster-maintenance-troubleshooting|集群运维与升级故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/04-ha-disaster-recovery-troubleshooting|集群高可用与灾备故障排查指南]]

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

- [[domain-07-platform-engineering/governance/14-large-scale-cluster-optimization]]
- Karpenter 节点自动扩展实践指南

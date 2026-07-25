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

- [[10-平台工程/02-运维/02-cluster-lifecycle-management.md|集群生命周期管理 (Cluster Lifecycle Management)]]
- 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)
- 集群容量规划

### 集群创建

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] 集群新建逻辑 — 基于官方代码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/01-overview.md)
- [预检阶段 (Preflight Checks)](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/02-preflight.md)
- [证书阶段 (Certificate Generation)](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/03-certs.md)
- [kubeconfig 阶段 (Kubeconfig Generation)](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/04-kubeconfig.md)
- [控制面阶段 (Control Plane & Static [[17-系统基础/06-知识字典/workloads/pods.md|Pods]])](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/05-control-plane.md)
- 节点加入流程 (kubeadm join)](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/06-join.md)
- [etcd 集群初始化细节](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/07-etcd.md)
- [高可用控制面搭建](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/08-ha.md)
- [集群升级流程](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/09-upgrade.md)
- [CNI 网络插件与集群网络](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/19-cni-networking.md)
- [Node 注册与 kubeadm token 详解](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/20-node-registration.md)
- Cluster Create — Kubernetes 集群新建源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-create/README.md)

### 集群证书

- [Kubernetes 集群 PKI 架构总览](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/01-pki-architecture.md)
- [CA 证书生成源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/02-ca-generation.md)
- [API Server 证书生成源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/03-apiserver-cert.md)
- [etcd 证书体系源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/04-etcd-cert.md)
- [kubelet 证书与 CSR 机制源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/05-kubelet-cert.md)
- [证书轮换机制源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/06-cert-rotation.md)
- [证书身份到 RBAC 的映射关系](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/functions-cluster-cert/08-rbac-mapping.md)
- Cluster Cert — Kubernetes 集群证书体系源码分析](../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/06-%E4%BB%A3%E7%A0%81%E5%88%86%E6%9E%90/cluster-cert/README.md)

### 集群删除

- [[10-平台工程/06-代码分析/functions-cluster-delete/05-etcd-cleanup.md|etcd 数据清理与成员移除 — 源码分析]]
- [[10-平台工程/06-代码分析/functions-cluster-delete/README.md|Cluster Delete — Kubernetes 集群删除源码分析]]

## 关联文档 (K8s 集成)

### 故障排查

- [[19-故障诊断/03-基础设施排障/34-upgrade-migration-troubleshooting.md|34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)]]
- [[19-故障诊断/04-高级排障/40-large-scale-cluster-operations.md|40 - 大规模集群运维 (Large Scale Cluster Operations)]]
- [[19-故障诊断/04-高级排障/structural-08-cluster-operations/01-cluster-maintenance-troubleshooting.md|集群运维与升级故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md|集群高可用与灾备故障排查指南]]

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

- [[10-平台工程/03-治理/14-large-scale-cluster-optimization.md|14 large scale cluster optimization]]
- Karpenter 节点自动扩展实践指南


<!-- risk-assessed -->

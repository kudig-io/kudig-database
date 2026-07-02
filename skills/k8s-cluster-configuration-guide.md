---
title: Kubernetes 集群配置最佳实践
description: '# Kubernetes 集群配置最佳实践'
summary: '本指南提供生产环境 Kubernetes 集群配置的最佳实践，涵盖从集群规划到配置优化的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- cluster
- configuration
- production
- high-availability
- etcd
- apiserver
- kubelet
- cilium
- calico
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 集群配置最佳实践 是什么
- 如何 Kubernetes 集群配置最佳实践
trigger_keywords:
- Kubernetes
- 集群配置最佳实践
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 集群配置最佳实践

## 概述

本指南提供生产环境 Kubernetes 集群配置的最佳实践，涵盖从集群规划到配置优化的全方位内容 ^[inferred]。

## 高可用控制平面

控制平面至少需要 3 个主节点，通过负载均衡器（HAProxy/Nginx）分发 API 请求。etcd 集群保证数据一致性，任意节点问题不影响集群功能 ^[inferred]。

### API Server 关键配置

- `max-requests-inflight: 1000` — 并发读取请求上限
- `max-mutating-requests-inflight: 500` — 并发变更请求上限
- 启用审计日志：`audit-log-path`、`audit-log-maxage: 30`、`audit-log-maxbackup: 10`
- 安全加固：`anonymous-auth: false`

### etcd 关键配置

- `quota-backend-bytes: 8589934592`（8GB）— 默认 2GB 对于大型集群不足 ^[inferred]
- `snapshot-count: 10000` — 快照频率
- `heartbeat-interval: 100`、`election-timeout: 1000` — Raft 参数
- 启用客户端和对等端证书认证 ^[inferred]

### Controller Manager 配置

- `concurrent-deployment-syncs: 10`
- `node-monitor-grace-period: 40s`
- `pod-eviction-timeout: 5m`
- `terminated-pod-gc-threshold: 100`

## 实施步骤

1. **环境准备**：配置内核参数（`net.bridge.bridge-nf-call-*`、`net.ipv4.ip_forward`），禁用 swap
2. **安装容器运行时**：containerd 1.6+，启用 SystemdCgroup
3. **安装 [[entities/kubernetes.md|Kubernetes 组件]]**：[[kubelet|kubelet]]、kubeadm、kubectl
4. **初始化控制平面**：使用 kubeadm-config.yaml 初始化，配置 controlPlaneEndpoint
5. **安装网络插件**：Calico/Cilium，确保 Pod CIDR 与集群配置一致 ^[inferred]
6. **加入工作节点**：使用 kubeadm join 命令

## 常见陷阱

### etcd 存储配额不足

默认 2GB 配额对于大型集群不足，会导致集群无法创建新资源、API Server 响应缓慢。应设置为 8GB 或更高 ^[inferred]。

### API Server 并发限制不当

并发限制设置过低会导致 API Server 过载、请求超时。生产环境建议 `max-requests-inflight: 1000`，`max-mutating-requests-inflight: 500` ^[inferred]。

### 网络插件 Pod CIDR 不匹配

网络插件的 Pod CIDR 必须与集群 `kubeadm-config.yaml` 中的 `podSubnet` 一致，否则会导致 Pod 间通信异常 ^[inferred]。

## 验证方法

- 检查控制平面节点状态：`kubectl get nodes`
- 检查 etcd 集群成员：`etcdctl member list`
- 检查系统组件：`kubectl get [[Pods|pods]] -n kube-system`
- 检查集群资源使用：`kubectl top nodes`

## 相关资源

- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[entities/kube-apiserver.md|kube-apiserver]]
- [[etcd|etcd]]
- [[skills/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]]

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践


<!-- risk-assessed -->

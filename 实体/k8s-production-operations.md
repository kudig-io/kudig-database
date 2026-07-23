---
title: 生产运维：GitOps、FinOps、灾备恢复与变更管理
description: '# 生产运维'
summary: '# 生产运维'
category: reference
tags:
- k8s
- production-ops
- gitops
- finops
- disaster-recovery
- change-management
- etcd
- flux
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 生产运维：GitOps、FinOps、灾备恢复与变更管理 是什么
- 如何 生产运维：GitOps、FinOps、灾备恢复与变更管理
trigger_keywords:
- 生产运维：GitOps
- FinOps
- 灾备恢复与变更管理
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 生产运维

> **CNCF 状态**: 实践指南 | **类别**: Operations | **主要语言**: YAML, Bash, Go

## 概述

Kubernetes 生产环境运维实践是一套涵盖集群全生命周期管理的运维方法论和最佳实践。它包括集群部署与升级、容量规划、高可用配置、备份恢复、监控告警、安全运维、性能调优等多个维度。该体系整合了 kubeadm、Cluster API、Velero、Prometheus、Grafana 等工具，为 K8s 生产集群提供标准化、可重复的运维流程。

## Key Features（核心能力）

- **集群生命周期管理**：基于 Cluster API 和 kubeadm 的集群创建、升级、扩缩容
- **高可用架构**：多 Master 节点、etcd 集群、负载均衡的高可用设计
- **备份与恢复**：基于 Velero 的集群资源和 PV 数据备份策略
- **监控告警体系**：Prometheus + Grafana + AlertManager 的可观测性栈
- **升级策略**：滚动升级、金丝雀升级、回滚机制的标准化流程
- **容量管理**：资源请求/限制规划、集群自动扩缩容（Cluster Autoscaler）

## 架构与工作原理

生产运维体系分层管理：基础设施层（网络、存储、计算资源管理）；控制平面层（API Server、etcd、Controller Manager 的 HA 部署）；工作节点层（节点池管理、运行时配置）；应用层（部署策略、HPA/VPA、PDB）；可观测性层（指标、日志、链路追踪的采集与告警）。通过 GitOps 和 IaC 实现运维自动化。

## K8s 集成

K8s 生产运维直接操作集群核心资源：通过 kubeadm/kops/EKS/GKE 管理控制平面；通过 MachineDeployment/MachineSet 管理节点生命周期；通过 HPA/VPA/CA 实现自动伸缩；通过 PDB/Topology Spread 确保可用性；通过 Velero 执行备份恢复；通过 Prometheus Operator 管理监控配置。

## 生产用例

- **大规模集群运维**：管理数百节点的生产 K8s 集群
- **多集群管理**：跨数据中心/云的多集群统一运维
- **灾难恢复**：制定和执行集群级别的灾难恢复计划
- **合规审计**：满足生产环境的安全合规和审计要求

## 安装与配置

```bash
# 🟢 kubeadm 集群初始化（HA 模式）
kubeadm init --control-plane-endpoint "vip:6443" \
  --upload-certs \
  --pod-network-cidr=10.244.0.0/16 \
  --service-cidr=10.96.0.0/12 \
  --apiserver-advertise-address=0.0.0.0

# 🟢 安装 Velero 备份工具
velero install --provider aws \
  --bucket k8s-backup \
  --backup-location-config region=us-east-1 \
  --snapshot-location-config region=us-east-1 \
  --use-volume-snapshots

# 🟢 验证备份工具
velero backup create test-backup --wait
velero backup get

# 🟢 安装 Prometheus 监控栈
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install monitoring prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  --set prometheus.prometheusSpec.retention=30d \
  --set alertmanager.enabled=true

# 🟢 安装 Cluster Autoscaler
kubectl apply -f cluster-autoscaler.yaml
```

### 备份策略配置

```yaml
# Velero 定时备份
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-full-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  template:
    includedNamespaces:
      - "*"
    excludedNamespaces:
      - kube-system
      - velero
    includeClusterResources: true
    storageLocation: default
    ttl: 168h  # 保留7天
    snapshotVolumes: true
---
# 备份存储位置
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: k8s-backup
    prefix: prod-cluster
  config:
    region: us-east-1
```

### 变更管理流程

```yaml
# GitOps 变更流程 (Flux)
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: production-apps
  namespace: flux-system
spec:
  interval: 5m
  path: ./clusters/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: flux-system
  validation: server
  # 变更审批（通过 PR 合并触发）
  postBuild:
    substituteFrom:
      - kind: ConfigMap
        name: cluster-vars
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: critical-app
      namespace: production
```

## 运维操作

```bash
# 🟢 集群健康检查
kubectl get componentstatuses
kubectl get nodes -o wide
kubectl top nodes

# 🟢 查看集群资源使用
kubectl top pods -A --sort-by=cpu | head -20
kubectl top pods -A --sort-by=memory | head -20

# 🟢 检查待处理 PVC
kubectl get pvc -A --field-selector=status.phase!=Bound

# 🟡 执行集群备份
velero backup create pre-upgrade-$(date +%Y%m%d) --wait

# 🟡 滚动升级 K8s 版本
kubeadm upgrade plan
kubeadm upgrade apply v1.30.0
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
kubectl uncordon <node>

# 🔴 灾难恢复（从备份恢复）
velero restore create --from-backup daily-full-backup-20260701 --wait
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 节点 NotReady | kubelet 异常 | `kubectl describe node <node>` | 检查 kubelet 日志和证书 |
| etcd 延迟高 | 磁盘 I/O 不足 | `etcdctl endpoint status` | 升级 SSD 或分离 etcd |
| 备份失败 | 存储后端不可用 | `velero backup logs <name>` | 检查 S3 连接和权限 |
| 升级失败 | 版本不兼容 | `kubeadm upgrade plan` | 检查版本跳跃和插件兼容 |

```bash
# 排查流程
# 1. 控制平面健康
kubectl get --raw /healthz?verbose
etcdctl endpoint health --cluster

# 2. 节点状态检查
kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,REASON:.status.conditions[-1].reason

# 3. 系统 Pod 状态
kubectl get pods -n kube-system -o wide | grep -v Running

# 4. 资源压力检查
kubectl describe nodes | grep -A5 "Conditions"
```

## 生产案例

### 案例1：集群升级零停机
- **场景**：生产集群从 K8s 1.28 升级到 1.30，要求零业务中断
- **方案**：先备份（Velero）；控制平面滚动升级；工作节点逐个 drain/upgrade/uncordon；PDB 确保应用可用性
- **效果**：升级全程零停机，回滚方案就绪（备份 + 旧节点池）

### 案例2：灾难恢复演练
- **场景**：验证集群级别灾难恢复能力（RTO < 1h, RPO < 5min）
- **方案**：Velero 定时备份 + etcd 快照；恢复流程自动化脚本；季度 DR 演练
- **效果**：实际恢复时间 45min，满足 RTO 要求；发现并修复 3 个恢复流程缺陷

## 对比替代方案

| 维度 | 自建运维 | 托管 K8s (EKS/GKE) | Rancher | Cluster API |
|------|----------|-------------------|---------|------------|
| 控制力 | 完全 | 受限 | 中 | 完全 |
| 运维复杂度 | 高 | 低 | 中 | 中 |
| 成本 | 中 | 高 | 中 | 低 |
| 多集群 | 手动 | 单云 | 强 | 强 |
| 学习曲线 | 高 | 低 | 中 | 中 |

## 检查清单

- [ ] 集群多 Master 高可用已配置
- [ ] etcd 定期备份已配置（每日）
- [ ] Velero 备份策略已配置且验证可恢复
- [ ] 监控告警已覆盖控制平面和工作节点
- [ ] 升级流程已文档化且在测试环境验证
- [ ] 变更管理流程已建立（GitOps/审批）
- [ ] 灾难恢复计划已制定且定期演练
- [ ] 容量规划已制定（节点/存储/网络）

## Related

- [[概念/GitOps × 平台工程.md|GitOps x 平台工程]] — GitOps x 平台工程
- [[概念/IaC × 多集群管理.md|IaC x 多集群管理]] — 基础设施即代码 x 多集群管理
- [[flux]] — Flux
- [[etcd]] — etcd
- [[argo]] — Argo Workflows


<!-- risk-assessed -->

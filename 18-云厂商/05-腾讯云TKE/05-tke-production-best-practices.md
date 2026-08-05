---
title: "腾讯 TKE 生产实践：集群创建、网络、存储与安全"
description: "腾讯云 TKE 生产环境最佳实践，涵盖集群规划、网络模式选择、存储方案、安全加固及监控体系"
summary: "系统讲解腾讯云 TKE（Tencent Kubernetes Engine）的生产部署实践：集群创建与规划、VPC-CNI/Flannel 网络模式对比、CBS/CFS 存储方案、安全加固、监控告警及与自建 K8s 的差异"
category: 云厂商
tags:
- tke
- tencent-cloud
- vpc-cni
- cbs
- cloud-native
- production
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "腾讯 TKE 生产环境怎么部署"
- "TKE 网络模式怎么选"
- "TKE 和自建 K8s 有什么区别"
trigger_keywords:
- tke
- tencent-cloud
- vpc-cni
- tencent
prerequisites:
- kubectl-basics
- k8s-networking
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 腾讯 TKE 生产实践

## 概述

腾讯云 TKE（Tencent Kubernetes Engine）是腾讯云提供的托管 Kubernetes 服务，支持标准集群、Serverless 集群（EKS）和边缘集群（TKE Edge）三种形态。TKE 深度集成腾讯云 VPC、CBS 云硬盘、CLB 负载均衡和 CAM 权限体系，为华南、华北及东南亚区域的企业提供低延迟的容器化基础设施。

本文聚焦 TKE 生产环境的最佳实践，涵盖集群规划、网络模式选择、存储方案、安全加固、监控体系，并与自建 K8s 进行对比分析。

## 核心概念

### TKE 集群类型

| 集群类型 | 控制面 | 节点管理 | 适用场景 | 成本模型 |
|---------|--------|---------|---------|---------|
| 标准集群 | 腾讯托管 | 用户管理 CVM | 通用生产 | 节点费用 + 管理费 |
| Serverless（EKS） | 腾讯托管 | 无节点（Pod 级） | 突发/批处理 | 按 Pod 资源计费 |
| 边缘集群 | 腾讯托管 | 边缘节点 | IoT/CDN 边缘 | 节点费用 |
| 注册集群 | 用户自建 | 用户管理 | 混合云 | 仅管理费 |

### 网络模式对比

| 维度 | VPC-CNI | Flannel（Overlay） |
|------|---------|-------------------|
| Pod IP | VPC 内真实 IP | 集群内虚拟 IP |
| 性能 | 接近原生（无封装） | 有封装开销（~5-10%） |
| Pod 密度 | 受 ENI/辅助 IP 限制 | 不受节点 IP 限制 |
| 安全组 | Pod 级安全组 | 节点级安全组 |
| 固定 IP | 支持（StatefulSet） | 不支持 |
| 网络策略 | 安全组 + NetworkPolicy | NetworkPolicy |
| 适用场景 | 高性能、需直连 VPC | 高密度、简单网络 |
| 节点要求 | 需弹性网卡配额 | 无特殊要求 |

### 存储方案

| 存储类型 | 产品 | 性能 | 适用场景 | 访问模式 |
|---------|------|------|---------|---------|
| 块存储 | CBS（云硬盘） | 高 IOPS | 数据库、有状态服务 | ReadWriteOnce |
| 文件存储 | CFS（文件存储） | 中等 | 共享文件、日志 | ReadWriteMany |
| 对象存储 | COS | 高吞吐 | 大数据、备份 | 通过 COSFS/CSI |
| 本地存储 | 本地 SSD | 极高 | 缓存、临时数据 | ReadWriteOnce |

## 生产部署

### 集群创建规划

```yaml
# 🟡 中风险：TKE 集群规划（通过 Terraform）
# terraform tencentcloud_kubernetes_cluster
resource "tencentcloud_kubernetes_cluster" "production" {
  vpc_id                  = var.vpc_id
  cluster_cidr            = "172.16.0.0/16"
  service_cidr            = "10.96.0.0/16"
  cluster_max_pod_num     = 64
  cluster_max_service_num = 256
  cluster_version         = "1.30.1"
  network_type            = "VPC-CNI"
  cluster_deploy_type     = "MANAGED_CLUSTER"
  cluster_os              = "tlinux2.4(tkernel4)x86_64"
  container_runtime       = "containerd"
  
  # 生产环境关键配置
  cluster_level           = "L5"  # 大规模集群
  delete_protection       = true
  audit_log_config {
    audit_log_switch_process = "On"
  }
  
  tags = {
    Environment = "production"
    Team        = "platform"
    CostCenter  = "CC-001"
  }
}

# 节点池配置
resource "tencentcloud_kubernetes_node_pool" "general" {
  name       = "general-pool"
  cluster_id = tencentcloud_kubernetes_cluster.production.id
  max_size   = 50
  min_size   = 5
  vpc_id     = var.vpc_id
  subnet_ids = var.subnet_ids
  
  desired_capacity = 10
  enable_auto_scale = true
  
  auto_scaling_config {
    instance_type = "S5.2XLARGE32"  # 8C32G
    system_disk_type = "CLOUD_SSD"
    system_disk_size = 100
    
    data_disk {
      disk_type = "CLOUD_PREMIUM"
      disk_size = 200
    }
    
    security_group_ids = [var.security_group_id]
  }
  
  node_config {
    docker_graph_path = "/var/lib/containerd"
  }
  
  labels = {
    "node-pool" = "general"
    "workload-type" = "stateless"
  }
  
  taints {
    key    = "dedicated"
    value  = "general"
    effect = "NoSchedule"
  }
}
```

### VPC-CNI 网络配置

```yaml
# 🟡 中风险：VPC-CNI 网络配置
# TKE VPC-CNI 使用弹性网卡（ENI）为 Pod 分配 VPC IP
# 节点可承载 Pod 数 = (ENI 数 × 每 ENI 辅助 IP 数) - 1

# 查看节点 ENI 配额
# 腾讯云控制台 → 私有网络 → 弹性网卡 → 配额

# Pod 固定 IP（StatefulSet 场景）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
  namespace: database
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
      annotations:
        # TKE 固定 IP 注解
        cni.projectcalico.org/ipAddrs: '["172.16.1.10"]'
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
        resources:
          requests:
            cpu: "4"
            memory: "8Gi"
          limits:
            cpu: "8"
            memory: "16Gi"
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: cbs-ssd
      resources:
        requests:
          storage: 100Gi
---
# CBS SSD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cbs-ssd
provisioner: com.tencent.cloud.csi.cbs
parameters:
  diskType: CLOUD_SSD
  diskChargeType: PREPAID
  diskChargePrepaidPeriod: "1"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 安全加固

```yaml
# 🟡 中风险：TKE 安全配置
# 1. Pod 安全标准
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# 2. NetworkPolicy 默认拒绝
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  egress:
  - to: []
    ports:
    - protocol: UDP
      port: 53  # 允许 DNS
---
# 3. RBAC 最小权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-deployer
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
```

### 监控与告警

```yaml
# 🟢 低风险：TKE 监控配置
# TKE 集成腾讯云 Prometheus（TMP）
# 部署 Prometheus 监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: tke-production-alerts
  namespace: monitoring
spec:
  groups:
  - name: tke-node-alerts
    rules:
    - alert: NodeHighCPU
      expr: 100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
      for: 10m
      labels:
        severity: warning
        cluster: tke-production
      annotations:
        summary: "节点 CPU 使用率超过 85%"
        description: "节点 {{ $labels.instance }} CPU 使用率 {{ $value }}%"
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total{namespace="production"}[15m]) > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Pod 频繁重启"
        description: "{{ $labels.namespace }}/{{ $labels.pod }} 在 15 分钟内重启"
```

## 运维操作

### 集群日常管理

```bash
# 🟢 低风险：TKE 集群日常操作
# 配置 kubectl 访问 TKE
tke kubeconfig --cluster-id cls-xxxxx --region ap-guangzhou > ~/.kube/tke-config
export KUBECONFIG=~/.kube/tke-config

# 查看集群状态
kubectl cluster-info
kubectl get nodes -o wide
kubectl top nodes

# 查看节点池
tke node-pool list --cluster-id cls-xxxxx

# 扩容节点池
# 🟡 中风险：扩容会产生费用
tke node-pool scale --cluster-id cls-xxxxx --node-pool-id np-xxxxx --desired-capacity 15

# 查看集群事件
kubectl get events -A --sort-by='.lastTimestamp' | tail -20
```

### 集群升级

```bash
# 🔴 高风险：集群版本升级
# 1. 检查升级兼容性
tke cluster upgrade-check --cluster-id cls-xxxxx

# 2. 升级控制面（腾讯托管，自动完成）
tke cluster upgrade --cluster-id cls-xxxxx --version 1.30.1

# 3. 滚动升级节点
# 🟡 中风险：逐节点升级
tke node-pool upgrade --cluster-id cls-xxxxx --node-pool-id np-xxxxx --max-unavailable 1

# 4. 验证升级结果
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion
```

### 存储管理

```bash
# 🟢 低风险：存储操作
# 查看 PVC 状态
kubectl get pvc -A

# 扩容 CBS 卷（需 StorageClass 支持 allowVolumeExpansion）
# 🟡 中风险：扩容操作
kubectl patch pvc mysql-data-0 -n database -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 查看 CBS 卷绑定
kubectl get pv -o custom-columns=NAME:.metadata.name,CAPACITY:.spec.capacity.status,CLAIM:.spec.claimRef.name,STATUS:.status.phase
```

## 故障排查

### 网络问题

```bash
# 🟢 低风险：TKE 网络诊断
# Pod 无法获取 IP（VPC-CNI ENI 配额不足）
kubectl describe pod <pod-name> -n <namespace>
# 错误：failed to allocate IP: no available ENI

# 解决：增加节点 ENI 配额或减少 Pod 密度
# 腾讯云控制台 → 弹性网卡 → 申请配额提升

# Service 无法访问
kubectl get svc -n production
kubectl get endpoints -n production
# 检查 CLB 健康检查
tke clb describe --cluster-id cls-xxxxx

# DNS 解析失败
kubectl exec -it <pod> -n production -- nslookup kubernetes.default
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
```

### 节点问题

```bash
# 🟢 低风险：节点诊断
# 节点 NotReady
kubectl describe node <node-name>
# 检查 kubelet 状态（SSH 到节点）
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago"

# 节点磁盘压力
kubectl describe node <node-name> | grep -A5 "Conditions"
# DiskPressure: True → 清理容器镜像和日志
crictl rmi --prune
journalctl --vacuum-size=500M
```

## 最佳实践

### TKE vs 自建 K8s 对比

| 维度 | TKE 托管 | 自建 K8s |
|------|---------|---------|
| 控制面运维 | 腾讯负责（免费） | 自行运维（人力成本） |
| 升级 | 一键升级 | 手动滚动升级 |
| 高可用 | 默认多 AZ | 需自行配置 etcd 集群 |
| 网络 | VPC-CNI 原生集成 | 需自行部署 CNI |
| 存储 | CBS/CFS CSI 内置 | 需自行部署 CSI Driver |
| 监控 | TMP 集成 | 需自行部署 Prometheus |
| 安全 | CAM + 安全组 | 需自行配置 RBAC + 网络策略 |
| 成本 | 管理费 + 资源费 | 纯资源费 + 人力 |
| 灵活性 | 受限于 TKE 功能 | 完全自主 |
| 适用团队 | 中小团队/快速上线 | 大团队/特殊需求 |

### 生产建议

1. **多 AZ 部署**：节点池跨可用区分布，避免单 AZ 故障
2. **节点池分离**：按工作负载类型划分节点池（通用/计算密集/GPU/有状态）
3. **VPC-CNI 优先**：生产环境推荐 VPC-CNI，性能更好且支持 Pod 级安全组
4. **CBS 快照策略**：有状态服务配置定时快照，RPO < 1 小时
5. **审计日志**：开启集群审计日志，保留 180 天
6. **与 [[18-云厂商/07-多云混合/11-multicloud-comparison-decision-matrix|多云对比]] 配合**：评估是否需要多云部署
7. **参考 [[10-平台工程/03-治理/04-security-compliance|安全合规]] 完善安全体系**

## Related

- [[18-云厂商/07-多云混合/11-multicloud-comparison-decision-matrix|多云对比决策矩阵]]
- [[18-云厂商/06-华为云CCE/05-cce-production-best-practices|华为 CCE 生产实践]]
- [[23-实体/02-K8s核心组件/cni-plugins|CNI 插件]]
- [[22-概念/04-存储/csi-drivers|CSI 驱动]]
- [[10-平台工程/03-治理/04-security-compliance|安全合规]]
- [[23-实体/07-可观测性/prometheus|Prometheus 监控]]

---
title: "华为 CCE 生产实践：集群类型、网络模型与特色功能"
description: "华为云 CCE 生产环境最佳实践，涵盖集群类型选择、网络模型、存储方案、安全运维及特色功能"
summary: "系统讲解华为云 CCE（Cloud Container Engine）的生产部署实践：CCE/CCE Turbo/CCE Autopilot 集群类型对比、VPC/容器隧道网络模型、EVS/SFS 存储、安全加固及华为云特色功能"
category: 云厂商
tags:
- cce
- huawei-cloud
- vpc-network
- evs
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
- "华为 CCE 生产环境怎么部署"
- "CCE Turbo 和标准 CCE 有什么区别"
- "华为云容器网络模型怎么选"
trigger_keywords:
- cce
- huawei-cloud
- cce-turbo
- huawei
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

# 华为 CCE 生产实践

## 概述

华为云 CCE（Cloud Container Engine）是华为云提供的企业级 Kubernetes 服务，在国内政企市场（金融、政务、运营商）占据重要份额。CCE 提供三种集群形态：标准 CCE（托管控制面 + ECS 节点）、CCE Turbo（云原生 2.0，VPC 直通网络）和 CCE Autopilot（全托管 Serverless）。

华为云 CCE 的差异化优势在于：与华为云 IAM/VPC/EVS 深度集成、支持鲲鹏 ARM 节点、提供 AOM（应用运维管理）一站式可观测、以及在政企合规（等保、密评）方面的成熟支持。

## 核心概念

### CCE 集群类型对比

| 维度 | CCE 标准 | CCE Turbo | CCE Autopilot |
|------|---------|-----------|---------------|
| 控制面 | 托管（3 副本） | 托管（3 副本） | 全托管 |
| 节点 | ECS/BMS | ECS（需 ENI） | 无节点 |
| 网络模型 | 容器隧道/VPC | VPC 直通（ENI） | VPC 直通 |
| Pod 性能 | 有封装开销 | 接近裸金属 | 接近裸金属 |
| 最大节点数 | 5000 | 5000 | N/A |
| 最大 Pod 数 | 200,000 | 200,000 | 按需 |
| GPU 支持 | 完整 | 完整 | 有限 |
| ARM 支持 | 鲲鹏 ECS | 鲲鹏 ECS | 有限 |
| 适用场景 | 通用生产 | 高性能/金融 | 突发/Serverless |
| 成本 | 节点费 + 管理费 | 节点费 + 管理费 | 按 Pod 计费 |

### 网络模型

CCE 支持三种网络模型：

1. **VPC 网络（推荐）**：Pod IP 来自 VPC 子网，通过 VPC 路由转发，无封装开销
2. **容器隧道网络**：Pod IP 为集群内部 CIDR，通过 VXLAN/Geneve 隧道通信
3. **云原生 2.0 网络（CCE Turbo）**：Pod 直接使用 ENI 弹性网卡，性能最优

```
VPC 网络模型：
Pod A (172.16.1.10) ──→ VPC Router ──→ Pod B (172.16.2.20)
                        （VPC 路由表）

容器隧道网络：
Pod A (10.244.1.10) ──→ VXLAN 封装 ──→ 节点 B ──→ Pod B (10.244.2.20)

云原生 2.0（CCE Turbo）：
Pod A (ENI-1: 172.16.1.10) ──→ VPC 直通 ──→ Pod B (ENI-2: 172.16.2.20)
```

### 存储方案

| 存储类型 | 产品 | 协议 | 适用场景 | 性能 |
|---------|------|------|---------|------|
| 块存储 | EVS（云硬盘） | iSCSI | 数据库、有状态 | 高 IOPS |
| 文件存储 | SFS/SFS Turbo | NFS | 共享文件、AI 训练 | 高吞吐 |
| 对象存储 | OBS | S3 兼容 | 大数据、备份 | 高带宽 |
| 极速文件 | SFS Turbo | NFS v3 | AI 训练、HPC | 极低延迟 |
| 本地存储 | 本地 SSD | 直连 | 缓存、临时 | 极高 |

## 生产部署

### CCE Turbo 集群创建

```yaml
# 🟡 中风险：CCE Turbo 集群规划（通过华为云 API/Terraform）
# Terraform huaweicloud_cce_cluster_v3
resource "huaweicloud_cce_cluster_v3" "production" {
  name                   = "cce-production"
  cluster_type           = "VirtualMachine"
  flavor_id              = "cce.s2.large"  # 大规模集群规格
  vpc_id                 = var.vpc_id
  subnet_id              = var.subnet_id
  container_network_type = "eni"  # CCE Turbo VPC 直通
  eni_subnet_id          = var.eni_subnet_id
  eni_subnet_cidr        = "172.16.0.0/16"
  service_network_cidr   = "10.247.0.0/16"
  cluster_version        = "v1.30"
  authentication_mode    = "rbac"
  multi_az               = true  # 多 AZ 高可用
  
  # 安全配置
  delete_evs             = "skip"  # 删除集群时保留 EVS
  delete_obs             = "skip"
  delete_sfs30           = "skip"
  
  tags = {
    environment = "production"
    team        = "platform"
  }
}

# 节点池（鲲鹏 ARM + x86 混合）
resource "huaweicloud_cce_node_pool_v3" "arm_pool" {
  cluster_id         = huaweicloud_cce_cluster_v3.production.id
  name               = "kunpeng-arm-pool"
  flavor_id          = "kc2.large.2"  # 鲲鹏 920
  initial_node_count = 5
  availability_zone  = "cn-north-4a"
  
  root_volume {
    size       = 100
    volumetype = "SSD"
  }
  
  data_volumes {
    size       = 200
    volumetype = "SSD"
  }
  
  os = "EulerOS 2.9"
  
  labels = {
    "arch"        = "arm64"
    "node-pool"   = "kunpeng"
  }
  
  taints {
    key    = "arch"
    value  = "arm64"
    effect = "NoSchedule"
  }
}
```

### VPC 直通网络配置

```yaml
# 🟡 中风险：CCE Turbo Pod 网络配置
# Pod 直接使用 ENI，获得 VPC 内真实 IP
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-server
  template:
    metadata:
      labels:
        app: api-server
      annotations:
        # CCE Turbo 网络注解
        vpc.eni.cce.io/bindEniSubnet: "subnet-xxxxx"
        vpc.eni.cce.io/bindSecurityGroup: "sg-xxxxx"
    spec:
      containers:
      - name: api
        image: swr.cn-north-4.myhuaweicloud.com/myorg/api-server:v2.1
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
---
# EVS SSD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: evs-ssd
provisioner: everest-csi-provisioner
parameters:
  csi.storage.k8s.io/csi-driver-name: disk.csi.everest.io
  csi.storage.k8s.io/fstype: ext4
  type: SSD
  dsspool_id: ""
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 安全与合规

```yaml
# 🟡 中风险：CCE 安全配置
# 1. 命名空间安全标准
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
---
# 2. 华为云 IAM 集成（RBAC 映射）
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: iam-admin-binding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: User
  name: "iam-user:platform-admin@mycompany.com"
---
# 3. 网络策略（配合华为云安全组）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: production-ingress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
    ports:
    - protocol: TCP
      port: 8080
```

### 监控与运维（AOM 集成）

```yaml
# 🟢 低风险：CCE 监控配置
# CCE 集成华为云 AOM（应用运维管理）和 Prometheus
# 部署自定义 PrometheusRule
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cce-production-rules
  namespace: monitoring
spec:
  groups:
  - name: cce-alerts
    rules:
    - alert: EVSVolumeNearFull
      expr: kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "EVS 卷使用率超过 85%"
        description: "PVC {{ $labels.persistentvolumeclaim }} 使用率 {{ $value | humanizePercentage }}"
    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "节点内存压力"
        description: "节点 {{ $labels.node }} 处于内存压力状态"
```

## 运维操作

### 集群管理

```bash
# 🟢 低风险：CCE 集群操作
# 配置 kubectl（通过华为云 CLI）
hcloud CCE ShowCluster --cluster-id=xxxxx --region=cn-north-4
# 下载 kubeconfig
hcloud CCE CreateKubernetesClusterCert --cluster-id=xxxxx --region=cn-north-4

# 查看集群状态
kubectl cluster-info
kubectl get nodes -o wide --show-labels

# 查看节点池
hcloud CCE ListNodePools --cluster-id=xxxxx

# 节点池扩容
# 🟡 中风险：扩容产生费用
hcloud CCE UpdateNodePool --cluster-id=xxxxx --nodepool-id=npxxxxx \
  --body='{"spec":{"desiredNodeCount":10}}'

# 查看集群审计日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=50
```

### 集群升级

```bash
# 🔴 高风险：CCE 集群升级
# 1. 检查升级前置条件
hcloud CCE ShowClusterUpgradeInfo --cluster-id=xxxxx

# 2. 升级控制面
hcloud CCE UpgradeCluster --cluster-id=xxxxx \
  --body='{"spec":{"version":"v1.30"}}'

# 3. 滚动升级节点池
# 🟡 中风险：逐节点升级
hcloud CCE UpdateNodePool --cluster-id=xxxxx --nodepool-id=npxxxxx \
  --body='{"spec":{"rollingUpdate":{"maxUnavailable":1}}}'

# 4. 验证
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion,STATUS:.status.conditions[-1].type
```

### 存储运维

```bash
# 🟢 低风险：存储管理
# 查看 PVC
kubectl get pvc -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,SIZE:.spec.resources.requests.storage,STATUS:.status.phase

# EVS 扩容
# 🟡 中风险：扩容操作
kubectl patch pvc data-pvc -n production -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# SFS Turbo 挂载验证
kubectl exec -it <pod> -n production -- df -h /mnt/sfs
```

## 故障排查

### 网络问题

```bash
# 🟢 低风险：CCE 网络诊断
# CCE Turbo Pod 无法获取 ENI IP
kubectl describe pod <pod-name> -n production
# 错误：failed to create ENI: quota exceeded
# 解决：提升 ENI 配额或减少 Pod 密度

# Service 无法访问
kubectl get svc -n production
kubectl get endpoints -n production
# 检查 ELB（弹性负载均衡）状态
hcloud ELB ShowLoadBalancer --loadbalancer-id=xxxxx

# 跨节点 Pod 通信失败
# 检查 VPC 路由表
hcloud VPC ListRouteTables --vpc-id=xxxxx
```

### 节点问题

```bash
# 🟢 低风险：节点诊断
# 节点 NotReady
kubectl describe node <node-name> | grep -A10 "Conditions"

# SSH 到节点检查
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago" | grep -i error

# 鲲鹏 ARM 节点特有问题
# 检查内核版本兼容性
uname -r
# EulerOS 2.9 ARM64 需要 kernel >= 4.19.90
```

## 最佳实践

### CCE 特色功能

1. **鲲鹏 ARM 混合部署**：通过节点池 Taint + Pod nodeAffinity 实现 ARM/x86 工作负载分离，参考 [[17-系统基础/01-Linux/12-arm-architecture-k8s-optimization|ARM 架构优化]]
2. **AOM 一站式运维**：集成日志（LTS）、监控（AOM）、告警（SMN），无需自建监控栈
3. **SWR 镜像仓库**：华为云 SWR 支持多区域同步，配合 CCE 实现就近拉取
4. **等保合规**：CCE 支持等保三级/四级，满足金融政务合规要求
5. **CCE Autopilot**：突发流量场景使用 Autopilot 实现秒级弹性，无需管理节点

### 生产建议

1. **CCE Turbo 优先**：生产环境推荐 CCE Turbo（VPC 直通），网络性能最优
2. **多 AZ 部署**：控制面和节点跨 AZ 分布
3. **EVS 快照策略**：配置自动快照，RPO < 1 小时
4. **安全组精细化**：CCE Turbo 支持 Pod 级安全组，按服务粒度配置
5. **与 [[18-云厂商/07-多云混合/11-multicloud-comparison-decision-matrix|多云对比]] 配合**：评估混合云需求
6. **参考 [[10-平台工程/03-治理/04-security-compliance|安全合规]] 完善治理**

## Related

- [[18-云厂商/07-多云混合/11-multicloud-comparison-decision-matrix|多云对比决策矩阵]]
- [[18-云厂商/05-腾讯云TKE/05-tke-production-best-practices|腾讯 TKE 生产实践]]
- [[17-系统基础/01-Linux/12-arm-architecture-k8s-optimization|ARM 架构 K8s 优化]]
- [[23-实体/02-K8s核心组件/cni-plugins|CNI 插件]]
- [[22-概念/04-存储/csi-drivers|CSI 驱动]]
- [[10-平台工程/03-治理/04-security-compliance|安全合规]]

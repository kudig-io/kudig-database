---
title: 'P1: ACK 集群生命周期管理'
description: 'title: P1: ACK 集群生命周期管理'
summary: 'title: P1: ACK 集群生命周期管理'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- flannel
- statefulset
- daemonset
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'P1: ACK 集群生命周期管理 是什么'
- '如何 P1: ACK 集群生命周期管理'
trigger_keywords:
- 'P1:'
- ACK
- 集群生命周期管理
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---



---
title: P1: ACK 集群生命周期管理
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK cluster lifecycle management full流程
  - aliyun cs cluster creation deletion upgrade
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] cluster VPC vSwitch network planning
  - ACK cluster certificate renewal
  - Cluster upgrade replacement strategy
trigger_keywords:
  - cluster lifecycle
  - create cluster
  - delete cluster
  - upgrade cluster
  - VPC
  - vSwitch
  - certificate
  - CIDR
  - kubeconfig
reading_level: intermediate
audience:
  - ACK beginners
  - DevOps engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-creation
  - cluster-deletion
  - cluster-upgrade
  - cluster-certificate
---

# P1: ACK 集群生命周期管理

> **对应周次**: Week 1 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐

---

## 概述

本项目将带你完成 ACK 集群的完整生命周期管理：从网络规划、集群创建、节点池配置、集群升级到最终删除清理。全程使用 aliyun CLI + 控制台双通道操作，帮助你建立对集群全生命周期的实操经验。

---

## 项目目标

独立完成 ACK 集群的全生命周期操作：从规划网络、创建集群、升级版本到最终删除清理，全程使用 aliyun CLI + 控制台双通道操作。

## 前置条件

- [ ] 完成 Week 1 全部教案 (Day 1-7)
- [ ] 已安装并配置 aliyun CLI
- [ ] 拥有测试账号的 RAM 权限 (cs:FullAccess)
- [ ] 了解 VPC/vSwitch 网络基础

---

## 核心概念

### 集群生命周期全景

```
规划阶段: 网络规划 → 集群类型选择 → 版本选择
  ↓
创建阶段: VPC/vSwitch → 集群创建 → 节点池创建 → 组件安装
  ↓
运维阶段: 监控接入 → 升级管理 → 证书轮换 → 节点维护
  ↓
退役阶段: 业务清理 → 集群删除 → 资源回收
```

### 网络规划要点

| 网段 | 建议范围 | 说明 |
|------|---------|------|
| VPC CIDR | 172.16.0.0/12 | 底层网络基础，需包含所有 vSwitch |
| Pod CIDR | 10.0.0.0/16 (Flannel) | Flannel 模式专用，决定 Pod IP 范围 |
| [[Service|Service]] CIDR | 192.168.0.0/16 | ClusterIP 范围，创建后不可修改 |
| vSwitch CIDR | 172.16.0.0/24 | 每个可用区一个，需预留足够 IP |

---

## 实施步骤

### Step 1: 网络规划与 VPC 创建 (30min)

#### 1.1 规划网络 CIDR

```bash
# 网络规划参数
VPC_CIDR="172.16.0.0/12"
POD_CIDR="10.0.0.0/16"
SERVICE_CIDR="192.168.0.0/16"
VSW_A_CIDR="172.16.0.0/24"
VSW_B_CIDR="172.16.1.0/24"
REGION="cn-hangzhou"
ZONE_A="cn-hangzhou-h"
ZONE_B="cn-hangzhou-i"
```

#### 1.2 创建 VPC

```bash
# 创建 VPC (如无可用 VPC)
VPC_RESULT=$(aliyun vpc CreateVpc \
  --RegionId $REGION \
  --CidrBlock $VPC_CIDR \
  --VpcName "ack-training-vpc" \
  --Description "ACK training cluster VPC")

VPC_ID=$(echo "$VPC_RESULT" | jq -r '.VpcId')
echo "VPC 创建成功: $VPC_ID"

# 等待 VPC 可用
aliyun vpc DescribeVpcAttribute --VpcId $VPC_ID --RegionId $REGION | jq '.Status'
# "Available"
```

#### 1.3 创建 vSwitch (至少 2 个可用区)

```bash
# 创建 vSwitch A
VSW_A_RESULT=$(aliyun vpc CreateVSwitch \
  --RegionId $REGION \
  --ZoneId $ZONE_A \
  --VpcId $VPC_ID \
  --CidrBlock $VSW_A_CIDR \
  --VSwitchName "ack-training-vsw-a")

VSW_A_ID=$(echo "$VSW_A_RESULT" | jq -r '.VSwitchId')
echo "vSwitch A 创建成功: $VSW_A_ID"

# 创建 vSwitch B
VSW_B_RESULT=$(aliyun vpc CreateVSwitch \
  --RegionId $REGION \
  --ZoneId $ZONE_B \
  --VpcId $VPC_ID \
  --CidrBlock $VSW_B_CIDR \
  --VSwitchName "ack-training-vsw-b")

VSW_B_ID=$(echo "$VSW_B_RESULT" | jq -r '.VSwitchId')
echo "vSwitch B 创建成功: $VSW_B_ID"

# 验证 vSwitch
aliyun vpc DescribeVSwitchAttributes --VSwitchId $VSW_A_ID | jq '{VSwitchId, CidrBlock, ZoneId, Status}'
```

---

### Step 2: 创建 ACK 托管版集群 (30min)

#### 2.1 通过 API 创建集群

```bash
CLUSTER_RESULT=$(aliyun cs POST /clusters --body '{
  "name": "training-cluster-01",
  "cluster_type": "ManagedKubernetes",
  "kubernetes_version": "1.28.9-aliyun.1",
  "region_id": "cn-hangzhou",
  "vpcid": "'"$VPC_ID"'",
  "container_cidr": "10.0.0.0/16",
  "service_cidr": "192.168.0.0/16",
  "vswitch_ids": ["'"$VSW_A_ID"'", "'"$VSW_B_ID"'"],
  "num_of_nodes": 0,
  "endpoint_public_access": true,
  "snat_entry": true,
  "addons": [
    {"name": "flannel"},
    {"name": "csi-plugin"},
    {"name": "csi-provisioner"},
    {"name": "nginx-ingress-controller"},
    {"name": "metrics-server"},
    {"name": "ack-node-problem-detector"}
  ]
}')

CLUSTER_ID=$(echo "$CLUSTER_RESULT" | jq -r '.cluster_id')
echo "集群创建已提交: $CLUSTER_ID"
```

#### 2.2 查看集群创建进度

```bash
# 持续查看创建状态
watch -n 10 "aliyun cs GET /clusters/$CLUSTER_ID | jq '{state, current_version, size}'"

# 状态变化:
# creating → running (约 10-15 分钟)

# 查看创建日志
aliyun cs GET /clusters/$CLUSTER_ID/logs | jq -r '.[] | "\(.created) \(.log)"'
```

#### 2.3 获取 kubeconfig

```bash
# 获取 kubeconfig
KC=$(aliyun cs GET /k8s/$CLUSTER_ID/user_config)
echo "$KC" | jq -r '.config' > ~/.kube/config-training

# 使用 kubeconfig
export KUBECONFIG=~/.kube/config-training

# 验证连接
kubectl cluster-info
# Kubernetes control plane is running at https://xxx.cs.cn-hangzhou.alicontainer.com:6443

kubectl get nodes
# (此时应该没有节点)
```

---

### Step 3: 添加节点池 (30min)

#### 3.1 创建系统节点池

```bash
SYSTEM_POOL=$(aliyun cs POST /clusters/$CLUSTER_ID/nodepools --body '{
  "nodepool_info": {
    "name": "system-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["'"$VSW_A_ID"'"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 2
  },
  "kubernetes_config": {
    "labels": [
      {"key": "node-role", "value": "system"}
    ],
    "taints": [
      {"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}
    ]
  },
  "management": {
    "auto_upgrade": true,
    "auto_repair": true
  }
}')

SYSTEM_POOL_ID=$(echo "$SYSTEM_POOL" | jq -r '.nodepool_id')
echo "系统节点池创建已提交: $SYSTEM_POOL_ID"
```

#### 3.2 创建业务节点池

```bash
APP_POOL=$(aliyun cs POST /clusters/$CLUSTER_ID/nodepools --body '{
  "nodepool_info": {
    "name": "app-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["'"$VSW_A_ID"'", "'"$VSW_B_ID"'"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 2
  },
  "kubernetes_config": {
    "labels": [
      {"key": "node-role", "value": "app"}
    ]
  },
  "auto_scaling": {
    "enable": true,
    "min_size": 0,
    "max_size": 10
  },
  "management": {
    "auto_upgrade": true,
    "auto_repair": true
  }
}')

APP_POOL_ID=$(echo "$APP_POOL" | jq -r '.nodepool_id')
echo "业务节点池创建已提交: $APP_POOL_ID"
```

#### 3.3 等待节点就绪

```bash
# 等待节点 Ready
kubectl get nodes -w
# NAME                                STATUS   ROLES    AGE   VERSION
# cn-hangzhou.172.16.0.x              Ready    <none>   1m    v1.28.9
# cn-hangzhou.172.16.0.y              Ready    <none>   1m    v1.28.9
# cn-hangzhou.172.16.1.x              Ready    <none>   1m    v1.28.9
# cn-hangzhou.172.16.1.y              Ready    <none>   1m    v1.28.9

# 验证节点标签和污点
kubectl get nodes --show-labels
kubectl describe node <node-name> | grep Taints
```

---

### Step 4: 集群升级 (30min)

#### 4.1 查看当前版本

```bash
kubectl version --short
# Client Version: v1.28.9
# Server Version: v1.28.9-aliyun.1
```

#### 4.2 查看可升级版本

```bash
aliyun cs GET /upgrade/cluster/$CLUSTER_ID | jq '.next_versions'
```

#### 4.3 升级管控面 (如有可用升级)

```bash
# 注意: 先在测试环境验证
# TARGET_VERSION 需替换为实际可升级版本

aliyun cs POST /api/v2/clusters/$CLUSTER_ID/upgrade \
  --body '{
    "next_version": "<target-version>"
  }'

# 查看升级状态
aliyun cs GET /clusters/$CLUSTER_ID/upgradestatus | jq '.status'
```

#### 4.4 升级节点 (替换升级)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 通过替换方式升级节点池 (推荐)
# 在控制台: 集群 → 节点池 → 选择节点池 → 升级

# 或通过 API:
# 1. 扩容新节点
# 2. Cordon + Drain 旧节点
# 3. 确认 Pod 迁移
# 4. 移除旧节点

kubectl cordon <old-node>
kubectl drain <old-node> --ignore-daemonsets --delete-emptydir-data
kubectl get pods -A -o wide | grep <old-node>
```

---

### Step 5: 集群删除与清理 (30min)

#### 5.1 删除前检查

```bash
# 检查业务工作负载
kubectl get deployments -A
kubectl get statefulsets -A

# 检查 LoadBalancer 类型 Service (会产生 SLB 费用)
kubectl get svc -A | grep LoadBalancer

# 检查 PVC
kubectl get pvc -A

# 检查 Ingress
kubectl get ingress -A
```

#### 5.2 删除业务资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

```bash
kubectl delete all --all -n default  # ⚠️ 批量删除，波及面大
kubectl delete namespace <business-ns> 2>/dev/null  # ⚠️ 不可逆：永久删除命名空间及全部资源
```

#### 5.3 删除集群

```bash
# 删除集群 (释放所有资源)
aliyun cs DELETE /clusters/$CLUSTER_ID \
  --body '{"retain_all_resources": false}'

# 查看删除进度
aliyun cs GET /clusters/$CLUSTER_ID/logs | tail -20

# 验证删除完成
aliyun cs GET /clusters/$CLUSTER_ID
# 应返回 404
```

#### 5.4 清理 VPC 资源

```bash
# 清理 vSwitch
aliyun vpc DeleteVSwitch --VSwitchId $VSW_A_ID --RegionId $REGION
aliyun vpc DeleteVSwitch --VSwitchId $VSW_B_ID --RegionId $REGION

# 清理 VPC
aliyun vpc DeleteVpc --VpcId $VPC_ID --RegionId $REGION

# 验证清理
aliyun vpc DescribeVpcs --VpcId $VPC_ID 2>/dev/null || echo "VPC 已清理"
```

---

## 验收清单

- [ ] 成功规划网络 CIDR 并创建 VPC/vSwitch
- [ ] 通过 API 创建 ACK 托管版集群
- [ ] 创建了系统节点池和业务节点池
- [ ] 完成管控面升级 (或了解升级流程)
- [ ] 正确删除集群并清理关联资源
- [ ] 全程记录了操作步骤和遇到的问题

---

## 注意事项

| 注意事项 | 说明 |
|----------|------|
| 费用控制 | 创建集群会产生 ECS、SLB 等费用，完成后及时删除 |
| 数据备份 | 集群删除前确认已备份重要数据 |
| CIDR 规划 | 网段规划要预留扩展空间，避免冲突 |
| 删除顺序 | 先删业务资源，再删集群，最后清理网络 |
| 操作记录 | 全程截图或记录命令输出，便于复盘 |

---

## 常见问题

### Q1: 集群创建失败怎么办？

```bash
# 查看创建日志
aliyun cs GET /clusters/$CLUSTER_ID/logs

# 常见原因:
# - vSwitch 不存在或已满
# - ECS 实例库存不足
# - CIDR 冲突
# - RAM 权限不足
```

### Q2: 节点加入失败怎么办？

```bash
# 查看节点状态
kubectl describe node <name>

# 在节点上查看 kubelet 日志
journalctl -u kubelet -n 100

# 检查网络连通性
curl -k https://<api-server>:6443/healthz
```

### Q3: 删除集群时部分资源残留？

```bash
# 手动检查残留资源
aliyun slb DescribeLoadBalancers --VpcId $VPC_ID
aliyun ecs DescribeNetworkInterfaces --VpcId $VPC_ID
aliyun ecs DescribeSecurityGroups --VpcId $VPC_ID

# 手动释放残留资源
aliyun slb DeleteLoadBalancer --LoadBalancerId <slb-id>
```

---

## 要点总结

| 阶段 | 关键操作 | 核心要点 |
|------|---------|---------|
| 规划 | CIDR 设计 | 三段不重叠、预留扩展 |
| 创建 | VPC + 集群 + 节点池 | 组件选型、多可用区 |
| 运维 | 升级 + 维护 | 替换升级、先测后升 |
| 删除 | 清理 + 回收 | 先删业务、确认资源释放 |

---

## 延伸阅读

- [ACK 集群管理](../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md)
- K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [集群生命周期管理](../../domain-07-platform-engineering/02-cluster-lifecycle-management.md)

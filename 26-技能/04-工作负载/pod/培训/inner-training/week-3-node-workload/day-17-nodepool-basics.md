---
title: 'Day 17: 节点池基础'
description: '- 自管理节点池'
summary: '节点池（NodePool）是阿里云 ACK 的核心概念之一，它将一组具有相同配置的节点组织在一起进行统一管理。在传统的 K8s 集群中，管理员需要逐个管理节点，这在节点数量较多时非常低效。节点池解决了这个问题——你可以通过一个配置来管理数十甚至数百个节点，包括它们的实例规格、网络配置、标签和污点。'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- prometheus
- coredns
- containerd
- hpa
- ingress
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 17: 节点池基础 是什么'
- '如何 Day 17: 节点池基础'
trigger_keywords:
- Day
- '17:'
- 节点池基础
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- iac-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 17: 节点池基础
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK node pool concept architecture
  - ACK managed vs self-managed node pool
  - Node pool creation configuration
  - Node pool scaling management
  - Multi-layer node pool architecture design
trigger_keywords:
  - nodepool
  - 节点池
  - managed nodepool
  - 托管节点池
  - self-managed nodepool
  - 自管理节点池
  - auto repair
  - auto upgrade
  - 节点池架构
  - scaling
  - 扩缩容
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-3-node
  - 云厂商
  - 故障诊断
related_topics:
  - nodepool-advanced
  - node-basics
  - ack-ecs-compute
---

# Day 17: 节点池基础

## 概述

节点池（NodePool）是阿里云 ACK 的核心概念之一，它将一组具有相同配置的节点组织在一起进行统一管理。在传统的 K8s 集群中，管理员需要逐个管理节点，这在节点数量较多时非常低效。节点池解决了这个问题——你可以通过一个配置来管理数十甚至数百个节点，包括它们的实例规格、网络配置、标签和污点。

理解节点池对于 ACK 运维至关重要，因为几乎所有的节点管理操作（扩容、缩容、升级、维护）都是以节点池为粒度进行的。

### 学习目标

- 理解节点池的概念、价值和架构设计
- 掌握节点池的创建与核心参数配置
- 深入理解托管节点池与自管理节点池的区别与选型
- 能够通过控制台和 API 管理节点池

---

## 核心概念详解

### 什么是节点池

节点池本质上是一组"同质"的节点——它们具有相同的实例规格、相同的系统镜像、相同的网络配置、相同的标签和污点。当你需要新增节点时，只需增加节点池的期望数量，ACK 会自动创建并配置新的 ECS 实例并将其加入集群。

节点池的核心价值：

- **批量管理**: 一次配置应用到所有节点，无需逐个操作
- **自动伸缩**: 配置自动伸缩策略后，节点池可以根据工作负载需求自动增减节点
- **统一升级**: 节点池内的节点可以统一进行操作系统补丁和 K8s 版本升级
- **故障自愈**: 托管节点池可以自动检测和替换问题节点
- **分层隔离**: 通过创建不同的节点池，将系统组件、不同业务的工作负载隔离到不同的节点上

### 节点池核心参数

创建节点池时需要配置以下几类参数：

**实例规格（Instance Type）** 决定了节点的计算能力。在 ACK 中，常用的实例系列包括：

| 实例系列 | CPU:内存比 | 适用场景 | 典型规格 | 参考月费 |
|---------|-----------|---------|---------|---------|
| 通用型 g7 | 1:4 | Web 应用、微服务 | ecs.g7.xlarge (4C16G) | ~500 元 |
| 计算型 c7 | 1:2 | 计算密集、批处理 | ecs.c7.2xlarge (8C16G) | ~650 元 |
| 内存型 r7 | 1:8 | 缓存、数据库、大数据 | ecs.r7.2xlarge (8C64G) | ~1100 元 |
| GPU 型 gn7 | GPU + CPU | AI 推理、ML 训练 | ecs.gn7i-c16g1.4xlarge | ~8000 元 |
| 本地盘型 i3 | 本地 NVMe | 高 IO 数据库 | ecs.i3.2xlarge (8C32G) | ~900 元 |
| 突发性能型 t6 | 基线 + 突发 | 开发测试 | ecs.t6-c1m2.large (2C4G) | ~80 元 |

**网络配置** 包括 VPC 和 vSwitch 的选择。建议在不同可用区各配置一个 vSwitch，这样节点池可以在多个可用区中创建节点，实现跨可用区的高可用。

**系统盘和数据盘** 配置。系统盘建议使用 ESSD（PL0 或 PL1），大小至少 100GB。如果应用需要本地存储，可以额外挂载数据盘。

| 盘类型 | 性能等级 | 最大 IOPS | 最大吞吐 | 适用场景 |
|-------|---------|----------|---------|---------|
| ESSD PL0 | 基础 | 10,000 | 180 MB/s | 系统盘、低 IO |
| ESSD PL1 | 标准 | 50,000 | 350 MB/s | 通用业务 |
| ESSD PL2 | 高性能 | 100,000 | 700 MB/s | 数据库、高 IO |
| ESSD PL3 | 极致 | 1,000,000 | 4,000 MB/s | 核心数据库 |

**[[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 配置** 包括节点名称模式、标签（Labels）、污点（Taints）和用户数据（User Data）。标签用于调度——Pod 可以通过 nodeSelector 或 nodeAffinity 指定调度到带有特定标签的节点池。污点用于排斥——没有设置对应 Toleration 的 Pod 不会被调度到该节点池。

### 托管节点池 vs 自管理节点池

这是 ACK 中最重要的架构选择之一。

**托管节点池（Managed NodePool）** 的核心特性：

- **自动修复（Auto Repair）**: 当节点连续 NotReady 超过设定时间（默认 20 分钟），系统自动创建新节点替换问题节点。替换过程：创建新节点 → 新节点 Ready → Drain 问题节点上的 Pod → 释放问题节点
- **自动升级（Auto Upgrade）**: 可以配置自动进行操作系统安全补丁更新和 K8s 版本升级。支持配置维护窗口（如每周六凌晨 2:00-6:00），升级操作只在维护窗口内执行
- **CVE 自动修复**: 当发现影响节点安全的高危漏洞时，系统自动在维护窗口内应用补丁
- **推荐配置**: 生产环境强烈推荐使用托管节点池，以减少日常运维工作量

**自管理节点池（Self-Managed NodePool）** 的特点：

- **完全控制**: 用户负责节点的所有运维操作，包括故障恢复、安全补丁、版本升级
- **定制灵活**: 可以在 User Data 中自定义节点的初始化脚本，安装特定的软件包或进行特殊的系统配置
- **适用场景**: 需要特殊的内核参数调优、自定义操作系统镜像、安装特定的硬件驱动等

| 特性 | 托管节点池 | 自管理节点池 |
|------|-----------|-------------|
| 节点自动修复 | 支持 | 不支持 |
| 自动升级 | 支持 | 不支持 |
| 自定义初始化 | 有限支持 | 完全自定义 |
| 运维负担 | 低 | 高 |
| 推荐场景 | 生产环境 | 特殊需求 |
| 问题响应时间 | 自动 20 分钟 | 人工响应 |
| OS 补丁管理 | 自动 | 手动 |
| K8s 版本升级 | 自动 | 手动 |

### 节点池架构设计

生产环境的节点池架构通常采用"分层隔离"设计：

**系统节点池**: 专门运行 K8s 系统组件和基础服务（如 [[CoreDNS|CoreDNS]]、Ingress Controller、Prometheus、日志采集 Agent）。建议配置：

- 实例规格: ecs.g6.xlarge（4C16G）或更高
- 节点数量: 至少 2 个（高可用）
- 污点: `CriticalAddonsOnly=true:NoSchedule`（阻止业务 Pod 调度）
- 标签: `node-role=system`

**业务节点池**: 运行应用工作负载。可以根据业务类型进一步细分：

- 在线服务池: 运行 Web 应用、API 服务等延迟敏感型应用
- 离线任务池: 运行批处理、数据分析等 CPU 密集型任务
- 特殊硬件池: 运行需要 GPU、本地盘等特殊硬件的工作负载

**弹性节点池**: 配置自动伸缩的节点池，用于应对突发流量。可以与 Spot 实例结合以降低成本。

生产环境节点池设计示例：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────┐
│                    ACK Cluster                          │
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ System Pool  │  │  App Pool    │  │ Elastic Pool │  │
│  │  ecs.g6.2xl  │  │ ecs.g6.xl    │  │ ecs.g6.xl    │  │
│  │  2 nodes     │  │ 3-10 nodes   │  │ 0-20 nodes   │  │
│  │  AZ-a, AZ-b  │  │ AZ-a, AZ-b   │  │ Spot 实例    │  │
│  │  Taint:      │  │  Label:      │  │  Auto Scale  │  │
│  │  SystemOnly  │  │  workload=   │  │  Min:0 Max:20│  │
│  │              │  │  app         │  │              │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│                                                         │
│  ┌──────────────┐  ┌──────────────┐                     │
│  │  Data Pool   │  │  GPU Pool    │                     │
│  │ ecs.r6.2xl   │  │ ecs.gn7i     │                     │
│  │ 2 nodes      │  │ 2-4 nodes    │                     │
│  │ Local SSD    │  │  Taint:      │                     │
│  │ Taint:       │  │  nvidia.com  │                     │
│  │  data-only   │  │  /gpu=true   │                     │
│  └──────────────┘  └──────────────┘                     │
└─────────────────────────────────────────────────────────┘
```
---

## 实战演练

### 任务 1: 查看和分析现有节点池 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群中的节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools | jq '.[].nodepool_info'

# 示例输出:
# {
#   "name": "system-pool",
#   "id": "np-xxxxxxxxx",
#   "type": "managed",
#   "created": "2024-01-15T10:30:00+08:00"
# }
# {
#   "name": "app-pool",
#   "id": "np-yyyyyyyyy",
#   "type": "managed",
#   "created": "2024-01-15T11:00:00+08:00"
# }

# 查看节点池详情（包含实例规格、节点数量、标签等）
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '{
  name: .nodepool_info.name,
  type: .type,
  count: .status.total_nodes,
  instance_type: .scaling_group.instance_types,
  labels: .kubernetes_config.labels,
  taints: .kubernetes_config.taints
}'

# 示例输出:
# {
#   "name": "system-pool",
#   "type": "managed",
#   "count": 2,
#   "instance_type": ["ecs.g6.2xlarge"],
#   "labels": [
#     {"key": "node-role", "value": "system"},
#     {"key": "workload", "value": "system-components"}
#   ],
#   "taints": [
#     {"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}
#   ]
# }

# 通过 kubectl 查看节点所属的节点池
kubectl get nodes -o custom-columns='NAME:.metadata.name,POOL:.metadata.labels.alibabacloud\.com/nodepool-id,INSTANCE:.metadata.labels.alibabacloud\.com/instance-type'

# 示例输出:
# NAME            POOL            INSTANCE
# node-192-168-0-1   np-xxxxxxxxx   ecs.g6.2xlarge
# node-192-168-0-2   np-xxxxxxxxx   ecs.g6.2xlarge
# node-192-168-1-1   np-yyyyyyyyy   ecs.g6.xlarge
# node-192-168-1-2   np-yyyyyyyyy   ecs.g6.xlarge
# node-192-168-1-3   np-yyyyyyyyy   ecs.g6.xlarge

# 查看每个节点池的节点数量和资源总量
kubectl get nodes -o json | jq -r '
  .items | group_by(.metadata.labels["alibabacloud.com/nodepool-id"]) | 
  .[] | {
    pool: .[0].metadata.labels["alibabacloud.com/nodepool-id"],
    count: length,
    total_cpu: (map(.status.capacity.cpu | tonumber) | add),
    total_memory: (map(.status.capacity.memory | rtrimstr("Ki") | tonumber / 1024 / 1024 | floor) | add | tostring + " Gi")
  }
'

# 示例输出:
# {
#   "pool": "np-xxxxxxxxx",
#   "count": 2,
#   "total_cpu": 16,
#   "total_memory": "64 Gi"
# }
# {
#   "pool": "np-yyyyyyyyy",
#   "count": 3,
#   "total_cpu": 12,
#   "total_memory": "48 Gi"
# }

# 查看节点池自动伸缩配置
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '{
  name: .nodepool_info.name,
  auto_scaling: .auto_scaling,
  management: .management
}'
```
### 任务 2: 创建新节点池 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 API 创建业务节点池
cat > create-nodepool.json << 'EOF'
{
  "nodepool_info": {
    "name": "app-frontend"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "data_disks": [
      {
        "category": "cloud_essd",
        "size": 200
      }
    ],
    "key_pair": "<your-key-pair>",
    "tags": [
      {"key": "Environment", "value": "production"},
      {"key": "Team", "value": "frontend"}
    ]
  },
  "kubernetes_config": {
    "node_name_mode": "customized,fe-,5,suffix",
    "labels": [
      {"key": "workload", "value": "frontend"},
      {"key": "tier", "value": "web"}
    ],
    "taints": [],
    "runtime": "containerd",
    "kubelet_configuration": {
      "maxPods": 110
    }
  },
  "auto_scaling": {
    "enable": false
  },
  "count": 3
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools \
  --body "$(cat create-nodepool.json)"

# 示例输出:
# {
#   "nodepool_id": "np-zzzzzzzzz",
#   "task_id": "task-xxxxxxxxx"
# }

# 查询创建任务状态
aliyun cs GET /clusters/<cluster_id>/nodepools/np-zzzzzzzzz | jq '{
  name: .nodepool_info.name,
  status: .status.state,
  total_nodes: .status.total_nodes,
  healthy_nodes: .status.healthy_nodes
}'

# 等待节点池创建完成
# 监控新节点状态
kubectl get nodes -w | grep fe-

# 示例输出:
# fe-00123   NotReady   <none>   10s   v1.28.3-aliyun.1
# fe-00123   Ready      <none>   85s   v1.28.3-aliyun.1
# fe-00124   NotReady   <none>   5s    v1.28.3-aliyun.1
# fe-00124   Ready      <none>   78s   v1.28.3-aliyun.1
# fe-00125   NotReady   <none>   8s    v1.28.3-aliyun.1
# fe-00125   Ready      <none>   90s   v1.28.3-aliyun.1

# 验证新节点标签
kubectl get nodes -l workload=frontend --show-labels

# 示例输出:
# NAME        STATUS   ROLES    AGE    VERSION            LABELS
# fe-00123    Ready    <none>   5m     v1.28.3-aliyun.1   workload=frontend,tier=web,...
# fe-00124    Ready    <none>   4m     v1.28.3-aliyun.1   workload=frontend,tier=web,...
# fe-00125   Ready    <none>   3m     v1.28.3-aliyun.1   workload=frontend,tier=web,...
```
### 任务 3: 托管节点池配置 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建托管节点池（生产推荐）
cat > managed-nodepool.json << 'EOF'
{
  "nodepool_info": {
    "name": "managed-app-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": ["ecs.g6.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120
  },
  "kubernetes_config": {
    "labels": [
      {"key": "workload", "value": "app"},
      {"key": "managed", "value": "true"}
    ],
    "taints": []
  },
  "management": {
    "auto_repair": true,
    "auto_upgrade": true,
    "upgrade_config": {
      "auto_upgrade_kubelet": true,
      "maintenance_window": {
        "enable": true,
        "maintenance_time": "02:00:00",
        "duration": "4h",
        "weekly_period": "Sat"
      }
    },
    "security_group": "<security-group-id>"
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 2,
    "max_instances": 10
  },
  "count": 3
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools \
  --body "$(cat managed-nodepool.json)"

# 验证托管配置
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '.management'

# 示例输出:
# {
#   "auto_repair": true,
#   "auto_upgrade": true,
#   "upgrade_config": {
#     "auto_upgrade_kubelet": true,
#     "maintenance_window": {
#       "enable": true,
#       "maintenance_time": "02:00:00",
#       "duration": "4h",
#       "weekly_period": "Sat"
#     }
#   }
# }

# 验证自动伸缩配置
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '.auto_scaling'

# 示例输出:
# {
#   "enable": true,
#   "min_instances": 2,
#   "max_instances": 10,
#   "type": "hpa"
# }
```
### 任务 4: 控制台节点池操作 (30min)

```
# ACK 控制台操作流程:
# 1. 登录阿里云控制台 → 容器服务 ACK
# 2. 选择集群 → 节点管理 → 节点池
# 3. 点击"创建节点池"
# 4. 配置:
#    - 基本信息: 名称、类型（托管/自管理）
#    - 实例配置: 规格族、规格、数量
#    - 网络配置: VPC、vSwitch
#    - 系统配置: 系统盘、数据盘
#    - K8s 配置: 标签、污点、运行时
#    - 弹性伸缩: 是否启用、最小/最大实例数
# 5. 点击"确认创建"
```

### 任务 5: 节点池扩缩容操作 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 手动扩容节点池
aliyun cs PUT /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{
    "nodepool_info": {
      "name": "app-frontend"
    },
    "count": 5
  }'

# 观察扩容过程
kubectl get nodes -w | grep fe-

# 手动缩容（移除指定节点）
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<nodepool_id>/nodes \
  --body '{
    "nodes": ["<node-id-1>", "<node-id-2>"],
    "release_node": true,
    "drain_node": true
  }'

# 查看节点池当前状态
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '{
  name: .nodepool_info.name,
  desired: .count,
  total: .status.total_nodes,
  healthy: .status.healthy_nodes,
  state: .status.state
}'
```
---

## 配置参考

### 节点池 JSON 配置模板

```json
{
  "nodepool_info": {
    "name": "<nodepool-name>"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-1>", "<vsw-id-2>"],
    "instance_types": ["<instance-type-1>"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "system_disk_performance_level": "PL1",
    "data_disks": [
      {
        "category": "cloud_essd",
        "size": 200,
        "performance_level": "PL1",
        "encrypted": false
      }
    ],
    "key_pair": "<key-pair-name>",
    "tags": [
      {"key": "Environment", "value": "production"}
    ],
    "platform": "AliyunLinux",
    "image_id": "<custom-image-id>"
  },
  "kubernetes_config": {
    "node_name_mode": "customized,prefix,5,suffix",
    "labels": [
      {"key": "workload", "value": "app"},
      {"key": "environment", "value": "prod"}
    ],
    "taints": [
      {"key": "dedicated", "value": "app", "effect": "NoSchedule"}
    ],
    "runtime": "containerd",
    "kubelet_configuration": {
      "maxPods": 110,
      "registryPullQPS": 5,
      "registryBurst": 10,
      "eventRecordQPS": 50,
      "eventBurst": 100,
      "cpuManagerPolicy": "static",
      "topologyManagerPolicy": "best-effort"
    },
    "user_data": "#!/bin/bash\necho 'Custom init script'"
  },
  "management": {
    "auto_repair": true,
    "auto_upgrade": true,
    "upgrade_config": {
      "auto_upgrade_kubelet": true,
      "maintenance_window": {
        "enable": true,
        "maintenance_time": "02:00:00",
        "duration": "4h",
        "weekly_period": "Sat"
      }
    },
    "security_group": "<security-group-id>"
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 2,
    "max_instances": 10,
    "type": "hpa"
  },
  "count": 3,
  "tee_config": {
    "type": ""
  }
}
```

### 节点池常用 kubectl 查询

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点及其所属节点池
kubectl get nodes -o custom-columns='\
NAME:.metadata.name,\
STATUS:.status.conditions[?(@.type=="Ready")].status,\
POOL:.metadata.labels.alibabacloud\.com/nodepool-id,\
INSTANCE:.metadata.labels.alibabacloud\.com/instance-type,\
ZONE:.metadata.labels.topology\.kubernetes\.io/zone,\
AGE:.metadata.creationTimestamp'

# 查看特定节点池的资源使用情况
kubectl top nodes -l workload=frontend

# 查看节点池中节点的详细信息
kubectl get nodes -l workload=frontend -o json | jq -r '
.items[] | {
  name: .metadata.name,
  cpu_allocatable: .status.allocatable.cpu,
  memory_allocatable: .status.allocatable.memory,
  cpu_capacity: .status.capacity.cpu,
  memory_capacity: .status.capacity.memory,
  pod_count: .status.capacity.pods,
  conditions: [.status.conditions[] | select(.status == "True") | .type]
}
'

# 统计节点池资源总量
kubectl get nodes -o json | jq -r '
.items | group_by(.metadata.labels["alibabacloud.com/nodepool-id"]) |
.[] | {
  pool: (.[0].metadata.labels["alibabacloud.com/nodepool-id"] // "unknown"),
  nodes: length,
  total_cpu: (map(.status.allocatable.cpu | tonumber) | add),
  total_memory_gb: (map(.status.allocatable.memory | rtrimstr("Ki") | tonumber / 1024 / 1024 | floor) | add),
  total_pods: (map(.status.allocatable.pods | tonumber) | add)
}
'
```
### 节点池标签与调度配置

```yaml
# Pod 调度到特定节点池
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      nodeSelector:
        workload: frontend
        tier: web
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "1"
            memory: 1Gi
---
# 使用 nodeAffinity 进行更灵活的调度
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: workload
                operator: In
                values: ["app", "backend"]
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values: ["cn-hangzhou-a", "cn-hangzhou-b"]
      tolerations:
      - key: dedicated
        value: app
        effect: NoSchedule
      containers:
      - name: app
        image: app:v1
```

---

## 常见问题

### Q1: 一个集群应该创建几个节点池？

取决于业务复杂度。最小配置：1 个系统节点池 + 1 个业务节点池。典型生产配置：1 个系统节点池 + 2-3 个业务节点池（按业务类型分）+ 1 个弹性节点池。不建议超过 10 个节点池，过多的节点池会增加管理复杂度。

### Q2: 托管节点池的自动修复会影响业务吗？

自动修复过程中，系统会先 Drain 问题节点上的 Pod（将其优雅地迁移到其他节点），然后再释放问题节点。如果你的应用配置了正确的 readinessProbe 和足够的副本数，自动修复不会影响业务。建议确保每个应用至少有 2 个副本分布在不同节点上。

### Q3: 节点池中的实例规格选错了怎么办？

节点池的实例规格在创建后不能修改。如果需要更换规格，需要创建一个新的节点池（使用新规格），将工作负载迁移到新节点池，然后删除旧节点池。这也是为什么建议在创建节点池前充分评估资源需求。

### Q4: vSwitch 配置有什么注意事项？

建议在节点池中配置多个可用区的 vSwitch，这样节点会分布在不同可用区，提高可用性。确保 vSwitch 的 IP 地址段足够大（建议至少 /22，可分配约 1020 个 IP），避免 IP 耗尽导致新节点无法创建。

### Q5: 如何监控节点池的健康状态？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池状态汇总
aliyun cs GET /clusters/<cluster_id>/nodepools | jq '.[] | {
  name: .nodepool_info.name,
  state: .status.state,
  total: .status.total_nodes,
  healthy: .status.healthy_nodes,
  unhealthy: (.status.total_nodes - .status.healthy_nodes)
}'

# 使用 kubectl 检查节点健康
kubectl get nodes -o json | jq -r '
.items[] | select(.status.conditions[] | select(.type == "Ready" and .status == "False")) |
"\(.metadata.name) is NotReady"
'

# 检查节点资源压力
kubectl get nodes -o json | jq -r '
.items[] | {
  name: .metadata.name,
  disk_pressure: (.status.conditions[] | select(.type == "DiskPressure") | .status),
  memory_pressure: (.status.conditions[] | select(.type == "MemoryPressure") | .status),
  pid_pressure: (.status.conditions[] | select(.type == "PIDPressure") | .status)
} | select(.disk_pressure == "True" or .memory_pressure == "True" or .pid_pressure == "True")
'
```
### Q6: 如何使用 Terraform 管理节点池？

```hcl
resource "alicloud_cs_kubernetes_node_pool" "app_pool" {
  name                 = "app-frontend"
  cluster_id           = alicloud_cs_managed_kubernetes.cluster.id
  vswitch_ids          = [alicloud_vswitch.vsw_a.id, alicloud_vswitch.vsw_b.id]
  instance_types       = ["ecs.g6.xlarge"]
  key_name             = alicloud_key_pair.key.key_name
  system_disk_category = "cloud_essd"
  system_disk_size     = 120
  data_disk_category   = "cloud_essd"
  data_disk_size       = 200
  desired_size         = 3
  pod_cidr             = "172.20.0.0/16"

  labels {
    key   = "workload"
    value = "frontend"
  }

  taints {
    key    = "dedicated"
    value  = "frontend"
    effect = "NoSchedule"
  }

  management {
    auto_repair  = true
    auto_upgrade = true
  }

  auto_scaling {
    min_size = 2
    max_size = 10
  }
}
```

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| 节点池概念 | 一组同质节点的统一管理单元 |
| 核心参数 | 实例规格、网络、标签、污点、伸缩策略 |
| 托管 vs 自管理 | 托管自动修复升级，自管理完全控制 |
| 架构设计 | 系统池 + 业务池 + 弹性池的分层隔离 |
| 创建方式 | API、控制台、Terraform |
| 扩缩容 | 手动调整 count 或配置 auto_scaling |
| 监控 | 通过 API 和 kubectl 查看节点池健康状态 |

---

## 延伸阅读

- [ACK 服务总览](../../云厂商/04-alicloud-ack/alicloud-ack-overview.md)
- [ECS 计算资源](../../云厂商/04-alicloud-ack/240-ack-ecs-compute.md)
- [集群自动伸缩排障](../../故障诊断/28-cluster-autoscaler-troubleshooting.md)
- [K8s 架构与组件](../../集群基础/02-core-components-deep-dive.md)


<!-- risk-assessed -->

---
title: 'Day 4: K8S 新建集群'
description: '**学习时间**: 4-5 小时 | **主题**: 掌握集群创建流程与配置选项'
summary: '**学习时间**: 4-5 小时 | **主题**: 掌握集群创建流程与配置选项'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- flannel
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
- 'Day 4: K8S 新建集群 是什么'
- '如何 Day 4: K8S 新建集群'
trigger_keywords:
- Day
- '4:'
- K8S
- 新建集群
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 4: K8S 新建集群
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK cluster creation process VPC vSwitch
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] cluster network CIDR planning
  - aliyun cs POST clusters API
  - ACK console cluster creation wizard
  - Terway Flannel CNI selection
trigger_keywords:
  - create cluster
  - VPC
  - vSwitch
  - CIDR
  - Pod CIDR
  - [[Service|Service]] CIDR
  - CNI
  - Terway
  - Flannel
  - cluster creation
reading_level: intermediate
audience:
  - ACK beginners
  - DevOps engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-12-cloud-providers
  - domain-6-networking
  - domain-10-troubleshooting-diagnostics
related_topics:
  - ack-overview
  - ack-vpc-network
  - ack-ecs-compute
---

# Day 4: K8S 新建集群

> **学习时间**: 4-5 小时 | **主题**: 掌握集群创建流程与配置选项

---

## 今日目标

- [ ] 掌握 ACK 集群创建的完整参数配置
- [ ] 理解 VPC/vSwitch/安全组等网络前置依赖
- [ ] 能通过控制台和 API 两种方式创建集群
- [ ] 了解不同集群类型的创建差异

---

## 理论学习 (2h)

### 必读文档

1. **ACK 服务总览与集群类型**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: 不同集群类型的创建参数差异

2. **VPC 网络规划**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md`
   - 重点: VPC CIDR 规划、Pod CIDR、Service CIDR

3. **ECS 计算资源**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md`
   - 重点: 实例规格选择、系统盘配置

### 阅读要点

- 创建集群前需要准备: VPC、vSwitch (至少 2 个可用区)、安全组
- CIDR 规划原则: Pod CIDR 与 VPC CIDR 不重叠、预留足够 IP 地址
- 托管版创建参数: K8S 版本、CNI 类型 (Terway/Flannel)、节点池配置
- 节点池参数: 实例规格、系统盘、数据盘、节点数量

---

## 实践任务 (2.5h)

### 任务 1: 网络前置准备 (30min)

```bash
# 创建 VPC (如果没有现成的)
aliyun vpc CreateVpc \
  --CidrBlock 172.16.0.0/12 \
  --VpcName training-vpc \
  --RegionId cn-hangzhou

# 创建 vSwitch (至少 2 个可用区)
aliyun vpc CreateVSwitch \
  --VpcId <vpc_id> \
  --CidrBlock 172.16.0.0/20 \
  --ZoneId cn-hangzhou-h \
  --VSwitchName training-vsw-a

aliyun vpc CreateVSwitch \
  --VpcId <vpc_id> \
  --CidrBlock 172.16.16.0/20 \
  --ZoneId cn-hangzhou-i \
  --VSwitchName training-vsw-b

# 查看安全组
aliyun ecs DescribeSecurityGroups --VpcId <vpc_id>
```

### 任务 2: 通过控制台创建集群 (45min)

```
# 登录 ACK 控制台 -> 创建集群

# 关键配置项:
# 1. 集群配置
#    - 集群名称: training-cluster
#    - K8S 版本: 选择最新稳定版
#    - 地域与可用区: cn-hangzhou
#    - VPC: 选择已创建的 VPC
#    - Pod 虚拟交换机: 选择 vSwitch
#    - Service CIDR: 172.21.0.0/20

# 2. 节点池配置
#    - 实例规格: ecs.g6.xlarge (4C16G)
#    - 系统盘: ESSD 120GB
#    - 节点数量: 3
#    - 登录方式: 密钥对

# 3. 组件配置
#    - CNI: Terway (推荐) 或 Flannel
#    - Ingress: Nginx Ingress Controller
#    - 监控: 安装 ARMS Prometheus
#    - 日志: 安装 Logtail

# 创建后关注:
# - 创建进度和日志
# - 预计创建时间 10-15 分钟
# - 可能的失败原因
```

### 任务 3: 通过 API 创建集群 (45min)

```bash
# 使用 aliyun CLI 创建集群
cat > create-cluster.json << 'EOF'
{
  "name": "training-cluster-api",
  "cluster_type": "ManagedKubernetes",
  "kubernetes_version": "1.28.9-aliyun.1",
  "region_id": "cn-hangzhou",
  "vpcid": "<vpc_id>",
  "container_cidr": "10.0.0.0/16",
  "service_cidr": "172.21.0.0/20",
  "num_of_nodes": 3,
  "master_vswitch_ids": [],
  "worker_vswitch_ids": ["<vsw_id_a>", "<vsw_id_b>"],
  "worker_instance_types": ["ecs.g6.xlarge"],
  "worker_system_disk_category": "cloud_essd",
  "worker_system_disk_size": 120,
  "login_password": "<password>",
  "addons": [
    {"name": "terway-eniip"},
    {"name": "csi-plugin"},
    {"name": "csi-provisioner"},
    {"name": "nginx-ingress-controller"}
  ]
}
EOF

aliyun cs POST /clusters --body "$(cat create-cluster.json)"

# 查看创建进度
aliyun cs GET /clusters/<new_cluster_id>/logs
```

### 任务 4: 验证集群状态 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config > kubeconfig.yaml
export KUBECONFIG=./kubeconfig.yaml

# 验证集群连接
kubectl cluster-info
kubectl get nodes -o wide
kubectl get pods -n kube-system

# 检查关键组件状态
kubectl get pods -n kube-system -l app=flannel  # 或 terway
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl get pods -n kube-system -l app=metrics-server

# 检查集群版本
kubectl version
```
---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **创建 ACK 集群前需要准备哪些网络资源？为什么需要多可用区？**
   - 提示: VPC、vSwitch、安全组; 高可用需要跨可用区部署

2. **Pod CIDR 和 Service CIDR 的规划原则是什么？**
   - 提示: 不与 VPC CIDR 重叠、预留足够 IP、考虑集群规模

3. **集群创建失败时，最常见的原因有哪些？**
   - 提示: 资源库存不足、CIDR 冲突、权限不足、配额限制

---

## 今日检验

- [ ] 能说出创建集群需要的前置网络资源
- [ ] 能通过控制台创建一个完整的 ACK 集群
- [ ] 能通过 API 创建集群并查看创建日志
- [ ] 能验证集群创建成功并获取 kubeconfig

---

## 核心概念总结

| 参数 | 说明 | 注意事项 |
|------|------|---------|
| VPC/vSwitch | 集群网络基础 | 多可用区部署保证高可用 |
| Pod CIDR | Pod 网络地址段 | 不能与 VPC CIDR 重叠 |
| Service CIDR | Service 虚拟 IP 段 | 创建后不可修改 |
| CNI 类型 | Terway 或 Flannel | 创建后不可更改，影响网络性能 |
| 节点池 | Worker 节点分组管理 | 按业务需求设计节点池 |

---

## 明日预告

Day 5 将学习集群删除流程，理解资源清理和依赖关系处理。


<!-- risk-assessed -->

---
title: 'Day 2: ACK SDK & API'
description: '## 概述'
summary: 'ACK 提供了完整的 OpenAPI 接口，支持通过 aliyun CLI、Python SDK、Java SDK 等多种方式调用。掌握 API 调用是自动化运维的基础——从集群创建到节点管理，从组件安装到证书轮换，所有操作都可以通过 API 完成。今天你将学习 ACK API 的认证机制、核心接口分类，'
category: learning
tags:
- k8s
- training
- hands-on
- controller-manager
- prometheus
- flannel
- coredns
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 2: ACK SDK & API 是什么'
- '如何 Day 2: ACK SDK & API'
trigger_keywords:
- Day
- '2:'
- ACK
- SDK
- API
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
title: Day 2: ACK SDK & API
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK OpenAPI SDK Python JavaScript
  - aliyun cs GET POST DELETE API calls
  - ACK API authentication AK SK STS
  - aliyun CLI installation configuration
  - DescribeClusterUserKubeconfig API
trigger_keywords:
  - SDK
  - API
  - OpenAPI
  - aliyun CLI
  - Python SDK
  - authentication
  - AK
  - SK
  - STS
  - RAM role
  - cluster management
reading_level: intermediate
audience:
  - Developers
  - DevOps engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - 云厂商
  - 集群基础
related_topics:
  - ack-overview
  - ack-openapi
  - ack-ram-authorization
---

# Day 2: ACK SDK & API

> **学习时间**: 4-5 小时 | **主题**: ACK SDK 使用与 API 调用方式

---

## 概述

ACK 提供了完整的 OpenAPI 接口，支持通过 aliyun CLI、Python SDK、Java SDK 等多种方式调用。掌握 API 调用是自动化运维的基础——从集群创建到节点管理，从组件安装到证书轮换，所有操作都可以通过 API 完成。今天你将学习 ACK API 的认证机制、核心接口分类，以及如何使用 aliyun CLI 和 Python SDK 进行实际操作。

---

## 今日目标

- [ ] 掌握 ACK OpenAPI 核心接口
- [ ] 能够使用 aliyun CLI 调用 ACK API
- [ ] 理解 SDK 认证方式 (AK/SK、STS Token、RAM 角色)
- [ ] 能够通过 API 完成集群信息查询

---

## 核心概念

### 1. ACK API 认证方式对比

| 认证方式 | 安全等级 | 适用场景 | 有效期 |
|----------|---------|---------|--------|
| AK/SK (AccessKey) | 中 | 服务端程序、脚本自动化 | 永久 (需手动轮换) |
| STS Token | 高 | 临时授权、跨账号访问 | 15min - 12h |
| RAM 角色 (ECS) | 高 | ECS 上运行的应用 | 自动轮换 |
| OIDC | 最高 | [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] ServiceAccount | 短期自动 |

### 2. ACK API 核心接口分类

| 分类 | 核心接口 | 说明 |
|------|---------|------|
| 集群管理 | CreateCluster, DeleteCluster, DescribeClusterDetail | 集群 CRUD |
| 节点管理 | DescribeClusterNodes, RemoveClusterNodes | 节点查询和移除 |
| 节点池 | CreateClusterNodePool, ModifyClusterNodePool, DescribeClusterNodePools | 节点池管理 |
| 组件管理 | DescribeClusterAddonsVersion, InstallClusterAddons, UninstallClusterAddons | 组件安装卸载 |
| kubeconfig | DescribeClusterUserKubeconfig | 获取连接凭证 |
| 升级 | UpgradeCluster, DescribeClusterUpgradeStatus | 版本升级 |
| 证书 | RenewClusterCertificate | 证书轮换 |

### 3. API 签名机制

ACK API 基于 ROA 风格 (RESTful)，请求需要经过签名才能通过认证:

```
请求流程:
1. 构造规范化请求字符串 (CanonicalizedQueryString)
2. 构造待签名字符串 (StringToSign)
3. 使用 AK/SK 计算 HMAC-SHA1 签名
4. 将签名 Base64 编码后放入 Authorization 头
```

---

## 理论学习 (2h)

### 必读文档

1. **ACK OpenAPI 概览**
   - 文件: `../../../云厂商/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: API 接口分类、调用方式、签名机制

2. **ACK RAM 授权**
   - 文件: `../../../云厂商/04-alicloud-ack/243-ack-ram-authorization.md`
   - 重点: RAM 策略与 ACK API 权限映射

---

## 实战演练 (2.5h)

### 任务 1: aliyun CLI 配置与基础 API 调用 (45min)

#### 1.1 安装和配置 aliyun CLI

```bash
# 安装 aliyun CLI (macOS)
brew install aliyun-cli

# 配置认证信息
aliyun configure set \
  --profile default \
  --mode AK \
  --access-key-id <your-ak> \
  --access-key-secret <your-sk> \
  --region cn-hangzhou

# 验证配置
aliyun configure list
```

示例输出:

```
Profile   Mode         AccessKey Id        Region
default   AK           LTAI5t***********   cn-hangzhou
```

#### 1.2 集群管理 API

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群列表
aliyun cs GET /api/v1/clusters

# 示例输出:
# [
#   {
#     "cluster_id": "c-xxxxxxxxxxxxx",
#     "name": "production-cluster",
#     "cluster_type": "ManagedKubernetes",
#     "current_version": "1.28.9-aliyun.1",
#     "state": "running",
#     "region_id": "cn-hangzhou",
#     "size": 5,
#     "network_mode": "terway"
#   }
# ]

# 查看集群详情
aliyun cs GET /clusters/<cluster_id>

# 查看集群 kubeconfig (JSON 格式)
aliun cs GET /k8s/<cluster_id>/user_config

# 保存 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config-ack

# 合并到默认 kubeconfig
export KUBECONFIG=~/.kube/config:~/.kube/config-ack
kubectl config view --flatten > ~/.kube/config.merged
mv ~/.kube/config.merged ~/.kube/config

# 查看集群创建日志
aliyun cs GET /clusters/<cluster_id>/logs
```
#### 1.3 集群列表 jq 过滤

```bash
# 只显示集群 ID 和名称
aliyun cs GET /api/v1/clusters | jq '.[] | {cluster_id, name, state, current_version}'

# 过滤 running 状态的集群
aliyun cs GET /api/v1/clusters | jq '.[] | select(.state=="running")'

# 按节点数量排序
aliyun cs GET /api/v1/clusters | jq -r '.[] | "\(.size)\t\(.name)\t\(.cluster_id)"' | sort -n
```

---

### 任务 2: 节点与节点池 API (45min)

```bash
# 查看集群节点列表
aliyun cs GET /clusters/<cluster_id>/nodes

# 示例输出 (关键字段):
# {
#   "nodes": [
#     {
#       "node_name": "cn-hangzhou.192.168.0.1",
#       "instance_id": "i-xxxxxxxxxx",
#       "instance_type": "ecs.g6.xlarge",
#       "instance_role": "Worker",
#       "state": "ready",
#       "ip_address": ["192.168.0.1"]
#     }
#   ],
#   "page": { "total_count": 5, "page_number": 1, "page_size": 10 }
# }

# 查看节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools

# 查看节点池详情
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>

# 节点池详情关键字段说明:
# {
#   "nodepool_info": {
#     "name": "system-pool",              // 节点池名称
#     "type": "ess"                        // 类型: ess(弹性伸缩)
#   },
#   "scaling_group": {
#     "vswitch_ids": ["vsw-xxx"],         // 交换机 ID
#     "instance_types": ["ecs.g6.xlarge"],// 实例规格
#     "system_disk_category": "cloud_essd",// 系统盘类型
#     "system_disk_size": 120,            // 系统盘大小
#     "desired_size": 2,                  // 期望节点数
#     "min_size": 0,                      // 最小节点数
#     "max_size": 10                      // 最大节点数
#   },
#   "kubernetes_config": {
#     "labels": [{"key": "node-role", "value": "system"}],
#     "taints": [{"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}]
#   },
#   "auto_scaling": {
#     "enable": true                      // 是否启用自动伸缩
#   },
#   "management": {
#     "auto_upgrade": true,               // 自动升级
#     "auto_repair": true                 // 自动修复
#   }
# }

# 查看节点池中节点状态
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>/nodes
```

---

### 任务 3: 组件管理 API (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群已安装组件
aliyun cs GET /clusters/<cluster_id>/components

# 示例输出:
# {
#   "name": "coredns",
#   "state": "installed",
#   "version": "1.9.3",
#   "description": "DNS 服务"
# }

# 查看可用组件版本和升级状态
aliyun cs GET /clusters/<cluster_id>/components/upgradestatus

# 常见核心组件列表:

# | 组件名 | 说明 | 必装 |
# |--------|------|------|
# | coredns | 集群 DNS 服务 | 是 |
# | metrics-server | 指标采集 (kubectl top) | 是 |
# | cloud-controller-manager | 云资源控制器 | 是 |
# | csi-plugin | CSI 存储插件 | 是 |
# | csi-provisioner | CSI 存储供应器 | 是 |
# | terway-eniip | Terway 网络插件 (Terway 模式) | 二选一 |
# | flannel | Flannel 网络插件 (Flannel 模式) | 二选一 |
# | nginx-ingress-controller | Nginx Ingress 控制器 | 推荐 |
# | ack-node-problem-detector | 节点问题检测 | 推荐 |
# | arms-prometheus | ARMS Prometheus 监控 | 可选 |
# | logtail-ds | 日志采集 | 可选 |
```
---

### 任务 4: SDK 调用实践 (30min)

#### 4.1 Python SDK 示例

```python
# 安装 SDK
# pip install alibabacloud_cs20151215 alibabacloud_tea_openapi

from alibabacloud_cs20151215.client import Client
from alibabacloud_tea_openapi.models import Config
from alibabacloud_tea_util.models import RuntimeOptions
import json

# 初始化客户端
config = Config(
    access_key_id='<your-ak>',
    access_key_secret='<your-sk>',
    region_id='cn-hangzhou'
)
client = Client(config)
runtime = RuntimeOptions()

# 查看集群列表
response = client.describe_clusters_v1()
for cluster in response.body.clusters:
    print(f"集群: {cluster.name}")
    print(f"  ID: {cluster.cluster_id}")
    print(f"  状态: {cluster.state}")
    print(f"  版本: {cluster.current_version}")
    print(f"  类型: {cluster.cluster_type}")
    print(f"  节点数: {cluster.size}")
    print(f"  网络: {cluster.network_mode}")
    print()

# 查看集群详情
detail = client.describe_cluster_detail('<cluster_id>')
print(f"集群类型: {detail.body.cluster_type}")
print(f"节点数量: {detail.body.size}")
print(f"VPC ID: {detail.body.vpc_id}")
print(f"Pod CIDR: {detail.body.subnet_cidr}")
print(f"Service CIDR: {detail.body.service_cidr}")

# 获取 kubeconfig
kubeconfig = client.describe_cluster_user_kubeconfig(
    cluster_id='<cluster_id>',
    runtime=runtime
)
with open('/tmp/ack-kubeconfig', 'w') as f:
    f.write(kubeconfig.body.config)
```

#### 4.2 批量集群巡检脚本

```bash
cat > cluster_inspect.sh << 'SCRIPT'
#!/bin/bash

REGIONS=("cn-hangzhou" "cn-shanghai" "cn-beijing" "cn-shenzhen")

echo "=========================================="
echo "ACK 集群巡检报告"
echo "时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="
echo ""

for region in "${REGIONS[@]}"; do
  echo "--- 地域: $region ---"
  
  clusters=$(aliyun cs GET /api/v1/clusters --region "$region" 2>/dev/null)
  
  if [ -z "$clusters" ] || [ "$clusters" = "[]" ]; then
    echo "  无集群"
    echo ""
    continue
  fi

  echo "$clusters" | jq -r '.[] | "\(.cluster_id)\t\(.name)\t\(.state)\t\(.current_version)\t\(.size)"' | \
  while IFS=$'\t' read -r id name state version size; do
    echo "  集群: $name ($id)"
    echo "    状态: $state | 版本: $version | 节点数: $size"
    
    if [ "$state" != "running" ]; then
      echo "    [警告] 集群状态异常: $state"
    fi
    
    if [ "$size" -eq 0 ] 2>/dev/null; then
      echo "    [警告] 集群无节点"
    fi
    
    echo ""
  done
done

echo "=========================================="
echo "巡检完毕"
echo "=========================================="
SCRIPT

chmod +x cluster_inspect.sh
./cluster_inspect.sh
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK API 的认证方式有哪几种？各自适合什么场景？**
   - 提示: AK/SK 适合服务端，STS 适合临时授权，RAM 角色适合 ECS 上的应用

2. **如何通过 API 获取集群的 kubeconfig？获取后如何使用？**
   - 提示: DescribeClusterUserKubeconfig 接口，保存到 ~/.kube/config

3. **查看集群节点池信息时，哪些字段最重要？为什么？**
   - 提示: scaling_group 决定节点规格，auto_scaling 决定弹性能力

4. **为什么生产环境建议使用 RAM 角色而不是 AK/SK 硬编码？**
   - 提示: 安全性、自动轮换、无需代码中存放凭证

---

## 今日检验

- [ ] 能使用 aliyun CLI 查询集群、节点、组件信息
- [ ] 理解 ACK API 的认证和签名机制
- [ ] 能说出至少 5 个核心 ACK API 接口的用途
- [ ] 能使用 SDK (Python/Java) 编写简单的集群查询脚本

---

## 配置参考

### aliyun CLI 常用配置

```bash
# 多 profile 管理
aliyun configure set --profile prod --mode AK --access-key-id xxx --access-key-secret yyy --region cn-hangzhou
aliyun configure set --profile staging --mode AK --access-key-id xxx --access-key-secret yyy --region cn-shanghai

# 切换 profile
aliyun configure set --profile prod

# 使用 ECS RAM 角色 (在 ECS 上运行)
aliyun configure set --profile ecs-role --mode EcsRamRole --ram-role-name <role-name> --region cn-hangzhou
```

### API 调用频率限制

| API 类别 | 频率限制 | 说明 |
|----------|---------|------|
| 查询类 (GET) | 100 QPS | DescribeClustersV1 等 |
| 操作类 (POST/PUT/DELETE) | 10 QPS | CreateCluster 等 |
| kubeconfig 获取 | 5 QPS | DescribeClusterUserKubeconfig |

### 常用 RAM 策略

```json
{
  "Version": "1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "cs:DescribeClustersV1",
        "cs:DescribeClusterDetail",
        "cs:DescribeClusterNodes",
        "cs:DescribeClusterNodePools",
        "cs:DescribeClusterAddonsVersion",
        "cs:DescribeClusterUserKubeconfig"
      ],
      "Resource": "*"
    }
  ]
}
```

---

## 常见问题

### Q1: API 返回 Forbidden 怎么办？

检查 RAM 用户是否有对应的 API 权限。ACK API 需要 RAM 层面授权，不同操作需要不同 Action。只读操作只需要 `cs:Describe*` 权限。

### Q2: 如何使用 STS Token 调用 API？

```bash
# 获取 STS Token
aliyun sts AssumeRole --RoleArn acs:ram::xxx:role/xxx --RoleSessionName test --DurationSeconds 3600

# 使用 STS Token 配置 CLI
aliyun configure set --profile sts --mode StsToken \
  --access-key-id <sts-ak> \
  --access-key-secret <sts-sk> \
  --sts-token <sts-token> \
  --region cn-hangzhou
```

### Q3: SDK 调用超时如何处理？

```python
from alibabacloud_tea_util.models import RuntimeOptions

runtime = RuntimeOptions(
    read_timeout=10000,
    connect_timeout=10000,
    max_attempts=3,
    autoretry=True
)
response = client.describe_clusters_v1(runtime=runtime)
```

---

## 要点总结

| 概念 | 说明 | 生产注意事项 |
|------|------|--------------|
| OpenAPI | ACK 对外提供的 RESTful API | 注意 API 频率限制 |
| AK/SK | 永久访问凭证 | 生产环境避免硬编码，使用 RAM 角色 |
| STS Token | 临时安全令牌 | 有过期时间，适合跨账号场景 |
| aliyun CLI | 命令行调用工具 | 日常运维和脚本自动化首选 |
| SDK | 编程语言 SDK | 适合自动化系统和工具开发 |
| RAM 角色 | ECS 实例角色 | 最安全的方式，自动轮换凭证 |

---

## 明日预告

Day 3 将学习 ACK/ACR 控制台操作，熟悉界面功能入口和核心操作流程。

---

## 延伸阅读

- [ACK 服务总览](../../云厂商/04-alicloud-ack/alicloud-ack-overview.md)
- [ACK RAM 授权](../../云厂商/04-alicloud-ack/243-ack-ram-authorization.md)
- [ACK OpenAPI 文档](https://help.aliyun.com/document_detail/260907.html)


<!-- risk-assessed -->

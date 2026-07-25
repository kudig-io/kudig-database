---
title: 'Day 1: ACK/ACR 管控 SR'
description: '- 服务架构'
summary: '作为内部培训的第一天，本课程将系统性地介绍阿里云容器服务 ACK（Alibaba Cloud [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]]）和容器镜像服务 ACR（Alibaba Cloud Container Registry）的服务架构、产品形态和管控层组件。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- controller-manager
- flannel
- coredns
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 1: ACK/ACR 管控 SR 是什么'
- '如何 Day 1: ACK/ACR 管控 SR'
trigger_keywords:
- Day
- '1:'
- ACK
- ACR
- 管控
- SR
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 1: ACK/ACR 管控 SR
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - ACK ACR [[Service|service]] architecture overview
  - ACK managed dedicated serverless cluster types
  - ACR personal enterprise edition difference
  - ACK SR service request handling process
  - ACK cluster manager meta-service
trigger_keywords:
  - ACK
  - ACR
  - 服务架构
  - 托管版
  - 专有版
  - Serverless
  - 产品形态
  - SR
  - 服务请求
  - 管控层
reading_level: beginner
audience:
  - All learners
  - New joiners
  - Anyone interested in ACK/ACR
estimated_read_time: 45min
related_domains:
  - 云厂商
  - 集群基础
related_topics:
  - ack-overview
  - ack-cluster-types
  - ack-acr-integration
---

# Day 1: ACK/ACR 管控 SR

> **学习时间**: 4-5 小时 | **主题**: ACK/ACR 服务架构与管控层基本概念

---

## 概述

作为内部培训的第一天，本课程将系统性地介绍阿里云容器服务 ACK（Alibaba Cloud [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]]）和容器镜像服务 ACR（Alibaba Cloud Container Registry）的服务架构、产品形态和管控层组件。理解 ACK/ACR 的整体架构是后续所有运维工作的基础——只有了解系统是如何构建的，才能在出现问题时快速定位和解决。

本课程还将介绍内部 SR（Service Request，服务请求）的处理流程，这是日常工作中最频繁接触的任务类型。理解 SR 的分类、优先级和常见场景，能帮助你在接到用户请求时快速判断处理方向。

**学习目标**：
- 理解 ACK 服务架构（托管版、专有版、Serverless 三种形态）
- 理解 ACR 服务架构（个人版、企业版）
- 掌握 ACK/ACR 管控层组件和工作流程
- 了解内部 SR (Service Request) 处理流程

**前置条件**：
- 有阿里云基础操作经验
- 了解容器和 Kubernetes 基本概念
- 有 Linux 命令行基础

---

## 核心概念

### ACK 产品形态

阿里云容器服务 ACK 提供三种产品形态，满足不同场景的需求：

#### 三种形态对比

| 维度 | 托管版 (Managed) | 专有版 (Dedicated) | Serverless (ASK) |
|------|-----------------|-------------------|------------------|
| **Master 节点** | 阿里云托管 | 用户自建 | 无需 Master |
| **Worker 节点** | 用户管理 | 用户管理 | 无需 Worker |
| **计费方式** | 按节点 + 管控费 | 按节点 | 按 Pod |
| **运维负担** | 低 | 高 | 最低 |
| **自定义程度** | 中 | 高 | 低 |
| **适用场景** | 大多数生产环境 | 需要深度定制 | 突发流量/CI-CD |
| **控制平面 SLA** | 99.95% | 用户负责 | 99.95% |
| **K8s 版本支持** | 1.24-1.33 | 1.24-1.33 | 1.24-1.33 |

#### 托管版架构详解

```
┌─────────────────────────────────────────────────────┐
│                    阿里云托管层                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │API Server│  │Scheduler │  │Ctrl Mgr  │           │
│  └──────────┘  └──────────┘  └──────────┘           │
│  ┌──────────┐                                       │
│  │  etcd    │  ← 多可用区部署，自动备份               │
│  └──────────┘                                       │
│  ┌──────────┐                                       │
│  │Cloud Ctrl│  ← 云资源管理 (SLB/ECS/NAS/OSS)       │
│  └──────────┘                                       │
└─────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│                    用户 VPC                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │ Worker 1 │  │ Worker 2 │  │ Worker 3 │  ...      │
│  │ kubelet  │  │ kubelet  │  │ kubelet  │           │
│  │ kube-proxy│  │ kube-proxy│  │ kube-proxy│         │
│  └──────────┘  └──────────┘  └──────────┘           │
└─────────────────────────────────────────────────────┘
```

### ACR 产品形态

#### 个人版 vs 企业版对比

| 功能 | 个人版 | 企业版 |
|------|--------|--------|
| **实例类型** | 共享实例 | 独享实例 |
| **镜像存储** | 共享存储 | 独立存储 |
| **安全扫描** | 手动触发 | 自动扫描 |
| **多地域同步** | 不支持 | 支持 |
| **镜像签名** | 不支持 | 支持 (Notation) |
| **P2P 加速** | 不支持 | 支持 |
| **访问控制** | 基础 RAM | 细粒度 RAM + 镜像级别 |
| **SBOM 生成** | 不支持 | 支持 |
| **合规报告** | 不支持 | CIS 基准报告 |
| **费用** | 免费 | 按实例规格付费 |
| **适用场景** | 个人开发/测试 | 企业生产环境 |

### 管控层组件

ACK 管控层负责集群的全生命周期管理，核心组件包括：

| 组件 | 功能 | 关键操作 |
|------|------|---------|
| **cluster-manager** | 集群生命周期管理 | 创建/删除/升级集群 |
| **meta-service** | 元数据管理 | 集群配置/组件版本管理 |
| **addon-manager** | 组件管理 | 安装/更新/删除集群组件 |
| **node-controller** | 节点管理 | 节点加入/移除/修复 |
| **cloud-controller** | 云资源管理 | SLB/云盘/NAS 等资源管理 |

### SR 处理流程

| 优先级 | 定义 | 响应 SLA | 示例 |
|--------|------|---------|------|
| **P1** | 生产问题，业务受影响 | 15 分钟 | 集群不可用、Pod 大面积异常 |
| **P2** | 功能异常，有 workaround | 2 小时 | 节点添加失败、升级异常 |
| **P3** | 使用咨询/功能建议 | 8 小时 | 最佳实践咨询、配置指导 |
| **P4** | 文档问题 | 24 小时 | 文档错误、描述不清晰 |

---

## 实战演练

### 任务 1: 了解 ACK 集群类型 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 配置 aliyun CLI
aliyun configure
# 输入 AccessKey ID 和 Secret

# Step 2: 查看当前账号下的集群列表
aliyun cs GET /api/v1/clusters

# 预期输出:
# [
#   {
#     "cluster_id": "c-abc123456789",
#     "name": "production-cluster",
#     "cluster_type": "ManagedKubernetes",
#     "state": "running",
#     "current_version": "1.30.1-aliyun.1",
#     "region_id": "cn-hangzhou",
#     "vpc_id": "vpc-xxx",
#     "size": 5,
#     "node_status": "running",
#     "created": "2025-12-01T10:00:00+08:00"
#   }
# ]

# Step 3: 查看单个集群详情
aliyun cs GET /clusters/<cluster_id>

# 关注返回字段:
# - cluster_type: ManagedKubernetes(托管版) / Kubernetes(专有版) / Ask(Serverless)
# - state: running / creating / upgrading / deleting
# - current_version: K8s 版本
# - meta_data: 管控面元数据 (API Server 地址、端口等)

# Step 4: 查看 K8s 版本支持列表
aliyun cs GET /api/v1/versions

# Step 5: 连接到集群
# 下载 kubeconfig:
# ACK 控制台 → 集群详情 → 连接信息 → 复制 kubeconfig

mkdir -p ~/.kube
# 将 kubeconfig 内容写入 ~/.kube/config
kubectl get nodes

# 预期输出:
# NAME            STATUS   ROLES    AGE   VERSION
# node-worker-1   Ready    worker   30d   v1.30.1
# node-worker-2   Ready    worker   30d   v1.30.1
```
### 任务 2: 了解 ACR 实例 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看 ACR 个人版仓库列表
aliyun cr GET /repos

# 预期输出:
# {
#   "data": {
#     "repos": [
#       {
#         "repoName": "my-app",
#         "repoNamespace": "my-team",
#         "repoBuildType": "LOCAL",
#         "repoType": "PUBLIC"
#       }
#     ]
#   }
# }

# Step 2: 查看 ACR 企业版实例列表
aliyun cr ListInstance

# 预期输出:
# {
#   "Instances": [
#     {
#       "InstanceId": "cri-xxx",
#       "InstanceName": "enterprise-registry",
#       "InstanceType": "Enterprise",
#       "InstanceStatus": "RUNNING",
#       "RegionId": "cn-hangzhou"
#     }
#   ]
# }

# Step 3: 对比功能差异
# 登录 ACR 个人版:
docker login --username=xxx registry.cn-hangzhou.aliyuncs.com

# 登录 ACR 企业版:
docker login --username=xxx enterprise-registry-registry.cn-hangzhou.cr.aliyuncs.com

# Step 4: 推送镜像到 ACR
docker tag nginx:1.25-alpine registry.cn-hangzhou.aliyuncs.com/my-team/my-app:v1.0
docker push registry.cn-hangzhou.aliyuncs.com/my-team/my-app:v1.0
```
### 任务 3: 梳理管控层架构 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看 kube-system 中的核心组件
kubectl get pods -n kube-system

# 预期输出:
# NAME                                       READY   STATUS    RESTARTS   AGE
# coredns-66f5b8f7f5-abc12                   1/1     Running   0          30d
# csi-plugin-xxxxx                           1/1     Running   0          30d
# csi-provisioner-xxxxx                      1/1     Running   0          30d
# kube-proxy-worker-1                        1/1     Running   0          30d
# kube-proxy-worker-2                        1/1     Running   0          30d
# terway-eniip-xxxxx                         1/1     Running   0          30d

# Step 2: 查看组件版本
kubectl get pods -n kube-system -o custom-columns='NAME:.metadata.name,IMAGE:.spec.containers[0].image'

# Step 3: 组件功能说明
# coredns: DNS 解析服务
# metrics-server: 指标采集 (kubectl top)
# csi-plugin: 存储插件 (云盘/NAS/OSS)
# terway/flannel: CNI 网络插件
# kube-proxy: Service 负载均衡
# cloud-controller-manager: 云资源管理 (SLB/路由)

# Step 4: 查看集群附加组件
aliyun cs GET /clusters/<cluster_id>/components
```
### 任务 4: 内部 SR 流程熟悉 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# === SR 分类与处理 ===

# P1: 集群创建失败
# 排查路径: VPC → vSwitch → 安全组 → ECS 库存 → RAM 权限
# 关键命令:
aliyun cs GET /clusters/<cluster_id> | jq '.state'
aliyun cs GET /clusters/<cluster_id>/logs  # 查看创建日志

# P2: 节点添加失败
# 排查路径: ECS 库存 → 节点规格 → 网络配置 → 初始化脚本
# 关键命令:
aliyun cs GET /clusters/<cluster_id>/nodes
kubectl get nodes -o wide

# P3: 集群升级问题
# 排查路径: 版本兼容性 → 组件状态 → etcd 健康 → 节点状态
# 关键命令:
aliyun cs GET /clusters/<cluster_id>/upgradestatus
kubectl get cs

# P4: 镜像拉取失败
# 排查路径: ACR 权限 → 网络策略 → 镜像地址 → 凭证配置
# 关键命令:
kubectl get pods -A | grep ImagePullBackOff
kubectl describe pod <pod> | grep -A 10 Events
```
---

## 配置参考

### ACK 集群创建参数

```json
{
  "name": "production-cluster",
  "cluster_type": "ManagedKubernetes",
  "kubernetes_version": "1.30.1-aliyun.1",
  "region_id": "cn-hangzhou",
  "vpcid": "vpc-xxx",
  "vswitch_ids": ["vsw-xxx1", "vsw-xxx2"],
  "security_group_id": "sg-xxx",
  "container_cidr": "172.20.0.0/16",
  "service_cidr": "172.21.0.0/20",
  "snat_entry": true,
  "cloud_monitor_flags": true,
  "addons": [
    {"name": "terway-eniip"},
    {"name": "csi-plugin"},
    {"name": "csi-provisioner"},
    {"name": "ack-node-problem-detector"},
    {"name": "coredns"}
  ],
  "nodepool": {
    "nodepool_name": "default-pool",
    "node_count": 3,
    "node_spec": "ecs.g7.xlarge",
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "image_type": "AliyunLinux3",
    "key_pair": "my-keypair"
  }
}
```

### 参数说明表

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `cluster_type` | 集群类型 | ManagedKubernetes |
| `kubernetes_version` | K8s 版本 | 最新稳定版 |
| `container_cidr` | Pod CIDR | 不与 VPC/Service CIDR 冲突 |
| `service_cidr` | Service CIDR | 不与 VPC/Pod CIDR 冲突 |
| `snat_entry` | 开启 SNAT | true（VPC 访问公网需要） |
| `node_spec` | 节点规格 | ecs.g7.xlarge (4C16G) 起步 |
| `system_disk_size` | 系统盘大小 | >= 120GB |
| `image_type` | 操作系统 | AliyunLinux3 (推荐) |

---

## 常见问题

### Q1: ACK 托管版和专有版的核心区别是什么？

**A**: 核心区别在于控制平面的管理责任：
- **托管版**: Master 节点由阿里云托管和维护，用户只需管理 Worker 节点。控制平面 SLA 99.95%
- **专有版**: 用户自行管理所有节点（包括 Master），灵活性最高但运维负担最大
- **建议**: 除非有特殊的合规要求或需要深度定制控制平面，否则选择托管版

### Q2: 当用户报告"集群创建失败"时，第一步排查思路是什么？

**A**: 按资源依赖链路排查：
1. **VPC/vSwitch**: 确认 VPC 存在、vSwitch 有可用 IP、可用区正确
2. **安全组**: 确认安全组存在、规则允许必要端口
3. **RAM 权限**: 确认调用者有创建集群的权限
4. **ECS 库存**: 确认指定规格的 ECS 有库存
5. **查看创建日志**: `aliyun cs GET /clusters/<id>/logs`

### Q3: ACR 企业版相比个人版多了哪些关键能力？

**A**: 企业版核心增强：
1. **安全**: 自动镜像扫描、镜像签名 (Notation)、SBOM 生成
2. **可用性**: 多地域同步、P2P 加速分发、独享实例
3. **管理**: 细粒度访问控制、镜像版本不可变、垃圾回收
4. **合规**: CIS 基准报告、审计日志

### Q4: 如何查看集群的管控面状态？

**A**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ACK 控制台查看
# 控制台 → 容器服务 → 集群列表 → 点击集群名 → 集群信息

# API 查看集群状态
aliyun cs GET /clusters/<cluster_id> | jq '{state, current_version, size}'

# 查看组件健康
kubectl get cs
kubectl get pods -n kube-system
```
### Q5: kube-system 中的组件哪些由阿里云管理，哪些由用户管理？

**A**: 在 ACK 托管版中：
- **阿里云管理**: etcd、API Server、Scheduler、Controller Manager、cloud-controller-manager
- **用户管理 (kube-system 中)**: coredns、kube-proxy、csi-plugin、terway/flannel、metrics-server
- 用户的 kube-system 组件出现问题，需要用户自行排查或提交 SR

---

## 要点总结

- **ACK 三种形态**: 托管版（推荐）、专有版（深度定制）、Serverless（按 Pod 计费）
- **ACR 两个版本**: 个人版（免费，开发测试）、企业版（付费，生产推荐）
- **管控层**负责集群生命周期管理，理解其工作流程有助于排障定位
- **SR 四个优先级**: P1(15min) > P2(2h) > P3(8h) > P4(24h)
- 集群创建失败按 **VPC → vSwitch → 安全组 → ECS → RAM** 链路排查
- **kube-system 组件**版本需要与 K8s 版本兼容

---

## 延伸阅读

- [ACK 产品文档](https://help.aliyun.com/product/85222.html)
- [ACR 产品文档](https://help.aliyun.com/product/60716.html)
- [Kubernetes 架构概述](https://kubernetes.io/docs/concepts/architecture/)
- [文件: `../../../云厂商/04-alicloud-ack/alicloud-ack-overview.md`](../../../云厂商/04-alicloud-ack/alicloud-ack-overview.md)
- [文件: `../../../云厂商/04-alicloud-ack/service-ack-practical-guide.md`](../../../云厂商/04-alicloud-ack/service-ack-practical-guide.md)
- [文件: `../../../集群基础/01-kubernetes-architecture-overview.md`](../../../集群基础/01-kubernetes-architecture-overview.md)

---

## 明日预告

Day 2 将学习 ACK SDK 和 API 的使用方式，掌握通过编程方式管理集群资源。


<!-- risk-assessed -->

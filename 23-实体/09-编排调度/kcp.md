---
title: kcp
description: '## 概述'
summary: 'kcp 是一个类 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 服务器，提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kcp
- etcd
- rbac
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kcp 是什么
- 如何 kcp
trigger_keywords:
- kcp
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kcp

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

kcp 是一个类 Kubernetes API 服务器，由 Upbound 和 Red Hat 等团队推动开发，2021 年作为 CNCF 沙箱项目加入。它提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。kcp 利用 Kubernetes 的 API 机制（CRD、控制器、准入控制等），将其从容器编排中解耦出来，作为通用的 API 平台使用。kcp 支持在单个服务器上运行数千个逻辑集群（Workspace），每个 Workspace 拥有独立的 API 视图和资源隔离。这使得 kcp 非常适合作为 SaaS 平台的控制平面、多租户 API 服务或自定义控制器平台。

## 核心能力

- **逻辑集群（Workspace）**: 单进程内运行数千个逻辑集群，每个 Workspace 拥有独立的资源视图
- **APIExport/APIBinding**: 将自定义 API 跨 Workspace 共享和绑定，实现平台 API 的组合
- **多租户隔离**: 基于 Workspace 的强隔离，RBAC 细粒度权限控制
- **Syncer 机制**: 将逻辑集群中的资源同步到物理 Kubernetes 集群执行
- **标准 Kubernetes API**: 完全兼容 kubectl、client-go、CRD、Webhook 等原生工具链
- **轻量级**: 单二进制部署，无需完整控制面栈，适合嵌入式场景

## 架构

kcp 的核心架构围绕 "API as a Platform" 理念设计：

- **kcp server**: 单一进程，内嵌 etcd 和 Kubernetes API 服务器逻辑，管理所有 Workspace
- **Workspace**: 逻辑隔离单元，类似于虚拟集群，拥有独立的 Namespace 和资源
- **APIExport**: Workspace 声明可对外暴露的 API 资源（如自定义 CRD）
- **APIBinding**: 消费者 Workspace 绑定其他 Workspace 暴露的 API
- **Syncer**: 部署在物理集群中的 Agent，监听逻辑集群中的资源并同步到实际集群
- **Workload API**: 跨 Workspace 管理工作负载的生命周期

架构模式：`kcp (逻辑控制面) → Syncer → 物理 Kubernetes 集群 (执行面)`

## K8s 集成

kcp 本身就是一个精简版的 Kubernetes API 服务器，完全兼容 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 规范。通过 kubectl 和标准 client-go 库直接操作。Syncer 组件以 Deployment 方式部署在物理 Kubernetes 集群中，将 kcp 逻辑集群中的 Deployment、StatefulSet 等资源同步到实际集群运行。kcp 支持 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的所有原生资源类型，可通过 CRD 扩展自定义资源。

## 生产场景

1. **SaaS 平台控制面**: 为每个租户提供独立的逻辑集群，通过 APIExport 统一管理平台 API
2. **多团队开发平台**: 大型组织为每个团队分配独立 Workspace，集中管控但互不干扰
3. **自定义控制器平台**: 在 kcp 上运行业务控制器，无需完整 Kubernetes 集群
4. **混合云管理**: 通过 Syncer 将工作负载分发到不同云厂商的物理集群

## 安装与配置

```bash
# 安装 kcp
kubectl krew install kcp
kcp start
# 或从源码安装
git clone https://github.com/kcp-dev/kcp.git
cd kcp && make build
./bin/kcp start

# 配置 kubectl 连接 kcp
export KUBECONFIG=$(pwd)/.kcp/admin.kubeconfig
kubectl get workspaces
```

```bash
# 创建 Workspace
kubectl kcp workspace create my-team --type=universal
kubectl kcp workspace use my-team

# 在 Workspace 中创建资源
kubectl create namespace app
kubectl apply -f deployment.yaml

# APIExport/APIBinding 示例
kubectl apply -f - <<EOF
apiVersion: apis.kcp.io/v1alpha1
kind: APIExport
metadata:
  name: my-platform-api
spec:
  latestResourceSchemas:
    - widgets.myplatform.io
EOF
```

## 运维操作

```bash
# 🟢 查看 Workspace 状态
kubectl get workspaces
kubectl kcp workspace tree

# 🟢 查看 API 导出和绑定
kubectl get apiexports -A
kubectl get apibindings -A

# 🟢 检查 etcd 状态
etcdctl --endpoints=http://localhost:2379 endpoint health
etcdctl --endpoints=http://localhost:2379 endpoint status --write-out=table

# 🟡 切换 Workspace
kubectl kcp workspace use <workspace-name>

# 🟡 重启 kcp
# Ctrl+C 停止后重新 kcp start

# 🔴 删除 Workspace（级联删除所有资源）
kubectl delete workspace <name>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Workspace 无法访问 | 权限不足/Workspace 不存在 | `kubectl get workspaces` | 检查 RBAC 和 Workspace 名称 |
| APIBinding 失败 | APIExport 不存在/版本不匹配 | `kubectl describe apibinding <name>` | 检查 APIExport 和 ResourceSchema |
| Syncer 同步失败 | 物理集群不可达/RBAC 不足 | `kubectl logs -n kcp-syncer` | 检查 kubeconfig 和权限 |
| etcd 连接失败 | 内嵌 etcd 未启动 | 检查 kcp 启动日志 | 重启 kcp 或检查端口占用 |
| CRD 注册失败 | Workspace 类型不支持 | `kubectl api-resources` | 确认 Workspace 类型支持 CRD |

```
排查流程：
├─ Workspace 问题
│  ├─ kubectl get workspaces 检查状态
│  ├─ 检查当前 Workspace 上下文
│  └─ 检查 RBAC 权限
├─ API 共享问题
│  ├─ 检查 APIExport 是否 Ready
│  ├─ 检查 APIBinding 状态
│  └─ 确认 ResourceSchema 版本匹配
└─ Syncer 问题
   ├─ 检查 Syncer Pod 状态
   ├─ 检查物理集群连接
   └─ 检查同步资源 RBAC
```

## 生产案例

### 案例 1：SaaS 平台多租户控制面

- **场景**: SaaS 平台需要为 1000+ 租户提供独立的 API 视图，传统方案需 1000 个 K8s 集群
- **排查**: 评估 vCluster/Capsule，均需要宿主集群且资源开销大
- **方案**: 使用 kcp 单进程运行 1000+ Workspace，通过 APIExport 统一管理平台 API
- **效果**: 单节点承载所有租户控制面，资源成本降低 95%

### 案例 2：混合云工作负载分发

- **场景**: 工作负载需要分发到 AWS/GCP/本地 3 个物理集群
- **排查**: 传统多集群管理复杂度高，需要统一控制面
- **方案**: kcp 作为逻辑控制面，通过 Syncer 将工作负载同步到各物理集群
- **效果**: 统一 API 视图管理多云工作负载，运维复杂度降低 70%

## 对比

| 维度 | kcp | vCluster | Capsule | KubeStellar |
|------|-----|----------|---------|-------------|
| 隔离方式 | 逻辑 Workspace | 虚拟集群 | Namespace 聚合 | 多集群分发 |
| 原生 API | ✅ 完全兼容 | ✅ 完全兼容 | ⚠️ 共享 API | ✅ |
| 无需物理集群 | ✅ 单进程 | ❌ 需宿主 | ❌ 需宿主 | ❌ 需多集群 |
| 适用场景 | API 平台 | 开发测试 | 多租户隔离 | 多集群管理 |

## 架构定位

在 CNCF 生态中，kcp 属于 **Orchestration** 类别，为云原生应用提供关键的多租户 API 平台能力。

## 参考链接

- [[etcd]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[flatcar]] — Flatcar Container Linuxux 生产环境速查卡|Linux]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kcp
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->

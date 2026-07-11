---
title: Metal3
description: 'summary: "Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。"'
summary: 'Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力，实现裸金属即服务。'
category: general
tags:
- k8s
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Metal3 是什么
- 如何 Metal3
trigger_keywords:
- Metal3
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Metal3

> **CNCF 状态**: Incubating | **类别**: Metal/Bare Metal | **主要语言**: Go

## 概述

Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力，由 Nordix、Equinor Metal、Red Hat 等推动开发，2021 年加入 CNCF 孵化。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"（Bare Metal as a Service）。Metal3 将裸金属服务器抽象为 Kubernetes 原生资源（BareMetalHost CRD），通过 IPMI/Redfish BMC 协议控制服务器的开机/关机/重启，通过 Ironic（OpenStack 组件）进行 PXE 网络启动和操作系统安装。这使得裸金属服务器的管理可以像虚拟机一样通过 Kubernetes API 声明式操作，是私有云 Kubernetes 基础设施管理的关键组件。

## 核心能力

- **Kubernetes 原生**: 通过 BareMetalHost CRD 声明式管理裸金属服务器
- **Cluster API 集成**: 与 Cluster API 统一的集群生命周期管理
- **自动发现**: 通过 IPMI/Redfish 发现和注册裸金属服务器
- **配置管理**: 基于 Ironic 自动化 PXE 启动和操作系统安装
- **生命周期管理**: 开机、关机、重装、回收等裸金属全生命周期
- **无代理**: 使用 BMC 协议（IPMI/Redfish），无需在服务器上安装任何代理

## 架构

Metal3 基于 Ironic + Cluster API 构建：

- **Metal3 BareMetal Operator**: 管理 BareMetalHost CRD 的生命周期
- **BareMetalHost CRD**: 裸金属服务器抽象，定义 BMC 地址、硬件规格、镜像、用户数据
- **Ironic (conductor)**: 核心裸金属供应引擎，执行 PXE/iPXE 启动和 OS 安装
- **Ironic Inspector**: 硬件自动发现和规格检测
- **Metal3 Provider (CAPMVM)**: Cluster API 的 Metal3 Provider，将 BareMetalHost 与 Cluster API Machine 关联
- **BMC (Baseboard Management Controller)**: 服务器主板上的远程管理接口（IPMI/Redfish）

供应流程：`BareMetalHost CRD → Operator → Ironic (PXE) → OS 安装 → 节点 Ready`

## K8s 集成

Metal3 以 Kubernetes Operator 方式运行。管理集群中部署 Metal3 BareMetal Controller 和 Ironic，通过 BareMetalHost CRD 管理裸金属服务器。每个 BareMetalHost 定义了 BMC 地址和凭据（通过 Secret 引用）、硬件规格和目标镜像。Controller 调用 Ironic 执行 PXE 启动和 OS 安装。与 Cluster API 集成时，CAPMVM Provider 将 BareMetalHost 与 Cluster API Machine 关联，实现裸金属上 Kubernetes 集群的自动化部署。与 [[kubernetes-architecture-overview|Kubernetes 架构]] 中的 Node、Machine 等资源统一管理。

## 生产场景

1. **私有云裸金属集群**: 在裸金属服务器上自动化部署 Kubernetes 集群（替代 vSphere/OpenStack）
2. **裸金属弹性扩容**: 根据负载自动开机新服务器并加入集群
3. **裸金属回收**: 节点下线时自动清除数据并恢复到可用状态
4. **多租户裸金属**: 为不同团队分配专用裸金属服务器，通过 BMC 物理隔离

## 安装

```bash
# 安装 Metal3 baremetal-operator
kubectl apply -f https://github.com/metal3-io/baremetal-operator/releases/latest/download/baremetal-operator.yaml

# 注册裸金属服务器
kubectl apply -f - <<EOF
apiVersion: metal3.io/v1alpha1
kind: BareMetalHost
metadata:
  name: worker-1
spec:
  bmc:
    address: ipmi://192.168.1.100
    credentialsName: bmc-credentials
  bootMACAddress: 00:11:22:33:44:55
  online: true
  image:
    url: http://image-server/centos.qcow2
    checksum: sha256:abc123...
  userData:
    namespace: metal3
    name: worker-user-data
    key: userData
EOF

# 创建 BMC 凭据
kubectl create secret generic bmc-credentials \
  --from-literal=username=admin --from-literal=password=password
```

## 对比

| 特性 | Metal3 | Tinkerbell | MAAS | Ironic (standalone) |
|------|--------|-----------|------|---------------------|
| K8s 原生 | ✅ CRD | ✡ | ❌ | ❌ |
| Cluster API | ✅ | ✅ | ❌ | ❌ |
| BMC 支持 | ✅ IPMI/Redfish | ✅ | ✅ | ✅ |
| CNCF 状态 | Incubating | Sandbox | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Metal3 属于 **Metal/Bare Metal** 类别，为云原生应用提供裸金属基础设施管理能力。

## 参考链接

- [[deployment]]
- [[crd-custom-resources]]
- [[operator-pattern]]
- [[controller-pattern]]
- [[secrets-management]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- index/node-index|Node 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

---
title: urunc
description: '## 概述'
summary: 'urunc 是一个符合 OCI 标准的容器运行时，专门用于在 Kubernetes 中运行 Unikernel 应用。Unikernel 是将应用与最小化操作系统库编译为单一镜像的技术，具有极小的攻击面、亚毫秒级启动时间和极低的内存占用。urunc 将 Unikernel 打包为 OCI 镜像，'
category: entities
tags:
- k8s
- cncf
- runtime
- urunc
- containerd
- cri-o
- gateway
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- urunc 是什么
- 如何 urunc
trigger_keywords:
- urunc
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# urunc

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

urunc 是由 NEC 实验室开源的符合 OCI 标准的容器运行时，专门用于在 Kubernetes 中运行 Unikernel 应用，2022 年加入 CNCF Sandbox。Unikernel 是将应用与最小化操作系统库编译为单一可执行镜像的技术，具有极小的攻击面、亚毫秒级启动时间和极低的内存占用。urunc 将 Unikernel 打包为 OCI 镜像，使其能够通过标准的容器工作流（containerd、CRI-O）在 Kubernetes 上部署和管理。

## 核心特性

- **OCI 标准**: 完全兼容 OCI Runtime Specification
- **多 Unikernel 框架**: Unikraft、MirageOS、IncludeOS、HermitCore
- **多 VMM 后端**: Firecracker、QEMU、solo5-hvt
- **标准容器工作流**: 通过 containerd/CRI-O 管理 Unikernel Pod
- **RuntimeClass**: 通过 Kubernetes RuntimeClass 选择 urunc 运行时
- **镜像兼容**: Unikernel 打包为标准 OCI 镜像

## 架构

urunc 作为 OCI Runtime 实现（类似 runc），由 containerd/CRI-O 通过 CRI 接口调用。当 Pod 指定 `runtimeClassName: urunc` 时，kubelet 通过 CRI 调用 containerd，containerd 通过 containerd-shim 调用 urunc。urunc 从 OCI 镜像中提取 Unikernel 二进制文件和配置，调用 VMM（Firecracker 或 QEMU）启动 micro-VM 运行 Unikernel。Unikernel 在 VM 内以单地址空间运行，无操作系统内核开销。urunc 负责 Pod 生命周期管理（创建、启动、停止、状态查询）。

## Kubernetes 集成

urunc 通过 RuntimeClass 与 Kubernetes 集成。节点安装 urunc 二进制和 containerd-shim 插件后，创建 RuntimeClass `urunc`。Pod spec 中指定 `runtimeClassName: urunc` 即可使用 Unikernel 运行时。Unikernel 镜像通过标准 OCI Distribution 分发。在混合部署场景中，容器使用 runc/crun，Unikernel 使用 urunc，通过 RuntimeClass 灵活选择。支持标准的 Pod API（环境变量、卷挂载通过 virtio 传递）。

## 生产使用场景

1. **安全敏感微服务**: 使用 Unikernel 的极小攻击面运行安全敏感服务
2. **Serverless 函数**: 亚毫秒级启动适合高密度函数计算
3. **边缘 IoT**: 极低内存占用适配资源受限的边缘设备
4. **合规隔离**: 利用 VM 级隔离满足严格的安全合规要求

## 安装

```bash
# 安装 urunc
git clone https://github.com/nickurunc/urunc && cd urunc && make install
# 配置 containerd
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.urunc]
  runtime_type = "io.containerd.urunc.v2"
# 创建 RuntimeClass
kubectl apply -f - <<EOF
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata: { name: urunc }
handler: urunc
EOF
# 运行 Unikernel Pod
kubectl run unikernel-app --image=unikraft/helloworld:latest --overrides='{"spec":{"runtimeClassName":"urunc"}}'
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **urunc** | OCI 标准、Unikernel 专业化 | 社区小、生态不成熟 |
| Kata Containers | VM 级隔离、成熟 | 资源开销较大 |
| gVisor | 用户态内核 | 性能开销 |
| Wasm (SpinKube) | 极速启动 | 非 VM 隔离 |

## 架构定位

在 CNCF 生态中，urunc 属于 **Runtime / Unikernel** 类别，是将 Unikernel 技术引入 Kubernetes 标准工作流的桥梁。

## 参考链接

- [[containerd]]
- [[概念/container-runtime-comparison.md|container-runtime-comparison]]
- [[pod-lifecycle]]

## Related

- [[dex]] — Dex
- [[kgateway]] — kgateway
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[cri-o]] — CRI-O

- urunc
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.1
- RELEASE-NOTES-0.0
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- [[实体/flatcar.md|[[Flatcar Container Linux|Flatcar Container Linux]]ux 生产环境速查卡|Linux]]]]
- [[实体/composefs.md|composefs]]
- [[实体/04-containerd-upgrade-migration.md|containerd 升级迁移]]
- [[实体/wasmedge.md|WasmEdge]]
- [[实体/spinkube.md|SpinKube]]
- [[实体/05-containerd-windows-support.md|containerd Windows 支持]]
- [[实体/02-containerd-v2-features.md|containerd 2.0 新特性]]
- [[实体/08-containerd-multi-tenant.md|containerd 多租户]]
- [[实体/k0s.md|K0s]]
- [[实体/03-containerd-security-hardening.md|containerd 安全加固]]
- [[实体/bootc.md|bootc]]
- [[实体/container2wasm.md|container2wasm]]
- [[实体/kubean.md|Kubean]]
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->

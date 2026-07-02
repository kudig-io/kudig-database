---
title: K8S 专项技术
summary: K8S 专项技术：eBPF 已成为云原生基础设施的核心技术：
category: concepts
tags:
- ebpf
- wasm
- edge
- serverless
- knative
- dapr
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8S 专项技术

## eBPF 生态

eBPF 已成为云原生基础设施的核心技术：

- **[[Cilium]]**：CNCF 毕业项目，基于 eBPF 的 CNI/Service Mesh，替代 kube-proxy 提供高性能网络、可观测性与安全策略
- **[[Tetragon]]** v1.7+：Cilium 子项目，eBPF 驱动的运行时安全，内核级拦截进程、文件、网络行为，无需 sidecar
- **[[Pixie]]**：CNCF 沙箱，基于 eBPF 的自动遥测，零代码采集 K8S 集群的全栈指标与追踪
- **[[Falco]]**：支持 eBPF 探针的运行时威胁检测，替代传统内核模块方案

## WebAssembly on K8S

WASM 正在成为 K8S 的第二运行时：

- **[[SpinKube]]**：CNCF Sandbox，将 Spin 应用编排为 K8S 工作负载，支持 CRD、Helm 部署
- **[[wasmCloud]]**：CNCF Sandbox，分布式 WASM 应用平台，actor 模型 + 可组合能力
- **runwasi**：containerd 官方 WASM shim，使 kubelet 透明调度 WASM 工作负载

WASM 工作负载特点：亚毫秒启动、极小内存占用、跨平台字节码。参见 [[container-runtime-evolution]]。

## 边缘计算

- **[[KubeEdge]]**：CNCF 毕业项目（v1.22+），将 K8S 能力延伸至边缘节点，支持离线自治
- **[[K3s]]**：轻量级 K8S 发行版（<100MB 二进制），适用于 IoT/ARM/边缘场景
- **MicroK8s**：Canonical 维护的单节点 K8S，snap 包分发，适合开发与边缘
- **[[Akri]]**：CNCF 沙箱，自动发现边缘设备（摄像头、传感器等）并暴露为 K8S 资源

## Serverless / FaaS

- **[[Knative]]**：CNCF 毕业项目，提供事件驱动与请求驱动的 serverless 平台，核心能力为 **scale-to-zero** 与自动扩缩
- **[[Dapr]]**：CNCF 毕业项目，分布式应用运行时，通过 sidecar 提供服务调用、状态管理、发布订阅等构建块 API
- **[[OpenFunction]]**：CNCF 沙箱，云原生 FaaS 平台，支持多种运行时（Node.js、Go、WASM 等）

## ARM64 全面支持

- Kubernetes 核心组件已全面支持 ARM64
- 主流 CNI（Cilium、Calico）、CSI、Ingress Controller 均提供 ARM64 镜像
- AWS Graviton、Ampere Altra、Apple Silicon 成为主流 ARM 服务器平台
- 多架构镜像（multi-arch）已成为容器镜像构建标准实践

## Windows 容器

- Kubernetes 支持 Windows Server 容器节点（Windows Server 2019/2022）
- containerd 成为 Windows 容器默认运行时（替代 dockershim）
- HPA、资源限制、Network Policy 等核心功能在 Windows 节点上可用
- 混合 Linux/Windows 集群可通过 nodeSelector 和 taint/toleration 调度

## Related

- [[concepts/container-runtime-evolution.md|container runtime evolution]] — 容器运行时演进
- [[concepts/k8s-networking-evolution.md|k8s networking evolution]] — K8S 网络技术演进
- [[concepts/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — K8S AI/ML 基础设施


<!-- risk-assessed -->

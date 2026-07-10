---
title: 系统基础：硬件与内核
summary: 系统基础：硬件与内核：本文档梳理 2025-2026 年间 Kubernetes 生态在硬件层面与 Linux 内核层面的关键演进，涵盖 DPU/GPU/ARM64
  硬件趋势、内核基础设施变革及 K8S 事件审计机制。
category: concepts
tags:
- hardware
- dpu
- gpu
- arm64
- linux-kernel
- ebpf
- cgroup
- k8s
tier: supporting
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 系统基础：硬件与内核

本文档梳理 2025-2026 年间 Kubernetes 生态在硬件层面与 Linux 内核层面的关键演进，涵盖 DPU/GPU/ARM64 硬件趋势、内核基础设施变革及 K8S 事件审计机制。

---

## 一、硬件趋势

### 1.1 DPU / SmartNIC

- **NVIDIA BlueField-4**：集成 16 个 ARM Cortex-A78AE 核心，支持 400Gb/s 网络带宽，内置加密加速器与存储虚拟化引擎。可在硬件层面完成 OVS 流表、RDMA 网络隔离与存储 I/O 路径卸载，使主机 CPU 专注应用计算。
- **AMD Pensando (Elba SoC)**：提供可编程数据面（P4/C），支持策略驱动的微分段、加密与遥测。广泛集成于 Azure、Oracle Cloud 的网络虚拟化栈。
- **对 K8S 的影响**：DPU 正成为 Kubernetes 节点的"第三类计算资源"——除 CPU/GPU 外的专用 I/O 处理单元。K8S Device Plugin 与 CDI（Container Device Interface）扩展可将 DPU 能力暴露给 Pod。

### 1.2 GPU

| 架构 | 型号 | 关键指标 |
|------|------|----------|
| NVIDIA Blackwell | B100 / B200 / B300 | 208B 晶体管（双 die），FP4 推理 20 PFLOPS，HBM3e 192GB |
| AMD CDNA 3 | MI300X / MI300A | 192GB HBM3，统一 CPU+GPU 内存架构，ROCm 6.x |
| Intel Gaudi 3 | — | 专用 AI 加速，与 K8S Device Plugin 集成 |

- **虚拟化**：MIG（Multi-Instance GPU）与 vGPU 使多租户共享成为 K8S 调度的基本能力。NVIDIA GPU Operator 自动化驱动、MIG 配置与 DCGM 遥测部署。

### 1.3 ARM64 服务器

- **Ampere AmpereOne**：192 个单线程核心，256MB L3 缓存，256 条 PCIe Gen5 通道。面向云原生微服务高密度部署。
- **AWS Graviton4**：基于 Neoverse V2，性能较 Graviton3 提升 30%，深度集成于 EKS。
- **Microsoft Azure Cobalt 100**：基于 Neoverse N2，128 核，面向 Azure 通用计算与 AKS 工作负载。
- **K8S 生态**：多架构镜像（linux/amd64 + linux/arm64）已成为 OCI 镜像分发标准。Node Affinity 与 RuntimeClass 可按架构调度。

### 1.4 机密计算（Confidential Computing）

- **Intel TDX**（Trust Domain Extensions）：硬件级 VM 隔离，每个 TD 拥有独立密钥与度量值。
- **AMD SEV-SNP**：内存加密完整性校验，防止 hypervisor 级攻击。
- **K8S 集成**：Confidential Containers (CoCo) 项目通过 Kata Containers + attestation agent 实现机密 Pod。`io.katacontainers.ccruntime` RuntimeClass 使工作负载在 TDX/SEV-SNP 环境中运行。

### 1.5 CXL 2.0 / 3.0 内存池化

- **CXL 2.0**：支持 Type 3 设备（内存扩展器），实现跨主机内存共享与分层。
- **CXL 3.0**：引入 fabric 打拓扑、多级交换、共享内存池（memory pooling）。
- **K8S 影响**：CXL 内存可作为 NUMA 异构内存节点暴露，Topology Manager 与 memory QoS 需适配 CXL 三级延迟层次。

---

## 二、Linux 内核与 K8S

### 2.1 cgroup v2 成为默认

- Kubernetes 1.25+ 将 cgroup v2 设为推荐默认。v2 统一层级模型简化了资源控制器管理。
- 关键改进：统一的 IO/内存/CPU 控制器、Pressure Stall Information (PSI) 支持、更好的内存 OOM 处理。

### 2.2 eBPF 6.x 内核

- **网络**：Cilium 使用 eBPF 替代 iptables/kube-proxy，实现 O(1) 服务转发与 XDP 级 DDoS 防护。
- **可观测性**：eBPF tracing（bpftrace, Tetragon）提供零侵入的运行时安全策略与性能分析。
- **调度**：sched_ext（6.12+）允许用 eBPF 自定义 CPU 调度策略，AI 训练场景可实现 GPU-aware 调度。

### 2.3 io_uring 存储

- io_uring 提供异步 I/O 框架，减少 syscall 开销。6.x 内核中 io_uring 支持网络 send/recv 与 splice。
- K8S 存储场景：CSI 驱动可利用 io_uring 加速本地 PV 的异步 I/O，提升数据库/消息队列类工作负载吞吐。

### 2.4 PSI GA（v1.36）

- Pressure Stall Information 提供 CPU/内存/IO 的实时压力指标（some/full）。
- K8S 1.36 将 PSI 集成至 kubelet，支持基于实际资源压力的动态调度与 Pod 驱逐，替代仅基于阈值的 OOM Killer。

### 2.5 User Namespaces GA（v1.36）

- K8S 1.36 将 User Namespaces 标记为 GA。Pod 内的 root 映射为宿主机非特权用户。
- 大幅降低容器逃逸风险，与 Seccomp、AppArmor 形成纵深防御。

### 2.6 KSM AI 内存去重

- Kernel Same-page Merging 传统用于合并相同内存页。新方向结合 ML 模型预测哪些页面值得比较，降低扫描开销。
- AI 训练场景中多 Pod 加载相同模型权重时，KSM 可节省 30-50% 内存。

### 2.7 NUMA 拓扑管理

- Topology Manager 在 kubelet 中协调 CPU、设备与内存的 NUMA 亲和性。
- 多 socket / CXL 异构内存场景下，需结合 `topologyPolicy: single-numa-node` 或 `restricted` 以保证低延迟访问。

---

## 三、K8S 事件与审计

### 3.1 结构化 JSON 审计（4 级）

Kubernetes Audit Logging 提供四个级别：

| 级别 | 说明 |
|------|------|
| None | 不记录 |
| Metadata | 请求元数据 |
| Request | 请求体 |
| RequestResponse | 请求 + 响应体 |

通过 `AuditPolicy` 按资源/用户/动作为不同 API 组配置不同级别。

### 3.2 事件压缩（KEP-3836）

- KEP-3836 引入事件聚合与压缩机制，将高频重复事件合并为单条摘要事件。
- 减少 etcd 写入与 apiserver 负载，大规模集群中事件存储量降低 60-80%。

### 3.3 Fluent Bit / OTel SIEM 集成

- **Fluent Bit** 作为 K8S 日志路由层，支持将审计日志转发至 Splunk、Elastic SIEM、CrowdStrike Falcon LogScale 等。
- **OpenTelemetry Collector**：统一采集审计日志、指标与 trace，通过 OTLP 协议发送至后端。K8S 审计事件可作为安全信号源。

### 3.4 云审计日志

- AWS CloudTrail / GCP Audit Logs / Azure Monitor 各自提供 K8S API 审计集成（EKS/AKS/GKE）。
- 统一分析需借助 SIEM 平台关联云审计与集群内审计日志。

---

## 四、与其他概念的关联

- **容器运行时演进** → [[概念/container-runtime-evolution.md|container runtime evolution]]：机密容器（CoCo）依赖 Kata Containers 运行时；cgroup v2 影响所有运行时的资源限制实现。
- **AI/ML 基础设施** → [[概念/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]]：GPU 虚拟化、KSM 内存去重、NUMA 拓扑管理直接支撑 AI 训练/推理调度。
- **安全合规** → [[概念/k8s-security-compliance.md|k8s security compliance]]：User Namespaces GA、机密计算、审计日志是合规基线的核心组件。

---

## 五、参考来源

- NVIDIA BlueField-4 DPU Architecture Whitepaper (2025)
- CNCF Runtime Class & CoCo Project Reports
- Kubernetes Enhancement Proposals: KEP-3836, KEP-127 (User Namespaces)
- Linux Kernel 6.x changelogs & LWN.net coverage
- CXL Consortium Specifications 2.0 / 3.0
- Ampere AmpereOne / AWS Graviton4 / Azure Cobalt 100 product briefs


<!-- risk-assessed -->

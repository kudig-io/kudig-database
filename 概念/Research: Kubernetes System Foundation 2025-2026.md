---
title: 'Research: Kubernetes System Foundation 2025-2026'
summary: 'Research: Kubernetes System Foundation 2025-2026：2025-2026 年间，Kubernetes
  生态在系统基础层面经历了三大变革浪潮：'
category: synthesis
tags:
- hardware
- linux-kernel
- system
- k8s
- research
tier: supporting
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Research: Kubernetes System Foundation 2025-2026

## 概述

2025-2026 年间，Kubernetes 生态在系统基础层面经历了三大变革浪潮：

1. **DPU / ARM64 硬件革命**：专用数据处理器（BlueField-4、Pensando）将网络与存储虚拟化从主机 CPU 卸载至硬件；ARM64 服务器（AmpereOne 192 核、Graviton4、Cobalt 100）凭借核心密度与能效比重塑云原生部署经济学。
2. **cgroup v2 + eBPF 内核演进**：cgroup v2 成为 K8S 默认；eBPF 在 6.x 内核中覆盖网络、安全、调度（sched_ext）三大领域；io_uring 为存储 I/O 提供异步加速。
3. **User Namespaces / PSI GA**：K8S 1.36 将 User Namespaces 与 Pressure Stall Information 推进至 GA，分别解决容器安全隔离与资源压力感知两大长期痛点。

核心概念详见 → [[概念/system-foundation-hardware-kernel.md|system foundation hardware kernel]]

---

## 六大关键发现

### 1. DPU 正在成为 K8S 的"第三类计算资源"
DPU 不再是云厂商专属硬件。随着 Device Plugin 与 CDI 接口标准化，DPU 可在通用 K8S 集群中作为可调度资源暴露，卸载 OVS、加密与存储 I/O 路径，释放 10-20% 主机 CPU 给应用工作负载。

### 2. ARM64 已从"可选"变为"默认选项"
三大云厂商均推出自研 ARM64 处理器并深度集成至托管 K8S 服务。多架构镜像（linux/amd64 + linux/arm64）已成为 CI/CD 流水线的标准输出。性能/成本比优势在微服务场景中达到 30-40%。

### 3. cgroup v2 迁移是 2025 年最大基础设施变更
cgroup v2 的统一层级模型带来 PSI 支持、更好的内存 OOM 行为，但也引入了与旧版工具链（cadvisor、某些 monitoring agents）的兼容性断裂。迁移需系统性验证所有依赖 cgroup v1 文件路径的组件。

### 4. eBPF 从"网络加速"扩展为"通用内核可编程层"
sched_ext 允许用 eBPF 自定义 CPU 调度策略，结合 GPU 感知调度可优化 AI 训练的 CPU-GPU 数据流水线。Tetragon 将 eBPF 安全策略从网络层扩展至进程级。

### 5. 机密计算从实验室走向生产
Intel TDX 与 AMD SEV-SNP 已在主流云平台提供 GA 支持。Confidential Containers 项目使 K8S Pod 可在硬件隔离环境中运行，满足金融、医疗合规要求。

### 6. K8S 审计从"日志收集"进化为"安全信号源"
结构化 JSON 审计 + OpenTelemetry Collector + SIEM 集成形成完整的安全可观测链路。KEP-3836 事件压缩解决了大规模集群中事件爆炸的运维痛点。

---

## 核心概念

详细技术分析见 → [[概念/system-foundation-hardware-kernel.md|system foundation hardware kernel]]，涵盖：
- 硬件趋势：DPU/SmartNIC、GPU Blackwell/MI300X、ARM64、机密计算、CXL 内存池化
- Linux 内核与 K8S：cgroup v2、eBPF 6.x、io_uring、PSI、User Namespaces、KSM、NUMA
- K8S 事件与审计：JSON 审计 4 级、事件压缩、Fluent Bit/OTel 集成

---

## 跨域关联

| 领域 | 关联文档 | 关联点 |
|------|----------|--------|
| 容器运行时 | [[概念/container-runtime-evolution.md|container runtime evolution]] | 机密容器依赖 Kata 运行时；cgroup v2 影响所有运行时资源限制 |
| AI/ML 基础设施 | [[概念/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] | GPU 虚拟化、KSM 内存去重、NUMA 拓扑管理支撑 AI 调度 |
| 安全合规 | [[概念/k8s-security-compliance.md|k8s security compliance]] | User Namespaces GA、机密计算、审计日志是合规基线 |
| 网络演进 | [[概念/k8s-networking-evolution.md|k8s networking evolution]] | DPU 卸载网络虚拟化；eBPF Cilium 替代 kube-proxy |

---

## 矛盾与张力

1. **DPU 卸载 vs 生态碎片化**：各厂商 DPU SDK 不统一，Device Plugin 标准化尚在进行中，跨云迁移困难。
2. **cgroup v2 的必要性 vs 迁移成本**：v2 是正确方向，但监控工具链的兼容性断裂导致部分生产集群被迫延迟迁移。
3. **机密计算的隐私保证 vs 性能开销**：TDX/SEV-SNP 的内存加密引入 5-15% 性能损耗，且 attestation 流程增加部署复杂度。
4. **eBPF 强大能力 vs 内核版本依赖**：sched_ext 等特性要求 6.12+ 内核，与企业级 LTS 发行版的保守升级策略冲突。

---

## 参考来源

- NVIDIA BlueField-4 / Blackwell 架构白皮书 (2025)
- CXL Consortium Specifications 2.0 / 3.0
- Kubernetes KEP-3836 (Event Compression), KEP-127 (User Namespaces)
- Linux Kernel 6.x (sched_ext, io_uring improvements)
- CNCF Confidential Containers Project
- Ampere / AWS / Microsoft ARM64 processor announcements
- OpenTelemetry Kubernetes audit integration docs

## Related

- research/ — tag hub


<!-- risk-assessed -->

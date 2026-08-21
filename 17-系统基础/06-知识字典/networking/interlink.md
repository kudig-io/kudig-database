---
title: InterLink HPC 互联
description: InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将
  HPC（高性能计算）...
summary: InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将 HPC（高性能计算）...
category: dictionary
tags:
- k8s
- glossary
- networking
- hpc
- virtual-kubelet
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- InterLink HPC 互联 是什么
- InterLink 详解
trigger_keywords:
- InterLink HPC 互联
- InterLink
- dictionary
prerequisites:
- kubernetes
---



# InterLink HPC 互联（InterLink）

## 概述

InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将 HPC（高性能计算）资源接入 Kubernetes，实现 K8s 工作负载在 HPC 集群上运行。

## 核心概念/原理

- **HPC 集成**：将 HPC 集群（Slurm/HTCondor）接入 K8s
- **Virtual Kubelet**：基于 VK Provider 模式实现
- **CNCF Sandbox**：INFN 主导
- **科学计算**：为科学研究提供 HPC 资源

## 关键机制或特性

- Virtual Kubelet Provider for HPC
- 支持 Slurm/HTCondor/Kubernetes 后端
- Pod 到 HPC Job 的转换
- 数据管理（输入/输出文件传输）
- GPU/大内存节点的调度
- Sidecar 容器支持
- HPC 资源配额管理

## 使用场景与最佳实践

- AI/ML 训练的 HPC 资源利用
- 科学计算工作负载的 K8s 管理
- 混合云+HPC 的资源调度
- 大规模模拟任务的资源弹性
- 科研机构的计算资源统一管理

## 架构深度解析

### 组件架构

```
┌───────────────────────────┐     ┌───────────────────────────┐
│     Kubernetes 集群         │     │        HPC 集群             │
│  ┌─────────────────────┐   │     │  ┌─────────────────────┐   │
│  │  Virtual Kubelet     │   │     │  │   InterLink Agent    │   │
│  │  (InterLink Provider)│   │◀───▶│  │  - Slurm 后端         │   │
│  │  - Pod → HPC Job 转换 │   │ gRPC│  │  - HTCondor 后端     │   │
│  │  - 状态/日志回传       │   │     │  │  - K8s 后端          │   │
│  └─────────────────────┘   │     │  │  - 数据管理(文件传输)   │   │
│  ┌─────────────────────┐   │     │  └─────────────────────┘   │
│  │  InterLink 主服务     │   │     │  ┌─────────────────────┐   │
│  │  (API Server 对接)    │   │     │  │  Slurm / HTCondor    │   │
│  └─────────────────────┘   │     │  │  (调度器/资源管理器)    │   │
│  (可部署于 K8s 集群或 HPC 侧)│     │  └─────────────────────┘   │
└───────────────────────────┘     └───────────────────────────┘
```

### 源码关键路径（intertwin-eu/interLink）

| 模块 | 路径 | 职责 |
|------|------|------|
| InterLink 主服务 | `cmd/interlink` | gRPC 服务，接收 Virtual Kubelet 的 Pod 请求 |
| Virtual Kubelet | `cmd/vk` | 基于 virtual-kubelet 框架的 Provider 实现 |
| Slurm 后端 | `plugin/slurm` | Pod 转换为 Slurm `sbatch` Job |
| HTCondor 后端 | `plugin/htcondor` | Pod 转换为 HTCondor 提交 |
| 数据管理 | `pkg/datamanager` | 输入/输出文件与共享目录的传输管理 |
| 遥测 | `pkg/telemetry` | OpenTelemetry 指标与日志上报 |

### Pod 到 HPC Job 的执行流程

1. 用户在 K8s 提交 Pod/Job，调度器选中标记为 `virtual-kubelet` 的虚拟节点
2. Virtual Kubelet 将 Pod spec 序列化为 gRPC 请求发送给 InterLink 主服务
3. InterLink 按后端类型转换：Slurm 后端生成 `sbatch` 脚本并提交作业
4. 作业运行状态（PENDING/RUNNING/COMPLETED）轮询回传 Virtual Kubelet
5. 容器输出日志与退出码映射回 Pod 状态；文件通过共享存储或数据管理模块传输

## 生产案例

### 案例 1：HPC Job 提交后 Pod 一直 Pending

| 时间 | 事件 |
|------|------|
| 11:00 | 科研用户提交 GPU 训练 Pod，长时间 `Pending` |
| 11:05 | Virtual Kubelet 日志显示 Job 已提交但状态未回传 |
| 11:10 | 检查 Slurm 队列，发现作业卡在 `PENDING (Resources)` |
| 11:20 | 确认是 Slurm 分区 GPU 配额不足，且 InterLink 未配置优先级回退 |
| 11:45 | 调整 Slurm QOS 或增加分区资源，作业进入 RUNNING |

**根因**：InterLink 将 Pod 直接映射为 Slurm 作业，未处理集群资源不足时的排队语义；Pod 侧表现为 Pending 而非明确的排队说明。

**修复命令**：
```bash
# 查看虚拟节点上 Pod 状态 🟢 只读
kubectl describe pod train-gpu-123 -n ai | grep -i condition
# 查看 InterLink 日志中的 Slurm 提交信息 🟢 只读
journalctl -u interlink | grep -i "slurm\|job" | tail -20
# 在 HPC 侧查看作业队列与原因 🟢 只读
squeue -u research01 -l
# 调整 QOS 后重新提交（HPC 侧）🟡 中风险
scontrol update jobid=45210 QOS=high-priority
```

### 案例 2：大文件数据集传输阻塞作业启动

**现象**：需要 50GB 数据集的作业反复启动失败，检查点文件无法回传。

**诊断**：InterLink 默认使用 SSH/HTTP 通道传输输入输出文件，大数据集场景下传输超时；K8s 侧容器已退出但 HPC 侧作业仍在等待数据。

**修复**：改用共享文件系统（NFS/Lustre）挂载替代逐文件传输；配置 `DataManager` 的异步传输与断点续传；对超大输入采用 `rsync` 预同步后再提交作业。

## 对比评测

| 维度 | InterLink | Volcano + Slurm 插件 | 手工 HPC 网关 |
|------|-----------|---------------------|---------------|
| 接入模式 | Virtual Kubelet 虚拟节点 | K8s 原生调度器插件 | 自研 API 网关 |
| 作业语义 | Pod → HPC Job 双向映射 | 队列/优先级原生语义 | 自定义 |
| 数据管理 | 内置 DataManager | 依赖共享存储 | 自行实现 |
| 适用场景 | 科研/混合云 HPC 接入 | 高性能 AI 训练 | 简单脚本化接入 |
| 成熟度 | CNCF Sandbox | 高（CNCF） | 自维护 |

**选型建议**：需要"K8s 统一入口 + HPC 异构后端"选 InterLink；AI 训练类高性能调度选 Volcano；只有少量 HPC 作业可考虑轻量网关。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| Pod 卡 Pending | `squeue -u <user> -l` | Slurm 资源不足或 QOS 受限 |
| Job 完成但 Pod 不结束 | 对比 InterLink 日志与 Slurm 退出码 | 状态回传映射失败 |
| 文件传输失败 | `journalctl -u interlink | grep data` | SSH 通道超时或权限错误 |
| 虚拟节点 NotReady | `kubectl get node -l virtual-kubelet` | InterLink 服务不可达 |

## 生产部署清单

- [ ] 明确 Pod→HPC Job 的资源映射规则（CPU/内存/GPU 的配额换算），避免超额提交
- [ ] 为 HPC 侧配置作业超时与抢占策略，防止僵尸作业占用资源
- [ ] 大数据集优先共享文件系统，SSH 传输仅限小文件与检查点
- [ ] 虚拟节点打污点（taint）防止普通工作负载误调度到 HPC
- [ ] InterLink 主服务与 HPC 调度器建立健康检查与重连机制
- [ ] 记录 Pod UID ↔ HPC Job ID 映射日志，便于排障回溯

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 作业批量提交失败或状态回传中断 | 立即检查 InterLink 服务与 HPC 调度器连通性 |
| P1 | HPC 侧调度器版本升级（Slurm 22.x → 23.x） | 先在测试分区验证 InterLink 后端兼容性 |
| P2 | 大数据集传输频繁超时 | 规划共享文件系统接入替代 SSH 传输 |

## 运维要点

- InterLink 以 Virtual Kubelet Provider 形式接入集群，节点名为虚拟节点（如 `interlink-vk`）。
- HPC 作业提交依赖 Slurm/HTCondor 侧配置，变更需与 HPC 管理员协同验证。
- 监控虚拟节点上的 Pod 状态与 HPC 任务队列深度，避免任务堆积。
- 网络打通（K8s ↔ HPC 集群）是排障首要检查项（API 与数据通道）。

## 面试要点

> 以下 Q&A 覆盖 InterLink 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：InterLink 是如何让 K8s 调度 HPC 资源的？**
   A：基于 Virtual Kubelet 框架实现 Provider：虚拟节点出现在 K8s 节点列表中，Pod 调度到该节点后，Provider 将 Pod spec 转换为 HPC 作业（Slurm sbatch / HTCondor），作业状态轮询回传映射为 Pod 状态，实现 K8s API 语义下透明调度 HPC 资源。

2. **Q：Pod 与 HPC Job 的状态映射存在哪些不一致风险？**
   A：HPC 队列语义（排队中、被抢占）与 K8s Pod 语义（Pending/Running/Succeeded）不完全对应：作业排队时 Pod 会长期 Pending；作业被抢占时需映射为 Failed 或重新排队；作业完成但文件回传失败时 Pod 状态可能滞后，需在 InterLink 层定义明确的映射与超时策略。

3. **Q：InterLink 如何处理输入输出数据？**
   A：内置 DataManager 通过 SSH/HTTP 在 K8s 与 HPC 间传输文件；生产推荐使用共享文件系统（NFS/Lustre/GPFS）挂载到作业目录，消除逐文件传输瓶颈；超大检查点建议 rsync 预同步与异步回传。

## 参考链接

- https://interlink-expect.github.io/
- https://github.com/intertwin-eu/interLink

## Related

- [[17-系统基础/06-知识字典/fundamentals/virtual-kubelet.md|Virtual Kubelet]]
- [[17-系统基础/06-知识字典/scheduling/volcano.md|Volcano]]
- [[17-系统基础/06-知识字典/scheduling/hami.md|HAMi]]

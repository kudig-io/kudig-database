---
title: AI Agent 基础设施
summary: 深入研究 Kubernetes 上构建 AI Agent 运行时的基础设施需求，涵盖 Agent 编排、工具调用、状态管理、可观测性、安全沙箱等核心能力。
category: research
tags:
- research
- ai-agent
- llm
- infrastructure
- orchestration
tier: supporting
created: '2026-07-21'
updated: '2026-07-21'
last_updated: '2026-07-21'
status: done
---

# AI Agent 基础设施

## 研究背景

随着 LLM（大语言模型）能力的快速提升，AI Agent（智能体）从概念验证走向生产部署。AI Agent 不同于传统的无状态 Web 服务，它具有：
- **有状态对话**：需要维护多轮对话上下文
- **工具调用**：需要安全地执行外部工具（代码执行、API 调用、数据库查询）
- **长时间运行**：单次任务可能持续数分钟到数小时
- **资源弹性**：推理负载波动大，需要快速扩缩容
- **安全隔离**：Agent 执行的代码/命令需要沙箱隔离

这些特性对 Kubernetes 基础设施提出了新的要求。

## 核心问题

1. AI Agent 在 K8s 上的运行时架构应该如何设计？
2. 如何安全地实现 Agent 的工具调用（代码执行、文件操作、网络访问）？
3. Agent 的状态管理和持久化策略是什么？
4. 如何实现 Agent 的可观测性（Token 消耗、延迟、成功率）？
5. Agent 工作负载的资源调度和弹性伸缩策略？

## 调研发现

### 发现一：AI Agent 运行时架构模式

| 模式 | 描述 | 适用场景 | 代表 |
|------|------|----------|------|
| 无状态 API | Agent 作为无状态服务，状态外置 | 简单对话、客服 | LangChain + Redis |
| 有状态 Pod | Agent 维护本地状态，PVC 持久化 | 复杂任务、长对话 | AgentScope |
| Job 模式 | 每个任务一个 Job，完成即销毁 | 批量处理、代码生成 | Argo Workflows |
| Sidecar 模式 | Agent 作为 Sidecar 辅助主应用 | 应用内嵌 AI 能力 | Dapr + LLM |
| Operator 模式 | CRD 定义 Agent，控制器管理生命周期 | 平台级 Agent 管理 | 自定义 Operator |

### 发现二：工具调用安全沙箱

| 方案 | 隔离级别 | 性能 | 适用场景 |
|------|----------|------|----------|
| gVisor (runsc) | 用户态内核 | 中 | 不可信代码执行 |
| Kata Containers | VM 级 | 低 | 最高安全要求 |
| Firecracker microVM | 轻量 VM | 中 | 代码解释器 |
| 容器 + seccomp/AppArmor | 系统调用过滤 | 高 | 受限工具调用 |
| 远程沙箱 (E2B/Modal) | 完全隔离 | 取决于网络 | SaaS 场景 |

### 发现三：Agent 可观测性关键指标

| 指标 | 描述 | 告警阈值 |
|------|------|----------|
| Token 消耗速率 | 每分钟输入/输出 Token 数 | 超预算 150% |
| 任务成功率 | 成功完成的任务比例 | < 90% |
| 首 Token 延迟 (TTFT) | 用户等待首个响应的时间 | > 3s |
| 工具调用成功率 | 外部工具调用的成功比例 | < 95% |
| 上下文窗口使用率 | Token 占上下文窗口的比例 | > 80% |
| 幻觉率 | 输出与事实不符的比例 | > 5% |

### 发现四：资源调度策略

| 工作负载类型 | 资源特征 | 调度策略 |
|-------------|----------|----------|
| LLM 推理 | GPU 密集、高内存 | GPU 节点池 + 独占调度 |
| Agent 编排 | CPU 密集、有状态 | 通用节点 + PVC |
| 工具执行 | 突发、短生命周期 | Job + 弹性节点池 |
| 向量检索 | 内存密集 | 高内存节点 + 本地 SSD |
| 嵌入计算 | GPU/CPU 混合 | 弹性伸缩 + Spot 实例 |

## 落地方案

### 生产级 Agent 平台架构

```
┌─────────────────────────────────────────────┐
│              API Gateway / Ingress           │
├─────────────────────────────────────────────┤
│         Agent Orchestration Layer            │
│  (LangChain / AgentScope / Custom)          │
├──────────┬──────────┬───────────────────────┤
│ LLM 推理 │ 工具沙箱 │ 状态存储              │
│ (vLLM/   │ (gVisor/ │ (Redis/              │
│  Triton) │  Kata)   │  PostgreSQL)         │
├──────────┴──────────┴───────────────────────┤
│         可观测性 (Langfuse / OTel)           │
├─────────────────────────────────────────────┤
│         Kubernetes Infrastructure            │
│  (GPU Operator + Storage + Networking)       │
└─────────────────────────────────────────────┘
```

## 参考资源

- [LangChain on Kubernetes](https://python.langchain.com/docs/)
- [AgentScope](https://github.com/modelscope/agentscope)
- [vLLM Production Deployment](https://docs.vllm.ai/)
- [E2B Code Sandbox](https://e2b.dev/)
- [Langfuse Observability](https://langfuse.com/)

## Related Tags

- [[27-标签/06-AI与专项/ai-ml-infra|ai-ml-infra]]
- [[27-标签/06-AI与专项/gpu|gpu]]
- [[27-标签/01-核心平台/k8s|k8s]]
- [[27-标签/03-安全与合规/security|security]]

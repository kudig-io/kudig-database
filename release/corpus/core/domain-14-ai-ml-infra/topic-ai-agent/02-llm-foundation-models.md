---
title: LLM 基座模型选型与评估 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: 技术选型指南 | **最后更新**: 2026-03 | **关键词**: LLM 选型,
  GPT-4o, Claude'
summary: 'description: ''**文档类型**: 技术选型指南 | **最后更新**: 2026-03 | **关键词**: LLM 选型, GPT-4o,
  Claude'
category: general
tags:
- ai
- ai-agent
- docker
- networkpolicy
- operator
- gpu
- nvidia
- vllm
- llm
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- LLM 基座模型选型与评估 是什么
- 如何 LLM 基座模型选型与评估
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- LLM
- 基座模型选型与评估
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---



title: LLM 基座模型选型与评估
description: '**文档类型**: 技术选型指南 | **最后更新**: 2026-03 | **关键词**: LLM 选型, GPT-4o, Claude
  3.5, Gemini, Llama-3, Qwen-2.5, DeepSeek, 微调 vs RAG, 模型评估, Agent 基座'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- docker
- [[NetworkPolicy|networkpolicy]]
- operator
- gpu
- nvidia
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- LLM 基座模型选型与评估 是什么
- 如何 LLM 基座模型选型与评估
trigger_keywords:
- LLM
- 基座模型选型与评估
- ai
- agent
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# LLM 基座模型选型与评估

> **文档类型**: 技术选型指南 | **最后更新**: 2026-03 | **关键词**: LLM 选型, GPT-4o, Claude 3.5, Gemini, Llama-3, Qwen-2.5, DeepSeek, 微调 vs RAG, 模型评估, Agent 基座

---

<!-- chunk: 概述 -->## 概述

LLM 的选择是构建 Agent 系统的首要决策，直接决定 Agent 的推理能力上限、工具调用可靠性、成本结构和合规边界。本文提供主流模型的全维度对比矩阵、面向 Agent 场景的专项评估指标、微调 vs RAG 的决策框架，以及生产环境中的模型路由策略。

---

<!-- chunk: 1. 模型全景概览 -->## 1. 模型全景概览

## 1.1 主流模型分类

```
LLM 生态全景
│
├── 闭源商业模型（API 调用）
│   ├── OpenAI 系列: GPT-4o, GPT-4o-mini, o1, o3-mini
│   ├── Anthropic 系列: Claude 3.5 Sonnet, Claude 3.5 Haiku, Claude 3 Opus
│   ├── Google 系列: Gemini 2.0 Flash, Gemini 1.5 Pro, Gemini Ultra
│   └── 国内: 通义千问、文心一言、智谱 GLM-4、Moonshot Kimi
│
├── 开源/半开源模型（自部署）
│   ├── Meta: Llama 3.1 (8B/70B/405B), Llama 3.3 70B
│   ├── 阿里: Qwen2.5 (7B/14B/32B/72B), Qwen2.5-Coder
│   ├── DeepSeek: DeepSeek-V3, DeepSeek-R1, DeepSeek-R1-Distill
│   ├── Mistral: Mistral Large, Mixtral 8x22B, Mistral Nemo
│   └── Google: Gemma 2 (9B/27B)
│
└── 专用模型
    ├── 代码: DeepSeek-Coder-V2, Qwen2.5-Coder-32B, CodeLlama
    ├── 嵌入: text-embedding-3-large, BGE-M3, Jina v3
    └── 多模态: GPT-4V, Claude 3.5 (视觉), Gemini 1.5 Pro
```

---

<!-- chunk: 2. 主流模型性能对比矩阵 -->## 2. 主流模型性能对比矩阵

## 2.1 综合能力对比（2025年底基准）

| 模型 | 参数量 | 上下文窗口 | 推理能力 | 代码能力 | 工具调用 | 中文能力 | 成本/1M Token | 延迟(首Token) |
|------|-------|-----------|---------|---------|---------|---------|--------------|-------------|
| **GPT-4o** | 未知 | 128K | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★☆ | $2.5/$10 | ~0.5s |
| **GPT-4o-mini** | 未知 | 128K | ★★★★☆ | ★★★★☆ | ★★★★★ | ★★★★☆ | $0.15/$0.6 | ~0.3s |
| **o1** | 未知 | 200K | ★★★★★+ | ★★★★★ | ★★★★☆ | ★★★★☆ | $15/$60 | 5-30s |
| **Claude 3.5 Sonnet** | 未知 | 200K | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★☆ | $3/$15 | ~0.5s |
| **Claude 3.5 Haiku** | 未知 | 200K | ★★★★☆ | ★★★★☆ | ★★★★★ | ★★★★☆ | $0.8/$4 | ~0.3s |
| **Gemini 2.0 Flash** | 未知 | 1M | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★★☆ | $0.1/$0.4 | ~0.4s |
| **Gemini 1.5 Pro** | 未知 | 2M | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★★☆ | $1.25/$5 | ~1s |
| **Llama 3.3 70B** | 70B | 128K | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★☆☆ | 自部署 | 取决于硬件 |
| **Qwen2.5-72B** | 72B | 128K | ★★★★☆ | ★★★★★ | ★★★★☆ | ★★★★★ | 自部署 | 取决于硬件 |
| **DeepSeek-V3** | 671B(MoE) | 128K | ★★★★★ | ★★★★★ | ★★★★☆ | ★★★★★ | $0.27/$1.1 | ~1s |
| **DeepSeek-R1** | 671B(MoE) | 128K | ★★★★★+ | ★★★★★ | ★★★☆☆ | ★★★★★ | $0.55/$2.19 | 10-60s |

> 注: 价格为 Input/Output Token 单价，以 API 调用为准，随厂商调整而变化。自部署成本取决于 GPU 资源。

## 2.2 Agent 专项能力对比

| 模型 | 工具调用可靠性 | 多步规划 | 指令遵循 | 自我纠错 | 长上下文理解 | 并行工具调用 |
|------|-------------|---------|---------|---------|------------|------------|
| GPT-4o | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★☆ | 原生支持 |
| GPT-4o-mini | ★★★★☆ | ★★★★☆ | ★★★★★ | ★★★★☆ | ★★★★☆ | 原生支持 |
| Claude 3.5 Sonnet | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★★ | 原生支持 |
| Gemini 2.0 Flash | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★★★ | 原生支持 |
| Llama 3.3 70B | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★☆☆ | ★★★★☆ | 支持 |
| Qwen2.5-72B | ★★★★☆ | ★★★★☆ | ★★★★★ | ★★★★☆ | ★★★★☆ | 支持 |
| DeepSeek-V3 | ★★★★☆ | ★★★★★ | ★★★★★ | ★★★★☆ | ★★★★☆ | 支持 |
| DeepSeek-R1 | ★★★☆☆ | ★★★★★+ | ★★★★☆ | ★★★★★ | ★★★★☆ | 有限支持 |

---

<!-- chunk: 3. 场景选型决策树 -->## 3. 场景选型决策树

## 3.1 主决策框架

```
选型决策起点
│
├── 数据是否可以发送到外部 API?
│   ├── 否（合规/安全要求内部部署）
│   │   └── → 开源模型自部署（Llama/Qwen/DeepSeek）
│   │
│   └── 是
│       ├── 预算是否充裕（>$50/月）?
│       │   ├── 是
│       │   │   ├── 需要最强推理能力? → GPT-4o 或 Claude 3.5 Sonnet
│       │   │   ├── 需要超长上下文(>200K)? → Gemini 1.5 Pro (2M)
│       │   │   └── 主要处理中文? → DeepSeek-V3 或 通义千问-Max
│       │   │
│       │   └── 否（成本敏感）
│       │       ├── 简单任务/高频调用 → GPT-4o-mini 或 Gemini Flash
│       │       └── 中文场景成本敏感 → DeepSeek-V3 (极低价格)
│       │
│       └── 是否有严格的工具调用要求?
│           ├── 是 → GPT-4o 或 Claude 3.5 Sonnet（工具调用最可靠）
│           └── 否 → 根据成本/性能比选择
```

## 3.2 K8s 运维 Agent 专项选型建议

| Agent 类型 | 推荐模型 | 备选模型 | 原因 |
|-----------|---------|---------|------|
| 实时诊断 Agent | GPT-4o-mini | Claude 3.5 Haiku | 高频调用需低延迟低成本，能力足够 |
| 复杂故障分析 | Claude 3.5 Sonnet | GPT-4o | 需要多步推理 + 长上下文理解大量日志 |
| 配置生成 Agent | GPT-4o | Qwen2.5-72B | YAML 生成需高精度指令遵循 |
| 离线根因分析 | DeepSeek-R1 | o1 | 深度推理任务，延迟不敏感 |
| 私有化部署 | Qwen2.5-72B | Llama 3.3 70B | 强中文能力 + 工具调用 + 开源 |

---

<!-- chunk: 4. 模型评估方法论 -->## 4. 模型评估方法论

## 4.1 针对 Agent 的评估维度

```python
# Agent 能力基准测试框架
class AgentBenchmark:
    
    def evaluate_tool_calling_accuracy(self, model, test_cases: list) -> float:
        """评估工具调用准确性"""
        # 测试点:
        # 1. 是否选择了正确的工具
        # 2. 工具参数是否准确填写
        # 3. 是否正确处理工具错误并重试
        # 4. 是否避免不必要的工具调用
        correct = 0
        for case in test_cases:
            result = model.invoke(case["input"], tools=case["tools"])
            if self._check_tool_call(result, case["expected_tool_call"]):
                correct += 1
        return correct / len(test_cases)
    
    def evaluate_multi_step_planning(self, model, scenarios: list) -> dict:
        """评估多步骤规划能力"""
        metrics = {
            "task_completion_rate": 0,
            "avg_steps_efficiency": 0,  # 实际步骤/最优步骤
            "hallucination_rate": 0,    # 无凭据声明的比例
        }
        # ... 评估实现
        return metrics
    
    def evaluate_context_retention(self, model, long_conv: list) -> float:
        """评估长对话中的上下文保持能力"""
        # 在第 20 轮对话后提问第 3 轮提到的信息
        # 测量回忆准确率
        pass
```

## 4.2 推荐评测基准集

| 基准集 | 测量维度 | 适用场景 |
|--------|---------|---------|
| **MMLU** | 多领域知识储备 | 通用能力基线 |
| **HumanEval / MBPP** | 代码生成能力 | 代码 Agent |
| **ToolBench** | 工具调用准确性 | 工具调用 Agent |
| **AgentBench** | Agent 任务完成率 | 综合 Agent 评估 |
| **GAIA** | 真实世界任务 | 通用 Agent |
| **SWE-bench** | 软件工程任务 | 代码/DevOps Agent |
| **τ-bench** | 工具 + 推理综合 | 复杂 Agent 场景 |
| **K8s 专项测试集** | K8s 知识深度 | 运维 Agent（自建） |

## 4.3 自建评估集（K8s 运维场景）

```python
K8S_AGENT_TEST_CASES = [
    {
        "id": "pod-pending-001",
        "category": "故障诊断",
        "difficulty": "medium",
        "input": "Pod nginx-xxx 一直处于 Pending 状态，已经 10 分钟了",
        "required_tools": ["kubectl_describe", "kubectl_get_nodes"],
        "expected_root_causes": ["资源不足", "节点亲和性", "PVC 未绑定", "Taint 不匹配"],
        "evaluation_rubric": {
            "tool_sequence_correct": 0.3,  # 30% 分权重
            "root_cause_identified": 0.4,
            "fix_suggestion_correct": 0.3,
        }
    },
    {
        "id": "service-unreachable-001",
        "category": "网络诊断",
        "difficulty": "hard",
        "input": "frontend Pod 无法访问 backend Service，但 Service 存在",
        "required_tools": ["kubectl_get_endpoints", "kubectl_exec_curl", "kubectl_get_networkpolicy"],
        "expected_root_causes": ["Endpoints 为空", "NetworkPolicy 阻断", "kube-proxy 问题"],
    }
]
```

---

<!-- chunk: 5. 微调（Fine-tuning）vs RAG 决策框架 -->## 5. 微调（Fine-tuning）vs RAG 决策框架

## 5.1 决策矩阵

| 维度 | 微调（Fine-tuning） | RAG |
|------|-------------------|-----|
| **适用场景** | 固定格式输出、领域风格、专业术语 | 知识更新频繁、需要引用来源、知识量大 |
| **知识更新成本** | 高（需重新训练） | 低（更新知识库即可） |
| **训练数据需求** | 需要高质量标注数据（数百~数千条） | 无需训练数据 |
| **推理成本** | 低（无需额外检索） | 较高（检索 + 生成） |
| **延迟** | 低 | 较高（+100~500ms 检索时间） |
| **幻觉风险** | 较高（模型可能过度泛化） | 较低（有检索结果支撑） |
| **可解释性** | 差（知识融入权重） | 好（可追溯检索来源） |
| **初始投入** | 高（训练成本 + 基础设施） | 中（构建知识库 + 向量化）|

## 5.2 决策判断树

```
选择微调 还是 RAG?
│
├── 知识更新频率 > 每月一次?
│   └── 是 → 优先 RAG（微调太慢）
│
├── 需要引用具体来源/文档?
│   └── 是 → 必须 RAG
│
├── 目标是改变输出格式/风格（如固定 JSON 结构）?
│   └── 是 → 考虑微调（或 Prompt Engineering 先试）
│
├── 专业术语/缩略语大量出现，基座模型不理解?
│   └── 是 → 微调（词汇层面的问题 RAG 难以解决）
│
├── 知识库超过 100 万 tokens?
│   └── 是 → 必须 RAG（塞不进上下文）
│
└── 两者都需要? → RAG + Fine-tuning 组合
    示例: 微调模型学习输出格式 + RAG 提供最新知识
```

## 5.3 实用建议

```
尝试顺序（成本从低到高）:
  1. 先尝试 Prompt Engineering（系统提示 + Few-shot 示例）
  2. 效果不佳 → 尝试 RAG
  3. RAG 后仍有格式/风格问题 → 考虑少量 Fine-tuning
  4. 以上都不够 → 组合策略（RAG + Fine-tuning）

常见误区:
  × 直接跳到微调，忽略 Prompt Engineering 的潜力
  × 以为微调了就不需要 RAG（知识更新仍然需要）
  × 只用 RAG 不做质量评估，导致检索质量差影响生成
```

---

<!-- chunk: 6. 模型路由策略（生产环境） -->## 6. 模型路由策略（生产环境）

在生产 Agent 系统中，通常需要多模型路由来平衡成本和质量：

```python
from enum import Enum
from dataclasses import dataclass

class TaskComplexity(Enum):
    SIMPLE = "simple"     # 简单问答、格式转换
    MEDIUM = "medium"     # 多步骤工具调用
    COMPLEX = "complex"   # 深度推理、复杂分析

@dataclass
class ModelConfig:
    name: str
    api_key_env: str
    cost_per_1m_input: float  # USD
    cost_per_1m_output: float
    max_context: int
    avg_latency_ms: int

MODEL_REGISTRY = {
    "gpt-4o": ModelConfig("gpt-4o", "OPENAI_API_KEY", 2.5, 10.0, 128000, 500),
    "gpt-4o-mini": ModelConfig("gpt-4o-mini", "OPENAI_API_KEY", 0.15, 0.6, 128000, 300),
    "claude-3-5-sonnet": ModelConfig("claude-3-5-sonnet-20241022", "ANTHROPIC_API_KEY", 3.0, 15.0, 200000, 500),
    "claude-3-5-haiku": ModelConfig("claude-3-5-haiku-20241022", "ANTHROPIC_API_KEY", 0.8, 4.0, 200000, 300),
    "deepseek-v3": ModelConfig("deepseek-chat", "DEEPSEEK_API_KEY", 0.27, 1.1, 128000, 1000),
}

class ModelRouter:
    def route(self, task: dict) -> str:
        """根据任务特征选择最优模型"""
        complexity = self._assess_complexity(task)
        requires_chinese = self._needs_chinese(task)
        is_latency_sensitive = task.get("latency_sensitive", False)
        context_length = self._estimate_context(task)
        
        # 超长上下文
        if context_length > 128_000:
            return "gemini-1.5-pro"  # 2M context
        
        # 深度推理任务（延迟不敏感）
        if complexity == TaskComplexity.COMPLEX and not is_latency_sensitive:
            if requires_chinese:
                return "deepseek-r1"
            return "claude-3-5-sonnet"
        
        # 高频简单任务（成本敏感）
        if complexity == TaskComplexity.SIMPLE:
            if requires_chinese:
                return "deepseek-v3"  # 极低价格 + 强中文
            return "gpt-4o-mini"
        
        # 中等复杂度（默认主力）
        if requires_chinese:
            return "deepseek-v3"
        return "gpt-4o-mini"
    
    def _assess_complexity(self, task: dict) -> TaskComplexity:
        """基于任务描述评估复杂度"""
        description = task.get("description", "")
        tool_count = len(task.get("available_tools", []))
        
        if tool_count > 10 or "分析" in description or "规划" in description:
            return TaskComplexity.COMPLEX
        elif tool_count > 3 or "诊断" in description:
            return TaskComplexity.MEDIUM
        return TaskComplexity.SIMPLE
```

---

<!-- chunk: 7. 开源模型自部署指南 -->## 7. 开源模型自部署指南

## 7.1 硬件需求速查

| 模型 | 最低 GPU | 推荐 GPU | 显存需求(FP16) | 推理吞吐量(vLLM) |
|------|---------|---------|--------------|----------------|
| Qwen2.5-7B | 1x RTX 3090 | 1x A100 | ~16GB | ~50 req/s |
| Llama 3.1 8B | 1x RTX 3090 | 1x A100 | ~16GB | ~50 req/s |
| Qwen2.5-14B | 1x A100 40G | 2x A100 | ~28GB | ~30 req/s |
| Qwen2.5-32B | 2x A100 80G | 4x A100 | ~64GB | ~15 req/s |
| Llama 3.3 70B | 4x A100 80G | 8x A100 | ~140GB | ~8 req/s |
| Qwen2.5-72B | 4x A100 80G | 8x A100 | ~144GB | ~8 req/s |
| DeepSeek-V3 671B | 8x H100 (FP8) | 16x H100 | ~350GB(FP8) | ~3 req/s |

## 7.2 vLLM 部署示例

```bash
# 部署 Qwen2.5-72B 的推荐配置
docker run --gpus all \
  -v /data/models:/models \
  -p 8000:8000 \
  vllm/vllm-openai:latest \
  --model /models/Qwen2.5-72B-Instruct \
  --served-model-name qwen2.5-72b \
  --tensor-parallel-size 4 \      # 4 张 GPU 张量并行
  --pipeline-parallel-size 1 \
  --max-model-len 32768 \         # 最大上下文长度（显存限制）
  --max-num-seqs 256 \            # 最大并发请求数
  --enable-chunked-prefill \      # 提升长文本处理效率
  --trust-remote-code \
  --dtype bfloat16 \
  --api-key "your-secret-key"
```

```yaml
# K8s 部署 vLLM（对接生产环境）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-qwen25-72b
  namespace: ai-serving
spec:
  replicas: 1
  selector:
    matchLabels:
      app: vllm-qwen25-72b
  template:
    metadata:
      labels:
        app: vllm-qwen25-72b
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.6.3
        args:
          - "--model=/models/Qwen2.5-72B-Instruct"
          - "--tensor-parallel-size=4"
          - "--max-model-len=32768"
          - "--served-model-name=qwen2.5-72b"
        resources:
          limits:
            nvidia.com/gpu: "4"
            memory: "200Gi"
          requests:
            nvidia.com/gpu: "4"
            memory: "180Gi"
        volumeMounts:
        - name: model-storage
          mountPath: /models
        - name: shm
          mountPath: /dev/shm
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-storage-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "20Gi"
      tolerations:
      - key: "nvidia.com/gpu"
        operator: "Exists"
        effect: "NoSchedule"
      nodeSelector:
        gpu-type: a100-80g
```

---

<!-- chunk: 8. 合规与数据安全考量 -->## 8. 合规与数据安全考量

## 8.1 数据分类与模型选择

```
数据密级 → 模型选择规则:

  公开数据 / 脱敏数据
  └── 可使用任何 API 模型（GPT-4o、Claude、Gemini）

  内部数据（业务数据、代码）
  ├── 签署 DPA（数据处理协议）后可用 API 模型
  └── 敏感时优先考虑私有化部署

  敏感/机密数据（客户 PII、财务数据、密钥）
  ├── 强烈建议私有化部署开源模型
  ├── 如用 API，必须开启数据不训练选项 + 签署保密协议
  └── 参考: 10-security-guardrails.md 中的 PII 处理规范

  监管数据（医疗/金融/政府）
  └── 通常要求完全本地化部署，不允许数据出境
```

## 8.2 国内合规要求

```
中国大陆合规要点:
  1. 《生成式人工智能服务管理暂行办法》
     - 向中国用户提供服务的 AIGC 产品需向国家互联网信息办公室备案
  
  2. 数据本地化要求
     - 重要数据和个人信息原则上在境内存储
     - 推荐使用国内 API（阿里云百炼、字节豆包、智谱 AI）或自部署
  
  3. 内容安全要求
     - 需部署内容安全过滤层
     - 推荐: 阿里云内容安全 SDK 或百度文字审核
```

---

<!-- chunk: 9. 最佳实践与反模式 -->## 9. 最佳实践与反模式

## 最佳实践

- **多模型策略**：不要把所有任务都发给同一个最贵的模型，按复杂度路由可节省 60-80% 成本
- **评估驱动选型**：用你自己的数据集测试，而不仅依赖公开基准——基准分数和实际业务效果存在差距
- **版本锁定**：生产环境固定模型版本（如 `gpt-4o-2024-11-20`），避免供应商静默升级影响输出一致性
- **温度配置**：工具调用类任务设置 `temperature=0`，创意类任务设置 `temperature=0.7`
- **流式输出**：对延迟敏感的场景使用 Streaming，降低用户感知等待时间

## 反模式

- **只用最贵的模型**：简单分类任务用 GPT-4o 是极大浪费，GPT-4o-mini 足够
- **不锁定版本**：`gpt-4o-latest` 会自动升级，可能导致输出格式突然变化
- **忽略 Token 计量**：不监控 Token 使用量，成本会迅速失控
- **过度依赖单一供应商**：OpenAI 服务不稳定时没有备选，应设计多供应商 Fallback

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | Agent 推理框架对模型能力的依赖 |
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | 不同框架与模型的兼容性 |
| [11 - 成本与延迟优化](./11-cost-latency-optimization.md) | 模型路由的成本优化实践 |
| [domain-14-ai-ml-infra/17-llm-inference-serving.md](../domain-14-ai-ml-infra/17-llm-inference-serving.md) | vLLM/TGI 部署详情 |
| [domain-14-ai-ml-infra/03-gpu-scheduling-management.md](../domain-14-ai-ml-infra/03-gpu-scheduling-management.md) | GPU 资源调度 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization.md|成本与延迟优化策略]]

## See Also

- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals
- 03-agent-frameworks-comparison
- 04-rag-knowledge-retrieval

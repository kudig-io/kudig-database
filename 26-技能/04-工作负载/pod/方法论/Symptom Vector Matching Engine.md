---
title: Symptom Vector Matching Engine
description: Symptom Vector Matching Engine — Kubernetes 生产运维知识库
summary: Symptom Vector Matching Engine — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- troubleshooting
- semantic-matching
- agent
- gpu
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Symptom Vector Matching Engine 是什么
- 如何 Symptom Vector Matching Engine
trigger_keywords:
- Symptom
- Vector
- Matching
- Engine
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Symptom Vector Matching Engine

## Purpose

Transforms natural language symptom descriptions (including colloquial expressions like "Pod keeps crashing", "container died") into structured feature vectors for pattern matching against known fault trees.

## 32-Dimensional Feature Space

| Index Range | Category | Dimensions |
|------------|----------|------------|
| 0-7 | Basic Symptoms | pod_restart, pod_pending, pod_evicted, oom_killed, not_ready, connection_fail, timeout, error_log |
| 8-13 | Resource Features | memory_high, cpu_high, disk_full, network_latency, storage_io_high, gpu_memory_high |
| 14-17 | Exit Codes | exit_137 (OOM), exit_1 (generic), exit_143 (SIGTERM), exit_125 (runtime error) |
| 18-21 | Cloud Provider | ack_specific, aws_specific, gcp_specific, on_premise |
| 22-25 | Lifecycle Phase | startup_phase, runtime_phase, scale_phase, drain_phase |
| 26-31 | Severity | p0_critical, p1_major, p2_minor, user_impact_high, service_down, degraded |

## How It Works

1. **Input**: Natural language symptom (e.g., "Pod repeatedly restarting, OOMKilled, exit code 137")
2. **Vectorization**: Maps to 32D feature vector (e.g., pod_restart=0.9, oom_killed=0.95, exit_137=1.0)
3. **Semantic Expansion**: Expands colloquial terms ("Pod hung" -> "CrashLoopBackOff")
4. **Cosine Similarity**: Computes similarity against known pattern library
5. **Output**: Top-K candidate patterns with confidence scores

### Example

```python
# Input: "Pod 反复重启，OOMKilled，exit code 137"
result = pipeline.match(symptom, context={"cloud_provider": "ACK"})

# Output:
{
    "best_match": {
        "pattern": "OOMKilled Classic Pattern",
        "fta_path": "TE-2 -> IE-2.1 -> BE-2.3",
        "final_score": 0.92
    },
    "candidates": [
        {"pattern": "CrashLoopBackOff", "fta_path": "BE-2.1", "score": 0.78},
        {"pattern": "Evicted", "fta_path": "BE-3.2", "score": 0.35}
    ]
}
```

## Capabilities

| Capability | Description |
|-----------|-------------|
| **Semantic Understanding** | Understands colloquial expressions in Chinese/English |
| **Fuzzy Matching** | Finds similar patterns even without exact match |
| **Confidence Ranking** | Returns Top-K candidates with similarity scores |
| **Incremental Learning** | New symptom patterns can be added to the vector library |
| **Cross-language** | Supports Chinese, English, and mixed input |
| **Unknown Detection** | Flags symptoms with low similarity to all known patterns for escalation |

## Problem Solving Architecture Integration

The symptom vector matcher is the **input layer** of the Kudig-DB problem-solving architecture:

```
Symptom Input (this engine)
    ↓
FTA Diagnosis Engine (dynamic probability, pruning, Bayesian inference)
    ↓
Decision Output (root cause confirmation, remediation plan)
    ↓
Learning Loop (probability updates, new pattern discovery)
```

## 症状向量匹配实践

### 匹配引擎工作流程

```
用户输入/告警
    │
    ├── 症状提取 (NLP/关键词)
    │
    ├── 向量化 (Embedding)
    │
    ├── 相似度匹配 (Cosine Similarity)
    │
    ├── Top-K FTA 候选
    │
    └── 执行诊断 → 确认根因
```

### 症状向量维度

| 维度 | 示例 | 权重 |
|------|------|------|
| 资源类型 | Pod/Node/Service/PVC | 0.3 |
| 错误类型 | CrashLoop/Pending/Timeout | 0.3 |
| 影响范围 | 单 Pod/单节点/全集群 | 0.2 |
| 时间特征 | 突发/渐进/周期性 | 0.1 |
| 关联变更 | 发布/配置/扩缩容 | 0.1 |

### 匹配准确率优化

1. **多维度匹配**: 不仅匹配关键词，还匹配语义
2. **上下文感知**: 考虑最近变更和集群状态
3. **反馈学习**: 根据实际诊断结果调整权重
4. **多 FTA 并行**: 不确定时并行执行多个 FTA

## Related

- [[cloud-provider-fta]] — 云平台集成异常故障树分析
- [[backup-restore-fta]] — 备份/恢复异常故障树分析
- [[26-技能/04-工作负载/pod/方法论/skills-run-README.md|skills-run-README]] — Skills Demo — 本地运行工单诊断技能
- [[INDEX]] — Wiki Index
- [[score]] — Score
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[22-概念/08-可靠性与运维/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]


<!-- risk-assessed -->

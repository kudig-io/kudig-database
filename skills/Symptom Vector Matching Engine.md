---
title: Symptom Vector Matching Engine
description: Symptom Vector Matching Engine — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- troubleshooting
- semantic-matching
- agent
- gpu
- rag
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
created: "2026-05-23"
---

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

## Related

- [[cloud-provider-fta]] — 云平台集成异常故障树分析
- [[backup-restore-fta]] — 备份/恢复异常故障树分析
- [[skills/skills-run-README|skills-run-README]] — Skills Demo — 本地运行工单诊断技能
- [[INDEX]] — Wiki Index
- [[score]] — Score
- [[skills/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[skills/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]

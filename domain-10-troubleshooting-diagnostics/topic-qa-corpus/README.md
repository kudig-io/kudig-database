---
title: Agent QA 对语料库
description: |
  命令输出→诊断 I-O 对语料 + 结构化 QA pairs，供 SRE Agent 推理使用
category: general
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- agent
- qa
- corpus
- rag
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent 开发者
- SRE
- 运维工程师
estimated_read_time: 10min
intent_queries:
- Agent 命令输出诊断语料库
- kubectl 输出解读语料
- SRE Agent I-O pairs
- Kubernetes 故障诊断训练数据
trigger_keywords:
- Agent
- QA
- 命令输出
- 诊断
- troubleshooting
- diagnostics
- qa
- corpus
- I-O pairs
prerequisites:
- kubectl-basics
- troubleshooting-methodology
created: "2026-05-23"
---

# Agent QA 对语料库

> **生成日期**: 2026-05-21
> **最后更新**: 2026-05-21

## 语料结构

本语料库包含两类核心数据：

| 类型 | 文件位置 | 数量 | 用途 |
|------|----------|------|------|
| **命令输出→诊断 I-O 对** | `generated/command-output-diagnosis-*.md` | 469 条 | Agent 模式匹配诊断 |
| **结构化 QA pairs** | `../../*-qa.yaml` | 14,080 条 | RAG 检索评测 |
| **手工种子** | `seed/p0-core-scenarios.md` | 22 条 | 高质量参考模板 |
| **原始语料** | `command-output-diagnosis.md` | 23 条 | 遗留基准数据 |

## 命令输出诊断语料（I-O Pairs）

### 文件索引

| 文件 | 优先级 | I-O 对数 | 说明 |
|------|--------|----------|------|
| `generated/command-output-diagnosis-all.md` | ALL | 469 | 全量合并版 |
| `generated/command-output-diagnosis-all.json` | ALL | 469 | JSON 程序消费版 |
| `generated/command-output-diagnosis-all.yaml` | ALL | 469 | YAML 程序消费版 |
| `generated/command-output-diagnosis-p0.md` | P0 | 469 | 核心问题场景 |
| `generated/command-output-diagnosis-p1.md` | P1 | 469 | 扩展场景 |
| `generated/command-output-diagnosis-p2.md` | P2 | 469 | 高级/边缘场景 |
| `seed/p0-core-scenarios.md` | P0 | 22 | 手工精调种子 |
| `command-output-diagnosis.md` | - | 23 | 原始基准语料 |

### 覆盖统计

| 维度 | 数值 |
|------|------|
| 总 I-O 对数 | **469** |
| 覆盖 Skills | 32/32 (100%) |
| 覆盖 FTA | 44/44 (100%) |
| 覆盖 Domain | 15/18 |
| Severity critical | ~5% |
| Severity high | ~35% |
| Severity medium | ~55% |
| Severity low | ~5% |

### I-O 对格式

```yaml
io_pair_id: IODIAG-NODE-0001
skill_ref: SKILL-01
scenario: 节点 NotReady - kubelet 异常
severity: critical
command: kubectl get nodes
output_pattern: |
  NAME       STATUS     ROLES           AGE   VERSION
  node-01    NotReady   worker          5d    v1.32.0
diagnosis:
  - 节点不健康，kubelet 可能挂掉或网络不通
  - NotReady 超过 pod-eviction-timeout (默认5min) 后 Pod 会被驱逐
action:
  - kubectl describe node node-01
  - ssh node-01 'systemctl status kubelet'
confidence: 0.95
tags: [node, status, notready, kubelet]
```

### 字段说明

| 字段 | 说明 | 示例 |
|------|------|------|
| `io_pair_id` | 全局唯一标识 | `IODIAG-NODE-0001` |
| `skill_ref` | 关联 [[SKILL|Skill]] ID | `SKILL-01` |
| `scenario` | 问题场景名称 | `节点 NotReady` |
| `severity` | 严重程度 | `critical/high/medium/low` |
| `command` | 执行命令 | `kubectl get nodes` |
| `output_pattern` | 典型输出模式 | 含通配符/占位符 |
| `diagnosis` | 诊断结论列表 | 按确定性排序 |
| `action` | 后续操作建议 | 可执行的命令 |
| `confidence` | 匹配置信度 | `0.0-1.0` |
| `tags` | 分类标签 | 用于检索和过滤 |

## 生成方式

语料通过自动化流水线生成：

```bash
# 生成全量语料
cd scripts/corpus-generator
python3 generate.py --priority all \
  --output domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/

# 验证覆盖率
python3 validators/coverage_checker.py \
  --skills-dir domain-10-troubleshooting-diagnostics/topic-skills \
  --fta-dir domain-10-troubleshooting-diagnostics/topic-fta/list \
  --corpus-dir domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated
```

### 生成流水线

```
topic-skills/*.md ──┐
                   ├──→ extractors → generate.py → topic-qa-corpus/generated/
topic-fta/list/*.md ┘         ↑
                              │
seed/*.md ────────────────────┘  (人工精调覆盖)
```

## Agent 使用方式

1. **模式匹配**：Agent 执行命令后，将输出与 `output_pattern` 进行模糊匹配
2. **诊断获取**：匹配成功后，读取 `diagnosis` 和 `action` 列表
3. **置信度排序**：多条匹配时按 `confidence` 排序，取最高置信度结果
4. **上下文关联**：通过 `skill_ref` 和 `fta_ref` 跳转到完整诊断文档

## 已知缺口与改进计划

| 缺口 | 状态 | 计划 |
|------|------|------|
| GPU 场景 I-O 对 | 待补充 | 从 gpu-fta.md + 相关 Skill 提取 |
| [[Helm|Helm]] 场景 I-O 对 | 待补充 | 从 helm-fta.md + 相关 Skill 提取 |
| Webhook 场景 I-O 对 | 待补充 | 从 webhook-admission-fta.md 提取 |
| 命令去重率提升 | 进行中 | 优化归一化去重策略 |
| Critical severity 比例 | 进行中 | 增加根因级别诊断对 |

## Related

- [[domain-10-troubleshooting-diagnostics/topic-skills/README.md|Skills 故障诊断手册]]
- index.md|FTA 故障树索引]]
- [[domain-10-troubleshooting-diagnostics/topic-qa-corpus/command-output-diagnosis.md|原始命令输出诊断语料]]
- [[domain-10-troubleshooting-diagnostics/topic-qa-corpus/seed/p0-core-scenarios.md|P0 手工种子语料]]
- 发布前评估报告

---
title: topic-qa-corpus MOC
description: |
  topic-qa-corpus 专题导航页，覆盖命令输出诊断语料、结构化 QA pairs 和手工种子数据
summary: topic-qa-corpus 专题导航页，覆盖命令输出诊断语料、结构化 QA pairs 和手工种子数据
category: moc
tags:
- k8s
- moc
- qa
- agent
- troubleshooting
- command-output
- diagnosis
- corpus
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent 开发者
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- topic-qa-corpus 导航
- 命令输出诊断语料索引
- Agent I-O pairs 目录
trigger_keywords:
- topic-qa-corpus
- MOC
- 语料库
- 诊断
- I-O pairs
prerequisites:
- kubectl-basics
- troubleshooting-methodology
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-qa-corpus MOC

> **MOC 版本**: 2.0
> **专题**: topic-qa-corpus
> **文档数量**: 7 篇
> **最后更新**: 2026-05-21
> **用途**: Agent 命令输出→诊断语料库导航

---

## 专题概述

命令输出诊断语料库 —— 为 SRE Agent 提供 **kubectl/系统命令输出 → 诊断结论** 的结构化映射数据。

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-qa-corpus |
| **核心功能** | Agent 模式匹配诊断 |
| **数据规模** | 469 I-O 对 + 14,080 QA pairs |
| **覆盖域** | 15 Domain / 32 Skills / 44 FTA |
| **文档数量** | 7 篇 |

---

## 文档清单

| # | 文档 | 类型 | I-O 对数 | 说明 |
|---|---|---|---|---|
| 1 | 命令输出诊断语料 — 全量合并版 | 自动生成 | 469 | 全量 I-O 对，按 Domain 分组 |
| 2 | P0 核心场景手工种子 I-O 对 | 手工精调 | 22 | 高质量参考模板，覆盖 10 Domain |
| 3 | 命令输出解读语料 — 原始基准 | 原始基准 | 23 | 遗留基准数据 |
| 4 | [[22-概念/12-研究/ai-agent-README.md|ai agent README]] | 说明文档 | - | 语料结构、使用方式、生成流水线 |
| 5 | 命令输出诊断语料 — JSON 版 | 程序消费 | 469 | JSON 格式，供 Agent 直接加载 |
| 6 | 命令输出诊断语料 — YAML 版 | 程序消费 | 469 | YAML 格式，供 Agent 直接加载 |
| 7 | 覆盖率验证报告 | 验证报告 | - | 覆盖率检查输出 |

---

## 按 Domain 索引

| Domain | I-O 对数 | 对应问题场景 | 文件 |
|--------|----------|-------------|------|
| POD | 81 | Pod CrashLoop、Pending、OOM | `generated/command-output-diagnosis-all.md` |
| NET | 72 | [[service\|Service]]、Ingress、网络连通性 | 同上 |
| INGRESS | 54 | Ingress Controller、Gateway API | 同上 |
| SEC | 38 | RBAC、Quota、NetworkPolicy | 同上 |
| WORK | 33 | Deployment、StatefulSet、DaemonSet | 同上 |
| STORAGE | 30 | PVC、PV、CSI、Mount 失败 | 同上 |
| DNS | 24 | CoreDNS、解析失败 | 同上 |
| NODE | 21 | NotReady、资源压力 | 同上 |
| CERT | ~15 | 证书过期、轮换失败 | 同上 |
| CP | ~20 | API Server、Controller Manager | 同上 |
| ETCD | ~15 | etcd 健康、DB 大小 | 同上 |
| SCALE | 15 | HPA、VPA、Cluster Autoscaler | 同上 |

---

## 生成与维护

### 自动化流水线

```bash
# 1. 生成语料
cd scripts/corpus-generator
python3 generate.py --priority all \
  --output 19-故障诊断/10-QA语料/generated/  # N7: 旧路径 topic-qa-corpus 修复

# 2. 验证覆盖率
python3 validators/coverage_checker.py \
  --skills-dir 故障诊断/topic-skills \
  --fta-dir 故障诊断/FTA故障树/list \
  --corpus-dir 故障诊断/topic-qa-corpus/generated
```

### 手工维护

- **种子数据**：`seed/` 目录下手工编写高质量 I-O 对，自动生成时会自动合并
- **Schema 规范**：`scripts/corpus-generator/templates/io_pair_schema.yaml`

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 7 |
| I-O 对总数 | 469 |
| QA pairs 总数 | 14,080 |
| 覆盖 Skills | 32/32 (100%) |
| 覆盖 FTA | 44/44 (100%) |
| 手工种子 | 22 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## 语料使用指南

### 适用场景

| 场景 | 推荐语料 | 用途 |
|------|----------|------|
| AI Agent 训练 | command-output-diagnosis | 训练模型识别命令输出中的异常模式 |
| RAG 知识库 | seed/p0-core-scenarios | 提供高质量的检索增强生成素材 |
| 培训考核 | generated P0-P2 | 模拟真实故障场景的问答练习 |
| 自动化测试 | with_actions 系列 | 验证诊断工具链的准确性 |

### 语料质量要求

1. **准确性**：所有命令输出必须来自真实环境或精确模拟
2. **完整性**：包含完整的上下文（集群版本、组件状态、时间戳）
3. **可操作性**：每个 QA pair 必须包含可执行的诊断命令
4. **分级明确**：按 P0/P1/P2 分级，反映真实生产优先级

### 贡献新语料

1. 从生产事故复盘中提取命令输出和诊断过程
2. 按 `seed/` 目录下的模板格式化
3. 提交 PR 并经 SRE 团队审核后合入
4. 定期清理过时语料（K8s 版本升级后命令输出可能变化）

## Related

- [[19-故障诊断/08-技能体系/README.md|Skills 故障诊断手册]]
- [[19-故障诊断/06-FTA故障树/fta-index.md|FTA 故障树索引]]
- [[release-sre/qa/raw/MOC.md|Release SRE QA 语料导航]]
- [[release-sre/qa/raw/command-output-diagnosis.md|Release SRE 命令输出诊断语料]]
- [[19-故障诊断/10-QA语料/seed/p0-core-scenarios.md|Release SRE P0 核心场景种子数据]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p0.md|Release SRE P0 生成语料]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p1.md|Release SRE P1 生成语料]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p2.md|Release SRE P2 生成语料]]
- [[19-故障诊断/10-QA语料/command-output-diagnosis.with_actions.md|Release SRE 命令输出诊断（含 Actions）]]
- [[19-故障诊断/10-QA语料/seed/p0-core-scenarios.with_actions.md|Release SRE P0 核心场景种子数据（含 Actions）]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p0.with_actions.md|Release SRE P0 生成语料（含 Actions）]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p1.with_actions.md|Release SRE P1 生成语料（含 Actions）]]
- [[19-故障诊断/10-QA语料/generated/command-output-diagnosis-p2.with_actions.md|Release SRE P2 生成语料（含 Actions）]]
- 发布前评估报告
- [[19-故障诊断/10-QA语料/README.md|Agent QA 对语料库 README]]


<!-- risk-assessed -->

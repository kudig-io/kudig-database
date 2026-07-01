---
title: OpenClaw MEMORY.md 机制深度解析 (domain-14-ai-ml-infra)
description: 'title: OpenClaw MEMORY.md 机制深度解析'
summary: 'title: OpenClaw MEMORY.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- etcd
- kubelet
- coredns
- llm
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenClaw MEMORY.md 机制深度解析 是什么
- 如何 OpenClaw MEMORY.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- MEMORY.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- etcd-basics
---



title: OpenClaw MEMORY.md 机制深度解析
description: '# OpenClaw MEMORY.md 机制深度解析'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- [[kubelet|kubelet]]
- [[CoreDNS|coredns]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- OpenClaw MEMORY.md 机制深度解析 是什么
- 如何 OpenClaw MEMORY.md 机制深度解析
trigger_keywords:
- OpenClaw
- MEMORY.md
- 机制深度解析
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

# OpenClaw MEMORY.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, MEMORY.md, 记忆系统, Persistence 层, 长期记忆, 短期记忆, 新陈代谢, 经验积累

---

<!-- chunk: 概述 -->## 概述

MEMORY.md 是 OpenClaw File-First 架构中管理 **Agent 长期记忆** 的配置文件。它存储跨会话的经验、模式和确定性规则，让 Agent 具备"学习能力"——每次诊断的经验都能积累下来，逐步提升诊断效率和准确率。在 Harness Engineering 中主要映射到 **Persistence 层**。

MEMORY.md 配合 `memory/` 目录（短期记忆）构成完整的记忆系统：MEMORY.md 存储长期规则和模式，`memory/YYYY-MM-DD.md` 存储每日诊断流水。

---

<!-- chunk: 1. 设计原理 -->## 1. 设计原理

## 1.1 三层记忆模型

```
MEMORY.md 三层记忆模型:

Layer 1: 确定性规则（人工维护）
  │  集群环境基线: 节点数、版本、CNI、存储方案
  │  已知问题: KI-001 Terway ENI 延迟、KI-002 ESSD Multi-Attach
  │  团队约定: Namespace 命名规范、变更流程
  │  特点: 100% 准确，手动创建和更新
  │
Layer 2: 经验模式（Agent 自动提炼）
  │  高频故障模式: 症状→根因的统计规律
  │  有效诊断路径: 哪些步骤最高效
  │  失败教训: 走过的弯路和误判
  │  特点: 概率性知识，需标注置信度
  │
Layer 3: 用户偏好（交互学习）
     常用命令: 用户习惯的 kubectl 用法
     关注指标: 用户最关心的监控维度
     历史反馈: 对 Agent 输出的正/负面反馈
     特点: 个性化定制，随使用而丰富
```

## 1.2 记忆流转机制

```
记忆生命周期:

日常诊断中产生
  │
  ▼
短期记忆: memory/2026-04-03.md
  │  每次诊断的关键发现、使用的命令、诊断路径
  │  保留期: 7 天
  │
  ▼ 每周提炼（手动或自动）
情景记忆: 重要事件和解决方案
  │  "2026-04-03 ack-prod 集群发生大规模 OOM，根因是 Java 应用内存泄漏"
  │  保留期: 3 个月
  │
  ▼ 模式抽象（累计 3+ 次同类事件后）
语义记忆: MEMORY.md 中的规则和模式
  │  "FP-001: 高频故障模式 — Java 应用 OOM，首查 JVM 堆配置"
  │  保留期: 根据置信度和使用频率
  │
  ▼ 检索注入
下次会话: MEMORY.md + 最近 3 天 memory/ → 注入上下文
```

## 1.3 新陈代谢机制

```
记忆新陈代谢（防止记忆膨胀）:

保留策略:
  确定性规则 → 永久保留（手动删除）
  高置信模式 → 保留 6 个月
  中置信模式 → 保留 3 个月
  低置信模式 → 保留 1 个月

淘汰条件:
  - 超过保留期
  - 30 天内未被引用
  - 被新记忆覆盖

质量指标:
  avg_confidence: 0.82       # 平均置信度
  utilization_rate: 0.75     # 75% 的模式在近 30 天被引用
  stale_entries: 2           # 超 3 个月未引用的条目数

目标: 保持记忆的"信噪比" > 0.7
  记忆不是越多越好，过时记忆 = 噪声 = 误导决策
```

---

<!-- chunk: 2. Harness Engineering 映射 -->## 2. Harness Engineering 映射

## 2.1 映射关系

```
MEMORY.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
MEMORY.md     │      │       │    ◐    │    ●    │        │           │

● = 主要映射（Persistence 层 — 持久化存储）
◐ = 次要映射（Context 层 — 记忆注入上下文）
```

## 2.2 Persistence 层映射详解

| MEMORY.md 内容 | Harness Persistence 实现 | 存储方式 |
|---------------|------------------------|---------|
| 确定性规则（1） | `RuleStore` — 规则持久化 | YAML 格式，手动维护 |
| 经验模式（2） | `PatternStore` — 模式持久化 | Agent 自动写入，带置信度 |
| 用户偏好（3） | `PreferenceStore` — 偏好持久化 | 交互学习，自动更新 |
| 管理元数据（4） | `MemoryMetadata` — 元数据管理 | 自动统计和维护 |
| memory/ 目录 | `DailyLog` — 每日诊断日志 | 每天一个 Markdown 文件 |

## 2.3 Context 层映射

```
MEMORY.md 记忆注入 LLM 上下文的策略:

每次会话开始（唤醒协议 Step 3）:

1. 加载 MEMORY.md 全文（长期记忆）
   → 确定性规则 + 高频模式
   → ~800 tokens

2. 加载最近 3 天 memory/（短期记忆）
   → 最近的诊断上下文
   → ~500 tokens

3. 组装记忆上下文
   system_prompt += f"""
   <!-- chunk: 长期记忆 -->## 长期记忆
   {memory_md_summary}

   <!-- chunk: 近期上下文 -->## 近期上下文
   {recent_daily_logs}
   """

总记忆 Token 预算: ~1300 tokens（占 system_prompt 的 15-20%）
```

---

<!-- chunk: 3. K8S 运维实战案例 -->## 3. K8S 运维实战案例

## 3.1 案例：已知问题命中

```
场景: 用户报告 "Pod 启动很慢，等了 30 多秒"

MEMORY.md 已知问题匹配:
  KI-001: "Terway ENI 模式 Pod IP 分配延迟"
  症状: "Pod 启动慢（>30s）"
  根因: "ENI 弹性网卡分配需要调用 ECS API，高峰期有延迟"

Agent 响应（快速命中已知问题）:
  "症状匹配已知问题 KI-001: Terway ENI 分配延迟。
   验证: kubectl describe pod <pod> -n <ns> | grep 'waiting for ENI'
   如果确认:
   1. 检查节点 ENI 余量: kubectl get eniconfig
   2. 考虑预热 ENI 池
   参考: domain-10-troubleshooting-diagnostics/03-networking-cni-troubleshooting.md"

效果: 跳过常规诊断流程，直接给出已知解决方案
  诊断时间: 从 5 分钟缩短到 30 秒
```

## 3.2 案例：经验模式学习

```
第 1 次诊断（无经验）:
  问题: Java 应用 OOMKilled
  诊断路径: 常规流程 → 查看 resources → 查看日志 → 发现 JVM 堆溢出
  耗时: 8 分钟

Agent 记录到 memory/2026-04-01.md:
  "Java 应用 OOMKilled → 先查 JVM 堆配置(-Xmx vs container memory limit)"

第 3 次诊断（经验积累后）:
  问题: 另一个 Java 应用 OOMKilled
  MEMORY.md 高频模式匹配: FP-001
  Agent 直接执行: 查看 container resources + JVM -Xmx 参数
  耗时: 2 分钟

经验提炼到 MEMORY.md:
  FP-001:
    pattern: "Java 应用 OOMKilled"
    first_check: "JVM -Xmx vs container memory limit"
    confidence: 0.85
    occurrences: 5
```

## 3.3 案例：失败教训记录

```
失败案例:
  问题: DNS 解析偶发超时
  错误诊断: Agent 建议重启 CoreDNS → 问题未解决
  正确根因: conntrack race condition（KI-003）

记录到 MEMORY.md:
  LL-001:
    title: "DNS 超时不要急于重启 CoreDNS"
    wrong_approach: "重启 CoreDNS Pod"
    correct_approach: "检查 conntrack 竞态条件，配置 force_tcp"
    lesson: "5s 超时是 conntrack 特征，不是 CoreDNS 本身的问题"

下次遇到 DNS 超时:
  Agent 匹配 LL-001 → 避免重复犯错
  直接检查 conntrack 而非重启 CoreDNS
```

---

<!-- chunk: 4. 配置协作机制 -->## 4. 配置协作机制

## 4.1 MEMORY.md 与其他文件的协作

```
MEMORY.md 在配置体系中的记忆角色:

AGENTS.md ──→ MEMORY.md
  │           唤醒协议 Step 3: 加载 MEMORY.md
  │           Phase 5: 诊断结果写入 memory/
  │
SOUL.md ──→ MEMORY.md
  │          SOUL.md 诚实原则约束记忆质量
  │          只有数据支撑的结论才能写入
  │
SKILL.md ──→ MEMORY.md
  │           SKILL.md 提供 SOP → 诊断中发现新模式 → 记录到 MEMORY.md
  │           MEMORY.md 高频模式 → 反馈优化 SKILL.md SOP
  │
USER.md ──→ MEMORY.md
             USER.md 定义初始偏好
             MEMORY.md 从交互中学习更多偏好
```

## 4.2 memory/ 目录管理

```
短期记忆目录结构:

memory/
├── 2026-04-01.md    # Day 1 诊断流水
├── 2026-04-02.md    # Day 2 诊断流水
├── 2026-04-03.md    # Day 3 诊断流水（今天）
└── ...

每日文件格式:
  # 2026-04-03 诊断日志
  <!-- chunk: Session 1 (09:15) -->## Session 1 (09:15)
  - 问题: Pod coredns-xxx Pending
  - 根因: 节点 taint 不匹配
  - 解决: 添加 tolerations
  - 标记: routine（常规问题）

  <!-- chunk: Session 2 (14:30) -->## Session 2 (14:30)
  - 问题: API Server 响应慢
  - 根因: etcd compaction 未及时执行
  - 解决: 手动执行 etcdctl compact
  - 标记: key_insight（重要发现，建议提炼到 MEMORY.md）

清理策略:
  保留最近 7 天
  标记为 key_insight 的内容 → 提炼到 MEMORY.md 后可删除
  超过 7 天的文件 → 自动归档或删除
```

---

<!-- chunk: 5. AgentScope 集成代码 -->## 5. AgentScope 集成代码

## 5.1 MemoryManager 实现

```python
import os
import yaml
from datetime import datetime, timedelta
from typing import Optional


class MemoryManager:
    """基于 MEMORY.md 的记忆管理器"""

    def __init__(self, workspace_path: str):
        self.workspace_path = workspace_path
        self.memory_path = os.path.join(workspace_path, "MEMORY.md")
        self.daily_dir = os.path.join(workspace_path, "memory")
        self.long_term = self._load_long_term()

    def _load_long_term(self) -> str:
        """加载 MEMORY.md 长期记忆"""
        if os.path.exists(self.memory_path):
            with open(self.memory_path) as f:
                return f.read()
        return ""

    def load_context(self, days: int = 3) -> str:
        """加载记忆上下文（长期 + 最近 N 天短期）"""
        context_parts = []

        # 长期记忆
        context_parts.append("<!-- chunk: 长期记忆\n") -->## 长期记忆\n")
        context_parts.append(self._summarize_long_term())

        # 短期记忆
        context_parts.append("\n<!-- chunk: 近期上下文\n") -->## 近期上下文\n")
        recent_logs = self._load_recent_daily(days)
        if recent_logs:
            context_parts.append(recent_logs)
        else:
            context_parts.append("（无近期诊断记录）")

        return "\n".join(context_parts)

    def _summarize_long_term(self) -> str:
        """提取长期记忆摘要（控制 Token 消耗）"""
        content = self.long_term
        sections = []

        # 提取已知问题
        if "known_issues:" in content:
            sections.append("已知问题: KI-001(Terway ENI延迟), "
                          "KI-002(ESSD Multi-Attach), "
                          "KI-003(DNS 5s conntrack)")

        # 提取高频模式
        if "fault_patterns:" in content:
            sections.append("高频模式: FP-001(Java OOM→查JVM), "
                          "FP-002(Pod Pending→查资源+taint), "
                          "FP-003(API Server慢→查etcd)")

        # 提取失败教训
        if "lessons_learned:" in content:
            sections.append("教训: LL-001(DNS超时≠CoreDNS问题), "
                          "LL-002(节点NotReady先查kubelet)")

        return "\n".join(sections) if sections else "（长期记忆为空）"

    def _load_recent_daily(self, days: int) -> str:
        """加载最近 N 天的每日诊断日志"""
        if not os.path.exists(self.daily_dir):
            return ""

        logs = []
        for i in range(days):
            date = datetime.now() - timedelta(days=i)
            filename = date.strftime("%Y-%m-%d.md")
            filepath = os.path.join(self.daily_dir, filename)
            if os.path.exists(filepath):
                with open(filepath) as f:
                    content = f.read()
                    # 只取前 500 字符（控制 Token）
                    logs.append(content[:500])

        return "\n---\n".join(logs)

    def record_daily(self, session_summary: str, is_key_insight: bool = False):
        """记录每日诊断流水"""
        os.makedirs(self.daily_dir, exist_ok=True)
        today = datetime.now().strftime("%Y-%m-%d")
        filepath = os.path.join(self.daily_dir, f"{today}.md")

        timestamp = datetime.now().strftime("%H:%M")
        marker = " [key_insight]" if is_key_insight else ""

        entry = f"\n<!-- chunk: Session ({timestamp}){marker}\n{session_summary}\n" -->## Session ({timestamp}){marker}\n{session_summary}\n"

        with open(filepath, "a") as f:
            f.write(entry)

    def check_known_issues(self, symptoms: str) -> Optional[str]:
        """检查症状是否匹配已知问题"""
        known_issues = {
            "启动慢": "KI-001: Terway ENI 分配延迟",
            "waiting for ENI": "KI-001: Terway ENI 分配延迟",
            "Multi-Attach": "KI-002: ESSD 云盘 Multi-Attach 残留",
            "DNS.*5s": "KI-003: conntrack race condition",
            "DNS.*超时": "KI-003: conntrack race condition",
        }
        for pattern, issue in known_issues.items():
            if pattern.lower() in symptoms.lower():
                return issue
        return None


# === 使用示例 ===
memory = MemoryManager("domain-14-ai-ml-infra/topic-ai-agent/openclaw-workspace")

# 加载记忆上下文（唤醒协议 Step 3）
context = memory.load_context(days=3)

# 检查已知问题
match = memory.check_known_issues("Pod 启动很慢，等了 30 多秒")
# → "KI-001: Terway ENI 分配延迟"

# 记录诊断结果
memory.record_daily(
    "问题: Pod OOM, 根因: JVM -Xmx > container limit, 修复: 调整 limits",
    is_key_insight=False,
)
```

---

<!-- chunk: 6. 问题排除 -->## 6. 问题排除

## 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 不引用已知问题 | MEMORY.md 未在唤醒时加载 | 确认 AGENTS.md 唤醒协议包含 Step 3 |
| 记忆膨胀导致 Token 爆炸 | 未执行新陈代谢清理 | 设置保留策略，定期清理低价值条目 |
| 过时记忆误导决策 | 环境变更后未更新 MEMORY.md | 集群变更后同步更新环境基线 |
| 短期记忆丢失 | memory/ 目录未持久化 | 确保目录在 Git 中或使用持久化存储 |
| 经验模式置信度不准 | 样本量不足就标高置信度 | 累计 5+ 次同类事件后才标注高置信 |
| Agent 记录低质量记忆 | 未经验证的猜测也被记录 | SOUL.md 诚实原则约束：只记录有数据支撑的结论 |

## 6.2 调试检查清单

```
MEMORY.md 配置验证:

□ 环境基线：是否反映当前集群的真实配置？
□ 已知问题：是否有明确的症状描述和解决方案？
□ 经验模式：是否标注了置信度和出现次数？
□ 失败教训：是否记录了错误方法和正确方法？
□ 保留策略：是否定义了各级记忆的过期时间？
□ memory/ 目录：是否有最近 7 天的每日日志？
□ 元数据：total_entries / stale_entries 是否合理？
□ Token 控制：记忆上下文总量是否 < 1500 tokens？
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | MEMORY.md 在 7 文件体系中的定位 |
| [33 - Harness 上下文与记忆工程](./33-agent-harness-context-memory.md) | 三层记忆模型的工程化实现 |
| [openclaw-workspace/MEMORY.md](./openclaw-workspace/MEMORY.md) | K8S 运维 Agent 记忆系统完整配置 |
| [46 - AGENTS.md 机制解析](./46-openclaw-agents-mechanism.md) | 唤醒协议 Step 3 加载 MEMORY.md |
| [48 - SKILL.md 机制解析](./48-openclaw-skill-mechanism.md) | SKILL.md 诊断经验到 MEMORY.md 的流转 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容，深度解析 OpenClaw MEMORY.md 的设计机制与工程实现。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 47-openclaw-tools-mechanism
- 48-openclaw-skill-mechanism
- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals

---
title: OpenClaw IDENTITY.md 机制深度解析 (domain-14-ai-ml-infra)
description: 'title: OpenClaw IDENTITY.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- coredns
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenClaw IDENTITY.md 机制深度解析 是什么
- 如何 OpenClaw IDENTITY.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- IDENTITY.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

title: OpenClaw IDENTITY.md 机制深度解析
description: '# OpenClaw IDENTITY.md 机制深度解析'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
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
- OpenClaw IDENTITY.md 机制深度解析 是什么
- 如何 OpenClaw IDENTITY.md 机制深度解析
trigger_keywords:
- OpenClaw
- IDENTITY.md
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

# OpenClaw IDENTITY.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, IDENTITY.md, 身份标识, 品牌设计, 多渠道适配, 展示层, SOUL 分离

---

## 概述

IDENTITY.md 是 OpenClaw File-First 架构中定义 **Agent 对外形象** 的配置文件。它告诉用户"Agent 长什么样"——名称、风格、问候语、输出格式和多渠道适配策略。在 Harness Engineering 中没有直接的层映射，属于 **展示层（Presentation Layer）**。

IDENTITY.md 的核心设计哲学是**与 SOUL.md 分离**：SOUL.md 定义内在人格（不可随意修改），IDENTITY.md 定义外在形象（可灵活调整）。这意味着可以为同一个 Agent 换不同的"皮肤"而不影响其核心行为。

---

## 1. 设计原理

### 1.1 SOUL.md vs IDENTITY.md 分离设计

```
分离设计的核心价值:

SOUL.md（内在人格 — 稳定不变）:
  ✓ 核心身份: "我是 K8S 运维诊断专家"
  ✓ 价值观: 安全第一、数据驱动、诚实可信
  ✓ 绝对红线: 不删除、不编造、不泄露
  修改频率: 很少（重大版本升级时）
  修改影响: 高（改变 Agent 根本行为）

IDENTITY.md（外在形象 — 灵活调整）:
  ✓ 名称: KuDig Doctor / K8S 诊断助手
  ✓ 风格: 硬核 · 精准 · 高效 · 可信
  ✓ 问候语: "KuDig Doctor 就绪。"
  修改频率: 经常（适配不同场景、渠道、客户）
  修改影响: 低（仅改变外在表现）

类比:
  SOUL.md = 一个人的性格和价值观
  IDENTITY.md = 这个人的名片、着装和说话方式
  可以换名片和着装，但性格不变
```

### 1.2 品牌一致性模型

```
IDENTITY.md 品牌一致性四要素:

1. 视觉标识（Visual）
   │  名称: KuDig Doctor
   │  代号: K8S 诊断助手
   │  版本: v1.0
   │
2. 语言风格（Verbal）
   │  人格标签: 硬核 · 精准 · 高效 · 可信
   │  沟通调性: 不是"温暖的聊天助手"而是"靠谱的技术搭档"
   │
3. 交互模式（Interactive）
   │  问候: 简短直接（"KuDig Doctor 就绪。"）
   │  诊断: 专业简洁（现象→根因→修复）
   │  结束: 不客套（直接结束，不说"祝您顺利"）
   │
4. 输出格式（Format）
     统一模板: 现象→根因→修复→验证→预防
     代码块: 命令必须在代码块中
     表格: 对比信息用表格呈现
```

### 1.3 多渠道适配策略

| 渠道 | 格式特点 | IDENTITY.md 适配 |
|------|---------|-----------------|
| 终端 CLI | 纯文本，宽度受限 | ASCII 表格，长输出分页 |
| Studio WebUI | 完整 Markdown | 代码块高亮，Mermaid 图表 |
| API 响应 | JSON 结构化 | 字段分离（diagnosis/evidence/fix） |
| Telegram Bot | 简化 Markdown | 省略详细步骤，保留核心结论 |
| 工单系统 | 标准报告模板 | 包含工单号引用，结构化诊断报告 |

---

## 2. Harness Engineering 映射

### 2.1 映射关系

```
IDENTITY.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
IDENTITY.md   │      │       │    ◐    │         │        │           │

◐ = 次要映射（Context 层 — 身份上下文）
无主要映射 — IDENTITY.md 属于展示层，不影响 Agent 核心行为
```

### 2.2 展示层定位

```
IDENTITY.md 在 Harness 架构中的位置:

                    ┌─────────────────────┐
                    │   IDENTITY.md       │  ← 展示层（最外层）
                    │   品牌 · 风格 · 格式  │
                    └──────────┬──────────┘
                               │
  ┌────────────────────────────┼────────────────────────────┐
  │                   Harness 六层架构                        │
  │  ┌─── Loop ───┐  ┌── Tools ──┐  ┌── Context ──┐       │
  │  │ AGENTS.md  │  │ TOOLS.md  │  │ SOUL.md     │       │
  │  └────────────┘  └───────────┘  │ USER.md     │       │
  │  ┌─ Persist ──┐  ┌── Verify ─┐  │ SKILL.md    │       │
  │  │ MEMORY.md  │  │ 质量门禁   │  └─────────────┘       │
  │  └────────────┘  └───────────┘                         │
  │  ┌─ Constrain ─┐                                       │
  │  │ SOUL.md 红线 │                                       │
  │  └─────────────┘                                       │
  └────────────────────────────────────────────────────────┘

IDENTITY.md 包裹在最外层:
  不影响内部行为逻辑
  仅控制最终输出的"包装"方式
```

---

## 3. K8S 运维实战案例

### 3.1 案例：场景化问候

```
IDENTITY.md 定义的问候模板:

首次会话:
  "KuDig Doctor v1.0 就绪。
   当前配置: ack-prod-hangzhou 集群
   有什么需要诊断的？"

继续会话（有上下文）:
  "欢迎回来。
   上次我们在排查 coredns OOM 问题，需要继续吗？"

紧急模式（检测到 P0 关键词）:
  "P0 问题确认。进入快速诊断模式。
   请提供: 1. Namespace  2. 资源名称  3. 异常时间"

巡检模式:
  "开始每日集群巡检。
   检查范围: ack-prod-hangzhou 全部 Namespace"
```

### 3.2 案例：多渠道输出适配

```
同一诊断结果在不同渠道的输出:

=== CLI 终端 ===
根因: Node CPU 不足 (38/40 节点 >90%)
修复: kubectl scale nodepool app --replicas=42
验证: kubectl top nodes | awk '$3>90'

=== Studio WebUI ===
## 诊断结果

### 根因
Node CPU Allocatable 已用尽。当前 38/40 个 worker 节点 CPU requests 超过 90%。

### 修复方案
```bash
kubectl scale nodepool app --replicas=42
```

### 验证
```bash
kubectl top nodes | awk '$3>90'
```

=== API JSON ===
{
  "diagnosis": {
    "root_cause": "Node CPU 不足",
    "confidence": "高",
    "evidence": "38/40 节点 CPU >90%"
  },
  "fix": {
    "command": "kubectl scale nodepool app --replicas=42",
    "risk": "低"
  }
}

=== Telegram Bot ===
根因: 节点 CPU 不足
修复: 扩容 2 个节点
```

### 3.3 案例：版本标识管理

```
版本标识的显示策略:

默认关闭 — 正常诊断不显示版本信息

触发显示的场景:
  1. 用户问 "你是谁" / "版本信息"
     → [KuDig Doctor v1.0 | Harness L3 | Model: qwen-max]

  2. 正式诊断报告（报告模式）
     → 页脚: "Generated by KuDig Doctor v1.0 | 2026-04-03"

  3. Debug 模式
     → 每条输出附带: [v1.0 | SOUL:active | TOOLS:10 | MEM:15entries]
```

---

## 4. 配置协作机制

### 4.1 IDENTITY.md 与其他文件的协作

```
IDENTITY.md 在配置体系中的展示角色:

SOUL.md ──→ IDENTITY.md
  │          SOUL.md 定义内在人格（不变）
  │          IDENTITY.md 定义外在形象（可变）
  │          SOUL.md 优先级 > IDENTITY.md
  │
USER.md ──→ IDENTITY.md
  │          USER.md 的沟通偏好约束 IDENTITY.md 的表达方式
  │          如果 USER.md 说"不要客套" → IDENTITY.md 问候语简化
  │
AGENTS.md ──→ IDENTITY.md
               唤醒协议 Step 4: 按 IDENTITY.md 风格输出就绪问候
               Phase 5: 按 IDENTITY.md 格式输出诊断结果
```

### 4.2 多 Agent 场景的身份区分

```
多 Agent 协作时的身份管理:

Agent 1: KuDig Doctor（诊断专家）
  IDENTITY.md:
    name: KuDig Doctor
    role: 诊断
    style: 硬核 · 精准

Agent 2: KuDig Planner（方案规划）
  IDENTITY.md:
    name: KuDig Planner
    role: 规划
    style: 全面 · 谨慎

Agent 3: KuDig Monitor（监控巡检）
  IDENTITY.md:
    name: KuDig Monitor
    role: 巡检
    style: 简洁 · 周期性

三者共享同一个 SOUL.md（核心人格一致）
但各自有不同的 IDENTITY.md（外在表现不同）
```

---

## 5. AgentScope 集成代码

### 5.1 IdentityManager 实现

```python
from typing import Optional
from enum import Enum


class OutputChannel(Enum):
    CLI = "cli"
    WEB_UI = "web_ui"
    API = "api"
    TELEGRAM = "telegram"
    TICKET = "ticket"


class IdentityManager:
    """基于 IDENTITY.md 的身份管理器"""

    def __init__(self, identity_content: str):
        self.identity = identity_content
        self.name = "KuDig Doctor"
        self.version = "v1.0"
        self.channel = OutputChannel.CLI

    def set_channel(self, channel: OutputChannel):
        """设置当前输出渠道"""
        self.channel = channel

    def generate_greeting(self, context: Optional[str] = None,
                         is_urgent: bool = False) -> str:
        """生成场景化问候"""
        if is_urgent:
            return ("P0 问题确认。进入快速诊断模式。\n"
                    "请提供: 1. Namespace  2. 资源名称  3. 异常时间")

        if context:
            return f"欢迎回来。\n上次在排查 {context}，需要继续吗？"

        return f"{self.name} {self.version} 就绪。有什么需要诊断的？"

    def format_output(self, diagnosis: dict) -> str:
        """根据渠道格式化输出"""
        formatters = {
            OutputChannel.CLI: self._format_cli,
            OutputChannel.WEB_UI: self._format_webui,
            OutputChannel.API: self._format_api,
            OutputChannel.TELEGRAM: self._format_telegram,
        }
        formatter = formatters.get(self.channel, self._format_cli)
        return formatter(diagnosis)

    def _format_cli(self, d: dict) -> str:
        """CLI 格式: 纯文本简洁"""
        return (f"根因: {d.get('root_cause', 'N/A')}\n"
                f"修复: {d.get('fix_command', 'N/A')}\n"
                f"验证: {d.get('verify_command', 'N/A')}")

    def _format_webui(self, d: dict) -> str:
        """WebUI 格式: 完整 Markdown"""
        return (f"## 诊断结果\n\n"
                f"### 根因\n{d.get('root_cause', 'N/A')}\n\n"
                f"### 修复方案\n```bash\n{d.get('fix_command', 'N/A')}\n```\n\n"
                f"### 验证\n```bash\n{d.get('verify_command', 'N/A')}\n```")

    def _format_api(self, d: dict) -> str:
        """API 格式: JSON 结构化"""
        import json
        return json.dumps({
            "diagnosis": {"root_cause": d.get("root_cause"),
                         "confidence": d.get("confidence", "高")},
            "fix": {"command": d.get("fix_command"),
                   "risk": d.get("risk", "低")},
        }, ensure_ascii=False, indent=2)

    def _format_telegram(self, d: dict) -> str:
        """Telegram 格式: 精简"""
        return f"根因: {d.get('root_cause', 'N/A')}\n修复: {d.get('fix_summary', 'N/A')}"

    def get_version_info(self) -> str:
        """返回版本标识（用户请求时显示）"""
        return f"[{self.name} {self.version} | Harness L3]"


# === 使用示例 ===
with open("openclaw-workspace/IDENTITY.md") as f:
    identity_content = f.read()

identity = IdentityManager(identity_content)

# 生成问候
greeting = identity.generate_greeting()
# → "KuDig Doctor v1.0 就绪。有什么需要诊断的？"

# 格式化诊断输出
diagnosis = {
    "root_cause": "Node CPU 不足 (38/40 节点 >90%)",
    "fix_command": "kubectl scale nodepool app --replicas=42",
    "verify_command": "kubectl top nodes | awk '$3>90'",
    "fix_summary": "扩容 2 个节点",
}

identity.set_channel(OutputChannel.WEB_UI)
output = identity.format_output(diagnosis)
# → 完整 Markdown 格式
```

---

## 6. 问题排除

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 输出风格不一致 | IDENTITY.md 风格描述不够具体 | 给出每种场景的具体示例 |
| 问候语太长/太短 | 未区分首次/继续/紧急场景 | 定义 3+ 种场景化问候模板 |
| 多渠道输出混乱 | 未实现渠道检测和适配 | 为每个渠道定义明确的格式规范 |
| SOUL.md 人格被 IDENTITY.md 覆盖 | 两者职责混淆 | 明确分离：SOUL=内在，IDENTITY=外在 |
| 版本信息频繁显示 | 未设置显示条件 | 默认关闭，仅在用户请求或报告模式显示 |

### 6.2 调试检查清单

```
IDENTITY.md 配置验证:

□ 基础标识：名称、代号、版本是否明确？
□ 品牌风格：人格关键词是否有 3-5 个？
□ 沟通调性：是否为 5 种场景定义了不同调性？
□ 问候模板：是否覆盖 首次/继续/紧急/巡检 场景？
□ 输出格式：是否定义了统一的诊断结果模板？
□ 多渠道：是否为 CLI/WebUI/API 等渠道定义了适配策略？
□ 与 SOUL.md 分离：修改 IDENTITY.md 是否不影响核心行为？
□ 与 USER.md 一致：输出风格是否匹配用户偏好？
```

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | IDENTITY.md 在 7 文件体系中的定位 |
| [openclaw-workspace/IDENTITY.md](./openclaw-workspace/IDENTITY.md) | KuDig Doctor 品牌标识完整配置 |
| [44 - SOUL.md 机制解析](./44-openclaw-soul-mechanism.md) | SOUL.md 内在人格 vs IDENTITY.md 外在形象 |
| [45 - USER.md 机制解析](./45-openclaw-user-mechanism.md) | USER.md 偏好对 IDENTITY.md 输出风格的约束 |
| [46 - AGENTS.md 机制解析](./46-openclaw-agents-mechanism.md) | 唤醒协议 Step 4 按 IDENTITY.md 输出问候 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容，深度解析 OpenClaw IDENTITY.md 的设计机制与工程实现。*

---

## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails|安全护栏、提示注入防护与合规]]

## See Also

- 48-openclaw-skill-mechanism
- 49-openclaw-memory-mechanism
- 01-ai-agent-fundamentals
- 02-llm-foundation-models

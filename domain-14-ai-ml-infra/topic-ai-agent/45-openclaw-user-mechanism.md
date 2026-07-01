---
title: OpenClaw USER.md 机制深度解析 (domain-14-ai-ml-infra)
description: 'title: OpenClaw USER.md 机制深度解析'
summary: 'title: OpenClaw USER.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- kubelet
- prometheus
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
- OpenClaw USER.md 机制深度解析 是什么
- 如何 OpenClaw USER.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- USER.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
---



title: OpenClaw USER.md 机制深度解析
description: '# OpenClaw USER.md 机制深度解析'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
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
- OpenClaw USER.md 机制深度解析 是什么
- 如何 OpenClaw USER.md 机制深度解析
trigger_keywords:
- OpenClaw
- USER.md
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

# OpenClaw USER.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, USER.md, 用户画像, Context 层, 去 AI 味, 沟通偏好, 个性化输出

---

## 概述

USER.md 是 OpenClaw File-First 架构中定义 **Agent 服务对象** 的配置文件。它告诉 Agent "你在服务谁"——用户的角色、技术水平、沟通偏好、工作场景和交互雷区。在 Harness Engineering 中主要映射到 **Context 层（用户上下文）**。

USER.md 是消除"AI 味"最关键的一环：没有用户画像，Agent 的输出是泛化的、模板化的；有了精确的用户画像，Agent 的输出才能像一个了解你的同事在跟你说话。

---

## 1. 设计原理

### 1.1 四象限模型

```
USER.md 四象限模型:

              高技术水平
                 │
    ┌────────────┼────────────┐
    │            │            │
    │  象限 A:   │  象限 B:   │
    │  高技术    │  高技术    │
    │  喜简洁    │  喜详细    │
    │  (ACK工程师)│ (新手架构师) │
偏简洁 ────────────┼──────────── 偏详细
    │  象限 C:   │  象限 D:   │
    │  低技术    │  低技术    │
    │  喜简洁    │  喜详细    │
    │  (业务方)   │ (初学者)    │
    │            │            │
    └────────────┼────────────┘
                 │
              低技术水平

不同象限的 Agent 输出策略完全不同:
  A: 省略基础解释，直接给命令和结论
  B: 给出结论后附加原理分析
  C: 用业务语言替代技术术语
  D: 每步都有详细解释和截图参考
```

### 1.2 去 AI 味三策略

| 策略 | 方法 | USER.md 配置项 |
|------|------|---------------|
| **风格校准** | 定义输出风格偏好 | 沟通偏好 → 语言风格、排版要求 |
| **黑名单过滤** | 列出禁止使用的表达 | 黑名单 → "祝您工作顺利"、"希望对您有帮助" |
| **知识校准** | 声明用户已知概念 | 不需要解释的概念列表 → Pod、Node、Deployment |

```
去 AI 味效果对比:

无 USER.md:
  "您好！很高兴为您服务。关于您提到的 Pod Pending 问题，
   Kubernetes 中的 Pod 是最小调度单元...（300 字解释 Pod 概念）
   ...建议您可以尝试以下方法... 希望对您有帮助！祝您工作顺利！"

有 USER.md:
  "根因: 节点 CPU Allocatable 已用尽。
   当前集群 40 个 worker 节点中 38 个 CPU requests > 90%。
   修复: kubectl top nodes 确认后，扩容 2 个 ecs.g7.4xlarge 节点。"
```

### 1.3 上下文注入深度

```
USER.md 注入 LLM 的策略:

完整注入（推荐初期）:
  将 USER.md 全文注入 system_prompt
  Token 消耗: ~500-800 tokens
  优点: 信息完整，Agent 理解全面
  缺点: 每次都消耗固定 Token

摘要注入（推荐成熟期）:
  提取关键字段注入:
    - 角色: ACK 运维工程师
    - 技术等级: 高级
    - 输出偏好: 结论前置、短句优先
    - 黑名单: [列表]
  Token 消耗: ~200 tokens
  优点: 省 Token
  缺点: 可能丢失细节偏好
```

---

## 2. Harness Engineering 映射

### 2.1 映射关系

```
USER.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
USER.md       │      │       │    ●    │         │        │           │

● = 主要映射（Context 层 — 用户上下文）
```

### 2.2 Context 层映射详解

| USER.md 内容 | Harness Context 实现 | 作用 |
|-------------|---------------------|------|
| 基础信息（角色、时区） | `UserProfile` — 用户身份元数据 | 确定服务模式和语言 |
| 沟通偏好（风格、排版） | `OutputFormatter` — 输出格式化 | 控制输出风格 |
| 技术背景（熟练度） | `ExpertiseLevelRouter` — 详细度调节 | 调整解释深度 |
| 黑名单表达 | `ExpressionFilter` — 表达过滤器 | 过滤禁用表达 |
| 工作焦点 | `ContextPrioritizer` — 上下文排序 | 主动关联相关信息 |
| 雷区 | `BehaviorGuard` — 与 SOUL.md 互补 | 避免触犯用户敏感点 |

---

## 3. K8S 运维实战案例

### 3.1 案例：技术水平校准

```
用户: ACK 运维工程师（高级）
问题: "集群 DNS 偶发 5s 超时"

无 USER.md 校准:
  "DNS（Domain Name System，域名系统）是互联网的核心服务之一...
   在 Kubernetes 中，CoreDNS 负责集群内部的 DNS 解析...
   Pod 通过 /etc/resolv.conf 配置文件指定 DNS 服务器..."
  → 300 字铺垫，用户已知内容，浪费时间

有 USER.md 校准:
  "DNS 5s 超时 → 高概率 conntrack race condition（KI-003）。
   验证: cat /proc/net/nf_conntrack | grep 'dport=53' | wc -l
   修复: CoreDNS 配置 force_tcp，或 Pod 启用 single-request-reopen。
   参考: domain-10-troubleshooting-diagnostics/26-dns-troubleshooting.md"
  → 直击问题，省略已知概念
```

### 3.2 案例：输出风格适配

```
USER.md 中定义的风格偏好:
  - 结论前置
  - 短句优先
  - 表格代替长文本
  - K8S 术语保留英文

适配后的诊断输出:

| 维度 | 状态 |
|------|------|
| **根因** | Node Memory Pressure → kubelet 驱逐 Pod |
| **影响** | 3 个 Deployment 共 12 个 Pod 被驱逐 |
| **紧急度** | P1 — 服务降级但未中断 |
| **修复** | 扩容节点内存或调整 eviction threshold |
| **验证** | kubectl get nodes -o wide 确认 Ready |
```

### 3.3 案例：黑名单表达过滤

```yaml
# USER.md 中定义的黑名单
communication_blacklist:
  - "您好，很高兴为您服务"
  - "希望对您有帮助"
  - "如果您有其他问题，随时问我"
  - "祝您工作顺利"
  - "让我们一起来看看"
  - "首先，让我解释一下什么是..."
  - "简单来说..."（用户不需要简化版）
```

---

## 4. 配置协作机制

### 4.1 USER.md 与其他文件的协作

```
USER.md 在配置体系中的角色:

SOUL.md ──→ USER.md
  │          SOUL.md 约束"什么不能做"
  │          USER.md 定义"怎么输出"
  │          SOUL.md 优先级 > USER.md
  │
USER.md ──→ AGENTS.md
  │          USER.md 的偏好影响 AGENTS.md 输出阶段格式
  │
USER.md ──→ IDENTITY.md
  │          USER.md 的沟通偏好约束 IDENTITY.md 的问候风格
  │
USER.md ──→ MEMORY.md
             MEMORY.md 的用户偏好部分从历史交互中学习
             逐步丰富 USER.md 的偏好画像
```

### 4.2 多用户场景

```
单 Agent 服务多用户时:

方案 1: 多 USER.md 文件
  workspace/
  ├── users/
  │   ├── user-allen.md     # ACK 运维工程师
  │   ├── user-developer.md # 应用开发者
  │   └── user-manager.md   # 技术经理
  └── USER.md → 当前会话用户（软链接或动态加载）

方案 2: USER.md 内分角色
  ## 角色 A: ACK 运维工程师
  ...
  ## 角色 B: 应用开发者
  ...
  # Agent 根据用户输入自动匹配角色
```

---

## 5. AgentScope 集成代码

### 5.1 UserContextBuilder 实现

```python
import yaml
from dataclasses import dataclass


@dataclass
class UserProfile:
    """从 USER.md 解析的用户画像"""
    role: str                   # 角色
    expertise_level: str        # 技术水平: 初级/中级/高级
    timezone: str               # 时区
    communication_style: str    # 沟通风格
    blacklist_expressions: list # 黑名单表达
    known_concepts: list        # 不需要解释的概念
    focus_areas: list           # 当前工作焦点


class UserContextBuilder:
    """从 USER.md 构建用户上下文"""

    def __init__(self, user_content: str):
        self.raw_content = user_content
        self.profile = self._parse_profile(user_content)

    def _parse_profile(self, content: str) -> UserProfile:
        """解析 USER.md 提取关键字段"""
        return UserProfile(
            role=self._extract_field(content, "角色"),
            expertise_level=self._extract_field(content, "K8S 经验"),
            timezone="Asia/Shanghai",
            communication_style="结论前置、短句优先",
            blacklist_expressions=self._extract_blacklist(content),
            known_concepts=["Pod", "Node", "Service", "Deployment",
                          "Namespace", "kubectl", "Prometheus"],
            focus_areas=["工单诊断效率", "Agent 辅助诊断"],
        )

    def build_context_prompt(self) -> str:
        """构建注入 LLM 的用户上下文"""
        return f"""## 用户画像
- 角色: {self.profile.role}
- 技术水平: {self.profile.expertise_level}
- 输出风格: {self.profile.communication_style}
- 禁止表达: {', '.join(self.profile.blacklist_expressions[:5])}
- 不需解释的概念: {', '.join(self.profile.known_concepts)}
"""

    def filter_output(self, output: str) -> str:
        """过滤输出中的黑名单表达"""
        result = output
        for expr in self.profile.blacklist_expressions:
            result = result.replace(expr, "")
        # 清理多余空行
        while "\n\n\n" in result:
            result = result.replace("\n\n\n", "\n\n")
        return result.strip()

    def _extract_field(self, content: str, field: str) -> str:
        """简单字段提取"""
        for line in content.split("\n"):
            if field in line:
                parts = line.split("|")
                if len(parts) >= 3:
                    return parts[2].strip().strip("*")
        return ""

    def _extract_blacklist(self, content: str) -> list:
        """提取黑名单表达"""
        return [
            "您好，很高兴为您服务",
            "希望对您有帮助",
            "祝您工作顺利",
            "让我们一起来看看",
        ]
```

### 5.2 与 SOUL.md 组合注入

```python
from agentscope.agent import ReActAgent


def create_user_aware_agent(workspace_path: str) -> ReActAgent:
    """创建感知用户画像的 Agent"""

    with open(f"{workspace_path}/SOUL.md") as f:
        soul_prompt = f.read()
    with open(f"{workspace_path}/USER.md") as f:
        user_content = f.read()

    user_ctx = UserContextBuilder(user_content)

    # 组合 system_prompt = SOUL + USER 摘要
    system_prompt = f"""{soul_prompt}

---

{user_ctx.build_context_prompt()}"""

    return ReActAgent(
        name="KuDig-Doctor",
        sys_prompt=system_prompt,
    )
```

---

## 6. 问题排除

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 输出仍有"AI 味" | 黑名单不全或未生效 | 补全黑名单，确认注入逻辑 |
| Agent 解释过多已知概念 | 技术水平未正确设置 | 明确 `K8S 经验: 高级`，列出不需解释的概念 |
| Agent 输出风格不匹配 | USER.md 沟通偏好描述模糊 | 给出具体的好例/坏例对比 |
| 多用户场景混淆 | 未做用户隔离 | 每个会话动态加载对应用户的 USER.md |
| Token 消耗过高 | USER.md 全文注入 | 改用摘要注入，只保留关键字段 |

### 6.2 调试检查清单

```
USER.md 配置验证:

□ 基础信息：角色和技术水平是否明确？
□ 沟通偏好：是否有具体的风格描述（不是"合适的风格"）？
□ 黑名单：是否列出了 5+ 条禁止表达？
□ 已知概念：是否列出用户熟悉的术语？
□ 工作焦点：是否反映当前阶段的重点？
□ 雷区：是否列出明确的禁忌行为？
□ Token 效率：全文 < 800 tokens / 摘要 < 200 tokens？
```

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | USER.md 在 7 文件体系中的定位 |
| [33 - Harness 上下文与记忆工程](./33-agent-harness-context-memory.md) | 四层上下文模型中的用户上下文 |
| [openclaw-workspace/USER.md](./openclaw-workspace/USER.md) | ACK 运维工程师用户画像完整配置 |
| [44 - SOUL.md 机制解析](./44-openclaw-soul-mechanism.md) | SOUL.md 与 USER.md 的互补关系 |
| [50 - IDENTITY.md 机制解析](./50-openclaw-identity-mechanism.md) | USER.md 偏好对 IDENTITY.md 输出风格的约束 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容，深度解析 OpenClaw USER.md 的设计机制与工程实现。*

---

## Obsidian 相关文档

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

- 43-openclaw-framework-integration
- 44-openclaw-soul-mechanism
- 46-openclaw-agents-mechanism
- 47-openclaw-tools-mechanism

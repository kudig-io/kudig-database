---
title: OpenClaw File-First 架构与 Agent Harness 集成指南 (AI基础设施)
description: 'title: OpenClaw File-First 架构与 Agent Harness 集成指南'
summary: 'title: OpenClaw File-First 架构与 Agent Harness 集成指南'
category: general
tags:
- ai
- ai-agent
- prometheus
- llm
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenClaw File-First 架构与 Agent Harness 集成指南 是什么
- 如何 OpenClaw File-First 架构与 Agent Harness 集成指南
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- File-First
- 架构与
- Agent
- Harness
- 集成指南
- ai
- ml
prerequisites:
- kubectl-basics
- prometheus-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: OpenClaw File-First 架构与 Agent Harness 集成指南
description: '# OpenClaw File-First 架构与 Agent Harness 集成指南'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 10min
intent_queries:
- OpenClaw File-First 架构与 Agent Harness 集成指南 是什么
- 如何 OpenClaw File-First 架构与 Agent Harness 集成指南
trigger_keywords:
- OpenClaw
- File-First
- 架构与
- Agent
- Harness
- 集成指南
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

# OpenClaw File-First 架构与 Agent Harness 集成指南

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, File-First Architecture, SOUL.md, USER.md, AGENTS.md, TOOLS.md, SKILL.md, MEMORY.md, IDENTITY.md, Agent Harness, Harness Engineering, K8S 运维 Agent

---

<!-- chunk: 概述 -->## 概述

OpenClaw 是一个以 **File-First（文件优先）架构** 为核心设计理念的 AI Agent 框架。它将 Agent 的全部配置——人格、行为规范、工具授权、技能知识、记忆系统、身份标识——全部以 Markdown 文件形式管理，放弃了传统的代码配置或 API 参数注入方式。

这种设计在 Agent 个性化定制和快速迭代场景中具有独特优势，但也带来了 Token 消耗和运行效率的挑战。本文系统梳理 OpenClaw 的 7 大核心配置文件体系，分析其与 Harness Engineering 六层架构的映射关系，并提供面向 K8S 运维 Agent 的完整实施方案。

**核心价值**：理解 OpenClaw 的 File-First 架构，即使不使用 OpenClaw 本身，也能将其设计理念应用到 AgentScope、LangChain 等任何 Agent 框架中，提升 Agent 的可配置性和可维护性。

---

<!-- chunk: 1. OpenClaw 核心概念 -->## 1. OpenClaw 核心概念

## 1.1 什么是 File-First 架构

```
传统 Agent 配置方式:
  代码中硬编码 system_prompt → 修改需要重新部署
  API 参数注入 → 分散在各处，难以版本管理
  数据库存储 → 需要管理后台，修改门槛高

OpenClaw File-First 架构:
  所有配置集中在 ~/.openclaw/workspace/ 目录下
  全部以 Markdown 文件形式存在
  Git 版本控制 → 可追踪、可回滚、可协作
  直接编辑文本 → 零门槛修改
  Agent 运行时读取 → 热更新无需重启
```

## 1.2 七大核心配置文件

| 文件 | 职责 | Harness 层映射 | 类比 |
|------|------|---------------|------|
| **SOUL.md** | 角色人格、核心价值观、绝对红线 | Constraints 层 | "你是谁" |
| **USER.md** | 用户画像、偏好、交互雷区 | Context 层 | "服务谁" |
| **AGENTS.md** | 行为规范、唤醒协议、任务处理流程 | Loop 层 | "怎么干活" |
| **TOOLS.md** | 工具授权注册表、调用参数规范 | Tools 层 | "能用什么" |
| **SKILL.md** | 领域知识、专业技能、SOP 流程 | Context + Loop 层 | "会什么" |
| **MEMORY.md** | 长期记忆、经验积累、学习反馈 | Persistence 层 | "记住什么" |
| **IDENTITY.md** | 外观标识、问候语、交互风格 | — (展示层) | "长什么样" |

```
文件层级结构:

~/.openclaw/workspace/          # 或项目自定义路径
├── SOUL.md                     # 角色人格与绝对红线
├── USER.md                     # 用户画像与偏好
├── AGENTS.md                   # 行为规范与工作流
├── TOOLS.md                    # 工具授权与配置
├── SKILL.md                    # 领域知识与 SOP
├── MEMORY.md                   # 长期记忆与经验
├── IDENTITY.md                 # 对外身份与品牌
├── memory/                     # 日常运行日志与短期上下文
│   ├── 2026-04-01.md
│   ├── 2026-04-02.md
│   └── ...
└── skills/                     # 扩展技能目录（Anthropic Skill 规范）
    ├── k8s-pod-diagnosis/
    │   └── SKILL.md
    └── k8s-node-diagnosis/
        └── SKILL.md
```

## 1.3 File-First vs Harness Engineering 对比

```
设计理念对比:

OpenClaw File-First:
  ✓ 所有配置以 Markdown 明文存放
  ✓ 直观、易编辑、Git 友好
  ✓ 适合个人助手、小团队快速定制
  ✗ Token 消耗高（每次都要完整读取所有文件）
  ✗ 不支持 RAG（宁可牺牲效率也拒绝向量检索）
  ✗ 缺少运行时验证和安全沙箱

Harness Engineering 六层架构:
  ✓ 分层设计，每层独立优化
  ✓ 支持 RAG、向量检索、动态上下文构建
  ✓ 完整的验证层 + 约束层 + 可观测性
  ✓ 适合企业级生产 Agent
  ✗ 工程复杂度高
  ✗ 需要代码开发，修改门槛高

最佳实践 — 融合方案:
  用 File-First 管理"静态配置"（人格、规则、SOP）
  用 Harness 管理"动态行为"（循环、验证、约束执行）
  = 可配置性 + 生产可靠性
```

---

<!-- chunk: 2. 七大核心文件详解 -->## 2. 七大核心文件详解

## 2.1 SOUL.md — 角色人格与绝对红线

**定位**：Agent 的"灵魂"，定义核心身份、价值观和不可逾越的行为边界。

**对应 Harness 层**：Constraints 层 + System Context

```
SOUL.md 核心结构:

1. 核心身份与人格
   - 角色设定（"你是 K8S 运维诊断专家"）
   - 沟通风格（"技术问题精准回复，非技术问题简洁拒绝"）
   - 术语规范（"K8S 术语保留英文，解释用中文"）

2. 核心价值观与绝对红线
   - 安全边界（"生产环境禁止 delete/drain 操作"）
   - 隐私保护（"禁止输出任何 Secret/密钥内容"）
   - 诚实原则（"不确定时标注'需人工确认'"）

3. 长期指令与生存法则
   - 记忆连续性规则
   - 风险行为拦截机制
   - 输出格式强制约束
```

**关键设计原则**：越具体越好。"要有帮助" 产生模糊行为，"每个诊断必须包含 Event 证据引用" 产生精确行为。

## 2.2 USER.md — 用户画像与偏好

**定位**：写给 Agent 的"用户使用说明书"，让 Agent 理解服务对象。

**对应 Harness 层**：Context 层（用户上下文）

```
USER.md 核心结构:

1. 基础信息
   - 称呼、时区、角色（"ACK 工单负责人"）

2. 沟通与排版偏好
   - 语言风格（"短句优先，结论前置"）
   - 排版要求（"技术术语保留英文，关键结论加粗"）
   - 黑名单（"禁止空洞的客套话"）

3. 当前工作焦点
   - 正在处理的项目和关注领域
   - Agent 可据此主动提供相关建议

4. 雷区与禁忌
   - 绝对不要做的事（"不要随便修改线上配置"）
   - 敏感话题（"不讨论与工作无关的话题"）
```

**核心价值**：USER.md 是过滤"AI 味"最重要的一环——让 Agent 的输出风格匹配用户的实际期望。

## 2.3 AGENTS.md — 行为规范与工作流

**定位**：Agent 的"日常行为配置"，定义任务处理流程和决策规范。

**对应 Harness 层**：Loop 层（执行引擎 + 流程编排）

```
AGENTS.md 核心结构:

1. 唤醒协议（每次会话启动前执行）
   - 读取 SOUL.md → 确认身份
   - 读取 USER.md → 确认用户
   - 读取 memory/ → 获取最近上下文

2. 任务处理流程
   - 信息采集 → 根因分析 → 方案生成 → 安全评审 → 输出
   - 异常处理：超时、信息不足、安全风险各自的分支逻辑

3. 记忆管理规则
   - 每日流水存入 memory/YYYY-MM-DD.md
   - 定期精华提炼更新到 MEMORY.md
   - 重要发现标记为 "key_insight"

4. 绝对红线执行规则
   - 破坏性操作必须请求确认
   - 不确定时承认不确定
   - 引用必须标注来源
```

## 2.4 TOOLS.md — 工具授权注册表

**定位**：定义 Agent 被授权使用的工具集、调用参数和安全规范。

**对应 Harness 层**：Tools 层

```
TOOLS.md 核心结构:

1. 授权工具清单
   - 每个工具的名称、用途、参数规范
   - 工具的权限级别（只读 / 有限写 / 完全写）

2. 工具使用优先级
   - 优先使用只读工具收集信息
   - 写操作工具需要额外确认

3. 工具组合策略
   - 诊断场景的工具调用序列
   - 常见工具链模板

4. 安全约束
   - 禁止调用的命令模式
   - 敏感参数脱敏规则
```

**与 Anthropic Agent Skill 的关系**：TOOLS.md 定义"能力"（Agent 能做什么），SKILL.md 定义"知识"（Agent 怎么做）。

## 2.5 SKILL.md — 领域知识与 SOP

**定位**：Agent 的"专业教科书"，将领域知识结构化为可执行的标准操作流程。

**对应 Harness 层**：Context 层（知识上下文）+ Loop 层（SOP 驱动执行）

```
SKILL.md 核心结构:

1. 技能领域声明
   - 覆盖范围（K8S 故障诊断：Pod/Node/Network/Storage）
   - 专业深度（L1 基础诊断 → L3 深度根因分析）

2. 诊断 SOP 库
   - 每个问题类型的标准操作流程
   - Step-by-Step 命令序列
   - 分支决策树（根据症状选择路径）

3. 知识语料库引用
   - kudig-database 知识库关联
   - 外部文档参考链接

4. 输出格式模板
   - 诊断报告格式（现象→根因→修复→验证→预防）
   - 事故报告模板
```

**与 Anthropic Agent Skill 的兼容**：OpenClaw 的 SKILL.md 与 Anthropic 提出的 Agent Skill 规范完全兼容，可直接注册到 AgentScope 的 Toolkit 中。

## 2.6 MEMORY.md — 记忆系统与经验积累

**定位**：Agent 的"长期记忆"，存储跨会话的经验、模式和规则。

**对应 Harness 层**：Persistence 层

```
MEMORY.md 核心结构:

1. 确定性规则（手动维护）
   - 集群环境信息（节点数、版本、网络方案）
   - 已知问题与解决方案
   - 团队约定与流程规范

2. 经验模式（Agent 自动提炼）
   - 高频故障模式与根因
   - 成功诊断路径记录
   - 失败案例与教训

3. 用户偏好记忆
   - 用户常用命令习惯
   - 用户关注的指标偏好
   - 历史交互中的反馈

4. 元数据
   - 记忆条目的创建/更新时间
   - 置信度和使用频率
   - 过期淘汰策略
```

**记忆流转机制**：
```
短期记忆（memory/YYYY-MM-DD.md）
    │
    ▼ 定期提炼
情景记忆（重要事件和解决方案）
    │
    ▼ 模式抽象
语义记忆（MEMORY.md 中的规则和模式）
    │
    ▼ 检索注入
短期记忆（下次会话时注入上下文）
```

## 2.7 IDENTITY.md — 对外身份与品牌

**定位**：定义 Agent 的"外在形象"——名称、风格、问候语。

**对应 Harness 层**：无直接映射（展示层）

```
IDENTITY.md 核心结构:

1. 基础标识
   - 名称、代号、版本
   - 图标/头像描述

2. 交互风格
   - 问候语模板
   - 结束语模板
   - 错误/异常时的表达方式

3. 品牌一致性
   - 术语规范
   - 输出格式统一
   - 多渠道展示适配
```

**SOUL.md vs IDENTITY.md 的分离设计**：SOUL.md 告诉 Agent "你是谁"（内在人格），IDENTITY.md 告诉用户 "Agent 长什么样"（外在形象）。可以随时调整外在形象而保持核心人格不变。

---

<!-- chunk: 3. OpenClaw × Harness Engineering 映射矩阵 -->## 3. OpenClaw × Harness Engineering 映射矩阵

## 3.1 完整映射关系

```
OpenClaw 7 文件 × Harness 6 层 映射矩阵:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
SOUL.md       │      │       │    ◐    │         │        │     ●     │
USER.md       │      │       │    ●    │         │        │           │
AGENTS.md     │  ●   │       │    ◐    │         │   ◐    │     ◐     │
TOOLS.md      │      │   ●   │         │         │        │     ◐     │
SKILL.md      │  ◐   │       │    ●    │         │        │           │
MEMORY.md     │      │       │    ◐    │    ●    │        │           │
IDENTITY.md   │      │       │    ◐    │         │        │           │

● = 主要映射    ◐ = 次要映射
```

## 3.2 融合实现策略

```python
class HybridHarness:
    """File-First + Harness Engineering 融合方案"""

    def __init__(self, workspace_path: str):
        # File-First 层：从 Markdown 文件加载静态配置
        self.soul = self._load_md(f"{workspace_path}/SOUL.md")
        self.user = self._load_md(f"{workspace_path}/USER.md")
        self.agents = self._load_md(f"{workspace_path}/AGENTS.md")
        self.tools_config = self._load_md(f"{workspace_path}/TOOLS.md")
        self.skill = self._load_md(f"{workspace_path}/SKILL.md")
        self.memory = self._load_md(f"{workspace_path}/MEMORY.md")
        self.identity = self._load_md(f"{workspace_path}/IDENTITY.md")

        # Harness 层：从文件配置构建运行时组件
        self.loop = self._build_loop(self.agents)
        self.tools = self._build_tools(self.tools_config)
        self.context = self._build_context(self.soul, self.user, self.skill)
        self.persistence = self._build_persistence(self.memory)
        self.verification = self._build_verification(self.soul)
        self.constraints = self._build_constraints(self.soul, self.tools_config)

    def _build_context(self, soul, user, skill):
        """将 SOUL.md + USER.md + SKILL.md 组装为上下文"""
        return ContextManager(
            system_prompt=soul,         # SOUL.md → System Context
            user_context=user,          # USER.md → User Context
            knowledge_base=skill,       # SKILL.md → Knowledge Context
        )

    def _build_constraints(self, soul, tools_config):
        """从 SOUL.md 红线 + TOOLS.md 权限提取约束"""
        return ConstraintEnforcer(
            blocked_commands=extract_redlines(soul),
            allowed_tools=extract_tool_permissions(tools_config),
            read_only_mode=is_readonly(soul),
        )
```

---

<!-- chunk: 4. K8S 运维 Agent 实施方案 -->## 4. K8S 运维 Agent 实施方案

## 4.1 工作区目录设计

本项目提供一套完整的 K8S 运维 Agent 工作区配置，位于 `openclaw-workspace/` 目录：

```
# 🟢 低风险：只读/信息收集，通常无副作用
AI基础设施/02-ai-agents/openclaw-workspace/
├── SOUL.md         # K8S 运维诊断专家人格
├── USER.md         # ACK 运维工程师用户画像
├── AGENTS.md       # 工单诊断工作流与行为规范
├── TOOLS.md        # kubectl/prometheus/loki 工具授权
├── SKILL.md        # K8S 故障诊断 SOP 库
├── MEMORY.md       # 集群环境记忆与诊断经验
└── IDENTITY.md     # 诊断助手品牌标识
```
## 4.2 配置文件加载顺序

```
Agent 启动时的配置加载序列:

1. IDENTITY.md → 确定外在形象（问候语、名称）
2. SOUL.md → 加载核心人格和绝对红线（最高优先级）
3. USER.md → 加载用户画像和偏好
4. AGENTS.md → 加载行为规范和工作流
5. TOOLS.md → 注册授权工具集
6. SKILL.md → 加载领域知识（按需或预加载）
7. MEMORY.md → 加载长期记忆和经验

唤醒协议（每次会话开始）:
  SOUL.md → USER.md → MEMORY.md（最近 3 天） → AGENTS.md → 就绪
```

## 4.3 与 AgentScope 集成

```python
import agentscope
from agentscope.agent import ReActAgent
from agentscope.tool import Toolkit, execute_shell_command, view_text_file

# 1. 加载 OpenClaw 工作区配置
workspace = "AI基础设施/02-ai-agents/openclaw-workspace"

# 2. 读取 SOUL.md 作为 sys_prompt
with open(f"{workspace}/SOUL.md") as f:
    soul_prompt = f.read()

# 3. 读取 USER.md 作为用户上下文
with open(f"{workspace}/USER.md") as f:
    user_context = f.read()

# 4. 构建 sys_prompt = SOUL + USER
system_prompt = f"{soul_prompt}\n\n---\n\n<!-- chunk: 用户画像\n{user_context}" -->## 用户画像\n{user_context}"

# 5. 注册工具（遵循 TOOLS.md 中的授权清单）
toolkit = Toolkit()
toolkit.register_tool_function(execute_shell_command)
toolkit.register_tool_function(view_text_file)

# 6. 注册 Skill（SKILL.md 作为 Agent Skill）
toolkit.register_agent_skill(f"{workspace}")

# 7. 创建 Agent
agent = ReActAgent(
    name="KuDig-Doctor",      # 来自 IDENTITY.md
    sys_prompt=system_prompt,   # 来自 SOUL.md + USER.md
    toolkit=toolkit,            # 来自 TOOLS.md + SKILL.md
    # memory 配置来自 MEMORY.md
    # 行为约束来自 AGENTS.md（注入到 sys_prompt 的约束部分）
)
```

---

<!-- chunk: 5. 与现有知识体系的关联 -->## 5. 与现有知识体系的关联

## 5.1 OpenClaw 文件 × kudig-database 知识域映射

| OpenClaw 文件 | 关联的 kudig-database 知识域 | 知识注入方式 |
|--------------|---------------------------|-------------|
| SOUL.md | [安全](../安全/) | 安全红线规则来源 |
| USER.md | [平台工程](../平台工程/) | 运维工程师角色定义 |
| AGENTS.md | [故障诊断](../故障诊断/) | 诊断工作流模板 |
| TOOLS.md | [网络](../网络/), [可观测性](../可观测性/) | 工具使用规范 |
| SKILL.md | [topic-fta](../故障诊断/FTA故障树/), [topic-structural-trouble-shooting](../故障诊断/高级排障/) | 故障树 + SOP 知识 |
| MEMORY.md | [domain-33-kubernetes-events](../系统基础/) | 事件模式知识库 |
| IDENTITY.md | — | Agent 品牌独立设计 |

## 5.2 与 Harness Engineering 系列的关联

| OpenClaw 机制 | 对应 Harness 文档 | 深度参考 |
|--------------|------------------|---------|
| SOUL.md 红线设计 | [35 - 安全与约束工程](./35-agent-harness-security-constraints.md) | 四层约束模型 |
| AGENTS.md 工作流 | [31 - Loop 与执行引擎](./31-agent-harness-loop-execution.md) | FSM 状态机、反漂移 |
| TOOLS.md 工具授权 | [32 - 工具工程](./32-agent-harness-tool-engineering.md) | Schema 标准、安全沙箱 |
| SKILL.md 知识注入 | [33 - 上下文与记忆工程](./33-agent-harness-context-memory.md) | 四层上下文、RAG |
| MEMORY.md 记忆系统 | [33 - 上下文与记忆工程](./33-agent-harness-context-memory.md) | 三层记忆模型 |
| 整体验证 | [34 - 验证与质量门禁](./34-agent-harness-verification-quality.md) | 自检循环 |
| 可观测性 | [36 - 可观测性体系](./36-agent-harness-observability.md) | OTel 追踪 |

---

<!-- chunk: 6. 最佳实践与反模式 -->## 6. 最佳实践与反模式

## 最佳实践

| 实践 | 说明 | 来源 |
|------|------|------|
| **SOUL.md 越具体越好** | "禁止 delete namespace" 比 "注意安全" 有效 100 倍 | OpenClaw 核心设计 |
| **USER.md 过滤 AI 味** | 定义输出风格偏好，消除模板化表达 | 用户体验优化 |
| **AGENTS.md 唤醒协议** | 每次会话先加载核心配置，确保一致性 | 状态管理最佳实践 |
| **TOOLS.md 最小权限** | 只授权必需工具，精简 > 全面 | Vercel: 15→2 工具 |
| **SKILL.md 按域拆分** | 每个 Skill 聚焦一类问题，避免万能 Skill | Anthropic Agent Skill |
| **MEMORY.md 定期清理** | 过时记忆产生噪声，定期淘汰低价值条目 | 信噪比原则 |
| **Git 版本管理** | 所有配置文件纳入 Git，变更可追溯 | DevOps 基础实践 |
| **CI 质量门禁** | SOUL.md/SKILL.md 变更触发 Harness 回归测试 | Harness QA |

## 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **SOUL.md 内容模糊** | "要有帮助" → 行为不可预测 | 给出具体的行为约束条件 |
| **所有知识塞入 SKILL.md** | Token 爆炸，Agent 迷失 | 按问题域拆分为多个 Skill 目录 |
| **MEMORY.md 从不清理** | 过时记忆误导决策 | 设置过期机制，定期审查 |
| **跳过 AGENTS.md 唤醒** | 每次会话行为不一致 | 强制执行唤醒协议 |
| **TOOLS.md 授权过多** | 工具选择混乱，准确率下降 | 最小必要工具集 |
| **不做 Git 管理** | 配置变更不可追溯 | 所有 .md 文件纳入版本控制 |

---

<!-- chunk: 7. File-First 架构的适用性分析 -->## 7. File-First 架构的适用性分析

## 7.1 适用场景

| 场景 | 适合度 | 理由 |
|------|--------|------|
| 个人 AI 助手定制 | 极高 | 直接编辑文件即可调整人格和行为 |
| 小团队快速原型 | 高 | 零代码修改配置，快速迭代 |
| K8S 运维 Agent | 高 | SOP 自然以文档形式存在，直接作为 SKILL.md |
| 企业级多租户平台 | 中 | 需要额外的动态加载和权限隔离机制 |
| 高并发实时系统 | 低 | 文件读取 + 全量注入的 Token 开销过大 |

## 7.2 与其他 Agent 框架的兼容性

| 框架 | 兼容方式 | 难度 |
|------|---------|------|
| **AgentScope** | SOUL.md → sys_prompt，SKILL.md → Agent Skill | 低：原生支持 |
| **LangChain** | SOUL.md → SystemMessage，SKILL.md → RAG 文档 | 低：直接注入 |
| **Claude Code** | SOUL.md → CLAUDE.md，SKILL.md → 项目文档 | 低：文件约定类似 |
| **OpenAI Codex** | SOUL.md → system instructions | 低：标准接口 |
| **AutoGen/CrewAI** | SOUL.md → agent config，TOOLS.md → tool registration | 中：需适配层 |

---

<!-- chunk: 关联文档 -->## 关联文档

## 子文档索引（各配置文件深度机制解析）

| 序号 | 文档 | 内容概要 | 阅读耗时 |
|:---:|------|---------|:-------:|
| 44 | [SOUL.md 机制深度解析](./44-openclaw-soul-mechanism.md) | 三层结构模型、约束精确性原则、SoulConstraintEnforcer 代码、红线拦截案例 | 25min |
| 45 | [USER.md 机制深度解析](./45-openclaw-user-mechanism.md) | 四象限模型、去 AI 味三策略、UserContextBuilder 代码、技术水平校准 | 25min |
| 46 | [AGENTS.md 机制深度解析](./46-openclaw-agents-mechanism.md) | FSM 状态机、五阶段工作流、反漂移检测、AgentWorkflowEngine 代码 | 30min |
| 47 | [TOOLS.md 机制深度解析](./47-openclaw-tools-mechanism.md) | 四级权限模型、最小权限原则、ToolsManager 双重安全检查代码 | 25min |
| 48 | [SKILL.md 机制深度解析](./48-openclaw-skill-mechanism.md) | 渐进式披露、三种知识结构化范式、SkillLoader 按需加载代码 | 25min |
| 49 | [MEMORY.md 机制深度解析](./49-openclaw-memory-mechanism.md) | 三层记忆模型、新陈代谢机制、MemoryManager 代码、已知问题命中 | 25min |
| 50 | [IDENTITY.md 机制深度解析](./50-openclaw-identity-mechanism.md) | SOUL/IDENTITY 分离设计、多渠道适配、IdentityManager 代码 | 20min |

## Harness Engineering 关联

| 文档 | 关联内容 |
|------|--------|
| [29 - AgentScope Studio & Skill 实战](./29-agentscope-studio-skill-demo.md) | SKILL.md 规范、Agent Skill 注册机制 |
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 六层架构、SOUL.md/SKILL.md 分层设计 |
| [33 - 上下文与记忆工程](./33-agent-harness-context-memory.md) | 四层上下文模型、三层记忆系统 |
| [35 - 安全与约束工程](./35-agent-harness-security-constraints.md) | SOUL.md 红线的工程化实现 |
| [40 - 生产运维与成熟度](./40-agent-harness-production-maturity.md) | SOUL.md/SKILL.md 的 K8S ConfigMap 部署 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| OpenClaw 官方文档 | File-First 架构设计、7 大配置文件规范 | 2025-2026 |
| Anthropic | Agent Skill 规范（SKILL.md 标准） | 2025 |
| Martin Fowler / Birgitta Böckeler | Harness Engineering 概念定义 | 2026-02 |
| LobeHub Skills Marketplace | OpenClaw Skill 生态与实践 | 2026 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容，基于 OpenClaw 框架设计理念与 Harness Engineering 最佳实践编写。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents MOC
- [[AI基础设施/AI-Agents/README.md|AI Agent 工程专题]]
- [[AI基础设施/AI-Agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[AI基础设施/AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[AI基础设施/AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[AI基础设施/AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[AI基础设施/AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[AI基础设施/AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[AI基础设施/AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[AI基础设施/AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[AI基础设施/AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[AI基础设施/AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 41-react-harness-identification-guide
- 42-model-harness-compatibility-matrix
- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism


<!-- risk-assessed -->

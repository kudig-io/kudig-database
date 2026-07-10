---
title: OpenClaw AGENTS.md 机制深度解析 (AI基础设施)
description: 'title: OpenClaw AGENTS.md 机制深度解析'
summary: 'title: OpenClaw AGENTS.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- coredns
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
- OpenClaw AGENTS.md 机制深度解析 是什么
- 如何 OpenClaw AGENTS.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- AGENTS.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: OpenClaw AGENTS.md 机制深度解析
description: '# OpenClaw AGENTS.md 机制深度解析'
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
- OpenClaw AGENTS.md 机制深度解析 是什么
- 如何 OpenClaw AGENTS.md 机制深度解析
trigger_keywords:
- OpenClaw
- AGENTS.md
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

# OpenClaw AGENTS.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, AGENTS.md, 行为规范, 工作流, Loop 层, FSM 状态机, 唤醒协议, 反漂移检测

---

## 概述

AGENTS.md 是 OpenClaw File-First 架构中定义 **Agent 行为规范和任务处理流程** 的核心配置文件。它告诉 Agent "怎么干活"——从会话启动的唤醒协议，到任务分类、诊断工作流、异常处理，再到质量标准。在 Harness Engineering 中主要映射到 **Loop 层（执行引擎）**。

AGENTS.md 是将"非确定性 LLM 行为"转化为"确定性工作流"的关键——它用有限状态机（FSM）模型将 Agent 的行为约束在可预测的状态转换路径中。

---

## 1. 设计原理

### 1.1 FSM 状态机模型

```
AGENTS.md 定义的状态机:

正常路径:
  IDLE → TRIAGE → DIAGNOSE → PLAN → REVIEW → OUTPUT
    │       │         │         │       │        │
    │       │         │         │       │        └→ 输出诊断结果
    │       │         │         │       └→ SOUL.md 安全评审
    │       │         │         └→ 生成修复方案
    │       │         └→ 根因分析
    │       └→ 任务分类与优先级判定
    └→ 空闲等待用户输入

异常路径:
  任意状态 → ASK_INFO    （信息不足 → 请求补充）
  任意状态 → RETRY       （工具调用失败 → 重试）
  DIAGNOSE → ESCALATE    （超出能力范围 → 升级）
  任意状态 → BLOCKED     （触发 SOUL.md 红线 → 拦截）

反漂移:
  DIAGNOSE ──3次相同命令──→ BLOCKED（强制中断）
```

### 1.2 唤醒协议设计

```
为什么需要唤醒协议:

问题: LLM 每次会话是无状态的，不会"记住"自己是谁
  Session 1: Agent 表现如 SOUL.md 定义
  Session 2: Agent 可能忘记红线，行为不一致

解决: 每次会话开始强制执行唤醒序列
  Step 1: 加载 SOUL.md → 确认身份和红线
  Step 2: 加载 USER.md → 确认服务对象
  Step 3: 加载 MEMORY.md → 恢复上下文记忆
  Step 4: 就绪确认 → 按 IDENTITY.md 风格输出问候

效果: 每次会话都从一致的起点开始
```

### 1.3 五阶段工作流

```
# 🟢 低风险：只读/信息收集，通常无副作用
诊断工作流五阶段与资源分配:

Phase 1: 信息采集（30% Token 预算）
  │  宏观→微观: get → describe → logs
  │  工具: kubectl get/describe/logs/top
  │  输出: 原始数据收集完成
  │
Phase 2: 根因分析（25% Token 预算）
  │  方法: 排除法 + 故障树推理
  │  原则: 每个结论必须有数据支撑
  │  输出: 根因假设 + 置信度
  │
Phase 3: 方案生成（20% Token 预算）
  │  要求: 命令可直接复制执行
  │  内容: 修复步骤 + 风险评估 + 回滚方案
  │
Phase 4: 安全评审（15% Token 预算）
  │  检查: SOUL.md 红线 + TOOLS.md 权限
  │  决策: 通过/拦截/需确认
  │
Phase 5: 输出与闭环（10% Token 预算）
     格式: 现象→根因→修复→验证→预防
     记忆: 关键发现写入 memory/
```
---

## 2. Harness Engineering 映射

### 2.1 映射关系

```
AGENTS.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
AGENTS.md     │  ●   │       │    ◐    │         │   ◐    │     ◐     │

● = 主要映射（Loop 层 — 执行引擎）
◐ = 次要映射（Context: 工作流上下文 / Verify: 质量自检 / Constrain: 行为约束）
```

### 2.2 Loop 层映射详解

| AGENTS.md 内容 | Harness Loop 实现 | 执行方式 |
|---------------|-------------------|---------|
| 唤醒协议（1） | `InitSequence` — 启动前配置加载 | 每次会话首轮执行 |
| 任务分类（2） | `TaskRouter` — 关键词→任务类型路由 | 用户输入后第一步 |
| 五阶段工作流（3） | `DiagnosisPipeline` — 五阶段管道 | FSM 状态转换 |
| 异常处理（3.2） | `ExceptionHandler` — 异常分支路由 | 每步失败后触发 |
| 记忆管理（4） | `MemoryWriter` — 结果写入 memory/ | Phase 5 后触发 |
| 多 Agent 协作（5） | `AgentOrchestrator` — 任务转发 | 检测到超出能力时 |
| 质量标准（6） | `QualityChecker` — 自检清单 | 输出前执行 |

### 2.3 反漂移检测机制

```
# 🟢 低风险：只读/信息收集，通常无副作用
反漂移检测（Anti-Drift Detection）:

问题: Agent 在 DIAGNOSE 阶段可能陷入循环
  → 重复执行相同的 kubectl 命令
  → 无法推进到下一阶段
  → Token 被浪费

机制:
  维护最近 N 次工具调用的 hash
  if 连续 3 次 hash 相同:
    → 强制中断当前阶段
    → 输出: "诊断陷入循环，已尝试 {N} 次相同操作无进展"
    → 选项: a) 请求用户补充信息  b) 切换诊断策略  c) 升级人工

实现:
  command_history = []
  for each tool_call:
    h = hash(tool_call.name + tool_call.args)
    if command_history[-3:] == [h, h, h]:
      raise DriftDetected("连续3次相同操作")
    command_history.append(h)
```
---

## 3. K8S 运维实战案例

### 3.1 案例：唤醒协议执行

```
会话开始时的唤醒序列:

[内部执行 - 用户不可见]
  1. 读取 SOUL.md → ✓ 确认身份 "KuDig Doctor"，安全红线已激活
  2. 读取 USER.md → ✓ 服务对象 "ACK 运维工程师"，输出风格: 结论前置
  3. 读取 MEMORY.md → ✓ 加载长期记忆
     + 读取 memory/2026-04-01.md ~ 04-03.md → 最近 3 天上下文
     → 发现: 昨天 ack-prod-hangzhou 集群有 3 个 Pod OOM
  4. 就绪确认

[用户可见输出]
  "KuDig Doctor 就绪。
   注意到昨天 ack-prod-hangzhou 集群发生 3 次 OOM，是否需要跟进？"
```

### 3.2 案例：任务分类与路由

```
用户输入: "有几个 Pod 一直 Pending"

任务分类:
  关键词匹配: "Pending" → Pod 调度诊断
  优先级判定: 需确认 — 生产环境? 服务影响?

Agent 确认:
  "检测到 Pod Pending 问题。确认信息:
   1. 哪个集群/Namespace？
   2. 是否影响线上服务？（决定优先级 P0-P2）"

用户回复: "ack-prod-hangzhou，kube-system 下的 coredns"

重新判定:
  集群: 生产  +  组件: CoreDNS(基础设施)  →  P0 紧急
  启动五阶段诊断，时间预算: 15 分钟
```

### 3.3 案例：异常处理 — 信息不足

```
# 🟢 低风险：只读/信息收集，通常无副作用
Phase 1 信息采集:
  kubectl get pods -n kube-system | grep coredns
  → 错误: "Error from server (Forbidden): pods is forbidden"

异常处理分支: ASK_INFO
  Agent 输出:
    "当前 kubeconfig 权限不足，无法访问 kube-system。
     请确认:
     1. kubectl auth can-i get pods -n kube-system
     2. 如需切换上下文: kubectl config use-context <admin-context>
     3. 或者提供 describe 和 events 的输出，我来分析"

状态: DIAGNOSE → ASK_INFO → 等待用户补充 → 恢复到 DIAGNOSE
```
---

## 4. 配置协作机制

### 4.1 AGENTS.md 与其他文件的协作

```
AGENTS.md 在配置体系中的编排角色:

AGENTS.md = 流程编排器，引用所有其他文件:

  唤醒协议:
    Step 1 → 加载 SOUL.md
    Step 2 → 加载 USER.md
    Step 3 → 加载 MEMORY.md
    Step 4 → 按 IDENTITY.md 输出问候

  诊断工作流:
    Phase 1 → 使用 TOOLS.md 授权的工具
    Phase 2 → 参考 SKILL.md 的诊断 SOP
    Phase 4 → 对照 SOUL.md 安全评审
    Phase 5 → 按 USER.md 偏好格式化输出
              + 写入 MEMORY.md 记忆系统
```

### 4.2 工作流自定义

```
不同场景的工作流变体:

标准诊断（默认）: 5 阶段完整流程
  采集 → 分析 → 方案 → 评审 → 输出

快速诊断（P0 紧急）: 压缩为 3 阶段
  快速采集 → 直接方案 → 输出
  跳过深度分析，优先给出缓解方案

深度分析（P3 低优先级）: 扩展为 7 阶段
  采集 → 分析 → 趋势分析 → 方案 → 评审 → 输出 → 预防建议

巡检模式: 自动执行
  按预定义清单执行检查 → 汇总异常 → 输出报告
```

---

## 5. AgentScope 集成代码

### 5.1 AgentWorkflowEngine 实现

```python
from enum import Enum
from typing import Optional
import hashlib


class AgentState(Enum):
    """AGENTS.md 定义的状态机"""
    IDLE = "idle"
    TRIAGE = "triage"
    DIAGNOSE = "diagnose"
    PLAN = "plan"
    REVIEW = "review"
    OUTPUT = "output"
    ASK_INFO = "ask_info"
    BLOCKED = "blocked"
    ESCALATE = "escalate"


class AgentWorkflowEngine:
    """基于 AGENTS.md 的工作流执行引擎"""

    def __init__(self, agents_content: str, max_drift_count: int = 3):
        self.state = AgentState.IDLE
        self.agents_config = agents_content
        self.max_drift_count = max_drift_count
        self.command_history: list[str] = []
        self.phase_token_budget = {
            "collect": 0.30,
            "analyze": 0.25,
            "plan": 0.20,
            "review": 0.15,
            "output": 0.10,
        }

    def transition(self, target: AgentState) -> bool:
        """状态转换，验证合法性"""
        valid_transitions = {
            AgentState.IDLE: [AgentState.TRIAGE],
            AgentState.TRIAGE: [AgentState.DIAGNOSE, AgentState.ASK_INFO],
            AgentState.DIAGNOSE: [AgentState.PLAN, AgentState.ASK_INFO,
                                  AgentState.BLOCKED, AgentState.ESCALATE],
            AgentState.PLAN: [AgentState.REVIEW],
            AgentState.REVIEW: [AgentState.OUTPUT, AgentState.BLOCKED],
            AgentState.OUTPUT: [AgentState.IDLE],
            AgentState.ASK_INFO: [AgentState.TRIAGE, AgentState.DIAGNOSE],
            AgentState.BLOCKED: [AgentState.IDLE],
            AgentState.ESCALATE: [AgentState.IDLE],
        }
        if target in valid_transitions.get(self.state, []):
            self.state = target
            return True
        return False

    def check_drift(self, command: str) -> bool:
        """反漂移检测：连续 N 次相同命令则中断"""
        h = hashlib.md5(command.encode()).hexdigest()
        self.command_history.append(h)

        if len(self.command_history) >= self.max_drift_count:
            recent = self.command_history[-self.max_drift_count:]
            if len(set(recent)) == 1:
                self.transition(AgentState.BLOCKED)
                return True  # 检测到漂移
        return False

    def classify_task(self, user_input: str) -> dict:
        """基于 AGENTS.md 2.1 的任务分类"""
        keywords_map = {
            "pod_scheduling": ["Pending", "调度", "schedule"],
            "pod_crash": ["CrashLoop", "重启", "OOM", "OOMKilled"],
            "node_issue": ["NotReady", "节点异常", "节点不可用"],
            "network": ["Service 不通", "DNS", "网络", "连不上"],
            "storage": ["PVC", "存储", "挂载"],
            "performance": ["慢", "延迟高", "性能"],
        }
        for task_type, keywords in keywords_map.items():
            for kw in keywords:
                if kw.lower() in user_input.lower():
                    return {"type": task_type, "matched": kw}
        return {"type": "unknown", "matched": None}


# === 使用示例 ===
engine = AgentWorkflowEngine(agents_content="...")

# 任务分类
task = engine.classify_task("有个 Pod 一直 Pending")
# {"type": "pod_scheduling", "matched": "Pending"}

# 状态转换
engine.transition(AgentState.TRIAGE)    # IDLE → TRIAGE
engine.transition(AgentState.DIAGNOSE)  # TRIAGE → DIAGNOSE

# 反漂移检测
engine.check_drift("kubectl get pods -n default")  # False
engine.check_drift("kubectl get pods -n default")  # False
engine.check_drift("kubectl get pods -n default")  # True → BLOCKED
```

---

## 6. 问题排除

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 跳过唤醒协议 | 未在 Agent 初始化中强制执行 | 将唤醒序列写入 ReActAgent 的 init 阶段 |
| 任务分类错误 | 关键词匹配不完整 | 扩充关键词列表，增加模糊匹配 |
| 诊断陷入循环 | 反漂移阈值设置过高 | 将 max_drift_count 从 5 降为 3 |
| Phase 1 消耗过多 Token | 信息采集无约束 | 设置每个 Phase 的 Token 预算上限 |
| 异常处理不触发 | 未捕获工具调用异常 | 在每个工具调用外层包装 try-except |
| 多轮后行为漂移 | 长上下文导致 AGENTS.md 指令被稀释 | 定期在中间轮次重新注入关键规则 |

### 6.2 调试检查清单

```
AGENTS.md 配置验证:

□ 唤醒协议：是否包含完整的 4 步序列？
□ 任务分类：关键词列表是否覆盖所有常见问题类型？
□ 工作流：五阶段是否有明确的 Token 预算分配？
□ 异常处理：是否覆盖 信息不足/工具失败/安全拦截 三种异常？
□ 反漂移：是否配置了连续相同操作的中断机制？
□ 质量标准：是否有输出前自检清单？
□ 效率指标：是否定义了平均诊断步骤和 Token 消耗目标？
```

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | AGENTS.md 在 7 文件体系中的定位 |
| [31 - Harness Loop 与执行引擎](./31-agent-harness-loop-execution.md) | FSM 状态机、异步执行引擎、反漂移检测 |
| [openclaw-workspace/AGENTS.md](./openclaw-workspace/AGENTS.md) | K8S 运维诊断工作流完整配置 |
| [44 - SOUL.md 机制解析](./44-openclaw-soul-mechanism.md) | AGENTS.md Phase 4 安全评审引用 SOUL.md |
| [48 - SKILL.md 机制解析](./48-openclaw-skill-mechanism.md) | AGENTS.md Phase 2 参考 SKILL.md 的 SOP |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容，深度解析 OpenClaw AGENTS.md 的设计机制与工程实现。*

---

## Obsidian 相关文档

- 02-ai-agents KUDIG Database — Global MOC
- [[AI基础设施/02-ai-agents/README.md|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[AI基础设施/02-ai-agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[AI基础设施/02-ai-agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[AI基础设施/02-ai-agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[AI基础设施/02-ai-agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[AI基础设施/02-ai-agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[AI基础设施/02-ai-agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[AI基础设施/02-ai-agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[AI基础设施/02-ai-agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[AI基础设施/02-ai-agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[AI基础设施/02-ai-agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## Related

- 13-trusted-agent-system-fiscal-plan

## See Also

- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism
- 47-openclaw-tools-mechanism
- 48-openclaw-skill-mechanism


<!-- risk-assessed -->

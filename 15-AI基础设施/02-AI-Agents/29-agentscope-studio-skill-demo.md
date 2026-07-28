---
title: AgentScope Studio 与 Agent Skill 实战指南 (AI基础设施)
description: 'title: AgentScope Studio 与 Agent Skill 实战指南'
summary: 'title: AgentScope Studio 与 Agent Skill 实战指南'
category: general
tags:
- ai
- ai-agent
- daily-ops
- kubelet
- coredns
- docker
- hpa
- ingress
- networkpolicy
- llm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- AgentScope Studio 与 Agent Skill 实战指南 是什么
- 如何 AgentScope Studio 与 Agent Skill 实战指南
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- Studio
- Agent
- Skill
- 实战指南
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: AgentScope Studio 与 Agent [[SKILL|Skill]] 实战指南
description: '# AgentScope Studio 与 Agent Skill 实战指南'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[kubelet|kubelet]]
- [[coredns|coredns]]
- docker
- hpa
- [[ingress|ingress]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 10min
intent_queries:
- AgentScope Studio 与 Agent Skill 实战指南 是什么
- 如何 AgentScope Studio 与 Agent Skill 实战指南
trigger_keywords:
- AgentScope
- Studio
- Agent
- Skill
- 实战指南
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

# AgentScope Studio 与 Agent Skill 实战指南

> **文档类型**: 实战 Demo 专题 | **最后更新**: 2026-03 | **关键词**: AgentScope Studio, Agent Skill, ReAct Agent, 可视化追踪, K8s 诊断, SKILL.md, Toolkit, 项目管理, Trace, WebUI, 本地开发

---

<!-- chunk: 概述 -->## 概述

本文是 AgentScope 系列的实战补充篇，聚焦两个核心主题：

1. **AgentScope Studio** — 可视化追踪与交互工具：如何连接、使用和理解 Studio 的项目管理与 Trace 能力
2. **Agent Skill** — 领域知识动态加载：如何构建、注册和使用 Anthropic 提出的 Skill 机制提升 Agent 专业能力

通过完整的 Demo 代码和运行示例，帮助读者在 30 分钟内完成：从启动 Studio → 创建 Agent → 注册 Skill → 在 Studio 中观测全链路追踪的完整闭环。

> **前置条件**：
> - 已安装 `agentscope[full]`（参见 [16 - 概述与安装](./16-agentscope-overview-installation.md)）
> - 已启动 AgentScope Studio（参见 [22 - 生产部署](./22-agentscope-production-deployment.md)）
> - 已配置 LLM API Key（DashScope / OpenAI / Ollama 任选其一）

---

<!-- chunk: 1. AgentScope Studio 功能定位 -->## 1. AgentScope Studio 功能定位

## 1.1 Studio 不是什么

一个常见误解是将 Studio 当作"可视化拖拽建 Agent 的平台"。**AgentScope Studio 不是 Agent 构建器**，而是一个**可观测性与交互工具**。

```
Studio 功能边界
│
├── ✅ Studio 能做的
│   ├── Trace 可视化 — LLM 调用详情、工具调用链路、Token 统计
│   ├── 用户输入托管 — 通过 Web 界面与运行中的 Agent 实时对话
│   ├── 项目/Run 管理 — 管理多个 Agent 运行实例
│   ├── 评测界面 — Agent 评测结果可视化与版本对比
│   └── 消息追踪 — 完整消息流回放与 Agent 决策路径分析
│
└── ❌ Studio 不做的
    ├── 不提供拖拽式 Agent 构建
    ├── 不替代 Python 编码（Agent 始终通过代码创建）
    └── 不管理 Agent 的生命周期（由 Runtime 负责）
```

## 1.2 Studio 功能架构

```
AgentScope Studio 架构
│
├── 追踪可视化
│   ├── OpenTelemetry Trace 展示
│   ├── LLM 调用详情（Token、延迟、成本）
│   ├── 工具调用链路
│   └── Agent 决策路径回放
│
├── 项目管理
│   ├── Project — 组织和隔离不同 AI 应用
│   ├── Run — 项目内的单次执行实例（类似 Session）
│   ├── 运行记录管理
│   └── 版本对比
│
├── 用户交互
│   ├── WebSocket 实时用户输入托管
│   ├── 多 UserAgent 协作交互
│   └── Chatbot 风格消息展示
│
└── 评测界面
    ├── 评测结果可视化
    ├── Agent 版本 A/B 对比
    └── 评分分布分析
```

## 1.3 核心概念：Project 与 Run

| 概念 | 说明 | 类比 |
|------|------|------|
| **Project** | 组织和隔离不同 AI 应用或实验 | 类似 Git 仓库 |
| **Run** | 项目内的单次执行实例，追踪完整运行过程 | 类似一次 CI 构建 |
| **Reply** | 可视化层面组织多条消息的单元（一次 `agent.reply()` 调用） | 类似一轮对话 |
| **Message** | 最小消息单元（Msg 对象） | 单条消息 |

```
数据层次
│
├── Project: "K8s-Diagnosis-Agent"
│   ├── Run: "010533_JWCi" (运行中)
│   │   ├── Reply #1: Friday 的问候
│   │   │   └── Msg: "您好！有什么可以帮助您的吗？"
│   │   ├── Reply #2: 用户提问
│   │   │   └── Msg: "pod notready 的排查方案"
│   │   └── Reply #3: Friday 的诊断回复
│   │       ├── Msg: [system prompt 构造]
│   │       ├── Msg: [LLM 调用]
│   │       └── Msg: "当 Pod 处于 NotReady 状态时..."
│   └── Run: "010534_ABC" (已完成)
│
└── Project: "Code-Review-Agent"
    └── ...
```

---

<!-- chunk: 2. 第一个 Agent：创建并连接 Studio -->## 2. 第一个 Agent：创建并连接 Studio

## 2.1 最简示例

创建 `agent-demo.py`：

```python
import agentscope
from agentscope.agent import ReActAgent, UserAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.tool import Toolkit, execute_python_code, execute_shell_command
import os
import asyncio


async def main():
    # 1. 初始化并连接 Studio
    agentscope.init(
        studio_url="http://localhost:3000",  # Studio 地址
    )

    # 2. 准备工具
    toolkit = Toolkit()
    toolkit.register_tool_function(execute_python_code)
    toolkit.register_tool_function(execute_shell_command)

    # 3. 创建 ReAct Agent
    agent = ReActAgent(
        name="Friday",
        sys_prompt="你是一个名叫 Friday 的智能助手。",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        toolkit=toolkit,
    )

    # 4. 创建用户 Agent（接收终端或 Studio 输入）
    user = UserAgent(name="user")

    # 5. 对话循环
    msg = None
    while True:
        msg = await agent(msg)
        msg = await user(msg)
        if msg.get_text_content() == "exit":
            break


asyncio.run(main())
```

## 2.2 运行与验证

```bash
export DASHSCOPE_API_KEY="your-api-key"
python agent-demo.py
```

**预期输出**：

```
2026-03-18 01:05:34,679 | INFO | Connected to AgentScope Studio at "http://localhost:3000"
                          with run name "ewzz8KSdWLcdTn3zGHPfVb".
2026-03-18 01:05:34,679 | INFO | View the run at:
                          http://localhost:3000/projects/UnnamedProject_At20260318
Friday: 您好！有什么可以帮助您的吗？
```

日志中的关键信息：

| 字段 | 含义 |
|------|------|
| `Connected to AgentScope Studio` | 成功连接 Studio |
| `run name "ewzz8KSdWLcdTn3zGHPfVb"` | 本次运行的 Run ID |
| `UnnamedProject_At20260318` | 自动生成的项目名（日期后缀） |

## 2.3 自定义项目名

默认项目名为 `UnnamedProject_At{日期}`。建议通过 `project` 参数显式指定：

```python
agentscope.init(
    studio_url="http://localhost:3000",
    project="k8s-diagnosis-agent",  # 自定义项目名
)
```

## 2.4 使用本地模型（Ollama）

无需 API Key，使用本地 Ollama 模型：

```bash
# 确保 Ollama 已启动并拉取模型
ollama serve
ollama pull qwen2.5:7b
```

```python
from agentscope.model import OllamaChatModel
from agentscope.formatter import OllamaChatFormatter

agent = ReActAgent(
    name="Friday",
    sys_prompt="你是一个智能助手。",
    model=OllamaChatModel(
        model_name="qwen2.5:7b",
        host="http://localhost:11434",
    ),
    memory=InMemoryMemory(),
    formatter=OllamaChatFormatter(),
    toolkit=toolkit,
)
```

---

<!-- chunk: 3. Studio Web 界面功能详解 -->## 3. Studio Web 界面功能详解

## 3.1 界面布局

成功运行 Agent 后，打开 Studio Web 界面：

```
http://localhost:3000/projects/{ProjectName}
```

界面分为三个区域：

```
┌──────────────┬──────────────────────────────────┬───────────────────┐
│              │                                  │                   │
│  左侧边栏    │       中间：对话区域               │  右侧：数据视图    │
│              │                                  │                   │
│  Run 列表    │  Friday:                         │  当前运行实例的     │
│  (按时间排序) │   您好！有什么可以帮助您的吗？      │  统计信息          │
│              │                                  │                   │
│  010533_JWCi │  user:                           │  ┌──────────────┐ │
│  ·运行中     │   pod notready 的排查方案          │  │ 状态：运行中   │ │
│              │                                  │  │ 模型调用：1次  │ │
│              │  Friday:                         │  │ 总Token：465  │ │
│              │   当 Pod 处于 NotReady 状态时...   │  └──────────────┘ │
│              │                                  │                   │
│              │  ┌─────────────────────────┐     │  元数据            │
│              │  │  输入框（Studio 托管输入） │     │  名称/项目/时间戳  │
│              │  └─────────────────────────┘     │                   │
│              │                                  │  模型调用分布       │
│              │                                  │  qwen-max: ████ 1 │
│              │                                  │                   │
└──────────────┴──────────────────────────────────┴───────────────────┘
```

## 3.2 右侧数据视图的三个 Tab

| Tab | 功能 | 关键信息 |
|-----|------|---------|
| **运行** | 当前 Run 的统计概览 | 状态、模型调用次数、总 Token 数、调用分布 |
| **消息** | 所有消息列表（按 replyId 或 msg.id 查看） | 每条消息的 role、content、timestamp |
| **跟踪** | OpenTelemetry Trace 详情 | LLM 调用耗时、Tool 执行耗时、Agent 决策路径 |

## 3.3 Trace 可视化示例

在"跟踪"Tab 中，可以看到类似如下的追踪数据：

```
Trace: k8s-diagnosis-001 (总耗时: 12.3s)
│
├── [0-0.5s] Agent.reply() 开始
├── [0.5-1.2s] Formatter.format() — 格式化 5 条消息
├── [1.2-3.8s] Model.invoke() — qwen-max
│   ├── prompt_tokens: 2,340
│   ├── completion_tokens: 186
│   └── tool_calls: ["execute_shell_command"]
├── [3.8-5.1s] Tool.execute(execute_shell_command) — 1.3s
├── [5.1-7.2s] Model.invoke() — qwen-max（第二轮推理）
│   ├── prompt_tokens: 4,120
│   └── completion_tokens: 523
├── [7.2-10.5s] Memory.add() — 保存对话
└── [10.5-12.3s] Agent.print() — 输出结果
```

## 3.4 用户输入托管机制

Studio 通过 WebSocket 实现 `UserAgent` 的输入托管：

```
用户输入流程
│
├── 1. Agent 发送 requestUserInput 请求到 Studio Server
├── 2. Studio Server 通过 WebSocket 推送到 Web 前端
├── 3. 用户在 Web 界面输入框中输入内容
├── 4. Web 前端通过 WebSocket 发送回 Server
├── 5. Server 通过 WebSocket 转发给 Agent 的 Python 进程
└── 6. Agent 接收用户输入，继续执行
```

> **注意**：当终端和 Studio Web 同时运行时，用户输入以 Studio Web 优先。终端会显示"当前不需要用户进行输入"提示。

---

<!-- chunk: 4. Agent Skill 机制详解 -->## 4. Agent Skill 机制详解

## 4.1 什么是 Agent Skill

Agent Skill 是 [Anthropic 提出](https://docs.anthropic.com/en/docs/agents-and-tools/agent-skill) 的一种提升 Agent 在特定任务上能力的方法。核心思路：**将领域知识打包成目录，Agent 在需要时动态加载 `SKILL.md` 来获取专业指导**。

```
Agent Skill 工作原理
│
├── 开发者准备 Skill 目录
│   ├── SKILL.md（必须）— YAML frontmatter + 详细指导文档
│   └── 其他参考文件（可选）— 脚本、模板、配置文件
│
├── 注册 Skill 到 Toolkit
│   └── toolkit.register_agent_skill("skills/k8s-pod-diagnosis")
│
├── Skill Prompt 自动注入 sys_prompt
│   └── ReActAgent 自动附加所有 Skill 的简要描述
│
└── Agent 自主决策
    ├── 根据用户问题判断是否需要加载 Skill
    ├── 通过 view_text_file 工具读取 SKILL.md 详细内容
    └── 按 SKILL.md 中的流程执行诊断/操作
```

## 4.2 核心 API

| API | 功能 | 说明 |
|-----|------|------|
| `toolkit.register_agent_skill(dir)` | 注册 Skill 目录 | 目录中必须包含 `SKILL.md` |
| `toolkit.remove_agent_skill(name)` | 移除已注册的 Skill | 通过 `SKILL.md` 中定义的 name |
| `toolkit.get_agent_skill_prompt()` | 获取所有 Skill 的提示词 | 自动注入到 ReActAgent 的 sys_prompt |

## 4.3 SKILL.md 规范

每个 Skill 目录必须包含一个 `SKILL.md` 文件，格式要求：

```markdown
---
name: skill-name
description: 一句话描述这个 Skill 的用途
---

# Skill 标题

<!-- chunk: 使用场景 -->## 使用场景
描述何时应该使用这个 Skill...

<!-- chunk: 详细指导 -->## 详细指导
具体的操作步骤、诊断流程、命令示例...

<!-- chunk: 输出格式 -->## 输出格式
期望的输出结构...
```

**YAML frontmatter 字段**：

| 字段 | 必填 | 说明 |
|------|------|------|
| `name` | ✅ | Skill 的唯一标识名称 |
| `description` | ✅ | 简短描述，用于生成 Skill Prompt |

## 4.4 Skill Prompt 注入机制

注册 Skill 后，`ReActAgent` 会自动将 Skill 列表附加到 `sys_prompt` 末尾：

```python
toolkit = Toolkit()
toolkit.register_agent_skill("skills/k8s-pod-diagnosis")

# 查看生成的 Skill Prompt
print(toolkit.get_agent_skill_prompt())
```

**输出**：

```
# Agent Skills
The agent skills are a collection of folds of instructions, scripts,
and resources that you can load dynamically to improve performance
on specialized tasks. Each agent skill has a `SKILL.md` file in its
folder that describes how to use the skill. If you want to use a skill,
you MUST read its `SKILL.md` file carefully.

<!-- chunk: k8s-pod-diagnosis -->## k8s-pod-diagnosis
Kubernetes Pod 故障诊断技能，涵盖 Pending/CrashLoopBackOff/OOMKilled/
ImagePullBackOff 等常见问题的排查流程。
Check "skills/k8s-pod-diagnosis/SKILL.md" for how to use this skill
```

## 4.5 自定义 Skill Prompt 模板

可以通过 `Toolkit` 构造参数自定义 Skill 的提示词模板：

```python
toolkit = Toolkit(
    # 自定义 Skill 总览说明
    agent_skill_instruction=(
        "<system-info>"
        "你拥有一组专业技能，每个技能以目录形式提供，"
        "包含 SKILL.md 描述文件。需要时请先阅读对应的 SKILL.md。"
        "</system-info>\n"
    ),
    # 自定义每个 Skill 的格式模板，必须包含 {name}, {description}, {dir}
    agent_skill_template="- {name}({dir}): {description}",
)
```

---

<!-- chunk: 5. 实战 Demo：K8s Pod 诊断 Skill -->## 5. 实战 Demo：K8s Pod 诊断 Skill

## 5.1 目录结构

```
agent-skill-demo/
├── skills/
│   ├── k8s-pod-diagnosis/
│   │   ├── SKILL.md
│   │   └── diagnosis-checklist.sh    # 可选：辅助诊断脚本
│   ├── k8s-node-diagnosis/
│   │   └── SKILL.md
│   └── k8s-network-diagnosis/
│       └── SKILL.md
└── agent-skill-demo.py
```

## 5.2 创建 Skill 目录

## skills/k8s-pod-diagnosis/SKILL.md

```markdown
---
name: k8s-pod-diagnosis
description: Kubernetes Pod 故障诊断技能，涵盖 Pending/CrashLoopBackOff/OOMKilled/ImagePullBackOff 等常见问题的排查流程。
---

# K8s Pod 诊断技能

<!-- chunk: 使用场景 -->## 使用场景

当用户报告 Pod 异常时（Pending、CrashLoopBackOff、OOMKilled、ImagePullBackOff、Error 等状态），
按以下结构化流程进行诊断。

<!-- chunk: 诊断流程 -->## 诊断流程

## Step 1：确认 Pod 状态

```bash
kubectl get pod <pod-name> -n <namespace> -o wide
kubectl describe pod <pod-name> -n <namespace>
```

## Step 2：根据状态分支诊断

## Pending
- 检查事件：`kubectl get events --field-selector involvedObject.name=<pod-name> -n <namespace>`
- 常见原因：
  - 资源不足（CPU/Memory）→ 检查：`kubectl describe nodes | grep -A 5 "Allocated resources"`
  - NodeSelector / Affinity 不匹配 → 检查 Pod spec 的 nodeSelector/affinity
  - PVC 未绑定 → 检查：`kubectl get pvc -n <namespace>`
  - Taint/Toleration 不匹配 → 检查：`kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints`

## CrashLoopBackOff
- 查看日志：`kubectl logs <pod-name> -n <namespace> --previous`
- 常见原因：
  - 启动命令或参数错误
  - 依赖服务不可达（数据库、外部 API）
  - 配置文件缺失或格式错误
  - 端口冲突
- 检查 readiness/liveness probe 配置：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].livenessProbe}'`

## OOMKilled
- 查看资源限制：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].resources}'`
- 检查实际内存使用：`kubectl top pod <pod-name> -n <namespace>`
- 查看 OOM 事件：`kubectl get events -n <namespace> --field-selector reason=OOMKilling`
- 建议：调整 memory limits 或优化应用内存使用

## ImagePullBackOff
- 检查镜像名称和 tag 是否正确
- 检查 imagePullSecrets：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'`
- 检查网络连通性：能否从节点访问镜像仓库
- 检查镜像仓库认证：`kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d`

## Step 3：收集环境信息（如需进一步分析）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 集群资源总览
kubectl top nodes
kubectl get nodes -o wide

# 命名空间资源配额
kubectl get resourcequota -n <namespace>
kubectl get limitrange -n <namespace>
```
<!-- chunk: 输出格式 -->## 输出格式

诊断结果请严格按以下格式输出：

1. **现象**：Pod 的当前状态与异常表现
2. **根因**：导致问题的根本原因（基于实际数据判断）
3. **修复方案**：具体的修复命令和步骤
4. **验证方法**：修复后如何确认问题已解决
5. **预防建议**：如何避免类似问题再次发生
```
# 🟢 低风险：只读/信息收集，通常无副作用
## skills/k8s-node-diagnosis/SKILL.md

```markdown
---
name: k8s-node-diagnosis
description: Kubernetes 节点故障诊断技能，涵盖 NotReady/MemoryPressure/DiskPressure/PIDPressure/NetworkUnavailable 等状态排查。
---

# K8s 节点诊断技能

<!-- chunk: 使用场景 -->## 使用场景

当用户报告节点异常（NotReady、SchedulingDisabled、MemoryPressure 等）时使用。

<!-- chunk: 诊断流程 -->## 诊断流程

## Step 1：确认节点状态

```bash
kubectl get nodes -o wide
kubectl describe node <node-name>
```
## Step 2：检查节点 Conditions

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node <node-name> -o jsonpath='{.status.conditions}' | python3 -m json.tool
```
重点关注的 Condition：

| Condition | 正常值 | 异常值 | 含义 |
|-----------|--------|--------|------|
| Ready | True | False/Unknown | 节点是否就绪 |
| MemoryPressure | False | True | 内存压力 |
| DiskPressure | False | True | 磁盘压力 |
| PIDPressure | False | True | PID 压力 |
| NetworkUnavailable | False | True | 网络不可用 |

## Step 3：检查 kubelet 和系统状态

```bash
# kubelet 日志
journalctl -u kubelet --since "30 minutes ago" --no-pager | tail -50

# 系统资源
free -h
df -h
top -bn1 | head -20
```

<!-- chunk: 输出格式 -->## 输出格式

同 Pod 诊断：现象 → 根因 → 修复方案 → 验证方法 → 预防建议
```
# 🟢 低风险：只读/信息收集，通常无副作用
## skills/k8s-network-diagnosis/SKILL.md

```markdown
---
name: k8s-network-diagnosis
description: Kubernetes 网络故障诊断技能，涵盖 Service 不通、DNS 解析失败、Pod 间通信异常、Ingress 不可达等场景。
---

# K8s 网络诊断技能

<!-- chunk: 使用场景 -->## 使用场景

当用户报告网络相关问题（Service 无法访问、DNS 解析失败、Pod 间不通等）时使用。

<!-- chunk: 诊断流程 -->## 诊断流程

## Step 1：确认 Service 和 Endpoints

```bash
kubectl get svc <service-name> -n <namespace>
kubectl get endpoints <service-name> -n <namespace>
```
## Step 2：DNS 诊断

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用临时 Pod 测试 DNS
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <service-name>.<namespace>.svc.cluster.local

# 检查 CoreDNS
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
```
## Step 3：连通性测试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Pod 到 Service 连通性
kubectl run net-test --image=busybox:1.36 --rm -it --restart=Never -- wget -qO- --timeout=5 http://<service-name>.<namespace>:port

# 检查 NetworkPolicy
kubectl get networkpolicy -n <namespace>
```
<!-- chunk: 输出格式 -->## 输出格式

同 Pod 诊断：现象 → 根因 → 修复方案 → 验证方法 → 预防建议
```

## 5.3 编写 Agent 脚本

创建 `agent-skill-demo.py`：

```python
import agentscope
from agentscope.agent import ReActAgent, UserAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.tool import (
    Toolkit,
    execute_shell_command,
    view_text_file,
)
import os
import asyncio


async def main():
    # 1. 初始化，连接 Studio
    agentscope.init(
        studio_url="http://localhost:3000",
        project="k8s-skill-demo",
    )

    # 2. 创建 Toolkit 并注册工具
    toolkit = Toolkit()
    toolkit.register_tool_function(execute_shell_command)
    toolkit.register_tool_function(view_text_file)  # Agent 需要此工具读取 SKILL.md

    # 3. 注册 Agent Skills（可注册多个）
    toolkit.register_agent_skill("skills/k8s-pod-diagnosis")
    toolkit.register_agent_skill("skills/k8s-node-diagnosis")
    toolkit.register_agent_skill("skills/k8s-network-diagnosis")

    # 4. 查看生成的 Skill Prompt（调试用）
    print("=== Agent Skill Prompt ===")
    print(toolkit.get_agent_skill_prompt())
    print("==========================\n")

    # 5. 创建 ReAct Agent — Skill Prompt 自动附加到 sys_prompt
    agent = ReActAgent(
        name="K8s-Doctor",
        sys_prompt=(
            "你是一个 Kubernetes 运维诊断专家。\n"
            "诊断原则：\n"
            "1. 先收集信息再下结论\n"
            "2. 给出根因分析 + 修复步骤 + 验证方法\n"
            "3. 对破坏性操作给出风险提示\n"
            "4. 所有结论必须基于工具获取的实际数据\n"
            "5. 优先使用已加载的 Skill 中定义的诊断流程"
        ),
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        toolkit=toolkit,
    )

    # 6. 打印最终的 sys_prompt（验证 Skill 注入）
    print("=== Agent System Prompt ===")
    print(agent.sys_prompt)
    print("===========================\n")

    # 7. 用户 Agent
    user = UserAgent(name="user")

    # 8. 对话循环
    msg = None
    while True:
        msg = await agent(msg)
        msg = await user(msg)
        if msg.get_text_content() == "exit":
            break


asyncio.run(main())
```

## 5.4 运行与测试

**前台运行**（适合开发调试，终端直接交互）：

```bash
export DASHSCOPE_API_KEY="your-api-key"
python agent-skill-demo.py
```

**后台运行**（适合长期运行，通过 Studio Web 界面交互）：

```bash
export DASHSCOPE_API_KEY="your-api-key"

# 方式一：nohup — 简单后台运行，日志写入文件
nohup python agent-skill-demo.py > agent.log 2>&1 &
echo $!  # 记录 PID，后续可用 kill $PID 停止

# 方式二：screen — 可随时重新 attach 查看
screen -S agent-demo
python agent-skill-demo.py
# Ctrl+A D 分离会话
# screen -r agent-demo 重新连接

# 方式三：tmux — 同 screen，更现代的选择
tmux new -s agent-demo
python agent-skill-demo.py
# Ctrl+B D 分离会话
# tmux attach -t agent-demo 重新连接

# 方式四：systemd（生产推荐）— 开机自启 + 自动重启
# 见下方 systemd 配置
```

**systemd 服务单元**（适合生产环境长期运行）：

```ini
# /etc/systemd/system/agentscope-k8s-doctor.service
[Unit]
Description=AgentScope K8s Doctor Agent
After=network.target

[Service]
Type=simple
User=app
WorkingDirectory=/opt/agent-skill-demo
Environment=DASHSCOPE_API_KEY=your-api-key
ExecStart=/usr/bin/python3 agent-skill-demo.py
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用并启动
sudo systemctl daemon-reload
sudo systemctl enable --now agentscope-k8s-doctor

# 查看状态和日志
systemctl status agentscope-k8s-doctor
journalctl -u agentscope-k8s-doctor -f

# 停止
sudo systemctl stop agentscope-k8s-doctor
```
> **提示**：后台运行时，用户输入通过 Studio Web 界面（`http://localhost:3000`）进行，
> 无需终端交互。Agent 的 `UserAgent` 会自动由 Studio 托管输入。

**预期输出**：

```
=== Agent Skill Prompt ===
# Agent Skills
The agent skills are a collection of folds of instructions, scripts,
and resources that you can load dynamically to improve performance
on specialized tasks. Each agent skill has a `SKILL.md` file in its
folder that describes how to use the skill. If you want to use a skill,
you MUST read its `SKILL.md` file carefully.

<!-- chunk: k8s-pod-diagnosis -->## k8s-pod-diagnosis
Kubernetes Pod 故障诊断技能...
Check "skills/k8s-pod-diagnosis/SKILL.md" for how to use this skill

<!-- chunk: k8s-node-diagnosis -->## k8s-node-diagnosis
Kubernetes 节点故障诊断技能...
Check "skills/k8s-node-diagnosis/SKILL.md" for how to use this skill

<!-- chunk: k8s-network-diagnosis -->## k8s-network-diagnosis
Kubernetes 网络故障诊断技能...
Check "skills/k8s-network-diagnosis/SKILL.md" for how to use this skill
==========================

=== Agent System Prompt ===
你是一个 Kubernetes 运维诊断专家。
...（sys_prompt 内容）
...（自动附加的 Skill Prompt）
===========================

K8s-Doctor: 您好！我是 K8s 诊断专家，有什么可以帮您排查的问题吗？
```

## 5.5 测试对话示例

```
user: production 命名空间的 nginx-7d5b8c9f-x2k4j Pod 一直 Pending，帮我诊断
```

Agent 执行流程：

```
# 🟢 低风险：只读/信息收集，通常无副作用
Agent 决策路径
│
├── 1. 识别问题类型：Pod Pending → 匹配 k8s-pod-diagnosis Skill
│
├── 2. 读取 SKILL.md
│   └── [Tool] view_text_file("skills/k8s-pod-diagnosis/SKILL.md")
│
├── 3. 按 SKILL.md 中 Step 1 执行
│   ├── [Tool] execute_shell_command("kubectl get pod nginx-7d5b8c9f-x2k4j -n production -o wide")
│   └── [Tool] execute_shell_command("kubectl describe pod nginx-7d5b8c9f-x2k4j -n production")
│
├── 4. 按 SKILL.md 中 Step 2 — Pending 分支执行
│   ├── [Tool] execute_shell_command("kubectl get events --field-selector involvedObject.name=nginx-7d5b8c9f-x2k4j -n production")
│   └── [Tool] execute_shell_command("kubectl describe nodes | grep -A 5 'Allocated resources'")
│
└── 5. 按 SKILL.md 要求的输出格式返回诊断结果
    ├── 现象：Pod nginx-7d5b8c9f-x2k4j 处于 Pending 状态
    ├── 根因：集群 CPU 资源不足，无可调度节点
    ├── 修复方案：扩容节点 / 调整资源请求 / 清理低优先级 Pod
    ├── 验证方法：kubectl get pod -n production -w
    └── 预防建议：配置 ResourceQuota + HPA
```
---

<!-- chunk: 6. 生产模式：AgentApp + WebUI -->## 6. 生产模式：AgentApp + WebUI

## 6.1 使用 AgentScope Runtime

对于需要独立 Web 聊天界面的场景，使用 `agentscope-runtime` 的 `AgentApp`：

```python
import os
from contextlib import asynccontextmanager

from fastapi import FastAPI
from agentscope.agent import ReActAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.tool import Toolkit, execute_shell_command, view_text_file

from agentscope_runtime.engine import AgentApp
from agentscope_runtime.engine.schemas.agent_schemas import AgentRequest


@asynccontextmanager
async def lifespan(app: FastAPI):
    import agentscope
    agentscope.init(
        studio_url=os.getenv("STUDIO_URL", "http://localhost:3000"),
    )
    print("K8s-Doctor AgentApp 启动完成")
    yield
    print("K8s-Doctor AgentApp 已关闭")


agent_app = AgentApp(
    app_name="K8s-Doctor",
    app_description="K8s 运维诊断智能体（内置 Skill 系统）",
    lifespan=lifespan,
)


@agent_app.query(framework="agentscope")
async def query_func(self, msgs, request: AgentRequest = None, **kwargs):
    # 创建 Toolkit 并注册工具和 Skills
    toolkit = Toolkit()
    toolkit.register_tool_function(execute_shell_command)
    toolkit.register_tool_function(view_text_file)
    toolkit.register_agent_skill("skills/k8s-pod-diagnosis")
    toolkit.register_agent_skill("skills/k8s-node-diagnosis")
    toolkit.register_agent_skill("skills/k8s-network-diagnosis")

    agent = ReActAgent(
        name="K8s-Doctor",
        sys_prompt="你是 K8s 运维诊断专家，优先使用已加载的 Skill 诊断流程。",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.getenv("DASHSCOPE_API_KEY"),
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        toolkit=toolkit,
    )

    from agentscope.pipeline import stream_printing_messages
    async for msg, last in stream_printing_messages(
        agents=[agent],
        coroutine_task=agent(msgs),
    ):
        yield msg, last


# 启动服务 + WebUI
agent_app.run(host="0.0.0.0", port=8090, web_ui=True)
```

## 6.2 访问方式

| 入口 | 地址 | 说明 |
|------|------|------|
| **WebUI 聊天界面** | `http://localhost:5173` | 独立的聊天 UI，直接与 Agent 对话 |
| **Agent API 端点** | `http://localhost:8090/process` | SSE 流式 API，供前端或其他服务调用 |
| **健康检查** | `http://localhost:8090/health` | 服务健康状态 |
| **Studio 追踪** | `http://localhost:3000` | Trace 可视化与项目管理 |

## 6.3 也可使用托管 WebUI

无需本地安装前端，直接使用 AgentScope 官方托管的 WebUI：

```
http://webui.runtime.agentscope.io/
```

打开页面后，将 Agent 端点设置为你的 `process` URL：

```
http://your-server:8090/process
```

---

<!-- chunk: 7. Skill 进阶用法 -->## 7. Skill 进阶用法

## 7.1 Skill 目录中的辅助文件

Skill 目录不限于只放 `SKILL.md`，可以包含任意参考文件：

```
skills/k8s-pod-diagnosis/
├── SKILL.md                      # 必须：技能描述与指导
├── diagnosis-checklist.sh        # 可选：一键诊断脚本
├── common-errors.yaml            # 可选：常见错误代码映射表
└── templates/
    └── incident-report.md        # 可选：事故报告模板
```

Agent 可以通过 `view_text_file` 工具读取这些辅助文件。在 `SKILL.md` 中引导 Agent 使用：

```markdown
<!-- chunk: 辅助资源 -->## 辅助资源

- 一键诊断脚本：运行 `bash skills/k8s-pod-diagnosis/diagnosis-checklist.sh <pod-name> <namespace>`
- 常见错误映射：查看 `skills/k8s-pod-diagnosis/common-errors.yaml`
- 事故报告模板：查看 `skills/k8s-pod-diagnosis/templates/incident-report.md`
```

## 7.2 动态 Skill 管理

运行时可以动态添加或移除 Skill：

```python
# 注册
toolkit.register_agent_skill("skills/k8s-pod-diagnosis")

# 移除
toolkit.remove_agent_skill("k8s-pod-diagnosis")
```

## 7.3 Skill + MCP 工具组合

Skill 系统与 MCP 工具可以无缝组合：

```python
from agentscope.mcp import HttpStatelessClient

async def create_toolkit():
    toolkit = Toolkit()

    # 本地工具
    toolkit.register_tool_function(execute_shell_command)
    toolkit.register_tool_function(view_text_file)

    # MCP 远程工具（如 GitHub MCP Server）
    mcp_client = HttpStatelessClient(
        name="github",
        transport="streamable_http",
        url="https://mcp.github.com/mcp",
    )
    await toolkit.register_mcp_client(mcp_client)

    # Agent Skills（领域知识）
    toolkit.register_agent_skill("skills/k8s-pod-diagnosis")
    toolkit.register_agent_skill("skills/k8s-node-diagnosis")

    return toolkit
```

## 7.4 Skill + CoT 思考扩展

结合 Toolkit 的 JSON Schema 动态扩展，让 Agent 在使用 Skill 时输出推理过程：

```python
from pydantic import BaseModel, Field

class CoTThinking(BaseModel):
    """让 LLM 在调用工具前先输出推理过程"""
    thinking: str = Field(description="工具调用前的推理过程")

toolkit = Toolkit()
toolkit.register_tool_function(execute_shell_command)
toolkit.register_tool_function(view_text_file)
toolkit.register_agent_skill("skills/k8s-pod-diagnosis")

# 注入 CoT 思考字段
toolkit.set_extended_model(CoTThinking)
```

Agent 的工具调用会包含 `thinking` 字段，便于追踪决策过程：

```json
{
  "type": "tool_use",
  "name": "execute_shell_command",
  "input": {
    "thinking": "Pod Pending 问题需要先查看事件来确认调度失败原因...",
    "command": "kubectl get events --field-selector involvedObject.name=nginx-xxx -n production"
  }
}
```

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

## 最佳实践

| 实践 | 说明 |
|------|------|
| **Skill 职责单一** | 每个 Skill 聚焦一类问题（Pod / Node / Network），避免万能 Skill |
| **SKILL.md 结构化** | 使用清晰的 Step 1/2/3 流程，Agent 更容易遵循 |
| **提供具体命令** | SKILL.md 中给出完整的 kubectl 命令示例，减少 Agent 猜测 |
| **定义输出格式** | 在 SKILL.md 中明确要求输出格式（现象→根因→修复→验证） |
| **必须注册文件读取工具** | Agent 需要 `view_text_file` 或 `execute_shell_command` 来读取 SKILL.md |
| **连接 Studio 观测** | 通过 `agentscope.init(studio_url=...)` 启用 Trace，便于调试 Skill 效果 |
| **自定义项目名** | 使用 `project="xxx"` 参数，避免自动生成的 `UnnamedProject_At...` |
| **Skill + Tool 配合** | Skill 提供"知识"（如何做），Tool 提供"能力"（执行命令） |

## 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|---------|
| 将所有知识塞入一个巨大的 Skill | Token 消耗大，Agent 容易迷失 | 按问题域拆分为多个小 Skill |
| SKILL.md 中只有描述没有命令 | Agent 不知道具体该执行什么 | 提供完整可执行的命令示例 |
| 不注册文件读取工具 | Agent 无法读取 SKILL.md 内容 | 必须注册 `view_text_file` |
| 在 sys_prompt 中重复 Skill 内容 | 浪费 Token，且 Skill 失去动态加载优势 | 让 Agent 按需通过工具读取 |
| Skill 目录中缺少 SKILL.md | 注册会失败 | 每个 Skill 目录必须包含 SKILL.md |

---

<!-- chunk: 9. 关联文档 -->## 9. 关联文档

| 文档 | 与本文关系 |
|------|-----------|
| [16 - AgentScope 概述与安装](./16-agentscope-overview-installation.md) | 环境搭建与 Hello World 基础 |
| [17 - 核心概念与基础操作](./17-agentscope-core-concepts.md) | State/Message/Agent 等核心抽象 |
| [18 - 工具系统与 MCP 集成](./18-agentscope-tool-system.md) | Toolkit 注册、MCP 集成、自定义工具 |
| [22 - 生产部署与可观测性](./22-agentscope-production-deployment.md) | Studio 部署、Runtime、OTel Tracing |
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | SOUL.md/SKILL.md 分层设计——Harness 架构中的 Skill 层定位 |
| [43 - OpenClaw File-First 架构集成](./43-openclaw-framework-integration.md) | SKILL.md 与 OpenClaw 完整 7 文件体系的融合方案 |
| [openclaw-workspace/](./openclaw-workspace/) | 完整的 K8S 运维 Agent 工作区，含 SKILL.md 实例 |
| [官方 Agent Skill 教程](https://doc.agentscope.io/tutorial/task_agent_skill.html) | Anthropic Agent Skill 原始文档 |
| [AgentScope Studio GitHub](https://github.com/agentscope-ai/agentscope-studio) | Studio 源码与 API 协议 |
| [AgentScope Runtime WebUI](https://runtime.agentscope.io/zh/webui.html) | Runtime WebUI 使用指南 |

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents MOC
- [[15-AI基础设施/02-AI-Agents/README.md|AI Agent 工程专题]]
- [[15-AI基础设施/02-AI-Agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[15-AI基础设施/02-AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[15-AI基础设施/02-AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[15-AI基础设施/02-AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[15-AI基础设施/02-AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[15-AI基础设施/02-AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[15-AI基础设施/02-AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[15-AI基础设施/02-AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[15-AI基础设施/02-AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[15-AI基础设施/02-AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## Related

- 48-openclaw-skill-mechanism

## See Also

- 27-agent-cli-security-governance
- 28-agent-cli-enterprise-automation
- 30-agent-harness-engineering
- 31-agent-harness-loop-execution


<!-- risk-assessed -->

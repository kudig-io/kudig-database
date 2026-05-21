---
title: AgentScope 多 Agent 编排与工作流
description: 'description: ''**文档类型**: 多 Agent 编排专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, MsgHub,'
category: general
tags:
- ai
- ai-agent
- docker
- redis
- hpa
- vpa
- statefulset
- rbac
- networkpolicy
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- AgentScope 多 Agent 编排与工作流 是什么
- 如何 AgentScope 多 Agent 编排与工作流
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- Agent
- 编排与工作流
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- redis-basics
---

title: AgentScope 多 Agent 编排与工作流
description: '**文档类型**: 多 Agent 编排专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, MsgHub,
  Pipeline, 多 Agent, 消息编排, sequential_pipeline, 并发 Agent, Routing, Handoffs, Plan,
  工作流'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- docker
- redis
- hpa
- vpa
- statefulset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 多 Agent 编排与工作流 是什么
- 如何 AgentScope 多 Agent 编排与工作流
trigger_keywords:
- AgentScope
- Agent
- 编排与工作流
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

# AgentScope 多 Agent 编排与工作流

> **文档类型**: 多 Agent 编排专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, MsgHub, Pipeline, 多 Agent, 消息编排, sequential_pipeline, 并发 Agent, Routing, Handoffs, Plan, 工作流

---

<!-- chunk: 概述 -->## 概述

单 Agent 系统在复杂任务中能力受限。AgentScope 通过 **MsgHub**（消息中心）和 **Pipeline**（管道编排）提供灵活的多 Agent 协作机制，支持顺序、并行、路由、交接等多种工作流模式，同时内置 Plan 模块支持智能体自主规划子任务。

本文系统讲解 AgentScope 的多 Agent 编排能力，从基础的顺序对话到复杂的生产级多 Agent 系统。

---

<!-- chunk: 1. 多 Agent 编排全景 -->## 1. 多 Agent 编排全景

```
AgentScope 多 Agent 编排能力
│
├── MsgHub（消息中心）
│   ├── 发布-订阅消息模式
│   ├── 动态管理参与者（add/delete）
│   ├── 广播消息（broadcast）
│   └── 适合: 多人对话、游戏、社交仿真
│
├── Pipeline（管道编排）
│   ├── sequential_pipeline    → 顺序执行
│   ├── fanout_pipeline        → 并发执行（广播输入）
│   └── stream_printing_messages→ 流式输出展示
│
├── Routing（路由）
│   └── 根据条件将消息路由到不同 Agent
│
├── Handoffs（交接）
│   └── Agent 之间的任务交接
│
└── Plan（计划模块）
    ├── PlanNotebook   → 子任务管理器
    ├── SubTask        → 子任务数据结构
    └── 智能体自主分解和追踪子任务
```

---

<!-- chunk: 2. MsgHub — 消息中心 -->## 2. MsgHub — 消息中心

#<!-- chunk: 2.1 核心概念 -->## 2.1 核心概念

MsgHub 是 AgentScope 多 Agent 通信的核心，采用**发布-订阅**模式：

```
MsgHub 工作原理
│
├── 参与者（participants）
│   每个 Agent 加入 MsgHub 后，自动接收其他 Agent 的消息
│
├── 广播（broadcast）
│   向所有参与者发送消息
│
├── 公告（announcement）
│   MsgHub 创建时的初始消息，所有参与者都会收到
│
└── 动态管理
    ├── hub.add(agent)     → 动态加入新参与者
    └── hub.delete(agent)  → 移除参与者
```

#<!-- chunk: 2.2 基础使用 -->## 2.2 基础使用

```python
from agentscope.pipeline import MsgHub, sequential_pipeline
from agentscope.message import Msg
import asyncio


async def multi_agent_conversation():
    # 创建智能体
    analyst = create_agent("分析师", "你是数据分析专家")
    architect = create_agent("架构师", "你是系统架构专家")
    reviewer = create_agent("审查员", "你是代码审查专家")
    newcomer = create_agent("新成员", "你是新加入的安全专家")

    # 创建消息中心
    async with MsgHub(
        participants=[analyst, architect, reviewer],
        announcement=Msg(
            "Host",
            "请各位专家讨论 K8s 集群性能优化方案",
            "assistant",
        ),
    ) as hub:
        # 顺序发言
        await sequential_pipeline([analyst, architect, reviewer])

        # 动态管理参与者
        hub.add(newcomer)       # 新成员加入
        hub.delete(reviewer)    # 审查员离开

        # 广播消息
        await hub.broadcast(
            Msg("Host", "欢迎安全专家加入，请从安全角度补充建议", "assistant")
        )

        # 新一轮讨论
        await sequential_pipeline([analyst, architect, newcomer])


asyncio.run(multi_agent_conversation())
```

#<!-- chunk: 2.3 MsgHub 消息流 -->## 2.3 MsgHub 消息流

```
时间线 →

Host 公告: "请讨论 K8s 性能优化方案"
    │
    ├──► 分析师 收到公告
    ├──► 架构师 收到公告
    └──► 审查员 收到公告

分析师发言: "建议先分析 metrics-server 数据..."
    │
    ├──► 架构师 收到（自动 observe）
    └──► 审查员 收到（自动 observe）

架构师发言: "建议引入 HPA + VPA 联动..."
    │
    ├──► 分析师 收到
    └──► 审查员 收到

审查员发言: "需要注意 HPA 的 stabilization window..."
    │
    ├──► 分析师 收到
    └──► 架构师 收到

[Hub 操作: 新成员加入, 审查员离开]

Host 广播: "欢迎安全专家加入..."
    │
    ├──► 分析师 收到
    ├──► 架构师 收到
    └──► 新成员 收到（审查员已不在）
```

---

<!-- chunk: 3. Pipeline — 管道编排 -->## 3. Pipeline — 管道编排

#<!-- chunk: 3.1 sequential_pipeline（顺序执行） -->## 3.1 sequential_pipeline（顺序执行）

```python
from agentscope.pipeline import sequential_pipeline

# Agent 按顺序依次执行，每个 Agent 的输出作为下一个的输入
await sequential_pipeline([agent_a, agent_b, agent_c])

# 等价于:
# msg = await agent_a(initial_msg)
# msg = await agent_b(msg)
# msg = await agent_c(msg)
```

**K8s 诊断流水线示例**：

```python
async def k8s_diagnosis_pipeline():
    # 定义专业 Agent
    collector = create_agent(
        "信息收集",
        "你负责收集 K8s 集群状态信息，使用 kubectl 获取 Pod、Node、Event 信息",
    )
    analyzer = create_agent(
        "问题分析",
        "你负责分析收集到的信息，识别问题根因",
    )
    advisor = create_agent(
        "方案建议",
        "你负责根据分析结果给出修复建议，包含风险评估和回滚方案",
    )

    # 流水线执行
    initial = Msg("user", "production 命名空间 Pod Pending 问题", "user")

    # 信息收集 → 问题分析 → 方案建议
    await sequential_pipeline(
        [collector, analyzer, advisor],
        # 可传入初始消息
    )
```

#<!-- chunk: 3.2 fanout_pipeline（并发执行） -->## 3.2 fanout_pipeline（并发执行）

与 `sequential_pipeline` 相对，`fanout_pipeline` 将同一输入广播给多个 Agent 并发执行：

```python
from agentscope.pipeline import fanout_pipeline

# fanout_pipeline: 同一消息广播给多个 Agent，并发执行
results = await fanout_pipeline(
    [cpu_agent, mem_agent, net_agent, disk_agent],
)

# 等价于:
# results = await asyncio.gather(
#     cpu_agent(input_msg),
#     mem_agent(input_msg),
#     net_agent(input_msg),
#     disk_agent(input_msg),
# )
```

> **sequential vs fanout**：
> - `sequential_pipeline`：A → B → C（链式，前者输出作为后者输入）
> - `fanout_pipeline`：A / B / C 并行（广播，同一输入给所有 Agent）

#<!-- chunk: 3.3 stream_printing_messages（流式输出） -->## 3.3 stream_printing_messages（流式输出）

在生产环境中，`stream_printing_messages` 用于流式展示 Agent 的推理和工具调用过程：

```python
from agentscope.pipeline import stream_printing_messages

# 配合 AgentApp 使用，流式返回 Agent 执行过程中的每条消息
async for msg, is_last in stream_printing_messages(
    agents=[agent],
    coroutine_task=agent(user_msg),
):
    # msg: 当前消息（推理、工具调用、最终回复）
    # is_last: 是否为最后一条消息
    print(f"[{'FINAL' if is_last else 'STREAM'}] {msg.get_text_content()[:80]}")
```

> **用途**：主要用于 AgentScope Runtime 的 SSE 流式响应，让用户实时看到 Agent 的执行过程而非等待最终结果。

#<!-- chunk: 3.4 并行 Pipeline（手动 asyncio.gather） -->## 3.4 并行 Pipeline（手动 asyncio.gather）

对于更复杂的并行场景，可直接使用 `asyncio.gather`：

```python
import asyncio


async def parallel_diagnosis():
    """并行诊断多个维度"""

    cpu_agent = create_agent("CPU 分析师", "分析 CPU 使用率和瓶颈")
    mem_agent = create_agent("内存分析师", "分析内存使用和 OOM 风险")
    net_agent = create_agent("网络分析师", "分析网络延迟和连通性")
    disk_agent = create_agent("存储分析师", "分析磁盘 I/O 和容量")

    msg = Msg("user", "请全面诊断集群性能问题", "user")

    # 并行执行四个维度的诊断
    results = await asyncio.gather(
        cpu_agent(msg),
        mem_agent(msg),
        net_agent(msg),
        disk_agent(msg),
    )

    # 汇总结果
    summary_agent = create_agent(
        "汇总专家",
        "综合各维度分析结果，给出整体诊断报告",
    )

    combined = Msg(
        "system",
        f"CPU 分析: {results[0].get_text_content()}\n"
        f"内存分析: {results[1].get_text_content()}\n"
        f"网络分析: {results[2].get_text_content()}\n"
        f"存储分析: {results[3].get_text_content()}",
        "system",
    )

    final_report = await summary_agent(combined)
    return final_report
```

---

<!-- chunk: 4. Routing — 路由 -->## 4. Routing — 路由

#<!-- chunk: 4.1 条件路由 -->## 4.1 条件路由

根据消息内容或条件将任务路由到不同的专业 Agent：

```python
async def routing_example():
    """根据问题类型路由到专业 Agent"""

    # 专业 Agent 池
    agents = {
        "network": create_agent("网络专家", "专精 K8s 网络、CNI、NetworkPolicy"),
        "storage": create_agent("存储专家", "专精 PV/PVC、CSI、存储性能"),
        "compute": create_agent("计算专家", "专精调度、资源管理、HPA"),
        "security": create_agent("安全专家", "专精 RBAC、PSP、网络安全"),
    }

    # 路由器 Agent
    router = create_agent(
        "路由器",
        """你是问题分类路由器。根据用户问题，返回以下类别之一:
        - network: 网络相关问题
        - storage: 存储相关问题
        - compute: 计算/调度相关问题
        - security: 安全相关问题

        只返回类别名称，不要其他内容。""",
    )

    # 用户问题
    user_msg = Msg("user", "Pod 无法访问 ClusterIP Service", "user")

    # 路由决策
    route_result = await router(user_msg)
    category = route_result.get_text_content().strip()

    # 路由到专业 Agent
    if category in agents:
        expert = agents[category]
        response = await expert(user_msg)
        print(f"由 {category} 专家处理: {response.get_text_content()}")
    else:
        print(f"未知类别: {category}，使用默认处理")
```

#<!-- chunk: 4.2 多级路由 -->## 4.2 多级路由

```
多级路由架构
│
├── Level 1: 领域路由
│   ├── K8s 问题 → K8s 路由器
│   ├── Docker 问题 → Docker 路由器
│   └── Linux 问题 → Linux 路由器
│
├── Level 2: 子领域路由
│   K8s 路由器
│   ├── 网络 → 网络专家
│   ├── 存储 → 存储专家
│   └── 安全 → 安全专家
│
└── Level 3: 专业处理
    网络专家
    ├── DNS 问题 → DNS Agent
    ├── CNI 问题 → CNI Agent
    └── Service 问题 → Service Agent
```

---

<!-- chunk: 5. Handoffs — 任务交接 -->## 5. Handoffs — 任务交接

#<!-- chunk: 5.1 Agent 间交接 -->## 5.1 Agent 间交接

当一个 Agent 无法完成任务时，可以将任务交接给更合适的 Agent：

```python
async def handoff_example():
    """Agent 任务交接示例"""

    # 一线诊断 Agent
    l1_agent = create_agent(
        "L1 诊断",
        """你是一线诊断 Agent，负责:
1. 收集基础信息（kubectl get, describe, events）
2. 判断问题复杂度
3. 简单问题直接解决
4. 复杂问题交接给 L2 专家

当你判断问题需要深入分析时，回复 "[HANDOFF:L2] 原因: ..."
""",
    )

    # 二线专家 Agent
    l2_agent = create_agent(
        "L2 专家",
        """你是二线深度诊断专家，处理:
- 复杂的控制平面问题
- 性能瓶颈深度分析
- 跨组件的关联问题
""",
    )

    msg = Msg("user", "API Server 间歇性 504 超时", "user")

    # L1 处理
    l1_response = await l1_agent(msg)

    # 检查是否需要交接
    if "[HANDOFF:L2]" in l1_response.get_text_content():
        print("任务交接到 L2 专家...")
        # 传递上下文给 L2
        context_msg = Msg(
            "system",
            f"[L1 诊断上下文]\n{l1_response.get_text_content()}",
            "system",
        )
        await l2_agent.observe(context_msg)
        l2_response = await l2_agent(msg)
        return l2_response
    else:
        return l1_response
```

---

<!-- chunk: 6. Plan — 计划模块 -->## 6. Plan — 计划模块

#<!-- chunk: 6.1 PlanNotebook 与 SubTask -->## 6.1 PlanNotebook 与 SubTask

Plan 模块的核心是 `PlanNotebook`，它管理一组 `SubTask`：

```python
from agentscope.plan import PlanNotebook, SubTask
from agentscope.agent import ReActAgent

# 方式一：手动创建计划
notebook = PlanNotebook()
notebook.create_plan([
    SubTask(goal="盘点当前命名空间资源"),
    SubTask(goal="评估目标集群资源容量"),
    SubTask(goal="导出资源配置 YAML"),
    SubTask(goal="执行迁移"),
    SubTask(goal="验证并切流"),
])

# 方式二：传给 ReActAgent，让智能体自主创建和管理计划
agent = ReActAgent(
    name="Planner",
    plan_notebook=notebook,
    print_hint_msg=True,   # 打印计划进度提示
    ...
)
```

#<!-- chunk: 6.2 SubTask 状态管理 -->## 6.2 SubTask 状态管理

```python
# 查看子任务
tasks = notebook.view_subtasks()
for task in tasks:
    print(f"  [{task.state}] {task.goal}")

# 更新子任务状态
notebook.update_subtask_state(
    index=0,
    state="completed",
    result="发现 3 个 Deployment、11 个 Pod、1 个 StatefulSet",
)

# 完成计划
notebook.finish_plan()
```

#<!-- chunk: 6.3 计划模块工作流 -->## 6.3 计划模块工作流

```
Plan 模块工作流
│
├── 1. 接收复杂任务
│      "迁移 production 命名空间到新集群"
│
├── 2. 任务分解（Agent 自主或手动）
│      notebook.create_plan([SubTask(goal=...), ...])
│
├── 3. 逐步执行
│      每完成一个 SubTask，调用 update_subtask_state()
│
├── 4. 动态调整
│      如果子任务失败，重新规划剩余步骤
│
└── 5. 完成
       notebook.finish_plan()
```

#<!-- chunk: 6.4 计划可视化 Hooks -->## 6.4 计划可视化 Hooks

PlanNotebook 支持注册可视化钩子，实时展示计划进度：

```python
def on_plan_update(notebook: PlanNotebook):
    """计划更新时的回调"""
    tasks = notebook.view_subtasks()
    for i, task in enumerate(tasks):
        status = "✅" if task.state == "completed" else "⏳" if task.state == "in_progress" else "⬜"
        print(f"  {status} {i+1}. {task.goal}")

# 注册可视化钩子
notebook = PlanNotebook()
# 在 AgentScope Studio 中，PlanNotebook 自动与 Tracing 集成，显示计划进度
```

---

<!-- chunk: 7. 多 Agent 辩论 -->## 7. 多 Agent 辩论

#<!-- chunk: 7.1 辩论模式 -->## 7.1 辩论模式

多个 Agent 从不同角度分析同一问题，通过辩论收敛到最优解：

```python
from agentscope.pipeline import MsgHub, sequential_pipeline
from agentscope.message import Msg
import asyncio


async def multi_agent_debate():
    """多 Agent 辩论: K8s 高可用方案选型"""

    # 三个持不同立场的 Agent
    proponent = create_agent(
        "方案A支持者",
        """你支持 Active-Standby 高可用方案。
论证它的优势: 实现简单、资源消耗低、切换逻辑清晰。
要针对其他方案的缺点进行辩驳。""",
    )

    opponent = create_agent(
        "方案B支持者",
        """你支持 Active-Active 多活高可用方案。
论证它的优势: 无切换延迟、负载均衡、无单点故障。
要针对其他方案的缺点进行辩驳。""",
    )

    judge = create_agent(
        "评判者",
        """你是中立的技术评审。
在两轮辩论后，综合双方论点，给出最终推荐方案。
要明确说明推荐理由和适用条件。""",
    )

    # 辩论流程
    async with MsgHub(
        participants=[proponent, opponent, judge],
        announcement=Msg(
            "Host",
            "议题: K8s 控制平面高可用方案选型（200 节点规模，金融行业）",
            "assistant",
        ),
    ) as hub:
        # 第一轮辩论
        await sequential_pipeline([proponent, opponent])

        # 第二轮辩论（反驳）
        await sequential_pipeline([opponent, proponent])

        # 评判
        final = await judge(
            Msg("Host", "请综合双方论点，给出最终推荐", "assistant")
        )

    return final


asyncio.run(multi_agent_debate())
```

---

<!-- chunk: 8. 并发 Agent（Concurrent Agents） -->## 8. 并发 Agent（Concurrent Agents）

#<!-- chunk: 8.1 asyncio.gather 并发模式 -->## 8.1 asyncio.gather 并发模式

```python
import asyncio


async def concurrent_agents():
    """多个 Agent 并发处理独立任务"""

    agents = [
        create_agent("集群A诊断", "诊断 cluster-us-east-1 的问题"),
        create_agent("集群B诊断", "诊断 cluster-eu-west-1 的问题"),
        create_agent("集群C诊断", "诊断 cluster-ap-south-1 的问题"),
    ]

    msgs = [
        Msg("user", "检查 cluster-us-east-1 健康状态", "user"),
        Msg("user", "检查 cluster-eu-west-1 健康状态", "user"),
        Msg("user", "检查 cluster-ap-south-1 健康状态", "user"),
    ]

    # 三个集群的诊断并发执行
    results = await asyncio.gather(
        *[agent(msg) for agent, msg in zip(agents, msgs)]
    )

    for agent, result in zip(agents, results):
        print(f"{agent.name}: {result.get_text_content()[:100]}...")
```

---

<!-- chunk: 9. 生产级多 Agent 架构设计模式 -->## 9. 生产级多 Agent 架构设计模式

#<!-- chunk: 9.1 Supervisor-Worker 模式 -->## 9.1 Supervisor-Worker 模式

```
生产级 K8s 运维多 Agent 系统
│
├── Supervisor Agent（调度者）
│   ├── 接收用户问题
│   ├── 分解任务
│   ├── 分配给 Worker Agent
│   └── 汇总结果
│
├── Worker Agent 池
│   ├── Pod 诊断 Agent（kubectl get/describe/logs）
│   ├── Node 诊断 Agent（top/describe/ssh）
│   ├── 网络诊断 Agent（network policy/DNS/CNI）
│   ├── 存储诊断 Agent（PV/PVC/CSI）
│   └── 安全审计 Agent（RBAC/PSP/audit log）
│
└── 共享资源
    ├── MsgHub（消息中心）
    ├── Redis Session（状态持久化）
    └── 知识库（RAG 检索）
```

#<!-- chunk: 9.2 实现示例 -->## 9.2 实现示例

```python
async def supervisor_worker_system():
    """Supervisor-Worker 多 Agent 系统"""

    # Worker Agent
    pod_agent = create_agent(
        "Pod 诊断专家",
        "专精 Pod 生命周期、调度、CrashLoopBackOff、OOM 等问题诊断",
    )
    node_agent = create_agent(
        "Node 诊断专家",
        "专精 Node NotReady、资源压力、Taint/Toleration 等问题",
    )
    network_agent = create_agent(
        "网络诊断专家",
        "专精 Service、NetworkPolicy、DNS、CNI 等网络问题",
    )

    # Supervisor Agent
    supervisor = create_agent(
        "调度总监",
        """你是 K8s 运维团队的调度总监。

职责:
1. 分析用户问题，确定需要哪些专家介入
2. 将问题分配给合适的专家
3. 收到专家分析后，汇总为最终报告

你的团队:
- Pod 诊断专家: 处理 Pod 相关问题
- Node 诊断专家: 处理 Node 相关问题
- 网络诊断专家: 处理网络相关问题

根据问题类型，回复要请求哪些专家，格式:
[ASSIGN:pod] [ASSIGN:node] [ASSIGN:network]""",
    )

    workers = {
        "pod": pod_agent,
        "node": node_agent,
        "network": network_agent,
    }

    # 用户问题
    user_msg = Msg(
        "user",
        "Pod 启动后无法访问外部 Service，ping 也不通",
        "user",
    )

    # Supervisor 分析并分配
    assignment = await supervisor(user_msg)
    assignment_text = assignment.get_text_content()

    # 解析分配结果
    assigned_workers = []
    for key in workers:
        if f"[ASSIGN:{key}]" in assignment_text:
            assigned_workers.append(workers[key])

    # 并行执行分配的 Worker
    if assigned_workers:
        results = await asyncio.gather(
            *[worker(user_msg) for worker in assigned_workers]
        )

        # Supervisor 汇总
        summary_msg = Msg(
            "system",
            "专家分析结果:\n" + "\n\n".join(
                f"[{r.name}]: {r.get_text_content()}" for r in results
            ),
            "system",
        )
        await supervisor.observe(summary_msg)
        final = await supervisor(
            Msg("user", "请汇总专家意见，给出最终诊断报告", "user")
        )
        return final


asyncio.run(supervisor_worker_system())
```

---

<!-- chunk: 10. 最佳实践与反模式 -->## 10. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **MsgHub 用于对话式协作**：讨论、辩论、评审等需要多方参与的场景
- **Pipeline 用于工作流**：有明确顺序的任务链（收集→分析→建议）
- **asyncio.gather 用于并行**：独立任务并行执行，显著降低总延迟
- **Supervisor 限制 Worker 数量**：单个 Supervisor 管理 3-5 个 Worker 为佳
- **每个 Agent 职责单一**：专精一个领域比通才 Agent 的诊断准确率更高

#<!-- chunk: 反模式 -->## 反模式

- **过度设计 Agent 层级**：3 层以上的 Agent 层级增加复杂性但边际收益递减
- **所有 Agent 共享全部工具**：每个 Agent 只配备其职责所需的工具
- **忽略 Agent 间的上下文传递**：交接时必须传递足够的上下文
- **无限辩论**：辩论轮次应有上限（2-3 轮），否则发散不收敛
- **MsgHub 参与者过多**：超过 6 个参与者时消息爆炸，每个 Agent 的上下文快速膨胀

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [17 - 核心概念](./[[domain-14-ai-ml-infra/17-agentscope-core-concepts.md|17-agentscope-core-concepts]].md) | Agent 基础与消息系统 |
| [19 - 记忆管理](./[[domain-14-ai-ml-infra/19-agentscope-memory-context.md|19-agentscope-memory-context]].md) | 多 Agent 场景的记忆共享 |
| [21 - 高级特性](./[[domain-14-ai-ml-infra/21-agentscope-advanced-features.md|21-agentscope-advanced-features]].md) | A2A 协议、Hooks、中间件 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 通用多 Agent 架构模式 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-14-ai-ml-infra/topic-ai-agent/MOC.md|topic-ai-agent MOC]]
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

- [[domain-14-ai-ml-infra/18-agentscope-tool-system.md|18-agentscope-tool-system]]
- [[domain-14-ai-ml-infra/19-agentscope-memory-context.md|19-agentscope-memory-context]]
- [[domain-14-ai-ml-infra/21-agentscope-advanced-features.md|21-agentscope-advanced-features]]
- [[domain-14-ai-ml-infra/22-agentscope-production-deployment.md|22-agentscope-production-deployment]]

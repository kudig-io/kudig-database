---
title: Temporal持久化Agent执行
description: '基于Temporal的持久化Agent执行架构：Workflow/Activity模型、检查点恢复、Human-in-the-Loop、K8s部署与LLM框架集成'
summary: '基于Temporal的持久化Agent执行架构：Workflow/Activity模型、检查点恢复、Human-in-the-Loop、K8s部署与LLM框架集成'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- temporal
- durable-execution
- workflow
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- Temporal持久化Agent执行 是什么
- 如何用Temporal构建持久化Agent
- Temporal Workflow Agent最佳实践
trigger_keywords:
- temporal
- durable-execution
- agent-workflow
- checkpoint
prerequisites:
- llm-basics
- kubernetes-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Temporal持久化Agent执行

## 概述

Temporal是一个持久化执行平台，提供Workflow和Activity两级抽象，天然适合构建长时间运行的AI Agent。与传统Agent框架不同，Temporal保证执行过程中的状态持久化、故障恢复和精确一次语义，解决了LLM Agent在生产环境中面临的核心挑战：如何在跨越数小时甚至数天的Agent任务中保持可靠性。

传统Agent执行模型假设进程持续运行，一旦崩溃则丢失全部中间状态。Temporal通过Event Sourcing机制将每个操作记录为不可变事件，任何故障都可以从最后的检查点精确恢复。对于需要多轮LLM调用、外部工具交互和Human-in-the-Loop审批的复杂Agent任务，这种保证至关重要。

## Workflow/Activity模型

### 核心抽象

Temporal的编程模型由两个核心概念组成：

```
Workflow（工作流）:
  - 确定性执行的编排逻辑
  - 定义Activity的调用顺序和控制流
  - 状态由Event History持久化
  - 可以运行数天、数月甚至数年
  - 不允许直接执行副作用操作

Activity（活动）:
  - 执行实际的副作用操作（API调用、数据库读写、LLM推理）
  - 可以重试、超时、取消
  - 幂等性是关键要求
  - 支持 Heartbeat 机制报告长时间运行的进度
```

### Agent as Workflow模式

将Agent建模为Temporal Workflow是构建持久化Agent的核心模式：

```python
from temporalio import workflow
from datetime import timedelta

@workflow.defn
class AgentWorkflow:
    """持久化Agent执行主工作流"""

    def __init__(self):
        self.conversation_history = []
        self.tool_results = []
        self.iteration_count = 0

    @workflow.run
    async def run(self, agent_config: AgentConfig) -> AgentResult:
        max_iterations = agent_config.max_iterations or 20

        while self.iteration_count < max_iterations:
            # LLM推理 - 作为Activity执行，支持重试
            llm_response = await workflow.execute_activity(
                llm_inference,
                args=[agent_config.system_prompt, self.conversation_history],
                start_to_close_timeout=timedelta(seconds=120),
                retry_policy=RetryPolicy(
                    maximum_attempts=3,
                    initial_interval=timedelta(seconds=5),
                    backoff_coefficient=2.0,
                ),
            )

            # 解析LLM响应，判断是否需要工具调用
            if llm_response.has_tool_calls:
                for tool_call in llm_response.tool_calls:
                    # 工具执行 - 可能涉及外部系统
                    tool_result = await workflow.execute_activity(
                        execute_tool,
                        args=[tool_call.name, tool_call.arguments],
                        start_to_close_timeout=timedelta(seconds=300),
                        heartbeat_timeout=timedelta(seconds=30),
                    )
                    self.tool_results.append(tool_result)
                    self.conversation_history.append({
                        "role": "tool",
                        "content": tool_result.output,
                        "tool_call_id": tool_call.id,
                    })
            else:
                # Agent认为任务完成
                return AgentResult(
                    output=llm_response.content,
                    iterations=self.iteration_count,
                    tool_results=self.tool_results,
                )

            self.iteration_count += 1
            self.conversation_history.append({
                "role": "assistant",
                "content": llm_response.content,
            })

        return AgentResult(
            output="达到最大迭代次数限制",
            iterations=self.iteration_count,
            tool_results=self.tool_results,
            status="max_iterations_exceeded",
        )
```

### Activity实现细节

```python
from temporalio import activity

@activity.defn
async def llm_inference(
    system_prompt: str,
    conversation_history: list,
) -> LLMResponse:
    """调用LLM进行推理，支持流式响应和超时控制"""
    client = AsyncOpenAI()

    messages = [{"role": "system", "content": system_prompt}]
    messages.extend(conversation_history)

    # 心跳机制：长时间推理时报告进度
    activity.heartbeat("开始LLM推理")

    response = await client.chat.completions.create(
        model="gpt-4o",
        messages=messages,
        tools=TOOL_DEFINITIONS,
        temperature=0.1,
    )

    activity.heartbeat("LLM推理完成")

    return LLMResponse(
        content=response.choices[0].message.content,
        has_tool_calls=response.choices[0].message.tool_calls is not None,
        tool_calls=response.choices[0].message.tool_calls or [],
        token_usage=response.usage,
    )


@activity.defn
async def execute_tool(
    tool_name: str,
    tool_arguments: dict,
) -> ToolResult:
    """执行Agent工具调用，支持多种工具类型"""
    activity.heartbeat(f"执行工具: {tool_name}")

    tool_map = {
        "search_web": search_web_tool,
        "query_database": query_database_tool,
        "read_file": read_file_tool,
        "execute_code": execute_code_sandbox_tool,
    }

    handler = tool_map.get(tool_name)
    if not handler:
        return ToolResult(
            output=f"未知工具: {tool_name}",
            status="error",
        )

    try:
        result = await handler(**tool_arguments)
        return ToolResult(output=result, status="success")
    except Exception as e:
        return ToolResult(output=str(e), status="error")
```

## 检查点与恢复

### Event Sourcing机制

Temporal通过Event History实现自动检查点。每次Workflow执行的决策（Activity调用、Timer创建、Signal发送）都被记录为不可变事件：

```
Event History示例:

Event 1: WorkflowExecutionStarted
  - 输入: {agent_config: {...}}
  - 任务队列: agent-task-queue

Event 2: ActivityTaskScheduled
  - Activity: llm_inference
  - 参数: [system_prompt, conversation_history]

Event 3: ActivityTaskCompleted
  - 结果: LLMResponse{content: "...", has_tool_calls: true}

Event 4: ActivityTaskScheduled
  - Activity: execute_tool
  - 参数: ["search_web", {"query": "kubernetes best practices"}]

Event 5: ActivityTaskCompleted
  - 结果: ToolResult{output: "...", status: "success"}
```

### 恢复过程

当Worker崩溃时，Temporal自动执行恢复：

```python
# Worker启动 - Temporal自动从Event History恢复Workflow状态
async def main():
    worker = Worker(
        client,
        task_queue="agent-task-queue",
        workflows=[AgentWorkflow],
        activities=[llm_inference, execute_tool],
        # 并发控制
        max_concurrent_activities=10,
        max_concurrent_workflow_tasks=5,
    )
    await worker.run()
```

恢复过程中，Workflow代码从头执行，但所有Activity调用都返回缓存结果（因为Event History已记录完成状态）。这保证了确定性且不重复执行副作用。

### 手动检查点

对于特别长的Agent执行，可以创建子Workflow作为手动检查点：

```python
@workflow.defn
class AgentSubtaskWorkflow:
    """子任务工作流 - 作为检查点边界"""

    @workflow.run
    async def run(self, subtask: SubtaskInput) -> SubtaskResult:
        result = await workflow.execute_activity(
            process_subtask,
            args=[subtask],
            start_to_close_timeout=timedelta(hours=1),
        )
        return result


@workflow.defn
class LongRunningAgentWorkflow:
    """长时间运行Agent - 使用子任务作为检查点"""

    @workflow.run
    async def run(self, task: ComplexTask) -> AgentResult:
        results = []

        for subtask in task.subtasks:
            # 每个子任务是一个独立的子Workflow
            subtask_result = await workflow.execute_child_workflow(
                AgentSubtaskWorkflow.run,
                args=[subtask],
                id=f"{workflow.info().workflow_id}-subtask-{subtask.id}",
            )
            results.append(subtask_result)

        return AgentResult(subtask_results=results)
```

## Human-in-the-Loop信号

### Signal机制

Temporal的Signal允许从外部向运行中的Workflow发送消息，非常适合实现Human-in-the-Loop：

```python
@workflow.defn
class HumanInTheLoopAgentWorkflow:
    """支持人工介入的Agent工作流"""

    def __init__(self):
        self.human_feedback = None
        self.approval_received = False
        self.cancel_requested = False

    @workflow.signal
    async def provide_feedback(self, feedback: HumanFeedback):
        """人工提供反馈或修正"""
        self.human_feedback = feedback

    @workflow.signal
    async def approve(self, approval: ApprovalDecision):
        """人工审批决策"""
        self.approval_received = True
        self.approval_decision = approval

    @workflow.signal
    async def cancel(self):
        """取消Agent执行"""
        self.cancel_requested = True

    @workflow.query
    def get_status(self) -> AgentStatus:
        """查询Agent当前状态"""
        return AgentStatus(
            iteration=self.iteration_count,
            current_step=self.current_step,
            waiting_for_human=self.waiting_for_human,
            conversation_length=len(self.conversation_history),
        )

    @workflow.run
    async def run(self, config: AgentConfig) -> AgentResult:
        while not self.cancel_requested:
            # LLM推理
            llm_response = await workflow.execute_activity(
                llm_inference,
                args=[config.system_prompt, self.conversation_history],
                start_to_close_timeout=timedelta(seconds=120),
            )

            # 检查是否需要人工审批
            if self._needs_human_approval(llm_response):
                self.waiting_for_human = True

                # 等待人工反馈，设置超时
                try:
                    await workflow.wait_condition(
                        lambda: self.approval_received or self.human_feedback,
                        timeout=timedelta(hours=24),
                    )
                except asyncio.TimeoutError:
                    return AgentResult(
                        status="human_approval_timeout",
                        output="等待人工审批超时",
                    )

                self.waiting_for_human = False

                # 处理人工决策
                if self.approval_received:
                    if not self.approval_decision.approved:
                        return AgentResult(
                            status="rejected_by_human",
                            output=self.approval_decision.reason,
                        )
                elif self.human_feedback:
                    # 将人工反馈加入对话历史
                    self.conversation_history.append({
                        "role": "user",
                        "content": self.human_feedback.message,
                    })

            # 继续正常执行...
            self.conversation_history.append({
                "role": "assistant",
                "content": llm_response.content,
            })

        return AgentResult(status="cancelled")
```

### Query机制

Query允许外部系统读取Workflow状态而不影响执行：

```python
# 客户端查询Agent状态
async def monitor_agent(client: Client, workflow_id: str):
    handle = client.get_workflow_handle(workflow_id)

    while True:
        status = await handle.query(HumanInTheLoopAgentWorkflow.get_status)
        print(f"Agent状态: 迭代={status.iteration}, "
              f"当前步骤={status.current_step}, "
              f"等待人工={status.waiting_for_human}")

        if status.waiting_for_human:
            print("Agent正在等待人工审批")
            # 触发告警通知

        await asyncio.sleep(10)
```

## 超时与重试策略

### 多级超时

```python
# Workflow级别超时
@workflow.defn
class BoundedAgentWorkflow:
    @workflow.run
    async def run(self, config: AgentConfig) -> AgentResult:
        # 设置Workflow总执行时间上限
        # 通过start_to_close_timeout或schedule_to_close_timeout控制
        pass

# 启动Workflow时设置超时
await client.execute_workflow(
    BoundedAgentWorkflow.run,
    config,
    id="agent-task-001",
    task_queue="agent-queue",
    execution_timeout=timedelta(hours=2),  # Workflow总超时
    run_timeout=timedelta(hours=1),         # 单次运行超时
    task_timeout=timedelta(seconds=30),     # 单个Decision Task超时
)
```

### Activity重试策略

```python
from temporalio.common import RetryPolicy

# LLM推理重试 - 指数退避
llm_retry_policy = RetryPolicy(
    maximum_attempts=5,
    initial_interval=timedelta(seconds=2),
    backoff_coefficient=2.0,
    maximum_interval=timedelta(seconds=60),
    # 不重试的错误类型
    non_retryable_error_types=[
        "InvalidAPIKeyError",
        "ContentFilterError",
    ],
)

# 工具执行重试 - 更保守的策略
tool_retry_policy = RetryPolicy(
    maximum_attempts=3,
    initial_interval=timedelta(seconds=5),
    backoff_coefficient=1.5,
    maximum_interval=timedelta(seconds=30),
    non_retryable_error_types=[
        "InvalidToolArgumentsError",
        "ToolNotFoundError",
    ],
)
```

### Heartbeat机制

对于长时间运行的Activity，Heartbeat防止被误判为超时：

```python
@activity.defn
async def execute_long_running_tool(
    tool_name: str,
    arguments: dict,
) -> ToolResult:
    """长时间运行的工具执行，定期发送心跳"""
    for step in execute_tool_steps(tool_name, arguments):
        # 检查是否被取消
        if activity.is_cancelled():
            return ToolResult(status="cancelled")

        # 发送心跳，报告进度
        activity.heartbeat(f"处理步骤: {step.name}")

        result = await step.execute()
        if result.should_abort:
            return ToolResult(
                output=result.error,
                status="aborted",
            )

    return ToolResult(output="完成", status="success")
```

## K8s部署Temporal Server

### Helm部署

```yaml
# temporal-values.yaml
server:
  image:
    repository: temporalio/auto-setup
    tag: 1.24.0

  config:
    persistence:
      default:
        driver: "sql"
        sql:
          driver: "postgres"
          host: "temporal-postgresql"
          port: 5432
          database: "temporal"
          user: "temporal"
          existingSecret: "temporal-db-credentials"

      visibility:
        driver: "sql"
        sql:
          driver: "postgres"
          host: "temporal-postgresql"
          port: 5432
          database: "temporal_visibility"
          user: "temporal"
          existingSecret: "temporal-db-credentials"

  resources:
    requests:
      cpu: "500m"
      memory: "512Mi"
    limits:
      cpu: "2"
      memory: "2Gi"

  replicas: 2

web:
  enabled: true
  replicas: 2
  ingress:
    enabled: true
    className: "nginx"
    hosts:
      - temporal.example.com

prometheus:
  enabled: true
  serviceMonitor:
    enabled: true

postgresql:
  enabled: true
  auth:
    existingSecret: "temporal-db-credentials"
  primary:
    persistence:
      size: 50Gi
```

### Agent Worker Deployment

```yaml
# agent-worker-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agent-worker
  namespace: temporal
spec:
  replicas: 3
  selector:
    matchLabels:
      app: agent-worker
  template:
    metadata:
      labels:
        app: agent-worker
    spec:
      serviceAccountName: agent-worker-sa
      containers:
        - name: worker
          image: registry.example.com/agent-worker:latest
          env:
            - name: TEMPORAL_ADDRESS
              value: "temporal-frontend.temporal:7233"
            - name: TEMPORAL_NAMESPACE
              value: "agent-namespace"
            - name: TASK_QUEUE
              value: "agent-task-queue"
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: openai-credentials
                  key: api-key
            - name: MAX_CONCURRENT_ACTIVITIES
              value: "10"
            - name: MAX_CONCURRENT_WORKFLOWS
              value: "5"
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 5
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: agent-worker-hpa
  namespace: temporal
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: agent-worker
  minReplicas: 2
  maxReplicas: 20
  metrics:
    - type: Pods
      pods:
        metric:
          name: temporal_workflow_task_queue_length
        target:
          type: AverageValue
          averageValue: "5"
```

## 与LangGraph/CrewAI集成

### Temporal + LangGraph

```python
from langgraph.graph import StateGraph, END
from temporalio import workflow, activity

@workflow.defn
class LangGraphAgentWorkflow:
    """将LangGraph图执行包装为Temporal Workflow"""

    @workflow.run
    async def run(self, input_data: AgentInput) -> AgentResult:
        # LangGraph的状态图在Temporal中逐步执行
        state = {"messages": [HumanMessage(content=input_data.query)]}

        # 每个节点作为独立Activity执行
        current_node = "agent"

        while current_node != END:
            if current_node == "agent":
                state = await workflow.execute_activity(
                    agent_node_activity,
                    args=[state],
                    start_to_close_timeout=timedelta(seconds=120),
                )
            elif current_node == "tools":
                state = await workflow.execute_activity(
                    tools_node_activity,
                    args=[state],
                    start_to_close_timeout=timedelta(seconds=300),
                )
            elif current_node == "human_review":
                # 等待人工审批
                await workflow.wait_condition(
                    lambda: self.human_approved,
                    timeout=timedelta(hours=24),
                )

            current_node = state.get("next_node", END)

        return AgentResult(
            output=state["messages"][-1].content,
            state=state,
        )
```

### Temporal + CrewAI

```python
@workflow.defn
class CrewAIAgentWorkflow:
    """CrewAI多Agent协作工作流"""

    @workflow.run
    async def run(self, crew_config: CrewConfig) -> CrewResult:
        task_results = []

        for task in crew_config.tasks:
            # 为每个任务选择合适的Agent
            agent_config = crew_config.agents[task.agent_role]

            # 执行Agent任务
            result = await workflow.execute_activity(
                execute_agent_task,
                args=[agent_config, task, task_results],
                start_to_close_timeout=timedelta(minutes=30),
                retry_policy=RetryPolicy(maximum_attempts=2),
            )

            task_results.append({
                "task": task.description,
                "agent": agent_config.role,
                "result": result.output,
            })

            # CrewAI风格的上下文传递
            # 后续任务可以看到前面任务的结果

        return CrewResult(task_results=task_results)
```

## 生产实践要点

### 可观测性集成

```python
# 结合OpenTelemetry追踪
from opentelemetry import trace

@activity.defn
async def traced_llm_inference(
    system_prompt: str,
    messages: list,
) -> LLMResponse:
    tracer = trace.get_tracer("agent-temporal")

    with tracer.start_as_current_span("llm_inference") as span:
        span.set_attribute("model", "gpt-4o")
        span.set_attribute("message_count", len(messages))

        response = await call_llm(system_prompt, messages)

        span.set_attribute("tokens_used", response.token_usage.total_tokens)
        span.set_attribute("finish_reason", response.finish_reason)

        return response
```

### 部署清单

```
Temporal Agent部署检查项:

□ Temporal Server高可用部署（至少2个Frontend、2个History、2个Matching）
□ PostgreSQL主从复制或云托管数据库
□ Agent Worker水平自动伸缩配置
□ Worker资源限制（CPU/Memory）根据LLM并发需求设定
□ Secret管理（API Key通过K8s Secret注入）
□ 网络策略（Worker只能访问必要的外部服务）
□ 监控告警（Workflow失败率、Activity超时率、队列深度）
□ 日志聚合（结构化日志，关联WorkflowID）
□ 灾难恢复计划（Temporal支持跨数据中心复制）
```

---

*Temporal为LLM Agent提供了生产级的持久化执行保证，将Agent从脆弱的内存状态模型升级为可靠的持久化执行模型。*

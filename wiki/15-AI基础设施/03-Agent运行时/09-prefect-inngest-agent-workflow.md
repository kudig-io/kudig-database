---
title: Prefect/Inngest Agent工作流
description: '基于Prefect和Inngest的Agent工作流编排：Flow/Task模型、事件驱动Step Functions、Durable Execution语义与K8s部署'
summary: '基于Prefect和Inngest的Agent工作流编排：Flow/Task模型、事件驱动Step Functions、Durable Execution语义与K8s部署'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- prefect
- inngest
- workflow
- durable-execution
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
- Prefect Agent工作流 是什么
- 如何用Inngest编排Agent执行
- Durable Execution语义详解
trigger_keywords:
- prefect
- inngest
- agent-workflow
- durable-execution
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

# Prefect/Inngest Agent工作流

## 概述

Prefect和Inngest代表了两种不同的Agent工作流编排范式。Prefect提供Python原生的Flow/Task编排模型，适合需要精细控制的数据密集型Agent任务；Inngest基于事件驱动的Step Functions，提供完全托管的Durable Execution语义，适合构建响应式Agent系统。两者都解决了Agent执行中的核心问题：如何在分布式环境中可靠地编排长时间运行的Agent任务。

## Prefect Flow/Task编排Agent执行

### 核心概念

Prefect的编程模型基于Flow和Task两级抽象：

```
Flow（流）:
  - Agent任务的顶层编排单元
  - 管理整体执行流程和状态
  - 支持参数化、调度和重试
  - 可以嵌套调用其他Flow

Task（任务）:
  - Flow中的最小执行单元
  - 自动缓存和重试
  - 支持并发执行
  - 可配置超时和重试策略
```

### Agent Flow实现

```python
from prefect import flow, task
from prefect.tasks import task_input_hash
from datetime import timedelta
from typing import Optional

@task(
    retries=3,
    retry_delay_seconds=10,
    cache_key_fn=task_input_hash,
    cache_expiration=timedelta(hours=1),
    timeout_seconds=120,
)
async def llm_inference(
    system_prompt: str,
    messages: list,
    model: str = "gpt-4o",
) -> dict:
    """LLM推理任务"""
    client = AsyncOpenAI()

    response = await client.chat.completions.create(
        model=model,
        messages=[{"role": "system", "content": system_prompt}] + messages,
        tools=TOOL_DEFINITIONS,
        temperature=0.1,
    )

    return {
        "content": response.choices[0].message.content,
        "tool_calls": [
            {
                "id": tc.id,
                "name": tc.function.name,
                "arguments": json.loads(tc.function.arguments),
            }
            for tc in (response.choices[0].message.tool_calls or [])
        ],
        "usage": {
            "prompt_tokens": response.usage.prompt_tokens,
            "completion_tokens": response.usage.completion_tokens,
        },
    }


@task(retries=2, retry_delay_seconds=5, timeout_seconds=300)
async def execute_tool(
    tool_name: str,
    arguments: dict,
) -> dict:
    """工具执行任务"""
    tools = {
        "web_search": web_search_tool,
        "database_query": database_query_tool,
        "code_execute": code_execute_tool,
    }

    handler = tools.get(tool_name)
    if not handler:
        return {"status": "error", "output": f"Unknown tool: {tool_name}"}

    result = await handler(**arguments)
    return {"status": "success", "output": result}


@task(timeout_seconds=60)
async def evaluate_response(
    response: dict,
    evaluation_criteria: dict,
) -> dict:
    """评估Agent响应质量"""
    eval_prompt = f"""
    评估以下Agent响应的质量:
    响应: {response['content']}
    标准: {json.dumps(evaluation_criteria)}

    返回JSON格式评分 (0-100) 和改进建议。
    """

    eval_result = await llm_inference(
        system_prompt="你是一个评估专家。",
        messages=[{"role": "user", "content": eval_prompt}],
        model="gpt-4o-mini",
    )

    return json.loads(eval_result["content"])


@flow(
    name="agent-execution-flow",
    description="持久化Agent执行流程",
    retries=1,
    retry_delay_seconds=60,
    timeout_seconds=3600,
)
async def agent_flow(
    query: str,
    system_prompt: str,
    max_iterations: int = 20,
    evaluation_threshold: float = 80.0,
) -> dict:
    """Agent执行主流程"""
    conversation_history = [{"role": "user", "content": query}]
    tool_results = []
    total_tokens = {"prompt": 0, "completion": 0}

    for iteration in range(max_iterations):
        # LLM推理
        llm_response = await llm_inference(
            system_prompt=system_prompt,
            messages=conversation_history,
        )

        # 累计Token用量
        total_tokens["prompt"] += llm_response["usage"]["prompt_tokens"]
        total_tokens["completion"] += llm_response["usage"]["completion_tokens"]

        if llm_response["tool_calls"]:
            # 执行工具调用
            for tool_call in llm_response["tool_calls"]:
                result = await execute_tool(
                    tool_name=tool_call["name"],
                    arguments=tool_call["arguments"],
                )
                tool_results.append({
                    "tool": tool_call["name"],
                    "result": result,
                })

                conversation_history.append({
                    "role": "assistant",
                    "content": None,
                    "tool_calls": [tool_call],
                })
                conversation_history.append({
                    "role": "tool",
                    "content": json.dumps(result["output"]),
                    "tool_call_id": tool_call["id"],
                })
        else:
            # Agent生成最终响应
            evaluation = await evaluate_response(
                response=llm_response,
                evaluation_criteria={
                    "accuracy": "响应是否准确回答了问题",
                    "completeness": "响应是否完整",
                    "clarity": "响应是否清晰易懂",
                },
            )

            if evaluation["score"] >= evaluation_threshold:
                return {
                    "output": llm_response["content"],
                    "iterations": iteration + 1,
                    "tool_results": tool_results,
                    "total_tokens": total_tokens,
                    "evaluation": evaluation,
                    "status": "completed",
                }

            # 评分不达标，继续迭代
            conversation_history.append({
                "role": "user",
                "content": f"请改进你的回答。评估反馈: {evaluation['feedback']}",
            })

        conversation_history.append({
            "role": "assistant",
            "content": llm_response["content"],
        })

    return {
        "output": "达到最大迭代次数",
        "iterations": max_iterations,
        "tool_results": tool_results,
        "total_tokens": total_tokens,
        "status": "max_iterations_exceeded",
    }
```

### 并行Agent编排

```python
@flow(name="parallel-agent-flow")
async def parallel_agent_flow(
    queries: list[str],
    system_prompts: list[str],
) -> list[dict]:
    """并行执行多个Agent任务"""
    # Prefect自动并行执行独立的Flow/Task
    futures = []
    for query, prompt in zip(queries, system_prompts):
        future = agent_flow.submit(
            query=query,
            system_prompt=prompt,
        )
        futures.append(future)

    results = []
    for future in futures:
        result = await future.result()
        results.append(result)

    return results


@flow(name="hierarchical-agent-flow")
async def hierarchical_agent_flow(task: str) -> dict:
    """分层Agent编排 - Supervisor + Workers"""
    # Supervisor Agent分析任务并分配
    supervisor_response = await agent_flow(
        query=f"分析以下任务并拆分为子任务: {task}",
        system_prompt=SUPERVISOR_PROMPT,
    )

    subtasks = json.loads(supervisor_response["output"])["subtasks"]

    # Worker Agents并行执行子任务
    worker_results = await parallel_agent_flow(
        queries=[st["description"] for st in subtasks],
        system_prompts=[WORKER_PROMPT] * len(subtasks),
    )

    # Aggregator Agent合并结果
    aggregator_response = await agent_flow(
        query=f"合并以下结果: {json.dumps(worker_results)}",
        system_prompt=AGGREGATOR_PROMPT,
    )

    return {
        "final_output": aggregator_response["output"],
        "subtask_results": worker_results,
        "total_iterations": sum(r["iterations"] for r in worker_results),
    }
```

### Prefect部署配置

```yaml
# prefect-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: prefect-agent-worker
spec:
  replicas: 3
  selector:
    matchLabels:
      app: prefect-agent-worker
  template:
    spec:
      containers:
        - name: worker
          image: registry.example.com/prefect-agent:latest
          command: ["prefect", "worker", "start"]
          args:
            - "--pool"
            - "agent-pool"
            - "--type"
            - "process"
          env:
            - name: PREFECT_API_URL
              value: "https://prefect.example.com/api"
            - name: PREFECT_API_KEY
              valueFrom:
                secretKeyRef:
                  name: prefect-credentials
                  key: api-key
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: openai-credentials
                  key: api-key
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
```

## Inngest Step Functions事件驱动Agent

### Inngest核心概念

Inngest是一个事件驱动的Durable Execution平台，通过Step Functions提供可靠的异步任务编排：

```
Event（事件）:
  - 触发Function执行的消息
  - 包含类型(type)和数据(data)
  - 异步投递，支持批量处理

Function（函数）:
  - 响应事件执行的逻辑
  - 由多个Step组成
  - 自动重试和持久化

Step（步骤）:
  - Function中的最小执行单元
  - 结果自动缓存
  - 支持并行执行
  - 提供sleep/waitUntil能力
```

### Inngest Agent Function

```typescript
import { inngest } from "./client";

// 定义Agent事件
export const agentRequested = inngest.createFunction(
  { id: "agent-execution", name: "Agent Execution" },
  { event: "agent/requested" },
  async ({ event, step }) => {
    const { query, config } = event.data;

    // Step 1: 初始化对话
    const conversation = await step.run("init-conversation", async () => {
      return {
        messages: [{ role: "user", content: query }],
        tool_results: [],
      };
    });

    // Agent主循环
    let iteration = 0;
    const maxIterations = config.maxIterations || 20;

    while (iteration < maxIterations) {
      // Step 2: LLM推理（每步都是持久化的）
      const llmResponse = await step.run(
        `llm-inference-${iteration}`,
        async () => {
          const response = await openai.chat.completions.create({
            model: config.model || "gpt-4o",
            messages: [
              { role: "system", content: config.systemPrompt },
              ...conversation.messages,
            ],
            tools: TOOL_DEFINITIONS,
          });

          return {
            content: response.choices[0].message.content,
            toolCalls: response.choices[0].message.tool_calls || [],
            usage: response.usage,
          };
        }
      );

      if (llmResponse.toolCalls.length > 0) {
        // Step 3: 执行工具调用（支持并行）
        const toolResults = await Promise.all(
          llmResponse.toolCalls.map((toolCall, index) =>
            step.run(`tool-exec-${iteration}-${index}`, async () => {
              return await executeTool(
                toolCall.function.name,
                JSON.parse(toolCall.function.arguments)
              );
            })
          )
        );

        // 更新对话历史
        conversation.messages.push({
          role: "assistant",
          content: llmResponse.content,
          tool_calls: llmResponse.toolCalls,
        });

        toolResults.forEach((result, index) => {
          conversation.messages.push({
            role: "tool",
            content: JSON.stringify(result),
            tool_call_id: llmResponse.toolCalls[index].id,
          });
          conversation.tool_results.push(result);
        });
      } else {
        // Step 4: 等待人工审批（如果需要）
        if (config.requireApproval) {
          await step.waitForEvent("human-approval", {
            event: "agent/approval",
            timeout: "24h",
            match: "data.requestId",
          });
        }

        // Step 5: 返回最终结果
        await step.sendEvent("agent-completed", {
          name: "agent/completed",
          data: {
            requestId: event.data.requestId,
            output: llmResponse.content,
            iterations: iteration + 1,
            toolResults: conversation.tool_results,
          },
        });

        return {
          output: llmResponse.content,
          iterations: iteration + 1,
          toolResults: conversation.tool_results,
        };
      }

      iteration++;

      // Step 6: 迭代间延迟（防止速率限制）
      await step.sleep("iteration-delay", "2s");
    }

    return {
      output: "达到最大迭代次数",
      iterations: maxIterations,
      status: "max_iterations_exceeded",
    };
  }
);

// 多Agent协作Function
export const multiAgentOrchestration = inngest.createFunction(
  { id: "multi-agent", name: "Multi-Agent Orchestration" },
  { event: "agent/multi-agent-requested" },
  async ({ event, step }) => {
    const { task, agents } = event.data;

    // Step 1: Supervisor分析任务
    const plan = await step.run("supervisor-plan", async () => {
      const response = await agentExecute({
        query: `分析任务并制定执行计划: ${task}`,
        config: { systemPrompt: SUPERVISOR_PROMPT },
      });
      return JSON.parse(response.output);
    });

    // Step 2: 并行执行Worker Agents
    const workerResults = await Promise.all(
      plan.subtasks.map((subtask, index) =>
        step.run(`worker-${index}`, async () => {
          return await agentExecute({
            query: subtask.description,
            config: {
              systemPrompt: agents[subtask.agentType].prompt,
              model: agents[subtask.agentType].model,
            },
          });
        })
      )
    );

    // Step 3: Aggregator合并结果
    const finalResult = await step.run("aggregator", async () => {
      return await agentExecute({
        query: `合并以下结果: ${JSON.stringify(workerResults)}`,
        config: { systemPrompt: AGGREGATOR_PROMPT },
      });
    });

    return {
      plan,
      workerResults,
      finalOutput: finalResult.output,
    };
  }
);
```

### Inngest部署配置

```typescript
// inngest/client.ts
import { Inngest } from "inngest";

export const inngest = new Inngest({
  id: "agent-service",
  eventKey: process.env.INNGEST_EVENT_KEY,
  // 生产环境配置
  middleware: [
    // OpenTelemetry集成
    inngestMiddleware({
      name: "otel-middleware",
      init() {
        return {
          onFunctionRun({ ctx }) {
            return {
              beforeExecution() {
                // 创建Span
              },
              afterExecution() {
                // 关闭Span
              },
            };
          },
        };
      },
    }),
  ],
});
```

```yaml
# K8s部署Inngest Agent服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: inngest-agent-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: inngest-agent
  template:
    spec:
      containers:
        - name: agent
          image: registry.example.com/inngest-agent:latest
          ports:
            - containerPort: 3000
          env:
            - name: INNGEST_EVENT_KEY
              valueFrom:
                secretKeyRef:
                  name: inngest-credentials
                  key: event-key
            - name: INNGEST_SIGNING_KEY
              valueFrom:
                secretKeyRef:
                  name: inngest-credentials
                  key: signing-key
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: openai-credentials
                  key: api-key
          resources:
            requests:
              cpu: "250m"
              memory: "512Mi"
            limits:
              cpu: "1"
              memory: "2Gi"
```

## Durable Execution语义

### 核心保证

Durable Execution提供以下关键保证：

```
1. 自动检查点:
   - 每个Step完成后自动持久化结果
   - 崩溃恢复时从最后成功的Step继续
   - 不重复执行已完成的Step

2. 精确一次语义:
   - 每个Step的副作用只执行一次
   - 幂等性由框架保证
   - 开发者无需手动实现重试去重

3. 无限运行时间:
   - Function可以运行数天、数月
   - 支持长时间sleep
   - 不受进程生命周期限制

4. 透明恢复:
   - 开发者无需编写恢复逻辑
   - 框架自动处理状态重建
   - 代码从头执行，Step结果从缓存返回
```

### Prefect vs Inngest对比

```
特性对比:

持久化机制:
  Prefect: 基于数据库的状态跟踪
  Inngest: 基于事件日志的Step缓存

编程模型:
  Prefect: Python装饰器，同步/异步支持
  Inngest: TypeScript/Python，事件驱动

部署模型:
  Prefect: 自托管或Prefect Cloud
  Inngest: 完全托管（Inngest Cloud）

适用场景:
  Prefect: 数据密集型、批处理、传统工作流
  Inngest: 事件驱动、实时响应、Serverless

Agent特性:
  Prefect: 成熟的并发控制和资源管理
  Inngest: 原生的事件等待和Step Functions

监控能力:
  Prefect: 内置UI、指标、日志
  Inngest: 内置UI、实时追踪、调试工具
```

## 与Agent框架集成

### Prefect + LangChain

```python
from prefect import flow, task
from langchain.agents import AgentExecutor, create_openai_tools_agent
from langchain_openai import ChatOpenAI

@task(retries=2)
async def run_langchain_agent(
    query: str,
    agent_config: dict,
) -> dict:
    """在Prefect Task中执行LangChain Agent"""
    llm = ChatOpenAI(
        model=agent_config.get("model", "gpt-4o"),
        temperature=0.1,
    )

    agent = create_openai_tools_agent(
        llm=llm,
        tools=agent_config["tools"],
        prompt=agent_config["prompt"],
    )

    executor = AgentExecutor(
        agent=agent,
        tools=agent_config["tools"],
        max_iterations=agent_config.get("max_iterations", 10),
        verbose=True,
    )

    result = await executor.ainvoke({"input": query})
    return {
        "output": result["output"],
        "intermediate_steps": [
            {
                "action": step[0].tool,
                "input": step[0].tool_input,
                "output": step[1],
            }
            for step in result.get("intermediate_steps", [])
        ],
    }


@flow(name="langchain-agent-flow")
async def langchain_agent_flow(
    query: str,
    config: dict,
) -> dict:
    """LangChain Agent的Prefect Flow包装"""
    result = await run_langchain_agent(
        query=query,
        agent_config=config,
    )
    return result
```

### Inngest + OpenAI Assistants

```typescript
import { inngest } from "./client";

export const openaiAssistantAgent = inngest.createFunction(
  { id: "openai-assistant", name: "OpenAI Assistant Agent" },
  { event: "agent/assistant-requested" },
  async ({ event, step }) => {
    const { assistantId, query } = event.data;

    // Step 1: 创建Thread
    const thread = await step.run("create-thread", async () => {
      return await openai.beta.threads.create({
        messages: [{ role: "user", content: query }],
      });
    });

    // Step 2: 创建Run
    const run = await step.run("create-run", async () => {
      return await openai.beta.threads.runs.create(thread.id, {
        assistant_id: assistantId,
      });
    });

    // Step 3: 轮询Run状态（带超时）
    let runStatus = run;
    while (runStatus.status !== "completed") {
      runStatus = await step.run(
        `poll-run-${runStatus.id}`,
        async () => {
          return await openai.beta.threads.runs.retrieve(
            thread.id,
            run.id
          );
        }
      );

      if (runStatus.status === "requires_action") {
        // 处理工具调用
        const toolCalls =
          runStatus.required_action.submit_tool_outputs.tool_calls;

        const toolOutputs = await Promise.all(
          toolCalls.map((tc, i) =>
            step.run(`tool-${i}`, async () => {
              const result = await executeTool(
                tc.function.name,
                JSON.parse(tc.function.arguments)
              );
              return {
                tool_call_id: tc.id,
                output: JSON.stringify(result),
              };
            })
          )
        );

        await step.run("submit-tool-outputs", async () => {
          return await openai.beta.threads.runs.submitToolOutputs(
            thread.id,
            run.id,
            { outputs: toolOutputs }
          );
        });
      }

      // 等待后再轮询
      await step.sleep("poll-delay", "2s");
    }

    // Step 4: 获取最终消息
    const messages = await step.run("get-messages", async () => {
      return await openai.beta.threads.messages.list(thread.id);
    });

    const assistantMessage = messages.data.find(
      (m) => m.role === "assistant"
    );

    return {
      output: assistantMessage?.content[0]?.text?.value,
      threadId: thread.id,
      runId: run.id,
    };
  }
);
```

## 生产实践要点

### 监控与告警

```python
# Prefect监控配置
from prefect import flow
from prefect.runtime import flow_run

@flow(
    name="monitored-agent-flow",
    on_failure=[send_alert],
    on_completion=[log_metrics],
)
async def monitored_agent_flow(query: str) -> dict:
    """带监控的Agent Flow"""
    # 记录自定义指标
    from prometheus_client import Counter, Histogram

    agent_iterations = Counter(
        "agent_iterations_total",
        "Total agent iterations",
        ["agent_type", "status"],
    )

    agent_duration = Histogram(
        "agent_duration_seconds",
        "Agent execution duration",
        ["agent_type"],
    )

    with agent_duration.labels(agent_type="default").time():
        result = await agent_flow(query=query, system_prompt=DEFAULT_PROMPT)

    agent_iterations.labels(
        agent_type="default",
        status=result["status"],
    ).inc(result["iterations"])

    return result
```

### 部署清单

```
Prefect/Inngest Agent部署检查项:

Prefect:
  □ Prefect Server高可用部署或使用Prefect Cloud
  □ Worker Pool配置（CPU/Memory限制、并发限制）
  □ Work Queue优先级设置（不同Agent类型）
  □ 存储后端配置（S3/GCS用于Flow结果）
  □ Secret管理（通过Prefect Blocks或K8s Secrets）
  □ 监控集成（Prometheus指标、日志聚合）

Inngest:
  □ Inngest Cloud账户配置或自托管部署
  □ Event Key和Signing Key管理
  □ Function并发限制配置
  □ Step超时设置
  □ 错误处理和Dead Letter Queue配置
  □ 监控集成（Inngest Dashboard、Webhook告警）

通用:
  □ LLM API密钥管理
  □ 速率限制和配额管理
  □ 成本追踪和预算告警
  □ 日志聚合和结构化日志
  □ 分布式追踪集成
```

---

*Prefect和Inngest为Agent执行提供了互补的持久化编排能力，选择取决于事件驱动vs批处理的场景需求。*

---
title: LangChain/LangGraph 深度指南
description: 'LangChain 核心架构与 LangGraph 状态图引擎的全面深度解析，涵盖 Chain/Agent/Tool/Memory 四大组件、StateGraph 状态机、持久化检查点、Human-in-the-Loop、Streaming 及 K8s 生产部署'
summary: 'LangChain 核心架构与 LangGraph 状态图引擎的全面深度解析'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- langchain
- langgraph
- state-graph
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
- LangChain/LangGraph 深度指南 是什么
- 如何 LangChain/LangGraph 深度指南
- LangChain 核心架构
- LangGraph StateGraph 状态图
trigger_keywords:
- langchain
- langgraph
- state-graph
- checkpointer
- human-in-the-loop
prerequisites:
- llm-basics
- python-basics
- kubectl-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# LangChain/LangGraph 深度指南

## 1. LangChain 核心架构

### 1.1 整体设计哲学

LangChain 采用分层抽象设计，将 LLM 应用拆解为可组合的标准化组件。核心理念是**链式组合（Chain Composition）**——每个组件只做一件事，通过管道串联构建复杂应用。

```
┌─────────────────────────────────────────────────┐
│                  Application Layer               │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │  Chain   │  │  Agent   │  │  Retrieval   │   │
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘   │
│       │              │               │           │
│  ┌────┴──────────────┴───────────────┴───────┐   │
│  │           LCEL (LangChain Expression)     │   │
│  └────┬──────────┬──────────┬────────────────┘   │
│       │          │          │                    │
│  ┌────┴───┐ ┌────┴───┐ ┌───┴────┐               │
│  │ Model  │ │  Tool  │ │ Memory │               │
│  └────────┘ └────────┘ └────────┘               │
└─────────────────────────────────────────────────┘
```

### 1.2 LCEL — LangChain Expression Language

LCEL 是 LangChain 0.2+ 的核心编排语言，基于 Runnable 协议实现声明式管道：

```python
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser
from langchain_openai import ChatOpenAI

# LCEL 管道：Prompt → Model → Parser
prompt = ChatPromptTemplate.from_messages([
    ("system", "你是一个 Kubernetes 专家。"),
    ("human", "{question}")
])
model = ChatOpenAI(model="gpt-4o", temperature=0)
parser = StrOutputParser()

# 使用 | 运算符串联
chain = prompt | model | parser

# 同步调用
result = chain.invoke({"question": "解释 Pod 的 QoS 等级"})

# 批量调用
results = chain.batch([
    {"question": "什么是 DaemonSet？"},
    {"question": "什么是 StatefulSet？"}
])
```

**Runnable 协议核心方法：**

| 方法 | 用途 | 典型场景 |
|------|------|---------|
| `invoke` | 单次调用 | 同步请求-响应 |
| `batch` | 批量调用 | 并行处理多条输入 |
| `stream` | 流式输出 | 实时 UI 反馈 |
| `ainvoke` | 异步调用 | 高并发服务 |
| `astream` | 异步流式 | 异步实时输出 |

### 1.3 Chain 组件

Chain 是 LangChain 的基础编排单元，LCEL 取代了旧版 `LLMChain`、`SequentialChain` 等遗留类：

```python
from langchain_core.runnables import RunnablePassthrough, RunnableLambda

# 检索增强生成（RAG）管道
def format_docs(docs):
    return "\n\n".join(doc.page_content for doc in docs)

rag_chain = (
    {
        "context": retriever | RunnableLambda(format_docs),
        "question": RunnablePassthrough()
    }
    | prompt
    | model
    | parser
)

# 带回退的链
from langchain_core.runnables import RunnableWithFallbacks

chain_with_fallback = model.with_fallbacks([
    ChatOpenAI(model="gpt-4o-mini"),  # 回退到更小模型
])
```

### 1.4 Agent 架构

Agent 是具有工具调用能力的自主决策单元。LangChain 0.3+ 推荐使用 LangGraph 构建 Agent，但 `create_react_agent` 提供了便捷封装：

```python
from langgraph.prebuilt import create_react_agent
from langchain_core.tools import tool

@tool
def get_pod_status(namespace: str, pod_name: str) -> str:
    """查询指定 Pod 的运行状态。"""
    # 实际实现中调用 kubectl 或 K8s API
    import subprocess
    result = subprocess.run(
        ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "json"],
        capture_output=True, text=True
    )
    return result.stdout

@tool
def get_pod_logs(namespace: str, pod_name: str, tail_lines: int = 100) -> str:
    """获取 Pod 的最近日志。"""
    import subprocess
    result = subprocess.run(
        ["kubectl", "logs", pod_name, "-n", namespace,
         f"--tail={tail_lines}"],
        capture_output=True, text=True
    )
    return result.stdout

# 创建 ReAct Agent
agent = create_react_agent(
    model=ChatOpenAI(model="gpt-4o"),
    tools=[get_pod_status, get_pod_logs],
    state_modifier="你是 KuDig K8s 诊断专家。使用工具查询集群状态。"
)

# 执行
result = agent.invoke({
    "messages": [("user", "检查 default 命名空间下 nginx-pod 的状态")]
})
```

### 1.5 Tool 抽象

工具是 Agent 与外部世界交互的标准化接口：

```python
from langchain_core.tools import StructuredTool
from pydantic import BaseModel, Field

# 使用 Pydantic 定义输入 Schema
class KubectlInput(BaseModel):
    command: str = Field(description="kubectl 子命令，如 get/describe/logs")
    namespace: str = Field(default="default", description="K8s 命名空间")
    resource: str = Field(description="资源类型，如 pod/service/deployment")

def execute_kubectl(command: str, namespace: str, resource: str) -> str:
    """执行 kubectl 命令。"""
    import subprocess
    cmd = ["kubectl", command, resource, "-n", namespace]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if result.returncode != 0:
        return f"命令失败: {result.stderr}"
    return result.stdout

kubectl_tool = StructuredTool.from_function(
    func=execute_kubectl,
    name="kubectl",
    description="执行 kubectl 命令查询 K8s 集群资源",
    args_schema=KubectlInput,
    return_direct=False  # 设为 True 则直接返回给用户
)
```

### 1.6 Memory 系统

Memory 维护跨轮次的对话上下文：

```python
from langchain_community.chat_message_histories import ChatMessageHistory
from langchain_core.chat_history import BaseChatMessageHistory
from langchain_core.runnables.history import RunnableWithMessageHistory

# 内存型存储（生产环境用 Redis/Postgres）
store = {}

def get_session_history(session_id: str) -> BaseChatMessageHistory:
    if session_id not in store:
        store[session_id] = ChatMessageHistory()
    return store[session_id]

# 带历史的对话链
with_history = RunnableWithMessageHistory(
    chain,
    get_session_history,
    input_messages_key="question",
    history_messages_key="history"
)

# 每次调用指定 session_id
config = {"configurable": {"session_id": "user-123"}}
result = with_history.invoke({"question": "刚才查的是什么 Pod？"}, config=config)
```

**生产级 Memory 后端对比：**

| 后端 | 持久化 | 适用场景 | LangChain 模块 |
|------|--------|---------|----------------|
| 内存 | 无 | 开发测试 | `ChatMessageHistory` |
| Redis | 有 | 高并发会话 | `RedisChatMessageHistory` |
| PostgreSQL | 有 | 结构化查询 | `PostgresChatMessageHistory` |
| MongoDB | 有 | 灵活文档存储 | `MongoDBChatMessageHistory` |

---

## 2. LangGraph 状态图引擎

### 2.1 为什么需要 LangGraph

LangChain 的 Chain 是线性管道，无法表达分支、循环、并行等复杂控制流。LangGraph 将 LLM 应用建模为**有限状态机（FSM）**，每个节点是一个处理步骤，边定义状态转移。

```
┌──────────┐     条件边     ┌──────────┐
│  Start   │──────────────→│  LLM     │
└──────────┘               └────┬─────┘
                                │
                    ┌───────────┼───────────┐
                    ↓           ↓           ↓
              ┌─────────┐ ┌─────────┐ ┌─────────┐
              │ Tool A  │ │ Tool B  │ │ Tool C  │
              └────┬────┘ └────┬────┘ └────┬────┘
                   │           │           │
                   └───────────┼───────────┘
                               ↓
                         ┌──────────┐
                         │   End    │
                         └──────────┘
```

### 2.2 StateGraph 基础

```python
from typing import TypedDict, Annotated
from langgraph.graph import StateGraph, END
import operator

# 定义状态 Schema
class DiagnosisState(TypedDict):
    # messages 使用 add 操作符累加
    messages: Annotated[list, operator.add]
    # 诊断阶段
    phase: str
    # 收集的证据
    evidence: Annotated[list, operator.add]
    # 诊断结论
    conclusion: str

# 创建状态图
graph = StateGraph(DiagnosisState)

# 定义节点函数
def collect_info(state: DiagnosisState) -> dict:
    """信息采集节点。"""
    last_msg = state["messages"][-1]
    # 调用 LLM 分析需要哪些信息
    response = llm.invoke([
        SystemMessage(content="分析当前问题，列出需要采集的信息。"),
        *state["messages"]
    ])
    return {
        "messages": [response],
        "phase": "collecting"
    }

def analyze_root_cause(state: DiagnosisState) -> dict:
    """根因分析节点。"""
    evidence_summary = "\n".join(state["evidence"])
    response = llm.invoke([
        SystemMessage(content=f"基于以下证据分析根因：\n{evidence_summary}"),
        *state["messages"]
    ])
    return {
        "messages": [response],
        "phase": "analyzing",
        "conclusion": response.content
    }

def generate_fix(state: DiagnosisState) -> dict:
    """生成修复方案节点。"""
    response = llm.invoke([
        SystemMessage(content=f"根因: {state['conclusion']}。生成修复方案。"),
        *state["messages"]
    ])
    return {"messages": [response], "phase": "fixing"}

# 添加节点
graph.add_node("collect_info", collect_info)
graph.add_node("analyze", analyze_root_cause)
graph.add_node("fix", generate_fix)

# 设置入口
graph.set_entry_point("collect_info")

# 添加边
graph.add_edge("collect_info", "analyze")
graph.add_edge("analyze", "fix")
graph.add_edge("fix", END)

# 编译
diagnosis_app = graph.compile()
```

### 2.3 条件路由

条件边根据状态动态决定下一步：

```python
from langgraph.graph import END

def should_continue(state: DiagnosisState) -> str:
    """条件路由：决定是否需要更多信息。"""
    last_message = state["messages"][-1]

    # 如果 LLM 表示信息不足，继续采集
    if "需要更多信息" in last_message.content:
        return "collect_info"

    # 如果已生成修复方案，进入评审
    if state["phase"] == "fixing":
        return "review"

    # 默认继续分析
    return "analyze"

# 添加条件边
graph.add_conditional_edges(
    "collect_info",        # 源节点
    should_continue,       # 路由函数
    {
        "collect_info": "collect_info",  # 循环采集
        "analyze": "analyze",            # 进入分析
        "review": "review",              # 进入评审
    }
)
```

### 2.4 持久化检查点（Checkpointer）

Checkpointer 实现状态快照，支持断点恢复、时间旅行和 Human-in-the-Loop：

```python
from langgraph.checkpoint.postgres import PostgresSaver
from langgraph.checkpoint.memory import MemorySaver

# 内存型（开发）
checkpointer = MemorySaver()

# PostgreSQL 型（生产）
checkpointer = PostgresSaver.from_conn_string(
    "postgresql://user:pass@postgres:5432/langgraph"
)

# 编译时绑定 Checkpointer
app = graph.compile(checkpointer=checkpointer)

# 执行时指定 thread_id（会话标识）
config = {"configurable": {"thread_id": "diag-session-001"}}
result = app.invoke(
    {"messages": [("user", "Pod nginx-0 一直 CrashLoop")]},
    config=config
)

# 获取检查点快照
snapshot = app.get_state(config)
print(f"当前阶段: {snapshot.values['phase']}")
print(f"消息数: {len(snapshot.values['messages'])}")

# 时间旅行：回溯到某个检查点
history = list(app.get_state_history(config))
for i, state in enumerate(history):
    print(f"  [{i}] step={state.metadata['step']}")

# 恢复到特定检查点
old_config = history[2].config
app.update_state(old_config, {"phase": "collecting"})
```

### 2.5 Human-in-the-Loop 模式

在关键决策点暂停等待人工确认：

```python
from langgraph.graph import StateGraph, END

graph = StateGraph(DiagnosisState)

# 使用 interrupt_before 在节点执行前暂停
app = graph.compile(
    checkpointer=checkpointer,
    interrupt_before=["fix"]  # 在执行修复前暂停
)

# 执行到 fix 节点前会暂停
config = {"configurable": {"thread_id": "diag-002"}}
result = app.invoke(
    {"messages": [("user", "Pod OOMKilled")]},
    config=config
)

# 查看当前状态，等待人工确认
snapshot = app.get_state(config)
print(f"建议的修复方案: {snapshot.values.get('conclusion')}")

# 人工确认后继续执行
user_approved = True
if user_approved:
    app.invoke(None, config=config)  # 从断点继续
else:
    # 人工修改状态后继续
    app.update_state(config, {
        "conclusion": "修改后的方案: 先扩容内存到 512Mi",
    })
    app.invoke(None, config=config)
```

### 2.6 Streaming 支持

LangGraph 提供多种流式输出模式：

```python
# 模式 1: 流式 token（LLM 输出）
for event in app.stream(input_data, config=config, stream_mode="messages"):
    print(event.content, end="", flush=True)

# 模式 2: 流式状态更新（节点级）
for event in app.stream(input_data, config=config, stream_mode="updates"):
    for node, update in event.items():
        print(f"[{node}] 更新: {update}")

# 模式 3: 混合流
for event in app.stream(input_data, config=config, stream_mode=["updates", "messages"]):
    if isinstance(event, tuple):
        mode, data = event
        if mode == "updates":
            print(f"状态更新: {data}")
        elif mode == "messages":
            print(f"Token: {data.content}", end="")

# 模式 4: 自定义事件流
from langchain_core.callbacks import StreamingStdOutCallbackHandler

async for event in app.astream_events(input_data, config=config, version="v2"):
    kind = event["event"]
    if kind == "on_chat_model_stream":
        print(event["data"]["chunk"].content, end="")
    elif kind == "on_tool_start":
        print(f"\n[调用工具] {event['name']}")
    elif kind == "on_tool_end":
        print(f"[工具返回] {event['data'].content[:100]}...")
```

---

## 3. K8s 生产部署

### 3.1 Docker 化

```dockerfile
# Dockerfile
FROM python:3.11-slim

WORKDIR /app

# 系统依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl jq && \
    rm -rf /var/lib/apt/lists/*

# 安装 kubectl
RUN curl -LO "https://dl.k8s.io/release/$(curl -Ls https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    chmod +x kubectl && mv kubectl /usr/local/bin/

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY src/ ./src/

# 非 root 用户
RUN useradd -m agent && chown -R agent:agent /app
USER agent

EXPOSE 8000
CMD ["uvicorn", "src.main:app", "--host", "0.0.0.0", "--port", "8000"]
```

```text
# requirements.txt
langchain==0.3.*
langgraph==0.3.*
langchain-openai==0.3.*
langgraph-checkpoint-postgres==2.0.*
psycopg[binary]==3.2.*
uvicorn[standard]==0.34.*
fastapi==0.115.*
```

### 3.2 Helm Chart

```yaml
# helm/langgraph-agent/values.yaml
replicaCount: 2

image:
  repository: registry.example.com/langgraph-agent
  tag: "1.0.0"
  pullPolicy: IfNotPresent

env:
  - name: OPENAI_API_KEY
    valueFrom:
      secretKeyRef:
        name: llm-secrets
        key: openai-api-key
  - name: POSTGRES_URL
    valueFrom:
      secretKeyRef:
        name: langgraph-db
        key: connection-string

resources:
  requests:
    cpu: "500m"
    memory: "512Mi"
  limits:
    cpu: "2000m"
    memory: "2Gi"

# 专用 ServiceAccount（最小权限）
serviceAccount:
  create: true
  name: langgraph-agent
  annotations:
    # IRSA / Workload Identity 绑定
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789:role/langgraph-agent

# RBAC：只读 Pod/Service/Event
rbac:
  create: true
  rules:
    - apiGroups: [""]
      resources: ["pods", "services", "events", "configmaps"]
      verbs: ["get", "list", "watch"]
    - apiGroups: ["apps"]
      resources: ["deployments", "replicasets"]
      verbs: ["get", "list", "watch"]

# HPA 自动扩缩容
autoscaling:
  enabled: true
  minReplicas: 2
  maxReplicas: 10
  targetCPUUtilizationPercentage: 70
  targetMemoryUtilizationPercentage: 80

# 健康检查
healthCheck:
  liveness:
    path: /healthz
    initialDelaySeconds: 15
    periodSeconds: 10
  readiness:
    path: /readyz
    initialDelaySeconds: 5
    periodSeconds: 5
```

### 3.3 FastAPI 服务封装

```python
# src/main.py
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from langgraph.checkpoint.postgres import PostgresSaver
import uuid

app = FastAPI(title="LangGraph K8s Agent")

# 初始化 Checkpointer
checkpointer = PostgresSaver.from_conn_string(
    os.environ["POSTGRES_URL"]
)

# 编译 Agent
agent = build_diagnosis_agent(checkpointer)

class ChatRequest(BaseModel):
    message: str
    session_id: str | None = None

class ChatResponse(BaseModel):
    reply: str
    session_id: str
    phase: str

@app.post("/chat", response_model=ChatResponse)
async def chat(req: ChatRequest):
    session_id = req.session_id or str(uuid.uuid4())
    config = {"configurable": {"thread_id": session_id}}

    try:
        result = await agent.ainvoke(
            {"messages": [("user", req.message)]},
            config=config
        )
        snapshot = agent.get_state(config)
        return ChatResponse(
            reply=result["messages"][-1].content,
            session_id=session_id,
            phase=snapshot.values.get("phase", "unknown")
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/healthz")
async def healthz():
    return {"status": "ok"}

@app.get("/readyz")
async def readyz():
    # 检查 PostgreSQL 连接
    try:
        checkpointer.conn.execute("SELECT 1")
        return {"status": "ready"}
    except Exception:
        raise HTTPException(status_code=503, detail="DB not ready")
```

---

## 4. 生产最佳实践

### 4.1 错误处理与重试

```python
from langchain_core.runnables import RunnableRetry

# 自动重试
chain_with_retry = model.with_retry(
    retry_if_exception_type=(TimeoutError, ConnectionError),
    wait_exponential_jitter=True,
    stop_after_attempt=3
)

# 带回退的链
from langchain_core.runnables import RunnableWithFallbacks

resilient_chain = chain.with_fallbacks(
    [backup_chain],
    exceptions_to_handle=(RateLimitError,)
)
```

### 4.2 可观测性

```python
# LangSmith 追踪
import os
os.environ["LANGCHAIN_TRACING_V2"] = "true"
os.environ["LANGCHAIN_API_KEY"] = "your-key"
os.environ["LANGCHAIN_PROJECT"] = "kudig-k8s-agent"

# OpenTelemetry 集成
from langchain_community.callbacks.tracers import OpenTelemetryTracer

# 自定义回调
from langchain_core.callbacks import BaseCallbackHandler

class MetricsCallback(BaseCallbackHandler):
    def on_llm_end(self, response, **kwargs):
        tokens = response.llm_output.get("token_usage", {})
        # 推送到 Prometheus
        LLM_TOKENS.labels(model="gpt-4o").inc(tokens.get("total_tokens", 0))

    def on_tool_error(self, error, **kwargs):
        TOOL_ERRORS.labels(tool=kwargs.get("name", "unknown")).inc()
```

### 4.3 安全注意事项

| 风险 | 缓解措施 |
|------|---------|
| Prompt 注入 | 输入校验 + `RunnablePassthrough.with_types` 类型守卫 |
| 工具权限 | RBAC 最小权限 + Tool 输入 Schema 校验 |
| 代码执行 | 沙箱隔离（Docker/Kata）+ 超时控制 |
| API Key 泄露 | K8s Secret + External Secrets Operator |
| 输出泄露 | 输出过滤 + PII 检测中间件 |

### 4.4 性能优化

```python
# 1. 并行工具调用
from langgraph.prebuilt import ToolNode

tool_node = ToolNode(tools, handle_tool_errors=True)

# 2. 嵌入缓存
from langchain_community.cache import RedisCache
import langchain
langchain.llm_cache = RedisCache(redis.Redis(host="redis"))

# 3. 批量嵌入
from langchain_openai import OpenAIEmbeddings
embeddings = OpenAIEmbeddings(model="text-embedding-3-small")
batch_vectors = embeddings.embed_documents(texts, chunk_size=100)

# 4. 流式减少首字延迟
async for chunk in agent.astream(input_data, config=config):
    print(chunk, end="")
```

---

## 5. 总结与选型建议

| 特性 | LangChain | LangGraph |
|------|-----------|-----------|
| 定位 | 通用 LLM 编排框架 | 状态图 Agent 引擎 |
| 控制流 | 线性管道 (LCEL) | 状态机（分支/循环/并行） |
| 状态管理 | 依赖 Memory 组件 | 原生 State + Checkpointer |
| 适用场景 | RAG、简单 Agent、工具链 | 复杂 Agent、多步推理、人机协作 |
| 学习曲线 | 低 | 中 |
| 生产成熟度 | 高（v0.3 稳定） | 中高（快速迭代中） |

**推荐选择路径：**
- 简单 RAG / 工具调用 → LangChain LCEL
- 复杂 Agent / 需要状态持久化 → LangGraph
- 已有 LangChain 项目 → 逐步迁移到 LangGraph

---

## Related

- [[15-AI基础设施/03-Agent运行时/02-llamaindex-data-agent|LlamaIndex 数据 Agent]]
- [[15-AI基础设施/03-Agent运行时/07-agent-framework-selection-guide|Agent 框架选型决策树]]

## See Also

- [[15-AI基础设施/03-Agent运行时/03-crewai-multi-agent-framework|CrewAI 多 Agent 框架]]
- [[15-AI基础设施/03-Agent运行时/06-semantic-kernel-enterprise|Semantic Kernel 企业级 Agent]]


<!-- risk-assessed -->

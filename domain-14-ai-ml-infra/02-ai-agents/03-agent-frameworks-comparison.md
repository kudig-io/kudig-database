---
title: 主流 Agent 框架深度对比 (domain-14-ai-ml-infra)
description: 'title: 主流 Agent 框架深度对比'
category: general
tags:
- ai
- ai-agent
- docker
- redis
- postgresql
- hpa
- ingress
- networkpolicy
- operator
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 主流 Agent 框架深度对比 是什么
- 如何 主流 Agent 框架深度对比
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 主流
- Agent
- 框架深度对比
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- redis-basics
created: "2026-05-23"
---

title: 主流 Agent 框架深度对比
description: '# 主流 Agent 框架深度对比'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- docker
- redis
- postgresql
- hpa
- [[Ingress|ingress]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 主流 Agent 框架深度对比 是什么
- 如何 主流 Agent 框架深度对比
trigger_keywords:
- 主流
- Agent
- 框架深度对比
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

# 主流 Agent 框架深度对比

> **文档类型**: 技术选型指南 | **最后更新**: 2026-03 | **关键词**: LangChain, LlamaIndex, AutoGen, CrewAI, Dify, Semantic Kernel, Agent 框架, 框架选型, 多 Agent 框架

---

<!-- chunk: 概述 -->## 概述

Agent 框架是连接 LLM 与工具调用、记忆管理、多 Agent 协作的工程基础设施。框架选择直接影响开发效率、系统可维护性和扩展能力。本文对主流框架进行深度横向对比，提供选型决策矩阵，并附生产环境配置示例。

---

<!-- chunk: 1. 框架全景概览 -->## 1. 框架全景概览

#<!-- chunk: 1.1 主流框架定位 -->## 1.1 主流框架定位

```
Agent 框架生态
│
├── 通用 Agent 框架（代码优先）
│   ├── LangChain        - 最广泛使用，生态最丰富，学习曲线较陡
│   ├── LlamaIndex       - 以 RAG/知识检索为核心优势
│   ├── AutoGen          - 微软出品，多 Agent 对话编排见长
│   └── Semantic Kernel  - 微软出品，企业级 .NET/Python 集成
│
├── 多 Agent 协作框架
│   ├── CrewAI           - 角色扮演式多 Agent，上手简单
│   ├── LangGraph        - LangChain 生态，图结构状态机编排
│   └── AutoGen Studio   - AutoGen 的可视化编排扩展
│
├── 低代码/无代码平台
│   ├── Dify             - 开源 LLM 应用开发平台，可视化工作流
│   ├── Flowise          - 开源拖拽式 LangChain 可视化工具
│   └── Coze / 扣子      - 字节跳动出品，面向非技术用户
│
└── 垂直领域框架
    ├── OpenAgents       - 面向数据分析的 Agent
    ├── MetaGPT          - 模拟软件开发团队的多 Agent
    └── AgentScope       - 阿里出品，多 Agent 框架
```

---

<!-- chunk: 2. 核心框架深度分析 -->## 2. 核心框架深度分析

#<!-- chunk: 2.1 LangChain -->## 2.1 LangChain

**定位**：最全面的 LLM 应用开发框架，生态最成熟。

**核心组件**：
- `ChatModels`：统一的 LLM 接口抽象（支持 OpenAI/Anthropic/Gemini/本地模型）
- `Chains`：将多个 LLM 调用组合成流水线
- `Agents`：自主决策调用工具的执行引擎
- `Tools`：可供 Agent 调用的功能单元
- `Memory`：对话历史和状态持久化
- `Callbacks`：执行追踪和可观测性钩子

```python
# LangChain Agent 完整示例（K8s 诊断 Agent）
from langchain_openai import ChatOpenAI
from langchain.agents import AgentExecutor, create_openai_functions_agent
from langchain.tools import StructuredTool
from langchain.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain.memory import ConversationBufferWindowMemory
from pydantic import BaseModel

# 1. 定义工具
class KubectlDescribeInput(BaseModel):
    resource_type: str
    name: str
    namespace: str = "default"

def kubectl_describe(resource_type: str, name: str, namespace: str = "default") -> str:
    """执行 kubectl describe 命令"""
    import subprocess
    result = subprocess.run(
        ["kubectl", "describe", resource_type, name, "-n", namespace],
        capture_output=True, text=True, timeout=30
    )
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"

describe_tool = StructuredTool.from_function(
    func=kubectl_describe,
    name="kubectl_describe",
    description="获取 K8s 资源的详细信息和事件。用于诊断 Pod、Node、Service 等资源的状态问题。",
    args_schema=KubectlDescribeInput,
)

# 2. 配置 Agent
llm = ChatOpenAI(model="gpt-4o", temperature=0)

prompt = ChatPromptTemplate.from_messages([
    ("system", """你是一个 Kubernetes 生产运维专家 Agent。
    你的职责是诊断 K8s 集群问题并提供解决方案。
    
    诊断原则：
    1. 先收集足够信息再下结论
    2. 每次工具调用后分析结果，确定是否需要进一步信息
    3. 给出根因分析 + 修复步骤 + 验证方法
    4. 对破坏性操作给出风险提示"""),
    MessagesPlaceholder(variable_name="chat_history"),
    ("human", "{input}"),
    MessagesPlaceholder(variable_name="agent_scratchpad"),
])

tools = [describe_tool]  # 添加更多工具...

memory = ConversationBufferWindowMemory(
    memory_key="chat_history",
    return_messages=True,
    k=10  # 保留最近 10 轮对话
)

agent = create_openai_functions_agent(llm=llm, tools=tools, prompt=prompt)
agent_executor = AgentExecutor(
    agent=agent,
    tools=tools,
    memory=memory,
    verbose=True,
    max_iterations=10,
    handle_parsing_errors=True,
    return_intermediate_steps=True,  # 返回工具调用详情
)

# 3. 执行
result = agent_executor.invoke({
    "input": "Pod nginx-deploy-7d9b4f8c9-xvj2k 一直处于 Pending 状态，请帮我诊断"
})
```

**LangChain 优缺点**：

| 优点 | 缺点 |
|------|------|
| 生态最丰富，集成 300+ 工具 | 抽象层太多，调试困难 |
| 文档详尽，社区活跃 | 版本更新快，Breaking Changes 频繁 |
| LangSmith 可观测性完整 | 学习曲线较陡，过度封装 |
| 支持所有主流 LLM | 某些抽象设计不够直观 |

#<!-- chunk: 2.2 LlamaIndex -->## 2.2 LlamaIndex

**定位**：以数据连接和 RAG 为核心的 LLM 框架，特别擅长知识检索场景。

```python
# LlamaIndex RAG + Agent 示例
from llama_index.core import VectorStoreIndex, SimpleDirectoryReader, Settings
from llama_index.core.agent import ReActAgent
from llama_index.core.tools import QueryEngineTool, ToolMetadata
from llama_index.llms.openai import OpenAI
from llama_index.embeddings.openai import OpenAIEmbedding

# 1. 配置全局设置
Settings.llm = OpenAI(model="gpt-4o", temperature=0)
Settings.embed_model = OpenAIEmbedding(model="text-embedding-3-large")

# 2. 构建知识库索引（基于 kudig-database）
k8s_docs = SimpleDirectoryReader(
    input_dir="./domain-10-troubleshooting-diagnostics",
    required_exts=[".md"],
    recursive=True
).load_data()

k8s_index = VectorStoreIndex.from_documents(
    k8s_docs,
    show_progress=True
)

# 3. 创建查询工具
k8s_query_engine = k8s_index.as_query_engine(
    similarity_top_k=5,
    response_mode="tree_summarize",
)

k8s_tool = QueryEngineTool(
    query_engine=k8s_query_engine,
    metadata=ToolMetadata(
        name="k8s_troubleshooting_kb",
        description="查询 Kubernetes 故障排查知识库。输入问题描述，返回相关诊断步骤和解决方案。"
    ),
)

# 4. 创建 ReAct Agent
agent = ReActAgent.from_tools(
    tools=[k8s_tool],
    llm=Settings.llm,
    verbose=True,
    max_iterations=15,
    context="""你是一个基于 kudig-database 知识库的 K8s 运维专家 Agent。
    遇到问题时，先查询知识库获取相关知识，再结合具体场景给出建议。"""
)

response = agent.chat("Pod CrashLoopBackOff 的常见原因和排查步骤是什么？")
```

**LlamaIndex 优缺点**：

| 优点 | 缺点 |
|------|------|
| RAG 能力最强，分块/检索策略丰富 | Agent 能力相对 LangChain 较弱 |
| 数据连接器完善（S3/数据库/API） | 生态不如 LangChain 广 |
| 结构化数据查询（NL2SQL）出色 | 多 Agent 协作支持有限 |
| 轻量级，性能好 | 工具调用不如 LangChain 灵活 |

#<!-- chunk: 2.3 AutoGen -->## 2.3 AutoGen

**定位**：微软出品，专注于多 Agent 对话与协作，支持 Group Chat 编排。

```python
# AutoGen 多 Agent 协作示例（代码审查场景）
import autogen
from autogen import AssistantAgent, UserProxyAgent, GroupChat, GroupChatManager

# 1. 配置 LLM
llm_config = {
    "model": "gpt-4o",
    "api_key": "your-key",
    "temperature": 0,
    "timeout": 120,
}

# 2. 定义专业 Agent
planner = AssistantAgent(
    name="Planner",
    system_message="""你是运维规划专家。当收到问题报告时，制定诊断和修复计划。
    将复杂问题分解为有序的诊断步骤，并分配给合适的专家。""",
    llm_config=llm_config,
)

k8s_expert = AssistantAgent(
    name="K8s_Expert",
    system_message="""你是 Kubernetes 专家。执行 K8s 诊断命令并分析结果。
    专长：Pod 调度、网络策略、存储、控制平面组件。""",
    llm_config=llm_config,
)

sre_reviewer = AssistantAgent(
    name="SRE_Reviewer",
    system_message="""你是 SRE 审查员。审核修复方案的安全性和风险。
    确保操作不会造成业务中断，并提供回滚方案。""",
    llm_config=llm_config,
)

# 3. 用户代理（可执行代码/命令）
user_proxy = UserProxyAgent(
    name="User",
    human_input_mode="NEVER",  # 完全自动化
    code_execution_config={
        "work_dir": "/tmp/agent_workspace",
        "use_docker": True,  # 在 Docker 容器中安全执行
    },
    max_consecutive_auto_reply=5,
)

# 4. 创建 Group Chat
group_chat = GroupChat(
    agents=[planner, k8s_expert, sre_reviewer, user_proxy],
    messages=[],
    max_round=20,
    speaker_selection_method="auto",  # 自动选择下一个发言者
)

manager = GroupChatManager(groupchat=group_chat, llm_config=llm_config)

# 5. 启动协作
user_proxy.initiate_chat(
    manager,
    message="生产集群 prod-us-east-1 的 API Server 响应时间突然从 50ms 升至 2000ms，需要紧急诊断"
)
```

**AutoGen 优缺点**：

| 优点 | 缺点 |
|------|------|
| 多 Agent 协作设计直观 | 单 Agent 能力不如 LangChain 丰富 |
| 支持代码执行（安全沙箱） | 生态工具集成相对少 |
| Group Chat 编排灵活 | 对话管理有时难以控制 |
| 微软生态集成好（Azure OpenAI） | 文档质量参差不齐 |

#<!-- chunk: 2.4 CrewAI -->## 2.4 CrewAI

**定位**：角色扮演式多 Agent 框架，上手最简单，适合快速原型。

```python
# CrewAI 角色扮演示例（K8s 迁移评估团队）
from crewai import Agent, Task, Crew, Process
from crewai.tools import BaseTool
from langchain_openai import ChatOpenAI

llm = ChatOpenAI(model="gpt-4o", temperature=0)

# 1. 定义专业角色
assessment_expert = Agent(
    role="迁移评估专家",
    goal="全面评估 K8s 集群迁移的可行性和风险",
    backstory="""你有 10 年 Kubernetes 迁移经验，曾主导过数十个大规模集群迁移项目。
    你擅长识别潜在风险并制定缓解措施。""",
    verbose=True,
    allow_delegation=True,
    llm=llm,
    tools=[],  # 添加评估工具
)

network_expert = Agent(
    role="网络架构专家",
    goal="分析网络拓扑并设计迁移后的网络方案",
    backstory="深度熟悉 CNI 插件、Service Mesh 和 K8s 网络策略。",
    verbose=True,
    llm=llm,
)

cost_analyst = Agent(
    role="成本分析师",
    goal="提供迁移 TCO 分析和成本优化建议",
    backstory="专注于云原生成本优化，熟悉各云厂商定价模型。",
    verbose=True,
    llm=llm,
)

# 2. 定义任务
assess_task = Task(
    description="""评估客户现有 K8s 集群（自建 v1.27，200 节点）迁移到 ACK 的可行性。
    输出：风险评估报告 + 迁移优先级清单""",
    agent=assessment_expert,
    expected_output="包含风险等级（High/Medium/Low）的详细评估报告"
)

network_task = Task(
    description="设计迁移后的网络架构，包括 Ingress 策略、NetworkPolicy 和 Service Mesh 方案",
    agent=network_expert,
    expected_output="网络架构设计文档（含 Mermaid 图）"
)

cost_task = Task(
    description="计算迁移 TCO，对比自建 vs ACK 3年总成本",
    agent=cost_analyst,
    expected_output="3年 TCO 对比表（含人力成本、基础设施成本、运维成本）"
)

# 3. 创建 Crew
migration_crew = Crew(
    agents=[assessment_expert, network_expert, cost_analyst],
    tasks=[assess_task, network_task, cost_task],
    process=Process.sequential,  # 或 Process.hierarchical
    verbose=True,
)

result = migration_crew.kickoff()
```

**CrewAI 优缺点**：

| 优点 | 缺点 |
|------|------|
| 上手最简单，代码最少 | 复杂流程控制能力有限 |
| 角色扮演模型直观 | 底层基于 LangChain，有依赖开销 |
| 社区增长快，示例丰富 | 工具生态不如 LangChain 成熟 |
| 支持层级和顺序两种流程 | 缺乏细粒度的状态管理 |

#<!-- chunk: 2.5 Dify -->## 2.5 Dify

**定位**：开源 LLM 应用开发平台，提供可视化工作流 + API + 代码双模式，适合快速生产部署。

```yaml
# Dify 部署（Docker Compose）
version: "3"
services:
  api:
    image: langgenius/dify-api:0.15.0
    environment:
      - SECRET_KEY=your-secret-key
      - OPENAI_API_KEY=your-openai-key
      - DATABASE_URL=postgresql://dify:dify@db:5432/dify
      - REDIS_URL=redis://redis:6379/0
      - VECTOR_STORE=weaviate
      - WEAVIATE_ENDPOINT=http://weaviate:8080
    ports:
      - "5001:5001"
  
  web:
    image: langgenius/dify-web:0.15.0
    ports:
      - "3000:3000"
  
  db:
    image: postgres:15-alpine
    environment:
      - POSTGRES_DB=dify
      - POSTGRES_USER=dify
      - POSTGRES_PASSWORD=dify
  
  weaviate:
    image: semitechnologies/weaviate:1.21.2
    ports:
      - "8080:8080"
```

**Dify 工作流 API 集成**：

```python
import requests

# 调用 Dify Agent 工作流
response = requests.post(
    "http://your-dify-instance/v1/workflows/run",
    headers={
        "Authorization": "Bearer your-workflow-api-key",
        "Content-Type": "application/json"
    },
    json={
        "inputs": {
            "cluster_name": "prod-us-east-1",
            "problem_description": "API Server 响应延迟异常"
        },
        "response_mode": "streaming",
        "user": "ops-engineer-001"
    },
    stream=True
)

for line in response.iter_lines():
    if line:
        print(line.decode('utf-8'))
```

---

<!-- chunk: 3. 框架选型对比矩阵 -->## 3. 框架选型对比矩阵

#<!-- chunk: 3.1 核心能力对比 -->## 3.1 核心能力对比

| 特性 | LangChain | LlamaIndex | AutoGen | CrewAI | Dify |
|------|----------|-----------|---------|-------|------|
| **工具集成数量** | ★★★★★ (300+) | ★★★☆☆ (50+) | ★★★☆☆ | ★★★☆☆ | ★★★★☆ |
| **RAG/知识检索** | ★★★★☆ | ★★★★★ | ★★★☆☆ | ★★★☆☆ | ★★★★☆ |
| **多 Agent 协作** | ★★★★☆(LangGraph) | ★★★☆☆ | ★★★★★ | ★★★★☆ | ★★★☆☆ |
| **可观测性** | ★★★★★(LangSmith) | ★★★★☆ | ★★★☆☆ | ★★★☆☆ | ★★★★☆(内置) |
| **低代码/可视化** | ★★☆☆☆ | ★★☆☆☆ | ★★★☆☆ | ★★☆☆☆ | ★★★★★ |
| **上手难度** | 较难 | 中等 | 中等 | 简单 | 简单 |
| **生产成熟度** | ★★★★★ | ★★★★☆ | ★★★★☆ | ★★★☆☆ | ★★★★☆ |
| **社区活跃度** | ★★★★★ | ★★★★☆ | ★★★★☆ | ★★★★☆ | ★★★★☆ |
| **Python 支持** | ✅ | ✅ | ✅ | ✅ | ✅ API |
| **.NET/Java 支持** | ❌ | ❌ | ❌ | ❌ | ✅ API |
| **本地化部署** | ✅ | ✅ | ✅ | ✅ | ✅ |

#<!-- chunk: 3.2 选型决策树 -->## 3.2 选型决策树

```
框架选型入口
│
├── 是否需要可视化/低代码编排?
│   └── 是 → Dify（快速上线，技术门槛低）
│
├── 核心场景是 RAG/知识检索?
│   └── 是 → LlamaIndex（RAG 能力最强）
│
├── 核心场景是多 Agent 协作?
│   ├── 是 + 需要精细状态控制 → LangGraph（LangChain 的图编排模块）
│   ├── 是 + 需要代码执行 → AutoGen
│   └── 是 + 快速原型 → CrewAI
│
├── 需要最丰富的工具集成?
│   └── 是 → LangChain
│
├── 团队是 .NET 主栈?
│   └── 是 → Semantic Kernel
│
└── 通用 Agent 开发，需要最大灵活性?
    └── → LangChain 或 LlamaIndex（视 RAG 需求）
```

---

<!-- chunk: 4. LangGraph：图结构状态机编排 -->## 4. LangGraph：图结构状态机编排

LangGraph 是 LangChain 生态中用于**复杂多步骤 Agent 编排**的利器，基于有向图（DAG/循环图）定义执行流：

```python
from langgraph.graph import StateGraph, END
from typing import TypedDict, Annotated
import operator

# 1. 定义 Agent 状态
class K8sDiagnosisState(TypedDict):
    problem: str
    cluster_info: str
    events: list[str]
    diagnosis: str
    fix_plan: str
    risk_assessment: str
    approved: bool
    messages: Annotated[list, operator.add]  # 累积消息列表

# 2. 定义节点函数（每个节点代表一个处理步骤）
def collect_cluster_info(state: K8sDiagnosisState) -> K8sDiagnosisState:
    """收集集群状态信息"""
    # 调用 kubectl 工具
    info = kubectl_get_cluster_info()
    return {"cluster_info": info}

def analyze_events(state: K8sDiagnosisState) -> K8sDiagnosisState:
    """分析 K8s 事件"""
    events = kubectl_get_events(state["cluster_info"])
    return {"events": events}

def generate_diagnosis(state: K8sDiagnosisState) -> K8sDiagnosisState:
    """LLM 生成诊断结论"""
    llm_response = llm.invoke(f"""
    问题: {state['problem']}
    集群状态: {state['cluster_info']}
    事件记录: {state['events']}
    请给出根因分析。
    """)
    return {"diagnosis": llm_response.content}

def human_approval(state: K8sDiagnosisState) -> K8sDiagnosisState:
    """人工审批门禁"""
    # 发送 Slack 通知等待审批
    approved = send_approval_request(state["fix_plan"])
    return {"approved": approved}

def route_after_diagnosis(state: K8sDiagnosisState) -> str:
    """条件路由：根据诊断结果决定下一步"""
    if "Critical" in state["diagnosis"]:
        return "escalate"  # 升级处理
    elif "Safe" in state["diagnosis"]:
        return "auto_fix"  # 自动修复
    else:
        return "human_review"  # 人工审查

# 3. 构建图
workflow = StateGraph(K8sDiagnosisState)

# 添加节点
workflow.add_node("collect_info", collect_cluster_info)
workflow.add_node("analyze_events", analyze_events)
workflow.add_node("diagnose", generate_diagnosis)
workflow.add_node("plan_fix", generate_fix_plan)
workflow.add_node("assess_risk", assess_risk)
workflow.add_node("human_review", human_approval)
workflow.add_node("execute_fix", execute_fix_plan)
workflow.add_node("escalate", escalate_to_oncall)

# 定义执行顺序
workflow.set_entry_point("collect_info")
workflow.add_edge("collect_info", "analyze_events")
workflow.add_edge("analyze_events", "diagnose")

# 条件路由
workflow.add_conditional_edges(
    "diagnose",
    route_after_diagnosis,
    {
        "escalate": "escalate",
        "auto_fix": "execute_fix",
        "human_review": "human_review"
    }
)
workflow.add_edge("human_review", "execute_fix")
workflow.add_edge("execute_fix", END)
workflow.add_edge("escalate", END)

# 4. 编译并运行
app = workflow.compile()
result = app.invoke({
    "problem": "prod-cluster API Server 响应时间从 50ms 升至 2000ms",
    "messages": []
})
```

---

<!-- chunk: 5. 框架组合使用策略 -->## 5. 框架组合使用策略

生产环境中，最佳实践是**组合使用多个框架**，而不是强求单一框架解决所有问题：

```
推荐生产架构组合:

  Dify（可视化配置层）
      ↓  API
  LangGraph（复杂流程编排）
      ↓  调用
  LlamaIndex（知识检索引擎）
      +
  LangChain Tools（工具集成）
      ↓  可观测性
  LangSmith / Langfuse（全链路追踪）
```

```python
# 组合使用示例：LangGraph 编排 + LlamaIndex RAG
from langgraph.graph import StateGraph
from llama_index.core import VectorStoreIndex

# RAG 查询函数（LlamaIndex 实现）
def rag_query(question: str) -> str:
    return k8s_knowledge_index.as_query_engine().query(question).response

# LangGraph 节点中调用 LlamaIndex
def knowledge_augmented_diagnosis(state):
    # 使用 LlamaIndex 检索相关知识
    relevant_knowledge = rag_query(
        f"K8s 问题: {state['problem']} 的诊断方法和常见原因"
    )
    # 结合检索结果进行 LLM 推理
    diagnosis = llm.invoke(f"""
    基于以下知识库内容：
    {relevant_knowledge}
    
    针对问题：{state['problem']}
    结合集群实际状态：{state['cluster_info']}
    给出专业诊断。
    """)
    return {"diagnosis": diagnosis.content, "knowledge_used": relevant_knowledge}
```

---

<!-- chunk: 6. 最佳实践与反模式 -->## 6. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **不要过度依赖框架抽象**：生产中遇到问题时，了解底层 API 调用原理比理解框架抽象更重要
- **从简单开始**：先用最简单的 ReAct + OpenAI Functions，需要复杂编排时再引入 LangGraph
- **版本锁定**：LangChain 版本升级 Breaking Changes 多，在 `requirements.txt` 中精确锁定版本
- **评估框架的可观测性**：选框架时同时评估其可追踪性——Langfuse 可对接多数框架
- **自定义工具优于内置工具**：框架内置工具往往不适配你的环境，宁可自己写清晰的工具函数

#<!-- chunk: 反模式 -->## 反模式

- **为用框架而用框架**：简单的 API 调用场景，直接调用 OpenAI SDK 比引入 LangChain 更清晰
- **忽视 Streaming 支持**：对话场景必须用流式输出，框架的 Streaming API 差异较大
- **不处理框架的错误**：框架层的异常往往包装了底层错误，需要剥开才能获取真实信息
- **Dify 用于复杂编程逻辑**：低代码平台不适合需要复杂条件判断和循环的场景

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | Agent Loop 与推理框架 |
| [04 - RAG 检索增强](./04-rag-knowledge-retrieval.md) | LlamaIndex RAG 深度实践 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | LangGraph/AutoGen/CrewAI 在多 Agent 中的进阶用法 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | LangSmith/Langfuse 集成 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization|成本与延迟优化策略]]

## See Also

- 01-ai-agent-fundamentals
- 02-llm-foundation-models
- 04-rag-knowledge-retrieval
- 05-tool-use-function-calling

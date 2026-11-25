---
title: Agent通信协议
description: 'MCP/A2A/ACP协议深度解析：Transport/Tool/Resource模型、Agent-to-Agent协作、协议选型与集成实践'
summary: 'MCP/A2A/ACP协议深度解析：Transport/Tool/Resource模型、Agent-to-Agent协作、协议选型与集成实践'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- mcp
- a2a
- acp
- protocol
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
- Agent通信协议 是什么
- MCP协议详解
- A2A协议详解
- Agent协议选型对比
trigger_keywords:
- mcp
- a2a
- acp
- agent-protocol
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

# Agent通信协议

## 概述

随着AI Agent从单体应用演变为分布式多Agent系统，Agent之间的通信协议成为基础设施层的关键组件。本文档深入解析三大主流Agent通信协议：MCP（Model Context Protocol）、A2A（Agent-to-Agent）和ACP（Agent Communication Protocol），并提供协议选型指南和集成实践。

```
协议定位:

MCP (Model Context Protocol):
  - Anthropic主导的开放协议
  - 定义LLM与外部工具/资源的标准化接口
  - 类比: USB-C for AI - 统一的工具接入标准

A2A (Agent-to-Agent Protocol):
  - Google主导的开放协议
  - 定义Agent之间的发现、协作和通信机制
  - 类比: HTTP for Agents - Agent间通信标准

ACP (Agent Communication Protocol):
  - IBM主导的开放协议
  - 基于消息的Agent通信中间件
  - 类比: AMQP for Agents - 消息队列式通信
```

## MCP (Model Context Protocol) 深度解析

### 协议架构

MCP采用客户端-服务器架构，定义了LLM应用与外部资源之间的标准化通信方式：

```
+-------------------+     +-------------------+
|   MCP Host        |     |   MCP Server      |
| (LLM Application) |<--->| (Tool/Resource    |
|                   |     |  Provider)        |
+-------------------+     +-------------------+
        |                         |
        v                         v
+-------------------+     +-------------------+
|   MCP Client      |     |   External        |
| (Protocol Layer)  |     |   Resources       |
+-------------------+     +-------------------+

Transport层: stdio / SSE / Streamable HTTP
协议层: JSON-RPC 2.0
语义层: Tool / Resource / Prompt / Sampling
```

### Transport层

```python
# MCP支持三种Transport方式

# 1. stdio - 标准输入输出（本地进程）
from mcp.server import Server
from mcp.server.stdio import stdio_server

server = Server("my-tools")

@server.tool()
async def search_database(query: str) -> str:
    """搜索数据库"""
    results = await db.search(query)
    return json.dumps(results)

async def main():
    async with stdio_server() as (read_stream, write_stream):
        await server.run(read_stream, write_stream)


# 2. SSE - Server-Sent Events（HTTP长连接）
from mcp.server.sse import SseServerTransport
from starlette.applications import Starlette
from starlette.routing import Route

transport = SseServerTransport("/messages")

async def handle_sse(request):
    async with transport.connect_sse(
        request.scope, request.receive, request._send
    ) as streams:
        await server.run(streams[0], streams[1])

app = Starlette(routes=[
    Route("/sse", endpoint=handle_sse),
    Route("/messages", endpoint=transport.handle_post_message,
          methods=["POST"]),
])


# 3. Streamable HTTP（推荐的新方式）
from mcp.server.streamable_http import StreamableHTTPServerTransport

transport = StreamableHTTPServerTransport("/mcp")

app = Starlette(routes=[
    Route("/mcp", endpoint=transport.handle_request),
])
```

### Tool定义与实现

```python
from mcp.types import Tool, TextContent
from pydantic import BaseModel, Field

class SearchInput(BaseModel):
    query: str = Field(description="搜索查询")
    max_results: int = Field(default=10, description="最大结果数")

@server.tool()
async def web_search(input: SearchInput) -> list[TextContent]:
    """搜索互联网获取最新信息"""
    results = await search_engine.search(
        query=input.query,
        limit=input.max_results,
    )

    formatted = "\n".join(
        f"- {r.title}: {r.snippet}" for r in results
    )
    return [TextContent(type="text", text=formatted)]


@server.tool()
async def execute_sql(
    database: str,
    query: str,
) -> list[TextContent]:
    """执行SQL查询（只读）"""
    # 安全检查
    if any(keyword in query.upper() for keyword in
           ["INSERT", "UPDATE", "DELETE", "DROP", "ALTER"]):
        return [TextContent(
            type="text",
            text="错误: 只允许SELECT查询",
        )]

    try:
        result = await db.execute(database, query)
        return [TextContent(
            type="text",
            text=json.dumps(result, ensure_ascii=False),
        )]
    except Exception as e:
        return [TextContent(
            type="text",
            text=f"查询错误: {str(e)}",
        )]
```

### Resource暴露

```python
@server.resource("file:///{path}")
async def read_file(path: str) -> str:
    """读取文件内容"""
    full_path = validate_path(path)
    with open(full_path, "r") as f:
        return f.read()


@server.resource("db:///{table}")
async def get_table_schema(table: str) -> str:
    """获取数据库表结构"""
    schema = await db.get_schema(table)
    return json.dumps(schema)


@server.resource("config:///{key}")
async def get_config(key: str) -> str:
    """获取配置信息"""
    value = config.get(key)
    return json.dumps(value)
```

### Prompt模板

```python
@server.prompt()
async def code_review(
    code: str,
    language: str = "python",
) -> str:
    """代码审查提示模板"""
    return f"""请审查以下{language}代码，关注:
1. 潜在的bug和错误
2. 性能问题
3. 安全漏洞
4. 代码风格和最佳实践

```{language}
{code}
```

请提供详细的审查报告。"""


@server.prompt()
async def sql_generator(
    schema: str,
    requirement: str,
) -> str:
    """SQL生成提示模板"""
    return f"""基于以下数据库结构:
{schema}

生成满足以下需求的SQL查询:
{requirement}

要求:
1. 只生成SELECT查询
2. 添加适当的注释
3. 考虑查询性能
"""
```

### 客户端集成

```python
from mcp.client import ClientSession
from mcp.client.sse import sse_client

async def use_mcp_server():
    """连接MCP服务器并使用工具"""
    async with sse_client("http://localhost:8080/sse") as (
        read_stream, write_stream
    ):
        async with ClientSession(read_stream, write_stream) as session:
            # 初始化连接
            await session.initialize()

            # 列出可用工具
            tools = await session.list_tools()
            print(f"可用工具: {[t.name for t in tools.tools]}")

            # 列出可用资源
            resources = await session.list_resources()
            print(f"可用资源: {[r.uri for r in resources.resources]}")

            # 调用工具
            result = await session.call_tool(
                "web_search",
                arguments={
                    "query": "kubernetes best practices",
                    "max_results": 5,
                },
            )
            print(f"搜索结果: {result.content}")

            # 读取资源
            resource = await session.read_resource("file:///README.md")
            print(f"文件内容: {resource.contents}")
```

## A2A (Agent-to-Agent) 协议

### 协议概述

A2A协议定义了Agent之间的标准化通信方式，使不同框架构建的Agent能够相互发现和协作：

```
A2A核心概念:

Agent Card:
  - Agent的自描述文档
  - 包含能力、端点、认证信息
  - 类似API的OpenAPI规范

Task:
  - Agent间的工作单元
  - 包含状态机（submitted → working → completed）
  - 支持长时间运行

Artifact:
  - Task的输出产物
  - 支持多种内容类型
  - 可以是文件、数据、流式内容

Message:
  - Agent间的通信消息
  - 支持文本、结构化数据、文件
  - 包含角色和上下文信息
```

### Agent Card定义

```json
{
  "name": "Research Agent",
  "description": "执行深度研究并生成报告的Agent",
  "url": "https://research-agent.example.com",
  "version": "1.0.0",
  "capabilities": {
    "streaming": true,
    "pushNotifications": true,
    "stateTransitionHistory": true
  },
  "authentication": {
    "schemes": ["bearer"],
    "credentials": "Bearer token required"
  },
  "defaultInputModes": ["text/plain", "application/json"],
  "defaultOutputModes": ["text/plain", "text/markdown"],
  "skills": [
    {
      "id": "web-research",
      "name": "Web Research",
      "description": "搜索互联网并提取信息",
      "tags": ["research", "search", "information"],
      "examples": [
        "研究Kubernetes最佳实践",
        "查找最新的AI论文"
      ]
    },
    {
      "id": "report-generation",
      "name": "Report Generation",
      "description": "生成结构化的研究报告",
      "tags": ["writing", "report", "analysis"]
    }
  ]
}
```

### Task生命周期

```python
from a2a.types import Task, TaskState, Message, Artifact
from a2a.server import A2AServer

class ResearchAgentServer(A2AServer):
    """实现A2A协议的Research Agent"""

    async def handle_task(
        self,
        task: Task,
    ) -> Task:
        """处理Agent任务"""
        # 更新任务状态为工作中
        task.status.state = TaskState.WORKING
        await self.notify_status_change(task)

        try:
            # 提取用户消息
            user_message = task.message
            query = user_message.parts[0].text

            # 执行研究
            research_result = await self.conduct_research(query)

            # 创建输出产物
            artifact = Artifact(
                name="research-report",
                parts=[
                    {
                        "type": "text",
                        "text": research_result.report,
                    },
                    {
                        "type": "file",
                        "file": {
                            "name": "sources.json",
                            "mime_type": "application/json",
                            "data": base64.b64encode(
                                json.dumps(research_result.sources).encode()
                            ).decode(),
                        },
                    },
                ],
            )

            # 更新任务状态为完成
            task.status.state = TaskState.COMPLETED
            task.artifacts = [artifact]

        except Exception as e:
            task.status.state = TaskState.FAILED
            task.status.message = str(e)

        return task

    async def conduct_research(self, query: str) -> ResearchResult:
        """执行研究任务"""
        # 搜索相关信息
        search_results = await self.web_search(query)

        # 分析和综合
        analysis = await self.analyze(search_results)

        # 生成报告
        report = await self.generate_report(analysis)

        return ResearchResult(
            report=report,
            sources=search_results,
        )
```

### Agent间协作

```python
from a2a.client import A2AClient

async def multi_agent_research(topic: str):
    """多Agent协作研究"""

    # 1. 发现可用Agent
    research_agent = await A2AClient.discover(
        "https://research-agent.example.com"
    )
    writing_agent = await A2AClient.discover(
        "https://writing-agent.example.com"
    )
    review_agent = await A2AClient.discover(
        "https://review-agent.example.com"
    )

    # 2. 创建研究任务
    research_task = await research_agent.create_task(
        message=Message(
            role="user",
            parts=[{"type": "text", "text": f"深度研究: {topic}"}],
        ),
    )

    # 3. 等待研究完成
    research_result = await research_agent.wait_for_completion(
        research_task.id,
    )

    # 4. 将研究结果发送给写作Agent
    writing_task = await writing_agent.create_task(
        message=Message(
            role="user",
            parts=[{
                "type": "text",
                "text": f"基于以下研究结果撰写报告:\n{research_result.artifacts[0].parts[0].text}",
            }],
        ),
    )

    # 5. 等待写作完成
    writing_result = await writing_agent.wait_for_completion(
        writing_task.id,
    )

    # 6. 发送给审查Agent
    review_task = await review_agent.create_task(
        message=Message(
            role="user",
            parts=[{
                "type": "text",
                "text": f"审查以下报告:\n{writing_result.artifacts[0].parts[0].text}",
            }],
        ),
    )

    # 7. 获取最终结果
    review_result = await review_agent.wait_for_completion(
        review_task.id,
    )

    return {
        "report": writing_result.artifacts[0].parts[0].text,
        "review": review_result.artifacts[0].parts[0].text,
    }
```

## ACP (Agent Communication Protocol)

### 协议概述

ACP基于消息队列模式，提供异步、可靠的Agent通信：

```
ACP架构特点:

消息驱动:
  - 基于消息队列的异步通信
  - 支持发布/订阅模式
  - 消息持久化和可靠投递

松耦合:
  - Agent无需知道对方地址
  - 通过消息代理间接通信
  - 支持动态扩缩容

可靠性:
  - 消息确认机制
  - 死信队列处理失败消息
  - 支持消息重试
```

### ACP实现

```python
from acp import ACPAgent, Message, MessageType

class ResearchACPAgent(ACPAgent):
    """基于ACP的Research Agent"""

    def __init__(self, agent_id: str, broker_url: str):
        super().__init__(agent_id, broker_url)
        self.register_handler(
            MessageType.REQUEST,
            self.handle_research_request,
        )

    async def handle_research_request(self, message: Message):
        """处理研究请求"""
        query = message.payload["query"]

        # 执行研究
        result = await self.conduct_research(query)

        # 发送响应
        response = Message(
            type=MessageType.RESPONSE,
            sender=self.agent_id,
            recipient=message.sender,
            correlation_id=message.id,
            payload={
                "result": result.report,
                "sources": result.sources,
            },
        )

        await self.send(response)

    async def conduct_research(self, query: str):
        """执行研究逻辑"""
        # 发布搜索请求到搜索Agent
        search_response = await self.request(
            recipient="search-agent",
            payload={"query": query, "type": "web_search"},
            timeout=30,
        )

        # 发布分析请求到分析Agent
        analysis_response = await self.request(
            recipient="analysis-agent",
            payload={
                "data": search_response.payload["results"],
                "type": "text_analysis",
            },
            timeout=60,
        )

        return ResearchResult(
            report=analysis_response.payload["report"],
            sources=search_response.payload["results"],
        )


# 使用示例
async def main():
    # 启动Agent
    agent = ResearchACPAgent(
        agent_id="research-agent-001",
        broker_url="amqp://localhost:5672",
    )

    # 订阅主题
    await agent.subscribe("research.requests")

    # 开始处理消息
    await agent.start()
```

### 消息路由

```python
class ACPRouter:
    """ACP消息路由器"""

    def __init__(self, broker_url: str):
        self.broker = MessageBroker(broker_url)
        self.routes = {}

    def register_route(
        self,
        pattern: str,
        handler: ACPAgent,
    ):
        """注册消息路由"""
        self.routes[pattern] = handler

    async def route_message(self, message: Message):
        """路由消息到目标Agent"""
        # 基于消息类型路由
        if message.type == MessageType.REQUEST:
            # 查找目标Agent
            recipient = message.recipient
            if recipient in self.routes:
                await self.routes[recipient].receive(message)
            else:
                # 广播到订阅者
                await self.broker.publish(
                    topic=f"requests.{recipient}",
                    message=message,
                )

        elif message.type == MessageType.PUBLISH:
            # 发布/订阅模式
            topic = message.payload.get("topic", "default")
            await self.broker.publish(
                topic=topic,
                message=message,
            )
```

## OpenAI Agents SDK

### SDK概述

OpenAI Agents SDK提供原生的多Agent协作能力：

```python
from openai.agents import Agent, Runner

# 定义Agent
research_agent = Agent(
    name="Research Agent",
    instructions="""你是一个研究助手。
    搜索互联网获取信息，并提供准确、最新的答案。""",
    model="gpt-4o",
    tools=[
        WebSearchTool(),
        FileSearchTool(),
    ],
)

writing_agent = Agent(
    name="Writing Agent",
    instructions="""你是一个写作专家。
    基于提供的材料撰写清晰、结构化的报告。""",
    model="gpt-4o",
)

review_agent = Agent(
    name="Review Agent",
    instructions="""你是一个审查专家。
    审查内容的准确性、完整性和质量。""",
    model="gpt-4o",
    tools=[CodeInterpreterTool()],
)

# Agent间协作
async def research_and_write(topic: str):
    # 1. 研究阶段
    research_result = await Runner.run(
        starting_agent=research_agent,
        input=f"深度研究以下主题: {topic}",
    )

    # 2. 写作阶段
    writing_result = await Runner.run(
        starting_agent=writing_agent,
        input=f"基于以下研究撰写报告:\n{research_result.final_output}",
    )

    # 3. 审查阶段
    review_result = await Runner.run(
        starting_agent=review_agent,
        input=f"审查以下报告:\n{writing_result.final_output}",
    )

    return {
        "research": research_result.final_output,
        "report": writing_result.final_output,
        "review": review_result.final_output,
    }
```

### Handoff机制

```python
from openai.agents import Agent, handoff

# 定义带Handoff的Agent
triage_agent = Agent(
    name="Triage Agent",
    instructions="""你是任务分发Agent。
    根据用户请求的类型，将任务转交给合适的Agent:
    - 研究类请求 → Research Agent
    - 写作类请求 → Writing Agent
    - 代码类请求 → Code Agent""",
    model="gpt-4o",
    handoffs=[
        handoff(
            agent=research_agent,
            description="处理研究和信息收集类请求",
        ),
        handoff(
            agent=writing_agent,
            description="处理写作和内容生成类请求",
        ),
        handoff(
            agent=code_agent,
            description="处理编程和代码相关请求",
        ),
    ],
)

# 使用
result = await Runner.run(
    starting_agent=triage_agent,
    input="研究Kubernetes网络策略的最佳实践",
)
```

## 协议选型对比

### 功能对比

```
特性对比表:

                MCP          A2A          ACP          OpenAI SDK
─────────────────────────────────────────────────────────────────
定位         工具接入      Agent协作     消息通信      SDK集成
通信模式     请求/响应     任务驱动      异步消息      函数调用
发现机制     静态配置      Agent Card    主题订阅      代码定义
状态管理     无状态        Task状态机    消息状态      Runner管理
流式支持     SSE/HTTP      SSE           消息流        流式API
安全性       OAuth/API Key mTLS/JWT      SASL/TLS     API Key
适用场景     工具集成      跨组织协作    企业内部      快速原型
成熟度       高(GA)        中(Preview)   中           高(GA)
```

### 选型指南

```
选型决策树:

需要接入外部工具/API?
  └── 是 → MCP
       统一的工具接入标准
       广泛的生态支持

需要跨组织Agent协作?
  └── 是 → A2A
       标准化的Agent发现
       支持异构Agent

需要高可靠异步通信?
  └── 是 → ACP
       消息队列保证可靠投递
       支持复杂路由

快速原型或OpenAI生态?
  └── 是 → OpenAI Agents SDK
       开箱即用的多Agent支持
       与OpenAI服务深度集成

组合使用:
  MCP + A2A: 工具接入 + Agent协作
  MCP + ACP: 工具接入 + 异步通信
  A2A + ACP: Agent协作 + 消息可靠性
```

### 集成架构

```python
# 组合使用MCP + A2A的Agent架构
class HybridAgent:
    """支持MCP和A2A的混合Agent"""

    def __init__(self):
        # MCP: 作为Server暴露工具
        self.mcp_server = Server("hybrid-agent")

        # A2A: 作为Agent参与协作
        self.a2a_server = A2AServer(
            agent_card=self._build_agent_card(),
        )

        # 注册MCP工具
        self._register_mcp_tools()

        # 注册A2A处理器
        self._register_a2a_handlers()

    def _register_mcp_tools(self):
        @self.mcp_server.tool()
        async def search(query: str) -> str:
            """搜索工具"""
            return await self.search(query)

        @self.mcp_server.tool()
        async def analyze(data: str) -> str:
            """分析工具"""
            return await self.analyze(data)

    def _register_a2a_handlers(self):
        @self.a2a_server.task_handler()
        async def handle_research(task: Task) -> Task:
            """处理研究任务"""
            query = task.message.parts[0].text

            # 使用MCP工具执行研究
            search_result = await self.mcp_client.call_tool(
                "search", {"query": query}
            )
            analysis = await self.mcp_client.call_tool(
                "analyze", {"data": search_result}
            )

            task.status.state = TaskState.COMPLETED
            task.artifacts = [Artifact(
                parts=[{"type": "text", "text": analysis}]
            )]
            return task

    async def start(self):
        """启动Agent服务"""
        await asyncio.gather(
            self.mcp_server.run(),
            self.a2a_server.run(),
        )
```

---

*MCP/A2A/ACP三大协议分别解决了工具接入、Agent协作和消息通信三个维度的问题，组合使用可以构建完整的Agent通信基础设施。*

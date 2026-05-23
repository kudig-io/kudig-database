---
title: AgentScope 工具系统与 MCP 集成 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: 工具开发专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Toolkit,'
category: general
tags:
- ai
- ai-agent
- prometheus
- postgresql
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AgentScope 工具系统与 MCP 集成 是什么
- 如何 AgentScope 工具系统与 MCP 集成
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 工具系统与
- MCP
- 集成
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
created: "2026-05-23"
---

title: AgentScope 工具系统与 MCP 集成
description: '**文档类型**: 工具开发专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Toolkit,
  工具注册, MCP, Model Context Protocol, Function Calling, 并行工具调用, Agent [[SKILL|Skill]], Meta Tool,
  自定义工具'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- postgresql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 工具系统与 MCP 集成 是什么
- 如何 AgentScope 工具系统与 MCP 集成
trigger_keywords:
- AgentScope
- 工具系统与
- MCP
- 集成
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

# AgentScope 工具系统与 MCP 集成

> **文档类型**: 工具开发专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Toolkit, 工具注册, MCP, Model Context Protocol, Function Calling, 并行工具调用, Agent Skill, Meta Tool, 自定义工具

---

<!-- chunk: 概述 -->## 概述

工具系统是 Agent 从"对话助手"升级为"自主执行者"的关键。AgentScope 的工具系统设计极其灵活——**任何 Python 可调用对象都可以作为工具**，无需特定装饰器或 Schema 定义。同时原生支持 MCP（Model Context Protocol）协议，可无缝接入外部工具服务。

本文详解 AgentScope 工具系统的注册机制、内置工具、MCP 集成、并行调用、Meta Tool，以及面向 K8s 运维的自定义工具开发实践。

---

<!-- chunk: 1. 工具系统设计哲学 -->## 1. 工具系统设计哲学

#<!-- chunk: 1.1 "一切可调用对象皆工具" -->## 1.1 "一切可调用对象皆工具"

AgentScope 中的"工具"定义非常宽泛：

```
AgentScope 支持的工具类型
│
├── 函数（function）
├── 偏函数（functools.partial）
├── 实例方法（instance method）
├── 类方法（classmethod）
├── 静态方法（staticmethod）
└── 带 __call__ 方法的可调用实例
```

并且每种工具都可以是：

```
调用模式
├── 同步 (sync)   或 异步 (async)
├── 流式 (stream) 或 非流式 (non-stream)
└── 有状态 或 无状态
```

#<!-- chunk: 1.2 与其他框架的工具定义对比 -->## 1.2 与其他框架的工具定义对比

| 框架 | 工具定义方式 | 复杂度 |
|------|------------|--------|
| LangChain | 需要 `@tool` 装饰器或 `StructuredTool` + Pydantic Schema | 中等 |
| AutoGen | 通过函数注册或 `register_for_execution` | 中等 |
| CrewAI | 继承 `BaseTool` 或 `@tool` 装饰器 | 中等 |
| **AgentScope** | **直接注册任意可调用对象，无需装饰器** | **最简** |

---

<!-- chunk: 2. Toolkit — 工具注册中心 -->## 2. Toolkit — 工具注册中心

#<!-- chunk: 2.1 基础使用 -->## 2.1 基础使用

```python
from agentscope.tool import Toolkit, ToolResponse
import os


# 定义工具函数 — 返回值推荐使用 ToolResponse
def get_weather(city: str) -> ToolResponse:
    """获取指定城市的天气信息。

    Args:
        city: 城市名称，如 "北京"、"上海"

    Returns:
        天气信息
    """
    # 实际实现：调用天气 API
    return ToolResponse(text=f"{city}今日天气：晴，温度 25°C")


def calculate(expression: str) -> str:
    """计算数学表达式。

    Args:
        expression: 数学表达式，如 "2 + 3 * 4"

    Returns:
        计算结果
    """
    try:
        result = eval(expression)
        return str(result)
    except Exception as e:
        return f"计算错误: {e}"


# 注册工具
toolkit = Toolkit()
toolkit.register_tool_function(get_weather)
toolkit.register_tool_function(calculate)

# 使用 preset_kwargs 隐藏敏感参数（如 API Key），LLM 不可见这些参数
toolkit.register_tool_function(
    get_weather,
    preset_kwargs={"api_key": os.environ["WEATHER_API_KEY"]},
)

# 传递给 Agent
agent = ReActAgent(
    name="Assistant",
    toolkit=toolkit,
    ...
)
```

> **关键点**：
> - AgentScope 通过函数的 **docstring** 和 **type hints** 自动生成工具描述（JSON Schema），供 LLM 理解工具用途和参数。因此务必为工具函数编写清晰的文档字符串。
> - 工具函数推荐返回 `ToolResponse` 而非 `str`。`ToolResponse` 支持 `text`、`image_url` 等多种内容类型。
> - 使用 `preset_kwargs` 可将 API Key 等敏感参数预设进工具，不暴露给 LLM 的 JSON Schema。

#<!-- chunk: 2.2 异步工具 -->## 2.2 异步工具

```python
import aiohttp


async def async_fetch_url(url: str) -> str:
    """异步获取 URL 内容。

    Args:
        url: 要获取的 URL 地址

    Returns:
        URL 页面内容
    """
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            return await response.text()


toolkit = Toolkit()
toolkit.register_tool_function(async_fetch_url)
```

#<!-- chunk: 2.3 流式工具 -->## 2.3 流式工具

```python
from typing import AsyncGenerator


async def stream_log_tail(
    pod_name: str,
    namespace: str = "default",
    lines: int = 100,
) -> AsyncGenerator[str, None]:
    """流式获取 Pod 日志。

    Args:
        pod_name: Pod 名称
        namespace: 命名空间
        lines: 获取的日志行数

    Yields:
        日志行内容
    """
    import asyncio
    process = await asyncio.create_subprocess_exec(
        "kubectl", "logs", pod_name, "-n", namespace,
        f"--tail={lines}", "-f",
        stdout=asyncio.subprocess.PIPE,
    )
    async for line in process.stdout:
        yield line.decode().strip()


toolkit = Toolkit()
toolkit.register_tool_function(stream_log_tail)
```

#<!-- chunk: 2.4 偏函数与可调用对象 -->## 2.4 偏函数与可调用对象

```python
from functools import partial


def kubectl_command(verb: str, resource: str, name: str, namespace: str = "default") -> str:
    """执行 kubectl 命令"""
    import subprocess
    cmd = ["kubectl", verb, resource, name, "-n", namespace]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


# 使用偏函数创建特定工具
kubectl_get = partial(kubectl_command, verb="get")
kubectl_describe = partial(kubectl_command, verb="describe")

toolkit = Toolkit()
toolkit.register_tool_function(kubectl_get)
toolkit.register_tool_function(kubectl_describe)
```

```python
# 可调用对象作为工具
class DatabaseQuery:
    """数据库查询工具"""

    def __init__(self, connection_string: str):
        self.conn_str = connection_string

    async def __call__(self, sql: str) -> str:
        """执行 SQL 查询。

        Args:
            sql: SQL 查询语句（只读）

        Returns:
            查询结果
        """
        # 实际实现：执行 SQL 查询
        return f"Query result for: {sql}"


db_query = DatabaseQuery("postgresql://localhost:5432/k8s_metrics")
toolkit = Toolkit()
toolkit.register_tool_function(db_query)
```

---

<!-- chunk: 3. 内置工具 -->## 3. 内置工具

AgentScope 提供多类内置工具函数，开箱即用：

| 工具函数 | 用途 | 注意事项 |
|---------|------|--------|
| `execute_python_code` | 执行 Python 代码 | 生产环境必须在沙箱中运行 |
| `execute_shell_command` | 执行 Shell 命令 | 生产环境必须在沙箱中运行 |
| `view_text_file` | 查看文本文件内容 | 只读操作 |
| `write_text_file` | 写入文本文件 | 需文件系统权限 |
| `insert_text_file` | 在文件指定位置插入内容 | 精确编辑场景 |
| `dashscope_text_to_image` | 通义万相文生图 | 需 DashScope API Key |
| `openai_text_to_image` | DALL-E 文生图 | 需 OpenAI API Key |

#<!-- chunk: 3.1 代码执行 -->## 3.1 代码执行

```python
from agentscope.tool import (
    execute_python_code,
    execute_shell_command,
    view_text_file,
    write_text_file,
    Toolkit,
)

toolkit = Toolkit()
toolkit.register_tool_function(execute_python_code)
toolkit.register_tool_function(execute_shell_command)
toolkit.register_tool_function(view_text_file)
toolkit.register_tool_function(write_text_file)
```

**execute_python_code**：

```python
# Agent 调用示例
# Input: {"code": "import math; print(math.pi)", "timeout": 300}
# Output: "<returncode>0</returncode><stdout>3.141592653589793\n</stdout>"
```

**execute_shell_command**：

```python
# Agent 调用示例
# Input: {"command": "kubectl get pods -n production"}
# Output: 命令执行结果
```

> **安全警告**：在生产环境中，代码执行工具应在**沙箱**中运行。AgentScope Runtime 提供了安全沙箱环境，详见 [22 - 生产部署](./deployment.md|22-agentscope-production-deployment]].md)。

---

<!-- chunk: 4. 动态 JSON Schema 扩展 -->## 4. 动态 JSON Schema 扩展

AgentScope 支持通过 Pydantic 模型动态扩展工具的 JSON Schema，典型用例是在工具调用中添加 **Chain-of-Thought 思考字段**：

```python
from pydantic import BaseModel, Field
from agentscope.tool import Toolkit


class CoTThinking(BaseModel):
    """链式思考扩展——让 LLM 在调用工具前先输出推理过程"""
    thinking: str = Field(description="工具调用前的推理过程")


toolkit = Toolkit()
toolkit.register_tool_function(kubectl_get_pods)

# 将 CoT 思考字段动态注入到所有工具的 JSON Schema 中
toolkit.set_extended_model(CoTThinking)
```

加入后，LLM 生成的工具调用会包含额外的 `thinking` 字段：

```json
{
  "type": "tool_use",
  "name": "kubectl_get_pods",
  "input": {
    "thinking": "Pod Pending 问题需要先查看 Pod 列表确认状态...",
    "namespace": "production",
    "label_selector": "app=nginx"
  }
}
```

> **适用场景**：调试 Agent 的推理过程、可解释性要求高的生产场景、收集 Agentic RL 训练数据。

---

<!-- chunk: 5. 工具中断支持 -->## 5. 工具中断支持

当用户发送实时中断时，正在执行的工具会收到 `asyncio.CancelledError`。工具可以优雅地处理中断：

```python
import asyncio
from agentscope.tool import ToolResponse


async def long_running_analysis(
    namespace: str,
    depth: str = "full",
) -> ToolResponse:
    """执行深度集群分析（可能耗时较长）。

    Args:
        namespace: 目标命名空间
        depth: 分析深度，"quick" 或 "full"
    """
    results = []
    try:
        # 步骤 1: 收集 Pod 信息
        pod_info = await collect_pod_info(namespace)
        results.append(pod_info)

        # 步骤 2: 收集节点信息
        node_info = await collect_node_info()
        results.append(node_info)

        # 步骤 3: 资源分析...
        analysis = await run_analysis(results)
        return ToolResponse(text=analysis)

    except asyncio.CancelledError:
        # 优雅处理中断——返回已完成的部分结果
        partial = "\n".join(results) if results else "分析未开始"
        return ToolResponse(
            text=f"分析已中断。已完成的结果:\n{partial}",
            is_interrupted=True,  # 标记为中断状态
        )
```

> **注意**：工具函数内必须显式捕获 `asyncio.CancelledError`。如果不捕获，工具会被强制取消，返回空结果。设置 `is_interrupted=True` 后，Agent 会知道工具被中断，可继续处理用户的新指令。

---

<!-- chunk: 6. 并行工具调用 -->## 6. 并行工具调用

#<!-- chunk: 4.1 启用并行调用 -->## 4.1 启用并行调用

```python
agent = ReActAgent(
    name="K8s-Expert",
    parallel_tool_calls=True,   # 启用并行工具调用
    toolkit=toolkit,
    ...
)
```

当 LLM 在一次推理中生成多个工具调用时，AgentScope 会并行执行它们：

```
顺序执行 (parallel_tool_calls=False):
  get_pods() ──► describe_pod() ──► get_events()
  总耗时: t1 + t2 + t3

并行执行 (parallel_tool_calls=True):
  get_pods()     ──►
  describe_pod() ──►  （并行执行，取最长耗时）
  get_events()   ──►
  总耗时: max(t1, t2, t3)
```

#<!-- chunk: 4.2 适用场景 -->## 4.2 适用场景

| 场景 | 是否适合并行 | 原因 |
|------|------------|------|
| 同时查询多个资源状态 | 适合 | 各查询独立无依赖 |
| 先获取 Pod 列表再 describe | 不适合 | 后者依赖前者结果 |
| 同时检查 CPU + 内存 + 磁盘 | 适合 | 监控指标采集独立 |
| 执行修复操作 | 不适合 | 需要顺序验证每步结果 |

---

<!-- chunk: 7. MCP 集成 -->## 7. MCP 集成

#<!-- chunk: 7.1 什么是 MCP -->## 7.1 什么是 MCP

MCP（Model Context Protocol）是由 Anthropic 提出的标准化工具协议，允许 Agent 通过统一接口调用外部工具服务。AgentScope 原生支持 MCP。

```
MCP 架构
│
├── MCP Server（工具提供者）
│   提供标准化的工具描述和调用接口
│   例: 高德地图 MCP、GitHub MCP、Slack MCP
│
└── MCP Client（AgentScope 内置）
    ├── HttpStatelessClient  → 无状态 HTTP 连接（最常用）
    ├── HttpStatefulClient   → 有状态 HTTP 连接（持久会话）
    └── StdIOStatefulClient  → 本地进程通信（stdio）
```

#<!-- chunk: 7.2 MCP 客户端类型 -->## 7.2 MCP 客户端类型

| 客户端类型 | 传输方式 | 适用场景 |
|-----------|---------|--------|
| `HttpStatelessClient` | `streamable_http` | 远程 MCP Server（无状态，最常用） |
| `HttpStatefulClient` | `streamable_http` | 远程 MCP Server（有状态，持久会话） |
| `StdIOStatefulClient` | `stdio` | 本地进程 MCP Server（通过 stdin/stdout） |

#<!-- chunk: 7.3 使用 MCP 工具 -->## 7.3 使用 MCP 工具

**方式一：获取单个 MCP 工具作为本地函数**

```python
from agentscope.mcp import HttpStatelessClient
from agentscope.tool import Toolkit
import os


async def use_mcp_tool():
    # 初始化 MCP 客户端
    client = HttpStatelessClient(
        name="gaode_mcp",
        transport="streamable_http",
        url=f"https://mcp.amap.com/mcp?key={os.environ['GAODE_API_KEY']}",
    )

    # 获取 MCP 工具作为本地可调用函数
    # wrap_tool_result=True 让返回值自动包装为 ToolResponse
    geo_func = await client.get_callable_function(
        func_name="maps_geo",
        wrap_tool_result=True,
    )

    # 直接调用
    result = await geo_func(address="天安门广场", city="北京")
    print(result)

    # 注册到 Toolkit 供 Agent 使用
    toolkit = Toolkit()
    toolkit.register_tool_function(geo_func)
```

**方式二：使用 `register_mcp_client` 一键注册整个 MCP Server**

```python
async def register_all_mcp_tools():
    client = HttpStatelessClient(
        name="github_mcp",
        transport="streamable_http",
        url="https://mcp.github.com/mcp",
    )

    toolkit = Toolkit()

    # 一键注册——自动发现并注册 MCP Server 上的所有工具
    await toolkit.register_mcp_client(client)

    # 动态移除 MCP 客户端（并取消注册其工具）
    # toolkit.remove_mcp_clients("github_mcp")

    return toolkit
```

**方式三：本地 stdio MCP Server**

```python
from agentscope.mcp import StdIOStatefulClient

async def use_local_mcp():
    # 启动本地 MCP Server 进程（通过 stdio 通信）
    client = StdIOStatefulClient(
        name="local_tools",
        command="python",
        args=["-m", "my_mcp_server"],
    )

    toolkit = Toolkit()
    await toolkit.register_mcp_client(client)
    return toolkit
```

**方式四：组合 MCP 工具与本地工具**

```python
async def composite_toolkit():
    # MCP 工具
    mcp_client = HttpStatelessClient(
        name="maps",
        transport="streamable_http",
        url=f"https://mcp.amap.com/mcp?key={os.environ['GAODE_API_KEY']}",
    )

    # 本地工具
    def get_current_time() -> str:
        """获取当前时间"""
        from datetime import datetime
        return datetime.now().isoformat()

    # 组合注册
    toolkit = Toolkit()
    await toolkit.register_mcp_client(mcp_client)       # MCP 工具（一键注册）
    toolkit.register_tool_function(get_current_time)     # 本地工具
    toolkit.register_tool_function(execute_python_code)  # 内置工具

    return toolkit
```

---

<!-- chunk: 8. Meta Tool — 智能体自主管理工具 -->## 8. Meta Tool — 智能体自主管理工具

#<!-- chunk: 6.1 概念 -->## 6.1 概念

启用 Meta Tool 后，智能体可以在运行时**动态管理自己的工具集**——添加、移除、查询可用工具。

```python
agent = ReActAgent(
    name="Dynamic-Agent",
    enable_meta_tool=True,   # 启用 Meta Tool
    toolkit=toolkit,
    ...
)
```

#<!-- chunk: 6.2 适用场景 -->## 6.2 适用场景

```
Meta Tool 适用场景
│
├── 工具集过大（>20 个）时，智能体按需加载
├── 运行时发现新工具并注册
├── 根据任务阶段动态切换工具集
└── 多 Agent 场景中共享/传递工具
```

---

<!-- chunk: 9. Toolkit 中间件（Middleware） -->## 9. Toolkit 中间件（Middleware）

AgentScope 的中间件机制注册在 **Toolkit**（而非 Agent）上，采用洋葱模型（Onion Model），可在工具执行前后插入自定义逻辑。

#<!-- chunk: 9.1 洋葱模型 -->## 9.1 洋葱模型

```
Toolkit 中间件执行顺序（洋葱模型）
│
│  → AuthorizationMiddleware.pre  （最外层）
│    → LoggingMiddleware.pre
│      → 实际工具执行         （核心）
│    ← LoggingMiddleware.post
│  ← AuthorizationMiddleware.post （最外层）
```

#<!-- chunk: 9.2 中间件签名 -->## 9.2 中间件签名

```python
from typing import AsyncGenerator
from agentscope.tool import ToolResponse


async def my_middleware(
    kwargs: dict,           # 工具调用参数
    next_handler,           # 下一个中间件或实际工具
) -> AsyncGenerator[ToolResponse, None]:
    # === 前置逻辑（工具执行前） ===
    print(f"工具参数: {kwargs}")

    # 调用下一层
    async for response in next_handler(kwargs):
        # === 后置逻辑（工具执行后，可修改返回值） ===
        yield response
```

#<!-- chunk: 9.3 实践示例 -->## 9.3 实践示例

**权限控制中间件**：

```python
async def authorization_middleware(kwargs, next_handler):
    """工具执行权限控制——禁止危险操作"""
    tool_name = kwargs.get("_tool_name", "")
    dangerous_tools = {"execute_shell_command", "write_text_file"}

    if tool_name in dangerous_tools:
        yield ToolResponse(
            text=f"权限拒绝: {tool_name} 不允许在当前环境执行"
        )
        return  # 不调用实际工具

    async for response in next_handler(kwargs):
        yield response


# 注册中间件到 Toolkit
toolkit = Toolkit()
toolkit.register_tool_function(execute_shell_command)
toolkit.register_middleware(authorization_middleware)
```

**输出转换中间件**：

```python
async def output_transform_middleware(kwargs, next_handler):
    """统一截断过长的工具输出，防止上下文爆炸"""
    MAX_OUTPUT_LENGTH = 5000

    async for response in next_handler(kwargs):
        if response.text and len(response.text) > MAX_OUTPUT_LENGTH:
            truncated = response.text[:MAX_OUTPUT_LENGTH]
            yield ToolResponse(
                text=f"{truncated}\n\n[输出已截断，原始长度: {len(response.text)} 字符]"
            )
        else:
            yield response


toolkit.register_middleware(output_transform_middleware)
```

> **Hooks vs Middleware**：
> - **Hooks** 作用于 **Agent** 级别（reply/observe/print 的前后）
> - **Middleware** 作用于 **Toolkit** 级别（工具执行的前后）

---

<!-- chunk: 10. K8s 运维工具集成实践 -->## 10. K8s 运维工具集成实践

#<!-- chunk: 7.1 kubectl 工具集 -->## 7.1 kubectl 工具集

```python
import subprocess
from agentscope.tool import Toolkit


def kubectl_get_pods(namespace: str = "default", label_selector: str = "") -> str:
    """获取指定命名空间的 Pod 列表。

    Args:
        namespace: Kubernetes 命名空间
        label_selector: 标签选择器，如 "app=nginx"

    Returns:
        Pod 列表信息
    """
    cmd = ["kubectl", "get", "pods", "-n", namespace, "-o", "wide"]
    if label_selector:
        cmd.extend(["-l", label_selector])
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


def kubectl_describe_resource(
    resource_type: str,
    name: str,
    namespace: str = "default",
) -> str:
    """获取 Kubernetes 资源的详细信息和事件。

    Args:
        resource_type: 资源类型，如 "pod", "node", "service", "deployment"
        name: 资源名称
        namespace: 命名空间

    Returns:
        资源详细信息
    """
    cmd = ["kubectl", "describe", resource_type, name, "-n", namespace]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


def kubectl_get_events(
    namespace: str = "default",
    field_selector: str = "",
) -> str:
    """获取 Kubernetes 事件。

    Args:
        namespace: 命名空间
        field_selector: 字段选择器，如 "involvedObject.name=nginx-pod"

    Returns:
        事件列表
    """
    cmd = ["kubectl", "get", "events", "-n", namespace, "--sort-by=.lastTimestamp"]
    if field_selector:
        cmd.extend(["--field-selector", field_selector])
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


def kubectl_get_logs(
    pod_name: str,
    namespace: str = "default",
    container: str = "",
    tail_lines: int = 100,
    previous: bool = False,
) -> str:
    """获取 Pod 容器日志。

    Args:
        pod_name: Pod 名称
        namespace: 命名空间
        container: 容器名称（多容器 Pod 时必填）
        tail_lines: 获取最后 N 行日志
        previous: 是否获取上一次容器的日志（CrashLoopBackOff 排查用）

    Returns:
        容器日志内容
    """
    cmd = ["kubectl", "logs", pod_name, "-n", namespace, f"--tail={tail_lines}"]
    if container:
        cmd.extend(["-c", container])
    if previous:
        cmd.append("--previous")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


def kubectl_top_nodes() -> str:
    """获取集群节点资源使用情况。

    Returns:
        节点 CPU/内存使用量
    """
    result = subprocess.run(
        ["kubectl", "top", "nodes"],
        capture_output=True, text=True, timeout=30,
    )
    return result.stdout if result.returncode == 0 else f"Error: {result.stderr}"


# 注册 K8s 工具集
def create_k8s_toolkit() -> Toolkit:
    """创建 K8s 运维工具集"""
    toolkit = Toolkit()
    toolkit.register_tool_function(kubectl_get_pods)
    toolkit.register_tool_function(kubectl_describe_resource)
    toolkit.register_tool_function(kubectl_get_events)
    toolkit.register_tool_function(kubectl_get_logs)
    toolkit.register_tool_function(kubectl_top_nodes)
    return toolkit
```

#<!-- chunk: 7.2 完整 K8s 诊断 Agent -->## 7.2 完整 K8s 诊断 Agent

```python
import asyncio
import os
from agentscope.agent import ReActAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg


async def k8s_diagnosis_agent():
    toolkit = create_k8s_toolkit()

    agent = ReActAgent(
        name="K8s-Doctor",
        sys_prompt="""你是一个 Kubernetes 生产运维诊断专家。

诊断原则:
1. 先通过 kubectl 命令收集足够信息，再下结论
2. 对每个工具返回的结果进行分析
3. 给出: 根因分析 + 修复步骤 + 验证方法
4. 对破坏性操作明确标注风险等级
5. 所有结论必须基于工具获取的实际数据，禁止猜测

诊断顺序建议:
- Pod 问题: get_pods → describe → logs → events
- Node 问题: top_nodes → describe → events
- Service 问题: get_pods (label) → describe → events""",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        toolkit=toolkit,
        parallel_tool_calls=True,
        max_iters=15,
    )

    msg = Msg(
        name="user",
        content="production 命名空间的 nginx-deploy Pod 一直处于 Pending 状态，请诊断",
        role="user",
    )

    response = await agent(msg)
    print(f"\n诊断结果:\n{response.get_text_content()}")


asyncio.run(k8s_diagnosis_agent())
```

---

<!-- chunk: 11. 工具开发最佳实践 -->## 11. 工具开发最佳实践

#<!-- chunk: 8.1 编写高质量工具函数 -->## 8.1 编写高质量工具函数

```python
# 最佳实践: 清晰的 docstring + type hints + 错误处理

def query_prometheus_metric(
    metric_name: str,
    label_selector: str = "",
    duration: str = "5m",
    step: str = "15s",
) -> str:
    """查询 Prometheus 监控指标。

    适用场景: 查询集群或 Pod 级别的 CPU、内存、网络等监控数据。

    Args:
        metric_name: PromQL 指标名称，如 "container_cpu_usage_seconds_total"
        label_selector: 标签过滤器，如 'namespace="production",pod=~"nginx.*"'
        duration: 查询时间范围，如 "5m", "1h", "24h"
        step: 采样步长，如 "15s", "1m"

    Returns:
        JSON 格式的查询结果。失败时返回错误信息。

    Example:
        query_prometheus_metric(
            metric_name="container_memory_usage_bytes",
            label_selector='namespace="production"',
            duration="1h",
        )
    """
    import requests

    try:
        query = metric_name
        if label_selector:
            query = f"{metric_name}{{{label_selector}}}"

        response = requests.get(
            "http://prometheus:9090/api/v1/query_range",
            params={
                "query": query,
                "start": f"now()-{duration}",
                "end": "now()",
                "step": step,
            },
            timeout=10,
        )
        response.raise_for_status()
        return response.json()
    except requests.Timeout:
        return "Error: Prometheus 查询超时（10s）"
    except requests.ConnectionError:
        return "Error: 无法连接 Prometheus（检查服务地址和网络）"
    except Exception as e:
        return f"Error: {type(e).__name__}: {e}"
```

#<!-- chunk: 8.2 工具设计原则 -->## 8.2 工具设计原则

| 原则 | 说明 | 反模式 |
|------|------|--------|
| **单一职责** | 每个工具做一件事 | 一个工具同时查询+修改+验证 |
| **清晰描述** | docstring 说明用途、参数、返回值 | 无文档或描述模糊 |
| **类型标注** | 所有参数和返回值使用 type hints | `def tool(x, y)` 无类型 |
| **错误处理** | 捕获异常返回错误信息 | 异常直接抛出导致 Agent 循环中断 |
| **超时控制** | 网络调用设置 timeout | 无超时导致 Agent 挂起 |
| **只读优先** | 诊断类工具只读，修改类工具分离 | 查询工具附带副作用 |
| **工具数量** | 单 Agent 工具 ≤20 个 | 注册 50+ 工具导致选择准确率下降 |

---

<!-- chunk: 12. 最佳实践与反模式 -->## 12. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **返回 `ToolResponse`**：统一使用 `ToolResponse(text=...)` 而非纯字符串，支持多模态返回
- **`preset_kwargs` 隐藏敏感参数**：API Key、数据库密码等通过 `preset_kwargs` 传入，不暴露给 LLM
- **Docstring 决定工具质量**：LLM 通过 docstring 理解工具，描述越精准，调用越准确
- **type hints 必不可少**：AgentScope 依赖类型标注生成工具 Schema
- **利用偏函数简化工具**：`partial(kubectl, verb="get")` 比注册一个通用 kubectl 更清晰
- **`register_mcp_client` 优于手动遍历**：一键注册比手动 list_tools 遍历更简洁
- **Middleware 实现横切关注点**：权限控制、输出截断、日志记录用中间件而非写在工具内部
- **并行调用加速诊断**：独立的信息收集任务开启 `parallel_tool_calls=True`

#<!-- chunk: 反模式 -->## 反模式

- **无 docstring 的工具**：LLM 无法理解工具用途，随机调用
- **工具返回过大数据**：返回完整 YAML（10000+行）会占满上下文窗口——用 Middleware 截断
- **不处理错误**：工具异常导致 Agent Loop 中断
- **生产环境直接执行代码**：`execute_python_code` 必须在沙箱中运行
- **混合读写工具**：诊断 Agent 不应有 `kubectl delete` 权限——用 Middleware 拦截
- **忽略工具中断处理**：不捕获 `CancelledError` 导致用户中断时丢失已完成的部分结果

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [17 - 核心概念](./17-agentscope-core-concepts.md) | Tool 在核心抽象中的位置 |
| [19 - 记忆管理](./19-agentscope-memory-context.md) | 工具输出的记忆存储与上下文管理 |
| [22 - 生产部署](./22-agentscope-production-deployment.md) | Sandbox 安全执行环境 |
| [05 - Tool Use & Function Calling](./05-tool-use-function-calling.md) | 通用工具调用设计规范 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

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

- 16-agentscope-overview-installation
- 17-agentscope-core-concepts
- 19-agentscope-memory-context
- 20-agentscope-multi-agent-orchestration

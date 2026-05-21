---
title: Tool Use & Function Calling 设计规范
description: 'title: Tool Use & Function Calling 设计规范'
category: general
tags:
- ai
- ai-agent
- statefulset
- networkpolicy
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
- Tool Use & Function Calling 设计规范 是什么
- 如何 Tool Use & Function Calling 设计规范
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Tool
- Use
- Function
- Calling
- 设计规范
- ai
- ml
- infra
prerequisites:
- kubectl-basics
---

title: Tool Use & Function Calling 设计规范
description: '# Tool Use & Function Calling 设计规范'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- statefulset
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Tool Use & Function Calling 设计规范 是什么
- 如何 Tool Use & Function Calling 设计规范
trigger_keywords:
- Tool
- Use
- Function
- Calling
- 设计规范
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

# Tool Use & Function Calling 设计规范

> **文档类型**: 工程规范专题 | **最后更新**: 2026-03 | **关键词**: Function Calling, Tool Use, 工具调用, 并行调用, 错误恢复, 工具链设计, OpenAI Tools, Anthropic Tool Use, 工具定义规范

---

<!-- chunk: 概述 -->## 概述

Tool Use（工具调用）是 AI Agent 从"语言理解者"变为"行动执行者"的关键能力。高质量的工具定义、合理的工具链设计和健壮的错误处理，直接决定 Agent 的可靠性和执行效率。本文从工具定义规范、并行调用策略、错误恢复模式到生产级工具链设计，提供完整的工程指南。

---

<!-- chunk: 1. Function Calling 核心机制 -->## 1. Function Calling 核心机制

#<!-- chunk: 1.1 OpenAI Function Calling 协议 -->## 1.1 OpenAI Function Calling 协议

OpenAI 工具调用的完整生命周期：

```
用户消息
  │
  ▼
LLM 决策（是否需要调用工具）
  │
  ├── 不需要 → 直接生成文本回复
  │
  └── 需要 → 生成 tool_calls（包含工具名和参数）
              │
              ▼
         应用层执行工具
              │
              ▼
         将结果作为 tool 消息返回给 LLM
              │
              ▼
         LLM 基于工具结果生成最终回复
```

```python
from openai import OpenAI
import json

client = OpenAI()

# 1. 定义工具规格（JSON Schema）
tools = [
    {
        "type": "function",
        "function": {
            "name": "kubectl_get_pod_status",
            "description": """获取指定 Pod 的详细状态信息，包括：
            - Pod Phase（Running/Pending/Failed/Succeeded）
            - 容器状态和重启次数
            - 相关事件（Events）
            - 调度信息
            
            适用场景：诊断 Pod 异常、确认 Pod 是否正常运行""",
            "parameters": {
                "type": "object",
                "properties": {
                    "pod_name": {
                        "type": "string",
                        "description": "Pod 名称，例如 nginx-deploy-7d9b-xyz"
                    },
                    "namespace": {
                        "type": "string",
                        "description": "命名空间，默认为 default",
                        "default": "default"
                    },
                    "include_events": {
                        "type": "boolean",
                        "description": "是否包含 Events 信息，默认为 true",
                        "default": True
                    }
                },
                "required": ["pod_name"]
            }
        }
    },
]

# 2. 发送请求
messages = [
    {"role": "user", "content": "检查 production 命名空间的 api-server-xxx Pod 状态"}
]

response = client.chat.completions.create(
    model="gpt-4o",
    messages=messages,
    tools=tools,
    tool_choice="auto",  # "auto" | "required" | "none" | 指定工具
)

# 3. 处理工具调用响应
message = response.choices[0].message

if message.tool_calls:
    # 执行工具
    for tool_call in message.tool_calls:
        func_name = tool_call.function.name
        func_args = json.loads(tool_call.function.arguments)
        
        # 调用对应的工具函数
        tool_result = execute_tool(func_name, func_args)
        
        # 将工具结果加入消息历史
        messages.append(message)  # 包含 tool_calls 的 assistant 消息
        messages.append({
            "role": "tool",
            "content": json.dumps(tool_result, ensure_ascii=False),
            "tool_call_id": tool_call.id,
        })
    
    # 让 LLM 基于工具结果生成最终回复
    final_response = client.chat.completions.create(
        model="gpt-4o",
        messages=messages,
    )
    print(final_response.choices[0].message.content)
```

#<!-- chunk: 1.2 Anthropic Tool Use 协议 -->## 1.2 Anthropic Tool Use 协议

```python
import anthropic

client = anthropic.Anthropic()

tools = [
    {
        "name": "kubectl_describe",
        "description": "获取 K8s 资源的详细描述，包含状态、事件和配置",
        "input_schema": {
            "type": "object",
            "properties": {
                "resource_type": {
                    "type": "string",
                    "enum": ["pod", "deployment", "service", "node", "pvc"],
                    "description": "K8s 资源类型"
                },
                "name": {"type": "string", "description": "资源名称"},
                "namespace": {"type": "string", "default": "default"},
            },
            "required": ["resource_type", "name"]
        }
    }
]

response = client.messages.create(
    model="claude-3-5-sonnet-20241022",
    max_tokens=4096,
    tools=tools,
    messages=[
        {"role": "user", "content": "诊断 Pod nginx-xxx 的问题"}
    ]
)

# Claude 的工具调用在 content 块中
for block in response.content:
    if block.type == "tool_use":
        tool_name = block.name
        tool_input = block.input
        tool_use_id = block.id
        
        # 执行工具
        result = execute_tool(tool_name, tool_input)
        
        # 返回结果
        follow_up = client.messages.create(
            model="claude-3-5-sonnet-20241022",
            max_tokens=4096,
            tools=tools,
            messages=[
                {"role": "user", "content": "诊断 Pod nginx-xxx 的问题"},
                {"role": "assistant", "content": response.content},
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "tool_result",
                            "tool_use_id": tool_use_id,
                            "content": str(result),
                        }
                    ]
                }
            ]
        )
```

---

<!-- chunk: 2. 工具定义最佳规范 -->## 2. 工具定义最佳规范

#<!-- chunk: 2.1 工具描述质量准则 -->## 2.1 工具描述质量准则

工具 description 是 LLM 决定是否调用该工具的唯一依据，质量至关重要：

```python
# 糟糕的工具描述（避免）
bad_tool = {
    "name": "get_info",
    "description": "获取信息",  # 太模糊，LLM 不知道何时用
    "parameters": {
        "type": "object",
        "properties": {
            "input": {"type": "string"}  # 参数名不明确
        }
    }
}

# 优秀的工具描述（推荐）
good_tool = {
    "name": "kubectl_get_pod_logs",
    "description": """获取 Pod 容器的运行日志，用于诊断应用错误、崩溃原因和运行异常。
    
    适用场景：
    - Pod 处于 CrashLoopBackOff 状态时查看错误日志
    - 应用报错时查看详细堆栈
    - 查看启动失败的容器日志
    
    注意：
    - 默认返回最近 100 行，日志量大时请指定 tail_lines
    - 多容器 Pod 必须指定 container_name
    - 已 Completed 的 Pod 可以用 --previous 查看上一个容器实例的日志""",
    "parameters": {
        "type": "object",
        "properties": {
            "pod_name": {
                "type": "string",
                "description": "Pod 名称（不含命名空间）"
            },
            "namespace": {
                "type": "string",
                "description": "命名空间，默认 default",
                "default": "default"
            },
            "container_name": {
                "type": "string",
                "description": "容器名称（Pod 包含多容器时必须指定）"
            },
            "tail_lines": {
                "type": "integer",
                "description": "返回最后 N 行日志，默认 100，最大 1000",
                "default": 100,
                "minimum": 1,
                "maximum": 1000
            },
            "previous": {
                "type": "boolean",
                "description": "是否查看上一个容器实例的日志（用于已崩溃容器）",
                "default": False
            }
        },
        "required": ["pod_name"]
    }
}
```

#<!-- chunk: 2.2 工具粒度设计原则 -->## 2.2 工具粒度设计原则

```
工具粒度设计三原则:

1. 单一职责：每个工具只做一件事
   ✅ kubectl_get_pod_status（只获取 Pod 状态）
   ❌ kubectl_diagnose（诊断所有问题，太宽泛）

2. 正交性：工具间功能不重叠
   ✅ kubectl_get_nodes + kubectl_describe_node（分别列举和详情）
   ❌ get_all_info（返回所有信息，LLM 难以选择）

3. 适当粒度：太细导致工具数量爆炸，太粗影响复用性
   适中: 10-20 个核心工具 + 按需扩展
   避免: 超过 30 个工具（LLM 工具选择准确率显著下降）
```

#<!-- chunk: 2.3 K8s 运维工具集设计 -->## 2.3 K8s 运维工具集设计

```python
K8S_OPS_TOOLS = {
    # 信息收集类（只读）
    "cluster_overview": "获取集群整体状态概览（节点数、Pod 数、资源使用率）",
    "get_resource": "获取指定类型的 K8s 资源列表（带过滤条件）",
    "describe_resource": "获取单个 K8s 资源的详细描述和事件",
    "get_pod_logs": "获取 Pod 容器日志",
    "get_events": "获取命名空间或特定资源的事件历史",
    "get_metrics": "获取 Pod/Node 的 CPU/内存实时指标",
    "exec_command": "在 Pod 容器内执行命令（只读命令）",
    
    # 网络诊断类（只读）
    "test_connectivity": "测试 Pod 间或 Service 的网络连通性",
    "get_dns_resolution": "测试 DNS 解析结果",
    "get_network_policy": "查看影响特定 Pod 的 NetworkPolicy",
    
    # 变更操作类（需审批）
    "scale_deployment": "调整 Deployment 副本数",
    "rollout_restart": "触发 Deployment/StatefulSet 滚动重启",
    "apply_patch": "对资源应用 JSON/YAML patch",
    "update_configmap": "更新 ConfigMap 内容",
    
    # 高风险操作类（需双重审批）
    "delete_resource": "删除 K8s 资源",
    "drain_node": "驱逐节点上的所有 Pod（节点维护）",
    "force_delete_pod": "强制删除 Pod（慎用）",
}

# 按风险级别分组，自动应用审批策略
RISK_GROUPS = {
    "readonly": ["cluster_overview", "get_resource", "describe_resource",
                 "get_pod_logs", "get_events", "get_metrics", "exec_command",
                 "test_connectivity", "get_dns_resolution", "get_network_policy"],
    "low_risk": ["scale_deployment", "rollout_restart"],
    "medium_risk": ["apply_patch", "update_configmap"],
    "high_risk": ["delete_resource", "drain_node", "force_delete_pod"],
}
```

---

<!-- chunk: 3. 并行工具调用 -->## 3. 并行工具调用

现代 LLM（GPT-4o、Claude 3.5）支持在一次请求中并行调用多个工具，显著提升效率：

```python
import asyncio
from concurrent.futures import ThreadPoolExecutor

class ParallelToolExecutor:
    """并行执行多个工具调用"""
    
    def __init__(self, max_workers: int = 5):
        self.executor = ThreadPoolExecutor(max_workers=max_workers)
    
    async def execute_parallel(
        self, 
        tool_calls: list[dict]
    ) -> list[dict]:
        """异步并行执行所有工具调用"""
        
        # 识别可以并行的工具（只读工具总是可以并行）
        parallel_calls = []
        sequential_calls = []
        
        for tc in tool_calls:
            risk = RISK_GROUPS.get(tc["function"]["name"], "high_risk")
            if risk in ["readonly", "low_risk"]:
                parallel_calls.append(tc)
            else:
                sequential_calls.append(tc)
        
        results = {}
        
        # 并行执行只读工具
        if parallel_calls:
            tasks = [
                self._async_execute(tc) for tc in parallel_calls
            ]
            parallel_results = await asyncio.gather(*tasks)
            for tc, result in zip(parallel_calls, parallel_results):
                results[tc["id"]] = result
        
        # 顺序执行有副作用的工具（需要人工审批）
        for tc in sequential_calls:
            approved = await self._request_approval(tc)
            if approved:
                result = await self._async_execute(tc)
                results[tc["id"]] = result
            else:
                results[tc["id"]] = {"error": "操作被拒绝"}
        
        return results
    
    async def _async_execute(self, tool_call: dict) -> dict:
        """异步执行单个工具"""
        loop = asyncio.get_event_loop()
        func_name = tool_call["function"]["name"]
        func_args = json.loads(tool_call["function"]["arguments"])
        
        try:
            result = await loop.run_in_executor(
                self.executor,
                lambda: execute_tool(func_name, func_args)
            )
            return {"success": True, "result": result}
        except Exception as e:
            return {"success": False, "error": str(e), "error_type": type(e).__name__}

# 使用示例
executor = ParallelToolExecutor()

# LLM 返回了多个并行工具调用
tool_calls = [
    {"id": "tc1", "function": {"name": "get_pod_logs", "arguments": '{"pod_name": "api-xxx"}'}},
    {"id": "tc2", "function": {"name": "describe_resource", "arguments": '{"resource": "pod/api-xxx"}'}},
    {"id": "tc3", "function": {"name": "get_events", "arguments": '{"namespace": "prod"}'}},
]

# 同时执行三个工具，耗时 = max(单个工具耗时)，而非 sum
results = asyncio.run(executor.execute_parallel(tool_calls))
```

#<!-- chunk: 3.1 并行调用中的依赖关系处理 -->## 3.1 并行调用中的依赖关系处理

```python
class DependencyAwareExecutor:
    """处理工具调用间的依赖关系"""
    
    def build_execution_graph(self, tool_calls: list) -> dict:
        """构建工具调用的执行依赖图"""
        # 某些工具的参数依赖于前序工具的结果
        # 例如：先 get_pod_name，再 get_pod_logs(pod_name=<前序结果>)
        
        graph = {}
        for tc in tool_calls:
            deps = self._extract_dependencies(tc)
            graph[tc["id"]] = {
                "tool_call": tc,
                "dependencies": deps,
                "status": "pending"
            }
        return graph
    
    async def execute_with_dependencies(self, graph: dict) -> dict:
        """按依赖顺序执行（拓扑排序 + 并行）"""
        results = {}
        
        while any(v["status"] == "pending" for v in graph.values()):
            # 找到所有依赖已满足的待执行节点
            ready = [
                node_id for node_id, node in graph.items()
                if node["status"] == "pending" 
                and all(graph[dep]["status"] == "complete" 
                       for dep in node["dependencies"])
            ]
            
            if not ready:
                break  # 循环依赖检测
            
            # 并行执行所有就绪节点
            batch_results = await asyncio.gather(*[
                self._execute_node(graph[node_id], results)
                for node_id in ready
            ])
            
            for node_id, result in zip(ready, batch_results):
                results[node_id] = result
                graph[node_id]["status"] = "complete"
        
        return results
```

---

<!-- chunk: 4. 错误恢复模式 -->## 4. 错误恢复模式

#<!-- chunk: 4.1 工具错误分类与处理策略 -->## 4.1 工具错误分类与处理策略

```python
from enum import Enum

class ToolErrorType(Enum):
    TRANSIENT = "transient"        # 临时性错误，可重试
    PERMISSION = "permission"      # 权限错误，需升级处理
    NOT_FOUND = "not_found"        # 资源不存在，需修正参数
    INVALID_INPUT = "invalid_input"# 参数无效，需修正
    TIMEOUT = "timeout"            # 超时，可重试
    RATE_LIMIT = "rate_limit"      # 速率限制，等待后重试
    FATAL = "fatal"                # 致命错误，终止

ERROR_HANDLING_STRATEGY = {
    ToolErrorType.TRANSIENT: {"retry": True, "max_retries": 3, "backoff": "exponential"},
    ToolErrorType.TIMEOUT: {"retry": True, "max_retries": 2, "backoff": "linear", "wait": 5},
    ToolErrorType.RATE_LIMIT: {"retry": True, "max_retries": 5, "backoff": "exponential", "base_wait": 10},
    ToolErrorType.NOT_FOUND: {"retry": False, "llm_feedback": "资源不存在，请检查名称和命名空间"},
    ToolErrorType.PERMISSION: {"retry": False, "llm_feedback": "权限不足，需要管理员授权"},
    ToolErrorType.INVALID_INPUT: {"retry": False, "llm_feedback": "参数无效，请修正后重试"},
    ToolErrorType.FATAL: {"retry": False, "escalate": True},
}

class ResilientToolCaller:
    def call(self, tool_name: str, args: dict) -> tuple[bool, any]:
        """带错误恢复的工具调用"""
        strategy = None
        
        for attempt in range(10):  # 最大尝试次数
            try:
                result = execute_tool(tool_name, args)
                return True, result
            
            except subprocess.TimeoutExpired:
                error_type = ToolErrorType.TIMEOUT
            except PermissionError:
                error_type = ToolErrorType.PERMISSION
            except FileNotFoundError:
                error_type = ToolErrorType.NOT_FOUND
            except ValueError:
                error_type = ToolErrorType.INVALID_INPUT
            except Exception as e:
                error_type = self._classify_error(e)
            
            strategy = ERROR_HANDLING_STRATEGY[error_type]
            
            if not strategy["retry"] or attempt >= strategy.get("max_retries", 1) - 1:
                # 不可重试或已超过重试次数
                feedback = strategy.get("llm_feedback", f"工具调用失败: {error_type.value}")
                return False, {"error": feedback, "error_type": error_type.value}
            
            # 计算等待时间
            wait = self._calculate_wait(strategy, attempt)
            time.sleep(wait)
        
        return False, {"error": "达到最大重试次数"}
    
    def _calculate_wait(self, strategy: dict, attempt: int) -> float:
        base = strategy.get("base_wait", 1.0)
        backoff_type = strategy.get("backoff", "exponential")
        
        if backoff_type == "exponential":
            return base * (2 ** attempt)
        elif backoff_type == "linear":
            return base * (attempt + 1)
        return base
```

#<!-- chunk: 4.2 工具调用失败时的 LLM 反馈处理 -->## 4.2 工具调用失败时的 LLM 反馈处理

当工具失败后，需要给 LLM 有意义的错误信息，让其决定如何处理：

```python
def format_tool_error_for_llm(
    tool_name: str,
    error_type: str,
    error_message: str,
    args_used: dict,
) -> str:
    """将工具错误格式化为 LLM 可理解的反馈"""
    
    error_guidance = {
        "not_found": f"""工具 {tool_name} 执行失败：资源不存在
        使用的参数: {args_used}
        建议：
        1. 检查资源名称是否正确（区分大小写）
        2. 确认命名空间是否正确
        3. 先用 get_resource 列出所有资源再确认名称""",
        
        "permission": f"""工具 {tool_name} 执行失败：权限不足
        当前 Agent 没有执行此操作的权限。
        此操作需要升级处理，请告知用户需要管理员手动执行：
        {error_message}""",
        
        "timeout": f"""工具 {tool_name} 执行超时
        可能原因：集群负载高或网络延迟
        建议：稍后重试，或先检查集群整体健康状态""",
    }
    
    return error_guidance.get(error_type, f"工具 {tool_name} 执行失败: {error_message}")
```

---

<!-- chunk: 5. 工具链设计模式 -->## 5. 工具链设计模式

#<!-- chunk: 5.1 诊断工具链（顺序依赖） -->## 5.1 诊断工具链（顺序依赖）

```python
class K8sDiagnosisToolChain:
    """标准化的 K8s 诊断工具链"""
    
    DIAGNOSIS_STEPS = [
        {
            "step": "overview",
            "tool": "cluster_overview",
            "description": "获取集群概览",
            "args": {},
            "required": True,
        },
        {
            "step": "pod_status",
            "tool": "describe_resource",
            "description": "获取 Pod 详细状态",
            "args": {"resource_type": "{resource_type}", "name": "{resource_name}"},
            "required": True,
        },
        {
            "step": "events",
            "tool": "get_events",
            "description": "获取相关事件",
            "args": {"namespace": "{namespace}", "involved_object": "{resource_name}"},
            "required": True,
        },
        {
            "step": "logs",
            "tool": "get_pod_logs",
            "description": "获取 Pod 日志",
            "args": {"pod_name": "{resource_name}", "tail_lines": 100},
            "required": False,  # 可选步骤，根据前序结果决定
            "condition": "pod_in_error_state",
        },
    ]
    
    def build_context_for_llm(self, problem: str, results: dict) -> str:
        """将工具执行结果整理为 LLM 友好的上下文"""
        context_parts = [f"<!-- chunk: 诊断目标\n{problem}\n"] -->## 诊断目标\n{problem}\n"]
        
        for step, result in results.items():
            if result.get("success"):
                context_parts.append(f"<!-- chunk: {step}\n```\n{result['data']}\n```\n") -->## {step}\n```\n{result['data']}\n```\n")
            else:
                context_parts.append(f"<!-- chunk: {step}\n[获取失败: {result['error']}]\n") -->## {step}\n[获取失败: {result['error']}]\n")
        
        return "\n".join(context_parts)
```

#<!-- chunk: 5.2 自适应工具调用（Agent 自主决策） -->## 5.2 自适应工具调用（Agent 自主决策）

```python
ADAPTIVE_TOOL_SELECTION_PROMPT = """
你是 K8s 诊断专家。根据当前收集到的信息，决定下一步需要调用哪些工具。

当前已知信息:
{collected_info}

可用工具:
{available_tools}

诊断原则:
1. 从最基础的信息开始（Pod 状态 → 事件 → 日志）
2. 根据发现的问题针对性深入（网络问题 → 网络诊断工具）
3. 避免重复调用已有信息
4. 当信息已足够诊断根因时，停止收集，直接分析

请选择下一步要调用的工具（可以多个并行），并解释理由。
如果信息已足够，输出 DIAGNOSE_NOW。
"""

def adaptive_diagnosis(
    problem: str,
    llm,
    available_tools: list,
    max_rounds: int = 5
) -> dict:
    """自适应诊断：Agent 自主决定工具调用顺序"""
    collected_info = {}
    tool_call_history = []
    
    for round_num in range(max_rounds):
        # 询问 LLM 下一步该调用哪些工具
        decision = llm.invoke(ADAPTIVE_TOOL_SELECTION_PROMPT.format(
            collected_info=format_collected_info(collected_info),
            available_tools=format_tools(available_tools),
        ))
        
        if "DIAGNOSE_NOW" in decision.content:
            break
        
        # 解析工具调用决策
        selected_tools = parse_tool_selection(decision.content)
        
        # 并行执行选中的工具
        results = execute_tools_parallel(selected_tools)
        collected_info.update(results)
        tool_call_history.extend(selected_tools)
    
    # 最终诊断
    diagnosis = llm.invoke(f"""
    问题: {problem}
    收集到的信息: {collected_info}
    
    请给出完整的根因分析和修复建议。
    """)
    
    return {
        "diagnosis": diagnosis.content,
        "tool_calls": tool_call_history,
        "data_collected": collected_info,
    }
```

---

<!-- chunk: 6. 工具安全设计 -->## 6. 工具安全设计

#<!-- chunk: 6.1 输入验证与净化 -->## 6.1 输入验证与净化

```python
import re
from typing import Any

class ToolInputValidator:
    """工具输入安全验证"""
    
    # 危险命令模式（防止命令注入）
    DANGEROUS_PATTERNS = [
        r';\s*rm\s',
        r'&&\s*rm\s',
        r'\|\s*sh\s',
        r'>\s*/etc/',
        r'curl\s+.*\s*\|\s*sh',
        r'wget\s+.*\s*-O\s*-\s*\|\s*sh',
        r'\$\(',      # 命令替换
        r'`.*`',      # 反引号命令替换
    ]
    
    K8S_NAME_PATTERN = re.compile(r'^[a-z0-9][a-z0-9\-]*[a-z0-9]$')
    NAMESPACE_PATTERN = re.compile(r'^[a-z0-9][a-z0-9\-]*$')
    
    def validate_kubectl_args(self, args: dict) -> tuple[bool, str]:
        """验证 kubectl 相关工具的参数"""
        
        # 验证资源名称格式
        if "name" in args:
            if not self.K8S_NAME_PATTERN.match(args["name"]):
                return False, f"无效的资源名称格式: {args['name']}"
        
        # 验证命名空间格式
        if "namespace" in args:
            if not self.NAMESPACE_PATTERN.match(args["namespace"]):
                return False, f"无效的命名空间格式: {args['namespace']}"
        
        # 验证命令参数中没有注入
        if "command" in args:
            for pattern in self.DANGEROUS_PATTERNS:
                if re.search(pattern, args["command"], re.IGNORECASE):
                    return False, f"检测到潜在危险命令模式，拒绝执行"
        
        return True, "OK"
    
    def sanitize_output(self, output: str, max_length: int = 10000) -> str:
        """净化工具输出，防止 Prompt Injection"""
        # 截断过长输出
        if len(output) > max_length:
            output = output[:max_length] + "\n[... 输出已截断，显示前 10000 字符 ...]"
        
        # 移除可能的提示词注入
        injection_patterns = [
            r'SYSTEM:.*',
            r'Ignore previous instructions.*',
            r'You are now.*',
        ]
        for pattern in injection_patterns:
            output = re.sub(pattern, '[已过滤]', output, flags=re.IGNORECASE)
        
        return output
```

#<!-- chunk: 6.2 工具调用审计日志 -->## 6.2 工具调用审计日志

```python
import structlog
from datetime import datetime, UTC
from dataclasses import dataclass, asdict

@dataclass
class ToolCallAuditLog:
    trace_id: str
    session_id: str
    user_id: str
    tool_name: str
    args: dict
    result_summary: str  # 不记录完整结果（可能包含敏感信息）
    success: bool
    error_type: str
    duration_ms: float
    timestamp: str
    risk_level: str
    approved_by: str = ""  # 人工审批时填写

def audit_tool_call(tool_name: str, args: dict, user_id: str):
    """装饰器：自动记录工具调用审计日志"""
    def decorator(func):
        def wrapper(*call_args, **call_kwargs):
            start = time.time()
            trace_id = generate_trace_id()
            
            try:
                result = func(*call_args, **call_kwargs)
                success = True
                error_type = ""
                result_summary = str(result)[:200]  # 只记录前 200 字符
            except Exception as e:
                success = False
                error_type = type(e).__name__
                result_summary = str(e)
                raise
            finally:
                log = ToolCallAuditLog(
                    trace_id=trace_id,
                    session_id=get_current_session_id(),
                    user_id=user_id,
                    tool_name=tool_name,
                    args={k: mask_sensitive(v) for k, v in args.items()},
                    result_summary=result_summary,
                    success=success,
                    error_type=error_type,
                    duration_ms=(time.time() - start) * 1000,
                    timestamp=datetime.now(UTC).isoformat(),
                    risk_level=get_risk_level(tool_name),
                )
                structlog.get_logger().info("tool_call_audit", **asdict(log))
        
        return wrapper
    return decorator
```

---

<!-- chunk: 7. 最佳实践与反模式 -->## 7. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **工具描述要具体**：说明适用场景、不适用场景和注意事项，比说明功能更重要
- **参数使用 enum 约束**：有限选项的参数用 enum 而非 string，减少 LLM 猜测
- **工具数量控制在 20 以内**：超过 20 个工具后，LLM 工具选择准确率明显下降
- **并行调用只读操作**：显著降低延迟，只对有状态更改的操作强制串行
- **失败要给有意义的反馈**：告诉 LLM 失败原因和建议，让其能够自动纠正

#<!-- chunk: 反模式 -->## 反模式

- **工具返回过多信息**：返回 10000 行日志给 LLM，超出上下文窗口且推理效率低
- **工具名和功能不对应**：`execute_operation` 这样的名字让 LLM 无法正确选择
- **不处理工具超时**：kubectl 命令在集群异常时可能挂起，必须设置 timeout
- **相信工具输出中的所有内容**：攻击者可以在 Pod 日志中注入 Prompt，需要净化
- **忽略并行调用机会**：诊断场景中的多个只读查询完全可以并行，串行执行浪费 3-5x 时间

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | Agent Loop 中工具调用的位置 |
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | LangChain/LlamaIndex 工具封装 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 工具在多 Agent 间的共享 |
| [10 - 安全护栏](./[[domain-14-ai-ml-infra/10-security-guardrails.md|10-security-guardrails]].md) | 工具调用的安全策略 |
| [domain-10-troubleshooting-diagnostics](../domain-10-troubleshooting-diagnostics/) | K8s 运维工具对应的知识库 |

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
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization.md|成本与延迟优化策略]]

## See Also

- [[domain-14-ai-ml-infra/03-agent-frameworks-comparison.md|03-agent-frameworks-comparison]]
- [[domain-14-ai-ml-infra/04-rag-knowledge-retrieval.md|04-rag-knowledge-retrieval]]
- [[domain-14-ai-ml-infra/06-multi-agent-orchestration.md|06-multi-agent-orchestration]]
- [[domain-14-ai-ml-infra/07-memory-context-management.md|07-memory-context-management]]

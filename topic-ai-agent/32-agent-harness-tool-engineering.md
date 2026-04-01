# Agent Harness 工具工程：从设计到精简的完整实践

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Tool Engineering, 工具设计, Function Calling, 工具精简, 工具编排, MCP, 工具安全, Schema 设计, 工具注册, 工具发现

---

## 概述

Tools（工具层）是 Agent Harness 六层架构的第二层，让 Agent 从"只能说"变为"能做事"。但工具设计绝非"越多越好"——Vercel 的实证表明，将 15 个工具精简为 2 个后，准确率从 80% 跃升至 100%。

本文系统性地阐述工具层的设计原则、Schema 规范、注册发现机制、工具编排模式、安全沙箱、错误恢复策略，以及 K8S 运维场景中的工具工程最佳实践。

---

## 1. 工具设计原则

### 1.1 Less is More：精简的力量

```
工具精简的业务价值:

工具数量与决策质量的关系（实证数据）:
  2  个工具 → 决策准确率 ~100%（Vercel 实证）
  5  个工具 → 决策准确率 ~95%
  10 个工具 → 决策准确率 ~85%
  15 个工具 → 决策准确率 ~80%（Vercel 实证）
  25 个工具 → 决策准确率 ~65%
  50 个工具 → 决策准确率 ~45%

原因分析:
  1. 工具越多，LLM 需要阅读的 Schema 越长 → 占用上下文窗口
  2. 相似功能的工具导致 LLM 选择困难 → "read_file vs search_file vs grep_file"
  3. 工具描述的细微差别被忽略 → 调用错误工具
  4. 更多工具 = 更多参数组合 = 更多错误可能
```

### 1.2 工具设计六大原则

| 原则 | 说明 | 实践指南 |
|------|------|---------|
| **最小必要** | 只提供完成当前任务必需的工具 | 动态工具集：根据任务类型加载不同工具 |
| **无歧义** | 每个工具的用途必须唯一明确 | 工具名和描述不能让 LLM 混淆 |
| **自解释** | Schema 本身就是完整的使用文档 | 参数描述包含示例和约束 |
| **安全优先** | 工具执行不应产生不可逆后果 | 危险操作需要确认机制 |
| **幂等性** | 相同输入产生相同结果 | 避免工具有隐含的副作用 |
| **错误友好** | 失败时返回有意义的错误信息 | 帮助 Agent 理解为什么失败、如何修复 |

---

## 2. 工具 Schema 设计规范

### 2.1 标准工具接口

```python
from abc import ABC, abstractmethod
from typing import Any, Optional
from dataclasses import dataclass, field
import json

@dataclass
class ToolParameter:
    """工具参数定义"""
    name: str
    type: str                           # string, integer, boolean, array, object
    description: str
    required: bool = True
    default: Any = None
    enum: list = None                   # 可选值枚举
    example: Any = None                 # 示例值
    pattern: str = None                 # 正则验证
    min_value: Any = None
    max_value: Any = None

@dataclass
class ToolSchema:
    """工具 Schema 完整定义"""
    name: str
    description: str                    # 一句话描述（LLM 选择工具的依据）
    long_description: str = ""          # 详细说明
    parameters: list[ToolParameter] = field(default_factory=list)
    returns: str = ""                   # 返回值说明
    examples: list[dict] = field(default_factory=list)
    category: str = ""                  # 工具类别
    risk_level: str = "low"             # low / medium / high / critical
    idempotent: bool = True
    timeout_seconds: int = 30

    def to_openai_format(self) -> dict:
        """转换为 OpenAI Function Calling 格式"""
        properties = {}
        required = []
        for param in self.parameters:
            prop = {"type": param.type, "description": param.description}
            if param.enum:
                prop["enum"] = param.enum
            if param.example:
                prop["description"] += f" (例: {param.example})"
            properties[param.name] = prop
            if param.required:
                required.append(param.name)

        return {
            "type": "function",
            "function": {
                "name": self.name,
                "description": self.description,
                "parameters": {
                    "type": "object",
                    "properties": properties,
                    "required": required,
                },
            },
        }

class BaseTool(ABC):
    """工具基类"""

    @abstractmethod
    def get_schema(self) -> ToolSchema:
        """返回工具的 Schema 定义"""
        ...

    @abstractmethod
    def execute(self, **kwargs) -> dict:
        """执行工具"""
        ...

    def validate(self, **kwargs) -> tuple[bool, str]:
        """参数验证"""
        schema = self.get_schema()
        for param in schema.parameters:
            if param.required and param.name not in kwargs:
                return False, f"缺少必需参数: {param.name}"
            if param.name in kwargs and param.enum:
                if kwargs[param.name] not in param.enum:
                    return False, f"参数 {param.name} 的值必须是: {param.enum}"
        return True, "OK"
```

### 2.2 K8S 运维工具集设计

```python
class KubectlGetTool(BaseTool):
    """kubectl get 工具：获取 K8S 资源信息"""

    def get_schema(self) -> ToolSchema:
        return ToolSchema(
            name="kubectl_get",
            description="获取 Kubernetes 资源的列表或详细信息。用于查看 Pod、Node、Service 等资源状态。",
            parameters=[
                ToolParameter(
                    name="resource",
                    type="string",
                    description="资源类型",
                    enum=["pods", "nodes", "services", "deployments",
                          "events", "pvc", "configmaps", "ingress"],
                    example="pods",
                ),
                ToolParameter(
                    name="namespace",
                    type="string",
                    description="命名空间。使用 '--all-namespaces' 查看所有",
                    required=False,
                    default="default",
                    example="kube-system",
                ),
                ToolParameter(
                    name="name",
                    type="string",
                    description="资源名称。不指定则列出所有",
                    required=False,
                    example="nginx-deployment-7fb96c846b-xxxxx",
                ),
                ToolParameter(
                    name="output",
                    type="string",
                    description="输出格式",
                    required=False,
                    enum=["wide", "yaml", "json", "name"],
                    default="wide",
                ),
                ToolParameter(
                    name="selector",
                    type="string",
                    description="标签选择器",
                    required=False,
                    example="app=nginx",
                ),
            ],
            returns="资源信息的文本输出",
            risk_level="low",
            category="kubernetes",
            idempotent=True,
            timeout_seconds=15,
            examples=[
                {"args": {"resource": "pods", "namespace": "default"},
                 "description": "获取 default 命名空间的所有 Pod"},
                {"args": {"resource": "nodes", "output": "wide"},
                 "description": "获取所有节点详细信息"},
            ],
        )

    def execute(self, **kwargs) -> dict:
        resource = kwargs["resource"]
        namespace = kwargs.get("namespace", "default")
        name = kwargs.get("name", "")
        output = kwargs.get("output", "wide")
        selector = kwargs.get("selector", "")

        cmd = f"kubectl get {resource}"
        if name:
            cmd += f" {name}"
        if namespace == "--all-namespaces":
            cmd += " --all-namespaces"
        else:
            cmd += f" -n {namespace}"
        if output:
            cmd += f" -o {output}"
        if selector:
            cmd += f" -l {selector}"

        result = self._run_command(cmd)
        return {"command": cmd, "output": result["stdout"],
                "error": result.get("stderr"), "exit_code": result["exit_code"]}


class KubectlDescribeTool(BaseTool):
    """kubectl describe 工具：获取资源详细描述"""

    def get_schema(self) -> ToolSchema:
        return ToolSchema(
            name="kubectl_describe",
            description="获取 Kubernetes 资源的详细描述信息，包括 Events、Conditions、配置。用于诊断资源问题。",
            parameters=[
                ToolParameter(
                    name="resource",
                    type="string",
                    description="资源类型",
                    enum=["pod", "node", "service", "deployment",
                          "pvc", "ingress", "configmap"],
                    example="pod",
                ),
                ToolParameter(
                    name="name",
                    type="string",
                    description="资源名称",
                    example="nginx-pod-xxxxx",
                ),
                ToolParameter(
                    name="namespace",
                    type="string",
                    description="命名空间",
                    required=False,
                    default="default",
                ),
            ],
            returns="资源的详细描述文本，包含 Events 和 Conditions",
            risk_level="low",
            category="kubernetes",
            idempotent=True,
        )

    def execute(self, **kwargs) -> dict:
        resource = kwargs["resource"]
        name = kwargs["name"]
        namespace = kwargs.get("namespace", "default")
        cmd = f"kubectl describe {resource} {name} -n {namespace}"
        result = self._run_command(cmd)
        return {"command": cmd, "output": result["stdout"],
                "error": result.get("stderr"), "exit_code": result["exit_code"]}


class PrometheusQueryTool(BaseTool):
    """Prometheus 查询工具：执行 PromQL 获取监控指标"""

    def get_schema(self) -> ToolSchema:
        return ToolSchema(
            name="prometheus_query",
            description="执行 PromQL 查询获取监控指标数据。用于查看 CPU、内存、网络等系统指标。",
            parameters=[
                ToolParameter(
                    name="query",
                    type="string",
                    description="PromQL 查询表达式",
                    example='sum(rate(container_cpu_usage_seconds_total{namespace="default"}[5m]))',
                ),
                ToolParameter(
                    name="time_range",
                    type="string",
                    description="查询时间范围",
                    required=False,
                    enum=["5m", "15m", "1h", "6h", "24h"],
                    default="15m",
                ),
                ToolParameter(
                    name="step",
                    type="string",
                    description="数据点间隔",
                    required=False,
                    default="30s",
                ),
            ],
            returns="指标数据（JSON 格式）",
            risk_level="low",
            category="monitoring",
            timeout_seconds=30,
        )

    def execute(self, **kwargs) -> dict:
        query = kwargs["query"]
        time_range = kwargs.get("time_range", "15m")
        # 调用 Prometheus API
        result = self._query_prometheus(query, time_range)
        return {"query": query, "data": result, "time_range": time_range}
```

---

## 3. 工具注册与发现

### 3.1 工具注册中心

```python
from typing import Optional
import logging

logger = logging.getLogger("agent.tools")

class ToolRegistry:
    """工具注册中心：管理工具的注册、发现和生命周期"""

    def __init__(self):
        self._tools: dict[str, BaseTool] = {}
        self._categories: dict[str, list[str]] = {}
        self._risk_levels: dict[str, list[str]] = {}
        self._usage_stats: dict[str, dict] = {}

    def register(self, tool: BaseTool, override: bool = False):
        """注册工具"""
        schema = tool.get_schema()
        name = schema.name

        if name in self._tools and not override:
            raise ValueError(f"工具 '{name}' 已注册，使用 override=True 覆盖")

        self._tools[name] = tool

        # 分类索引
        category = schema.category or "uncategorized"
        self._categories.setdefault(category, []).append(name)

        # 风险等级索引
        self._risk_levels.setdefault(schema.risk_level, []).append(name)

        # 初始化使用统计
        self._usage_stats[name] = {
            "total_calls": 0, "success": 0, "failures": 0,
            "total_latency_ms": 0, "last_used": None,
        }

        logger.info(f"注册工具: {name} (category={category}, risk={schema.risk_level})")

    def get_tools_for_task(
        self,
        task_type: str = None,
        categories: list[str] = None,
        max_risk: str = "medium",
        max_tools: int = 8,
    ) -> list[BaseTool]:
        """根据任务类型和约束获取可用工具集

        实现"最小必要工具集"原则：只返回完成当前任务所需的工具。
        """
        risk_order = {"low": 0, "medium": 1, "high": 2, "critical": 3}
        max_risk_level = risk_order.get(max_risk, 1)

        candidates = []
        for name, tool in self._tools.items():
            schema = tool.get_schema()

            # 风险过滤
            if risk_order.get(schema.risk_level, 0) > max_risk_level:
                continue

            # 分类过滤
            if categories and schema.category not in categories:
                continue

            candidates.append(tool)

        # 按使用频率排序（常用工具优先）
        candidates.sort(
            key=lambda t: self._usage_stats.get(t.get_schema().name, {}).get("total_calls", 0),
            reverse=True,
        )

        # 截断到最大工具数
        return candidates[:max_tools]

    def execute(self, tool_name: str, args: dict) -> dict:
        """安全执行工具调用"""
        import time

        tool = self._tools.get(tool_name)
        if not tool:
            return {"success": False, "error": f"未知工具: {tool_name}",
                    "available_tools": list(self._tools.keys())}

        # 参数验证
        valid, msg = tool.validate(**args)
        if not valid:
            return {"success": False, "error": f"参数验证失败: {msg}"}

        # 执行
        start = time.time()
        try:
            schema = tool.get_schema()
            result = tool.execute(**args)
            latency = (time.time() - start) * 1000

            # 更新统计
            stats = self._usage_stats[tool_name]
            stats["total_calls"] += 1
            stats["success"] += 1
            stats["total_latency_ms"] += latency
            stats["last_used"] = time.time()

            return {"success": True, "result": result, "latency_ms": latency,
                    "tool": tool_name}

        except Exception as e:
            latency = (time.time() - start) * 1000
            self._usage_stats[tool_name]["total_calls"] += 1
            self._usage_stats[tool_name]["failures"] += 1

            logger.error(f"工具执行失败: {tool_name}, error={e}")
            return {"success": False, "error": str(e), "tool": tool_name,
                    "latency_ms": latency}

    def get_usage_report(self) -> dict:
        """获取工具使用报告"""
        report = {}
        for name, stats in self._usage_stats.items():
            total = stats["total_calls"]
            report[name] = {
                "total_calls": total,
                "success_rate": stats["success"] / total if total > 0 else 0,
                "avg_latency_ms": stats["total_latency_ms"] / total if total > 0 else 0,
            }
        return report
```

### 3.2 动态工具加载

```python
class DynamicToolLoader:
    """动态工具加载器：根据任务上下文按需加载工具"""

    # 任务类型到工具集的映射
    TASK_TOOL_MAPPING = {
        "pod_diagnosis": [
            "kubectl_get", "kubectl_describe", "kubectl_logs",
            "kubectl_events", "kubectl_top",
        ],
        "node_diagnosis": [
            "kubectl_get", "kubectl_describe", "kubectl_top",
            "prometheus_query",
        ],
        "network_diagnosis": [
            "kubectl_get", "kubectl_describe", "kubectl_exec",
            "prometheus_query",
        ],
        "storage_diagnosis": [
            "kubectl_get", "kubectl_describe", "kubectl_logs",
        ],
        "performance_analysis": [
            "prometheus_query", "kubectl_top", "kubectl_get",
        ],
    }

    def __init__(self, registry: ToolRegistry):
        self.registry = registry

    def load_for_task(self, task: str) -> list[dict]:
        """根据任务描述动态加载工具集"""
        task_type = self._classify_task(task)
        tool_names = self.TASK_TOOL_MAPPING.get(task_type, [])

        tools = []
        for name in tool_names:
            tool = self.registry._tools.get(name)
            if tool:
                tools.append(tool.get_schema().to_openai_format())
        return tools

    def _classify_task(self, task: str) -> str:
        """基于关键词的快速任务分类"""
        task_lower = task.lower()
        if any(kw in task_lower for kw in ["pod", "容器", "pending", "crashloop"]):
            return "pod_diagnosis"
        elif any(kw in task_lower for kw in ["node", "节点", "notready"]):
            return "node_diagnosis"
        elif any(kw in task_lower for kw in ["网络", "network", "dns", "service"]):
            return "network_diagnosis"
        elif any(kw in task_lower for kw in ["存储", "storage", "pvc", "volume"]):
            return "storage_diagnosis"
        elif any(kw in task_lower for kw in ["性能", "cpu", "memory", "延迟"]):
            return "performance_analysis"
        return "pod_diagnosis"  # 默认
```

---

## 4. 工具编排模式

### 4.1 五种编排模式

```
工具编排模式:

1. 顺序编排（Sequential）
   工具 A → 工具 B → 工具 C
   前一个工具的输出是后一个的输入
   示例: describe pod → 解析 events → query prometheus

2. 并行编排（Parallel）
   工具 A ─┐
   工具 B ─┼→ 聚合结果
   工具 C ─┘
   多个独立工具同时执行
   示例: 同时获取 pod、node、event 信息

3. 条件编排（Conditional）
   工具 A → 条件判断 → 工具 B 或 工具 C
   根据中间结果选择下一个工具
   示例: 如果 CPU > 90% → 查 top pods，否则 → 查 network

4. 瀑布编排（Waterfall）
   工具 A → 工具 B → 失败? → 工具 C（备选）
   失败时自动切换到备选工具
   示例: prometheus query 失败 → 切换到 kubectl top

5. 管道编排（Pipeline）
   工具 A 的输出经过转换后作为工具 B 的输入
   中间有数据清洗/转换步骤
   示例: kubectl get -o json → jq 提取 → prometheus query
```

### 4.2 工具链构建器

```python
class ToolChainBuilder:
    """工具链构建器：声明式构建工具编排"""

    def __init__(self, registry: ToolRegistry):
        self.registry = registry
        self._chain: list[dict] = []

    def then(self, tool_name: str, args_builder=None) -> 'ToolChainBuilder':
        """顺序添加工具"""
        self._chain.append({
            "type": "sequential",
            "tool": tool_name,
            "args_builder": args_builder,
        })
        return self

    def parallel(self, *tool_specs) -> 'ToolChainBuilder':
        """并行添加多个工具"""
        self._chain.append({
            "type": "parallel",
            "tools": [{"tool": name, "args": args} for name, args in tool_specs],
        })
        return self

    def conditional(self, condition, if_true, if_false) -> 'ToolChainBuilder':
        """条件分支"""
        self._chain.append({
            "type": "conditional",
            "condition": condition,
            "if_true": if_true,
            "if_false": if_false,
        })
        return self

    def build(self) -> list:
        return self._chain

    def execute(self, initial_context: dict = None) -> dict:
        """执行工具链"""
        context = initial_context or {}
        results = []

        for step in self._chain:
            if step["type"] == "sequential":
                args = step.get("args_builder", lambda c: {})(context) if step.get("args_builder") else {}
                result = self.registry.execute(step["tool"], args)
                results.append(result)
                context["last_result"] = result

            elif step["type"] == "parallel":
                import asyncio
                parallel_results = asyncio.run(self._execute_parallel(step["tools"]))
                results.extend(parallel_results)
                context["parallel_results"] = parallel_results

            elif step["type"] == "conditional":
                condition_met = step["condition"](context)
                branch = step["if_true"] if condition_met else step["if_false"]
                result = self.registry.execute(branch["tool"], branch.get("args", {}))
                results.append(result)
                context["last_result"] = result

        return {"chain_results": results, "final_context": context}


# 使用示例
def build_pod_diagnosis_chain(registry: ToolRegistry, pod_name: str, namespace: str):
    """构建 Pod 诊断工具链"""
    chain = ToolChainBuilder(registry)

    # Step 1: 并行收集基础信息
    chain.parallel(
        ("kubectl_get", {"resource": "pods", "name": pod_name, "namespace": namespace}),
        ("kubectl_describe", {"resource": "pod", "name": pod_name, "namespace": namespace}),
        ("kubectl_events", {"namespace": namespace, "field_selector": f"involvedObject.name={pod_name}"}),
    )

    # Step 2: 根据 Pod 状态条件分支
    chain.conditional(
        condition=lambda ctx: "Pending" in str(ctx.get("parallel_results", [{}])[0].get("result", "")),
        if_true={"tool": "kubectl_get", "args": {"resource": "nodes", "output": "wide"}},
        if_false={"tool": "kubectl_logs", "args": {"pod": pod_name, "namespace": namespace, "tail": 100}},
    )

    return chain
```

---

## 5. 工具安全沙箱

### 5.1 安全执行环境

```python
import subprocess
import shlex
import re

class ToolSandbox:
    """工具安全沙箱：控制工具的执行边界"""

    def __init__(self, config: dict = None):
        self.config = config or {}
        self.allowed_commands = self.config.get("allowed_commands", [])
        self.blocked_patterns = self.config.get("blocked_patterns", [])
        self.max_output_size = self.config.get("max_output_size", 100_000)  # 100KB
        self.command_timeout = self.config.get("command_timeout", 30)
        self.audit_log: list[dict] = []

    def execute_command(self, command: str, dry_run: bool = False) -> dict:
        """在沙箱中执行命令"""
        # 1. 安全检查
        safety_check = self._check_command_safety(command)
        if not safety_check["safe"]:
            self.audit_log.append({
                "command": command, "action": "blocked",
                "reason": safety_check["reason"],
            })
            return {"success": False, "error": safety_check["reason"],
                    "blocked": True}

        # 2. Dry-run 模式
        if dry_run:
            self.audit_log.append({"command": command, "action": "dry_run"})
            return {"success": True, "dry_run": True,
                    "would_execute": command}

        # 3. 实际执行
        try:
            result = subprocess.run(
                shlex.split(command),
                capture_output=True,
                text=True,
                timeout=self.command_timeout,
            )

            output = result.stdout[:self.max_output_size]
            if len(result.stdout) > self.max_output_size:
                output += f"\n... (输出被截断，总长度 {len(result.stdout)} bytes)"

            self.audit_log.append({
                "command": command, "action": "executed",
                "exit_code": result.returncode,
            })

            return {
                "success": result.returncode == 0,
                "stdout": output,
                "stderr": result.stderr[:10000],
                "exit_code": result.returncode,
            }

        except subprocess.TimeoutExpired:
            self.audit_log.append({
                "command": command, "action": "timeout",
            })
            return {"success": False, "error": f"命令超时 ({self.command_timeout}s)"}

    def _check_command_safety(self, command: str) -> dict:
        """命令安全检查"""
        # 检查阻止模式
        for pattern in self.blocked_patterns:
            if re.search(pattern, command, re.IGNORECASE):
                return {"safe": False, "reason": f"匹配禁止模式: {pattern}"}

        # 检查命令白名单
        if self.allowed_commands:
            cmd_base = command.split()[0] if command else ""
            allowed = any(command.startswith(ac) for ac in self.allowed_commands)
            if not allowed:
                return {"safe": False,
                        "reason": f"命令 '{cmd_base}' 不在允许列表中"}

        return {"safe": True}


# K8S 运维沙箱配置
K8S_SANDBOX_CONFIG = {
    "allowed_commands": [
        "kubectl get", "kubectl describe", "kubectl logs",
        "kubectl top", "kubectl events", "kubectl explain",
    ],
    "blocked_patterns": [
        r"kubectl\s+delete",
        r"kubectl\s+drain",
        r"kubectl\s+cordon",
        r"kubectl\s+edit",
        r"kubectl\s+apply",
        r"kubectl\s+patch",
        r"kubectl\s+exec.*--\s*(rm|dd|mkfs|shutdown|reboot)",
        r"helm\s+(uninstall|delete|rollback)",
        r";\s*rm\s+",                    # 命令注入
        r"\|\s*rm\s+",                   # 管道注入
        r"\$\(",                         # 命令替换注入
        r"`",                            # 反引号注入
    ],
    "max_output_size": 100_000,
    "command_timeout": 30,
}
```

### 5.2 工具权限模型

```python
from enum import IntEnum

class ToolPermission(IntEnum):
    """工具权限等级"""
    READ = 1         # 只读操作
    SUGGEST = 2      # 可建议修改但不执行
    WRITE_SAFE = 3   # 可执行安全写操作（如 scale up）
    WRITE_RISKY = 4  # 可执行风险写操作（如 drain node）
    ADMIN = 5        # 管理员操作

class ToolPermissionManager:
    """工具权限管理器"""

    def __init__(self, default_permission: ToolPermission = ToolPermission.READ):
        self.default_permission = default_permission
        self._tool_permissions: dict[str, ToolPermission] = {}
        self._namespace_permissions: dict[str, ToolPermission] = {}

    def set_tool_permission(self, tool_name: str, permission: ToolPermission):
        self._tool_permissions[tool_name] = permission

    def set_namespace_permission(self, namespace: str, permission: ToolPermission):
        self._namespace_permissions[namespace] = permission

    def check_permission(self, tool_name: str, args: dict) -> tuple[bool, str]:
        """检查工具调用权限"""
        required = self._tool_permissions.get(tool_name, self.default_permission)
        namespace = args.get("namespace", "default")
        ns_permission = self._namespace_permissions.get(namespace, self.default_permission)

        effective = max(required, ns_permission)

        if effective > self.default_permission:
            if effective >= ToolPermission.WRITE_RISKY:
                return False, f"需要人工审批: {tool_name} 在 {namespace} 的权限等级为 {effective.name}"
            if effective >= ToolPermission.WRITE_SAFE:
                return True, f"允许执行安全写操作: {tool_name}"

        return True, "OK"
```

---

## 6. 错误处理与恢复

### 6.1 工具错误分类与恢复策略

```python
class ToolErrorClassifier:
    """工具错误分类器"""

    ERROR_PATTERNS = {
        "auth_error": {
            "patterns": ["Unauthorized", "Forbidden", "403", "401",
                         "certificate has expired"],
            "recovery": "refresh_credentials",
            "retryable": True,
        },
        "not_found": {
            "patterns": ["NotFound", "not found", "404",
                         "No resources found"],
            "recovery": "suggest_alternative",
            "retryable": False,
        },
        "timeout": {
            "patterns": ["timed out", "deadline exceeded",
                         "context deadline"],
            "recovery": "retry_with_backoff",
            "retryable": True,
        },
        "resource_conflict": {
            "patterns": ["Conflict", "409", "already exists",
                         "the object has been modified"],
            "recovery": "retry_with_latest",
            "retryable": True,
        },
        "quota_exceeded": {
            "patterns": ["quota", "exceeded", "LimitRange",
                         "insufficient"],
            "recovery": "report_and_suggest",
            "retryable": False,
        },
        "connection_error": {
            "patterns": ["connection refused", "no such host",
                         "network unreachable"],
            "recovery": "check_connectivity",
            "retryable": True,
        },
    }

    def classify(self, error_message: str) -> dict:
        """分类错误并建议恢复策略"""
        for error_type, config in self.ERROR_PATTERNS.items():
            for pattern in config["patterns"]:
                if pattern.lower() in error_message.lower():
                    return {
                        "type": error_type,
                        "recovery": config["recovery"],
                        "retryable": config["retryable"],
                        "original_error": error_message,
                    }
        return {
            "type": "unknown",
            "recovery": "escalate",
            "retryable": False,
            "original_error": error_message,
        }


class ToolRetryHandler:
    """工具重试处理器"""

    def __init__(self, max_retries: int = 3, base_delay: float = 1.0):
        self.max_retries = max_retries
        self.base_delay = base_delay
        self.error_classifier = ToolErrorClassifier()

    def execute_with_retry(self, tool, args: dict) -> dict:
        """带智能重试的工具执行"""
        import time

        last_error = None
        for attempt in range(self.max_retries + 1):
            try:
                result = tool.execute(**args)
                if result.get("success"):
                    return result

                # 分类错误
                error = result.get("error", "Unknown error")
                classified = self.error_classifier.classify(error)

                if not classified["retryable"]:
                    return {
                        "success": False,
                        "error": error,
                        "error_classification": classified,
                        "attempts": attempt + 1,
                    }

                last_error = classified

                # 指数退避
                delay = self.base_delay * (2 ** attempt)
                time.sleep(delay)

            except Exception as e:
                last_error = {"type": "exception", "error": str(e)}
                delay = self.base_delay * (2 ** attempt)
                time.sleep(delay)

        return {
            "success": False,
            "error": f"重试 {self.max_retries} 次后仍失败",
            "last_error": last_error,
            "attempts": self.max_retries + 1,
        }
```

---

## 7. MCP（Model Context Protocol）集成

### 7.1 MCP 工具适配器

```python
class MCPToolAdapter:
    """MCP 协议工具适配器

    将 MCP Server 的工具转换为 Agent Harness 标准工具接口。
    支持动态发现和注册 MCP Server 提供的工具。
    """

    def __init__(self, mcp_server_url: str, auth_token: str = None):
        self.server_url = mcp_server_url
        self.auth_token = auth_token
        self._discovered_tools: dict = {}

    async def discover_tools(self) -> list[ToolSchema]:
        """从 MCP Server 发现可用工具"""
        response = await self._call_mcp("tools/list")
        tools = []
        for tool_def in response.get("tools", []):
            schema = self._convert_mcp_to_schema(tool_def)
            self._discovered_tools[schema.name] = tool_def
            tools.append(schema)
        return tools

    async def execute_tool(self, tool_name: str, args: dict) -> dict:
        """通过 MCP 协议调用工具"""
        if tool_name not in self._discovered_tools:
            return {"success": False, "error": f"MCP 工具未发现: {tool_name}"}

        response = await self._call_mcp("tools/call", {
            "name": tool_name,
            "arguments": args,
        })
        return {
            "success": not response.get("isError", False),
            "result": response.get("content", []),
            "error": response.get("error"),
        }

    def _convert_mcp_to_schema(self, mcp_tool: dict) -> ToolSchema:
        """将 MCP 工具定义转换为标准 Schema"""
        params = []
        input_schema = mcp_tool.get("inputSchema", {})
        properties = input_schema.get("properties", {})
        required = input_schema.get("required", [])

        for name, prop in properties.items():
            params.append(ToolParameter(
                name=name,
                type=prop.get("type", "string"),
                description=prop.get("description", ""),
                required=name in required,
                enum=prop.get("enum"),
            ))

        return ToolSchema(
            name=mcp_tool["name"],
            description=mcp_tool.get("description", ""),
            parameters=params,
            category="mcp",
        )
```

---

## 8. 最佳实践总结

### 8.1 工具设计核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **最小工具集** | 每个任务类型只加载必需的工具 | 使用 DynamicToolLoader 按需加载 |
| **清晰的 Schema** | 工具描述必须准确，避免歧义 | 包含示例和使用场景 |
| **参数验证** | 调用前校验参数合法性 | 在 validate() 中实现完整校验 |
| **安全沙箱** | 所有工具在沙箱中执行 | 使用 ToolSandbox 控制执行边界 |
| **智能重试** | 根据错误类型决定是否重试 | 使用 ToolErrorClassifier 分类错误 |
| **使用统计** | 记录每个工具的调用指标 | 使用 ToolRegistry 内置统计 |
| **权限控制** | 按命名空间和操作类型控制权限 | 使用 ToolPermissionManager |
| **MCP 标准化** | 遵循 MCP 协议实现工具互操作 | 使用 MCPToolAdapter 集成 |

### 8.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **工具过载** | 给 Agent 所有可能的工具 | 动态加载，最小必要集 |
| **模糊描述** | 工具描述含糊不清 | 每个工具一句话精确描述 |
| **无参数验证** | 接受任意输入 | 类型检查 + 枚举限制 + 正则校验 |
| **无错误处理** | 工具失败返回 "Error" | 返回具体错误类型和恢复建议 |
| **硬编码工具** | 所有场景用同一套工具 | 按任务类型动态组合 |
| **无审计日志** | 不记录工具调用历史 | 每次调用记入审计日志 |

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | Harness 六层架构总览，工具精简原则 |
| [31 - Loop 与执行引擎](./31-agent-harness-loop-execution.md) | 工具调用在 Loop 中的执行流程 |
| [35 - 安全与约束](./35-agent-harness-security-constraints.md) | 工具安全沙箱和权限控制 |
| [05 - Tool Use & Function Calling](./05-tool-use-function-calling.md) | 工具调用的基础理论和规范 |
| [25 - MCP 集成](./25-agent-cli-mcp-integration.md) | MCP 协议工具集成详解 |

---

## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Vercel 团队 | 工具精简实验 15→2，准确率 80%→100% | 2025 |
| Anthropic | Tool Use Best Practices | 2025-12 |
| OpenAI | Function Calling 规范与最佳实践 | 2025 |
| MCP 规范 | Model Context Protocol 1.0 | 2025-2026 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 工具工程设计。*

---
title: Agent Harness 多 Agent 编排 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Multi-Agent,'
category: general
tags:
- ai
- ai-agent
- prometheus
- helm
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
- Agent Harness 多 Agent 编排 是什么
- 如何 Agent Harness 多 Agent 编排
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- Agent
- 编排
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- logging-basics
created: "2026-05-23"
---

title: Agent Harness 多 Agent 编排
description: '**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Multi-Agent,
  编排, Orchestrator, 分层 Harness, Agent 通信, 任务分解, 冲突解决, 隔离原则, DAG, 工作流'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- [[Helm|helm]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent Harness 多 Agent 编排 是什么
- 如何 Agent Harness 多 Agent 编排
trigger_keywords:
- Agent
- Harness
- Agent
- 编排
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

# Agent Harness 多 Agent 编排

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Multi-Agent, 编排, Orchestrator, 分层 Harness, Agent 通信, 任务分解, 冲突解决, 隔离原则, DAG, 工作流

---

<!-- chunk: 概述 -->## 概述

单个 Agent 的 Harness 就绪后，下一步挑战是**多 Agent 的 Harness 编排**。生产级系统往往需要多个专业化 Agent 协作——诊断 Agent 找根因、修复 Agent 执行操作、验证 Agent 确认恢复。每个 Agent 都有独立的 Harness（不同的权限、工具、约束），编排层需要协调它们的协作、通信和冲突解决。

本文系统阐述多 Agent 编排模式、Orchestrator 设计、Agent 通信协议、任务分解与分配、Harness 隔离原则、冲突解决机制，以及 K8S 运维场景中的多 Agent 协作实践。

---

<!-- chunk: 1. 多 Agent 编排模式 -->## 1. 多 Agent 编排模式

#<!-- chunk: 1.1 四种核心编排模式 -->## 1.1 四种核心编排模式

```
多 Agent 编排模式:

1. 顺序流水线（Sequential Pipeline）
   Agent A → Agent B → Agent C
   每个 Agent 的输出是下一个的输入
   示例: 诊断 → 修复 → 验证

2. 并行扇出（Parallel Fan-out）
   Agent A ─┐
   Agent B ─┼→ 聚合器 → 输出
   Agent C ─┘
   多个 Agent 同时处理不同子任务
   示例: 同时检查 Pod/Node/Network

3. 层级委派（Hierarchical Delegation）
   Orchestrator Agent
      ├── 子 Agent A（诊断）
      ├── 子 Agent B（监控分析）
      └── 子 Agent C（文档生成）
   主 Agent 分解任务并委派
   示例: SRE 指挥官 Agent 调度诊断团队

4. 辩论共识（Debate & Consensus）
   Agent A ←→ Agent B
      ↓         ↓
      共识判断器
   多个 Agent 对同一问题给出独立判断，通过辩论达成共识
   示例: 两个诊断 Agent 交叉验证根因
```

#<!-- chunk: 1.2 模式选择矩阵 -->## 1.2 模式选择矩阵

| 模式 | 适用场景 | 延迟 | 成本 | 可靠性 | 复杂度 |
|------|---------|------|------|--------|--------|
| **顺序流水线** | 有明确阶段划分的任务 | 高 | 低 | 中 | 低 |
| **并行扇出** | 可并行分解的独立子任务 | 低 | 中 | 高 | 中 |
| **层级委派** | 复杂多步任务 | 中 | 高 | 高 | 高 |
| **辩论共识** | 高风险决策需要交叉验证 | 高 | 高 | 最高 | 高 |

---

<!-- chunk: 2. Orchestrator 设计 -->## 2. Orchestrator 设计

#<!-- chunk: 2.1 编排器架构 -->## 2.1 编排器架构

```python
from dataclasses import dataclass, field
from typing import Optional, Any
from enum import Enum
import asyncio

class AgentRole(Enum):
    DIAGNOSTICIAN = "diagnostician"
    REMEDIATOR = "remediator"
    VERIFIER = "verifier"
    ANALYST = "analyst"
    COORDINATOR = "coordinator"

@dataclass
class AgentSpec:
    """Agent 规格定义"""
    role: AgentRole
    harness_config: dict
    tools: list[str]
    constraints: dict
    model: str = "gpt-4o"
    priority: int = 0

class Orchestrator:
    """多 Agent 编排器"""

    def __init__(self, agent_specs: dict[str, AgentSpec]):
        self.specs = agent_specs
        self.agents: dict[str, Any] = {}
        self._execution_graph: list = []
        self._results: dict[str, Any] = {}

    def register_agent(self, name: str, agent, harness):
        """注册 Agent 及其 Harness"""
        self.agents[name] = {
            "agent": agent,
            "harness": harness,
            "spec": self.specs[name],
        }

    async def execute_pipeline(self, task: str, pipeline: list[dict]) -> dict:
        """执行顺序流水线"""
        context = {"original_task": task}

        for stage in pipeline:
            agent_name = stage["agent"]
            agent_info = self.agents[agent_name]
            stage_task = stage.get("task_template", "{task}").format(
                task=task, **context,
            )

            result = await self._run_agent(
                agent_name, stage_task, context,
            )

            context[f"{agent_name}_result"] = result
            self._results[agent_name] = result

            # 阶段间门控：如果当前阶段失败，是否继续
            if not result.get("success") and stage.get("gate", True):
                return {
                    "status": "pipeline_halted",
                    "halted_at": agent_name,
                    "reason": result.get("error"),
                    "results": self._results,
                }

        return {
            "status": "pipeline_complete",
            "results": self._results,
        }

    async def execute_parallel(self, task: str,
                                agent_names: list[str]) -> dict:
        """执行并行扇出"""
        tasks = []
        for name in agent_names:
            tasks.append(self._run_agent(name, task, {}))

        results = await asyncio.gather(*tasks, return_exceptions=True)

        parallel_results = {}
        for name, result in zip(agent_names, results):
            if isinstance(result, Exception):
                parallel_results[name] = {
                    "success": False, "error": str(result),
                }
            else:
                parallel_results[name] = result

        # 聚合结果
        aggregated = self._aggregate_results(parallel_results)

        return {
            "status": "parallel_complete",
            "individual_results": parallel_results,
            "aggregated": aggregated,
        }

    async def execute_hierarchical(self, task: str) -> dict:
        """执行层级委派"""
        # 1. Coordinator Agent 分解任务
        coordinator = self.agents.get("coordinator")
        decomposition = await self._run_agent(
            "coordinator", f"分解以下任务为子任务: {task}", {},
        )

        subtasks = decomposition.get("subtasks", [])
        if not subtasks:
            return {"status": "decomposition_failed", "error": "无法分解任务"}

        # 2. 分配子任务给专业 Agent
        sub_results = {}
        for subtask in subtasks:
            agent_name = subtask.get("assign_to")
            if agent_name in self.agents:
                result = await self._run_agent(
                    agent_name, subtask["description"], {},
                )
                sub_results[agent_name] = result

        # 3. Coordinator 综合结果
        synthesis = await self._run_agent(
            "coordinator",
            f"综合以下子任务结果:\n{sub_results}",
            {"sub_results": sub_results},
        )

        return {
            "status": "hierarchical_complete",
            "decomposition": decomposition,
            "sub_results": sub_results,
            "synthesis": synthesis,
        }

    async def _run_agent(self, name: str, task: str,
                          context: dict) -> dict:
        """运行单个 Agent"""
        agent_info = self.agents[name]
        harness = agent_info["harness"]

        result = await harness.async_run(task, context)
        return result

    def _aggregate_results(self, results: dict) -> dict:
        """聚合并行结果"""
        successful = {k: v for k, v in results.items() if v.get("success")}
        failed = {k: v for k, v in results.items() if not v.get("success")}

        return {
            "total_agents": len(results),
            "successful": len(successful),
            "failed": len(failed),
            "consensus": self._find_consensus(successful),
        }

    def _find_consensus(self, results: dict) -> Optional[str]:
        """在多个成功结果中寻找共识"""
        if len(results) <= 1:
            return list(results.values())[0].get("answer") if results else None

        # 简单策略：如果多数 Agent 的答案相似，采用多数答案
        answers = [v.get("answer", "") for v in results.values()]
        # 生产环境应使用语义相似度比较
        return answers[0]
```

---

<!-- chunk: 3. Agent 间通信 -->## 3. Agent 间通信

#<!-- chunk: 3.1 消息协议 -->## 3.1 消息协议

```python
from dataclasses import dataclass, field
from datetime import datetime
from typing import Any, Optional
from enum import Enum

class MessageType(Enum):
    TASK_ASSIGNMENT = "task_assignment"
    TASK_RESULT = "task_result"
    INFORMATION_REQUEST = "info_request"
    INFORMATION_RESPONSE = "info_response"
    APPROVAL_REQUEST = "approval_request"
    APPROVAL_RESPONSE = "approval_response"
    STATUS_UPDATE = "status_update"
    ESCALATION = "escalation"

@dataclass
class AgentMessage:
    """Agent 间通信消息"""
    id: str
    type: MessageType
    sender: str
    receiver: str
    content: dict
    timestamp: str = field(default_factory=lambda: datetime.utcnow().isoformat())
    correlation_id: Optional[str] = None  # 关联同一会话的消息
    priority: int = 0
    ttl_seconds: int = 300  # 消息过期时间

class MessageBus:
    """Agent 消息总线"""

    def __init__(self):
        self._queues: dict[str, list[AgentMessage]] = {}
        self._handlers: dict[str, list] = {}
        self._history: list[AgentMessage] = []

    def send(self, message: AgentMessage):
        """发送消息"""
        receiver = message.receiver
        if receiver not in self._queues:
            self._queues[receiver] = []
        self._queues[receiver].append(message)
        self._history.append(message)

        # 触发处理器
        for handler in self._handlers.get(receiver, []):
            handler(message)

    def receive(self, agent_name: str,
                message_type: MessageType = None) -> list[AgentMessage]:
        """接收消息"""
        queue = self._queues.get(agent_name, [])
        if message_type:
            messages = [m for m in queue if m.type == message_type]
        else:
            messages = queue.copy()

        # 清除已读消息
        for m in messages:
            if m in queue:
                queue.remove(m)

        return messages

    def subscribe(self, agent_name: str, handler):
        """订阅消息"""
        if agent_name not in self._handlers:
            self._handlers[agent_name] = []
        self._handlers[agent_name].append(handler)

    def broadcast(self, sender: str, content: dict,
                  message_type: MessageType = MessageType.STATUS_UPDATE):
        """广播消息给所有 Agent"""
        for agent_name in self._queues:
            if agent_name != sender:
                self.send(AgentMessage(
                    id=f"broadcast_{datetime.utcnow().timestamp()}",
                    type=message_type,
                    sender=sender,
                    receiver=agent_name,
                    content=content,
                ))
```

#<!-- chunk: 3.2 共享上下文管理 -->## 3.2 共享上下文管理

```python
class SharedContext:
    """多 Agent 共享上下文"""

    def __init__(self):
        self._shared_state: dict = {}
        self._agent_contributions: dict[str, list] = {}
        self._locks: dict[str, bool] = {}

    def write(self, agent_name: str, key: str, value: Any,
              overwrite: bool = False):
        """写入共享上下文"""
        if key in self._shared_state and not overwrite:
            # 追加而非覆盖
            if isinstance(self._shared_state[key], list):
                self._shared_state[key].append(value)
            else:
                self._shared_state[key] = [self._shared_state[key], value]
        else:
            self._shared_state[key] = value

        # 记录贡献
        if agent_name not in self._agent_contributions:
            self._agent_contributions[agent_name] = []
        self._agent_contributions[agent_name].append({
            "key": key, "timestamp": datetime.utcnow().isoformat(),
        })

    def read(self, key: str, default: Any = None) -> Any:
        """读取共享上下文"""
        return self._shared_state.get(key, default)

    def read_all(self) -> dict:
        """读取全部共享上下文"""
        return self._shared_state.copy()

    def get_agent_contributions(self, agent_name: str) -> list:
        """获取某个 Agent 的贡献记录"""
        return self._agent_contributions.get(agent_name, [])
```

---

<!-- chunk: 4. Harness 隔离原则 -->## 4. Harness 隔离原则

#<!-- chunk: 4.1 Agent 隔离架构 -->## 4.1 Agent 隔离架构

```
多 Agent Harness 隔离:

┌──────────────────────────────────────────────────────┐
│                  Orchestrator Harness                  │
│  全局约束 │ 任务分配 │ 结果聚合 │ 冲突解决            │
├──────────────────────────────────────────────────────┤
│                                                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ 诊断 Agent    │  │ 修复 Agent    │  │ 验证 Agent   │ │
│  │              │  │              │  │              │ │
│  │ Constraints: │  │ Constraints: │  │ Constraints: │ │
│  │  只读        │  │  写+审批     │  │  只读+对比   │ │
│  │              │  │              │  │              │ │
│  │ Tools:       │  │ Tools:       │  │ Tools:       │ │
│  │  get/describe│  │  apply/patch │  │  get/describe│ │
│  │  logs/events │  │  scale/rollout│ │  logs/events │ │
│  │  prom/loki   │  │              │  │  prom query  │ │
│  │              │  │              │  │              │ │
│  │ Verify:      │  │ Verify:      │  │ Verify:      │ │
│  │  事实一致性  │  │  安全检查    │  │  恢复确认    │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
│                                                        │
│  隔离规则:                                              │
│  1. 每个 Agent 有独立的 Harness 实例                    │
│  2. 诊断 Agent 不信任修复 Agent 的自述                  │
│  3. 验证 Agent 独立运行，不依赖修复 Agent 的输出        │
│  4. 共享上下文通过 Orchestrator 中转                    │
└──────────────────────────────────────────────────────┘
```

#<!-- chunk: 4.2 隔离配置实现 -->## 4.2 隔离配置实现

```python
class IsolatedHarnessFactory:
    """隔离 Harness 工厂：为不同角色创建独立的 Harness"""

    ROLE_CONFIGS = {
        AgentRole.DIAGNOSTICIAN: {
            "constraints": {
                "read_only": True,
                "can_write": False,
                "can_delete": False,
                "max_iterations": 10,
                "max_tokens": 30_000,
                "blocked_commands": [
                    "kubectl delete", "kubectl drain",
                    "kubectl apply", "kubectl patch",
                    "helm install", "helm uninstall",
                ],
            },
            "tools": [
                "kubectl_get", "kubectl_describe", "kubectl_logs",
                "kubectl_events", "kubectl_top",
                "prometheus_query", "loki_search",
            ],
            "verifiers": [
                "factual_consistency",
                "output_format",
                "completeness",
            ],
        },
        AgentRole.REMEDIATOR: {
            "constraints": {
                "read_only": False,
                "can_write": True,
                "can_delete": False,
                "max_iterations": 5,
                "max_tokens": 20_000,
                "require_approval": True,
                "blocked_commands": [
                    "kubectl delete namespace",
                    "kubectl drain --force",
                    "helm uninstall",
                ],
            },
            "tools": [
                "kubectl_get", "kubectl_describe",
                "kubectl_apply", "kubectl_patch",
                "kubectl_scale", "kubectl_rollout",
            ],
            "verifiers": [
                "command_safety",
                "output_format",
            ],
        },
        AgentRole.VERIFIER: {
            "constraints": {
                "read_only": True,
                "can_write": False,
                "can_delete": False,
                "max_iterations": 5,
                "max_tokens": 15_000,
            },
            "tools": [
                "kubectl_get", "kubectl_describe",
                "kubectl_logs", "kubectl_events",
                "prometheus_query",
            ],
            "verifiers": [
                "factual_consistency",
            ],
        },
    }

    def create_harness(self, role: AgentRole, llm, tools_registry) -> dict:
        """为指定角色创建隔离的 Harness"""
        config = self.ROLE_CONFIGS.get(role, {})

        # 过滤工具集
        allowed_tools = config.get("tools", [])
        filtered_tools = tools_registry.get_tools_for_task(
            categories=None,
            max_tools=len(allowed_tools),
        )

        return {
            "role": role,
            "constraints": config.get("constraints", {}),
            "tools": filtered_tools,
            "verifiers": config.get("verifiers", []),
        }
```

---

<!-- chunk: 5. 冲突解决 -->## 5. 冲突解决

#<!-- chunk: 5.1 冲突类型与解决策略 -->## 5.1 冲突类型与解决策略

```python
class ConflictResolver:
    """多 Agent 冲突解决器"""

    def resolve(self, agent_results: dict[str, dict],
                conflict_type: str) -> dict:
        """解决 Agent 间的冲突"""
        strategies = {
            "diagnosis_disagreement": self._resolve_diagnosis,
            "action_conflict": self._resolve_action,
            "priority_conflict": self._resolve_priority,
        }

        strategy = strategies.get(conflict_type, self._resolve_default)
        return strategy(agent_results)

    def _resolve_diagnosis(self, results: dict) -> dict:
        """诊断分歧解决：置信度加权投票"""
        diagnoses = []
        for agent, result in results.items():
            diagnoses.append({
                "agent": agent,
                "diagnosis": result.get("root_cause", ""),
                "confidence": result.get("confidence", 0),
                "evidence_count": len(result.get("evidence", [])),
            })

        # 按置信度 × 证据数量排序
        diagnoses.sort(
            key=lambda d: d["confidence"] * (1 + d["evidence_count"] * 0.1),
            reverse=True,
        )

        winner = diagnoses[0]

        # 如果最高置信度 < 0.7 且有分歧，升级到人工
        if winner["confidence"] < 0.7 and len(set(d["diagnosis"] for d in diagnoses)) > 1:
            return {
                "resolution": "escalate_to_human",
                "reason": "诊断分歧且置信度不足",
                "candidates": diagnoses,
            }

        return {
            "resolution": "accepted",
            "selected_agent": winner["agent"],
            "diagnosis": winner["diagnosis"],
            "confidence": winner["confidence"],
            "dissenting": [d for d in diagnoses if d["agent"] != winner["agent"]],
        }

    def _resolve_action(self, results: dict) -> dict:
        """行动冲突解决：安全优先"""
        actions = []
        for agent, result in results.items():
            actions.append({
                "agent": agent,
                "action": result.get("recommended_action", {}),
                "risk_level": result.get("risk_level", "unknown"),
            })

        # 选择风险最低的行动方案
        risk_order = {"low": 0, "medium": 1, "high": 2, "critical": 3, "unknown": 4}
        actions.sort(key=lambda a: risk_order.get(a["risk_level"], 4))

        return {
            "resolution": "lowest_risk",
            "selected": actions[0],
            "alternatives": actions[1:],
        }

    def _resolve_priority(self, results: dict) -> dict:
        """优先级冲突：按角色权重"""
        role_weights = {
            AgentRole.DIAGNOSTICIAN: 3,
            AgentRole.VERIFIER: 2,
            AgentRole.REMEDIATOR: 1,
        }
        # 按角色权重选择
        sorted_results = sorted(
            results.items(),
            key=lambda x: role_weights.get(x[1].get("role"), 0),
            reverse=True,
        )
        return {"resolution": "role_priority", "selected": sorted_results[0]}

    def _resolve_default(self, results: dict) -> dict:
        return {"resolution": "first_successful",
                "selected": next(iter(results.values()))}
```

---

<!-- chunk: 6. K8S 问题处置多 Agent 编排 -->## 6. K8S 问题处置多 Agent 编排

#<!-- chunk: 6.1 问题处置流水线 -->## 6.1 问题处置流水线

```python
class IncidentResponsePipeline:
    """K8S 问题处置多 Agent 流水线"""

    def __init__(self, orchestrator: Orchestrator):
        self.orchestrator = orchestrator

    async def handle_incident(self, incident: dict) -> dict:
        """处置问题"""

        # Stage 1: 并行诊断（多角度收集信息）
        parallel_diagnosis = await self.orchestrator.execute_parallel(
            task=f"诊断以下问题: {incident['description']}",
            agent_names=["pod_diagnostician", "node_diagnostician", "network_diagnostician"],
        )

        # Stage 2: 综合诊断结果
        diagnosis = self._synthesize_diagnosis(parallel_diagnosis)
        if diagnosis.get("confidence", 0) < 0.6:
            return {
                "status": "escalate",
                "reason": "多 Agent 诊断置信度不足",
                "diagnosis_results": parallel_diagnosis,
            }

        # Stage 3: 生成修复方案
        remediation = await self.orchestrator.execute_pipeline(
            task=f"根据诊断结果制定修复方案: {diagnosis['root_cause']}",
            pipeline=[
                {"agent": "remediator", "gate": True},
            ],
        )

        # Stage 4: 独立验证修复效果
        verification = await self.orchestrator.execute_pipeline(
            task=f"验证问题是否已恢复: {incident['description']}",
            pipeline=[
                {"agent": "verifier", "gate": False},
            ],
        )

        return {
            "status": "resolved" if verification.get("success") else "partially_resolved",
            "diagnosis": diagnosis,
            "remediation": remediation,
            "verification": verification,
        }

    def _synthesize_diagnosis(self, parallel_results: dict) -> dict:
        """综合多 Agent 的诊断结果"""
        resolver = ConflictResolver()
        individual = parallel_results.get("individual_results", {})

        # 如果所有 Agent 都指向同一根因
        root_causes = [
            r.get("answer", {}).get("root_cause", "")
            for r in individual.values()
            if r.get("success")
        ]

        if len(set(root_causes)) == 1 and root_causes[0]:
            return {
                "root_cause": root_causes[0],
                "confidence": 0.95,
                "consensus": "unanimous",
            }

        # 否则通过冲突解决
        return resolver.resolve(individual, "diagnosis_disagreement")
```

---

<!-- chunk: 7. 分层 Harness 架构 -->## 7. 分层 Harness 架构

#<!-- chunk: 7.1 基础层 + 场景层 + 用户层 -->## 7.1 基础层 + 场景层 + 用户层

```python
class LayeredHarnessArchitecture:
    """三层 Harness 架构"""

    def __init__(self):
        # Layer 1: 基础层（所有 Agent 共享）
        self.base_config = {
            "max_iterations": 20,
            "timeout_seconds": 300,
            "safety_checks": True,
            "otel_tracing": True,
            "pii_filtering": True,
            "audit_logging": True,
        }

        # Layer 2: 场景层（按场景差异化）
        self.scenario_configs = {
            "k8s_diagnosis": {
                "read_only": True,
                "tools": ["kubectl_get", "kubectl_describe", "kubectl_logs",
                         "prometheus_query"],
                "max_iterations": 10,
                "verifiers": ["factual_consistency", "output_format"],
            },
            "k8s_remediation": {
                "read_only": False,
                "require_approval": True,
                "tools": ["kubectl_apply", "kubectl_patch", "kubectl_scale"],
                "max_iterations": 5,
                "verifiers": ["command_safety", "output_format"],
            },
            "code_review": {
                "read_only": True,
                "tools": ["read_file", "search_code", "run_tests"],
                "max_iterations": 15,
                "verifiers": ["completeness"],
            },
        }

        # Layer 3: 用户层（用户自定义覆盖）
        self.user_config = None

    def build_harness(self, scenario: str, user_overrides: dict = None) -> dict:
        """构建最终 Harness 配置"""
        # 基础层
        config = self.base_config.copy()

        # 场景层覆盖
        scenario_config = self.scenario_configs.get(scenario, {})
        config.update(scenario_config)

        # 用户层覆盖
        if user_overrides:
            config.update(user_overrides)

        return config
```

---

<!-- chunk: 8. 最佳实践 -->## 8. 最佳实践

#<!-- chunk: 8.1 多 Agent 编排核心原则 -->## 8.1 多 Agent 编排核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **Harness 隔离** | 每个 Agent 独立 Harness | 诊断只读、修复需审批、验证独立 |
| **最小信任** | Agent 间不信任彼此的输出 | 验证 Agent 独立检查修复结果 |
| **共识决策** | 高风险操作需要多 Agent 共识 | 使用辩论共识模式 |
| **安全优先** | 冲突时选择风险最低的方案 | ConflictResolver 安全优先策略 |
| **分层配置** | 基础+场景+用户三层 Harness | 使用 LayeredHarnessArchitecture |
| **异步通信** | Agent 间通过消息总线通信 | 使用 MessageBus 解耦 |

#<!-- chunk: 8.2 反模式 -->## 8.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **共享 Harness** | 所有 Agent 用同一套约束 | 每个角色独立约束 |
| **直接通信** | Agent 直接调用彼此 | 通过 Orchestrator 中转 |
| **盲信结果** | 修复 Agent 说"已修复"就信 | 独立验证 Agent 确认 |
| **串行万物** | 所有 Agent 串行执行 | 可并行的诊断并行执行 |
| **无冲突处理** | 忽略 Agent 间的分歧 | 部署冲突解决机制 |

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 多 Agent 编排基础概念 |
| [35 - 安全与约束](./35-agent-harness-security-constraints.md) | Agent 隔离的约束实现 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 多 Agent 编排基础理论 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Anthropic | Multi-Agent 系统设计最佳实践 | 2026-02 |
| Microsoft | AutoGen Multi-Agent 框架 | 2025-2026 |
| LangChain | LangGraph 多 Agent 编排 | 2025-2026 |
| CrewAI | Agent 角色与协作模式 | 2025-2026 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 多 Agent 编排。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails|安全护栏、提示注入防护与合规]]

## See Also

- 35-agent-harness-security-constraints
- 36-agent-harness-observability
- 38-agent-harness-performance-cost
- 39-agent-harness-testing-benchmark

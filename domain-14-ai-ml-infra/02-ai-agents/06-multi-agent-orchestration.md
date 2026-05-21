---
title: 多 Agent 编排与协作架构
description: 'title: 多 Agent 编排与协作架构'
category: general
tags:
- ai
- ai-agent
- scheduler
- prometheus
- grafana
- redis
- postgresql
- kafka
- hpa
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 多 Agent 编排与协作架构 是什么
- 如何 多 Agent 编排与协作架构
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- 编排与协作架构
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
---

title: 多 Agent 编排与协作架构
description: '# 多 Agent 编排与协作架构'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- scheduler
- prometheus
- grafana
- redis
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
- 多 Agent 编排与协作架构 是什么
- 如何 多 Agent 编排与协作架构
trigger_keywords:
- Agent
- 编排与协作架构
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

# 多 Agent 编排与协作架构

> **文档类型**: 架构设计专题 | **最后更新**: 2026-03 | **关键词**: 多 Agent, Supervisor-Worker, 事件驱动, Agent 编排, LangGraph, AutoGen, 分布式 Agent, 冲突解决, Agent 通信协议

---

<!-- chunk: 概述 -->## 概述

单 Agent 系统在复杂、需要多领域专业知识的任务中能力受限。多 Agent 系统通过专业分工和协作，能够处理更复杂的任务、提高并行效率并降低单点故障风险。本文覆盖多 Agent 的核心设计模式、LangGraph/AutoGen 实现、通信协议、冲突解决策略，以及生产级多 Agent 平台的架构设计。

---

<!-- chunk: 1. 多 Agent 架构模式 -->## 1. 多 Agent 架构模式

#<!-- chunk: 1.1 六大核心模式 -->## 1.1 六大核心模式

```
多 Agent 架构模式
│
├── 1. Supervisor-Worker（主管-工作者）
│      Orchestrator 分解任务 → 分发给专业 Worker Agent
│      适合: 任务可分解为子任务的场景
│
├── 2. Pipeline（流水线）
│      Agent A → Agent B → Agent C → 结果
│      适合: 有明确处理顺序的串行任务
│
├── 3. Peer-to-Peer（对等协作）
│      多个 Agent 平等协商，共同决策
│      适合: 需要多视角验证的决策场景
│
├── 4. Blackboard（黑板系统）
│      共享状态黑板，多 Agent 读写协作
│      适合: 异步、松耦合的并行任务
│
├── 5. Debate（辩论模式）
│      多个 Agent 提出不同方案，通过辩论收敛到最优解
│      适合: 高风险决策需要多方验证
│
└── 6. Hierarchical（层级模式）
       多层 Orchestrator + 专业 Agent 的树状结构
       适合: 大规模复杂系统
```

---

<!-- chunk: 2. Supervisor-Worker 模式（生产最常用） -->## 2. Supervisor-Worker 模式（生产最常用）

#<!-- chunk: 2.1 架构设计 -->## 2.1 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                     Orchestrator Agent                       │
│                    （任务分解 + 调度）                          │
│              使用强模型: GPT-4o / Claude 3.5 Sonnet           │
└──────┬──────────────┬──────────────┬──────────────┬──────────┘
       │              │              │              │
       ▼              ▼              ▼              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│  网络     │  │  存储     │  │  应用     │  │  安全     │
│  诊断    │  │  诊断    │  │  诊断    │  │  审计    │
│  Worker  │  │  Worker  │  │  Worker  │  │  Worker  │
│(专用工具) │  │(专用工具) │  │(专用工具) │  │(专用工具) │
└──────────┘  └──────────┘  └──────────┘  └──────────┘
       │              │              │              │
       └──────────────┴──────────────┴──────────────┘
                              │
                     ┌──────────────┐
                     │ 结果聚合 Agent │
                     │ (综合报告生成) │
                     └──────────────┘
```

#<!-- chunk: 2.2 LangGraph 实现 -->## 2.2 LangGraph 实现

```python
from langgraph.graph import StateGraph, END
from langgraph.prebuilt import ToolNode
from langchain_openai import ChatOpenAI
from typing import TypedDict, Annotated, Literal
import operator

# 定义共享状态
class OrchestratorState(TypedDict):
    original_task: str
    subtasks: list[dict]
    worker_results: Annotated[dict, lambda a, b: {**a, **b}]  # 合并字典
    final_report: str
    current_stage: str
    error_count: int

# 初始化模型
orchestrator_llm = ChatOpenAI(model="gpt-4o", temperature=0)
worker_llm = ChatOpenAI(model="gpt-4o-mini", temperature=0)  # Worker 用便宜模型

# Orchestrator：任务分解
def orchestrator_node(state: OrchestratorState) -> OrchestratorState:
    """将复杂任务分解为专业子任务"""
    response = orchestrator_llm.invoke(f"""
    你是运维任务调度专家。将以下复杂任务分解为专业子任务：
    
    任务：{state['original_task']}
    
    可用的专业 Worker：
    - network_worker: 网络连通性、DNS、Service、NetworkPolicy 诊断
    - storage_worker: PVC、StorageClass、CSI 相关问题
    - app_worker: Pod 状态、容器日志、应用配置问题
    - security_worker: RBAC、证书、权限相关问题
    
    输出 JSON 格式的子任务列表，每个子任务指定：worker、任务描述、优先级
    """)
    
    subtasks = parse_subtasks(response.content)
    return {"subtasks": subtasks, "current_stage": "dispatched"}

# 网络诊断 Worker
def network_worker_node(state: OrchestratorState) -> OrchestratorState:
    """网络专项诊断"""
    network_tasks = [t for t in state["subtasks"] if t["worker"] == "network_worker"]
    if not network_tasks:
        return {"worker_results": {}}
    
    # 网络 Worker 有特定工具集
    network_tools = [test_connectivity_tool, get_dns_tool, get_networkpolicy_tool]
    network_agent = create_react_agent(worker_llm, network_tools)
    
    results = {}
    for task in network_tasks:
        result = network_agent.invoke({"input": task["description"]})
        results[f"network_{task['id']}"] = result["output"]
    
    return {"worker_results": results}

# 结果聚合
def aggregator_node(state: OrchestratorState) -> OrchestratorState:
    """聚合所有 Worker 的结果，生成综合报告"""
    report = orchestrator_llm.invoke(f"""
    原始任务：{state['original_task']}
    
    各 Worker 诊断结果：
    {format_worker_results(state['worker_results'])}
    
    请生成：
    1. 根因分析摘要
    2. 问题优先级排序
    3. 综合修复方案
    4. 实施步骤（带风险提示）
    """)
    
    return {"final_report": report.content, "current_stage": "complete"}

# 路由：决定运行哪些 Worker（并行）
def route_to_workers(state: OrchestratorState) -> list[str]:
    """并行调度所有需要的 Worker"""
    workers_needed = set(t["worker"] for t in state["subtasks"])
    return list(workers_needed)  # LangGraph 支持返回列表实现并行

# 构建图
workflow = StateGraph(OrchestratorState)

workflow.add_node("orchestrator", orchestrator_node)
workflow.add_node("network_worker", network_worker_node)
workflow.add_node("storage_worker", storage_worker_node)
workflow.add_node("app_worker", app_worker_node)
workflow.add_node("security_worker", security_worker_node)
workflow.add_node("aggregator", aggregator_node)

workflow.set_entry_point("orchestrator")

# 并行分发到多个 Worker
workflow.add_conditional_edges(
    "orchestrator",
    route_to_workers,
    {
        "network_worker": "network_worker",
        "storage_worker": "storage_worker",
        "app_worker": "app_worker",
        "security_worker": "security_worker",
    }
)

# 所有 Worker 完成后聚合
for worker in ["network_worker", "storage_worker", "app_worker", "security_worker"]:
    workflow.add_edge(worker, "aggregator")

workflow.add_edge("aggregator", END)

# 编译
multi_agent_app = workflow.compile()
```

---

<!-- chunk: 3. Debate（辩论）模式：高风险决策 -->## 3. Debate（辩论）模式：高风险决策

适用于生产变更等高风险场景，通过多个 Agent 从不同视角评审方案：

```python
class DebateOrchestrator:
    """辩论模式：多 Agent 对一个决策进行多轮辩论"""
    
    def __init__(self, llm, rounds: int = 2):
        self.llm = llm
        self.rounds = rounds
        
        # 不同角色的 Agent（同一个 LLM，不同系统提示）
        self.agents = {
            "proposer": "你是变更方案提出者，负责提出并捍卫你的技术方案",
            "critic": "你是技术审查员，专门发现方案中的风险和缺陷，持批评态度",
            "safety_reviewer": "你是 SRE 安全审查员，关注方案对生产稳定性的影响",
            "moderator": "你是讨论主持人，总结各方观点并推动收敛到最终决策",
        }
    
    def debate(self, proposal: str) -> dict:
        """执行多轮辩论"""
        debate_history = []
        
        # 初始提案
        proposer_response = self._agent_respond(
            "proposer", f"请详细阐述以下方案的技术实现和优势：\n{proposal}", []
        )
        debate_history.append({"role": "proposer", "content": proposer_response})
        
        # 多轮辩论
        for round_num in range(self.rounds):
            # 审查员提出质疑
            critic_response = self._agent_respond(
                "critic", 
                f"针对以下提案，指出3-5个技术风险和潜在缺陷：",
                debate_history
            )
            debate_history.append({"role": "critic", "content": critic_response})
            
            # 安全审查
            safety_response = self._agent_respond(
                "safety_reviewer",
                "从生产稳定性角度评估该方案的风险：",
                debate_history
            )
            debate_history.append({"role": "safety_reviewer", "content": safety_response})
            
            # 提案者回应
            defense = self._agent_respond(
                "proposer",
                "回应以上质疑，必要时修改和完善你的方案：",
                debate_history
            )
            debate_history.append({"role": "proposer", "content": defense})
        
        # 主持人总结
        conclusion = self._agent_respond(
            "moderator",
            "综合所有讨论，给出最终决策建议（通过/拒绝/修改后通过），说明理由：",
            debate_history
        )
        
        return {
            "original_proposal": proposal,
            "debate_history": debate_history,
            "conclusion": conclusion,
            "approved": "通过" in conclusion or "修改后通过" in conclusion,
        }
    
    def _agent_respond(self, role: str, task: str, history: list) -> str:
        system_msg = self.agents[role]
        messages = [{"role": "system", "content": system_msg}]
        
        if history:
            messages.append({
                "role": "user",
                "content": f"辩论历史：\n{self._format_history(history)}"
            })
        
        messages.append({"role": "user", "content": task})
        return self.llm.invoke(messages).content
```

---

<!-- chunk: 4. Blackboard（黑板）模式：异步协作 -->## 4. Blackboard（黑板）模式：异步协作

```python
import asyncio
from dataclasses import dataclass, field
from typing import Optional
import threading

@dataclass
class BlackboardEntry:
    key: str
    value: any
    written_by: str
    timestamp: float
    confidence: float = 1.0  # 置信度，冲突时用于决策

class Blackboard:
    """共享知识黑板：多 Agent 异步读写"""
    
    def __init__(self):
        self._data: dict[str, list[BlackboardEntry]] = {}
        self._lock = threading.RLock()
        self._observers: dict[str, list] = {}  # 订阅特定 key 变更的回调
    
    def write(self, key: str, value: any, agent_id: str, confidence: float = 1.0):
        """Agent 写入发现结果"""
        with self._lock:
            entry = BlackboardEntry(
                key=key, value=value, written_by=agent_id,
                timestamp=time.time(), confidence=confidence
            )
            if key not in self._data:
                self._data[key] = []
            self._data[key].append(entry)
            
            # 通知订阅者
            for callback in self._observers.get(key, []):
                asyncio.create_task(callback(key, entry))
    
    def read(self, key: str, resolve_conflicts: bool = True) -> Optional[any]:
        """读取黑板上的信息，自动解决冲突"""
        with self._lock:
            entries = self._data.get(key, [])
            if not entries:
                return None
            
            if not resolve_conflicts or len(entries) == 1:
                return entries[-1].value
            
            # 冲突解决：选择置信度最高的
            return max(entries, key=lambda e: e.confidence).value
    
    def subscribe(self, key: str, callback):
        """订阅特定 key 的变更事件"""
        if key not in self._observers:
            self._observers[key] = []
        self._observers[key].append(callback)

class BlackboardAgent:
    """基于黑板的异步 Agent"""
    
    def __init__(self, agent_id: str, specialization: str, blackboard: Blackboard):
        self.agent_id = agent_id
        self.specialization = specialization
        self.blackboard = blackboard
    
    async def observe_and_act(self):
        """持续观察黑板，根据新信息采取行动"""
        while True:
            # 检查黑板上是否有本专业相关的新信息
            task = self.blackboard.read(f"task_{self.specialization}")
            
            if task and not self.blackboard.read(f"result_{self.agent_id}"):
                # 执行专业分析
                result = await self._analyze(task)
                
                # 写回结果
                self.blackboard.write(
                    key=f"result_{self.agent_id}",
                    value=result,
                    agent_id=self.agent_id,
                    confidence=result.get("confidence", 0.8)
                )
            
            await asyncio.sleep(1)  # 避免忙等待
```

---

<!-- chunk: 5. 多 Agent 通信协议 -->## 5. 多 Agent 通信协议

#<!-- chunk: 5.1 标准化消息格式 -->## 5.1 标准化消息格式

```python
from dataclasses import dataclass
from enum import Enum
from typing import Optional

class MessageType(Enum):
    TASK_ASSIGN = "task_assign"       # 分配任务
    TASK_RESULT = "task_result"       # 返回结果
    CLARIFICATION = "clarification"   # 请求澄清
    STATUS_UPDATE = "status_update"   # 状态更新
    ERROR_REPORT = "error_report"     # 错误报告
    ESCALATION = "escalation"         # 升级处理
    APPROVAL_REQUEST = "approval_request"  # 请求审批

@dataclass
class AgentMessage:
    """Agent 间通信的标准消息格式"""
    message_id: str
    sender_id: str
    receiver_id: str            # 或 "broadcast"
    message_type: MessageType
    content: dict
    correlation_id: Optional[str] = None  # 关联的父消息 ID
    priority: int = 5           # 1-10，10 最高
    ttl_seconds: int = 300      # 消息有效期
    timestamp: str = ""

# 任务分配消息示例
task_message = AgentMessage(
    message_id="msg-001",
    sender_id="orchestrator",
    receiver_id="network_worker",
    message_type=MessageType.TASK_ASSIGN,
    content={
        "task_id": "task-001",
        "description": "检查 production 命名空间的网络连通性",
        "context": {
            "affected_pods": ["api-server-xxx", "backend-yyy"],
            "symptoms": "前端无法访问后端 Service",
        },
        "constraints": {
            "readonly_only": True,
            "timeout_seconds": 60,
        },
        "expected_output": "网络连通性诊断报告（含根因和修复建议）"
    },
    priority=8,
)

# 结果返回消息示例
result_message = AgentMessage(
    message_id="msg-002",
    sender_id="network_worker",
    receiver_id="orchestrator",
    message_type=MessageType.TASK_RESULT,
    correlation_id="msg-001",
    content={
        "task_id": "task-001",
        "status": "completed",
        "findings": {
            "root_cause": "NetworkPolicy 阻断了 frontend → backend 的 8080 端口",
            "evidence": ["kubectl get networkpolicy 输出...", "连通性测试结果..."],
            "confidence": 0.95,
        },
        "recommendations": [
            "修改 NetworkPolicy 允许 frontend → backend:8080",
            "或添加 backend Pod 的 spec.selector 标签",
        ],
        "fix_yaml": "apiVersion: networking.k8s.io/v1\n...",
    },
)
```

#<!-- chunk: 5.2 消息队列集成 -->## 5.2 消息队列集成

```python
import asyncio
from typing import Callable

class AgentMessageBus:
    """基于 Redis Stream 的 Agent 消息总线（生产级实现）"""
    
    def __init__(self, redis_client):
        self.redis = redis_client
        self.stream_name = "agent_messages"
    
    async def publish(self, message: AgentMessage):
        """发布消息到消息总线"""
        await self.redis.xadd(
            self.stream_name,
            {
                "message_id": message.message_id,
                "sender_id": message.sender_id,
                "receiver_id": message.receiver_id,
                "message_type": message.message_type.value,
                "content": json.dumps(message.content),
                "priority": str(message.priority),
            }
        )
    
    async def subscribe(
        self, 
        agent_id: str, 
        handler: Callable[[AgentMessage], None]
    ):
        """订阅发给特定 Agent 的消息"""
        last_id = "0"  # 从头读取，生产环境应从断点恢复
        
        while True:
            messages = await self.redis.xread(
                {self.stream_name: last_id},
                count=10,
                block=1000  # 阻塞等待 1 秒
            )
            
            for stream, stream_messages in messages:
                for msg_id, fields in stream_messages:
                    last_id = msg_id
                    
                    # 过滤属于本 Agent 的消息
                    if fields["receiver_id"] in [agent_id, "broadcast"]:
                        agent_msg = self._deserialize(fields)
                        await handler(agent_msg)
```

---

<!-- chunk: 6. 冲突解决策略 -->## 6. 冲突解决策略

当多个 Agent 对同一问题产生不同结论时：

```python
class ConflictResolver:
    """多 Agent 结论冲突解决器"""
    
    def resolve(
        self,
        question: str,
        agent_responses: list[dict],
        resolution_strategy: str = "weighted_confidence"
    ) -> dict:
        """解决多个 Agent 的结论冲突"""
        
        if resolution_strategy == "voting":
            return self._majority_vote(agent_responses)
        
        elif resolution_strategy == "weighted_confidence":
            return self._weighted_confidence(agent_responses)
        
        elif resolution_strategy == "llm_arbitration":
            return self._llm_arbitrate(question, agent_responses)
        
        elif resolution_strategy == "human_escalation":
            return self._escalate_to_human(question, agent_responses)
    
    def _majority_vote(self, responses: list[dict]) -> dict:
        """多数投票（适合分类型结论）"""
        from collections import Counter
        
        conclusions = [r["conclusion"] for r in responses]
        vote_counts = Counter(conclusions)
        winner = vote_counts.most_common(1)[0][0]
        
        return {
            "conclusion": winner,
            "method": "majority_vote",
            "vote_distribution": dict(vote_counts),
            "confidence": vote_counts[winner] / len(responses),
        }
    
    def _weighted_confidence(self, responses: list[dict]) -> dict:
        """加权置信度（适合有把握度的结论）"""
        if not responses:
            return {"conclusion": "无法确定", "confidence": 0}
        
        # 按置信度排序
        sorted_responses = sorted(
            responses, 
            key=lambda r: r.get("confidence", 0.5),
            reverse=True
        )
        
        best = sorted_responses[0]
        
        # 如果最高置信度 < 0.7，且存在显著分歧，升级处理
        if best.get("confidence", 0) < 0.7:
            return {
                "conclusion": best["conclusion"],
                "method": "weighted_confidence",
                "confidence": best.get("confidence", 0),
                "needs_review": True,
                "all_responses": responses,
            }
        
        return {
            "conclusion": best["conclusion"],
            "method": "weighted_confidence",
            "confidence": best.get("confidence", 0),
            "supporting_agents": [r["agent_id"] for r in sorted_responses 
                                  if r["conclusion"] == best["conclusion"]],
        }
    
    def _llm_arbitrate(self, question: str, responses: list[dict]) -> dict:
        """用 LLM 作为仲裁者（适合复杂技术判断）"""
        arbitration_prompt = f"""
        多个专业 Agent 对以下问题产生了不同结论，请作为仲裁者给出最终判断：
        
        问题：{question}
        
        各 Agent 结论：
        {json.dumps(responses, ensure_ascii=False, indent=2)}
        
        请：
        1. 分析各方结论的优缺点
        2. 给出最终判断及理由
        3. 如果确实无法确定，说明需要补充什么信息
        """
        
        final_judgment = self.arbitrator_llm.invoke(arbitration_prompt).content
        
        return {
            "conclusion": final_judgment,
            "method": "llm_arbitration",
            "original_responses": responses,
        }
```

---

<!-- chunk: 7. 生产级多 Agent 平台架构 -->## 7. 生产级多 Agent 平台架构

```
┌──────────────────────────────────────────────────────────────────┐
│                         API Gateway                               │
│                    (认证、限流、路由)                               │
└────────────────────────────┬─────────────────────────────────────┘
                              │
┌────────────────────────────▼─────────────────────────────────────┐
│                    Orchestration Layer                             │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│   │  Task Queue  │  │  Scheduler   │  │  State Manager        │   │
│   │  (Redis/Kafka│  │  (LangGraph) │  │  (Redis/PostgreSQL)  │   │
│   └──────────────┘  └──────────────┘  └──────────────────────┘   │
└────────────────────────────┬─────────────────────────────────────┘
                              │
┌────────────────────────────▼─────────────────────────────────────┐
│                       Agent Worker Pool                            │
│   ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌──────────┐   │
│   │ Network    │  │ Storage    │  │ Security   │  │  App     │   │
│   │ Agents     │  │ Agents     │  │ Agents     │  │  Agents  │   │
│   │ (K8s Pod)  │  │ (K8s Pod)  │  │ (K8s Pod)  │  │(K8s Pod) │   │
│   └────────────┘  └────────────┘  └────────────┘  └──────────┘   │
│         │               │               │               │         │
│   ┌─────▼───────────────▼───────────────▼───────────────▼──────┐ │
│   │              Shared Tool Registry (工具注册中心)              │ │
│   └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────▼────────────────────────────────────┐
│                      Observability Stack                           │
│      LangSmith / Langfuse + Prometheus + Grafana + 告警            │
└──────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 7.1 K8s 上的多 Agent 部署 -->## 7.1 K8s 上的多 Agent 部署

```yaml
# Agent Worker Deployment 模板
apiVersion: apps/v1
kind: Deployment
metadata:
  name: k8s-network-agent
  namespace: ai-agents
  labels:
    agent-type: network-specialist
spec:
  replicas: 2  # 高可用
  selector:
    matchLabels:
      app: k8s-network-agent
  template:
    metadata:
      labels:
        app: k8s-network-agent
    spec:
      serviceAccountName: agent-readonly-sa  # 最小权限 SA
      containers:
      - name: agent
        image: kudig/network-agent:v1.2.0
        env:
        - name: OPENAI_API_KEY
          valueFrom:
            secretKeyRef:
              name: llm-api-keys
              key: openai-key
        - name: AGENT_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: MESSAGE_BUS_URL
          value: "redis://redis-master.ai-infra.svc:6379"
        - name: ORCHESTRATOR_URL
          value: "http://orchestrator-svc.ai-agents.svc:8080"
        resources:
          requests:
            memory: "512Mi"
            cpu: "250m"
          limits:
            memory: "1Gi"
            cpu: "500m"
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
      
      # 可选：Sidecar 进行日志收集
      - name: log-shipper
        image: fluent/fluent-bit:latest
        # ...

---
# HPA：根据任务队列长度自动扩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: network-agent-hpa
  namespace: ai-agents
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: k8s-network-agent
  minReplicas: 1
  maxReplicas: 10
  metrics:
  - type: External
    external:
      metric:
        name: redis_queue_length
        selector:
          matchLabels:
            queue: network_agent_tasks
      target:
        type: AverageValue
        averageValue: "5"  # 每个 Pod 处理 5 个任务时扩容
```

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **明确边界**：每个 Agent 的职责范围要清晰，避免越界调用其他 Agent 的工具
- **异步通信**：Agent 间通过消息队列通信而非直接调用，提高解耦性和弹性
- **渐进式引入**：从单 Agent 开始，确认有多 Agent 价值后再重构
- **强模型当 Orchestrator**：任务分解和质量把控用 GPT-4o/Claude，执行用便宜模型
- **超时防护**：给每个 Worker 设置最大执行时间，避免一个卡住阻塞整体

#<!-- chunk: 反模式 -->## 反模式

- **过度拆分**：3 步任务拆成 5 个 Agent，沟通成本超过了并行收益
- **Agent 间直接调用**：点对点依赖导致强耦合，改用消息总线
- **共享可变状态**：多个 Agent 直接读写同一个数据结构，引入竞争条件
- **无中心状态管理**：任务状态分散在各 Agent 中，无法追踪整体进度和恢复故障
- **不设权限边界**：所有 Agent 共享同一个 K8s ServiceAccount，某个 Agent 被攻击后影响全局

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | Plan-and-Execute 模式与 Supervisor-Worker 的关系 |
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | LangGraph/AutoGen/CrewAI 框架实现 |
| [05 - 工具调用](./[[domain-14-ai-ml-infra/05-tool-use-function-calling.md|05-tool-use-function-calling]].md) | 多 Agent 间的工具共享和访问控制 |
| [09 - 生产部署](./[[domain-14-ai-ml-infra/09-production-deployment-guide.md|09-production-deployment-guide]].md) | 多 Agent 平台的 K8s 部署架构 |
| [14 - Agent 赋能设计与落地路径](./[[domain-14-ai-ml-infra/14-agent-kudig-design-strategy.md|14-agent-kudig-design-strategy]].md) | K8s 运维 Agent 的四大方向 |

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
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization.md|成本与延迟优化策略]]

## See Also

- [[domain-14-ai-ml-infra/04-rag-knowledge-retrieval.md|04-rag-knowledge-retrieval]]
- [[domain-14-ai-ml-infra/05-tool-use-function-calling.md|05-tool-use-function-calling]]
- [[domain-14-ai-ml-infra/07-memory-context-management.md|07-memory-context-management]]
- [[domain-14-ai-ml-infra/08-agent-evaluation-observability.md|08-agent-evaluation-observability]]

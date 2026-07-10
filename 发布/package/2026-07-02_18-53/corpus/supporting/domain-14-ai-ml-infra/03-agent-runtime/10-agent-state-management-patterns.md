---
title: Agent状态管理模式
description: '无状态vs有状态Agent架构、检查点策略、状态存储方案、状态回放调试与长时记忆分层设计'
summary: '无状态vs有状态Agent架构、检查点策略、状态存储方案、状态回放调试与长时记忆分层设计'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- state-management
- checkpoint
- memory
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
- Agent状态管理模式 是什么
- 如何管理Agent状态
- Agent检查点策略详解
trigger_keywords:
- agent-state
- checkpoint
- memory-management
- state-replay
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

# Agent状态管理模式

## 概述

Agent状态管理是构建生产级AI Agent的核心挑战。LLM Agent的执行往往跨越多轮对话、多次工具调用甚至多个会话，如何在保证可靠性的同时高效管理这些状态，直接决定了Agent系统的可维护性和可扩展性。

本文档系统介绍Agent状态管理的完整方法论：从无状态到有状态的架构选型、检查点策略设计、状态存储方案对比、状态回放调试技术，以及长时记忆的分层架构。

## 无状态vs有状态Agent

### 无状态Agent

无状态Agent每次执行都从零开始，不保留任何历史状态：

```python
class StatelessAgent:
    """无状态Agent - 每次调用独立"""

    def __init__(self, system_prompt: str, tools: list):
        self.system_prompt = system_prompt
        self.tools = tools

    async def execute(self, user_input: str) -> str:
        """执行单次Agent任务，不保留历史"""
        messages = [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": user_input},
        ]

        response = await self.llm_call(messages)

        while response.has_tool_calls:
            tool_results = []
            for tool_call in response.tool_calls:
                result = await self.execute_tool(tool_call)
                tool_results.append(result)

            messages.append(response.message)
            messages.extend(tool_results)
            response = await self.llm_call(messages)

        return response.content
```

无状态Agent的优势：

```
优势:
  - 简单可靠，无状态丢失风险
  - 水平扩展容易，无亲和性要求
  - 调试简单，每次执行独立
  - 适合单轮任务（问答、翻译、摘要）

劣势:
  - 无法维持多轮对话上下文
  - 无法学习和积累经验
  - 复杂任务需要重复输入上下文
  - Token消耗较高（重复传递历史）
```

### 有状态Agent

有状态Agent维护跨调用的执行状态：

```python
class StatefulAgent:
    """有状态Agent - 维护执行历史和记忆"""

    def __init__(self, agent_id: str, state_store: StateStore):
        self.agent_id = agent_id
        self.state_store = state_store

    async def execute(self, user_input: str) -> str:
        # 加载历史状态
        state = await self.state_store.load(self.agent_id)

        if state is None:
            state = AgentState(
                conversation_history=[],
                tool_results=[],
                memory=AgentMemory(),
                metadata={},
            )

        # 添加用户输入
        state.conversation_history.append({
            "role": "user",
            "content": user_input,
        })

        # 构建包含历史的消息
        messages = self._build_messages(state)

        response = await self.llm_call(messages)

        while response.has_tool_calls:
            for tool_call in response.tool_calls:
                result = await self.execute_tool(tool_call)
                state.tool_results.append({
                    "tool": tool_call.name,
                    "input": tool_call.arguments,
                    "output": result.output,
                    "timestamp": datetime.utcnow(),
                })

            state.conversation_history.append(response.message)
            state.conversation_history.append({
                "role": "tool",
                "content": result.output,
                "tool_call_id": tool_call.id,
            })

            response = await self.llm_call(self._build_messages(state))

        # 保存Agent响应
        state.conversation_history.append({
            "role": "assistant",
            "content": response.content,
        })

        # 更新记忆
        await state.memory.update(state.conversation_history)

        # 持久化状态
        await self.state_store.save(self.agent_id, state)

        return response.content

    def _build_messages(self, state: AgentState) -> list:
        """构建LLM消息，可能压缩历史"""
        messages = [{"role": "system", "content": self.system_prompt}]

        # 可能需要压缩历史以适应上下文窗口
        compressed_history = self._compress_history(
            state.conversation_history
        )
        messages.extend(compressed_history)

        return messages
```

### 架构选型指南

```
选型决策树:

任务是否需要跨调用上下文？
  ├── 否 → 无状态Agent
  └── 是 → 上下文是否仅限单次会话？
      ├── 是 → 会话级有状态Agent
      └── 否 → 需要跨会话记忆？
          ├── 否 → 会话级有状态Agent + 会话超时
          └── 是 → 持久化有状态Agent + 记忆分层

推荐方案:
  简单问答 → 无状态
  客服对话 → 会话级有状态（TTL 30分钟）
  代码助手 → 会话级有状态（TTL 2小时）
  研究助手 → 持久化有状态 + 长期记忆
  自主Agent → 持久化有状态 + 完整记忆分层
```

## 检查点策略

### 检查点时机

```python
from enum import Enum

class CheckpointStrategy(Enum):
    EVERY_STEP = "every_step"           # 每步检查点
    KEY_NODES = "key_nodes"             # 关键节点检查点
    TIME_BASED = "time_based"           # 定时检查点
    ADAPTIVE = "adaptive"               # 自适应检查点


class CheckpointManager:
    """Agent检查点管理器"""

    def __init__(
        self,
        strategy: CheckpointStrategy,
        checkpoint_store: CheckpointStore,
    ):
        self.strategy = strategy
        self.store = checkpoint_store
        self.last_checkpoint_time = datetime.utcnow()
        self.step_count = 0

    async def maybe_checkpoint(
        self,
        agent_id: str,
        state: AgentState,
        step_type: str,
    ) -> bool:
        """根据策略决定是否创建检查点"""
        should_checkpoint = False

        if self.strategy == CheckpointStrategy.EVERY_STEP:
            should_checkpoint = True

        elif self.strategy == CheckpointStrategy.KEY_NODES:
            # 只在关键节点创建检查点
            key_step_types = {
                "llm_inference",
                "tool_execution",
                "human_approval",
                "state_transition",
            }
            should_checkpoint = step_type in key_step_types

        elif self.strategy == CheckpointStrategy.TIME_BASED:
            # 每N秒创建一次检查点
            elapsed = (datetime.utcnow() - self.last_checkpoint_time).total_seconds()
            should_checkpoint = elapsed >= 30  # 每30秒

        elif self.strategy == CheckpointStrategy.ADAPTIVE:
            # 自适应策略：根据执行复杂度调整
            should_checkpoint = self._adaptive_decision(state, step_type)

        if should_checkpoint:
            await self._create_checkpoint(agent_id, state)
            self.last_checkpoint_time = datetime.utcnow()
            self.step_count += 1
            return True

        return False

    def _adaptive_decision(
        self,
        state: AgentState,
        step_type: str,
    ) -> bool:
        """自适应检查点决策"""
        # 高风险操作后立即检查点
        high_risk_steps = {"tool_execution", "state_mutation"}
        if step_type in high_risk_steps:
            return True

        # 状态大小超过阈值时检查点
        state_size = self._estimate_state_size(state)
        if state_size > 1024 * 1024:  # 1MB
            return True

        # 距离上次检查点超过一定步数
        if self.step_count % 5 == 0:
            return True

        return False

    async def _create_checkpoint(
        self,
        agent_id: str,
        state: AgentState,
    ) -> str:
        """创建检查点"""
        checkpoint_id = f"{agent_id}-{uuid.uuid4().hex[:8]}"

        checkpoint = Checkpoint(
            id=checkpoint_id,
            agent_id=agent_id,
            state=state.serialize(),
            created_at=datetime.utcnow(),
            step_count=self.step_count,
        )

        await self.store.save(checkpoint)
        return checkpoint_id
```

### 检查点存储结构

```python
@dataclass
class Checkpoint:
    """检查点数据结构"""
    id: str
    agent_id: str
    state: bytes              # 序列化的Agent状态
    created_at: datetime
    step_count: int
    metadata: dict = field(default_factory=dict)

    # 检查点链
    parent_checkpoint_id: Optional[str] = None

    # 状态摘要（用于快速浏览）
    summary: Optional[str] = None

    # 恢复信息
    recovery_point: Optional[str] = None  # 恢复点标识


@dataclass
class AgentState:
    """Agent完整状态"""
    # 对话历史
    conversation_history: list[Message]

    # 工具调用结果
    tool_results: list[ToolResult]

    # Agent记忆
    memory: AgentMemory

    # 执行上下文
    context: dict

    # 中间推理状态
    reasoning_state: Optional[dict] = None

    # 自定义元数据
    metadata: dict = field(default_factory=dict)
```

## 状态存储方案

### Redis存储

```python
import redis.asyncio as redis
import pickle

class RedisStateStore:
    """基于Redis的状态存储"""

    def __init__(self, redis_url: str, ttl: int = 3600):
        self.client = redis.from_url(redis_url)
        self.ttl = ttl

    async def save(self, agent_id: str, state: AgentState):
        """保存Agent状态"""
        key = f"agent:state:{agent_id}"
        serialized = pickle.dumps(state)

        await self.client.setex(
            name=key,
            time=self.ttl,
            value=serialized,
        )

        # 保存状态索引
        await self.client.zadd(
            f"agent:checkpoints:{agent_id",
            {state.checkpoint_id: state.step_count},
        )

    async def load(self, agent_id: str) -> Optional[AgentState]:
        """加载Agent状态"""
        key = f"agent:state:{agent_id}"
        data = await self.client.get(key)

        if data is None:
            return None

        return pickle.loads(data)

    async def save_checkpoint(
        self,
        agent_id: str,
        checkpoint: Checkpoint,
    ):
        """保存检查点"""
        key = f"agent:checkpoint:{checkpoint.id}"
        serialized = pickle.dumps(checkpoint)

        await self.client.setex(
            name=key,
            time=86400 * 7,  # 保留7天
            value=serialized,
        )

    async def load_checkpoint(
        self,
        checkpoint_id: str,
    ) -> Optional[Checkpoint]:
        """加载检查点"""
        key = f"agent:checkpoint:{checkpoint_id}"
        data = await self.client.get(key)

        if data is None:
            return None

        return pickle.loads(data)
```

### PostgreSQL存储

```python
from sqlalchemy import Column, String, DateTime, LargeBinary, Integer
from sqlalchemy.ext.asyncio import AsyncSession, create_async_engine
from sqlalchemy.orm import declarative_base

Base = declarative_base()

class AgentStateModel(Base):
    """Agent状态数据库模型"""
    __tablename__ = "agent_states"

    agent_id = Column(String, primary_key=True)
    state_data = Column(LargeBinary, nullable=False)
    step_count = Column(Integer, default=0)
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, onupdate=datetime.utcnow)
    metadata_json = Column(String, default="{}")


class CheckpointModel(Base):
    """检查点数据库模型"""
    __tablename__ = "agent_checkpoints"

    id = Column(String, primary_key=True)
    agent_id = Column(String, index=True)
    state_data = Column(LargeBinary, nullable=False)
    parent_checkpoint_id = Column(String, nullable=True)
    step_count = Column(Integer)
    summary = Column(String, nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow)


class PostgreSQLStateStore:
    """基于PostgreSQL的状态存储"""

    def __init__(self, database_url: str):
        self.engine = create_async_engine(database_url)
        self.session_factory = AsyncSession(self.engine)

    async def save(self, agent_id: str, state: AgentState):
        async with self.session_factory() as session:
            serialized = pickle.dumps(state)

            existing = await session.get(AgentStateModel, agent_id)
            if existing:
                existing.state_data = serialized
                existing.step_count = state.step_count
                existing.updated_at = datetime.utcnow()
            else:
                session.add(AgentStateModel(
                    agent_id=agent_id,
                    state_data=serialized,
                    step_count=state.step_count,
                ))

            await session.commit()

    async def load(self, agent_id: str) -> Optional[AgentState]:
        async with self.session_factory() as session:
            model = await session.get(AgentStateModel, agent_id)
            if model is None:
                return None
            return pickle.loads(model.state_data)

    async def list_checkpoints(
        self,
        agent_id: str,
        limit: int = 20,
    ) -> list[Checkpoint]:
        async with self.session_factory() as session:
            result = await session.execute(
                select(CheckpointModel)
                .where(CheckpointModel.agent_id == agent_id)
                .order_by(CheckpointModel.created_at.desc())
                .limit(limit)
            )
            return [
                pickle.loads(row.state_data) for row in result.scalars()
            ]
```

### S3对象存储

```python
import boto3
import json

class S3StateStore:
    """基于S3的状态存储，适合大规模长期存储"""

    def __init__(self, bucket: str, prefix: str = "agent-states"):
        self.client = boto3.client("s3")
        self.bucket = bucket
        self.prefix = prefix

    async def save(self, agent_id: str, state: AgentState):
        key = f"{self.prefix}/{agent_id}/current/state.pkl"
        serialized = pickle.dumps(state)

        self.client.put_object(
            Bucket=self.bucket,
            Key=key,
            Body=serialized,
            ContentType="application/octet-stream",
            Metadata={
                "agent_id": agent_id,
                "step_count": str(state.step_count),
                "updated_at": datetime.utcnow().isoformat(),
            },
        )

    async def save_checkpoint(
        self,
        agent_id: str,
        checkpoint: Checkpoint,
    ):
        key = f"{self.prefix}/{agent_id}/checkpoints/{checkpoint.id}.pkl"
        serialized = pickle.dumps(checkpoint)

        self.client.put_object(
            Bucket=self.bucket,
            Key=key,
            Body=serialized,
            ContentType="application/octet-stream",
        )

    async def load(self, agent_id: str) -> Optional[AgentState]:
        key = f"{self.prefix}/{agent_id}/current/state.pkl"

        try:
            response = self.client.get_object(
                Bucket=self.bucket,
                Key=key,
            )
            return pickle.loads(response["Body"].read())
        except self.client.exceptions.NoSuchKey:
            return None
```

### 存储方案对比

```
方案对比:

Redis:
  延迟: <1ms
  容量: 受限于内存（通常GB级）
  持久化: 可选RDB/AOF
  适用: 会话级状态、高频读写
  成本: 高（内存成本）

PostgreSQL:
  延迟: 1-10ms
  容量: TB级
  持久化: 原生ACID
  适用: 结构化状态、需要查询
  成本: 中等

S3/对象存储:
  延迟: 50-200ms
  容量: 无限
  持久化: 11个9持久性
  适用: 长期存储、大对象
  成本: 低

推荐组合:
  热状态 → Redis（当前会话）
  温状态 → PostgreSQL（近期历史）
  冷状态 → S3（长期归档）
```

## 状态回放与调试

### 回放引擎

```python
class StateReplayEngine:
    """Agent状态回放引擎，用于调试和分析"""

    def __init__(self, checkpoint_store: CheckpointStore):
        self.checkpoint_store = checkpoint_store

    async def replay(
        self,
        agent_id: str,
        from_checkpoint: Optional[str] = None,
        to_checkpoint: Optional[str] = None,
    ) -> list[ReplayStep]:
        """回放Agent执行过程"""
        checkpoints = await self.checkpoint_store.list_checkpoints(
            agent_id,
        )

        if from_checkpoint:
            start_idx = next(
                i for i, c in enumerate(checkpoints)
                if c.id == from_checkpoint
            )
        else:
            start_idx = 0

        if to_checkpoint:
            end_idx = next(
                i for i, c in enumerate(checkpoints)
                if c.id == to_checkpoint
            )
        else:
            end_idx = len(checkpoints) - 1

        replay_steps = []
        for i in range(start_idx, end_idx + 1):
            checkpoint = checkpoints[i]
            state = pickle.loads(checkpoint.state)

            step = ReplayStep(
                checkpoint_id=checkpoint.id,
                step_number=checkpoint.step_count,
                timestamp=checkpoint.created_at,
                state_snapshot=state,
                summary=checkpoint.summary,
                diff=self._compute_diff(
                    checkpoints[i - 1] if i > 0 else None,
                    checkpoint,
                ),
            )
            replay_steps.append(step)

        return replay_steps

    def _compute_diff(
        self,
        prev_checkpoint: Optional[Checkpoint],
        current_checkpoint: Checkpoint,
    ) -> StateDiff:
        """计算两个检查点之间的差异"""
        if prev_checkpoint is None:
            return StateDiff(
                added_messages=len(
                    pickle.loads(current_checkpoint.state)
                    .conversation_history
                ),
                tool_calls=0,
                state_changes=["initial_state"],
            )

        prev_state = pickle.loads(prev_checkpoint.state)
        curr_state = pickle.loads(current_checkpoint.state)

        return StateDiff(
            added_messages=(
                len(curr_state.conversation_history)
                - len(prev_state.conversation_history)
            ),
            tool_calls=(
                len(curr_state.tool_results)
                - len(prev_state.tool_results)
            ),
            state_changes=self._diff_state_fields(
                prev_state, curr_state
            ),
        )
```

### 调试工具

```python
class AgentDebugger:
    """Agent调试工具"""

    def __init__(self, replay_engine: StateReplayEngine):
        self.replay = replay_engine

    async def inspect_state(
        self,
        agent_id: str,
        checkpoint_id: str,
    ) -> StateInspection:
        """检查特定检查点的状态"""
        checkpoint = await self.replay.checkpoint_store.load_checkpoint(
            checkpoint_id,
        )
        state = pickle.loads(checkpoint.state)

        return StateInspection(
            checkpoint_id=checkpoint_id,
            conversation_length=len(state.conversation_history),
            tool_calls_count=len(state.tool_results),
            memory_size=state.memory.size(),
            token_usage=self._estimate_tokens(state),
            messages_preview=state.conversation_history[-5:],
        )

    async def find_anomaly(
        self,
        agent_id: str,
    ) -> list[Anomaly]:
        """检测执行异常"""
        checkpoints = await self.replay.replay(agent_id)
        anomalies = []

        for i, step in enumerate(checkpoints):
            # 检测重复工具调用
            if i > 0:
                prev = checkpoints[i - 1]
                if (step.state_snapshot.tool_results ==
                    prev.state_snapshot.tool_results):
                    anomalies.append(Anomaly(
                        type="duplicate_tool_call",
                        checkpoint_id=step.checkpoint_id,
                        description="工具调用结果未变化",
                    ))

            # 检测异常长的对话
            if step.state_snapshot.conversation_length > 100:
                anomalies.append(Anomaly(
                    type="long_conversation",
                    checkpoint_id=step.checkpoint_id,
                    description=f"对话长度异常: {step.state_snapshot.conversation_length}",
                ))

        return anomalies
```

## 对话历史压缩

### 压缩策略

```python
from abc import ABC, abstractmethod

class HistoryCompressor(ABC):
    """对话历史压缩器基类"""

    @abstractmethod
    async def compress(
        self,
        messages: list[Message],
        target_length: int,
    ) -> list[Message]:
        pass


class SummaryCompressor(HistoryCompressor):
    """摘要压缩器 - 用LLM生成摘要替代旧消息"""

    def __init__(self, llm_client):
        self.llm = llm_client

    async def compress(
        self,
        messages: list[Message],
        target_length: int,
    ) -> list[Message]:
        if len(messages) <= target_length:
            return messages

        # 保留最近的消息
        recent_messages = messages[-target_length:]
        old_messages = messages[:-target_length]

        # 生成摘要
        summary_prompt = f"""请将以下对话历史压缩为简洁的摘要，
保留关键信息、决策和上下文:

{self._format_messages(old_messages)}

输出格式: 
- 关键决策: ...
- 重要上下文: ...
- 未完成任务: ..."""

        summary_response = await self.llm.chat(
            messages=[{"role": "user", "content": summary_prompt}],
        )

        # 用摘要替代旧消息
        return [
            {
                "role": "system",
                "content": f"[历史对话摘要]\n{summary_response.content}",
            },
            *recent_messages,
        ]


class SlidingWindowCompressor(HistoryCompressor):
    """滑动窗口压缩器 - 保留最近N轮对话"""

    async def compress(
        self,
        messages: list[Message],
        target_length: int,
    ) -> list[Message]:
        return messages[-target_length:]


class ImportanceBasedCompressor(HistoryCompressor):
    """基于重要性的压缩器 - 保留高重要性消息"""

    def __init__(self, importance_scorer):
        self.scorer = importance_scorer

    async def compress(
        self,
        messages: list[Message],
        target_length: int,
    ) -> list[Message]:
        if len(messages) <= target_length:
            return messages

        # 为每条消息评分
        scored_messages = []
        for msg in messages:
            score = await self.scorer.score(msg)
            scored_messages.append((score, msg))

        # 按重要性排序
        scored_messages.sort(key=lambda x: x[0], reverse=True)

        # 保留最重要的消息
        important_messages = [
            msg for _, msg in scored_messages[:target_length]
        ]

        # 按原始顺序排列
        important_messages.sort(
            key=lambda m: messages.index(m)
        )

        return important_messages
```

## 长时记忆分层

### 三层记忆架构

```python
class AgentMemory:
    """分层Agent记忆系统"""

    def __init__(
        self,
        short_term_store: StateStore,
        long_term_store: VectorStore,
        episodic_store: DatabaseStore,
    ):
        self.short_term = ShortTermMemory(short_term_store)
        self.long_term = LongTermMemory(long_term_store)
        self.episodic = EpisodicMemory(episodic_store)

    async def update(self, conversation: list[Message]):
        """更新所有记忆层"""
        # 短期记忆：更新当前会话上下文
        await self.short_term.update(conversation)

        # 长期记忆：提取重要信息存入向量数据库
        important_info = await self._extract_important_info(conversation)
        for info in important_info:
            await self.long_term.store(info)

        # 情景记忆：记录完整交互事件
        await self.episodic.record_episode(conversation)

    async def recall(
        self,
        query: str,
        context: dict,
    ) -> MemoryRecallResult:
        """从所有记忆层召回相关信息"""
        # 短期记忆：最近的对话上下文
        recent = await self.short_term.get_recent(limit=10)

        # 长期记忆：语义相关的历史知识
        relevant = await self.long_term.search(query, top_k=5)

        # 情景记忆：相似的历史场景
        similar_episodes = await self.episodic.find_similar(
            query=query,
            context=context,
            top_k=3,
        )

        return MemoryRecallResult(
            short_term=recent,
            long_term=relevant,
            episodic=similar_episodes,
        )


class ShortTermMemory:
    """短期记忆 - 当前会话上下文"""

    def __init__(self, store: StateStore):
        self.store = store
        self.ttl = 3600  # 1小时TTL

    async def update(self, conversation: list[Message]):
        await self.store.save("short_term", {
            "messages": conversation,
            "updated_at": datetime.utcnow(),
        })

    async def get_recent(self, limit: int) -> list[Message]:
        data = await self.store.load("short_term")
        if data is None:
            return []
        return data["messages"][-limit:]


class LongTermMemory:
    """长期记忆 - 向量化的持久知识"""

    def __init__(self, vector_store: VectorStore):
        self.vector_store = vector_store

    async def store(self, knowledge: KnowledgeUnit):
        embedding = await self._embed(knowledge.text)
        await self.vector_store.upsert(
            id=knowledge.id,
            vector=embedding,
            metadata={
                "text": knowledge.text,
                "source": knowledge.source,
                "importance": knowledge.importance,
                "created_at": datetime.utcnow().isoformat(),
            },
        )

    async def search(
        self,
        query: str,
        top_k: int = 5,
    ) -> list[KnowledgeUnit]:
        embedding = await self._embed(query)
        results = await self.vector_store.query(
            vector=embedding,
            top_k=top_k,
        )
        return [
            KnowledgeUnit(
                id=r.id,
                text=r.metadata["text"],
                score=r.score,
            )
            for r in results
        ]


class EpisodicMemory:
    """情景记忆 - 完整交互事件记录"""

    def __init__(self, db: DatabaseStore):
        self.db = db

    async def record_episode(self, conversation: list[Message]):
        episode = Episode(
            id=str(uuid.uuid4()),
            conversation=conversation,
            summary=await self._generate_summary(conversation),
            timestamp=datetime.utcnow(),
            metadata={
                "turn_count": len(conversation),
                "tool_calls": self._count_tool_calls(conversation),
            },
        )
        await self.db.insert("episodes", episode)

    async def find_similar(
        self,
        query: str,
        context: dict,
        top_k: int = 3,
    ) -> list[Episode]:
        # 基于摘要的语义搜索
        results = await self.db.vector_search(
            collection="episodes",
            query=query,
            filters=context,
            top_k=top_k,
        )
        return results
```

---

*Agent状态管理是构建可靠Agent系统的基础设施，分层记忆架构使Agent具备持续学习和经验积累能力。*

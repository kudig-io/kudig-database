---
title: 记忆管理与上下文窗口工程 (domain-14-ai-ml-infra)
description: 'title: 记忆管理与上下文窗口工程'
summary: 'title: 记忆管理与上下文窗口工程'
category: general
tags:
- ai
- ai-agent
- redis
- postgresql
- llm
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 记忆管理与上下文窗口工程 是什么
- 如何 记忆管理与上下文窗口工程
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 记忆管理与上下文窗口工程
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 记忆管理与上下文窗口工程
description: '# 记忆管理与上下文窗口工程'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
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
- 记忆管理与上下文窗口工程 是什么
- 如何 记忆管理与上下文窗口工程
trigger_keywords:
- 记忆管理与上下文窗口工程
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

# 记忆管理与上下文窗口工程

> **文档类型**: 核心技术专题 | **最后更新**: 2026-03 | **关键词**: 记忆管理, 上下文窗口, 短期记忆, 长期记忆, 情节记忆, 语义记忆, 上下文压缩, Token 管理, 会话记忆, 向量记忆

---

<!-- chunk: 概述 -->## 概述

记忆是 Agent 实现跨会话连续性、避免重复询问用户、积累经验的核心能力。上下文窗口管理则决定了 Agent 在单次对话中能有效利用多少信息。本文系统覆盖 Agent 的四类记忆（感知、工作、情节、语义）、上下文压缩技术、长期记忆的存储与检索架构，以及生产环境中的记忆系统实现。

---

<!-- chunk: 1. Agent 记忆分类体系 -->## 1. Agent 记忆分类体系

```
Agent 记忆体系
│
├── 感知记忆（Sensory Memory）
│   - 最近的原始输入（当前对话轮次）
│   - 极短暂，处理后丢弃
│
├── 工作记忆（Working Memory）
│   - 当前任务的活跃上下文（LLM 上下文窗口）
│   - 工具调用中间结果
│   - 存储形式: LLM 的 messages 列表
│   - 容量: 受 Token 限制（4K~2M tokens）
│
├── 情节记忆（Episodic Memory）
│   - 过去的对话历史和操作记录
│   - "我上次怎么解决这个问题的"
│   - 存储形式: 数据库 + 向量索引
│   - 检索方式: 基于相似度的语义检索
│
└── 语义记忆（Semantic Memory）
    - 结构化的领域知识和事实
    - "K8s 的 Pod 有哪些状态"
    - 存储形式: 知识库（RAG）/ Fine-tuning
    - 来源: kudig-database 等知识库
```

---

<!-- chunk: 2. 工作记忆：上下文窗口管理 -->## 2. 工作记忆：上下文窗口管理

## 2.1 Token 预算规划

```python
# 各模型上下文窗口和推荐配置
CONTEXT_BUDGETS = {
    "gpt-4o": {
        "max_tokens": 128_000,
        "output_reserve": 4_096,  # 为输出预留
        "system_reserve": 2_000,  # 系统提示
        "tool_reserve": 3_000,    # 工具定义
        "usable_for_history": 118_904,
    },
    "claude-3-5-sonnet": {
        "max_tokens": 200_000,
        "output_reserve": 8_192,
        "system_reserve": 2_000,
        "tool_reserve": 3_000,
        "usable_for_history": 186_808,
    },
    "gpt-4o-mini": {
        "max_tokens": 128_000,
        "output_reserve": 4_096,
        "system_reserve": 1_000,
        "tool_reserve": 2_000,
        "usable_for_history": 120_904,
    },
}

class TokenBudgetManager:
    def __init__(self, model: str = "gpt-4o"):
        self.budget = CONTEXT_BUDGETS.get(model, CONTEXT_BUDGETS["gpt-4o"])
        self.encoding = tiktoken.encoding_for_model(model)
    
    def count_tokens(self, text: str) -> int:
        return len(self.encoding.encode(text))
    
    def count_messages_tokens(self, messages: list[dict]) -> int:
        total = 0
        for msg in messages:
            # 每条消息有 4 token 的固定开销
            total += 4
            total += self.count_tokens(str(msg.get("content", "")))
            if "tool_calls" in msg:
                total += self.count_tokens(str(msg["tool_calls"]))
        return total + 2  # 最终 2 token 开销
    
    def available_tokens_for_history(
        self, 
        system_prompt: str, 
        tool_definitions: list
    ) -> int:
        used = (
            self.count_tokens(system_prompt) +
            self.count_tokens(str(tool_definitions)) +
            self.budget["output_reserve"]
        )
        return self.budget["max_tokens"] - used
```

## 2.2 智能上下文截断

```python
from enum import Enum

class TrimStrategy(Enum):
    SLIDING_WINDOW = "sliding_window"      # 保留最近 N 条
    SUMMARY_COMPRESSION = "summary"        # 旧消息压缩为摘要
    IMPORTANCE_BASED = "importance"        # 保留重要消息
    HYBRID = "hybrid"                      # 混合策略

class ContextWindowManager:
    def __init__(
        self,
        model: str = "gpt-4o",
        strategy: TrimStrategy = TrimStrategy.HYBRID,
        summary_llm = None,
    ):
        self.budget_manager = TokenBudgetManager(model)
        self.strategy = strategy
        self.summary_llm = summary_llm
    
    def trim(
        self,
        messages: list[dict],
        system_prompt: str,
        tools: list = None,
    ) -> list[dict]:
        """修剪消息历史，确保不超出 Token 限制"""
        
        available = self.budget_manager.available_tokens_for_history(
            system_prompt, tools or []
        )
        
        current_tokens = self.budget_manager.count_messages_tokens(messages)
        
        if current_tokens <= available:
            return messages  # 不需要修剪
        
        if self.strategy == TrimStrategy.SLIDING_WINDOW:
            return self._sliding_window(messages, available)
        elif self.strategy == TrimStrategy.SUMMARY_COMPRESSION:
            return self._summary_compression(messages, available)
        elif self.strategy == TrimStrategy.IMPORTANCE_BASED:
            return self._importance_based(messages, available)
        else:  # HYBRID
            return self._hybrid_trim(messages, available)
    
    def _sliding_window(
        self, messages: list[dict], available_tokens: int
    ) -> list[dict]:
        """保留最近的消息，超出时从最旧的开始删除"""
        # 始终保留 system 消息
        system_msgs = [m for m in messages if m["role"] == "system"]
        other_msgs = [m for m in messages if m["role"] != "system"]
        
        # 从最新消息开始保留，直到 token 用完
        kept = []
        token_count = 0
        
        for msg in reversed(other_msgs):
            msg_tokens = self.budget_manager.count_messages_tokens([msg])
            if token_count + msg_tokens > available_tokens:
                break
            kept.insert(0, msg)
            token_count += msg_tokens
        
        return system_msgs + kept
    
    def _summary_compression(
        self, messages: list[dict], available_tokens: int
    ) -> list[dict]:
        """将早期对话压缩为摘要"""
        if not self.summary_llm:
            return self._sliding_window(messages, available_tokens)
        
        system_msgs = [m for m in messages if m["role"] == "system"]
        other_msgs = [m for m in messages if m["role"] != "system"]
        
        # 保留最近 1/3 的消息
        recent_count = max(4, len(other_msgs) // 3)
        recent_msgs = other_msgs[-recent_count:]
        old_msgs = other_msgs[:-recent_count]
        
        if not old_msgs:
            return messages
        
        # 压缩旧消息
        summary_prompt = f"""请将以下对话历史压缩为简洁摘要（200字以内），
        保留：关键决策、已执行的操作、发现的问题、重要配置信息：
        
        {self._format_messages_for_summary(old_msgs)}"""
        
        summary = self.summary_llm.invoke(summary_prompt).content
        
        summary_msg = {
            "role": "system",
            "content": f"[历史对话摘要]\n{summary}"
        }
        
        return system_msgs + [summary_msg] + recent_msgs
    
    def _importance_based(
        self, messages: list[dict], available_tokens: int
    ) -> list[dict]:
        """基于重要性保留消息"""
        system_msgs = [m for m in messages if m["role"] == "system"]
        other_msgs = [m for m in messages if m["role"] != "system"]
        
        # 重要性评分
        scored_msgs = []
        for i, msg in enumerate(other_msgs):
            score = self._importance_score(msg, i, len(other_msgs))
            scored_msgs.append((score, i, msg))
        
        # 按重要性排序，但保持时序
        scored_msgs.sort(key=lambda x: x[0], reverse=True)
        
        kept_indices = set()
        token_count = 0
        
        for score, idx, msg in scored_msgs:
            msg_tokens = self.budget_manager.count_messages_tokens([msg])
            if token_count + msg_tokens <= available_tokens:
                kept_indices.add(idx)
                token_count += msg_tokens
        
        # 按原始顺序返回（保持时序）
        kept = [msg for i, msg in enumerate(other_msgs) if i in kept_indices]
        return system_msgs + kept
    
    def _importance_score(self, msg: dict, idx: int, total: int) -> float:
        """计算消息重要性分数"""
        score = 0.0
        
        # 最近的消息更重要
        recency = idx / total
        score += recency * 0.4
        
        content = str(msg.get("content", ""))
        
        # 包含错误信息的消息重要
        if any(keyword in content.lower() for keyword in 
               ["error", "failed", "exception", "warning", "错误", "失败"]):
            score += 0.3
        
        # 工具调用结果重要
        if msg.get("role") == "tool":
            score += 0.2
        
        # 包含关键 K8s 资源的消息重要
        if any(keyword in content for keyword in 
               ["kubectl", "yaml", "apiVersion", "namespace", "Pod"]):
            score += 0.1
        
        return min(score, 1.0)
```

---

<!-- chunk: 3. 情节记忆：跨会话历史 -->## 3. 情节记忆：跨会话历史

## 3.1 情节记忆存储设计

```python
from datetime import datetime, UTC
from dataclasses import dataclass, asdict

@dataclass
class EpisodeRecord:
    """一次对话/操作的完整记录"""
    episode_id: str
    user_id: str
    agent_id: str
    timestamp: str
    summary: str              # 本次对话的摘要（100字以内）
    key_entities: list[str]   # 涉及的关键实体（Pod名、集群名等）
    problem_type: str         # 问题类型（网络/存储/应用）
    outcome: str              # 结果（resolved/escalated/pending）
    actions_taken: list[str]  # 执行的关键操作
    lessons_learned: str      # 经验教训（用于未来检索）
    raw_transcript: str       # 完整对话记录（可选，压缩存储）
    embedding: list[float]    # 向量化后的摘要（用于语义检索）

class EpisodicMemoryStore:
    """基于 PostgreSQL + pgvector 的情节记忆存储"""
    
    def __init__(self, db_url: str, embedding_model):
        self.db_url = db_url
        self.embedding_model = embedding_model
        self._init_db()
    
    def _init_db(self):
        """初始化数据库表"""
        # PostgreSQL with pgvector
        CREATE_TABLE_SQL = """
        CREATE TABLE IF NOT EXISTS episode_memory (
            episode_id VARCHAR PRIMARY KEY,
            user_id VARCHAR NOT NULL,
            agent_id VARCHAR NOT NULL,
            timestamp TIMESTAMPTZ NOT NULL,
            summary TEXT,
            key_entities TEXT[],
            problem_type VARCHAR,
            outcome VARCHAR,
            actions_taken TEXT[],
            lessons_learned TEXT,
            embedding vector(1536),
            created_at TIMESTAMPTZ DEFAULT NOW()
        );
        
        CREATE INDEX IF NOT EXISTS episode_embedding_idx 
        ON episode_memory USING ivfflat (embedding vector_cosine_ops)
        WITH (lists = 100);
        
        CREATE INDEX IF NOT EXISTS episode_user_idx 
        ON episode_memory (user_id, timestamp DESC);
        """
        # 执行建表
    
    def save_episode(self, episode: EpisodeRecord):
        """保存一次对话记录"""
        # 生成 embedding
        embedding_text = f"{episode.summary} {episode.lessons_learned}"
        embedding = self.embedding_model.embed_query(embedding_text)
        episode.embedding = embedding
        
        # 插入数据库
        INSERT_SQL = """
        INSERT INTO episode_memory 
        (episode_id, user_id, agent_id, timestamp, summary, key_entities,
         problem_type, outcome, actions_taken, lessons_learned, embedding)
        VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
        """
        # 执行插入
    
    def search_relevant_episodes(
        self,
        query: str,
        user_id: str = None,
        limit: int = 5,
        problem_type: str = None,
    ) -> list[EpisodeRecord]:
        """语义检索相关历史经验"""
        query_embedding = self.embedding_model.embed_query(query)
        
        # 带过滤的向量相似检索
        SEARCH_SQL = """
        SELECT *, 1 - (embedding <=> $1) AS similarity
        FROM episode_memory
        WHERE 1=1
          {user_filter}
          {type_filter}
          AND outcome = 'resolved'  -- 只检索成功解决的案例
        ORDER BY embedding <=> $1
        LIMIT $2
        """
        # 执行查询并返回结果
```

## 3.2 情节记忆的自动生成

```python
class EpisodeExtractor:
    """从对话历史自动提取结构化情节记忆"""
    
    def __init__(self, llm):
        self.llm = llm
    
    def extract_episode(
        self, 
        messages: list[dict],
        outcome: str
    ) -> EpisodeRecord:
        """从对话记录提取结构化记忆"""
        
        transcript = self._format_transcript(messages)
        
        extraction_result = self.llm.invoke(f"""
        请从以下对话记录中提取结构化信息，以 JSON 格式输出：
        
        {transcript}
        
        提取以下字段：
        - summary: 100字以内的对话摘要
        - key_entities: 涉及的关键实体列表（Pod名、Service名、集群名等）
        - problem_type: 问题类型（network/storage/scheduling/application/security/other）
        - actions_taken: 执行的关键操作列表
        - lessons_learned: 经验教训（下次遇到类似问题时的关键提示，50字以内）
        
        输出格式：
        {{
            "summary": "...",
            "key_entities": ["..."],
            "problem_type": "...",
            "actions_taken": ["..."],
            "lessons_learned": "..."
        }}
        """)
        
        data = json.loads(extraction_result.content)
        
        return EpisodeRecord(
            episode_id=generate_id(),
            user_id=extract_user_id(messages),
            agent_id=self.agent_id,
            timestamp=datetime.now(UTC).isoformat(),
            summary=data["summary"],
            key_entities=data["key_entities"],
            problem_type=data["problem_type"],
            outcome=outcome,
            actions_taken=data["actions_taken"],
            lessons_learned=data["lessons_learned"],
            raw_transcript=transcript,
            embedding=[],  # 在 save 时生成
        )
```

---

<!-- chunk: 4. 语义记忆：结构化知识库集成 -->## 4. 语义记忆：结构化知识库集成

## 4.1 语义记忆 vs RAG 的关系

```
语义记忆（Semantic Memory）与 RAG 的区别：

RAG（检索增强生成）:
  - 来源: 外部知识库文档（如 kudig-database）
  - 粒度: 文档块级别
  - 更新: 更新知识库文档
  - 适合: 大量文档型知识

语义记忆（Semantic Memory）:
  - 来源: Agent 自主学习和总结的知识
  - 粒度: 结构化事实、规则、关系
  - 更新: 通过新经验自动更新
  - 适合: 精炼的领域事实和操作规则

实践建议: 两者配合使用
  - RAG 提供背景知识（知识库）
  - 语义记忆存储 Agent 自己总结的经验规则
```

## 4.2 语义记忆实现

```python
class SemanticMemoryStore:
    """Agent 的语义记忆（结构化知识）"""
    
    def __init__(self, vector_store, llm):
        self.vector_store = vector_store
        self.llm = llm
    
    def learn_from_episode(self, episode: EpisodeRecord):
        """从情节中学习，提取可复用的知识点"""
        if episode.outcome != "resolved":
            return  # 只从成功案例中学习
        
        knowledge_extraction = self.llm.invoke(f"""
        基于以下成功解决的案例，提取1-3条可复用的知识点：
        
        问题类型: {episode.problem_type}
        摘要: {episode.summary}
        解决步骤: {episode.actions_taken}
        经验教训: {episode.lessons_learned}
        
        提取格式（每条知识点）：
        - 触发条件：什么情况下适用
        - 知识内容：具体的规则/方法/结论
        - 置信度：0-1（基于案例的充分程度）
        """)
        
        # 解析并存储知识点
        knowledge_points = self._parse_knowledge(knowledge_extraction.content)
        for kp in knowledge_points:
            self.vector_store.add_texts(
                texts=[kp["content"]],
                metadatas=[{
                    "type": "semantic_memory",
                    "problem_type": episode.problem_type,
                    "trigger": kp["trigger"],
                    "confidence": kp["confidence"],
                    "source_episode": episode.episode_id,
                }]
            )
    
    def recall(self, situation: str, limit: int = 3) -> list[str]:
        """根据当前情况召回相关知识点"""
        results = self.vector_store.similarity_search(
            situation,
            k=limit,
            filter={"type": "semantic_memory"}
        )
        return [r.page_content for r in results]
```

---

<!-- chunk: 5. 完整记忆系统集成 -->## 5. 完整记忆系统集成

```python
class AgentMemorySystem:
    """完整的 Agent 记忆系统（整合四类记忆）"""
    
    def __init__(
        self,
        model: str = "gpt-4o",
        episodic_store: EpisodicMemoryStore = None,
        semantic_store: SemanticMemoryStore = None,
        rag_retriever = None,
        summary_llm = None,
    ):
        self.working_memory = []  # 当前会话消息列表
        self.context_manager = ContextWindowManager(
            model=model,
            strategy=TrimStrategy.HYBRID,
            summary_llm=summary_llm,
        )
        self.episodic_store = episodic_store
        self.semantic_store = semantic_store
        self.rag_retriever = rag_retriever
        self.episode_extractor = EpisodeExtractor(summary_llm) if summary_llm else None
    
    def add_message(self, message: dict):
        """添加新消息到工作记忆"""
        self.working_memory.append(message)
    
    def get_context(
        self,
        system_prompt: str,
        tools: list = None,
        current_query: str = "",
    ) -> list[dict]:
        """
        组装完整的上下文：
        工作记忆 + 相关情节记忆 + 相关语义知识 + RAG 检索结果
        """
        # 1. 检索相关历史经验
        episodic_context = ""
        if self.episodic_store and current_query:
            relevant_episodes = self.episodic_store.search_relevant_episodes(
                current_query, limit=3
            )
            if relevant_episodes:
                episodic_context = "\n[相关历史案例]\n" + "\n".join([
                    f"- {ep.summary}（结论: {ep.lessons_learned}）"
                    for ep in relevant_episodes
                ])
        
        # 2. 检索相关语义知识
        semantic_context = ""
        if self.semantic_store and current_query:
            knowledge_points = self.semantic_store.recall(current_query)
            if knowledge_points:
                semantic_context = "\n[相关知识点]\n" + "\n".join([
                    f"- {kp}" for kp in knowledge_points
                ])
        
        # 3. RAG 检索
        rag_context = ""
        if self.rag_retriever and current_query:
            rag_docs = self.rag_retriever.get_relevant_documents(current_query)
            if rag_docs:
                rag_context = "\n[知识库参考]\n" + "\n\n".join([
                    d.page_content for d in rag_docs[:3]
                ])
        
        # 4. 组合增强的系统提示
        enhanced_system = system_prompt
        if episodic_context or semantic_context or rag_context:
            enhanced_system += f"\n\n{episodic_context}{semantic_context}{rag_context}"
        
        # 5. 修剪工作记忆（确保不超出 Token 限制）
        trimmed_messages = self.context_manager.trim(
            messages=self.working_memory,
            system_prompt=enhanced_system,
            tools=tools,
        )
        
        return [{"role": "system", "content": enhanced_system}] + trimmed_messages
    
    def finalize_session(self, outcome: str = "resolved"):
        """会话结束时，将本次对话转化为情节记忆"""
        if self.episodic_store and self.episode_extractor and self.working_memory:
            episode = self.episode_extractor.extract_episode(
                self.working_memory, outcome
            )
            self.episodic_store.save_episode(episode)
            
            # 从情节中学习，更新语义记忆
            if self.semantic_store and outcome == "resolved":
                self.semantic_store.learn_from_episode(episode)
        
        # 清空工作记忆（新会话开始）
        self.working_memory = []
```

---

<!-- chunk: 6. 记忆系统的隐私与安全 -->## 6. 记忆系统的隐私与安全

```python
class PrivacyAwareMemorySystem(AgentMemorySystem):
    """带隐私保护的记忆系统"""
    
    # PII 检测正则
    PII_PATTERNS = {
        "ip_address": r'\b(?:\d{1,3}\.){3}\d{1,3}\b',
        "api_key": r'(?i)(api[_-]?key|token|secret)["\s:=]+[a-zA-Z0-9+/=]{20,}',
        "password": r'(?i)(password|passwd|pwd)["\s:=]+\S+',
        "email": r'\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b',
    }
    
    def _sanitize_before_storage(self, text: str) -> str:
        """存储前脱敏处理"""
        import re
        sanitized = text
        
        for pii_type, pattern in self.PII_PATTERNS.items():
            sanitized = re.sub(pattern, f'[{pii_type.upper()}_REDACTED]', sanitized)
        
        return sanitized
    
    def save_episode(self, episode: EpisodeRecord):
        """脱敏后再存储"""
        episode.summary = self._sanitize_before_storage(episode.summary)
        episode.lessons_learned = self._sanitize_before_storage(episode.lessons_learned)
        # 完整对话记录不存储（含敏感信息）
        episode.raw_transcript = ""
        super().save_episode(episode)
    
    def user_data_deletion(self, user_id: str):
        """GDPR 合规：用户数据删除权"""
        DELETE_SQL = "DELETE FROM episode_memory WHERE user_id = $1"
        # 执行删除
        
    def get_user_data_export(self, user_id: str) -> list[dict]:
        """GDPR 合规：用户数据导出权"""
        # 返回该用户的所有情节记忆（不含 embedding）
        pass
```

---

<!-- chunk: 7. 记忆系统性能优化 -->## 7. 记忆系统性能优化

## 7.1 Redis 缓存层

```python
import redis
import json
import hashlib

class CachedMemorySystem:
    """带 Redis 缓存的记忆系统"""
    
    def __init__(self, memory_system: AgentMemorySystem, redis_client: redis.Redis):
        self.memory = memory_system
        self.redis = redis_client
        self.cache_ttl = 3600  # 1小时
    
    def get_context_cached(
        self,
        system_prompt: str,
        current_query: str,
        tools: list = None,
    ) -> list[dict]:
        """缓存上下文组装结果（RAG 检索结果变化不频繁）"""
        
        cache_key = hashlib.md5(
            f"{current_query}:{system_prompt[:100]}".encode()
        ).hexdigest()
        
        cached = self.redis.get(f"agent_context:{cache_key}")
        if cached:
            return json.loads(cached)
        
        context = self.memory.get_context(system_prompt, tools, current_query)
        
        # 只缓存不含工作记忆的部分（检索结果）
        # 工作记忆每次都需要实时组装
        
        return context
```

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

## 最佳实践

- **层次化记忆**：短期用工作记忆（上下文窗口），中期用情节记忆，长期用语义记忆
- **按需检索历史**：不要把所有历史都塞入上下文，先检索相关的再注入
- **摘要压缩早于截断**：先尝试摘要压缩，而不是直接删除消息（信息损失更少）
- **脱敏后存储**：情节记忆和语义记忆在存储前必须删除 PII 和密钥信息
- **情节记忆冷启动**：新部署的 Agent 没有历史，应提前导入典型案例作为种子数据

## 反模式

- **无限累积历史**：不管理上下文窗口，随着对话加长推理质量下降、成本飙升
- **丢弃所有历史**：每次新会话完全重置，用户需要重复描述上下文
- **存储原始对话**：未脱敏的原始对话可能包含密码、密钥、PII 等敏感信息
- **不区分记忆类型**：把所有信息都塞进系统提示，而非按类型合理分层
- **情节记忆不过期**：三年前的案例可能已经过时（K8s 版本差异很大），应设置老化机制

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | 上下文窗口在 Agent Loop 中的作用 |
| [04 - RAG 检索](./04-rag-knowledge-retrieval.md) | 语义记忆与 RAG 的结合 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 多 Agent 共享记忆的架构 |
| [11 - 成本优化](./11-cost-latency-optimization.md) | Token 压缩对成本的影响 |
| [domain-14-ai-ml-infra/20-vector-database-rag.md](../domain-14-ai-ml-infra/20-vector-database-rag.md) | 向量数据库选型 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/AI-Agents/README.md|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[domain-14-ai-ml-infra/AI-Agents/01-ai-agent-fundamentals.md|[[AI Agent 基础与核心架构|AI Agent 基础与核心架构]]]]
- [[domain-14-ai-ml-infra/AI-Agents/02-llm-foundation-models.md|[[LLM 基座模型选型与评估|LLM 基座模型选型与评估]]]]
- [[domain-14-ai-ml-infra/AI-Agents/03-agent-frameworks-comparison.md|[[主流 Agent 框架深度对比|主流 Agent 框架深度对比]]]]
- [[domain-14-ai-ml-infra/AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/AI-Agents/11-cost-latency-optimization.md|成本与延迟优化策略]]

## See Also

- 05-tool-use-function-calling
- 06-multi-agent-orchestration
- 08-agent-evaluation-observability
- 09-production-deployment-guide


<!-- risk-assessed -->

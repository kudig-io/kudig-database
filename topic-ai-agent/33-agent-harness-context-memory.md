# Agent Harness 上下文与记忆工程

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Context Engineering, Memory Systems, RAG, 上下文窗口, 信息压缩, 持久化, 向量检索, 短期记忆, 长期记忆, 情景记忆

---

## 概述

Context（上下文层）和 Persistence（持久化层）是 Agent Harness 六层架构的第三和第四层。上下文层决定 Agent 的"视野"——看到什么信息直接决定推理质量；持久化层让 Agent 拥有"记忆"——跨会话保持状态和经验。

**"上下文不对，推理全废"**——这是 Agent 工程中最被低估的真理。同一个模型，给它看错误的信息，输出结论可能完全相反。Context Engineering 正在成为 2026 年 Agent 工程的核心战场。

本文系统阐述上下文构建策略、信息优先级排序、窗口管理、RAG 集成、记忆系统架构，以及在 K8S 运维场景中的完整实现。

---

## 1. 上下文工程核心理论

### 1.1 上下文即决策依据

```
上下文对 Agent 输出的影响（实证数据）:

同一模型 + 同一任务 + 不同上下文:

  上下文 A（精准相关信息）    → 诊断准确率 95%
  上下文 B（信息过载）        → 诊断准确率 60%
  上下文 C（缺少关键信息）    → 诊断准确率 35%
  上下文 D（包含错误信息）    → 诊断准确率 15%

核心结论:
  1. 上下文的质量比模型的能力更重要
  2. 信息过载（noise）和信息缺失（gap）同样致命
  3. 错误信息比没有信息更危险
  4. 上下文构建是工程问题，不是提示词问题
```

### 1.2 信噪比原则

```
上下文信噪比（SNR）优化:

高信号信息（必须包含）:
  ✓ 当前任务直接相关的文档/代码
  ✓ 环境状态（集群信息、配置、版本）
  ✓ 错误日志和关键事件
  ✓ 历史类似问题的解决方案
  ✓ 约束规则和安全边界

低信号信息（应过滤）:
  ✗ 无关的系统日志噪声
  ✗ 重复的成功操作记录
  ✗ 过时的历史信息
  ✗ 与任务无关的知识文档
  ✗ 冗余的元数据

信噪比量化公式:
  SNR = 高信号信息 Token 数 / 总上下文 Token 数
  目标: SNR > 0.7（至少 70% 的上下文是高信号信息）
```

---

## 2. 上下文分层构建架构

### 2.1 四层上下文模型

```
上下文四层模型:

Layer 1: System Context（系统层）
  │  角色定义（SOUL.md）、约束规则、输出格式
  │  优先级: 最高 | 变更频率: 极低
  │
Layer 2: Environment Context（环境层）
  │  集群状态、命名空间列表、节点信息、当前配置
  │  优先级: 高 | 变更频率: 中（每次任务扫描）
  │
Layer 3: Knowledge Context（知识层）
  │  RAG 检索的相关文档、历史相似工单、SOP 流程
  │  优先级: 中 | 变更频率: 每次查询动态构建
  │
Layer 4: History Context（历史层）
  │  当前会话对话历史、执行轨迹、工具输出
  │  优先级: 动态 | 变更频率: 每步更新
  │
Token 预算分配（以 128K 窗口为例）:
  System:      ~5K tokens  (4%)
  Environment: ~10K tokens (8%)
  Knowledge:   ~30K tokens (23%)
  History:     ~40K tokens (31%)
  Reserved:    ~43K tokens (34%, 留给模型输出和推理)
```

### 2.2 上下文管理器完整实现

```python
from dataclasses import dataclass, field
from typing import Optional, Any
import tiktoken

@dataclass
class ContextBudget:
    """上下文 Token 预算"""
    total: int = 128000
    system: int = 5000
    environment: int = 10000
    knowledge: int = 30000
    history: int = 40000
    reserved: int = 43000  # 模型输出保留

class ContextManager:
    """上下文管理器：分层构建、优先级排序、动态压缩"""

    def __init__(
        self,
        budget: ContextBudget = None,
        rag_retriever=None,
        encoder_name: str = "cl100k_base",
    ):
        self.budget = budget or ContextBudget()
        self.rag = rag_retriever
        self.encoder = tiktoken.get_encoding(encoder_name)

    def count_tokens(self, text: str) -> int:
        """精确计算 token 数"""
        return len(self.encoder.encode(text))

    def build_context(
        self,
        task: str,
        system_prompt: str = None,
        environment: dict = None,
        history: list = None,
        additional_context: dict = None,
    ) -> str:
        """分层构建上下文"""
        context_parts = []
        remaining_budget = self.budget.total - self.budget.reserved

        # Layer 1: 系统上下文
        if system_prompt:
            system_text = self._format_system(system_prompt)
            system_tokens = self.count_tokens(system_text)
            if system_tokens <= self.budget.system:
                context_parts.append(("system", system_text, system_tokens))
                remaining_budget -= system_tokens

        # Layer 2: 环境上下文
        if environment:
            env_text = self._format_environment(environment)
            env_tokens = self.count_tokens(env_text)
            env_budget = min(self.budget.environment, remaining_budget)
            if env_tokens > env_budget:
                env_text = self._compress_environment(env_text, env_budget)
                env_tokens = self.count_tokens(env_text)
            context_parts.append(("environment", env_text, env_tokens))
            remaining_budget -= env_tokens

        # Layer 3: 知识上下文（RAG 检索）
        if self.rag:
            knowledge_budget = min(self.budget.knowledge, remaining_budget)
            knowledge_text = self._retrieve_knowledge(task, knowledge_budget)
            knowledge_tokens = self.count_tokens(knowledge_text)
            context_parts.append(("knowledge", knowledge_text, knowledge_tokens))
            remaining_budget -= knowledge_tokens

        # Layer 4: 历史上下文
        if history:
            history_budget = min(self.budget.history, remaining_budget)
            history_text = self._compress_history(history, history_budget)
            history_tokens = self.count_tokens(history_text)
            context_parts.append(("history", history_text, history_tokens))
            remaining_budget -= history_tokens

        # 组装最终上下文
        return self._assemble(context_parts)

    def _format_system(self, system_prompt: str) -> str:
        """格式化系统上下文"""
        return f"## 系统指令\n\n{system_prompt}"

    def _format_environment(self, environment: dict) -> str:
        """格式化环境上下文"""
        parts = ["## 当前环境"]
        if "cluster" in environment:
            parts.append(f"集群: {environment['cluster']}")
        if "kubernetes_version" in environment:
            parts.append(f"K8S 版本: {environment['kubernetes_version']}")
        if "nodes" in environment:
            parts.append(f"节点数: {len(environment['nodes'])}")
            for node in environment["nodes"][:5]:  # 最多展示 5 个
                parts.append(f"  - {node['name']}: {node.get('status', 'Unknown')}")
        if "namespaces" in environment:
            parts.append(f"活跃命名空间: {', '.join(environment['namespaces'][:10])}")
        return "\n".join(parts)

    def _retrieve_knowledge(self, task: str, budget: int) -> str:
        """RAG 知识检索"""
        documents = self.rag.retrieve(task, top_k=10)

        parts = ["## 相关知识"]
        current_tokens = self.count_tokens("## 相关知识\n")

        for doc in documents:
            doc_text = f"\n### {doc['title']}\n{doc['content']}\n"
            doc_tokens = self.count_tokens(doc_text)
            if current_tokens + doc_tokens > budget:
                # 尝试截断文档
                available = budget - current_tokens - 50
                if available > 200:
                    truncated = self._truncate_to_tokens(doc['content'], available)
                    parts.append(f"\n### {doc['title']}\n{truncated}\n...")
                break
            parts.append(doc_text)
            current_tokens += doc_tokens

        return "\n".join(parts)

    def _compress_history(self, history: list, budget: int) -> str:
        """智能历史压缩"""
        parts = ["## 执行历史"]
        current_tokens = self.count_tokens("## 执行历史\n")

        # 策略 1: 关键步骤始终保留
        key_steps = [h for h in history if h.get("is_key_step")]
        # 策略 2: 错误步骤始终保留
        error_steps = [h for h in history if h.get("error")]
        # 策略 3: 最近 N 步始终保留
        recent_steps = history[-3:]

        # 合并去重（保持时序）
        must_keep = set()
        for step in key_steps + error_steps + recent_steps:
            must_keep.add(id(step))

        # 优先添加必须保留的步骤
        for step in history:
            if id(step) in must_keep:
                step_text = self._format_step(step)
                step_tokens = self.count_tokens(step_text)
                if current_tokens + step_tokens > budget:
                    break
                parts.append(step_text)
                current_tokens += step_tokens

        # 如果还有预算，添加其他步骤的摘要
        remaining_steps = [h for h in history if id(h) not in must_keep]
        if remaining_steps and current_tokens < budget - 200:
            summary = f"\n[已省略 {len(remaining_steps)} 个中间步骤]"
            parts.append(summary)

        return "\n".join(parts)

    def _format_step(self, step: dict) -> str:
        """格式化单步记录"""
        parts = [f"\n### Step {step.get('iteration', '?')}"]
        if step.get("thought"):
            parts.append(f"思考: {step['thought'][:200]}")
        if step.get("action"):
            parts.append(f"动作: {step['action']}")
        if step.get("tool_result"):
            result = str(step["tool_result"])[:300]
            parts.append(f"结果: {result}")
        if step.get("error"):
            parts.append(f"错误: {step['error']}")
        return "\n".join(parts)

    def _truncate_to_tokens(self, text: str, max_tokens: int) -> str:
        """将文本截断到指定 token 数"""
        tokens = self.encoder.encode(text)
        if len(tokens) <= max_tokens:
            return text
        return self.encoder.decode(tokens[:max_tokens])

    def _assemble(self, context_parts: list) -> str:
        """组装最终上下文"""
        parts = []
        for name, text, tokens in context_parts:
            parts.append(text)
        return "\n\n---\n\n".join(parts)
```

---

## 3. RAG 集成深度设计

### 3.1 知识库索引架构

```
K8S 运维知识库索引架构:

数据源:
  ├── kudig-database 文档（950+ Markdown 文件）
  ├── Kubernetes 官方文档
  ├── 历史工单记录
  ├── SOP 操作手册
  └── 告警规则与处理指南

索引流水线:
  文档 → 分块（Chunking）→ 嵌入（Embedding）→ 向量存储（Vector Store）

分块策略:
  ├── 文档级分块: 按 ## 标题分割，保持逻辑完整性
  ├── 段落级分块: 500-1000 tokens/chunk，重叠 100 tokens
  ├── 代码块分块: 完整代码块作为独立 chunk
  └── 表格分块: 表格 + 上下文说明作为独立 chunk

检索策略:
  ├── 语义检索: Embedding 相似度 Top-K
  ├── 关键词检索: BM25 全文搜索
  ├── 混合检索: 语义 + 关键词加权融合
  └── 重排序: Cross-encoder 精排
```

### 3.2 RAG 检索器实现

```python
from dataclasses import dataclass
from typing import Optional

@dataclass
class Document:
    """文档模型"""
    id: str
    title: str
    content: str
    source: str
    category: str
    metadata: dict = None
    score: float = 0.0

class HybridRAGRetriever:
    """混合 RAG 检索器：语义 + 关键词 + 重排序"""

    def __init__(
        self,
        vector_store,
        bm25_index,
        reranker=None,
        semantic_weight: float = 0.6,
        keyword_weight: float = 0.4,
    ):
        self.vector_store = vector_store
        self.bm25_index = bm25_index
        self.reranker = reranker
        self.semantic_weight = semantic_weight
        self.keyword_weight = keyword_weight

    def retrieve(
        self,
        query: str,
        top_k: int = 5,
        category_filter: str = None,
        min_score: float = 0.3,
    ) -> list[Document]:
        """混合检索"""
        # Stage 1: 粗排——语义检索 + 关键词检索
        semantic_results = self.vector_store.search(
            query, top_k=top_k * 3, filter={"category": category_filter}
        )
        keyword_results = self.bm25_index.search(
            query, top_k=top_k * 3
        )

        # Stage 2: 分数融合（Reciprocal Rank Fusion）
        fused = self._reciprocal_rank_fusion(
            semantic_results, keyword_results,
            weights=[self.semantic_weight, self.keyword_weight],
        )

        # Stage 3: 精排（Cross-encoder Reranking）
        if self.reranker:
            fused = self.reranker.rerank(query, fused, top_k=top_k)
        else:
            fused = fused[:top_k]

        # Stage 4: 过滤低分结果
        return [doc for doc in fused if doc.score >= min_score]

    def _reciprocal_rank_fusion(
        self,
        *result_lists,
        weights: list[float] = None,
        k: int = 60,
    ) -> list[Document]:
        """Reciprocal Rank Fusion (RRF) 分数融合"""
        if weights is None:
            weights = [1.0] * len(result_lists)

        doc_scores: dict[str, float] = {}
        doc_map: dict[str, Document] = {}

        for results, weight in zip(result_lists, weights):
            for rank, doc in enumerate(results):
                rrf_score = weight / (k + rank + 1)
                doc_scores[doc.id] = doc_scores.get(doc.id, 0) + rrf_score
                doc_map[doc.id] = doc

        # 按融合分数排序
        sorted_ids = sorted(doc_scores.keys(), key=lambda x: doc_scores[x], reverse=True)
        result = []
        for doc_id in sorted_ids:
            doc = doc_map[doc_id]
            doc.score = doc_scores[doc_id]
            result.append(doc)

        return result


class ContextAwareRetriever:
    """上下文感知检索器：根据任务阶段调整检索策略"""

    def __init__(self, base_retriever: HybridRAGRetriever):
        self.base = base_retriever

    def retrieve_for_phase(
        self,
        query: str,
        phase: str,
        existing_context: str = "",
    ) -> list[Document]:
        """根据执行阶段调整检索"""
        if phase == "gather":
            # 信息收集阶段: 广泛检索
            return self.base.retrieve(query, top_k=8, min_score=0.2)
        elif phase == "analyze":
            # 分析阶段: 精确检索 + 排除已有信息
            docs = self.base.retrieve(query, top_k=5, min_score=0.5)
            return self._filter_redundant(docs, existing_context)
        elif phase == "act":
            # 执行阶段: 只检索 SOP 和操作指南
            return self.base.retrieve(
                query, top_k=3, category_filter="sop", min_score=0.4
            )
        return self.base.retrieve(query, top_k=5)

    def _filter_redundant(self, docs: list, existing_context: str) -> list:
        """过滤与已有上下文重复的文档"""
        filtered = []
        for doc in docs:
            # 简单去重：检查文档标题是否已在上下文中
            if doc.title not in existing_context:
                filtered.append(doc)
        return filtered
```

---

## 4. 记忆系统架构

### 4.1 三层记忆模型

```
Agent 记忆三层模型:

1. 短期记忆（Short-term Memory / Working Memory）
   │  当前会话的对话历史和执行轨迹
   │  生命周期: 单次会话
   │  存储: 内存
   │  用途: 保持对话连贯性
   │
2. 情景记忆（Episodic Memory）
   │  历史任务的完整执行记录
   │  生命周期: 持久存储，可检索
   │  存储: 向量数据库 + 关系数据库
   │  用途: 从历史经验中学习
   │
3. 语义记忆（Semantic Memory）
   │  提炼的知识、规则、模式
   │  生命周期: 永久存储，定期更新
   │  存储: 知识图谱 + 向量数据库
   │  用途: 提供领域知识

记忆流转:
  短期记忆 ──(任务完成后提取)──→ 情景记忆
  情景记忆 ──(模式提炼)──→ 语义记忆
  语义记忆 ──(检索注入)──→ 短期记忆
```

### 4.2 记忆系统完整实现

```python
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from typing import Optional, Any
import json

@dataclass
class MemoryEntry:
    """记忆条目"""
    id: str
    content: str
    memory_type: str        # short_term / episodic / semantic
    created_at: str
    task_id: Optional[str] = None
    tags: list = field(default_factory=list)
    importance: float = 0.5  # 0.0 - 1.0
    access_count: int = 0
    last_accessed: Optional[str] = None
    metadata: dict = field(default_factory=dict)

class MemorySystem:
    """Agent 记忆系统"""

    def __init__(self, vector_store, kv_store, max_short_term: int = 50):
        self.vector_store = vector_store
        self.kv_store = kv_store
        self.max_short_term = max_short_term
        self._short_term: list[MemoryEntry] = []

    # === 短期记忆 ===

    def add_to_short_term(self, content: str, importance: float = 0.5,
                          metadata: dict = None):
        """添加短期记忆"""
        entry = MemoryEntry(
            id=f"st_{len(self._short_term)}",
            content=content,
            memory_type="short_term",
            created_at=datetime.utcnow().isoformat(),
            importance=importance,
            metadata=metadata or {},
        )
        self._short_term.append(entry)

        # 容量管理：超出限制时淘汰低重要性记忆
        if len(self._short_term) > self.max_short_term:
            self._evict_short_term()

    def get_short_term(self, last_n: int = None) -> list[MemoryEntry]:
        """获取短期记忆"""
        if last_n:
            return self._short_term[-last_n:]
        return self._short_term

    def _evict_short_term(self):
        """短期记忆淘汰策略：保留高重要性 + 最近的"""
        # 保留重要性 > 0.7 的 + 最近 10 条
        important = [m for m in self._short_term if m.importance > 0.7]
        recent = self._short_term[-10:]
        keep = list({id(m): m for m in important + recent}.values())
        keep.sort(key=lambda m: m.created_at)
        self._short_term = keep[:self.max_short_term]

    # === 情景记忆 ===

    def save_episode(self, task_id: str, task: str, trajectory: list,
                     result: dict):
        """保存任务执行的情景记忆"""
        episode = {
            "task_id": task_id,
            "task": task,
            "result_status": result.get("status"),
            "answer": result.get("answer", "")[:500],
            "steps": len(trajectory),
            "key_findings": self._extract_key_findings(trajectory),
            "errors_encountered": self._extract_errors(trajectory),
            "tools_used": self._extract_tools(trajectory),
            "timestamp": datetime.utcnow().isoformat(),
        }

        # 存入向量数据库（支持语义检索）
        embedding_text = f"任务: {task}\n结果: {result.get('answer', '')[:200]}"
        self.vector_store.upsert(
            id=task_id,
            text=embedding_text,
            metadata=episode,
        )

        # 存入 KV 存储（支持精确查询）
        self.kv_store.set(f"episode:{task_id}", json.dumps(episode))

    def recall_similar_episodes(self, task: str, top_k: int = 3) -> list:
        """检索相似的历史任务"""
        results = self.vector_store.search(task, top_k=top_k)
        episodes = []
        for r in results:
            episodes.append({
                "task": r.metadata.get("task", ""),
                "result": r.metadata.get("result_status", ""),
                "key_findings": r.metadata.get("key_findings", []),
                "tools_used": r.metadata.get("tools_used", []),
                "similarity": r.score,
            })
        return episodes

    # === 语义记忆 ===

    def store_semantic(self, knowledge: str, tags: list, importance: float = 0.8):
        """存储语义记忆（提炼的知识）"""
        entry = MemoryEntry(
            id=f"sem_{datetime.utcnow().timestamp()}",
            content=knowledge,
            memory_type="semantic",
            created_at=datetime.utcnow().isoformat(),
            tags=tags,
            importance=importance,
        )
        self.vector_store.upsert(
            id=entry.id,
            text=knowledge,
            metadata={"type": "semantic", "tags": tags,
                       "importance": importance},
        )

    def recall_semantic(self, query: str, tags: list = None,
                        top_k: int = 5) -> list:
        """检索语义记忆"""
        filter_dict = {"type": "semantic"}
        if tags:
            filter_dict["tags"] = {"$in": tags}
        return self.vector_store.search(query, top_k=top_k, filter=filter_dict)

    # === 记忆提炼 ===

    def consolidate(self, llm, recent_episodes: int = 20):
        """记忆巩固：从近期情景记忆中提炼语义记忆"""
        episodes = self._get_recent_episodes(recent_episodes)
        if not episodes:
            return

        prompt = f"""
        分析以下 {len(episodes)} 个历史任务执行记录，
        提炼出可复用的模式和知识:
        
        {json.dumps(episodes, ensure_ascii=False, indent=2)}
        
        请输出:
        1. 常见故障模式及解决方案
        2. 高效的工具使用策略
        3. 需要注意的陷阱和反模式
        """
        insights = llm.invoke(prompt)
        self.store_semantic(
            insights,
            tags=["consolidated", "pattern"],
            importance=0.9,
        )

    # === 辅助方法 ===

    def _extract_key_findings(self, trajectory: list) -> list:
        """从轨迹中提取关键发现"""
        findings = []
        for step in trajectory:
            if step.get("is_key_step"):
                findings.append(step.get("thought", "")[:100])
        return findings

    def _extract_errors(self, trajectory: list) -> list:
        """从轨迹中提取错误"""
        return [
            step.get("error", "")[:100]
            for step in trajectory
            if step.get("error")
        ]

    def _extract_tools(self, trajectory: list) -> list:
        """从轨迹中提取使用的工具列表"""
        tools = set()
        for step in trajectory:
            if step.get("tool_name"):
                tools.add(step["tool_name"])
        return list(tools)

    def _get_recent_episodes(self, n: int) -> list:
        """获取最近 N 条情景记忆"""
        # 从 KV 存储中获取（按时间倒序）
        keys = self.kv_store.keys("episode:*")
        recent_keys = sorted(keys, reverse=True)[:n]
        return [json.loads(self.kv_store.get(k)) for k in recent_keys]
```

---

## 5. 上下文窗口管理

### 5.1 动态窗口策略

```python
class DynamicWindowManager:
    """动态上下文窗口管理器

    根据任务复杂度和执行阶段动态调整各层的 Token 预算。
    """

    def __init__(self, total_window: int = 128000):
        self.total = total_window
        self.reserved_for_output = int(total_window * 0.25)

    def allocate(self, task_complexity: str, phase: str,
                 history_length: int) -> ContextBudget:
        """动态分配上下文预算"""
        available = self.total - self.reserved_for_output

        if task_complexity == "simple":
            return ContextBudget(
                total=self.total,
                system=3000,
                environment=5000,
                knowledge=int(available * 0.3),
                history=int(available * 0.2),
                reserved=self.reserved_for_output,
            )
        elif task_complexity == "complex":
            # 复杂任务: 更多知识和历史
            return ContextBudget(
                total=self.total,
                system=5000,
                environment=10000,
                knowledge=int(available * 0.35),
                history=int(available * 0.3),
                reserved=self.reserved_for_output,
            )
        else:  # multi-step
            # 多步任务: 根据阶段调整
            if phase == "gather":
                knowledge_ratio = 0.4
                history_ratio = 0.15
            elif phase == "analyze":
                knowledge_ratio = 0.3
                history_ratio = 0.35
            else:  # act
                knowledge_ratio = 0.15
                history_ratio = 0.2

            return ContextBudget(
                total=self.total,
                system=5000,
                environment=8000,
                knowledge=int(available * knowledge_ratio),
                history=int(available * history_ratio),
                reserved=self.reserved_for_output,
            )
```

### 5.2 增量上下文更新

```python
class IncrementalContextUpdater:
    """增量上下文更新器：避免每步重建完整上下文"""

    def __init__(self, context_manager: ContextManager):
        self.ctx_mgr = context_manager
        self._cached_system: str = ""
        self._cached_environment: str = ""
        self._cached_knowledge: str = ""
        self._history_buffer: list = []

    def initial_build(self, task: str, system_prompt: str,
                      environment: dict) -> str:
        """初始构建（第一步）"""
        self._cached_system = self.ctx_mgr._format_system(system_prompt)
        self._cached_environment = self.ctx_mgr._format_environment(environment)
        if self.ctx_mgr.rag:
            self._cached_knowledge = self.ctx_mgr._retrieve_knowledge(
                task, self.ctx_mgr.budget.knowledge
            )
        return self._assemble()

    def update_after_step(self, step: dict) -> str:
        """步骤执行后增量更新（只更新历史层）"""
        self._history_buffer.append(step)

        # 历史压缩（超过预算时压缩）
        history_text = self.ctx_mgr._compress_history(
            self._history_buffer, self.ctx_mgr.budget.history
        )

        return self._assemble(history_override=history_text)

    def refresh_knowledge(self, new_query: str) -> str:
        """知识层刷新（当任务方向改变时）"""
        if self.ctx_mgr.rag:
            self._cached_knowledge = self.ctx_mgr._retrieve_knowledge(
                new_query, self.ctx_mgr.budget.knowledge
            )
        return self._assemble()

    def _assemble(self, history_override: str = None) -> str:
        """组装上下文"""
        parts = [self._cached_system, self._cached_environment]
        if self._cached_knowledge:
            parts.append(self._cached_knowledge)
        if history_override:
            parts.append(history_override)
        elif self._history_buffer:
            parts.append(self.ctx_mgr._compress_history(
                self._history_buffer, self.ctx_mgr.budget.history
            ))
        return "\n\n---\n\n".join(parts)
```

---

## 6. K8S 运维上下文模板

### 6.1 集群环境扫描器

```python
class K8sEnvironmentScanner:
    """K8S 集群环境扫描器：自动收集环境上下文"""

    def __init__(self, kubectl_tool):
        self.kubectl = kubectl_tool

    def scan(self) -> dict:
        """全面扫描集群环境"""
        env = {}

        # 基础信息
        env["cluster_info"] = self._get_cluster_info()
        env["kubernetes_version"] = self._get_version()

        # 节点信息
        env["nodes"] = self._get_node_summary()

        # 命名空间
        env["namespaces"] = self._get_namespaces()

        # 资源使用概览
        env["resource_usage"] = self._get_resource_overview()

        # 近期告警事件
        env["recent_warnings"] = self._get_recent_warnings()

        return env

    def _get_node_summary(self) -> list:
        """获取节点摘要"""
        result = self.kubectl.execute(resource="nodes", output="wide")
        nodes = []
        for line in result.get("output", "").split("\n")[1:]:
            parts = line.split()
            if len(parts) >= 5:
                nodes.append({
                    "name": parts[0],
                    "status": parts[1],
                    "roles": parts[2],
                    "version": parts[4],
                })
        return nodes

    def _get_recent_warnings(self, limit: int = 20) -> list:
        """获取近期告警事件"""
        result = self.kubectl.execute(
            resource="events",
            namespace="--all-namespaces",
            extra_args="--field-selector type=Warning --sort-by=.lastTimestamp",
        )
        warnings = []
        for line in result.get("output", "").split("\n")[1:limit + 1]:
            if line.strip():
                warnings.append(line.strip())
        return warnings

    def format_for_context(self, env: dict) -> str:
        """将环境信息格式化为上下文文本"""
        parts = [
            "## 集群环境信息",
            f"K8S 版本: {env.get('kubernetes_version', 'Unknown')}",
            f"节点数量: {len(env.get('nodes', []))}",
        ]

        # 节点状态摘要
        nodes = env.get("nodes", [])
        ready_count = sum(1 for n in nodes if n.get("status") == "Ready")
        parts.append(f"节点状态: {ready_count}/{len(nodes)} Ready")

        if env.get("recent_warnings"):
            parts.append("\n### 近期告警事件")
            for w in env["recent_warnings"][:10]:
                parts.append(f"  - {w}")

        return "\n".join(parts)
```

### 6.2 诊断任务上下文模板

```python
class DiagnosisContextTemplate:
    """诊断任务上下文模板"""

    SYSTEM_PROMPT_TEMPLATE = """
你是 K8S 运维诊断专家 Agent。你的任务是根据提供的集群环境信息和工具输出，
诊断 Kubernetes 集群中的问题。

## 工作原则
1. 每个诊断结论必须有具体的 Event 或日志证据支撑
2. 优先使用只读命令收集信息
3. 不确定的结论标注"需人工确认"
4. 输出的 YAML/命令必须语法正确

## 输出格式
- 根因分析: [具体原因]
- 证据: [Event/日志引用]
- 建议操作: [操作步骤]
- 风险等级: [高/中/低]
- 置信度: [0-100%]
"""

    def build_diagnosis_context(
        self,
        task: str,
        env_scan: dict,
        knowledge: list,
        history: list = None,
    ) -> str:
        """构建诊断任务的完整上下文"""
        parts = [
            self.SYSTEM_PROMPT_TEMPLATE,
            f"\n## 当前诊断任务\n{task}",
            self._format_env(env_scan),
        ]

        if knowledge:
            parts.append("\n## 相关知识\n")
            for doc in knowledge[:5]:
                parts.append(f"### {doc['title']}\n{doc['content'][:500]}\n")

        if history:
            parts.append("\n## 已执行步骤\n")
            for step in history[-5:]:
                parts.append(f"Step {step.get('iteration')}: "
                           f"{step.get('thought', '')[:150]}")

        return "\n".join(parts)

    def _format_env(self, env: dict) -> str:
        """格式化环境信息"""
        scanner = K8sEnvironmentScanner(None)
        return scanner.format_for_context(env)
```

---

## 7. 最佳实践

### 7.1 上下文工程核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **信噪比优先** | 上下文中高信号信息占比 > 70% | 严格过滤无关信息 |
| **分层构建** | 系统→环境→知识→历史四层分明 | 每层独立管理，动态调整 |
| **Token 预算** | 每层分配明确的 Token 预算 | 使用 ContextBudget 管控 |
| **增量更新** | 避免每步重建完整上下文 | 缓存不变层，只更新变化层 |
| **智能压缩** | 历史信息保留关键步骤 | 错误步骤 + 关键发现 + 最近 N 步 |
| **环境预扫描** | 任务开始前收集环境信息 | 使用 EnvironmentScanner |

### 7.2 记忆系统核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **三层分离** | 短期/情景/语义记忆独立管理 | 不同存储后端，不同生命周期 |
| **自动提炼** | 从情景记忆中自动提炼语义记忆 | 定期运行 consolidate |
| **相关性检索** | 根据当前任务检索相关历史 | 使用向量相似度检索 |
| **容量管控** | 短期记忆有上限 | 淘汰低重要性记忆 |
| **隐私保护** | 记忆中不存储敏感信息 | 存储前脱敏处理 |

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 六层架构中的 Context 层和 Persistence 层定义 |
| [31 - Loop 与执行引擎](./31-agent-harness-loop-execution.md) | 上下文在 Loop 中的使用流程 |
| [04 - RAG 知识检索](./04-rag-knowledge-retrieval.md) | RAG 基础理论和实现 |
| [07 - 记忆管理](./07-memory-context-management.md) | Agent 记忆系统基础概念 |

---

## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Anthropic | Context Engineering 最佳实践 | 2026-02 |
| LangChain | 上下文管理对 Agent 性能的影响实验 | 2026-02 |
| Simon Willison | Context Engineering vs Prompt Engineering | 2026-01 |
| Microsoft | AutoGen 记忆系统设计 | 2025-2026 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 上下文与记忆工程。*

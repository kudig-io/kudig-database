---
title: "Feature Store × RAG × ML 平台"
summary: "Feature Store 提供在线/离线特征服务，RAG 检索增强生成依赖向量索引与实时特征，两者在 ML 平台中融合为统一的'上下文服务层'"
category: synthesis
tags:
- feature-store
- rag
- ml-platform
- feast
- vector-database
- online-features
- retrieval-augmented-generation
tier: supporting
sources:
- 概念/k8s-ai-ml-infrastructure.md
- 概念/ai-ml-observability.md
- 概念/gpu-scheduling-ai-workloads.md
- 实体/kubeflow.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# Feature Store × RAG × ML 平台

## The Connection（为什么这两个领域交叉）

传统 ML 系统中，Feature Store 解决"训练和推理使用相同特征"的一致性问题——离线训练用批量计算的特征，在线推理用实时计算的特征，两者不一致（training-serving skew）是模型效果退化的首要原因。Feast、Tecton 等 Feature Store 提供统一的特征定义、离线物化和在线服务。

RAG（Retrieval-Augmented Generation）是 LLM 应用的核心架构模式：将外部知识（文档、数据库、API）通过向量检索注入 Prompt，使 LLM 能回答训练数据之外的问题。RAG 的"检索"本质上是一种"特征获取"——从外部知识源获取上下文特征，与 Feature Store 从特征仓库获取用户/物品特征在架构上同构。

交叉点在于：ML 平台正在从"传统 ML 特征服务"演进为"AI 上下文服务层"——既提供结构化特征（用户画像、物品属性），也提供非结构化上下文（文档片段、知识库条目）。Feature Store 的在线/离线一致性保证、特征版本化、特征监控等能力，同样适用于 RAG 的检索质量管理。两者在 ML 平台中融合为统一的"上下文即服务"（Context-as-a-Service）架构。

## Where They Co-occur（生产中的交叉场景）

### 场景一：推荐系统的特征 + 检索融合

电商推荐系统需要：用户实时特征（最近浏览、购物车）→ Feature Store 在线服务；商品语义向量 → 向量数据库检索；用户历史偏好 → 离线特征。三者融合为推荐模型的输入。Feature Store 提供结构化特征，向量检索提供语义匹配，共同构成"上下文"。

### 场景二：智能客服的 RAG + 用户特征

客服 AI 需要：用户账户信息（Feature Store：会员等级、历史工单、偏好）；产品知识库（RAG：产品文档、FAQ、政策）；对话上下文（会话管理）。Feature Store 提供个性化特征，RAG 提供知识上下文，LLM 综合生成回答。

### 场景三：风控系统的实时特征 + 知识检索

金融风控需要：用户实时交易特征（Feature Store：最近 5 分钟交易次数、金额分布）；欺诈模式知识库（RAG：已知欺诈手法文档）；规则引擎结果。Feature Store 的在线服务延迟要求 <10ms（实时决策），RAG 检索延迟可容忍 <100ms。

### 场景四：特征与文档的版本一致性

模型训练时使用特定版本的特征和文档。如果特征定义变了（如"月活用户"的计算口径变了）或知识库更新了（如政策文档修改），模型效果可能退化。Feature Store 的特征版本化 + RAG 的文档版本化共同保证训练-推理一致性。

### 场景五：在线/离线特征一致性监控

Training-serving skew 是 ML 系统的隐性杀手。Feature Store 通过统一特征定义（同一份特征计算逻辑用于离线和在线）减少 skew。RAG 中类似问题：训练时用的检索结果 vs 推理时的检索结果不一致（索引更新、embedding 模型变化）。需要监控两者的分布差异。

### 场景六：ML 平台的统一服务层

ML 平台（Kubeflow/SageMaker/自建）提供统一的模型服务入口，背后路由到：Feature Store（获取用户/物品特征）→ 向量数据库（检索相关文档）→ 模型推理（生成预测/回答）→ 后处理（格式化、安全过滤）。Feature Store 和 RAG 是推理链路中的两个"上下文获取"步骤。

## Production Patterns（生产模式与架构）

### 模式一：统一上下文服务架构

```
┌─────────────────────────────────────────────────────────┐
│  Unified Context Service Layer                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Request: user_id + query                               │
│       │                                                 │
│       ├──→ Feature Store (在线服务)                     │
│       │    ├── 用户特征: 会员等级、偏好、历史行为       │
│       │    ├── 实时特征: 最近 5min 行为、当前会话       │
│       │    └── 延迟要求: < 10ms (P99)                  │
│       │                                                 │
│       ├──→ Vector Store (RAG 检索)                      │
│       │    ├── 知识库检索: 产品文档、FAQ、政策          │
│       │    ├── 语义搜索: query embedding → top-K        │
│       │    └── 延迟要求: < 100ms (P99)                 │
│       │                                                 │
│       ├──→ 实时数据源 (可选)                            │
│       │    ├── 库存状态、价格、促销信息                 │
│       │    └── 延迟要求: < 50ms                        │
│       │                                                 │
│       ▼                                                 │
│  Context Assembly (Prompt 组装)                         │
│  ├── 系统 Prompt + 用户特征 + 检索结果 + 对话历史      │
│  └── Token 预算管理 (上下文窗口有限)                   │
│       │                                                 │
│       ▼                                                 │
│  LLM Inference (模型推理)                               │
│  ├── 输入: 组装后的 Prompt                              │
│  ├── 输出: 生成结果                                     │
│  └── 延迟要求: < 3s (首 Token < 500ms)                │
│       │                                                 │
│       ▼                                                 │
│  Post-processing (后处理)                               │
│  ├── 安全过滤、格式化、引用标注                         │
│  └── 响应返回                                           │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：Feast Feature Store 部署

```yaml
# Feast 在线服务 (Redis 后端)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: feast-online-server
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: feast
        image: feastdev/feature-server:0.38
        env:
        - name: FEAST_REDIS_HOST
          value: redis-cluster.feature-store.svc
        - name: FEAST_REDIS_PORT
          value: "6379"
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
        ports:
        - containerPort: 6566  # gRPC
        - containerPort: 8080  # REST
---
# 特征定义 (feature_repo/features.py)
# from feast import FeatureView, Field, Entity
# from feast.types import Float64, String, Int64
#
# user = Entity(name="user_id", join_keys=["user_id"])
#
# @feature_view(source=redis_source, entities=[user])
# def user_features():
#     return [
#         Field(name="membership_level", dtype=String),
#         Field(name="avg_order_value", dtype=Float64),
#         Field(name="last_purchase_hours", dtype=Int64),
#         Field(name="preferred_category", dtype=String),
#     ]
```

### 模式三：RAG 检索管线

```
RAG Pipeline:

  1. 离线索引构建:
     文档 → 分块 (Chunking, 512 tokens)
          → Embedding (text-embedding-3-large)
          → 向量写入 (Milvus/Qdrant/Weaviate)
          → 元数据写入 (来源、版本、时间)

  2. 在线检索:
     Query → Query Embedding
           → 向量相似度搜索 (top-K=10)
           → 重排序 (Cross-encoder rerank)
           → 过滤 (相关性阈值 > 0.7)
           → 返回 top-3 文档片段

  3. 质量保障:
     ├── 检索命中率监控 (用户反馈相关性)
     ├── 文档新鲜度 (过期文档降权)
     ├── 多路召回 (向量 + 关键词 + 知识图谱)
     └── A/B 测试 (检索策略对比)

  K8s 部署:
  ├── Embedding 服务 (GPU Pod, 模型推理)
  ├── 向量数据库 (StatefulSet, Milvus/Qdrant)
  ├── Reranker 服务 (GPU Pod, Cross-encoder)
  └── 检索 API (Deployment, 无状态)
```

### 模式四：在线/离线一致性保证

```
Training-Serving Skew 防控:

  Feature Store 层面:
  ├── 统一特征定义 (同一份计算逻辑)
  ├── 离线: Spark/Flink 批量计算 → 物化到离线存储
  ├── 在线: 实时流计算 → 写入 Redis/DynamoDB
  ├── 一致性验证: 定期对比离线/在线特征分布
  └── Point-in-time Join: 训练时按时间戳获取历史特征

  RAG 层面:
  ├── 文档版本化 (每次更新记录版本)
  ├── Embedding 模型版本固定 (不随意升级)
  ├── 索引快照 (训练时使用特定索引版本)
  ├── 检索结果缓存 (相同 query 返回一致结果)
  └── 分布监控: 训练时 vs 推理时的检索结果分布

  监控指标:
  ├── 特征 PSI (Population Stability Index) > 0.2 → 告警
  ├── 检索结果重叠率 (训练/推理) < 80% → 告警
  ├── 特征缺失率 > 5% → 告警
  └── Embedding 分布漂移检测
```

### 模式五：ML 平台集成（Kubeflow）

```
Kubeflow Pipeline 中的 Feature Store + RAG:

  训练 Pipeline:
  1. 数据准备 → 从 Feature Store 离线获取历史特征
  2. 文档准备 → 从向量数据库导出训练时索引快照
  3. 模型训练 → 使用特征 + 文档训练/微调
  4. 模型评估 → 对比有/无 RAG 的效果
  5. 模型注册 → MLflow Model Registry

  推理 Pipeline:
  1. 请求接入 → API Gateway
  2. 特征获取 → Feature Store 在线服务 (< 10ms)
  3. 文档检索 → 向量数据库 (< 100ms)
  4. Prompt 组装 → 特征 + 检索结果 + 对话历史
  5. 模型推理 → LLM 服务 (vLLM/TGI)
  6. 后处理 → 安全过滤 + 格式化
  7. 响应返回

  监控:
  ├── 特征服务延迟 P99 < 10ms
  ├── 检索延迟 P99 < 100ms
  ├── 端到端延迟 P99 < 3s
  ├── 特征 PSI 监控
  └── 检索质量监控 (用户反馈)
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | Feast (开源) | Tecton (商业) | 自建 Feature Service | 无 Feature Store |
|------|-------------|-------------|-------------------|-----------------|
| 部署复杂度 | 中 | 低（SaaS） | 高 | 无 |
| 在线延迟 | <10ms (Redis) | <5ms | 取决于实现 | N/A |
| 离线支持 | Spark/Flink | 内置 | 需自建 | 脚本 |
| 一致性保证 | Point-in-time Join | 内置 | 需自建 | 无 |
| 特征监控 | 基础 | 完整 | 需自建 | 无 |
| 成本 | 开源 + 基础设施 | SaaS 订阅 | 开发成本 | 无 |
| 适用规模 | 中大型 | 大型 | 大型 | 小型 |

### 向量数据库对比

| 维度 | Milvus | Qdrant | Weaviate | Pinecone | pgvector |
|------|--------|--------|----------|----------|----------|
| 部署 | K8s 原生 | K8s/Docker | K8s/Docker | SaaS | PG 扩展 |
| 规模 | 十亿级 | 亿级 | 亿级 | 十亿级 | 百万级 |
| 性能 | 高 | 高 | 中 | 高 | 中 |
| 过滤 | 标量+向量 | 标量+向量 | 标量+向量 | 标量+向量 | SQL |
| 运维 | 中（多组件） | 低 | 低 | 无 | 低 |
| 适用 | 大规模生产 | 中大规模 | 中小规模 | 不想运维 | 已有 PG |

## Anti-patterns & Pitfalls（反模式）

### 反模式一：训练和推理用不同特征计算逻辑

训练时用 Spark SQL 计算特征，推理时用 Python 重新实现。两者逻辑微妙不同（如 NULL 处理、时区、精度），导致 training-serving skew。**正确做法**：Feature Store 统一定义特征计算逻辑，离线和在线共用同一份定义。

### 反模式二：RAG 检索不考虑时效性

知识库文档更新后，向量索引未重建。用户问"最新政策是什么"，检索到的是旧版本文档。**正确做法**：文档更新触发增量索引重建；检索结果附带时间戳，LLM 优先使用最新内容；定期全量重建索引。

### 反模式三：Feature Store 在线服务无降级

Redis 不可用时，在线特征服务完全中断，推理链路阻塞。**正确做法**：特征服务降级策略——Redis 不可用时返回默认值/缓存值；设置超时（< 5ms）；熔断器保护。

### 反模式四：向量检索只靠相似度

纯向量相似度检索返回"语义相似但不相关"的结果。如用户问"退货政策"，检索到"换货流程"（语义相似但答案不同）。**正确做法**：多路召回（向量 + BM25 关键词 + 元数据过滤）；Reranker 二次排序；元数据过滤（类别、时间）。

### 反模式五：忽略 Token 预算管理

检索返回 10 个文档片段（每个 512 tokens），加上用户特征和对话历史，总上下文超过模型窗口限制。截断导致关键信息丢失。**正确做法**：设置 Token 预算（如 4096 tokens 给检索内容）；按相关性排序截断；动态调整检索数量。

### 反模式六：特征/索引更新无版本控制

特征定义或向量索引更新后，无法回滚到之前的版本。模型效果退化时无法确定是特征变了还是索引变了。**正确做法**：特征定义入 Git（版本化）；索引构建记录版本号和参数；支持快速回滚到任意版本。

## Operational Checklist（运维检查清单）

### Feature Store

- [ ] 在线服务高可用（Redis Cluster ≥ 6 节点）
- [ ] 在线延迟监控：P99 < 10ms
- [ ] 特征缺失率监控：< 5%
- [ ] 离线/在线一致性验证（每周）
- [ ] 特征 PSI 监控（分布漂移检测）
- [ ] 特征定义版本化（Git 管理）

### RAG 检索

- [ ] 向量数据库高可用（Milvus ≥ 3 节点）
- [ ] 检索延迟监控：P99 < 100ms
- [ ] 索引新鲜度监控（文档更新 → 索引更新延迟）
- [ ] 检索质量监控（用户反馈、相关性评分）
- [ ] Embedding 模型版本固定（不随意升级）
- [ ] 多路召回 + Reranker 部署

### ML 平台集成

- [ ] 端到端延迟监控：P99 < 3s
- [ ] Token 用量监控（成本控制）
- [ ] 模型效果监控（A/B 测试、用户满意度）
- [ ] 降级策略：Feature Store 不可用 → 默认值
- [ ] 降级策略：向量数据库不可用 → 纯 LLM 回答
- [ ] 全链路追踪（OTel：特征获取 → 检索 → 推理）

### 数据治理

- [ ] 特征文档（定义、计算逻辑、Owner）
- [ ] 知识库文档版本管理
- [ ] 数据质量检查（特征空值率、异常值）
- [ ] 合规：用户特征使用授权（GDPR）
- [ ] 数据保留策略（过期特征/文档清理）

## Related

- [[22-概念/12-研究/k8s-ai-ml-infrastructure.md|K8s AI/ML 基础设施]]
- [[22-概念/06-可观测性/ai-ml-observability.md|AI/ML 可观测性]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads.md|GPU 调度与 AI 工作负载]]
- [[23-实体/11-AI与边缘/kubeflow.md|Kubeflow]]
- [[24-综合/01-AI与机器学习/storage-ai-workload-data-pipeline.md|存储 × AI 工作负载 × 数据管线]]
- [[24-综合/01-AI与机器学习/observability-ai-llm-monitoring.md|可观测性 × AI/LLM 监控]]
- [[24-综合/01-AI与机器学习/ai-workload-cost-optimization-finops.md|AI 工作负载 × 成本优化 × FinOps]]
- [[24-综合/07-平台与数据/kafka-database-cdc-streaming.md|Kafka × 数据库 × CDC × 流处理]]

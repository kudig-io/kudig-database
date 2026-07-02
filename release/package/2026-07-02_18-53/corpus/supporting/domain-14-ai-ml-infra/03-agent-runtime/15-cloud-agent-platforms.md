---
title: 云Agent平台即服务
description: '主流云厂商Agent平台对比：AWS Bedrock Agents、Azure AI Agent Service、Google Vertex AI Agent Builder、阿里云百炼'
summary: '主流云厂商Agent平台对比：AWS Bedrock Agents、Azure AI Agent Service、Google Vertex AI Agent Builder、阿里云百炼'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- cloud
- paas
- bedrock
- vertex
- azure
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
- 云Agent平台即服务 是什么
- 如何选择云Agent平台
- AWS Bedrock Agents 怎么用
- Azure AI Agent Service 架构
- Google Vertex AI Agent Builder 对比
trigger_keywords:
- cloud agent
- bedrock agents
- vertex ai agent
- azure ai agent
- 百炼
- agent paas
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 云Agent平台即服务

## 概述

云Agent平台（Agent PaaS）将Agent构建所需的基础设施——LLM调用、知识库检索、工具编排、会话管理——封装为托管服务。相比自建Agent框架，云平台提供开箱即用的编排能力、与云生态的深度集成、以及按用量计费的弹性模型。

本文覆盖四大主流平台：AWS Bedrock Agents、Azure AI Agent Service、Google Vertex AI Agent Builder、阿里云百炼Agent，并提供对比选型框架。

## 1. AWS Bedrock Agents

### 1.1 核心架构

Bedrock Agents 采用三组件模型：

```
┌─────────────────────────────────────────────────┐
│                  Bedrock Agent                   │
│                                                  │
│  ┌──────────┐  ┌──────────────┐  ┌───────────┐ │
│  │ Foundation│  │  Knowledge   │  │  Action   │ │
│  │  Model    │  │   Bases      │  │  Groups   │ │
│  │ (Claude/  │  │ (OpenSearch/ │  │ (Lambda/  │ │
│  │  Titan/   │  │  S3/RDS)     │  │  Step Fn) │ │
│  │  Llama)   │  │              │  │           │ │
│  └──────────┘  └──────────────┘  └───────────┘ │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │         Agent Alias (版本管理)            │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

**Foundation Model**：Agent绑定的基础模型，支持Claude、Titan、Llama等Bedrock托管模型。模型选择在Agent创建时指定，可通过Alias切换。

**Knowledge Base**：RAG检索层，支持以下数据源：
- Amazon S3（文档/PDF/HTML/Markdown）
- Amazon OpenSearch Serverless（向量检索）
- Aurora PostgreSQL（pgvector）
- Kendra（企业搜索）

Knowledge Base自动处理文档分块、Embedding生成（使用Amazon Titan Embedding或Cohere Embed）、向量索引构建。检索时自动执行Hybrid Search（向量+关键词）。

**Action Group**：Agent的能力边界，定义可调用的工具：
- **Lambda Action**：调用AWS Lambda函数执行操作（数据库写入、API调用、文件操作等）
- **Schema定义**：使用OpenAPI 3.0 Schema描述Action的输入/输出
- **Return Control**：将Action结果返回给Agent进行推理

### 1.2 Agent Alias与版本管理

```
Agent（DEV）
  ├── Alias: LIVE → Version 3
  ├── Alias: STAGING → Version 4 (draft)
  └── Version 1 (archived)
  └── Version 2 (archived)
  └── Version 3 (published)
```

每个Agent可创建多个Alias，每个Alias指向一个具体版本。版本包括：
- 指令（Instructions）
- Action Group定义
- Knowledge Base配置
- 基础模型设置

支持Blue/Green部署：创建新版本后，切换Alias指向即可完成流量切换。

### 1.3 会话管理与Memory

Bedrock Agents维护会话状态（Session State），支持：
- **Session ID**：每个会话唯一标识
- **Session Attributes**：跨轮次传递的键值对（最大3KB）
- **Prompt Session Attributes**：仅在当前推理步骤可见的临时数据
- **Memory**（2025年新增）：跨会话的长期记忆，可配置记忆窗口

### 1.4 企业级特性

```yaml
安全:
  - IAM细粒度权限控制（Agent/Action/KB独立授权）
  - VPC端点支持（私有网络访问）
  - KMS加密（数据静态加密）
  - CloudTrail审计日志

可观测性:
  - CloudWatch Metrics（调用次数/Latency/Token消耗）
  - CloudWatch Logs（推理Trace/Action调用日志）
  - X-Ray分布式追踪

合规:
  - SOC 1/2/3
  - HIPAA
  - FedRAMP
  - ISO 27001
```

### 1.5 定价模型

```
Bedrock Agents 定价（2026 Q2）:
  - Agent编排费: $0.003/次 调用
  - LLM调用: 按模型Token价格（另计）
  - Knowledge Base检索: $0.0005/次
  - Lambda执行: 按Lambda定价（另计）
  - 存储: S3/OpenSearch按各自定价

示例: 10,000次/天 Agent调用
  Agent编排: 10000 × $0.003 = $30/天
  LLM (Claude Sonnet): ~$50/天（假设5K token/次）
  KB检索: 10000 × $0.0005 = $5/天
  总计: ~$85/天 ≈ $2,550/月
```

## 2. Azure AI Agent Service

### 2.1 架构与Azure AI Foundry集成

Azure AI Agent Service深度集成Azure AI Foundry（原Azure AI Studio），提供统一的Agent构建体验：

```
┌─────────────────────────────────────────────────┐
│              Azure AI Foundry                    │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │           AI Agent Service                │   │
│  │  ┌────────┐ ┌────────┐ ┌──────────────┐ │   │
│  │  │ Model  │ │ Tools  │ │  Knowledge   │ │   │
│  │  │(GPT-4/ │ │(Func/  │ │  (AI Search/ │ │   │
│  │  │ Phi-3/ │ │ Code   │ │  Blob/ShareP)│ │   │
│  │  │ Llama) │ │ Interp)│ │              │ │   │
│  │  └────────┘ └────────┘ └──────────────┘ │   │
│  └──────────────────────────────────────────┘   │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │  Connected Agents (Multi-Agent编排)       │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

**核心组件**：
- **Agent**：配置模型、指令、工具的运行实体
- **Thread**：对话线程，维护消息历史和上下文
- **Run**：一次推理执行，包含工具调用循环
- **Run Step**：推理过程中的原子步骤

### 2.2 工具类型

```python
# Azure AI Agent Service 工具类型

# 1. Code Interpreter - 内置代码执行
agent = client.agents.create_agent(
    model="gpt-4o",
    name="data-analyst",
    instructions="你是数据分析助手",
    tools=[{"type": "code_interpreter"}]
)

# 2. File Search - RAG检索
agent = client.agents.create_agent(
    tools=[{"type": "file_search"}],
    tool_resources={
        "file_search": {
            "vector_store_ids": ["vs_abc123"]
        }
    }
)

# 3. Function Calling - 自定义函数
agent = client.agents.create_agent(
    tools=[{
        "type": "function",
        "function": {
            "name": "get_weather",
            "description": "获取天气",
            "parameters": {
                "type": "object",
                "properties": {
                    "location": {"type": "string"}
                }
            }
        }
    }]
)

# 4. Azure AI Search - 企业搜索
# 5. Azure Functions - 无服务器执行
# 6. Bing Search - 联网搜索
```

### 2.3 Connected Agents（多Agent编排）

Azure支持Agent间的层级调用：

```python
# 创建子Agent
billing_agent = client.agents.create_agent(
    model="gpt-4o-mini",
    name="billing-agent",
    instructions="处理账单查询"
)

# 主Agent通过Connected Agent调用子Agent
main_agent = client.agents.create_agent(
    model="gpt-4o",
    name="customer-service",
    instructions="你是客服总代理",
    tools=[{
        "type": "connected_agent",
        "connected_agent": {
            "name": "billing_agent",
            "id": billing_agent.id
        }
    }]
)
```

### 2.4 企业特性

```
与Azure生态集成:
  - Entra ID (AAD) 认证
  - Azure RBAC 角色控制
  - Managed Identity 免密访问Azure资源
  - Private Endpoint 私有部署
  - Azure Monitor 全链路监控
  - Microsoft Purview 数据治理
```

## 3. Google Vertex AI Agent Builder

### 3.1 双模式Agent

Vertex AI Agent Builder提供两种Agent构建模式：

**Conversation Agent（对话Agent）**：
```
┌──────────────────────────────────────┐
│       Conversation Agent             │
│                                      │
│  ┌──────────┐  ┌────────────────┐   │
│  │ Gemini   │  │  Tools         │   │
│  │ Models   │  │  - Extensions  │   │
│  │          │  │  - Functions   │   │
│  │          │  │  - Data Stores │   │
│  └──────────┘  └────────────────┘   │
│                                      │
│  ┌──────────────────────────────┐   │
│  │  Playbooks (行为编排)         │   │
│  └──────────────────────────────┘   │
└──────────────────────────────────────┘
```

**Search Agent（搜索Agent）**：
- 基于Vertex AI Search构建
- 面向企业知识库的检索增强生成
- 支持结构化/非结构化数据源
- 自动抽取、分块、索引

### 3.2 Extensions与Function Calling

```python
# Vertex AI Extension 示例
from vertexai.preview import extensions

# 创建天气查询Extension
extension = extensions.Extension.create(
    display_name="weather-extension",
    description="查询天气信息",
    manifest={
        "name": "weather",
        "description": "Weather API",
        "api_spec": {
            "open_api_gcs_uri": "gs://bucket/weather_api.yaml"
        },
        "operation_config": {
            "allowed_operations": ["GET /weather"]
        }
    }
)

# 部署到Agent
agent = agent_builder.create_agent(
    model="gemini-1.5-pro",
    tools=[
        {"extension": extension.resource_name}
    ]
)
```

### 3.3 Playbooks（行为编排）

Playbook是Vertex AI Agent Builder的高级编排原语，定义Agent在特定场景下的行为：

```yaml
# Playbook 定义示例
playbook:
  name: "customer-escalation"
  trigger:
    condition: "用户表达不满或请求人工客服"
  steps:
    - id: detect_sentiment
      action: sentiment_analysis
    - id: check_urgency
      action: classify_urgency
      condition: "sentiment == negative"
    - id: escalate
      action: route_to_human
      condition: "urgency == high"
    - id: offer_solution
      action: suggest_alternative
      condition: "urgency != high"
```

### 3.4 Grounding（知识接地）

```
Grounding选项:
  1. Data Store Grounding
     - Vertex AI Search数据存储
     - 支持网站、PDF、BigQuery等数据源
     - 自动引用原文出处

  2. Web Grounding
     - 实时互联网搜索
     - 基于Google Search
     - 返回带引用的答案

  3. Custom Grounding
     - 自定义检索接口
     - 对接企业内部系统
```

## 4. 阿里云百炼Agent

### 4.1 平台架构

百炼（Bailian）是阿里云的Agent构建平台，基于通义千问系列模型：

```
┌──────────────────────────────────────────────────┐
│                  阿里云百炼                        │
│                                                    │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────┐ │
│  │  模型服务    │  │  Agent编排   │  │  知识库   │ │
│  │  通义千问    │  │  工作流引擎  │  │  向量检索 │ │
│  │  Qwen-Max   │  │  多Agent     │  │  文档解析 │ │
│  │  Qwen-Plus  │  │  对话管理    │  │  RAG     │ │
│  └─────────────┘  └─────────────┘  └──────────┘ │
│                                                    │
│  ┌──────────────────────────────────────────────┐│
│  │  应用接入: 钉钉/微信/飞书/Web/API             ││
│  └──────────────────────────────────────────────┘│
└──────────────────────────────────────────────────┘
```

### 4.2 核心能力

```yaml
模型层:
  - 通义千问系列（Qwen-Max/Plus/Turbo）
  - 支持第三方模型接入（OpenAI兼容接口）
  - 模型微调平台

Agent编排:
  - 可视化工作流编辑器
  - 条件分支/循环/并行节点
  - 代码节点（Python/JavaScript）
  - 多Agent协作（主Agent+子Agent）

知识库:
  - 文档解析（PDF/Word/网页/图片OCR）
  - 向量检索（基于DashVector）
  - 多路召回（向量+关键词+重排序）
  - 引用溯源

应用发布:
  - API接入
  - 钉钉机器人
  - 微信公众号/小程序
  - 飞书机器人
  - Web Widget
```

### 4.3 企业版特性

```
私有化部署:
  - 支持部署到客户VPC
  - 数据不出域
  - 模型私有化部署（GPU集群）

安全合规:
  - RAM访问控制
  - 数据加密（传输+存储）
  - 操作审计（ActionTrail）
  - 内容安全审核（绿网）
```

## 5. 对比选型

### 5.1 功能对比

| 维度 | AWS Bedrock Agents | Azure AI Agent Service | Vertex AI Agent Builder | 阿里云百炼 |
|------|-------------------|----------------------|------------------------|-----------|
| **模型选择** | Claude/Titan/Llama/Mistral | GPT-4o/Phi-3/Llama | Gemini/Anthropic/OpenAI | 通义千问系列+第三方 |
| **RAG方案** | Knowledge Base(OpenSearch/S3) | File Search + AI Search | Data Store + Web Grounding | DashVector知识库 |
| **工具调用** | Lambda + OpenAPI Schema | Function + Code Interpreter | Extension + Function | 工作流节点 + API |
| **多Agent** | 基础（需自建编排） | Connected Agents | Agent-to-Agent | 子Agent模式 |
| **会话管理** | Session State + Memory | Thread + Run | Session + Context | 对话变量 |
| **可视化编辑** | 控制台基础配置 | AI Foundry Studio | Agent Builder Console | 可视化工作流 |
| **中文支持** | 良好（需选中文模型） | 良好 | 一般 | 原生优化 |
| **私有化部署** | 不支持 | 不支持（仅Private Link） | 不支持 | 支持 |

### 5.2 定价对比

```
# 🟢 低风险：只读/信息收集，通常无副作用
价格区间（基于2026 Q2公开信息）:

AWS Bedrock Agents:
  编排费: $0.003/次
  LLM: Claude Sonnet $3/$15 per 1M token (input/output)
  隐性成本: Lambda/S3/OpenSearch另计

Azure AI Agent Service:
  编排费: 包含在API调用中
  LLM: GPT-4o $2.50/$10 per 1M token
  隐性成本: AI Search/Azure Functions另计

Vertex AI Agent Builder:
  编排费: $0.002/次 (Conversation Agent)
  LLM: Gemini 1.5 Pro $1.25/$5 per 1M token
  隐性成本: AI Search/Cloud Functions另计

阿里云百炼:
  编排费: 0.003元/次
  LLM: Qwen-Max 0.12元/千token
  隐性成本: DashVector/OSS另计

小规模 (10K次/天): 各平台月费 $500-$3,000
中规模 (100K次/天): 各平台月费 $5,000-$30,000
大规模 (1M次/天): 需协商企业折扣
```
### 5.3 厂商锁定风险评估

```
# 🟢 低风险：只读/信息收集，通常无副作用
锁定维度分析:

┌─────────────┬──────────┬──────────┬──────────┬──────────┐
│ 维度         │ AWS      │ Azure    │ GCP      │ 阿里云   │
├─────────────┼──────────┼──────────┼──────────┼──────────┤
│ 模型迁移     │ 低       │ 中       │ 低       │ 高       │
│ API兼容性    │ 自有API  │ OpenAI   │ 自有API  │ 自有API  │
│ 知识库迁移   │ 中       │ 中       │ 中       │ 高       │
│ 工具迁移     │ 高(Lambda)│ 中      │ 中       │ 高       │
│ 会话格式     │ 自有     │ 自有     │ 自有     │ 自有     │
│ 综合锁定风险 │ 高       │ 中       │ 中       │ 高       │
└─────────────┴──────────┴──────────┴──────────┴──────────┘

降低锁定策略:
  1. 使用OpenAI兼容接口层（LiteLLM/OneAPI）
  2. 工具定义标准化（OpenAPI 3.0）
  3. 知识库数据独立存储
  4. Agent逻辑与平台API解耦
```
### 5.4 选型决策框架

```
选型决策树:

Q1: 已有哪个云平台的深度投入？
    → AWS深度用户 → 优先Bedrock Agents
    → Azure深度用户 → 优先Azure AI Agent Service
    → GCP深度用户 → 优先Vertex AI Agent Builder
    → 阿里云深度用户 → 优先百炼

Q2: 是否需要私有化部署？
    → 是 → 百炼企业版（唯一支持）
    → 否 → 继续评估

Q3: 中文场景占比？
    > 80% → 百炼 > Bedrock > Azure > Vertex
    < 20% → Azure ≈ Bedrock > Vertex > 百炼

Q4: 多Agent编排复杂度？
    高 → Azure（Connected Agents最成熟）
    中 → 百炼（可视化工作流）
    低 → 各平台均可

Q5: 预算敏感度？
    高 → Vertex（性价比最佳）/ 百炼（国内价格优势）
    中 → Azure（OpenAI模型质量）
    低 → Bedrock（企业级稳定性）
```

## 6. 混合架构实践

生产环境中，单一云平台往往无法满足所有需求。混合架构模式：

```yaml
混合模式一: 多云Agent网关
  描述: 统一Agent网关，后端路由到不同云平台
  适用: 需要利用各平台优势模型
  架构:
    API Gateway → Agent Router
      → Bedrock (Claude for reasoning)
      → Vertex (Gemini for multimodal)
      → 百炼 (Qwen for Chinese)

混合模式二: 云平台+自建框架
  描述: 云平台处理基础设施，自建框架管理编排
  适用: 需要复杂自定义编排逻辑
  架构:
    自建Agent Framework (LangGraph/CrewAI)
      → Bedrock API (模型调用)
      → OpenSearch (知识库)
      → Lambda (工具执行)

混合模式三: 渐进式迁移
  描述: 从一个平台迁移到另一个平台
  阶段:
    1. 双写: 新旧平台并行运行
    2. 影子流量: 新平台接收副本流量
    3. 金丝雀: 逐步切换流量
    4. 完全切换: 旧平台下线
```

## 7. 与K8s集成

在K8s环境中使用云Agent平台的典型模式：

```yaml
# K8s中运行Agent应用，调用云平台API
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agent-app
spec:
  template:
    spec:
      containers:
      - name: agent
        image: agent-app:latest
        env:
        - name: CLOUD_PROVIDER
          value: "bedrock"  # 或 azure/vertex/bailian
        - name: AGENT_ID
          valueFrom:
            secretKeyRef:
              name: agent-config
              key: agent-id
        - name: API_KEY
          valueFrom:
            secretKeyRef:
              name: agent-config
              key: api-key
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "2"
            memory: "2Gi"
---
# 多平台路由配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: agent-router-config
data:
  routing.yaml: |
    routes:
      - match:
          intent: "reasoning"
        target: bedrock
        model: claude-sonnet
      - match:
          intent: "multimodal"
        target: vertex
        model: gemini-1.5-pro
      - match:
          intent: "chinese"
        target: bailian
        model: qwen-max
```

## 相关主题

- [[domain-14-ai-ml-infra/03-agent-runtime/16-coze-agent-platform|Coze Agent平台]]
- [[domain-14-ai-ml-infra/03-agent-runtime/17-agent-rate-limiting-cost-control|Agent限流与成本控制]]
- [[domain-14-ai-ml-infra/03-agent-runtime/21-agent-runtime-architecture-overview|Agent Runtime架构总览]]

## 参考资料

- AWS Bedrock Agents Documentation
- Azure AI Agent Service Documentation
- Vertex AI Agent Builder Documentation
- 阿里云百炼产品文档


<!-- risk-assessed -->

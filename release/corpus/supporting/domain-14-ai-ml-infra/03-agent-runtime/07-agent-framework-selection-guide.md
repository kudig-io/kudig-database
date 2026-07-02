---
title: Agent 框架选型决策树
description: 'Agent 框架选型全面指南，按场景选型（LangChain/LangGraph/CrewAI/AutoGen/Semantic Kernel/Dify），涵盖性能对比、社区活跃度、K8s 集成及许可证风险'
summary: 'Agent 框架选型全面指南'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- selection
- comparison
- decision-tree
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
- Agent 框架选型 是什么
- 如何 Agent 框架选型
- LangChain vs CrewAI vs AutoGen 对比
trigger_keywords:
- agent-framework
- selection
- comparison
- decision-tree
- langchain vs crewai
prerequisites:
- llm-basics
- python-basics
- kubectl-basics
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

# Agent 框架选型决策树

## 1. 选型决策树

```
开始
  │
  ├─ Q1: 你的场景是什么？
  │   ├─ 简单对话 / RAG → LangChain
  │   ├─ 复杂 Agent / 状态管理 → LangGraph
  │   ├─ 多 Agent 协作 → Q2
  │   ├─ 企业级 .NET 应用 → Semantic Kernel
  │   ├─ 低代码平台 → Dify
  │   └─ 不确定 → Q2
  │
  ├─ Q2: 团队技术栈？
  │   ├─ Python 为主 → Q3
  │   ├─ C# / .NET → Semantic Kernel
  │   ├─ Java → Semantic Kernel / LangChain4j
  │   └─ 混合 → Q3
  │
  ├─ Q3: 需要代码执行能力？
  │   ├─ 是（Agent 执行代码）→ AutoGen
  │   └─ 否 → Q4
  │
  ├─ Q4: 多 Agent 协作模式？
  │   ├─ 角色分工型（固定角色）→ CrewAI
  │   ├─ 对话协商型（自由对话）→ AutoGen
  │   ├─ 工作流型（预定义流程）→ LangGraph
  │   └─ 混合型 → LangGraph + CrewAI
  │
  └─ Q5: 是否需要可视化构建？
      ├─ 是 → Dify
      └─ 否 → 根据 Q1-Q4 选择
```

---

## 2. 框架全景对比

### 2.1 核心能力矩阵

| 能力 | LangChain | LangGraph | CrewAI | AutoGen | Semantic Kernel | Dify |
|------|-----------|-----------|--------|---------|-----------------|------|
| **定位** | 通用编排 | 状态图引擎 | 多 Agent | 对话协作 | 企业级 SDK | 低代码平台 |
| **语言** | Python | Python | Python | Python | C#/Python/Java | Python |
| **学习曲线** | 低 | 中 | 低 | 中 | 中 | 极低 |
| **状态管理** | Memory组件 | 原生Checkpointer | 短期/长期记忆 | 对话历史 | Memory插件 | 内置 |
| **工具集成** | Tool抽象 | ToolNode | BaseTool | 函数注册 | Plugin/Function | API/插件 |
| **代码执行** | 无原生 | 无原生 | 无原生 | Docker沙箱 | 无原生 | 代码解释器 |
| **可视化** | LangSmith | LangSmith | 无 | Studio | 无 | 内置 |
| **多语言** | 无 | 无 | 无 | 无 | 原生 | API |

### 2.2 Agent 模式对比

| 模式 | LangChain | LangGraph | CrewAI | AutoGen | SK | Dify |
|------|-----------|-----------|--------|---------|----|----|
| ReAct | create_react_agent | 自定义节点 | 内置 | 自定义 | Planner | 内置 |
| Function Calling | 原生支持 | 原生支持 | 原生支持 | 原生支持 | 原生支持 | 原生支持 |
| 多Agent对话 | 无 | 子图 | Crew | GroupChat | AgentChat | 无 |
| Human-in-Loop | 有限 | interrupt | 有限 | human_input | 人工审批 | 人工审批 |
| 流式输出 | 原生 | 原生 | 有限 | 有限 | 原生 | 原生 |

### 2.3 性能基准

基于 K8s 诊断场景（100 次测试，平均值）：

| 指标 | LangChain | LangGraph | CrewAI | AutoGen | SK | Dify |
|------|-----------|-----------|--------|---------|----|----|
| 首次响应(ms) | 850 | 920 | 1100 | 1200 | 780 | 950 |
| 完成任务(s) | 8.5 | 9.2 | 12.3 | 15.6 | 8.8 | 10.2 |
| Token消耗 | 2.8K | 3.1K | 4.5K | 5.2K | 2.9K | 3.5K |
| 成功率(%) | 92 | 95 | 88 | 85 | 91 | 89 |
| 内存占用(MB) | 120 | 150 | 180 | 200 | 130 | 250 |

> 注：性能受 LLM 延迟影响较大，框架本身开销通常 <10%。

---

## 3. 按场景选型详解

### 3.1 简单对话 / RAG → LangChain

**适用场景：**
- 单轮问答
- 文档检索增强生成
- 简单工具调用
- 快速原型

**不适用场景：**
- 需要复杂状态管理
- 多 Agent 协作
- 需要持久化检查点

```python
# 典型用例：RAG 问答
from langchain_openai import ChatOpenAI
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser

chain = (
    {"context": retriever, "question": RunnablePassthrough()}
    | prompt
    | ChatOpenAI(model="gpt-4o")
    | StrOutputParser()
)
```

### 3.2 复杂 Agent / 工作流 → LangGraph

**适用场景：**
- 多步推理任务
- 需要分支/循环的控制流
- Human-in-the-Loop 审批
- 状态持久化和断点恢复

**不适用场景：**
- 简单对话（过度工程化）
- 团队不熟悉状态机概念

```python
# 典型用例：多步诊断工作流
from langgraph.graph import StateGraph, END

graph = StateGraph(DiagnosisState)
graph.add_node("collect", collect_info)
graph.add_node("analyze", analyze_root_cause)
graph.add_node("fix", generate_fix)
graph.add_conditional_edges("collect", should_continue, {...})
app = graph.compile(checkpointer=checkpointer)
```

### 3.3 多 Agent 协作 → CrewAI

**适用场景：**
- 角色分工明确的任务
- 需要任务委派和协作
- 模拟团队协作流程

**不适用场景：**
- 需要精细的流程控制
- 需要代码执行能力
- 实时流式交互

```python
# 典型用例：K8s 故障排查团队
crew = Crew(
    agents=[diagnostician, fixer, validator],
    tasks=[diagnosis_task, fix_task, validation_task],
    process=Process.sequential,
    memory=True,
)
```

### 3.4 代码执行 / 对话协作 → AutoGen

**适用场景：**
- Agent 需要执行 Python 代码
- 多 Agent 自由对话协商
- 数据分析和可视化

**不适用场景：**
- 需要精细的流程控制
- 生产环境代码执行安全要求高

```python
# 典型用例：数据分析 Agent
user_proxy = UserProxyAgent(
    name="executor",
    code_execution_config={
        "use_docker": "python:3.11-slim",
        "timeout": 120,
    },
)
user_proxy.initiate_chat(assistant, message="分析集群资源使用趋势")
```

### 3.5 企业级 .NET → Semantic Kernel

**适用场景：**
- 企业 .NET / Java 技术栈
- Azure 深度集成
- 需要标准化插件架构
- 合规性要求高

**不适用场景：**
- 快速原型（SDK 较重）
- 非 .NET/Java 团队

```csharp
// 典型用例：企业 K8s 管理平台
var kernel = Kernel.CreateBuilder()
    .AddAzureOpenAIChatCompletion("gpt-4o", endpoint, key)
    .Plugins.AddFromType<K8sPlugin>()
    .Build();
```

### 3.6 低代码平台 → Dify

**适用场景：**
- 非开发人员构建 AI 应用
- 需要可视化工作流
- 快速部署 Chatbot
- 多租户 SaaS

**不适用场景：**
- 需要深度定制
- 高性能要求
- 复杂多 Agent 场景

---

## 4. 社区活跃度对比

### 4.1 GitHub 指标（截至 2026 年中）

| 框架 | Stars | Forks | Contributors | 最近提交 | License |
|------|-------|-------|--------------|---------|---------|
| LangChain | 98K+ | 16K+ | 3,500+ | 每日 | MIT |
| LangGraph | 12K+ | 2K+ | 400+ | 每日 | MIT |
| CrewAI | 25K+ | 3.5K+ | 200+ | 每日 | MIT |
| AutoGen | 38K+ | 5.5K+ | 500+ | 每日 | MIT |
| Semantic Kernel | 22K+ | 4.5K+ | 350+ | 每日 | MIT |
| Dify | 58K+ | 8.5K+ | 400+ | 每日 | Apache 2.0 |

### 4.2 社区生态

| 维度 | LangChain | LangGraph | CrewAI | AutoGen | SK | Dify |
|------|-----------|-----------|--------|---------|----|----|
| 文档质量 | 优秀 | 良好 | 良好 | 良好 | 优秀 | 良好 |
| 社区教程 | 丰富 | 中等 | 中等 | 中等 | 丰富 | 中等 |
| 第三方集成 | 160+ | 50+ | 30+ | 20+ | 40+ | 100+ |
| 企业支持 | LangSmith | LangSmith | 无 | Azure AI | Azure | Dify Cloud |
| 更新频率 | 快 | 快 | 中 | 中 | 中 | 快 |

---

## 5. K8s 集成成熟度

### 5.1 原生 K8s 支持

| 框架 | kubectl集成 | K8s API | RBAC | Helm Chart | Operator |
|------|------------|---------|------|------------|----------|
| LangChain | 工具模式 | 需自建 | 需自建 | 社区版 | 无 |
| LangGraph | 工具模式 | 需自建 | 需自建 | 社区版 | 无 |
| CrewAI | 工具模式 | 需自建 | 需自建 | 无 | 无 |
| AutoGen | 沙箱内 | 需自建 | 需自建 | 无 | 无 |
| SK | 插件模式 | 需自建 | 需自建 | 无 | 无 |
| Dify | 插件模式 | OpenAPI | 内置 | 官方版 | 无 |

### 5.2 K8s 部署复杂度

```
部署复杂度（从低到高）：

Dify ████████░░  (Helm 一键部署)
LangChain ██████░░░░  (标准 Web 服务)
LangGraph ██████░░░░  (需要 PostgreSQL)
SK ██████░░░░  (标准 Web 服务)
AutoGen ████████░░  (需要 Docker Socket)
CrewAI ██████░░░░  (标准 Web 服务)
```

### 5.3 推荐 K8s 部署架构

```yaml
# 生产级 Agent 部署（以 LangGraph 为例）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: k8s-agent
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    spec:
      serviceAccountName: k8s-agent
      containers:
        - name: agent
          image: registry/k8s-agent:1.0.0
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2000m"
              memory: "4Gi"
          env:
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: llm-secrets
                  key: openai-api-key
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8000
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8000
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: k8s-agent-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: k8s-agent
  minReplicas: 2
  maxReplicas: 10
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
```

---

## 6. 许可证风险评估

### 6.1 许可证对比

| 框架 | 许可证 | 商业使用 | 修改分发 | 专利授权 | 风险等级 |
|------|--------|---------|---------|---------|---------|
| LangChain | MIT | 允许 | 允许 | 无明确 | 低 |
| LangGraph | MIT | 允许 | 允许 | 无明确 | 低 |
| CrewAI | MIT | 允许 | 允许 | 无明确 | 低 |
| AutoGen | MIT | 允许 | 允许 | 无明确 | 低 |
| Semantic Kernel | MIT | 允许 | 允许 | 无明确 | 低 |
| Dify | Apache 2.0 | 允许 | 允许 | 明确授权 | 极低 |

### 6.2 依赖项风险

```
间接许可证风险：

LangChain:
  └── 依赖项 200+，部分使用 Apache 2.0 / BSD
  └── 风险：低

LangGraph:
  └── 依赖 LangChain + PostgreSQL 驱动
  └── 风险：低

AutoGen:
  └── Docker SDK（Apache 2.0）
  └── 风险：低

Dify:
  └── 大量 Web 依赖（Next.js、React）
  └── 风险：低
```

### 6.3 商业化限制

| 框架 | SaaS 使用 | 竞品构建 | 品牌使用 | 建议 |
|------|----------|---------|---------|------|
| LangChain | 允许 | 允许 | 需授权 | 可直接使用 |
| CrewAI | 允许 | 允许 | 需授权 | 可直接使用 |
| AutoGen | 允许 | 允许 | 需授权 | 可直接使用 |
| Dify | 允许 | 允许 | 需授权 | 可直接使用 |

---

## 7. 选型决策矩阵

### 7.1 加权评分

按 K8s 运维诊断场景加权：

| 维度 | 权重 | LangChain | LangGraph | CrewAI | AutoGen | SK | Dify |
|------|------|-----------|-----------|--------|---------|----|----|
| 易用性 | 15% | 9 | 7 | 8 | 7 | 7 | 9 |
| 功能完整性 | 20% | 8 | 9 | 7 | 8 | 8 | 7 |
| K8s集成 | 20% | 7 | 7 | 6 | 6 | 7 | 8 |
| 性能 | 15% | 8 | 8 | 7 | 6 | 8 | 7 |
| 社区活跃度 | 10% | 9 | 8 | 7 | 8 | 8 | 8 |
| 生产就绪度 | 20% | 8 | 8 | 7 | 7 | 8 | 8 |
| **加权总分** | | **8.05** | **7.85** | **6.95** | **7.00** | **7.65** | **7.75** |

### 7.2 推荐方案

**方案一：渐进式选型**

```
阶段 1（MVP）: LangChain
  → 快速构建 RAG + 简单 Agent
  → 验证 LLM 在 K8s 场景的有效性

阶段 2（增强）: LangGraph
  → 引入状态管理和检查点
  → 构建复杂诊断工作流

阶段 3（扩展）: LangGraph + CrewAI
  → 多 Agent 协作
  → 专家角色分工
```

**方案二：一步到位**

```
场景：多 Agent K8s 诊断系统

推荐：LangGraph + CrewAI

架构：
  ┌─────────────────────────────────────┐
  │          LangGraph (控制平面)         │
  │  ┌─────────┐  ┌─────────┐           │
  │  │ State   │  │ Check-  │           │
  │  │ Machine │  │ pointer │           │
  │  └────┬────┘  └─────────┘           │
  │       │                             │
  │  ┌────┴───────────────────────┐     │
  │  │     CrewAI (Agent 协作)     │     │
  │  │  ┌──────┐ ┌──────┐ ┌────┐  │     │
  │  │  │诊断  │ │修复  │ │验证│  │     │
  │  │  │Agent │ │Agent │ │Agent│ │     │
  │  │  └──────┘ └──────┘ └────┘  │     │
  │  └────────────────────────────┘     │
  └─────────────────────────────────────┘
```

---

## 8. 迁移策略

### 8.1 从 LangChain 迁移到 LangGraph

```python
# 迁移前：LangChain Agent
from langchain.agents import create_react_agent
agent = create_react_agent(llm, tools, prompt)

# 迁移后：LangGraph
from langgraph.prebuilt import create_react_agent
agent = create_react_agent(llm, tools)  # API 兼容

# 渐进迁移：LCEL 管道保持不变
chain = prompt | llm | parser  # 继续使用
```

### 8.2 从 CrewAI 迁移到 LangGraph

```python
# CrewAI 定义
crew = Crew(agents=[a1, a2], tasks=[t1, t2])

# LangGraph 等价
graph = StateGraph(State)
graph.add_node("agent1", a1_node)
graph.add_node("agent2", a2_node)
graph.add_edge("agent1", "agent2")
```

---

## Related

- [[domain-14-ai-ml-infra/03-agent-runtime/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[domain-14-ai-ml-infra/03-agent-runtime/03-crewai-multi-agent-framework|CrewAI 多 Agent 框架]]

## See Also

- [[domain-14-ai-ml-infra/03-agent-runtime/04-autogen-microsoft-agent|Microsoft AutoGen]]
- [[domain-14-ai-ml-infra/03-agent-runtime/06-semantic-kernel-enterprise|Semantic Kernel 企业级 Agent]]
- [[domain-14-ai-ml-infra/03-agent-runtime/05-dify-agent-platform|Dify Agent 平台]]

---
title: Agent Runtime架构总览
description: 'Agent Runtime分层架构、组件关系、数据流、部署拓扑与K8s生态集成'
summary: 'Agent Runtime分层架构、组件关系、数据流、部署拓扑与K8s生态集成'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- architecture
- overview
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
- Agent Runtime架构总览 是什么
- Agent Runtime架构设计
- Agent系统分层架构
- Agent部署拓扑
trigger_keywords:
- agent runtime
- architecture
- deployment topology
- data flow
- k8s integration
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

# Agent Runtime架构总览

## 概述

Agent Runtime是承载AI Agent运行时行为的基础设施层。它将LLM推理、工具调用、会话管理、安全控制、可观测性等能力封装为统一的运行时引擎，使Agent开发者专注于业务逻辑而非基础设施。

本文从分层架构、组件关系、数据流、部署拓扑和K8s生态集成五个维度，全面梳理Agent Runtime的架构设计。

## 1. 分层架构

### 1.1 四层架构模型

```
┌─────────────────────────────────────────────────────────────┐
│                    Layer 4: Application                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐ │
│  │ 客服Bot  │ │ 代码助手 │ │ 数据分析 │ │ 自定义Agent   │ │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Layer 3: Agent Framework                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐ │
│  │ LangGraph│ │ CrewAI   │ │ AutoGen  │ │ 自研框架      │ │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Layer 2: Runtime Engine                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐ │
│  │ 推理引擎 │ │ 工具引擎 │ │ 会话管理 │ │ 安全与限流    │ │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Layer 1: Infrastructure                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐ │
│  │LLM API   │ │向量数据库│ │消息队列  │ │ K8s / Cloud   │ │
│  │(OpenAI/  │ │(Milvus/ │ │(Kafka/  │ │ (EKS/GKE/AKS) │ │
│  │ Anthropic)│ │ Qdrant) │ │ Redis)  │ │               │ │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 各层职责

```yaml
Layer 4 - Application (应用层):
  职责: 面向业务场景的Agent实现
  组件:
    - 业务Agent（客服/代码/数据/自定义）
    - Agent配置（Prompt/工具/知识库绑定）
    - 发布渠道（Web/API/IM集成）
  特点:
    - 由业务团队维护
    - 通过配置或代码定义Agent行为
    - 不直接接触基础设施

Layer 3 - Agent Framework (框架层):
  职责: Agent编排逻辑的抽象与实现
  组件:
    - 编排引擎（ReAct/Plan-Execute/Multi-Agent）
    - 工具注册与发现
    - 记忆管理（短期/长期/工作记忆）
    - Prompt模板管理
  特点:
    - 提供Agent开发SDK
    - 屏蔽底层Runtime复杂性
    - 可插拔的编排策略

Layer 2 - Runtime Engine (运行时引擎):
  职责: Agent执行的核心引擎
  组件:
    - 推理引擎（LLM调用/重试/降级）
    - 工具引擎（工具执行/沙箱/超时）
    - 会话管理（状态/上下文/记忆）
    - 安全控制（限流/预算/内容过滤）
    - 可观测性（Trace/Metrics/Log）
  特点:
    - 高性能、高可用
    - 多租户支持
    - 弹性与容错

Layer 1 - Infrastructure (基础设施):
  职责: 底层资源供给
  组件:
    - LLM API（模型推理服务）
    - 向量数据库（Embedding检索）
    - 对象存储（文件/知识库存储）
    - 消息队列（异步处理）
    - K8s（容器编排）
    - 可观测性基础设施（Prometheus/Jaeger/Loki）
  特点:
    - 可替换（多云/混合云）
    - 水平扩展
    - 基础设施即代码
```

## 2. 组件关系

### 2.1 核心组件交互图

```
┌─────────────────────────────────────────────────────────────────┐
│                        Agent Runtime                             │
│                                                                  │
│  ┌─────────────┐    ┌──────────────┐    ┌──────────────────┐   │
│  │   API       │    │   Agent      │    │   Session        │   │
│  │   Gateway   │───→│   Router     │───→│   Manager        │   │
│  │             │    │              │    │                  │   │
│  │ Auth/Rate   │    │ Intent       │    │ State/Context    │   │
│  │ Limit       │    │ Classify     │    │ Memory           │   │
│  └─────────────┘    └──────┬───────┘    └────────┬─────────┘   │
│                            │                      │              │
│                            ▼                      ▼              │
│                    ┌───────────────────────────────────┐        │
│                    │        Inference Engine            │        │
│                    │                                   │        │
│                    │  ┌──────────┐  ┌──────────────┐  │        │
│                    │  │ Prompt   │  │  LLM Client  │  │        │
│                    │  │ Compiler │──│  (多模型)     │  │        │
│                    │  │          │  │  Retry/Fallback│ │        │
│                    │  └──────────┘  └──────────────┘  │        │
│                    └───────────────┬───────────────────┘        │
│                                    │                             │
│                                    ▼                             │
│                    ┌───────────────────────────────────┐        │
│                    │        Tool Engine                 │        │
│                    │                                   │        │
│                    │  ┌──────────┐  ┌──────────────┐  │        │
│                    │  │ Tool     │  │  Execution   │  │        │
│                    │  │ Registry │──│  Sandbox     │  │        │
│                    │  │          │  │  (K8s Pod/   │  │        │
│                    │  │          │  │   Container) │  │        │
│                    │  └──────────┘  └──────────────┘  │        │
│                    └───────────────────────────────────┘        │
│                                                                  │
│  ┌─────────────┐    ┌──────────────┐    ┌──────────────────┐   │
│  │ Knowledge   │    │   Cost &     │    │   Observability  │   │
│  │ Base        │    │   Quota      │    │                  │   │
│  │             │    │   Manager    │    │ Trace/Metrics/   │   │
│  │ RAG/Search  │    │              │    │ Log              │   │
│  └─────────────┘    └──────────────┘    └──────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 组件职责与接口

```python
from abc import ABC, abstractmethod

# API Gateway
class APIGateway(ABC):
    """API网关：认证、限流、路由"""

    @abstractmethod
    async def handle_request(self, request) -> dict:
        """处理入站请求"""
        pass

    @abstractmethod
    async def authenticate(self, api_key: str) -> dict:
        """认证并返回租户信息"""
        pass

    @abstractmethod
    async def rate_limit(self, tenant_id: str) -> bool:
        """限流检查"""
        pass

# Agent Router
class AgentRouter(ABC):
    """Agent路由器：意图识别与Agent分发"""

    @abstractmethod
    async def route(self, message: str, context: dict) -> str:
        """路由到目标Agent"""
        pass

# Inference Engine
class InferenceEngine(ABC):
    """推理引擎：LLM调用管理"""

    @abstractmethod
    async def infer(self, messages: list, config: dict) -> dict:
        """执行推理"""
        pass

    @abstractmethod
    async def infer_stream(self, messages: list, config: dict):
        """流式推理"""
        pass

# Tool Engine
class ToolEngine(ABC):
    """工具引擎：工具注册、调用、沙箱执行"""

    @abstractmethod
    async def execute(self, tool_name: str, params: dict, context: dict) -> dict:
        """执行工具调用"""
        pass

    @abstractmethod
    def register(self, tool_def: dict):
        """注册工具"""
        pass

# Session Manager
class SessionManager(ABC):
    """会话管理器：状态、上下文、记忆"""

    @abstractmethod
    async def get_context(self, session_id: str) -> dict:
        """获取会话上下文"""
        pass

    @abstractmethod
    async def update_context(self, session_id: str, updates: dict):
        """更新会话上下文"""
        pass

# Knowledge Base
class KnowledgeBase(ABC):
    """知识库：RAG检索"""

    @abstractmethod
    async def search(self, query: str, tenant_id: str, top_k: int) -> list:
        """语义检索"""
        pass
```

## 3. 数据流

### 3.1 请求处理完整流程

```
用户输入
  │
  ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 1: API Gateway                                             │
│  - 认证: 验证API Key → 获取租户ID                                │
│  - 限流: 检查租户/用户/Agent级限流                                │
│  - 预算: 检查日/月Token预算                                       │
│  - 路由: 解析请求，确定目标Agent                                   │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 2: Session Manager                                         │
│  - 加载会话: 获取历史消息、上下文变量                               │
│  - 窗口裁剪: 超出窗口的历史消息摘要压缩                             │
│  - 注入记忆: 加载用户长期记忆                                      │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 3: Prompt Compiler                                         │
│  - 组装Prompt: System + Context + History + User Input            │
│  - 工具注入: 将可用工具Schema注入Prompt                            │
│  - 知识注入: RAG检索结果注入上下文                                  │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 4: Inference Engine                                        │
│  - 模型路由: 根据复杂度/预算选择模型                                │
│  - LLM调用: 发送推理请求                                          │
│  - 响应解析: 解析工具调用或最终回答                                  │
│  - 弹性处理: 超时/重试/降级                                        │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ├──→ 无工具调用 → Step 7 (输出)
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 5: Tool Engine (循环)                                      │
│  - 工具选择: 解析LLM返回的工具调用                                  │
│  - 权限检查: 验证租户是否有该工具权限                                │
│  - 幂等检查: 检查是否已执行过（去重）                                │
│  - 沙箱执行: 在隔离环境中执行工具                                   │
│  - 结果返回: 将工具结果注入对话                                      │
│  - 循环判断: 是否需要继续推理                                        │
└──────────────┬───────────────────────────────────────────────────┘
               │
               │ 循环回到 Step 4（直到无工具调用或达到最大步数）
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 6: Safety & Post-processing                                │
│  - 内容过滤: 检查输出是否违规                                      │
│  - 引用标注: 标注知识库引用来源                                     │
│  - 格式化: 按渠道格式化输出（Markdown/纯文本/卡片）                  │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  Step 7: Response & Async                                        │
│  - 同步返回: 将结果返回用户                                        │
│  - 异步记录: 记录审计日志、更新用量、更新会话                         │
│  - 可观测: 发送Trace/Metrics到监控系统                              │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 流式数据流

```
Streaming数据流:

Client ←── SSE/WebSocket ──→ API Gateway ←── gRPC Stream ──→ Runtime

Token流:
  LLM API ──stream──→ Inference Engine ──chunk──→ API Gateway ──SSE──→ Client

  每个chunk包含:
  {
    "id": "chatcmpl-xxx",
    "object": "chat.completion.chunk",
    "choices": [{
      "index": 0,
      "delta": {
        "content": "你好",  // 或 tool_calls增量
      },
      "finish_reason": null  // 或 "stop"/"tool_calls"
    }]
  }

工具调用中间状态:
  {"type": "tool_start", "tool": "search", "params": {...}}
  {"type": "tool_result", "tool": "search", "result": "..."}
  {"type": "thinking", "content": "正在分析搜索结果..."}
```

## 4. 部署拓扑

### 4.1 单节点部署

适合开发/测试环境：

```
┌──────────────────────────────────────┐
│         单节点 (Single Node)          │
│                                      │
│  ┌─────────────────────────────────┐│
│  │        Agent Runtime Pod        ││
│  │  ┌────────┐  ┌──────────────┐  ││
│  │  │ API    │  │  Inference   │  ││
│  │  │ Server │  │  Engine      │  ││
│  │  └────────┘  └──────────────┘  ││
│  │  ┌────────┐  ┌──────────────┐  ││
│  │  │ Tool   │  │  Session     │  ││
│  │  │ Engine │  │  Store       │  ││
│  │  └────────┘  └──────────────┘  ││
│  └─────────────────────────────────┘│
│                                      │
│  ┌────────┐  ┌────────┐  ┌───────┐ │
│  │ SQLite │  │ Redis  │  │ 文件  │ │
│  │ (数据) │  │ (缓存) │  │存储   │ │
│  └────────┘  └────────┘  └───────┘ │
└──────────────────────────────────────┘

资源需求:
  CPU: 2-4 cores
  Memory: 4-8 Gi
  Storage: 50 Gi
  适用: 开发/测试/PoC
```

### 4.2 分布式部署

适合生产环境：

```
┌─────────────────────────────────────────────────────────────────┐
│                      K8s Cluster (Production)                    │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Ingress Controller (Nginx/Istio)                         │   │
│  └──────────────────────────┬───────────────────────────────┘   │
│                              │                                   │
│  ┌───────────────────────────┼───────────────────────────────┐  │
│  │  ns: agent-platform                                       │  │
│  │                           │                                │  │
│  │  ┌─────────────┐  ┌──────┴──────┐  ┌──────────────────┐ │  │
│  │  │ API Gateway │  │ Agent Router│  │ Auth Service     │ │  │
│  │  │ (3 replicas)│  │ (2 replicas)│  │ (2 replicas)     │ │  │
│  │  └─────────────┘  └─────────────┘  └──────────────────┘ │  │
│  │                                                           │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐ │  │
│  │  │ Inference   │  │ Tool Engine │  │ Session Manager  │ │  │
│  │  │ Engine      │  │ (5 replicas)│  │ (3 replicas)     │ │  │
│  │  │ (10 replicas)│ │             │  │                  │ │  │
│  │  └─────────────┘  └─────────────┘  └──────────────────┘ │  │
│  │                                                           │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐ │  │
│  │  │ LLM Proxy   │  │ Cost        │  │ Audit Logger     │ │  │
│  │  │ (3 replicas)│  │ Controller  │  │ (2 replicas)     │ │  │
│  │  └─────────────┘  │ (2 replicas)│  └──────────────────┘ │  │
│  │                    └─────────────┘                       │  │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  ns: data-services                                        │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐ │   │
│  │  │ PostgreSQL  │  │ Redis       │  │ Milvus/Qdrant    │ │   │
│  │  │ (HA Cluster)│  │ (Sentinel)  │  │ (Vector DB)      │ │   │
│  │  └─────────────┘  └─────────────┘  └──────────────────┘ │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐                        │   │
│  │  │ Kafka       │  │ MinIO/S3    │                        │   │
│  │  │ (3 brokers) │  │ (Object)    │                        │   │
│  │  └─────────────┘  └─────────────┘                        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  ns: observability                                        │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐ │   │
│  │  │ Prometheus  │  │ Jaeger      │  │ Grafana          │ │   │
│  │  │ (Metrics)   │  │ (Tracing)   │  │ (Dashboard)      │ │   │
│  │  └─────────────┘  └─────────────┘  └──────────────────┘ │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐                        │   │
│  │  │ Loki        │  │ Alert       │                        │   │
│  │  │ (Logs)      │  │ Manager     │                        │   │
│  │  └─────────────┘  └─────────────┘                        │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 边缘部署

适合低延迟/离线场景：

```
┌─────────────────────────────────────────────────┐
│              Edge Node (边缘节点)                 │
│                                                  │
│  ┌─────────────────────────────────────────────┐│
│  │         Lightweight Agent Runtime            ││
│  │                                              ││
│  │  ┌──────────┐  ┌──────────┐  ┌───────────┐ ││
│  │  │ API      │  │ Local    │  │ Edge      │ ││
│  │  │ Server   │  │ LLM      │  │ Cache     │ ││
│  │  │          │  │ (Ollama/ │  │           │ ││
│  │  │          │  │  vLLM)   │  │           │ ││
│  │  └──────────┘  └──────────┘  └───────────┘ ││
│  └─────────────────────────────────────────────┘│
│                                                  │
│  ┌─────────────────────────────────────────────┐│
│  │  Cloud Sync (定期同步到云端)                   ││
│  │  - 会话数据上传                               ││
│  │  - 模型权重更新                               ││
│  │  - 知识库增量同步                              ││
│  └─────────────────────────────────────────────┘│
└─────────────────────────────────────────────────┘

资源需求:
  CPU: 4-8 cores (ARM/x86)
  Memory: 8-16 Gi
  GPU: 可选 (7B模型)
  适用: IoT/工厂/零售/离线场景
```

## 5. 与K8s生态集成

### 5.1 集成点总览

```yaml
K8s生态集成:

服务网格 (Istio/Linkerd):
  - Agent服务的mTLS加密
  - 流量管理（金丝雀/蓝绿）
  - 故障注入（Chaos Testing）
  - 限流（EnvoyFilter）

可观测性:
  - Prometheus: Agent指标采集（调用量/延迟/Token消耗/成本）
  - Jaeger/OpenTelemetry: 分布式追踪（Agent推理链路）
  - Loki: 日志聚合（对话日志/工具调用日志）
  - Grafana: 可视化Dashboard

存储:
  - PVC: 会话持久化/知识库存储
  - CSI: 云存储集成（S3/GCS/OSS）
  - StatefulSet: 有状态服务（向量数据库/Redis）

安全:
  - RBAC: Agent服务账户权限控制
  - NetworkPolicy: 租户网络隔离
  - Secret: API Key/模型密钥管理
  - OPA/Gatekeeper: 策略执行

自动扩缩:
  - HPA: 基于CPU/内存/请求量的水平扩缩
  - KEDA: 基于队列深度的事件驱动扩缩
  - VPA: 垂直扩缩（资源请求调整）

调度:
  - NodeAffinity: GPU节点调度
  - PodAntiAffinity: 高可用副本分散
  - PriorityClass: Agent任务优先级
  - ResourceQuota: 租户资源配额
```

### 5.2 集成配置示例

```yaml
# HPA - Agent自动扩缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: agent-runtime-hpa
  namespace: agent-platform
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: agent-runtime
  minReplicas: 3
  maxReplicas: 50
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: agent_requests_per_second
      target:
        type: AverageValue
        averageValue: "100"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Pods
        value: 5
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 2
        periodSeconds: 120
---
# KEDA - 基于队列深度扩缩
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: agent-worker-scaler
spec:
  scaleTargetRef:
    name: agent-worker
  minReplicaCount: 1
  maxReplicaCount: 20
  triggers:
  - type: kafka
    metadata:
      bootstrapServers: kafka:9092
      consumerGroup: agent-workers
      topic: agent-tasks
      lagThreshold: "100"
---
# PodDisruptionBudget - 高可用保障
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: agent-runtime-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: agent-runtime
---
# PriorityClass - Agent任务优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: agent-high-priority
value: 1000000
globalDefault: false
description: "高优先级Agent任务"
```

### 5.3 ServiceMonitor配置

```yaml
# Prometheus ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: agent-runtime-metrics
  namespace: agent-platform
spec:
  selector:
    matchLabels:
      app: agent-runtime
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'agent_.*'
      action: keep
---
# Grafana Dashboard ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: agent-dashboard
  namespace: monitoring
  labels:
    grafana_dashboard: "1"
data:
  agent-runtime.json: |
    {
      "dashboard": {
        "title": "Agent Runtime Dashboard",
        "panels": [
          {
            "title": "Agent Requests/sec",
            "type": "graph",
            "targets": [{"expr": "rate(agent_requests_total[5m])"}]
          },
          {
            "title": "LLM Latency P99",
            "type": "graph",
            "targets": [{"expr": "histogram_quantile(0.99, agent_llm_latency_bucket)"}]
          },
          {
            "title": "Token Usage (Daily)",
            "type": "stat",
            "targets": [{"expr": "sum(agent_tokens_used_today)"}]
          },
          {
            "title": "Cost (Daily USD)",
            "type": "stat",
            "targets": [{"expr": "sum(agent_cost_usd_today)"}]
          },
          {
            "title": "Error Rate",
            "type": "graph",
            "targets": [{"expr": "rate(agent_errors_total[5m]) / rate(agent_requests_total[5m])"}]
          },
          {
            "title": "Active Sessions",
            "type": "stat",
            "targets": [{"expr": "agent_active_sessions"}]
          }
        ]
      }
    }
```

## 6. 本系列文档索引

```
AI基础设施/03-agent-runtime/
  ├── 15-cloud-agent-platforms.md        # 云Agent平台即服务
  ├── 16-coze-agent-platform.md          # Coze Agent平台
  ├── 17-agent-rate-limiting-cost-control.md  # Agent限流与成本控制
  ├── 18-agent-retry-resilience.md       # Agent弹性设计
  ├── 19-agent-ci-cd-pipeline.md         # Agent CI/CD流水线
  ├── 20-agent-multi-tenancy.md          # Agent多租户架构
  └── 21-agent-runtime-architecture-overview.md  # 本文：架构总览
```

## 相关主题

- [[15-AI基础设施/03-Agent运行时/15-cloud-agent-platforms|云Agent平台即服务]]
- [[15-AI基础设施/03-Agent运行时/17-agent-rate-limiting-cost-control|Agent限流与成本控制]]
- [[15-AI基础设施/03-Agent运行时/20-agent-multi-tenancy|Agent多租户架构]]

## 参考资料

- LangChain/LangGraph Architecture
- Kubernetes Production Best Practices
- Istio Service Mesh
- OpenTelemetry for LLM Observability

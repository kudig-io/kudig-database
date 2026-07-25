---
title: Dify Agent 平台深度指南
description: 'Dify 平台架构全面解析，涵盖 API/Worker/Plugin/Proxy 四层架构、Workflow 编排、Agent 策略、知识库管理及 K8s Helm 部署'
summary: 'Dify 平台架构全面解析'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- dify
- workflow
- low-code
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
- Dify Agent 平台 是什么
- 如何 Dify Agent 平台
- Dify Workflow 编排
trigger_keywords:
- dify
- workflow
- chatflow
- knowledge-base
- plugin
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Dify Agent 平台深度指南

## 1. 平台架构

### 1.1 整体架构

Dify 是一个开源 LLM 应用开发平台，提供可视化的 Agent 和 Workflow 构建能力：

```
┌─────────────────────────────────────────────────────────────┐
│                      Dify 架构                               │
│                                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐ │
│  │ Web UI   │  │ REST API │  │ Plugin   │  │ Model Proxy  │ │
│  │ (Next.js)│  │ (Flask)  │  │ Service  │  │              │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └──────┬───────┘ │
│       │              │             │               │         │
│  ┌────┴──────────────┴─────────────┴───────────────┴──────┐  │
│  │                    API Gateway                          │  │
│  └────┬──────────────┬─────────────┬───────────────┬──────┘  │
│       │              │             │               │         │
│  ┌────┴───┐    ┌─────┴────┐  ┌────┴─────┐  ┌─────┴──────┐  │
│  │ App    │    │ Workflow │  │ Knowledge│  │ Model      │  │
│  │ Engine │    │ Engine   │  │ Base     │  │ Service    │  │
│  └────────┘    └──────────┘  └──────────┘  └────────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              Storage Layer                            │    │
│  │  ┌────────┐  ┌──────────┐  ┌─────────┐  ┌─────────┐  │    │
│  │  │Postgres│  │  Redis   │  │ Weaviate│  │  S3     │  │    │
│  │  └────────┘  └──────────┘  └─────────┘  └─────────┘  │    │
│  └──────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心服务

| 服务 | 职责 | 技术栈 |
|------|------|--------|
| API Server | REST API + 业务逻辑 | Python / Flask |
| Web Frontend | 可视化控制台 | Next.js / React |
| Worker | 异步任务处理 | Celery / Redis |
| Plugin Service | 插件加载与管理 | Python |
| Model Proxy | LLM 调用代理 | Python / 多供应商适配 |

### 1.3 数据存储

| 存储 | 用途 |
|------|------|
| PostgreSQL | 应用配置、用户数据、对话记录 |
| Redis | 缓存、会话状态、Celery Broker |
| Weaviate / Qdrant | 向量存储（知识库） |
| S3 / MinIO | 文件存储（上传文档） |

---

## 2. Workflow 编排

### 2.1 应用类型

Dify 提供两种核心应用类型：

**Chatflow（对话流）：**
- 面向多轮对话场景
- 自动管理会话状态
- 支持上下文记忆

**Workflow（工作流）：**
- 面向自动化任务
- 无状态的一次性处理
- 适合批量处理、数据管道

### 2.2 节点类型

```yaml
# Dify Workflow 节点类型
nodes:
  # 开始节点
  - type: start
    config:
      variables:
        - name: namespace
          type: string
          required: true
        - name: pod_name
          type: string
          required: true

  # LLM 节点
  - type: llm
    config:
      model: gpt-4o
      prompt: |
        你是 K8s 诊断专家。
        Pod {{pod_name}} 在 {{namespace}} 命名空间出现异常。
        请分析可能的原因。
      temperature: 0

  # 知识检索节点
  - type: knowledge_retrieval
    config:
      knowledge_base: k8s_docs
      query: "{{start.output}}"
      top_k: 5

  # 代码执行节点
  - type: code
    config:
      language: python
      code: |
        import subprocess
        result = subprocess.run(
            ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "json"],
            capture_output=True, text=True
        )
        return {"pod_status": result.stdout}

  # 条件分支节点
  - type: if_else
    config:
      conditions:
        - variable: "{{code.pod_status}}"
          operator: "contains"
          value: "CrashLoopBackOff"
          then: llm_diagnosis
          else: end_success

  # HTTP 请求节点
  - type: http_request
    config:
      method: GET
      url: "http://prometheus:9090/api/v1/query"
      params:
        query: 'container_memory_usage_bytes{pod="{{pod_name}}"}'

  # 变量聚合节点
  - type: variable_aggregator
    config:
      variables:
        - "{{llm.output}}"
        - "{{knowledge.output}}"
        - "{{http.output}}"

  # 结束节点
  - type: end
    config:
      output: "{{variable_aggregator.output}}"
```

### 2.3 工作流示例：K8s 自动诊断

```python
# 通过 API 创建和运行 Workflow
import requests

API_BASE = "http://dify-api/v1"
API_KEY = "app-xxxxx"

# 运行 Workflow
response = requests.post(
    f"{API_BASE}/workflows/run",
    headers={"Authorization": f"Bearer {API_KEY}"},
    json={
        "inputs": {
            "namespace": "default",
            "pod_name": "nginx-abc123",
        },
        "response_mode": "streaming",  # blocking / streaming
        "user": "operator-001",
    },
)

# 流式处理结果
for line in response.iter_lines():
    if line:
        event = json.loads(line.decode())
        if event["event"] == "node_started":
            print(f"[节点开始] {event['data']['node_id']}")
        elif event["event"] == "node_finished":
            print(f"[节点完成] {event['data']['node_id']}")
            print(f"  输出: {event['data'].get('outputs', {})}")
        elif event["event"] == "workflow_finished":
            print(f"[工作流完成] 状态: {event['data']['status']}")
            print(f"  最终输出: {event['data']['outputs']}")
```

---

## 3. Agent 策略

### 3.1 ReAct 策略

```yaml
# Agent 配置（ReAct 模式）
agent:
  strategy: react
  model: gpt-4o
  max_iterations: 10
  tools:
    - name: kubectl_query
      description: "查询 K8s 集群资源状态"
      parameters:
        namespace:
          type: string
          description: "命名空间"
          default: default
        resource:
          type: string
          description: "资源类型"
      api_endpoint: "http://kubectl-proxy/get"

    - name: log_search
      description: "搜索 Pod 日志中的错误"
      parameters:
        pod_name:
          type: string
        keyword:
          type: string
      api_endpoint: "http://log-service/search"

  system_prompt: |
    你是 KuDig K8s 运维专家。
    使用工具查询集群状态，分析问题根因。
    每次只调用一个工具，等待结果后再决定下一步。
```

### 3.2 Function Calling 策略

```yaml
# Function Calling 模式
agent:
  strategy: function_calling
  model: gpt-4o
  tools:
    - name: get_pod_status
      description: "获取 Pod 状态"
      parameters:
        type: object
        properties:
          namespace:
            type: string
          pod_name:
            type: string
        required: [namespace]
      # 直接映射到 OpenAI Function Schema
```

### 3.3 工具集成方式

Dify 提供三种工具集成方式：

```python
# 1. 内置工具（Dify 官方提供）
builtin_tools = [
    "web_search",      # 网页搜索
    "calculator",      # 计算器
    "wikipedia",       # 维基百科查询
    "code_interpreter", # 代码解释器
]

# 2. API 工具（通过 OpenAPI Schema 导入）
api_tool_schema = {
    "openapi": "3.0.0",
    "info": {"title": "K8s API", "version": "1.0"},
    "paths": {
        "/api/v1/pods": {
            "get": {
                "operationId": "listPods",
                "summary": "列出 Pod",
                "parameters": [
                    {
                        "name": "namespace",
                        "in": "query",
                        "schema": {"type": "string"},
                    }
                ],
            }
        }
    }
}

# 3. 自定义工具（通过插件开发）
# 在 Dify 插件系统中注册
```

---

## 4. 知识库管理

### 4.1 知识库创建

```python
# 通过 API 创建知识库
import requests

# 创建知识库
resp = requests.post(
    f"{API_BASE}/datasets",
    headers={"Authorization": f"Bearer {API_KEY}"},
    json={
        "name": "K8s 运维手册",
        "indexing_technique": "high_quality",  # high_quality / economy
        "permission": "all_team_members",
    },
)
dataset_id = resp.json()["id"]

# 上传文档
with open("k8s-troubleshooting.md", "rb") as f:
    resp = requests.post(
        f"{API_BASE}/datasets/{dataset_id}/documents",
        headers={"Authorization": f"Bearer {API_KEY}"},
        files={"file": f},
        data={
            "process_rule": json.dumps({
                "mode": "automatic",
                "rules": {
                    "pre_processing_rules": [
                        {"id": "remove_extra_spaces", "enabled": True},
                        {"id": "remove_urls_emails", "enabled": True},
                    ],
                    "segmentation": {
                        "separator": "\n\n",
                        "max_tokens": 500,
                    },
                },
            }),
        },
    )
```

### 4.2 检索模式

```yaml
# 混合检索配置
retrieval:
  model: text-embedding-3-small
  search_method: hybrid  # semantic / keyword / hybrid
  reranking:
    enabled: true
    model: rerank-v2
    top_n: 5
  top_k: 10
  score_threshold:
    enabled: true
    value: 0.5
```

### 4.3 多知识库检索

```yaml
# Agent 配置多个知识库
agent:
  knowledge_bases:
    - name: k8s_docs
      description: "K8s 官方文档"
      weight: 1.0
    - name: troubleshooting_guides
      description: "故障排查指南"
      weight: 0.8
    - name: best_practices
      description: "最佳实践"
      weight: 0.6
```

---

## 5. 插件生态

### 5.1 插件结构

```
dify-plugin-k8s/
├── manifest.yaml          # 插件元数据
├── provider/
│   ├── k8s.yaml          # Provider 定义
│   └── k8s.py            # Provider 实现
├── tools/
│   ├── get_pod.yaml      # 工具定义
│   ├── get_pod.py        # 工具实现
│   ├── get_logs.yaml
│   └── get_logs.py
└── requirements.txt
```

```yaml
# manifest.yaml
name: k8s-tools
version: 1.0.0
description: "Kubernetes 集群管理工具集"
author: KUDIG Team
type: plugin
icon: k8s.png

plugins:
  tools:
    - provider/k8s.yaml
```

```yaml
# tools/get_pod.yaml
name: get_pod_status
description: "查询 Pod 状态详情"
parameters:
  namespace:
    type: string
    description: "命名空间"
    required: true
  pod_name:
    type: string
    description: "Pod 名称"
    required: false
```

### 5.2 插件实现

```python
# tools/get_pod.py
from dify_plugin import Tool
from dify_plugin.entities.tool import ToolInvokeMessage

class GetPodTool(Tool):
    def _invoke(self, tool_parameters: dict) -> ToolInvokeMessage:
        namespace = tool_parameters.get("namespace", "default")
        pod_name = tool_parameters.get("pod_name", "")

        import subprocess
        cmd = ["kubectl", "get", "pods", "-n", namespace, "-o", "json"]
        if pod_name:
            cmd = ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "json"]

        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

        if result.returncode != 0:
            return self.create_text_message(f"查询失败: {result.stderr}")

        return self.create_json_message(json.loads(result.stdout))
```

---

## 6. K8s Helm 部署

### 6.1 Helm Chart 配置

```yaml
# values.yaml
api:
  replicaCount: 2
  image:
    repository: langgenius/dify-api
    tag: "0.6.0"
  resources:
    requests:
      cpu: "500m"
      memory: "1Gi"
    limits:
      cpu: "2000m"
      memory: "4Gi"
  env:
    - name: SECRET_KEY
      valueFrom:
        secretKeyRef:
          name: dify-secrets
          key: secret-key
    - name: DB_USERNAME
      valueFrom:
        secretKeyRef:
          name: dify-db
          key: username

worker:
  replicaCount: 2
  image:
    repository: langgenius/dify-api
    tag: "0.6.0"
  resources:
    requests:
      cpu: "500m"
      memory: "1Gi"

web:
  replicaCount: 2
  image:
    repository: langgenius/dify-web
    tag: "0.6.0"

# 外部 PostgreSQL（推荐 RDS）
externalPostgres:
  enabled: true
  host: "dify-db.xxxx.rds.amazonaws.com"
  port: 5432
  database: "dify"

# 外部 Redis
externalRedis:
  enabled: true
  host: "dify-redis.xxxx.cache.amazonaws.com"
  port: 6379

# 向量数据库
vectorStore:
  type: qdrant  # qdrant / weaviate / milvus
  qdrant:
    endpoint: "http://qdrant:6333"

# 文件存储
storage:
  type: s3  # s3 / azure_blob / local
  s3:
    bucket: "dify-files"
    region: "us-east-1"
```

### 6.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add dify https://langgenius.github.io/dify-helm
helm repo update

# 安装
helm install dify dify/dify \
  -n ai-platform \
  --create-namespace \
  -f values.yaml

# 升级
helm upgrade dify dify/dify -n ai-platform -f values.yaml
```
---

## 7. 多租户配置

### 7.1 工作空间隔离

```yaml
# Dify 支持多工作空间
workspace:
  # 每个团队独立工作空间
  teams:
    - name: "sre-team"
      plan: "professional"
      max_apps: 50
      max_knowledge_docs: 10000
    - name: "dev-team"
      plan: "basic"
      max_apps: 10
      max_knowledge_docs: 1000
```

### 7.2 API 密钥管理

```python
# 每个应用独立 API Key
# 通过 Dify 控制台创建

# 应用级别访问控制
headers = {"Authorization": "Bearer app-xxxxx"}

# 用户级别标识
payload = {"user": "user-id-123"}
```

---

## Related

- [[15-AI基础设施/03-Agent运行时/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[15-AI基础设施/03-Agent运行时/07-agent-framework-selection-guide|Agent 框架选型决策树]]

## See Also

- [[15-AI基础设施/03-Agent运行时/03-crewai-multi-agent-framework|CrewAI 多 Agent 框架]]
- [[15-AI基础设施/03-Agent运行时/06-semantic-kernel-enterprise|Semantic Kernel 企业级 Agent]]


<!-- risk-assessed -->

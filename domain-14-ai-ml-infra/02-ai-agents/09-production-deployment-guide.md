---
title: 生产部署指南：K8s 上运行 Agent 服务 (domain-14-ai-ml-infra)
description: 'title: 生产部署指南：K8s 上运行 Agent 服务'
category: general
tags:
- ai
- ai-agent
- deployment
- production
- guide
- prometheus
- istio
- redis
- postgresql
- hpa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 生产部署指南：K8s 上运行 Agent 服务 是什么
- 如何 生产部署指南：K8s 上运行 Agent 服务
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 生产部署指南：K8s
- 上运行
- Agent
- 服务
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- redis-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

title: 生产部署指南：K8s 上运行 Agent 服务
description: '# 生产部署指南：K8s 上运行 Agent 服务'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- [[Istio|istio]]
- redis
- postgresql
- hpa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 生产部署指南：K8s 上运行 Agent 服务 是什么
- 如何 生产部署指南：K8s 上运行 Agent 服务
trigger_keywords:
- 生产部署指南：K8s
- 上运行
- Agent
- 服务
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

# 生产部署指南：K8s 上运行 Agent 服务

> **文档类型**: 生产运维专题 | **最后更新**: 2026-03 | **关键词**: Agent 部署, K8s 生产, GPU 调度, HPA, 限流, 灰度发布, FastAPI, vLLM, Ray Serve, ServiceMesh

---

<!-- chunk: 概述 -->## 概述

将 Agent 服务部署到 [[Kubernetes|Kubernetes]] 生产环境，需要解决 LLM 推理服务的 GPU 资源管理、长连接和流式输出的网络处理、基于队列长度的弹性扩缩容，以及 Agent 服务特有的限流和成本控制需求。本文提供完整的生产级部署架构、YAML 清单和运维手册。

---

<!-- chunk: 1. Agent 服务架构设计 -->## 1. Agent 服务架构设计

#<!-- chunk: 1.1 生产架构全景 -->## 1.1 生产架构全景

```
                    外部流量
                       │
         ┌─────────────▼─────────────┐
         │        Ingress / Gateway   │
         │  (Kong / Nginx / Istio)    │
         │  - SSL 终止               │
         │  - 认证鉴权               │
         │  - 速率限制               │
         └─────────────┬─────────────┘
                       │
         ┌─────────────▼─────────────┐
         │      Agent API Gateway     │
         │  (FastAPI / Flask)         │
         │  - 请求路由               │
         │  - 用户配额检查            │
         │  - 异步任务入队            │
         └──────────┬────────────────┘
                    │
       ┌────────────┼────────────┐
       ▼            ▼            ▼
┌──────────┐ ┌──────────┐ ┌──────────┐
│  Agent   │ │  Agent   │ │  Agent   │
│  Worker  │ │  Worker  │ │  Worker  │
│  Pod #1  │ │  Pod #2  │ │  Pod #3  │
└──────────┘ └──────────┘ └──────────┘
       │            │            │
       └────────────┼────────────┘
                    ▼
       ┌─────────────────────────┐
       │    LLM Inference Layer  │
       │  vLLM / TGI (GPU)      │
       │  + OpenAI API (外部)    │
       └─────────────────────────┘
                    │
       ┌─────────────────────────┐
       │    Data & Storage Layer │
       │  Qdrant (向量库)         │
       │  Redis (缓存/任务队列)   │
       │  PostgreSQL (记忆/配置)  │
       └─────────────────────────┘
```

#<!-- chunk: 1.2 同步 vs 异步模式选择 -->## 1.2 同步 vs 异步模式选择

| 模式 | 适用场景 | 最大超时 | 实现复杂度 |
|------|---------|---------|-----------|
| **同步请求** | 简单问答、实时对话 | 30-60s | 低 |
| **流式输出（SSE）** | 对话场景、用户实时体验 | 无限制 | 中 |
| **异步任务** | 长时间分析、批处理、多 Agent | 无限制 | 高 |

---

<!-- chunk: 2. Agent API 服务 -->## 2. Agent API 服务

#<!-- chunk: 2.1 FastAPI Agent 服务 -->## 2.1 FastAPI Agent 服务

```python
from fastapi import FastAPI, HTTPException, Depends, BackgroundTasks
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
from typing import Optional, AsyncGenerator
import asyncio
import json
import uuid
from datetime import datetime

app = FastAPI(title="K8s Agent API", version="1.0.0")

# 请求/响应模型
class AgentRequest(BaseModel):
    task: str = Field(..., description="任务描述", max_length=2000)
    session_id: Optional[str] = Field(None, description="会话 ID，用于多轮对话")
    agent_type: str = Field("general", description="Agent 类型: general/network/storage/security")
    stream: bool = Field(False, description="是否流式输出")
    max_steps: int = Field(10, ge=1, le=20, description="最大执行步骤数")
    timeout_seconds: int = Field(60, ge=10, le=300)

class AgentResponse(BaseModel):
    request_id: str
    session_id: str
    answer: str
    steps_taken: int
    tools_used: list[str]
    success: bool
    duration_ms: float
    tokens_used: int

# 异步流式输出
async def agent_stream_generator(
    task: str,
    agent_executor,
    session_id: str,
) -> AsyncGenerator[str, None]:
    """生成 Server-Sent Events 格式的流式输出"""
    
    async for event in agent_executor.astream_events(
        {"input": task},
        version="v2",
    ):
        event_type = event["event"]
        
        if event_type == "on_chat_model_stream":
            # LLM 生成文本片段
            content = event["data"]["chunk"].content
            if content:
                yield f"data: {json.dumps({'type': 'token', 'content': content})}\n\n"
        
        elif event_type == "on_tool_start":
            # 工具调用开始
            tool_name = event["name"]
            tool_input = event["data"]["input"]
            yield f"data: {json.dumps({'type': 'tool_start', 'tool': tool_name, 'input': tool_input})}\n\n"
        
        elif event_type == "on_tool_end":
            # 工具调用结束
            tool_name = event["name"]
            yield f"data: {json.dumps({'type': 'tool_end', 'tool': tool_name})}\n\n"
        
        elif event_type == "on_chain_end" and event.get("name") == "AgentExecutor":
            # Agent 执行完成
            final_output = event["data"]["output"]["output"]
            yield f"data: {json.dumps({'type': 'done', 'content': final_output})}\n\n"
    
    yield "data: [DONE]\n\n"

@app.post("/v1/agent/run")
async def run_agent(
    request: AgentRequest,
    background_tasks: BackgroundTasks,
):
    """同步或流式执行 Agent 任务"""
    
    request_id = str(uuid.uuid4())
    session_id = request.session_id or str(uuid.uuid4())
    
    # 获取对应类型的 Agent
    agent_executor = get_agent(request.agent_type, request.max_steps)
    
    if request.stream:
        return StreamingResponse(
            agent_stream_generator(request.task, agent_executor, session_id),
            media_type="text/event-stream",
            headers={
                "X-Request-ID": request_id,
                "X-Session-ID": session_id,
                "Cache-Control": "no-cache",
                "Connection": "keep-alive",
            }
        )
    else:
        # 同步执行（带超时）
        try:
            start_time = asyncio.get_event_loop().time()
            result = await asyncio.wait_for(
                agent_executor.ainvoke({"input": request.task}),
                timeout=request.timeout_seconds,
            )
            duration_ms = (asyncio.get_event_loop().time() - start_time) * 1000
            
            # 异步记录审计日志
            background_tasks.add_task(
                log_agent_execution,
                request_id=request_id,
                session_id=session_id,
                task=request.task,
                result=result,
                duration_ms=duration_ms,
            )
            
            return AgentResponse(
                request_id=request_id,
                session_id=session_id,
                answer=result.get("output", ""),
                steps_taken=len(result.get("intermediate_steps", [])),
                tools_used=list(set(
                    step[0].tool for step in result.get("intermediate_steps", [])
                )),
                success=True,
                duration_ms=duration_ms,
                tokens_used=result.get("tokens_used", 0),
            )
        
        except asyncio.TimeoutError:
            raise HTTPException(
                status_code=408,
                detail=f"Agent 执行超时（{request.timeout_seconds}s）"
            )
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

# 健康检查
@app.get("/health")
async def health_check():
    return {
        "status": "healthy",
        "timestamp": datetime.utcnow().isoformat(),
        "version": "1.0.0",
    }

@app.get("/ready")
async def readiness_check():
    """就绪检查：验证关键依赖是否可用"""
    checks = {}
    
    # 检查 LLM 可用性
    try:
        # 轻量 ping 检查
        checks["llm"] = await check_llm_health()
    except Exception as e:
        checks["llm"] = f"unhealthy: {e}"
    
    # 检查向量库
    try:
        checks["vector_store"] = await check_qdrant_health()
    except Exception as e:
        checks["vector_store"] = f"unhealthy: {e}"
    
    all_healthy = all(v == "healthy" for v in checks.values())
    
    if not all_healthy:
        raise HTTPException(status_code=503, detail=checks)
    
    return {"status": "ready", "checks": checks}
```

---

<!-- chunk: 3. K8s 生产部署清单 -->## 3. K8s 生产部署清单

#<!-- chunk: 3.1 Agent 服务 Deployment -->## 3.1 Agent 服务 Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: k8s-agent-api
  namespace: ai-agents
  labels:
    app: k8s-agent-api
    version: v1.2.0
    env: production
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # 零停机滚动更新
  selector:
    matchLabels:
      app: k8s-agent-api
  template:
    metadata:
      labels:
        app: k8s-agent-api
        version: v1.2.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
        prometheus.io/path: "/metrics"
    spec:
      serviceAccountName: agent-api-sa
      
      # 优雅终止：等待现有请求完成
      terminationGracePeriodSeconds: 120
      
      containers:
      - name: agent-api
        image: your-registry/k8s-agent-api:v1.2.0
        imagePullPolicy: Always
        
        ports:
        - name: http
          containerPort: 8080
        
        env:
        - name: OPENAI_API_KEY
          valueFrom:
            secretKeyRef:
              name: llm-credentials
              key: openai-api-key
        - name: LANGFUSE_PUBLIC_KEY
          valueFrom:
            secretKeyRef:
              name: observability-keys
              key: langfuse-public-key
        - name: QDRANT_URL
          value: "http://qdrant.ai-infra.svc:6333"
        - name: REDIS_URL
          value: "redis://redis-master.ai-infra.svc:6379"
        - name: LOG_LEVEL
          value: "INFO"
        - name: WORKERS
          value: "4"  # Uvicorn worker 数
        - name: MAX_CONCURRENT_REQUESTS
          value: "20"
        
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "2"
            memory: "2Gi"
        
        # 就绪探针：确认依赖就绪后才接收流量
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 20
          periodSeconds: 10
          failureThreshold: 3
        
        # 存活探针：检测死锁等问题
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 30
          failureThreshold: 3
        
        # 优雅关机处理
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 10"]  # 等待 LB 摘流
      
      # 节点亲和：Agent 服务部署到非 GPU 节点
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: k8s-agent-api
              topologyKey: kubernetes.io/hostname
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-type
                operator: NotIn
                values: ["gpu"]  # 不占用 GPU 节点
      
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: k8s-agent-api
```

#<!-- chunk: 3.2 HPA（基于自定义指标） -->## 3.2 HPA（基于自定义指标）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: k8s-agent-api-hpa
  namespace: ai-agents
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: k8s-agent-api
  minReplicas: 2
  maxReplicas: 20
  
  metrics:
  # 基于 CPU 利用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60
  
  # 基于内存利用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 70
  
  # 基于请求队列深度（自定义指标）
  - type: External
    external:
      metric:
        name: redis_queue_length
        selector:
          matchLabels:
            queue: agent_task_queue
      target:
        type: AverageValue
        averageValue: "5"  # 每个 Pod 处理 5 个待处理任务
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60   # 扩容稳定窗口 1 分钟
      policies:
      - type: Pods
        value: 3
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容稳定窗口 5 分钟（防止震荡）
      policies:
      - type: Pods
        value: 1
        periodSeconds: 120
```

#<!-- chunk: 3.3 Service 和 Ingress -->## 3.3 Service 和 Ingress

```yaml
apiVersion: v1
kind: Service
metadata:
  name: k8s-agent-api
  namespace: ai-agents
  annotations:
    # 支持 WebSocket 和长连接（SSE 流式输出）
    nginx.ingress.kubernetes.io/proxy-read-timeout: "600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "600"
spec:
  selector:
    app: k8s-agent-api
  ports:
  - name: http
    port: 80
    targetPort: 8080
  type: ClusterIP

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: k8s-agent-api
  namespace: ai-agents
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
    # 全局速率限制
    nginx.ingress.kubernetes.io/limit-rpm: "60"
    nginx.ingress.kubernetes.io/limit-burst-multiplier: "5"
    # 超时配置（Agent 任务可能运行较长）
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "300"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "300"
    # 请求体大小限制
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    # 启用 gzip 压缩
    nginx.ingress.kubernetes.io/enable-access-log: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - agent-api.your-domain.com
    secretName: agent-api-tls
  rules:
  - host: agent-api.your-domain.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: k8s-agent-api
            port:
              number: 80
```

---

<!-- chunk: 4. LLM 推理服务部署（vLLM） -->## 4. LLM 推理服务部署（vLLM）

#<!-- chunk: 4.1 vLLM 生产配置 -->## 4.1 vLLM 生产配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-qwen25-72b
  namespace: ai-serving
spec:
  replicas: 1  # GPU 资源有限，通常单副本
  selector:
    matchLabels:
      app: vllm-qwen25-72b
  template:
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.6.3
        command:
        - python3
        - -m
        - vllm.entrypoints.openai.api_server
        args:
        - --model=/models/Qwen2.5-72B-Instruct
        - --served-model-name=qwen2.5-72b
        - --tensor-parallel-size=4
        - --max-model-len=32768
        - --max-num-seqs=256
        - --enable-chunked-prefill
        - --enable-prefix-caching    # KV Cache 复用，降低重复 Prefix 的延迟
        - --gpu-memory-utilization=0.9
        - --dtype=bfloat16
        - --trust-remote-code
        - --port=8000
        
        ports:
        - containerPort: 8000
        
        env:
        - name: VLLM_API_KEY
          valueFrom:
            secretKeyRef:
              name: vllm-secret
              key: api-key
        
        resources:
          limits:
            nvidia.com/gpu: "4"
            memory: "200Gi"
          requests:
            nvidia.com/gpu: "4"
            memory: "180Gi"
            cpu: "8"
        
        readinessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 120  # 模型加载需要时间
          periodSeconds: 10
          failureThreshold: 30
        
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-storage-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "20Gi"
      
      tolerations:
      - key: "nvidia.com/gpu"
        operator: "Exists"
        effect: "NoSchedule"
      
      nodeSelector:
        gpu-type: a100-80g
      
      priorityClassName: gpu-high-priority
```

#<!-- chunk: 4.2 LLM 服务的多副本路由 -->## 4.2 LLM 服务的多副本路由

```yaml
# 多模型服务统一入口（通过 Label 区分）
apiVersion: v1
kind: Service
metadata:
  name: llm-router
  namespace: ai-serving
spec:
  selector:
    # 不指定具体 app，通过 endpoints 手动管理
  ports:
  - port: 8000
    targetPort: 8000

---
# 使用 KEDA 基于 GPU 利用率扩缩容
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-scaler
  namespace: ai-serving
spec:
  scaleTargetRef:
    name: vllm-qwen25-7b  # 7B 模型可以多副本
  minReplicaCount: 1
  maxReplicaCount: 4
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring.svc:9090
      metricName: vllm_requests_waiting
      query: sum(vllm:num_requests_waiting{job="vllm"})
      threshold: "10"  # 等待队列超过 10 时扩容
```

---

<!-- chunk: 5. 灰度发布策略 -->## 5. 灰度发布策略

#<!-- chunk: 5.1 Canary 发布 -->## 5.1 Canary 发布

```yaml
# 稳定版（90% 流量）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: k8s-agent-api-stable
  labels:
    app: k8s-agent-api
    track: stable
spec:
  replicas: 9
  selector:
    matchLabels:
      app: k8s-agent-api
      track: stable
  template:
    metadata:
      labels:
        app: k8s-agent-api
        track: stable
        version: v1.1.0

---
# Canary 版（10% 流量）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: k8s-agent-api-canary
  labels:
    app: k8s-agent-api
    track: canary
spec:
  replicas: 1  # 1 副本 = 约 10% 流量
  selector:
    matchLabels:
      app: k8s-agent-api
      track: canary
  template:
    metadata:
      labels:
        app: k8s-agent-api
        track: canary
        version: v1.2.0

---
# Service 选择两个 Deployment（通过 app 标签）
apiVersion: v1
kind: Service
metadata:
  name: k8s-agent-api
spec:
  selector:
    app: k8s-agent-api  # 同时匹配 stable 和 canary
  ports:
  - port: 80
    targetPort: 8080
```

#<!-- chunk: 5.2 基于 Argo Rollouts 的智能灰度 -->## 5.2 基于 Argo Rollouts 的智能灰度

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: k8s-agent-api
  namespace: ai-agents
spec:
  replicas: 10
  strategy:
    canary:
      # 分阶段灰度，基于成功率自动推进
      analysis:
        templates:
        - templateName: agent-success-rate
        startingStep: 1
      steps:
      - setWeight: 5    # 先放 5% 流量
      - pause: {duration: 10m}  # 观察 10 分钟
      - setWeight: 20
      - pause: {duration: 10m}
      - setWeight: 50
      - pause: {duration: 10m}
      - setWeight: 100  # 全量

---
# 分析模板：成功率低于 95% 自动回滚
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: agent-success-rate
  namespace: ai-agents
spec:
  metrics:
  - name: success-rate
    interval: 2m
    successCondition: result[0] >= 0.95
    failureLimit: 2  # 连续 2 次失败触发回滚
    provider:
      prometheus:
        address: http://prometheus.monitoring.svc:9090
        query: |
          sum(rate(agent_requests_total{status="success"}[2m])) /
          sum(rate(agent_requests_total[2m]))
```

---

<!-- chunk: 6. 限流与配额管理 -->## 6. 限流与配额管理

#<!-- chunk: 6.1 用户级别限流 -->## 6.1 用户级别限流

```python
import redis.asyncio as aioredis
from fastapi import Request, HTTPException

class RateLimiter:
    """基于 Redis 的滑动窗口限流"""
    
    def __init__(self, redis_url: str):
        self.redis = aioredis.from_url(redis_url)
    
    async def check_rate_limit(
        self,
        user_id: str,
        limit: int,
        window_seconds: int,
    ) -> tuple[bool, dict]:
        """检查是否超过速率限制"""
        
        key = f"rate_limit:{user_id}"
        current_time = time.time()
        window_start = current_time - window_seconds
        
        pipe = self.redis.pipeline()
        pipe.zremrangebyscore(key, 0, window_start)  # 清理过期记录
        pipe.zadd(key, {str(current_time): current_time})
        pipe.zcard(key)
        pipe.expire(key, window_seconds)
        _, _, count, _ = await pipe.execute()
        
        remaining = max(0, limit - count)
        allowed = count <= limit
        
        headers = {
            "X-RateLimit-Limit": str(limit),
            "X-RateLimit-Remaining": str(remaining),
            "X-RateLimit-Reset": str(int(current_time + window_seconds)),
        }
        
        return allowed, headers

# 定义用户层级配额
USER_RATE_LIMITS = {
    "free": {"rpm": 5, "rpd": 50, "tokens_per_day": 100_000},
    "pro": {"rpm": 30, "rpd": 500, "tokens_per_day": 2_000_000},
    "enterprise": {"rpm": 200, "rpd": 10000, "tokens_per_day": 50_000_000},
}

@app.middleware("http")
async def rate_limit_middleware(request: Request, call_next):
    user_id = request.headers.get("X-User-ID", "anonymous")
    user_tier = await get_user_tier(user_id)
    limits = USER_RATE_LIMITS.get(user_tier, USER_RATE_LIMITS["free"])
    
    allowed, headers = await rate_limiter.check_rate_limit(
        user_id=user_id,
        limit=limits["rpm"],
        window_seconds=60,
    )
    
    if not allowed:
        raise HTTPException(
            status_code=429,
            detail="请求频率超过限制，请稍后重试",
            headers=headers,
        )
    
    response = await call_next(request)
    response.headers.update(headers)
    return response
```

---

<!-- chunk: 7. 生产运维 Runbook -->## 7. 生产运维 Runbook

#<!-- chunk: 7.1 常见故障处理 -->## 7.1 常见故障处理

```
问题1: Agent API 响应时间突增（>10s P95）

诊断步骤:
  1. kubectl top pods -n ai-agents
  2. kubectl get hpa -n ai-agents  # 检查是否需要扩容
  3. 检查 LLM 服务延迟: curl http://vllm.ai-serving/health
  4. 检查 Redis 连接: redis-cli ping
  5. 查看 Langfuse/LangSmith 追踪，定位是哪一步慢

常见原因与处理:
  a. LLM 响应慢 → 检查 GPU 利用率，必要时增加 vLLM 副本或切换备用 API
  b. 队列积压 → 增加 Agent Worker 副本
  c. 向量检索慢 → Qdrant 索引未热加载，重启并预热

恢复验证:
  kubectl run test-pod --rm -it --image=curlimages/curl -- \
    curl -X POST http://k8s-agent-api.ai-agents/v1/agent/run \
    -d '{"task": "简单测试"}'

问题2: Agent Pod OOMKilled

诊断:
  1. kubectl describe pod <pod-name> -n ai-agents | grep -A5 OOM
  2. 检查是否有超大上下文请求（Token 超过 50K 的请求）
  3. 检查 embedding 缓存是否异常增大

处理:
  1. 临时: 增加内存 limits.memory
  2. 根本: 限制单请求最大 Token 数
     MAX_INPUT_TOKENS=10000 (环境变量)
  3. 添加内存告警: agent_memory_usage > 1.5Gi

问题3: vLLM OOM（GPU 内存不足）

诊断:
  1. kubectl exec -n ai-serving <vllm-pod> -- nvidia-smi
  2. 查看 vLLM 指标: /metrics 端点的 gpu_cache_usage_perc

处理:
  1. 减少 --max-num-seqs（并发请求数）
  2. 减少 --max-model-len（最大上下文长度）
  3. 调低 --gpu-memory-utilization 至 0.85
  4. 重启 Pod: kubectl rollout restart deployment/vllm-xxx
```

#<!-- chunk: 7.2 关键监控检查清单 -->## 7.2 关键监控检查清单

```bash
# Agent 系统健康巡检脚本
#!/bin/bash

echo "=== Agent 系统健康检查 ==="
echo "时间: $(date)"

# 1. Pod 状态
echo "\n[Pod 状态]"
kubectl get pods -n ai-agents -o wide

# 2. HPA 状态
echo "\n[HPA 状态]"
kubectl get hpa -n ai-agents

# 3. Agent API 健康
echo "\n[API 健康]"
curl -s http://k8s-agent-api.ai-agents/health | python3 -m json.tool

# 4. 成功率（过去 1 小时）
echo "\n[过去1小时成功率]"
curl -s "http://prometheus.monitoring.svc:9090/api/v1/query?query=\
  sum(rate(agent_requests_total{status='success'}[1h]))/\
  sum(rate(agent_requests_total[1h]))*100" | \
  python3 -c "import sys,json; data=json.load(sys.stdin); \
  print(f\"成功率: {float(data['data']['result'][0]['value'][1]):.1f}%\")"

# 5. LLM 服务状态
echo "\n[LLM 服务状态]"
kubectl get pods -n ai-serving -l app=vllm

# 6. Redis 状态
echo "\n[Redis 队列深度]"
kubectl exec -n ai-infra redis-master-0 -- redis-cli llen agent_task_queue
```

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **零停机部署**：`maxUnavailable: 0` + `preStop sleep` + 就绪探针的组合确保无缝滚动更新
- **流式输出优先**：对话场景必须支持 SSE 流式输出，显著提升用户体验
- **HPA 使用自定义指标**：CPU 利用率不能准确反映 Agent 负载，用任务队列深度更准确
- **LLM 服务独立部署**：vLLM 和 Agent 服务分开部署，避免相互影响并利于独立扩缩容
- **灰度发布必须携带质量分析**：纯按比例的 Canary 不够，要加自动回滚的成功率检测

#<!-- chunk: 反模式 -->## 反模式

- **Agent 和 LLM 共用 Pod**：两者资源需求差异极大，合并导致资源浪费或 OOM
- **无限流请求体大小**：不设置 `proxy-body-size`，大型 Prompt 攻击会打垮服务
- **就绪探针不检查 LLM**：Agent 启动了但 LLM 连不上，就绪探针仍然通过，接入流量后全部失败
- **不设 terminationGracePeriodSeconds**：滚动更新时强制终止进行中的 Agent 任务，造成用户体验断裂

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 多 Worker Pod 的协同 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | Prometheus 指标和 Langfuse |
| [11 - 成本优化](./11-cost-latency-optimization.md) | 资源配额和成本控制 |
| [domain-14-ai-ml-infra/17-llm-inference-serving.md](../domain-14-ai-ml-infra/17-llm-inference-serving.md) | vLLM/TGI 推理服务详情 |
| [domain-02-workloads-applications](../domain-02-workloads-applications/) | K8s Deployment 最佳实践 |
| [domain-32-yaml-manifests](../domain-18-manifests-patterns/) | 完整 YAML 模板参考 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization|成本与延迟优化策略]]

## Related

- 40-agent-harness-production-maturity
- 41-react-harness-identification-guide

## See Also

- 07-memory-context-management
- 08-agent-evaluation-observability
- 10-security-guardrails
- 11-cost-latency-optimization

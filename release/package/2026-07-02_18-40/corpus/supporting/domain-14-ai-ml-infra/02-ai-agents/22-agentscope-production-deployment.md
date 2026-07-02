---
title: AgentScope 生产部署与可观测性 (domain-14-ai-ml-infra)
description: 'title: AgentScope 生产部署与可观测性'
summary: 'title: AgentScope 生产部署与可观测性'
category: general
tags:
- ai
- ai-agent
- deployment
- production
- prometheus
- grafana
- jaeger
- docker
- opa
- redis
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AgentScope 生产部署与可观测性 是什么
- 如何 AgentScope 生产部署与可观测性
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 生产部署与可观测性
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- redis-basics
- policy-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: AgentScope 生产部署与可观测性
description: '# AgentScope 生产部署与可观测性'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- grafana
- [[Jaeger|jaeger]]
- docker
- opa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 生产部署与可观测性 是什么
- 如何 AgentScope 生产部署与可观测性
trigger_keywords:
- AgentScope
- 生产部署与可观测性
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

# AgentScope 生产部署与可观测性

> **文档类型**: 生产部署专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Runtime, AgentApp, 生产部署, Docker, [[Kubernetes|Kubernetes]], Serverless, Sandbox, AgentScope Studio, OpenTelemetry, Tracing, 可观测性, AaaS, Agent-as-a-Service

---

<!-- chunk: 概述 -->## 概述

从开发环境到生产环境，Agent 系统需要解决状态管理、安全执行、弹性伸缩和全链路可观测等关键问题。AgentScope 通过独立的 **agentscope-runtime** 项目提供生产级运行时，支持 Agent-as-a-Service（AaaS）模式部署，内置[[domain-14-ai-ml-infra/03-agent-runtime/12-agent-sandbox-isolation.md|沙箱安全执行]]、OpenTelemetry 追踪和 K8s 原生部署能力。

本文系统讲解 AgentScope 的生产部署全流程。

---

<!-- chunk: 1. 生产部署架构全景 -->## 1. 生产部署架构全景

```
AgentScope 生产部署架构
│
├── 客户端层
│   ├── Web 前端（curl / 浏览器）
│   ├── CLI 工具
│   └── 其他 Agent（A2A 协议）
│
├── API 网关层
│   └── Nginx / Kong / APISIX
│       ├── 认证鉴权
│       ├── 限流
│       └── 负载均衡
│
├── Agent 服务层（AgentScope Runtime）
│   ├── AgentApp（FastAPI 继承）
│   │   ├── /process — SSE 流式响应
│   │   ├── /health — 健康检查
│   │   └── 自定义端点
│   ├── 状态管理
│   │   ├── JSONSession
│   │   └── Agent state_dict（同步）
│   ├── 记忆持久化
│   │   ├── AsyncSQLAlchemyMemory（连接池）
│   │   └── RedisMemory（分布式）
│   └── 沙箱执行
│       └── 安全隔离的工具执行环境
│
├── 存储层
│   ├── PostgreSQL/SQLite — AsyncSQLAlchemyMemory 持久化
│   ├── Redis — RedisMemory 分布式记忆
│   └── 向量数据库 — RAG 检索
│
└── 可观测性层
    ├── agentscope.init(tracing_url=...) — 全链路追踪
    ├── AgentScope Studio — 可视化
    ├── 第三方集成 — Arize-Phoenix / Langfuse / CloudMonitor
    └── Prometheus + Grafana — 指标监控
```

---

<!-- chunk: 2. AgentScope Runtime -->## 2. AgentScope Runtime

## 2.1 什么是 Runtime

AgentScope Runtime（`agentscope-runtime`）是独立于核心框架的**生产运行时**，提供：

```
# 🟢 低风险：只读/信息收集，通常无副作用
AgentScope Runtime 核心能力
│
├── Agent-as-a-Service（AaaS）
│   将 Agent 暴露为流式 API 服务
│
├── 工具沙箱
│   工具调用在安全隔离的沙箱中执行
│
├── 弹性部署
│   本地 / Docker / K8s / Serverless
│
├── 全栈可观测性
│   日志 / 追踪 / 指标
│
└── 框架兼容
    不仅支持 AgentScope，还兼容 LangGraph、AutoGen 等
```
## 2.2 安装

```bash
# 核心安装
pip install agentscope-runtime

# 安装扩展
pip install "agentscope-runtime[ext]"

# 预览版
pip install --pre agentscope-runtime
```

## 2.3 框架兼容性

| 框架 | 消息/事件 | 工具 |
|------|----------|------|
| AgentScope | 完整支持 | 完整支持 |
| LangGraph | 完整支持 | 开发中 |
| Microsoft Agent Framework | 完整支持 | 完整支持 |
| Agno | 完整支持 | 完整支持 |
| AutoGen | 开发中 | 完整支持 |

---

<!-- chunk: 3. AgentApp — Agent 服务化 -->## 3. AgentApp — Agent 服务化

## 3.1 三阶段开发模式

AgentApp 采用 **init → query → shutdown** 三阶段模式：

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
AgentApp 生命周期
│
├── init（启动阶段）
│   ├── 初始化 Session 管理器
│   ├── 加载模型配置
│   └── 预热连接池
│
├── query（请求处理）
│   ├── 接收请求
│   ├── 加载 Agent 状态
│   ├── 执行推理和工具调用
│   ├── 流式返回响应（SSE）
│   └── 保存 Agent 状态
│
└── shutdown（关闭阶段）
    ├── 保存未完成的状态
    ├── 关闭连接池
    └── 清理资源
```
## 3.2 完整示例

```python
import os
from contextlib import asynccontextmanager

from fastapi import FastAPI
from agentscope.agent import ReActAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.tool import Toolkit, execute_python_code
from agentscope.pipeline import stream_printing_messages
from agentscope.memory import AsyncSQLAlchemyMemory, CompressionConfig
from agentscope.session import JSONSession

from agentscope_runtime.engine import AgentApp
from agentscope_runtime.engine.schemas.agent_schemas import AgentRequest


# 1. 生命周期管理
@asynccontextmanager
async def lifespan(app: FastAPI):
    """服务启动和关闭的资源管理"""
    import agentscope

    # 启动: 初始化追踪和 Session
    agentscope.init(
        studio_url=os.getenv("STUDIO_URL"),
        tracing_url=os.getenv("TRACING_URL"),
    )
    app.state.session = JSONSession(save_dir=os.getenv("SESSION_DIR", "./sessions"))

    print("AgentApp 启动完成")
    yield

    print("AgentApp 已关闭")


# 2. 创建 AgentApp
agent_app = AgentApp(
    app_name="K8s-Expert",
    app_description="K8s 运维诊断智能体服务",
    lifespan=lifespan,
)


# 3. 请求处理逻辑
@agent_app.query(framework="agentscope")
async def query_func(
    self,
    msgs,
    request: AgentRequest = None,
    **kwargs,
):
    session_id = request.session_id
    user_id = request.user_id

    # 创建工具
    toolkit = Toolkit()
    toolkit.register_tool_function(execute_python_code)
    # 注册 K8s 工具...

    # 创建 Agent（使用 AsyncSQLAlchemyMemory 持久化记忆）
    agent = ReActAgent(
        name="K8s-Expert",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.getenv("DASHSCOPE_API_KEY"),
            stream=True,
        ),
        sys_prompt="你是 K8s 运维诊断专家...",
        toolkit=toolkit,
        memory=AsyncSQLAlchemyMemory(
            url=os.getenv("DB_URL", "sqlite+aiosqlite:///./memory.db"),
            pool_size=10,
        ),
        compression_config=CompressionConfig(
            trigger_threshold=50,
            keep_recent=10,
        ),
        formatter=DashScopeChatFormatter(),
    )
    agent.set_console_output_enabled(enabled=False)

    # 恢复会话状态（同步 API）
    agent_app.state.session.load_session_state(
        session_id=session_id,
        user_id=user_id,
        agent=agent,
    )

    # 流式执行
    async for msg, last in stream_printing_messages(
        agents=[agent],
        coroutine_task=agent(msgs),
    ):
        yield msg, last

    # 保存会话状态（同步 API）
    agent_app.state.session.save_session_state(
        session_id=session_id,
        user_id=user_id,
        agent=agent,
    )


# 4. 启动服务
agent_app.run(host="0.0.0.0", port=8090)
```

## 3.3 API 调用

```bash
# SSE 流式请求
curl -N \
  -X POST "http://localhost:8090/process" \
  -H "Content-Type: application/json" \
  -d '{
    "input": [
      {
        "role": "user",
        "content": [
          {"type": "text", "text": "Pod nginx-xxx 处于 Pending 状态，请诊断"}
        ]
      }
    ],
    "session_id": "session-001",
    "user_id": "ops-engineer-001"
  }'
```

**SSE 响应格式**：

```
data: {"sequence_number":0,"object":"response","status":"created",...}
data: {"sequence_number":1,"object":"response","status":"in_progress",...}
data: {"sequence_number":2,"object":"message","status":"in_progress",...}
data: {"sequence_number":3,"object":"content","status":"in_progress","text":"正在"}
data: {"sequence_number":4,"object":"content","status":"in_progress","text":"检查 Pod 状态..."}
...
data: {"sequence_number":N,"object":"response","status":"completed",...}
```

---

<!-- chunk: 4. Sandbox — 安全沙箱执行 -->## 4. Sandbox — 安全沙箱执行

## 4.1 为什么需要沙箱

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```
无沙箱风险
│
├── Agent 执行 "rm -rf /" → 系统级灾难  # ⚠️ 删除系统/数据文件
├── Agent 执行恶意代码 → 安全漏洞
├── Agent 访问敏感文件 → 数据泄露
└── Agent 消耗大量资源 → 宿主机 OOM
```

## 4.2 AgentScope 沙箱类型

| 沙箱类型 | 适用场景 | 隔离级别 |
|---------|---------|---------|
| **PythonSandbox** | Python 代码执行 | 进程级隔离 |
| **ShellSandbox** | Shell 命令执行 | 进程级隔离 |
| **GuiSandbox** | GUI 应用操作 | 容器级隔离 + VNC |
| **BrowserSandbox** | 浏览器自动化 | 容器级隔离 + VNC |
| **FilesystemSandbox** | 文件系统操作 | 容器级隔离 |
| **MobileSandbox** | 移动端操作 | 容器级隔离 |

## 4.3 沙箱配置示例

```python
from agentscope_runtime.sandbox import PythonSandbox

# 创建 Python 沙箱
sandbox = PythonSandbox(
    # 资源限制
    max_memory_mb=512,
    max_cpu_seconds=30,
    timeout_seconds=60,

    # 网络隔离
    network_enabled=False,

    # 文件系统限制
    writable_dirs=["/tmp/agent_workspace"],
    readonly_dirs=["/opt/data"],
)

# 在沙箱中执行代码
result = await sandbox.run_ipython_cell(
    code="import os; print(os.listdir('/tmp'))"
)
```

---

<!-- chunk: 5. 部署方式 -->## 5. 部署方式

## 5.1 本地部署

```bash
# 直接运行
python agent_app.py

# 或使用 uvicorn
uvicorn agent_app:agent_app --host 0.0.0.0 --port 8090 --workers 4
```

## 5.2 Docker 部署

```dockerfile
# Dockerfile
FROM python:3.11-slim

WORKDIR /app

# 安装依赖
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 复制应用代码
COPY . .

# 健康检查
HEALTHCHECK --interval=30s --timeout=5s \
  CMD curl -f http://localhost:8090/health || exit 1

EXPOSE 8090

CMD ["python", "agent_app.py"]
```

```yaml
# docker-compose.yml
version: "3.8"
services:
  agent:
    build: .
    ports:
      - "8090:8090"
    environment:
      - DASHSCOPE_API_KEY=${DASHSCOPE_API_KEY}
      - REDIS_HOST=redis
      - REDIS_PORT=6379
    depends_on:
      - redis
    restart: unless-stopped

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    volumes:
      - redis-data:/data
    command: redis-server --appendonly yes

volumes:
  redis-data:
```

## 5.3 Kubernetes 部署

```yaml
# k8s-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agentscope-k8s-expert
  namespace: agent-system
  labels:
    app: agentscope-k8s-expert
spec:
  replicas: 2
  selector:
    matchLabels:
      app: agentscope-k8s-expert
  template:
    metadata:
      labels:
        app: agentscope-k8s-expert
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8090"
    spec:
      containers:
        - name: agent
          image: your-registry/agentscope-k8s-expert:v1.0
          ports:
            - containerPort: 8090
              name: http
          env:
            - name: DASHSCOPE_API_KEY
              valueFrom:
                secretKeyRef:
                  name: agent-secrets
                  key: dashscope-api-key
            - name: REDIS_HOST
              value: "agent-redis.agent-system.svc.cluster.local"
            - name: REDIS_PORT
              value: "6379"
          resources:
            requests:
              memory: "512Mi"
              cpu: "250m"
            limits:
              memory: "2Gi"
              cpu: "1000m"
          livenessProbe:
            httpGet:
              path: /health
              port: 8090
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /health
              port: 8090
            initialDelaySeconds: 5
            periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: agentscope-k8s-expert
  namespace: agent-system
spec:
  selector:
    app: agentscope-k8s-expert
  ports:
    - port: 8090
      targetPort: 8090
      protocol: TCP
  type: ClusterIP
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: agentscope-k8s-expert-hpa
  namespace: agent-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: agentscope-k8s-expert
  minReplicas: 2
  maxReplicas: 10
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
---
apiVersion: v1
kind: Secret
metadata:
  name: agent-secrets
  namespace: agent-system
type: Opaque
stringData:
  dashscope-api-key: "sk-your-dashscope-api-key"
```

## 5.4 Ingress 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: agentscope-ingress
  namespace: agent-system
  annotations:
    nginx.ingress.kubernetes.io/proxy-read-timeout: "300"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "300"
    # SSE 需要关闭缓冲
    nginx.ingress.kubernetes.io/proxy-buffering: "off"
spec:
  ingressClassName: nginx
  rules:
    - host: agent.your-domain.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: agentscope-k8s-expert
                port:
                  number: 8090
  tls:
    - hosts:
        - agent.your-domain.com
      secretName: agent-tls
```

---

<!-- chunk: 6. AgentScope Studio -->## 6. AgentScope Studio

## 6.1 功能概览

AgentScope Studio 是独立的本地可视化开发工具，基于 Node.js，为 AgentScope Agent 应用提供透明、直观的开发、调试和评测体验。

> 官方仓库: https://github.com/agentscope-ai/agentscope-studio

```
AgentScope Studio
│
├── 追踪可视化
│   ├── OpenTelemetry Trace 展示
│   ├── LLM 调用详情（Token、延迟、成本）
│   ├── 工具调用链路
│   └── Agent 决策路径回放
│
├── 项目管理
│   ├── Projects & Runs 组织管理
│   ├── 配置管理
│   └── 版本对比
│
├── 实时交互
│   └── Chatbot 风格的 Agent 实时对话界面
│
├── 评测界面
│   ├── 评测结果可视化
│   ├── Agent 版本 A/B 对比
│   └── 评分分布分析
│
└── 内置 Copilot（Friday）
    ├── 开发助手 + Playground
    └── 快速二次开发与高级功能集成
```

## 6.2 安装与启动

**前置条件**：Node.js >= 20.0.0、npm >= 10.0.0，RHEL/CentOS 还需安装 `gcc-c++ make`（编译原生模块 better-sqlite3）。

> 详细的环境准备步骤见 [16 - 概述与安装入门 § 3.6](./16-agentscope-overview-installation.md)。

**方式一：NPM 安装（推荐）**

```bash
# 国内环境建议使用淘宝镜像
npm config set registry https://registry.npmmirror.com

# 全局安装
npm install -g @agentscope/studio

# 启动（默认 http://localhost:3000）
as_studio

# 生产环境: 绑定所有网卡 + 后台运行
nohup as_studio --host 0.0.0.0 > /tmp/as_studio.log 2>&1 &
```

**方式二：Docker 部署**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 国内环境需配置镜像加速（Docker Hub 直连可能超时）
# Podman 用户: 编辑 /etc/containers/registries.conf 添加 mirror
docker run -d --name as-studio -p 3000:3000 agentscope/studio:latest
```
**方式三：从源码运行（开发模式）**

```bash
git clone https://github.com/agentscope-ai/agentscope-studio
cd agentscope-studio
npm install
npm run dev
```

## 6.3 连接 AgentScope 应用

在 Python 代码中配置 `studio_url`，Agent 的运行数据（Trace、LLM 调用、工具链路）将实时上报到 Studio：

```python
import agentscope

agentscope.init(
    # ...其他配置...
    studio_url="http://localhost:3000"
)
```

如果 Studio 和 Agent 应用分开部署（如 Studio 在本地 macOS，Agent 在远程服务器），将 `studio_url` 指向 Studio 实际地址即可。

## 6.4 云服务器访问排查

在阿里云 ECS 等云服务器上部署后无法通过公网 IP 访问时，按以下顺序排查：

```bash
# ① 确认服务正常运行且绑定地址正确
ss -tlnp | grep 3000
# 正确: 0.0.0.0:3000    错误: 127.0.0.1:3000

# ② 本机验证
curl -s http://127.0.0.1:3000 | head -5

# ③ 防火墙放行
firewall-cmd --add-port=3000/tcp --permanent
firewall-cmd --reload

# ④ 阿里云安全组: ECS 控制台 → 安全组 → 入方向 → 添加 TCP/3000 规则
```

---

<!-- chunk: 7. OpenTelemetry Tracing -->## 7. OpenTelemetry Tracing

## 7.1 AgentScope 追踪体系

AgentScope 基于 OpenTelemetry 实现全链路追踪：

```
追踪数据结构
│
├── Trace（完整请求链路）
│   ├── Span: Agent.reply()
│   │   ├── Span: Formatter.format()
│   │   ├── Span: Model.invoke() [LLM 调用]
│   │   │   ├── 属性: model_name, tokens, latency
│   │   │   └── 属性: prompt_tokens, completion_tokens
│   │   ├── Span: Tool.execute() [工具调用]
│   │   │   ├── 属性: tool_name, input, output
│   │   │   └── 属性: duration, success
│   │   └── Span: Memory.add()
│   └── Span: Agent.print()
│
└── 支持导出到:
    ├── AgentScope Studio（内置可视化）
    ├── Jaeger
    ├── Zipkin
    └── 任何 OTLP 兼容后端
```

## 7.2 启用追踪

通过 `agentscope.init()` 统一初始化追踪（与 lifespan 中一致）：

```python
import agentscope

# 方式一：导出到 AgentScope Studio（推荐）
agentscope.init(
    studio_url="http://studio:3000",
)

# 方式二：导出到第三方 OTLP 后端（Jaeger / Zipkin / Arize-Phoenix）
agentscope.init(
    tracing_url="http://otel-collector:4317",
)

# 方式三：同时导出到 Studio 和第三方后端
agentscope.init(
    studio_url="http://studio:3000",
    tracing_url="http://otel-collector:4317",
)
```

> **注意**：`agentscope.init()` 应在应用启动时调用一次（如 FastAPI lifespan），而非每次请求调用。
> 无需手动导入 `setup_tracing`，`agentscope.init()` 内部会自动完成追踪配置。

## 7.2.1 第三方集成

| 后端 | 配置方式 | 说明 |
|------|---------|------|
| **AgentScope Studio** | `studio_url=...` | 内置可视化，Trace + 评测 |
| **Arize-Phoenix** | `tracing_url=...` 指向 Phoenix OTLP | 开源 LLM 可观测性 |
| **Langfuse** | `tracing_url=...` 指向 Langfuse OTLP | 开源 LLM 追踪 |
| **Alibaba Cloud CloudMonitor** | `tracing_url=...` 指向阿里云 OTLP | 企业级，与 DashScope 深度集成 |
| **Jaeger / Zipkin** | `tracing_url=...` 指向对应 OTLP | 通用分布式追踪 |

## 7.3 追踪数据示例

```
Trace: k8s-diagnosis-001 (总耗时: 12.3s)
│
├── [0-0.5s] Agent.reply() 开始
├── [0.5-1.2s] Formatter.format() — 格式化 5 条消息
├── [1.2-3.8s] Model.invoke() — qwen-max
│   ├── prompt_tokens: 2,340
│   ├── completion_tokens: 186
│   └── tool_calls: ["kubectl_get_pods", "kubectl_describe_resource"]
├── [3.8-5.1s] Tool.execute(kubectl_get_pods) — 1.3s
├── [5.1-7.2s] Tool.execute(kubectl_describe_resource) — 2.1s
├── [7.2-10.5s] Model.invoke() — qwen-max（第二轮推理）
│   ├── prompt_tokens: 4,120
│   └── completion_tokens: 523
├── [10.5-12.0s] Memory.add() — 保存对话
└── [12.0-12.3s] Agent.print() — 输出结果
```

---

<!-- chunk: 8. 生产最佳实践 -->## 8. 生产最佳实践

## 8.1 部署清单

| 检查项 | 必要性 | 说明 |
|--------|--------|------|
| API Key 使用 Secret 管理 | 必须 | 不硬编码在镜像或配置中 |
| 健康检查配置 | 必须 | liveness + readiness |
| 资源限制（requests/limits） | 必须 | 防止单 Pod 耗尽节点资源 |
| HPA 自动伸缩 | 推荐 | 应对流量波动 |
| Session 持久化（JSONSession / Redis） | 必须 | 防止重启或副本漂移导致状态丢失 |
| SSE 代理配置 | 必须 | Nginx 关闭缓冲 |
| OpenTelemetry 追踪 | 推荐 | 生产问题排查必备 |
| 沙箱执行 | 必须 | 代码执行工具必须隔离 |
| 限流配置 | 推荐 | API 网关层限流 |
| TLS 加密 | 必须 | 生产环境必须 HTTPS |

## 8.2 监控告警

```yaml
# Prometheus 告警规则示例
groups:
  - name: agentscope-alerts
    rules:
      # Agent 响应延迟过高
      - alert: AgentHighLatency
        expr: histogram_quantile(0.95, agent_reply_duration_seconds_bucket) > 30
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Agent P95 延迟超过 30 秒"

      # LLM 调用失败率过高
      - alert: LLMHighErrorRate
        expr: rate(llm_invoke_errors_total[5m]) / rate(llm_invoke_total[5m]) > 0.05
        for: 3m
        labels:
          severity: critical
        annotations:
          summary: "LLM 调用错误率超过 5%"

      # Token 消耗异常
      - alert: HighTokenConsumption
        expr: sum(rate(llm_tokens_total[1h])) > 1000000
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "每小时 Token 消耗超过 100 万"
```

## 8.3 成本控制

```
成本控制策略
│
├── 模型路由
│   简单问题 → qwen-turbo（低成本）
│   复杂问题 → qwen-max（高质量）
│
├── 语义缓存
│   相似问题直接返回缓存结果
│   减少重复 LLM 调用
│
├── Token 预算
│   每个会话设置 Token 上限
│   超出预算时降级或拒绝
│
└── 批处理
    非实时请求合并处理
    利用 LLM 批量推理降低单价
```

---

<!-- chunk: 9. 故障排查 -->## 9. 故障排查

## 9.1 常见问题

| 问题 | 排查方向 | 解决方案 |
|------|---------|---------|
| SSE 连接中断 | Nginx proxy_read_timeout 过短 | 增加到 300s+，关闭 proxy_buffering |
| Agent 响应缓慢 | LLM API 延迟 or 工具执行慢 | 检查 Tracing，定位瓶颈 Span |
| 会话状态丢失 | Session 存储异常 | JSONSession 检查文件权限；Redis 检查连接和 AOF |
| 沙箱执行失败 | Docker 服务异常 | 检查 Docker daemon，验证沙箱镜像 |
| OOM Killed | Agent 上下文过大 | 启用记忆压缩，减小 max_iters |
| Token 超限 | 历史消息过多 | 配置上下文窗口管理，启用截断 |

## 9.2 诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Agent 服务健康
curl http://agent-service:8090/health

# 查看 Agent Pod 状态
kubectl get pods -n agent-system -l app=agentscope-k8s-expert

# 查看 Agent 日志
kubectl logs -n agent-system -l app=agentscope-k8s-expert --tail=100

# 查看 Redis Session 状态
kubectl exec -n agent-system agent-redis-0 -- redis-cli info memory

# 查看 HPA 状态
kubectl get hpa -n agent-system
```
---

<!-- chunk: 10. 最佳实践与反模式 -->## 10. 最佳实践与反模式

## 最佳实践

- **AgentApp 继承 FastAPI**：充分利用 FastAPI 生态（中间件、依赖注入、OpenAPI 文档）
- **每次 reply 后保存状态**：防止异常导致会话丢失
- **SSE 流式响应**：生产环境必须使用流式，避免用户等待超时
- **沙箱执行所有代码**：`execute_python_code` 和 `execute_shell_command` 必须在沙箱中运行
- **追踪覆盖所有 LLM 调用**：Token 消耗和延迟是成本控制的基础数据

## 反模式

- **Agent 服务无状态但不持久化 Session**：多副本部署时会话漂移导致状态丢失
- **Nginx 默认配置处理 SSE**：缓冲会导致流式响应变为批量响应
- **不设资源限制**：单个 Agent 复杂推理可能消耗大量内存
- **所有请求用最强模型**：80% 的简单问题可用轻量模型处理，节省 90% 成本
- **忽视 Tracing**：生产环境无追踪时，性能问题和错误几乎无法定位

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [16 - 概述与安装](./16-agentscope-overview-installation.md) | AgentScope 基础安装 |
| [19 - 记忆管理](./19-agentscope-memory-context.md) | Session 持久化详解 |
| [21 - 高级特性](./21-agentscope-advanced-features.md) | 评测、Hooks、A2A |
| [09 - 生产部署指南](./09-production-deployment-guide.md) | K8s Agent 服务通用部署 |
| [domain-20-enterprise-monitoring-alerting](../domain-06-observability/) | 监控告警体系 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents MOC
- [[domain-14-ai-ml-infra/02-ai-agents/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/02-ai-agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/02-ai-agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/02-ai-agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/02-ai-agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/02-ai-agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/02-ai-agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/02-ai-agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/02-ai-agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/02-ai-agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/02-ai-agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## Related

- 40-agent-harness-production-maturity

## See Also

- 20-agentscope-multi-agent-orchestration
- 21-agentscope-advanced-features
- 23-agent-cli-fundamentals
- 24-agent-cli-tools-comparison


<!-- risk-assessed -->

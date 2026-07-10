---
title: CrewAI 多 Agent 框架深度指南
description: 'CrewAI 四层抽象（Crew/Agent/Task/Tool）全面解析，涵盖角色定义、任务委派、流程模式、记忆机制及 K8s 生产部署'
summary: 'CrewAI 四层抽象全面解析，涵盖角色定义、任务委派、流程模式及 K8s 部署'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- crewai
- multi-agent
- orchestration
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
- CrewAI 多 Agent 框架 是什么
- 如何 CrewAI 多 Agent 框架
- CrewAI 角色定义与任务委派
trigger_keywords:
- crewai
- multi-agent
- crew
- agent
- task
- delegation
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


# CrewAI 多 Agent 框架深度指南

## 1. 核心架构

### 1.1 四层抽象模型

CrewAI 围绕四个核心概念构建多 Agent 协作系统：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────┐
│                   Crew（团队）                    │
│  ┌──────────────────────────────────────────┐    │
│  │  Process: Sequential / Hierarchical      │    │
│  └──────────────────────────────────────────┘    │
│                                                  │
│  ┌──────────────┐  ┌──────────────┐              │
│  │ Agent A      │  │ Agent B      │              │
│  │ Role: 诊断师  │  │ Role: 修复师  │              │
│  │ Goal: 定位根因 │  │ Goal: 执行修复 │              │
│  │ Tools: [k8s] │  │ Tools: [kubectl]│            │
│  └──────┬───────┘  └──────┬───────┘              │
│         │                  │                     │
│  ┌──────┴───────┐  ┌──────┴───────┐              │
│  │ Task 1       │  │ Task 2       │              │
│  │ 诊断 Pod 异常  │  │ 执行修复方案   │              │
│  │ Context: []  │  │ Context: [T1] │              │
│  └──────────────┘  └──────────────┘              │
└─────────────────────────────────────────────────┘
```
### 1.2 Agent 定义

```python
from crewai import Agent

# K8s 诊断专家 Agent
diagnostician = Agent(
    role="K8s 诊断专家",
    goal="快速准确地定位 Kubernetes 集群中 Pod 异常的根因",
    backstory=(
        "你是一位资深的 Kubernetes SRE 工程师，拥有 10 年集群运维经验。"
        "你擅长从日志、事件和指标中提取关键信息，用排除法缩小故障范围。"
        "你总是先收集足够的数据再下结论，从不猜测。"
    ),
    tools=[kubectl_tool, log_analyzer_tool, metrics_query_tool],
    llm="gpt-4o",
    verbose=True,
    allow_delegation=False,  # 是否允许委派任务给其他 Agent
    max_iter=15,             # 最大推理迭代次数
    max_retry_limit=3,       # 最大重试次数
    memory=True,             # 启用短期记忆
)

# 修复执行 Agent
fixer = Agent(
    role="K8s 修复工程师",
    goal="安全高效地执行 Kubernetes 集群修复操作",
    backstory=(
        "你是 K8s 集群修复专家，擅长在最小影响范围内恢复服务。"
        "你总是先确认回滚方案再执行任何写操作。"
    ),
    tools=[kubectl_apply_tool, rollout_tool],
    llm="gpt-4o",
    allow_delegation=False,
)

# 验证 Agent
validator = Agent(
    role="修复验证工程师",
    goal="验证修复操作是否成功，确认服务恢复正常",
    backstory=(
        "你负责在修复后进行全面验证，确保问题已解决且无副作用。"
    ),
    tools=[kubectl_tool, health_check_tool, metrics_query_tool],
    llm="gpt-4o",
    allow_delegation=True,  # 可以委派任务
)
```

### 1.3 Task 定义

```python
from crewai import Task

# 诊断任务
diagnosis_task = Task(
    description=(
        "分析 default 命名空间下 nginx-deployment 的 Pod 异常。"
        "具体步骤：\n"
        "1. 查看 Pod 状态和最近事件\n"
        "2. 检查容器日志（最近 500 行）\n"
        "3. 查看 Pod 资源使用情况\n"
        "4. 检查相关 ConfigMap 和 Secret 是否存在\n"
        "5. 输出根因分析报告"
    ),
    expected_output=(
        "一份结构化的诊断报告，包含：\n"
        "- 问题描述\n"
        "- 证据列表（日志片段、事件、指标）\n"
        "- 根因分析\n"
        "- 修复建议（含具体命令）\n"
        "- 风险评估"
    ),
    agent=diagnostician,
    # 输出文件（可选）
    output_file="diagnosis_report.md",
)

# 修复任务（依赖诊断任务的输出）
fix_task = Task(
    description=(
        "根据诊断报告执行修复操作。要求：\n"
        "1. 先列出修复步骤和回滚方案\n"
        "2. 逐步执行，每步验证\n"
        "3. 记录执行过程和结果"
    ),
    expected_output="修复执行报告，包含每步操作和结果",
    agent=fixer,
    context=[diagnosis_task],  # 依赖诊断任务的输出
)

# 验证任务
validation_task = Task(
    description=(
        "验证修复是否成功：\n"
        "1. 检查 Pod 状态是否 Running\n"
        "2. 验证健康检查是否通过\n"
        "3. 确认无新的错误事件\n"
        "4. 对比修复前后的指标"
    ),
    expected_output="验证报告，确认服务已恢复正常",
    agent=validator,
    context=[fix_task],
)
```

---

## 2. 流程模式

### 2.1 顺序流程（Sequential）

任务按顺序依次执行，前一个任务的输出自动传递给下一个：

```python
from crewai import Crew, Process

# 顺序流程：诊断 → 修复 → 验证
crew = Crew(
    agents=[diagnostician, fixer, validator],
    tasks=[diagnosis_task, fix_task, validation_task],
    process=Process.sequential,
    verbose=True,
    memory=True,           # 启用团队记忆
    max_rpm=10,            # API 调用速率限制
    share_crew=False,      # 是否共享 Crew 上下文
)

# 执行
result = crew.kickoff(inputs={
    "namespace": "default",
    "pod_name": "nginx-abc123",
})
print(result.raw)          # 最终输出
print(result.tasks_output) # 各任务输出列表
```

### 2.2 层级流程（Hierarchical）

由 Manager Agent 自动协调任务分配：

```python
from crewai import Crew, Process

crew = Crew(
    agents=[diagnostician, fixer, validator],
    tasks=[diagnosis_task, fix_task, validation_task],
    process=Process.hierarchical,
    manager_llm="gpt-4o",  # Manager Agent 使用的 LLM
    manager_agent=None,     # 可自定义 Manager Agent
    verbose=True,
)

# Manager Agent 会自动：
# 1. 分析任务依赖关系
# 2. 决定执行顺序
# 3. 将任务分配给合适的 Agent
# 4. 处理任务间的上下文传递
result = crew.kickoff()
```

### 2.3 并行执行

独立任务可以并行执行：

```python
# 并行收集不同维度的信息
collect_logs_task = Task(
    description="收集 Pod 日志",
    agent=log_analyzer,
    async_execution=True,  # 标记为异步执行
)

collect_metrics_task = Task(
    description="收集资源指标",
    agent=metrics_analyzer,
    async_execution=True,
)

collect_events_task = Task(
    description="收集集群事件",
    agent=event_analyzer,
    async_execution=True,
)

# 汇总任务（等待所有并行任务完成）
synthesize_task = Task(
    description="汇总所有信息，分析根因",
    agent=diagnostician,
    context=[
        collect_logs_task,
        collect_metrics_task,
        collect_events_task,
    ],
)

crew = Crew(
    agents=[log_analyzer, metrics_analyzer, event_analyzer, diagnostician],
    tasks=[
        collect_logs_task,
        collect_metrics_task,
        collect_events_task,
        synthesize_task,
    ],
    process=Process.sequential,
)
```

---

## 3. 自定义工具开发

### 3.1 工具基类

```python
from crewai.tools import BaseTool
from pydantic import BaseModel, Field
from typing import Type

# 工具输入 Schema
class KubectlQueryInput(BaseModel):
    namespace: str = Field(
        default="default",
        description="Kubernetes 命名空间"
    )
    resource: str = Field(
        description="资源类型：pod/service/deployment/configmap"
    )
    name: str = Field(
        default="",
        description="资源名称，留空则列出所有"
    )

# 工具实现
class KubectlQueryTool(BaseTool):
    name: str = "kubectl_query"
    description: str = (
        "查询 Kubernetes 集群资源状态。"
        "可以查看 Pod、Service、Deployment 等资源的详细信息。"
    )
    args_schema: Type[BaseModel] = KubectlQueryInput

    def _run(
        self,
        namespace: str = "default",
        resource: str = "pod",
        name: str = "",
    ) -> str:
        import subprocess
        cmd = ["kubectl", "get", resource]
        if name:
            cmd.append(name)
        cmd.extend(["-n", namespace, "-o", "wide"])

        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30
        )
        if result.returncode != 0:
            return f"命令执行失败: {result.stderr}"
        return result.stdout
```

### 3.2 高级工具示例

```python
from crewai.tools import BaseTool
from typing import Type

class LogAnalysisInput(BaseModel):
    namespace: str = Field(description="命名空间")
    pod_name: str = Field(description="Pod 名称")
    keyword: str = Field(default="", description="过滤关键词")
    tail_lines: int = Field(default=200, description="获取最后 N 行日志")

class LogAnalysisTool(BaseTool):
    name: str = "log_analysis"
    description: str = (
        "分析 Pod 日志，支持关键词过滤和错误模式识别。"
        "返回最近的日志条目和错误统计。"
    )
    args_schema: Type[BaseModel] = LogAnalysisInput

    def _run(
        self,
        namespace: str,
        pod_name: str,
        keyword: str = "",
        tail_lines: int = 200,
    ) -> str:
        import subprocess
        import re
        from collections import Counter

        # 获取日志
        cmd = [
            "kubectl", "logs", pod_name,
            "-n", namespace,
            f"--tail={tail_lines}",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode != 0:
            return f"获取日志失败: {result.stderr}"

        lines = result.stdout.strip().split("\n")

        # 关键词过滤
        if keyword:
            lines = [l for l in lines if keyword.lower() in l.lower()]

        # 错误模式统计
        error_patterns = Counter()
        for line in lines:
            if re.search(r"\b(error|exception|fatal|panic)\b", line, re.I):
                # 提取错误类型
                match = re.search(r"(\w+Error|\w+Exception)", line)
                if match:
                    error_patterns[match.group()] += 1

        # 格式化输出
        output = f"=== 日志分析 ({namespace}/{pod_name}) ===\n"
        output += f"总行数: {len(lines)}\n"
        output += f"错误模式:\n"
        for pattern, count in error_patterns.most_common(10):
            output += f"  {pattern}: {count} 次\n"
        output += f"\n最近 20 行日志:\n"
        for line in lines[-20:]:
            output += f"  {line}\n"

        return output
```

### 3.3 异步工具

```python
import aiohttp
from crewai.tools import BaseTool

class PrometheusQueryTool(BaseTool):
    name: str = "prometheus_query"
    description: str = "执行 PromQL 查询获取集群指标"

    def _run(self, query: str, duration: str = "1h") -> str:
        import requests
        url = "http://prometheus:9090/api/v1/query_range"
        params = {
            "query": query,
            "start": f"now-{duration}",
            "end": "now",
            "step": "60s",
        }
        resp = requests.get(url, params=params, timeout=30)
        data = resp.json()
        if data["status"] != "success":
            return f"查询失败: {data.get('error', 'unknown')}"
        return json.dumps(data["data"]["result"][:5], indent=2)
```

---

## 4. 记忆与委派机制

### 4.1 短期记忆

CrewAI 内置记忆系统，跨任务保持上下文：

```python
crew = Crew(
    agents=[diagnostician, fixer, validator],
    tasks=[diagnosis_task, fix_task, validation_task],
    memory=True,          # 启用短期记忆
    embedder={            # 自定义嵌入模型
        "provider": "openai",
        "config": {
            "model": "text-embedding-3-small",
        }
    },
)
```

### 4.2 长期记忆

持久化记忆存储，跨会话保留知识：

```python
from crewai.memory import LongTermMemory, EntityMemory
from crewai.memory.storage import SQLiteStorage

# 配置持久化存储
crew = Crew(
    agents=[diagnostician, fixer],
    tasks=[diagnosis_task],
    memory=True,
    long_term_memory=LongTermMemory(
        storage=SQLiteStorage(db_path="./crew_memory.db"),
    ),
    entity_memory=EntityMemory(
        storage=SQLiteStorage(db_path="./crew_entities.db"),
    ),
)
```

### 4.3 任务委派

Agent 可以将子任务委派给其他 Agent：

```python
# 允许委派的 Agent
supervisor = Agent(
    role="运维主管",
    goal="协调团队完成集群故障排查",
    allow_delegation=True,  # 启用委派
    tools=[],
)

# Agent 可以：
# 1. 将工具调用委派给更专业的 Agent
# 2. 请求其他 Agent 的输入
# 3. 委托验证和确认任务
```

---

## 5. K8s 部署与扩展

### 5.1 Docker 化

```dockerfile
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY src/ ./src/

# 安装 kubectl
RUN curl -LO "https://dl.k8s.io/release/$(curl -Ls \
    https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    chmod +x kubectl && mv kubectl /usr/local/bin/

EXPOSE 8000
CMD ["uvicorn", "src.main:app", "--host", "0.0.0.0", "--port", "8000"]
```

### 5.2 Helm Chart

```yaml
# helm/crewai-agent/values.yaml
replicaCount: 2

image:
  repository: registry.example.com/crewai-agent
  tag: "1.0.0"

env:
  - name: OPENAI_API_KEY
    valueFrom:
      secretKeyRef:
        name: llm-secrets
        key: openai-api-key
  - name: CREW_MEMORY_DB
    value: "/data/crew_memory.db"

# 持久化存储（记忆数据库）
persistence:
  enabled: true
  storageClass: gp3
  size: 10Gi
  mountPath: /data

resources:
  requests:
    cpu: "500m"
    memory: "1Gi"
  limits:
    cpu: "2000m"
    memory: "4Gi"

serviceAccount:
  create: true
  name: crewai-agent

rbac:
  create: true
  rules:
    - apiGroups: [""]
      resources: ["pods", "services", "events", "configmaps", "secrets"]
      verbs: ["get", "list", "watch"]
    - apiGroups: ["apps"]
      resources: ["deployments", "replicasets", "statefulsets"]
      verbs: ["get", "list", "watch", "patch", "update"]
```

### 5.3 FastAPI 服务封装

```python
from fastapi import FastAPI, BackgroundTasks
from pydantic import BaseModel
from crewai import Crew, Process
import uuid

app = FastAPI(title="CrewAI K8s Agent")

class DiagnosisRequest(BaseModel):
    namespace: str = "default"
    pod_name: str = ""
    description: str = ""
    priority: str = "P2"

class TaskStatus(BaseModel):
    task_id: str
    status: str  # pending/running/completed/failed
    result: str | None = None

# 任务存储
tasks_db: dict[str, TaskStatus] = {}

@app.post("/diagnose")
async def start_diagnosis(
    req: DiagnosisRequest,
    background_tasks: BackgroundTasks,
):
    task_id = str(uuid.uuid4())
    tasks_db[task_id] = TaskStatus(task_id=task_id, status="pending")

    background_tasks.add_task(run_diagnosis, task_id, req)
    return {"task_id": task_id, "status": "pending"}

@app.get("/tasks/{task_id}")
async def get_task_status(task_id: str) -> TaskStatus:
    if task_id not in tasks_db:
        raise HTTPException(404, "Task not found")
    return tasks_db[task_id]

async def run_diagnosis(task_id: str, req: DiagnosisRequest):
    tasks_db[task_id].status = "running"
    try:
        crew = Crew(
            agents=[diagnostician, fixer, validator],
            tasks=[diagnosis_task, fix_task, validation_task],
            process=Process.sequential,
            memory=True,
        )
        result = crew.kickoff(inputs={
            "namespace": req.namespace,
            "pod_name": req.pod_name,
            "description": req.description,
        })
        tasks_db[task_id].status = "completed"
        tasks_db[task_id].result = result.raw
    except Exception as e:
        tasks_db[task_id].status = "failed"
        tasks_db[task_id].result = str(e)
```

---

## 6. 生产最佳实践

### 6.1 速率限制与成本控制

```python
crew = Crew(
    agents=agents,
    tasks=tasks,
    max_rpm=20,           # 每分钟最大请求数
    language="zh-CN",     # 输出语言
    full_output=True,     # 完整输出（含中间步骤）
)
```

### 6.2 错误处理

```python
try:
    result = crew.kickoff()
except Exception as e:
    # CrewAI 会自动重试 max_retry_limit 次
    # 超出后抛出异常
    logger.error(f"Crew 执行失败: {e}")
    # 降级到单 Agent 模式
    fallback_result = diagnostician.kickoff()
```

### 6.3 可观测性

```python
# CrewAI 集成 LangSmith
import os
os.environ["LANGCHAIN_TRACING_V2"] = "true"
os.environ["LANGCHAIN_API_KEY"] = "your-key"

# 自定义回调
from crewai.utilities.events import CrewAgentExecutionEvent

def on_agent_step(event: CrewAgentExecutionEvent):
    print(f"[{event.agent.role}] {event.type}: {event.output}")

crew = Crew(agents=agents, tasks=tasks, verbose=True)
```

---

## Related

- [[AI基础设施/03-agent-runtime/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[AI基础设施/03-agent-runtime/04-autogen-microsoft-agent|Microsoft AutoGen]]

## See Also

- [[AI基础设施/03-agent-runtime/07-agent-framework-selection-guide|Agent 框架选型决策树]]


<!-- risk-assessed -->

---
title: Microsoft AutoGen 多 Agent 框架深度指南
description: 'AutoGen ConversableAgent 架构全面解析，涵盖 GroupChat 多 Agent 对话、代码执行沙箱、嵌套对话、AutoGen Studio 及 Semantic Kernel 集成'
summary: 'AutoGen ConversableAgent 架构全面解析'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- autogen
- microsoft
- multi-agent
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
- Microsoft AutoGen 是什么
- 如何 Microsoft AutoGen
- AutoGen GroupChat 多 Agent 对话
trigger_keywords:
- autogen
- conversable-agent
- group-chat
- code-executor
- semantic-kernel
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


# Microsoft AutoGen 多 Agent 框架深度指南

## 1. AutoGen 架构概述

### 1.1 设计哲学

AutoGen 是微软开源的多 Agent 对话框架，核心理念是**通过对话实现协作**。与 LangGraph 的状态机不同，AutoGen 将 Agent 间交互建模为**会话（Conversation）**，Agent 通过消息传递完成任务。

```
┌─────────────────────────────────────────────────┐
│                AutoGen 架构                      │
│                                                  │
│  ┌──────────────────────────────────────────┐    │
│  │         ConversableAgent (基类)          │    │
│  │  ┌────────┐ ┌────────┐ ┌──────────────┐  │    │
│  │  │ System │ │ LLM    │ │ Code         │  │    │
│  │  │ Prompt │ │ Config │ │ Executor     │  │    │
│  │  └────────┘ └────────┘ └──────────────┘  │    │
│  └───────────────┬──────────────────────────┘    │
│                  │                               │
│    ┌─────────────┼─────────────┐                 │
│    ↓             ↓             ↓                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐         │
│  │Assistant │ │UserProxy │ │ GroupChat│         │
│  │Agent     │ │Agent     │ │ Manager  │         │
│  └──────────┘ └──────────┘ └──────────┘         │
└─────────────────────────────────────────────────┘
```

### 1.2 核心组件

| 组件 | 职责 | 典型用途 |
|------|------|---------|
| ConversableAgent | 所有 Agent 的基类 | 自定义 Agent |
| AssistantAgent | LLM 驱动的对话 Agent | 代码生成、推理 |
| UserProxyAgent | 代理用户输入和代码执行 | 人机交互、工具调用 |
| GroupChat | 多 Agent 群聊 | 复杂协作场景 |
| GroupChatManager | 管理群聊流程 | 自动路由消息 |

---

## 2. ConversableAgent 架构

### 2.1 基础 Agent 定义

```python
from autogen import ConversableAgent, AssistantAgent, UserProxyAgent

# LLM 配置
llm_config = {
    "model": "gpt-4o",
    "api_key": os.environ["OPENAI_API_KEY"],
    "temperature": 0,
    "cache_seed": None,  # 禁用缓存用于生产
}

# Assistant Agent（LLM 驱动）
assistant = AssistantAgent(
    name="k8s_expert",
    system_message=(
        "你是 Kubernetes 集群诊断专家。\n"
        "你有以下能力：\n"
        "1. 分析 Pod 异常状态\n"
        "2. 解读集群事件\n"
        "3. 生成修复命令\n\n"
        "在给出最终诊断结论时，用 TERMINATE 结束对话。"
    ),
    llm_config=llm_config,
)

# User Proxy Agent（代理用户和执行代码）
user_proxy = UserProxyAgent(
    name="user_proxy",
    human_input_mode="NEVER",  # 不需要人工输入
    max_consecutive_auto_reply=10,
    is_termination_msg=lambda x: x.get("content", "").rstrip().endswith("TERMINATE"),
    code_execution_config={
        "work_dir": "./workspace",
        "use_docker": "python:3.11-slim",  # Docker 沙箱执行
        "timeout": 120,
    },
)
```

### 2.2 两 Agent 对话

```python
# 最简单的对话模式：Assistant ↔ UserProxy
result = user_proxy.initiate_chat(
    assistant,
    message=(
        "default 命名空间下的 nginx-deployment 的 Pod 一直 CrashLoopBackOff，"
        "请帮我诊断问题并给出修复方案。"
    ),
    max_turns=8,
)

# 查看对话历史
for msg in result.chat_history:
    print(f"[{msg['role']}] {msg['content'][:200]}")

# 获取摘要
print(f"摘要: {result.summary}")
print(f"总 Token: {result.cost}")
```

### 2.3 自定义 ConversableAgent

```python
from autogen import ConversableAgent

class K8sDiagnosticAgent(ConversableAgent):
    """自定义 K8s 诊断 Agent。"""

    DEFAULT_SYSTEM_MESSAGE = (
        "你是 KuDig K8s 诊断专家。"
        "使用 kubectl 工具查询集群状态，分析根因。"
    )

    def __init__(self, name="k8s_diagnostician", **kwargs):
        super().__init__(
            name=name,
            system_message=kwargs.pop(
                "system_message", self.DEFAULT_SYSTEM_MESSAGE
            ),
            **kwargs,
        )
        self._register_tools()

    def _register_tools(self):
        """注册 K8s 工具。"""

        def get_pod_status(namespace: str, pod_name: str = "") -> str:
            """查询 Pod 状态。"""
            import subprocess
            cmd = ["kubectl", "get", "pods", "-n", namespace, "-o", "wide"]
            if pod_name:
                cmd = ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "yaml"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            return result.stdout

        def get_events(namespace: str) -> str:
            """获取命名空间事件。"""
            import subprocess
            result = subprocess.run(
                ["kubectl", "get", "events", "-n", namespace,
                 "--sort-by=.lastTimestamp"],
                capture_output=True, text=True, timeout=30
            )
            return result.stdout

        # 注册为函数调用
        self.register_for_llm(
            name="get_pod_status",
            description="查询指定命名空间的 Pod 状态",
        )(get_pod_status)

        self.register_for_llm(
            name="get_events",
            description="获取命名空间的事件列表",
        )(get_events)
```

---

## 3. GroupChat 多 Agent 对话

### 3.1 基础群聊

```python
from autogen import GroupChat, GroupChatManager

# 定义多个专业 Agent
diagnostician = AssistantAgent(
    name="diagnostician",
    system_message="你是诊断专家，负责分析问题根因。",
    llm_config=llm_config,
)

fixer = AssistantAgent(
    name="fixer",
    system_message="你是修复工程师，负责制定和执行修复方案。",
    llm_config=llm_config,
)

validator = AssistantAgent(
    name="validator",
    system_message="你是验证工程师，负责验证修复是否成功。",
    llm_config=llm_config,
)

# GroupChat 配置
group_chat = GroupChat(
    agents=[user_proxy, diagnostician, fixer, validator],
    messages=[],
    max_round=20,
    speaker_selection_method="auto",  # 自动选择发言者
    # speaker_selection_method="round_robin",  # 轮询
    # speaker_selection_method="random",        # 随机
    # speaker_selection_method="manual",        # 手动
    allow_repeat_speaker=False,  # 不允许连续发言
)

# GroupChatManager 管理对话
manager = GroupChatManager(
    groupchat=group_chat,
    llm_config=llm_config,
)

# 启动群聊
user_proxy.initiate_chat(
    manager,
    message="Pod nginx-abc123 出现 OOMKilled，请团队协作排查。",
)
```

### 3.2 自定义发言者选择

```python
def custom_speaker_selection(last_speaker, group_chat):
    """自定义发言者选择逻辑。"""
    messages = group_chat.messages

    if len(messages) == 0:
        return user_proxy  # 第一个发言者

    last_msg = messages[-1]["content"]

    # 诊断完成后 → 修复工程师
    if "根因" in last_msg and last_speaker == diagnostician:
        return fixer

    # 修复完成后 → 验证工程师
    if "修复完成" in last_msg and last_speaker == fixer:
        return validator

    # 验证失败 → 回到诊断
    if "验证失败" in last_msg:
        return diagnostician

    # 默认：诊断专家发言
    return diagnostician

group_chat = GroupChat(
    agents=[user_proxy, diagnostician, fixer, validator],
    messages=[],
    max_round=20,
    speaker_selection_method=custom_speaker_selection,
)
```

### 3.3 嵌套对话（Nested Chat）

Agent 可以在内部启动子对话处理复杂子任务：

```python
# 诊断 Agent 的嵌套对话：调用知识库
from autogen import AssistantAgent

knowledge_agent = AssistantAgent(
    name="knowledge_base",
    system_message="你是 K8s 知识库助手，提供文档查询。",
    llm_config=llm_config,
)

# 为诊断 Agent 注册嵌套对话
diagnostician.register_nested_chats(
    [
        {
            "recipient": knowledge_agent,
            "message": lambda recipient, messages, sender, config: (
                f"查询以下问题的相关文档: {messages[-1]['content']}"
            ),
            "summary_method": "last_msg",
            "max_turns": 2,
        }
    ],
    trigger=lambda sender: sender != knowledge_agent,  # 避免递归
)
```

---

## 4. 代码执行沙箱

### 4.1 Docker 沙箱（推荐）

```python
user_proxy = UserProxyAgent(
    name="executor",
    code_execution_config={
        "use_docker": "python:3.11-slim",  # 使用 Docker 镜像
        "work_dir": "/workspace",
        "timeout": 120,
        "last_n_messages": 3,  # 检查最近 N 条消息中的代码
    },
)

# 自定义 Docker 镜像（包含 kubectl）
docker_config = {
    "use_docker": "custom-k8s-agent:latest",
    "work_dir": "/workspace",
    "timeout": 180,
    "docker_volume": "/tmp/autogen-workspace",
    "docker_network": "autogen-network",
}
```

```dockerfile
# Dockerfile 用于代码执行沙箱
FROM python:3.11-slim

RUN apt-get update && apt-get install -y curl jq
RUN curl -LO "https://dl.k8s.io/release/$(curl -Ls \
    https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    chmod +x kubectl && mv kubectl /usr/local/bin/

WORKDIR /workspace
```

### 4.2 本地执行（不推荐用于生产）

```python
user_proxy = UserProxyAgent(
    name="executor",
    code_execution_config={
        "use_docker": False,  # 本地执行
        "work_dir": "/tmp/autogen-workspace",
        "timeout": 60,
    },
)
```

### 4.3 禁用代码执行

```python
# 仅对话模式，不执行代码
user_proxy = UserProxyAgent(
    name="user",
    code_execution_config=False,  # 禁用代码执行
    human_input_mode="ALWAYS",    # 每轮等待人工输入
)
```

---

## 5. AutoGen Studio

### 5.1 安装与启动

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装
pip install autogenstudio

# 启动 Web UI
autogenstudio ui --port 8080 --host 0.0.0.0

# Docker 启动
docker run -p 8080:8080 \
    -e OPENAI_API_KEY=$OPENAI_API_KEY \
    ghcr.io/microsoft/autogen/autogenstudio:latest
```
### 5.2 Studio 功能

AutoGen Studio 提供：
- **可视化 Agent 编辑器**：拖拽式创建和配置 Agent
- **技能管理**：定义和测试 Agent 技能（函数调用）
- **会话管理**：创建、监控和调试 Agent 对话
- **评估面板**：运行基准测试评估 Agent 性能
- **API 暴露**：通过 REST API 集成到外部系统

### 5.3 K8s 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: autogen-studio
spec:
  replicas: 1
  selector:
    matchLabels:
      app: autogen-studio
  template:
    metadata:
      labels:
        app: autogen-studio
    spec:
      containers:
        - name: studio
          image: ghcr.io/microsoft/autogen/autogenstudio:latest
          ports:
            - containerPort: 8080
          env:
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: llm-secrets
                  key: openai-api-key
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
            limits:
              cpu: "1000m"
              memory: "2Gi"
---
apiVersion: v1
kind: Service
metadata:
  name: autogen-studio
spec:
  selector:
    app: autogen-studio
  ports:
    - port: 80
      targetPort: 8080
```

---

## 6. 与 Semantic Kernel 集成

### 6.1 集成模式

AutoGen 和 Semantic Kernel 可以互补使用：

```python
import semantic_kernel as sk
from autogen import AssistantAgent

# Semantic Kernel 提供插件和函数
kernel = sk.Kernel()
kernel.add_plugin(K8sPlugin(), "k8s")

# AutoGen 提供多 Agent 对话
class SKPoweredAgent(AssistantAgent):
    """使用 Semantic Kernel 的 Agent。"""

    def __init__(self, kernel: sk.Kernel, **kwargs):
        super().__init__(**kwargs)
        self.kernel = kernel

    def generate_reply(self, messages, sender, **kwargs):
        # 使用 Semantic Kernel 执行函数
        last_msg = messages[-1]["content"]

        if "查询 Pod" in last_msg:
            # 调用 SK 插件
            result = asyncio.run(
                self.kernel.invoke(
                    plugin_name="k8s",
                    function_name="get_pod_status",
                    namespace="default",
                )
            )
            return str(result)

        # 回退到 LLM 对话
        return super().generate_reply(messages, sender, **kwargs)
```

### 6.2 SK Agent 与 AutoGen 对话

```python
from semantic_kernel.agents import ChatCompletionAgent
from autogen import GroupChat, GroupChatManager

# Semantic Kernel Agent
sk_agent = ChatCompletionAgent(
    service_id="default",
    kernel=kernel,
    name="sk_k8s_expert",
    instructions="你是 K8s 专家，使用 SK 插件查询集群。",
)

# AutoGen Agent
autogen_agent = AssistantAgent(
    name="autogen_analyst",
    system_message="你是分析专家，负责综合信息。",
    llm_config=llm_config,
)

# 通过中间层桥接
class AgentBridge:
    """在 SK Agent 和 AutoGen 之间桥接。"""

    def __init__(self, sk_agent, autogen_agent):
        self.sk_agent = sk_agent
        self.autogen_agent = autogen_agent

    async def process(self, query: str):
        # SK Agent 获取数据
        sk_result = await self.sk_agent.invoke(query)

        # AutoGen Agent 分析
        autogen_result = self.autogen_agent.generate_reply(
            [{"role": "user", "content": f"分析以下数据:\n{sk_result}"}],
            sender=None,
        )

        return autogen_result
```

---

## 7. 生产最佳实践

### 7.1 对话控制

```python
# 限制对话轮数
result = user_proxy.initiate_chat(
    assistant,
    message=query,
    max_turns=8,
    summary_method="last_msg",  # 摘要方式: last_msg/llm/all
)

# 设置终止条件
def is_termination(msg):
    content = msg.get("content", "")
    return (
        "TERMINATE" in content or
        "任务完成" in content or
        len(content) == 0
    )
```

### 7.2 错误处理

```python
from autogen import ConversableAgent

# 配置重试
llm_config_with_retry = {
    "model": "gpt-4o",
    "api_key": os.environ["OPENAI_API_KEY"],
    "temperature": 0,
    "max_retries": 3,
    "retry_wait_time": 1,
    "retry_exponential_base": 2,
    "timeout": 60,
}
```

### 7.3 成本控制

```python
# 使用小模型处理简单任务
simple_config = {"model": "gpt-4o-mini", "api_key": "..."}

# 使用大模型处理复杂任务
complex_config = {"model": "gpt-4o", "api_key": "..."}

# 按 Agent 分配模型
simple_agent = AssistantAgent(
    name="formatter",
    system_message="你是格式化助手。",
    llm_config=simple_config,  # 小模型
)

complex_agent = AssistantAgent(
    name="reasoner",
    system_message="你是推理专家。",
    llm_config=complex_config,  # 大模型
)
```

---

## Related

- [[15-AI基础设施/03-Agent运行时/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[15-AI基础设施/03-Agent运行时/06-semantic-kernel-enterprise|Semantic Kernel 企业级 Agent]]

## See Also

- [[15-AI基础设施/03-Agent运行时/07-agent-framework-selection-guide|Agent 框架选型决策树]]


<!-- risk-assessed -->

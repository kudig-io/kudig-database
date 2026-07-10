---
title: Semantic Kernel 企业级 Agent 深度指南
description: 'Semantic Kernel Kernel/Plugin/Function 三层架构全面解析，涵盖 Planner 自动规划、多语言支持、Azure OpenAI 集成及 AutoGen 互通'
summary: 'Semantic Kernel Kernel/Plugin/Function 三层架构全面解析'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- semantic-kernel
- microsoft
- enterprise
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
- Semantic Kernel 是什么
- 如何 Semantic Kernel
- Semantic Kernel Planner 自动规划
trigger_keywords:
- semantic-kernel
- kernel
- plugin
- planner
- azure-openai
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


# Semantic Kernel 企业级 Agent 深度指南

## 1. 核心架构

### 1.1 设计定位

Semantic Kernel（SK）是微软开源的 AI 编排 SDK，专为企业级应用设计。核心优势：
- **原生 .NET / C# 支持**：适合企业 .NET 技术栈
- **Azure 深度集成**：原生支持 Azure OpenAI、Azure AI Search
- **插件化架构**：标准化的 Plugin/Function 抽象
- **多语言支持**：C#、Python、Java

```
┌─────────────────────────────────────────────────┐
│              Semantic Kernel 架构                 │
│                                                  │
│  ┌──────────────────────────────────────────┐    │
│  │              Application                  │    │
│  └──────────────────┬───────────────────────┘    │
│                     │                            │
│  ┌──────────────────┴───────────────────────┐    │
│  │           Kernel (核心运行时)              │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐  │    │
│  │  │ AI       │ │ Plugin   │ │ Service  │  │    │
│  │  │ Services │ │ Manager  │ │ Selector │  │    │
│  │  └──────────┘ └──────────┘ └──────────┘  │    │
│  └──────────────────┬───────────────────────┘    │
│                     │                            │
│  ┌──────────────────┴───────────────────────┐    │
│  │              Plugins                       │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐  │    │
│  │  │ Native   │ │ OpenAPI  │ │ Memory   │  │    │
│  │  │ Functions│ │ Plugins  │ │ Plugins  │  │    │
│  │  └──────────┘ └──────────┘ └──────────┘  │    │
│  └──────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘
```

### 1.2 Kernel 初始化（C#）

```csharp
using Microsoft.SemanticKernel;
using Microsoft.SemanticKernel.Connectors.OpenAI;

// 创建 Kernel
var builder = Kernel.CreateBuilder();

// 添加 AI 服务
builder.AddAzureOpenAIChatCompletion(
    deploymentName: "gpt-4o",
    endpoint: "https://my-resource.openai.azure.com/",
    apiKey: Environment.GetEnvironmentVariable("AZURE_OPENAI_KEY")
);

// 添加插件
builder.Plugins.AddFromType<K8sPlugin>();
builder.Plugins.AddFromType<LogAnalysisPlugin>();

var kernel = builder.Build();
```

### 1.3 Kernel 初始化（Python）

```python
import semantic_kernel as sk
from semantic_kernel.connectors.ai.open_ai import AzureChatCompletion
from semantic_kernel.functions import KernelPlugin

# 创建 Kernel
kernel = sk.Kernel()

# 添加 AI 服务
kernel.add_service(
    AzureChatCompletion(
        service_id="default",
        deployment_name="gpt-4o",
        endpoint="https://my-resource.openai.azure.com/",
        api_key=os.environ["AZURE_OPENAI_KEY"],
    )
)

# 添加插件
kernel.add_plugin(K8sPlugin(), "k8s")
kernel.add_plugin(LogPlugin(), "logs")
```

---

## 2. Plugin / Function 架构

### 2.1 Native Plugin（C#）

```csharp
using Microsoft.SemanticKernel;
using System.ComponentModel;

public class K8sPlugin
{
    [KernelFunction("get_pod_status")]
    [Description("查询 Kubernetes Pod 的运行状态")]
    public async Task<string> GetPodStatus(
        [Description("命名空间")] string namespace = "default",
        [Description("Pod 名称")] string podName = "")
    {
        var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "kubectl",
                Arguments = $"get pods -n {namespace} -o wide",
                RedirectStandardOutput = true,
                UseShellExecute = false,
            }
        };
        process.Start();
        return await process.StandardOutput.ReadToEndAsync();
    }

    [KernelFunction("describe_pod")]
    [Description("获取 Pod 的详细描述信息")]
    public async Task<string> DescribePod(
        [Description("命名空间")] string @namespace,
        [Description("Pod 名称")] string podName)
    {
        var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "kubectl",
                Arguments = $"describe pod {podName} -n {@namespace}",
                RedirectStandardOutput = true,
                UseShellExecute = false,
            }
        };
        process.Start();
        return await process.StandardOutput.ReadToEndAsync();
    }
}
```

### 2.2 Native Plugin（Python）

```python
from semantic_kernel.functions import kernel_function
from semantic_kernel.kernel_pydantic import KernelBaseModel

class K8sPlugin(KernelBaseModel):
    """Kubernetes 集群管理插件。"""

    @kernel_function(
        description="查询 Kubernetes Pod 的运行状态",
        name="get_pod_status",
    )
    def get_pod_status(
        self, namespace: str = "default", pod_name: str = ""
    ) -> str:
        import subprocess
        cmd = ["kubectl", "get", "pods", "-n", namespace, "-o", "wide"]
        if pod_name:
            cmd = ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "yaml"]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        return result.stdout

    @kernel_function(
        description="获取 Pod 的详细描述信息",
        name="describe_pod",
    )
    def describe_pod(self, namespace: str, pod_name: str) -> str:
        import subprocess
        result = subprocess.run(
            ["kubectl", "describe", "pod", pod_name, "-n", namespace],
            capture_output=True, text=True, timeout=30
        )
        return result.stdout
```

### 2.3 OpenAPI Plugin

```python
# 从 OpenAPI Schema 导入插件
from semantic_kernel.functions import KernelPluginFromOpenAPI

# 加载 OpenAPI Spec
plugin = kernel.add_plugin_from_openapi(
    plugin_name="k8s_api",
    openapi_document_path="./k8s-openapi.json",
    execution_settings=sk_openapi.OpenAPIFunctionExecutionParameters(
        enable_payload_namespacing=True,
    ),
)
```

### 2.4 Memory Plugin

```python
from semantic_kernel.memory import SemanticTextMemory

# 配置记忆存储
kernel.add_plugin(
    TextMemoryPlugin(
        memory=SemanticTextMemory(
            storage=VolatileMemoryStore(),
            embeddings=AzureTextEmbedding(
                deployment_name="text-embedding-3-small",
                endpoint="https://my-resource.openai.azure.com/",
            ),
        )
    ),
    "memory",
)

# 存储知识
await kernel.memory.save_information(
    collection="k8s_docs",
    id="doc-001",
    text="Pod OOMKilled 通常是由于容器内存使用超出 limits 配置",
)

# 语义搜索
results = await kernel.memory.search(
    collection="k8s_docs",
    query="容器内存不足",
    limit=5,
)
```

---

## 3. Planner 自动规划

### 3.1 Function Calling Planner（推荐）

```python
from semantic_kernel.connectors.ai.open_ai import OpenAIPromptExecutionSettings
from semantic_kernel.planners import FunctionCallingStepwisePlanner

# 创建 Planner
planner = FunctionCallingStepwisePlanner(
    service_id="default",
    max_iterations=10,
)

# 执行规划
result = await planner.invoke(
    kernel=kernel,
    question="检查 default 命名空间下 nginx Pod 为什么一直重启，并给出修复建议",
)

print(f"最终答案: {result.final_answer}")
print(f"执行步数: {len(result.steps)}")
for step in result.steps:
    print(f"  [{step.plugin_name}.{step.function_name}] → {step.output[:100]}")
```

### 3.2 AgentChat（多 Agent 协作）

```python
from semantic_kernel.agents import (
    ChatCompletionAgent,
    AgentGroupChat,
    AgentTerminationStrategy,
)

# 定义 Agent
diagnostician = ChatCompletionAgent(
    service_id="default",
    kernel=kernel,
    name="diagnostician",
    instructions="你是 K8s 诊断专家，负责分析问题根因。",
)

fixer = ChatCompletionAgent(
    service_id="default",
    kernel=kernel,
    name="fixer",
    instructions="你是 K8s 修复工程师，负责执行修复操作。",
)

# 终止策略
class K8sTerminationStrategy(AgentTerminationStrategy):
    async def should_agent_terminate(self, agent, history):
        return "TERMINATE" in history[-1].content

# 创建 Agent 群聊
group_chat = AgentGroupChat(
    agents=[diagnostician, fixer],
    termination_strategy=K8sTerminationStrategy(),
)

# 执行对话
result = await group_chat.invoke(
    message="Pod nginx-abc123 出现 CrashLoopBackOff",
)
```

### 3.3 Stepwise Planner（逐步规划）

```python
from semantic_kernel.planners import FunctionCallingStepwisePlanner

# 逐步规划，每步都可以人工审核
planner = FunctionCallingStepwisePlanner(
    service_id="default",
    max_iterations=15,
)

# 获取规划步骤（不执行）
plan = await planner.create_plan(
    kernel=kernel,
    question="分析集群中所有 Pending 状态的 Pod 并给出调度建议",
)

# 逐步执行
for step in plan.steps:
    print(f"执行: {step.plugin_name}.{step.function_name}")
    # 可以在这里添加人工审批逻辑
    result = await step.invoke(kernel=kernel)
    print(f"结果: {result}")
```

---

## 4. 多语言支持

### 4.1 C# / .NET

```csharp
// .NET 8 + Semantic Kernel
var builder = WebApplication.CreateBuilder(args);

builder.Services.AddKernel()
    .AddAzureOpenAIChatCompletion("gpt-4o", endpoint, apiKey)
    .Plugins.AddFromType<K8sPlugin>();

var app = builder.Build();

app.MapPost("/diagnose", async (Kernel kernel, DiagnosisRequest request) =>
{
    var result = await kernel.InvokePromptAsync(
        $"诊断 {request.Namespace} 命名空间中 {request.PodName} 的异常",
        new KernelArguments
        {
            ["namespace"] = request.Namespace,
            ["pod_name"] = request.PodName,
        }
    );
    return Results.Ok(new { diagnosis = result.ToString() });
});

app.Run();
```

### 4.2 Python

```python
from fastapi import FastAPI
import semantic_kernel as sk

app = FastAPI()

@app.post("/diagnose")
async def diagnose(namespace: str, pod_name: str):
    kernel = sk.Kernel()
    kernel.add_service(AzureChatCompletion(...))
    kernel.add_plugin(K8sPlugin(), "k8s")

    result = await kernel.invoke_prompt(
        f"诊断 {namespace} 命名空间中 {pod_name} 的异常",
        namespace=namespace,
        pod_name=pod_name,
    )
    return {"diagnosis": str(result)}
```

### 4.3 Java

```java
import com.microsoft.semantickernel.Kernel;
import com.microsoft.semantickernel.aiservices.openai.chatcompletion.OpenAIChatCompletion;

var kernel = Kernel.builder()
    .withAIService(OpenAIChatCompletion.builder()
        .withModelId("gpt-4o")
        .withEndpoint(endpoint)
        .withApiKey(apiKey)
        .build())
    .withPlugin(new K8sPlugin())
    .build();

var result = kernel.invokePromptAsync(
    "诊断 default 命名空间中 nginx Pod 的异常"
).block();
```

---

## 5. Azure OpenAI 集成

### 5.1 连接配置

```python
from semantic_kernel.connectors.ai.open_ai import AzureChatCompletion

# 基础配置
kernel.add_service(
    AzureChatCompletion(
        service_id="default",
        deployment_name="gpt-4o",
        endpoint=os.environ["AZURE_OPENAI_ENDPOINT"],
        api_key=os.environ["AZURE_OPENAI_KEY"],
        api_version="2024-08-01-preview",
    )
)

# 使用 Managed Identity（推荐生产环境）
from azure.identity import DefaultAzureCredential

credential = DefaultAzureCredential()
kernel.add_service(
    AzureChatCompletion(
        service_id="default",
        deployment_name="gpt-4o",
        endpoint=os.environ["AZURE_OPENAI_ENDPOINT"],
        ad_token_provider=credential.get_token(
            "https://cognitiveservices.azure.com/.default"
        ),
    )
)
```

### 5.2 Azure AI Search 集成

```python
from semantic_kernel.connectors.memory.azure import AzureAISearchMemoryStore

# 配置 Azure AI Search
kernel.add_plugin(
    TextMemoryPlugin(
        memory=SemanticTextMemory(
            storage=AzureAISearchMemoryStore(
                endpoint=os.environ["AZURE_SEARCH_ENDPOINT"],
                api_key=os.environ["AZURE_SEARCH_KEY"],
            ),
            embeddings=AzureTextEmbedding(
                deployment_name="text-embedding-3-small",
            ),
        )
    ),
    "memory",
)
```

---

## 6. 与 AutoGen 互通

### 6.1 SK Agent 在 AutoGen 群聊中

```python
from autogen import GroupChat, GroupChatManager, AssistantAgent
from semantic_kernel.agents import ChatCompletionAgent

# SK Agent
sk_agent = ChatCompletionAgent(
    service_id="default",
    kernel=kernel,
    name="sk_expert",
    instructions="你是 K8s 专家，使用 SK 插件查询集群。",
)

# AutoGen Agent
autogen_agent = AssistantAgent(
    name="analyst",
    system_message="你是分析专家。",
    llm_config=llm_config,
)

# 桥接层
class SKAutoGenBridge:
    """将 SK Agent 包装为 AutoGen 兼容的 Agent。"""

    def __init__(self, sk_agent: ChatCompletionAgent):
        self.sk_agent = sk_agent

    async def generate_reply(self, messages):
        last_msg = messages[-1]["content"]
        result = await self.sk_agent.invoke(last_msg)
        return str(result[0].content)
```

### 6.2 统一编排

```python
# 使用 SK 作为工具层，AutoGen 作为对话层
class HybridOrchestrator:
    def __init__(self):
        self.kernel = sk.Kernel()
        self.kernel.add_plugin(K8sPlugin(), "k8s")

    async def run(self, task: str):
        # SK 执行工具调用
        tool_result = await self.kernel.invoke(
            plugin_name="k8s",
            function_name="get_pod_status",
            namespace="default",
        )

        # AutoGen 处理对话
        autogen_result = user_proxy.initiate_chat(
            assistant,
            message=f"分析以下 Pod 状态:\n{tool_result}\n\n任务: {task}",
            max_turns=5,
        )

        return autogen_result.summary
```

---

## 7. 生产最佳实践

### 7.1 依赖注入

```csharp
// C# 依赖注入
builder.Services.AddSingleton<IK8sService, K8sService>();
builder.Services.AddKernel()
    .AddAzureOpenAIChatCompletion("gpt-4o", endpoint, key)
    .Plugins.AddFromType<K8sPlugin>(sp =>
        new K8sPlugin(sp.GetRequiredService<IK8sService>()));
```

### 7.2 可观测性

```python
# OpenTelemetry 集成
from opentelemetry import trace
from semantic_kernel.functions import KernelPlugin

# SK 内置追踪
kernel = sk.Kernel()
# 自动记录所有函数调用
```

### 7.3 错误处理

```python
from semantic_kernel.exceptions import KernelException

try:
    result = await kernel.invoke(
        plugin_name="k8s",
        function_name="get_pod_status",
    )
except KernelException as e:
    logger.error(f"SK 函数调用失败: {e}")
    # 降级处理
```

### 7.4 安全配置

```yaml
# Azure RBAC
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: sk-agent-reader
rules:
  - apiGroups: [""]
    resources: ["pods", "services", "events"]
    verbs: ["get", "list", "watch"]
```

---

## Related

- [[AI基础设施/Agent运行时/04-autogen-microsoft-agent|Microsoft AutoGen]]
- [[AI基础设施/Agent运行时/07-agent-framework-selection-guide|Agent 框架选型决策树]]

## See Also

- [[AI基础设施/Agent运行时/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[AI基础设施/Agent运行时/05-dify-agent-platform|Dify Agent 平台]]


<!-- risk-assessed -->

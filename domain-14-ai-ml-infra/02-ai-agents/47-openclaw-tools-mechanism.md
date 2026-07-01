---
title: OpenClaw TOOLS.md 机制深度解析 (domain-14-ai-ml-infra)
description: 'title: OpenClaw TOOLS.md 机制深度解析'
summary: 'title: OpenClaw TOOLS.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- etcd
- prometheus
- helm
- llm
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenClaw TOOLS.md 机制深度解析 是什么
- 如何 OpenClaw TOOLS.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- TOOLS.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- etcd-basics
- logging-basics
---



title: OpenClaw TOOLS.md 机制深度解析
description: '# OpenClaw TOOLS.md 机制深度解析'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- [[Prometheus|prometheus]]
- [[Helm|helm]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- OpenClaw TOOLS.md 机制深度解析 是什么
- 如何 OpenClaw TOOLS.md 机制深度解析
trigger_keywords:
- OpenClaw
- TOOLS.md
- 机制深度解析
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

# OpenClaw TOOLS.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, TOOLS.md, 工具授权, Tools 层, 四级权限, 安全沙箱, MCP 集成, kubectl

---

## 概述

TOOLS.md 是 OpenClaw File-First 架构中定义 **Agent 被授权使用的工具集、调用参数和安全规范** 的配置文件。它告诉 Agent "能用什么"——哪些工具可用、各自的权限级别、参数规范、以及不可触碰的命令黑名单。在 Harness Engineering 中主要映射到 **Tools 层**。

TOOLS.md 与 SOUL.md 构成**双重安全检查**：SOUL.md 约束"绝不能做什么"（行为层面），TOOLS.md 约束"被允许用什么"（能力层面）。

---

## 1. 设计原理

### 1.1 四级权限模型

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

```
TOOLS.md 四级权限模型:

Level 0: 只读（默认授权，无需确认）
  │  kubectl get / describe / logs / top / events
  │  prometheus_query / loki_search
  │  view_text_file
  │
Level 1: 有限写（需用户确认后执行）
  │  kubectl apply / scale / rollout restart
  │  kubectl label / annotate
  │
Level 2: 高风险（需二次确认 + 影响评估后执行）
  │  kubectl drain（需特殊授权）
  │  helm upgrade
  │
Level 3: 禁止（绝对不可执行）
     kubectl delete / cordon / taint --effect=NoExecute
     etcdctl del / defrag
     helm uninstall  # ⚠️ 删除 release 及关联资源

原则: 默认只授权 Level 0
      Level 1+ 需在 TOOLS.md 中显式声明
```

### 1.2 最小权限原则

| 原则 | 说明 | 反面案例（Vercel 教训） |
|------|------|----------------------|
| 工具精简 | 只注册诊断必需的工具 | Vercel 注册 15 个工具，Agent 选择混乱 |
| 参数约束 | 每个工具定义明确的参数规范 | 不约束参数 → Agent 自由发挥执行危险操作 |
| 权限最低 | 默认只读，写操作逐个授权 | 全部授权写权限 → 误操作风险极高 |
| 工具组合 | 预定义常见诊断的工具链 | 无指导 → Agent 随机组合工具，效率低 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
Vercel 教训:
  Before: 15 个工具注册 → Agent 准确率 40%
  After:  2 个核心工具 → Agent 准确率 85%

K8S Agent 推荐:
  核心只读: kubectl get/describe/logs/top/events (5 个)
  监控查询: prometheus_query/loki_search (2 个)
  有限写入: kubectl apply/scale/rollout (3 个，需确认)
  总计: 10 个工具 ← 精简且覆盖 95% 诊断场景
```

### 1.3 工具使用优先级

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
诊断场景的工具调用金字塔:

         ┌──────────┐
         │ Level 4  │  写操作（需确认）
         │  apply   │  kubectl apply/scale
         ├──────────┤
         │ Level 3  │  深度监控
         │ prom/loki│  PromQL/LogQL 查询
         ├──────────┤
         │ Level 2  │  微观详情
         │ describe │  describe/logs/top
         ├──────────┤
         │ Level 1  │  宏观状态（必做）
         │   get    │  get pods/nodes/events
         └──────────┘

原则: 从底向上，先宏观后微观，先只读后写入
```

---

## 2. Harness Engineering 映射

### 2.1 映射关系

```
TOOLS.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
TOOLS.md      │      │   ●   │         │         │        │     ◐     │

● = 主要映射（Tools 层 — 工具注册与管理）
◐ = 次要映射（Constraints 层 — 权限约束）
```

### 2.2 Tools 层映射详解

| TOOLS.md 内容 | Harness Tools 实现 | 执行方式 |
|--------------|-------------------|---------|
| 授权工具清单（1） | `ToolRegistry` — 工具注册表 | Agent 初始化时加载 |
| 工具使用优先级（2） | `ToolPrioritizer` — 优先级排序 | 推荐工具选择顺序 |
| 参数规范（3） | `ParameterValidator` — 参数校验 | 工具调用前验证 |
| 安全约束（4） | `CommandBlocker` — 命令黑名单 | 调用前拦截 |
| 工具组合模板（5） | `ToolChainTemplate` — 预定义工具链 | SOP 驱动的工具序列 |
| MCP 集成（6） | `MCPClientManager` — MCP 客户端 | 远程工具注册 |

### 2.3 与 Constraints 层的交叉

```
双重安全检查流程:

用户请求 → Agent 决策调用工具
  │
  ├→ Check 1: SOUL.md 红线检查（Constraints 层）
  │    → 命令是否在绝对禁止列表中？
  │    → 是 → 直接拦截
  │
  ├→ Check 2: TOOLS.md 权限检查（Tools 层）
  │    → 工具是否在授权清单中？
  │    → 权限级别是否足够？
  │    → 参数是否符合规范？
  │
  ├→ 通过 → 执行工具
  │
  └→ 执行后: 输出脱敏（SOUL.md 信息安全红线）
```

---

## 3. K8S 运维实战案例

### 3.1 案例：工具链模板执行

```
场景: Pod Pending 诊断

TOOLS.md 定义的工具链模板:

Step 1: 确认状态（Level 1 - get）
  kubectl get pod <pod> -n <ns> -o wide

Step 2: 查看事件（Level 2 - describe）
  kubectl describe pod <pod> -n <ns> | grep -A 20 "Events:"

Step 3: 检查节点资源（Level 2 - top）
  kubectl top nodes

Step 4: 检查调度约束（Level 2 - get jsonpath）
  kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.nodeSelector}'
  kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.affinity}'

Agent 按模板顺序执行，避免遗漏关键步骤
```

### 3.2 案例：权限检查拦截

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
场景: Agent 判断需要重启 Deployment 来解决问题

Agent 决策: kubectl rollout restart deployment/nginx -n production

权限检查:
  1. SOUL.md 红线: "rollout restart" 不在绝对禁止列表 → 通过
  2. TOOLS.md 权限: kubectl rollout → Level 1（有限写） → 需用户确认

Agent 输出:
  "诊断建议执行 rollout restart 来应用最新配置。
   命令: kubectl rollout restart deployment/nginx -n production
   影响: 会触发滚动更新，约 30s 内所有 Pod 重建
   风险: 低（滚动更新不影响服务可用性）
   确认执行？[Y/N]"
```

### 3.3 案例：命令黑名单拦截

```yaml
# TOOLS.md 中定义的命令黑名单正则
command_blacklist:
  - pattern: "kubectl\\s+delete\\s+(?!pod.*--field-selector)"
    reason: "禁止直接 delete 资源"
    alternative: "使用 kubectl apply 替换配置"

  - pattern: "kubectl\\s+exec.*\\s+--\\s+(rm|dd|mkfs)"
    reason: "禁止在容器内执行破坏性命令"

  - pattern: "-A|--all-namespaces.*delete"
    reason: "禁止跨 Namespace 批量删除"

  - pattern: "kubectl\\s+drain.*--force.*--delete-emptydir-data"
    reason: "强制 drain 可能丢失数据"
    alternative: "先评估影响再手动执行"
```

---

## 4. 配置协作机制

### 4.1 TOOLS.md 与其他文件的协作

```
TOOLS.md 在配置体系中的角色:

SOUL.md ──→ TOOLS.md
  │          SOUL.md 红线 = 绝对禁止（无论 TOOLS.md 怎么配置）
  │          TOOLS.md 权限 = 在红线内的细粒度授权
  │
AGENTS.md ──→ TOOLS.md
  │           AGENTS.md 工作流定义"何时使用工具"
  │           TOOLS.md 定义"可以使用什么工具"
  │
TOOLS.md ──→ SKILL.md
  │           SKILL.md 的 SOP 引用 TOOLS.md 中的工具链模板
  │           SKILL.md 说"怎么做" → TOOLS.md 说"用什么做"
  │
TOOLS.md ──→ MCP Server
              通过 MCP 协议集成远程工具
              kubectl-mcp / prometheus-mcp / loki-mcp
```

### 4.2 工具注册与发现

```
工具注册流程:

1. 静态注册（TOOLS.md 中声明）:
   → Agent 启动时读取，注册到 Toolkit
   → 适合固定的诊断工具集

2. 动态发现（MCP 协议）:
   → 通过 MCP Server 动态发现远程工具
   → 适合多集群、多环境场景
   → 工具能力列表由 Server 端提供

3. 按需注册（SKILL.md 驱动）:
   → SKILL.md 的 SOP 中引用工具 → 按需注册
   → 避免注册不需要的工具
```

---

## 5. AgentScope 集成代码

### 5.1 ToolsManager 实现

```python
import re
from typing import Callable, Optional
from agentscope.tool import Toolkit, execute_shell_command


class ToolsManager:
    """基于 TOOLS.md 的工具管理器"""

    def __init__(self, tools_content: str, soul_content: str):
        self.tools_config = tools_content
        self.soul_content = soul_content
        self.blacklist = self._build_blacklist(tools_content)
        self.soul_redlines = self._build_soul_redlines(soul_content)
        self.toolkit = Toolkit()

    def _build_blacklist(self, content: str) -> list[re.Pattern]:
        """从 TOOLS.md 构建命令黑名单"""
        return [
            re.compile(r"kubectl\s+delete\s+(?!pod.*--field-selector)", re.I),
            re.compile(r"kubectl\s+exec.*\s+--\s+(rm|dd|mkfs)", re.I),
            re.compile(r"-A|--all-namespaces.*delete", re.I),
            re.compile(r"kubectl\s+drain.*--force", re.I),
            re.compile(r"helm\s+(uninstall|delete)", re.I),
            re.compile(r"etcdctl\s+(del|defrag)", re.I),
        ]

    def _build_soul_redlines(self, content: str) -> list[re.Pattern]:
        """从 SOUL.md 构建绝对红线"""
        return [
            re.compile(r"kubectl\s+(delete|drain|cordon|taint)", re.I),
            re.compile(r"kubectl\s+.*--force", re.I),
        ]

    def safe_execute(self, command: str) -> str:
        """安全执行: SOUL.md 红线 → TOOLS.md 权限 → 执行 → 脱敏"""

        # Check 1: SOUL.md 绝对红线
        for pattern in self.soul_redlines:
            if pattern.search(command):
                return f"⛔ SOUL.md 安全拦截: 命令匹配绝对红线 '{pattern.pattern}'"

        # Check 2: TOOLS.md 命令黑名单
        for pattern in self.blacklist:
            if pattern.search(command):
                return f"⚠️ TOOLS.md 权限拦截: 命令匹配黑名单 '{pattern.pattern}'"

        # Check 3: 权限级别检查
        permission = self._check_permission_level(command)
        if permission == "needs_confirm":
            return f"📋 写操作需确认: {command}\n请回复 Y 确认执行"

        # 执行
        result = execute_shell_command(command)

        # 输出脱敏
        return self._sanitize_output(str(result))

    def _check_permission_level(self, command: str) -> str:
        """检查命令的权限级别"""
        write_patterns = [
            r"kubectl\s+(apply|scale|rollout|label|annotate)",
        ]
        for p in write_patterns:
            if re.search(p, command, re.I):
                return "needs_confirm"
        return "allowed"

    def _sanitize_output(self, output: str) -> str:
        """输出脱敏"""
        patterns = [
            (r"(password|token|secret)\s*[:=]\s*\S+", "***[已脱敏]***"),
            (r"eyJ[A-Za-z0-9_-]+\.eyJ[A-Za-z0-9_-]+", "***[JWT已脱敏]***"),
        ]
        result = output
        for pattern, replacement in patterns:
            result = re.sub(pattern, replacement, result, flags=re.I)
        return result

    def register_tools(self):
        """注册安全工具到 AgentScope Toolkit"""
        self.toolkit.register_tool_function(self.safe_execute)
        return self.toolkit


# === 使用示例 ===
with open("openclaw-workspace/TOOLS.md") as f:
    tools_content = f.read()
with open("openclaw-workspace/SOUL.md") as f:
    soul_content = f.read()

manager = ToolsManager(tools_content, soul_content)

# 安全执行
result = manager.safe_execute("kubectl get pods -n default -o wide")
# → 正常执行并返回结果

result = manager.safe_execute("kubectl delete pod nginx -n default")
# → "⛔ SOUL.md 安全拦截: ..."

result = manager.safe_execute("kubectl apply -f deployment.yaml")
# → "📋 写操作需确认: ..."
```

---

## 6. 问题排除

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 不使用预定义工具链 | 工具链模板未注入 Prompt | 将模板写入 SKILL.md SOP 中引用 |
| 正常命令被误拦截 | 黑名单正则太宽 | 缩小正则范围，增加排除条件 |
| Agent 注册过多工具 | TOOLS.md 列出了所有可能的工具 | 只注册当前场景需要的核心工具 |
| MCP 工具连接失败 | MCP Server 未启动或网络不通 | 检查 Server 健康状态，配置超时重试 |
| 写操作未要求确认 | 权限级别检查逻辑遗漏 | 补全 write_patterns 列表 |

### 6.2 调试检查清单

```
TOOLS.md 配置验证:

□ 授权清单：是否按权限级别分类（只读/有限写/禁止）？
□ 最小权限：注册工具数量是否 ≤ 15 个？
□ 参数规范：每个工具是否定义了参数模板？
□ 安全约束：命令黑名单是否覆盖 delete/drain/cordon/taint？
□ 工具组合：是否提供了常见问题的工具链模板？
□ MCP 配置：远程工具是否配置了超时和重试？
□ 与 SOUL.md 一致：TOOLS.md 权限是否在 SOUL.md 红线范围内？
```

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | TOOLS.md 在 7 文件体系中的定位 |
| [32 - Harness 工具工程](./32-agent-harness-tool-engineering.md) | Schema 标准、工具注册发现、安全沙箱 |
| [openclaw-workspace/TOOLS.md](./openclaw-workspace/TOOLS.md) | K8S 运维工具授权注册表完整配置 |
| [44 - SOUL.md 机制解析](./44-openclaw-soul-mechanism.md) | SOUL.md 红线与 TOOLS.md 权限的双重检查 |
| [48 - SKILL.md 机制解析](./48-openclaw-skill-mechanism.md) | SKILL.md SOP 引用 TOOLS.md 工具链模板 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容，深度解析 OpenClaw TOOLS.md 的设计机制与工程实现。*

---

## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 45-openclaw-user-mechanism
- 46-openclaw-agents-mechanism
- 48-openclaw-skill-mechanism
- 49-openclaw-memory-mechanism

```
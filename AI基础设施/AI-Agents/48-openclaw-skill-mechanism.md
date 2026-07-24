---
title: OpenClaw SKILL.md 机制深度解析 (AI基础设施)
description: 'title: OpenClaw SKILL.md 机制深度解析'
summary: 'title: OpenClaw SKILL.md 机制深度解析'
category: general
tags:
- ai
- ai-agent
- daily-ops
- etcd
- gpu
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
- OpenClaw SKILL.md 机制深度解析 是什么
- 如何 OpenClaw SKILL.md 机制深度解析
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- OpenClaw
- SKILL.md
- 机制深度解析
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: OpenClaw [[SKILL|SKILL]].md 机制深度解析
description: '# OpenClaw SKILL.md 机制深度解析'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- gpu
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- OpenClaw SKILL.md 机制深度解析 是什么
- 如何 OpenClaw SKILL.md 机制深度解析
trigger_keywords:
- OpenClaw
- SKILL.md
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

# OpenClaw SKILL.md 机制深度解析

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: OpenClaw, SKILL.md, 领域知识, SOP, Context 层, 渐进式披露, Agent Skill, kudig-database

---

<!-- chunk: 概述 -->## 概述

SKILL.md 是 OpenClaw File-First 架构中定义 **Agent 领域知识和标准操作流程（SOP）** 的配置文件。它告诉 Agent "会什么"——覆盖哪些故障域、每个问题类型的标准诊断步骤、决策树和知识库关联。在 Harness Engineering 中映射到 **Context 层（知识上下文）+ Loop 层（SOP 驱动执行）**。

SKILL.md 与 Anthropic Agent Skill 规范完全兼容，是连接"知识库"与"Agent 执行能力"的桥梁。

---

<!-- chunk: 1. 设计原理 -->## 1. 设计原理

## 1.1 渐进式披露（Progressive Disclosure）

```
Anthropic Agent Skill 规范的核心原则:

Level 1: 元数据（~100 tokens）
  │  name: k8s-operations-skill
  │  description: "Kubernetes 运维诊断全栈技能库"
  │  Agent 通过元数据判断"这个 Skill 是否与当前任务相关"
  │
Level 2: 指令（< 5000 tokens）
  │  诊断流程框架、输出格式模板
  │  Agent 需要时读取完整指令
  │
Level 3: 资源（按需加载）
     具体的 SOP 步骤、命令序列、故障树
     Agent 仅在执行具体诊断时加载对应章节

效果:
  传统方式: 每次都注入全部 SKILL.md（~5000 tokens）
  渐进式:   按需加载，平均只消耗 ~1500 tokens/次
  Token 节省: 70%
```

## 1.2 三种知识结构化范式

| 范式 | 说明 | 适用场景 | SKILL.md 对应 |
|------|------|---------|--------------|
| **SOP 范式** | Step-by-Step 标准操作流程 | 已知问题类型的确定性诊断 | 第 2-6 章的诊断流程 |
| **决策树范式** | If-Then 分支判断 | 症状→根因的推导 | 每个问题类型的分支逻辑 |
| **知识图谱范式** | 实体-关系网络 | 复杂关联分析 | 第 8 章知识库关联表 |

```
# 🟢 低风险：只读/信息收集，通常无副作用
三种范式在 SKILL.md 中的协作:

用户描述症状
  │
  ▼ 决策树范式
症状匹配 → 问题类型识别
  │  "Pending" → Pod 调度问题
  │  "CrashLoop" → Pod 运行异常
  │
  ▼ SOP 范式
执行标准诊断流程
  │  Step 1: kubectl get pod ...
  │  Step 2: kubectl describe ...
  │  Step 3: kubectl logs ...
  │
  ▼ 知识图谱范式
关联 kudig-database 知识库
  │  故障诊断/05-pod-pending-diagnosis.md
  │  故障诊断/FTA故障树/故障树模型
  │
  ▼ 输出结论
```
## 1.3 TOOLS.md vs SKILL.md 的边界

```
# 🟢 低风险：只读/信息收集，通常无副作用
清晰的职责分离:

TOOLS.md = "能力"（Agent 能做什么）
  定义: 工具清单、参数规范、权限级别
  回答: "可以用哪些工具？怎么安全地调用？"
  类比: 工具箱里有什么工具

SKILL.md = "知识"（Agent 怎么做）
  定义: SOP 流程、决策树、领域知识
  回答: "遇到 X 问题时，按什么步骤诊断？"
  类比: 使用工具的操作手册

示例:
  TOOLS.md: "kubectl describe pod — 查看资源详细信息，权限级别: 只读"
  SKILL.md: "Pod Pending 诊断 Step 2 → 执行 kubectl describe pod 查看 Events，
             关注 FailedScheduling/Insufficient 等关键字"
```
---

<!-- chunk: 2. Harness Engineering 映射 -->## 2. Harness Engineering 映射

## 2.1 映射关系

```
SKILL.md × Harness 六层映射:

               │ Loop │ Tools │ Context │ Persist │ Verify │ Constrain │
──────────────┼──────┼───────┼─────────┼─────────┼────────┼───────────│
SKILL.md      │  ◐   │       │    ●    │         │        │           │

● = 主要映射（Context 层 — 知识上下文）
◐ = 次要映射（Loop 层 — SOP 驱动的执行逻辑）
```

## 2.2 Context 层映射

| SKILL.md 内容 | Harness Context 实现 | 注入方式 |
|--------------|---------------------|---------|
| 技能覆盖范围（1） | `SkillRegistry` — 技能元数据 | 始终加载（~100 tokens） |
| 诊断 SOP（2-6） | `SOPProvider` — 按需提供 SOP | 匹配问题类型后加载 |
| 知识库关联（8） | `KnowledgeLinker` — 关联外部文档 | Agent 需要深度参考时加载 |
| 输出格式模板（7） | `OutputTemplate` — 格式化模板 | 输出阶段注入 |

## 2.3 Loop 层映射

```
# 🟢 低风险：只读/信息收集，通常无副作用
SOP 驱动的执行逻辑:

SKILL.md 的 SOP 直接影响 Agent 在 DIAGNOSE 阶段的行为:

Agent 识别问题类型 → 从 SKILL.md 加载对应 SOP → 按步骤执行

示例: Pod Pending
  SKILL.md SOP:
    1. kubectl get pod <pod> -n <ns> -o wide
    2. kubectl describe pod <pod> -n <ns>（关注 Events）
    3. 根据 Events 关键词分支:
       - "Insufficient cpu" → 检查节点资源
       - "node(s) had taint" → 检查 Tolerations
       - "no nodes available" → 检查节点状态

  Agent 将 SOP 转化为 Loop 中的执行步骤
```
---

<!-- chunk: 3. K8S 运维实战案例 -->## 3. K8S 运维实战案例

## 3.1 案例：SOP 驱动的 Pod Pending 诊断

```
# 🟢 低风险：只读/信息收集，通常无副作用
用户输入: "Pod nginx-xxx 一直 Pending"

SKILL.md 匹配: Pod 故障域 → Pending（调度失败）

按 SOP 执行:

Step 1: 确认状态
  > kubectl get pod nginx-xxx -n production -o wide
  结果: STATUS=Pending, NODE=<none>

Step 2: 查看事件
  > kubectl describe pod nginx-xxx -n production
  Events:
    Warning  FailedScheduling  0/40 nodes are available:
    20 Insufficient cpu, 20 node(s) had taint {dedicated: gpu}

Step 3: 分支判断（SKILL.md 决策树）
  关键词 "Insufficient cpu" → 检查节点资源
  关键词 "taint" → 检查 Tolerations

Step 4: 深入检查
  > kubectl top nodes | sort -k 3 -rn | head -5
  > kubectl get pod nginx-xxx -o jsonpath='{.spec.tolerations}'

Step 5: 输出诊断结论
  根因: 20 个 worker 节点 CPU 已满，另 20 个 GPU 节点有 taint 不匹配
  修复: 扩容 worker 节点 或 调整 Pod 的 resource requests
```
## 3.2 案例：kudig-database 知识注入

```
SKILL.md 知识库关联的实际使用:

场景: Agent 完成诊断后，需要提供深度参考

SKILL.md 第 8 章定义:
  Pod 问题 → 故障诊断/05-pod-pending-diagnosis.md
  Node 问题 → 故障诊断/06-node-notready-diagnosis.md
  故障树 → 故障诊断/FTA故障树/ 完整故障树分析模型

Agent 输出:
  "<!-- chunk: 诊断结果 -->## 诊断结果
   根因: 节点 CPU 不足
   ...
   <!-- chunk: 深度参考 -->## 深度参考
   - [Pod Pending 完整诊断指南](故障诊断/05-pod-pending-diagnosis.md)
   - [故障树: Pod 调度失败](故障诊断/FTA故障树/pod-scheduling-failure.md)
   这些文档包含更详细的排查步骤和历史案例。"
```

## 3.3 案例：多 Skill 按域拆分

```
推荐的 Skill 拆分方式:

反模式: 一个超大 SKILL.md 包含所有知识
  → Token 爆炸（单文件 10000+ tokens）
  → Agent 注意力分散

正确做法: 按故障域拆分为多个 Skill

skills/
├── k8s-pod-diagnosis/
│   └── SKILL.md        # Pod 故障域 SOP
├── k8s-node-diagnosis/
│   └── SKILL.md        # Node 故障域 SOP
├── k8s-network-diagnosis/
│   └── SKILL.md        # Network 故障域 SOP
├── k8s-storage-diagnosis/
│   └── SKILL.md        # Storage 故障域 SOP
└── k8s-performance/
    └── SKILL.md        # Performance 故障域 SOP

每个 Skill 平均 ~1000 tokens
Agent 按需加载匹配的 Skill
实际 Token 消耗降低 80%
```

---

<!-- chunk: 4. 配置协作机制 -->## 4. 配置协作机制

## 4.1 SKILL.md 与其他文件的协作

```
SKILL.md 在配置体系中的知识角色:

AGENTS.md ──→ SKILL.md
  │           AGENTS.md Phase 2 根因分析时
  │           加载 SKILL.md 对应故障域的 SOP
  │
TOOLS.md ──→ SKILL.md
  │           SKILL.md SOP 中的命令
  │           必须是 TOOLS.md 授权的工具
  │
SKILL.md ──→ kudig-database
  │           SKILL.md 第 8 章关联表
  │           指向 domain-12、topic-fta 等知识域
  │
SKILL.md ──→ MEMORY.md
              成功诊断的路径记录到 MEMORY.md
              丰富 SKILL.md 的经验模式
```

## 4.2 知识更新流程

```
SKILL.md 知识更新闭环:

1. 发现新故障模式
   → 诊断过程中发现 SKILL.md 未覆盖的场景

2. 记录到 MEMORY.md
   → 先记录为经验模式（中等置信度）

3. 模式验证
   → 同一模式出现 3 次以上

4. 提炼到 SKILL.md
   → 新增 SOP 或更新决策树

5. 同步 kudig-database
   → 更新对应的 domain 文档

6. CI 验证
   → Harness 回归测试确认新 SOP 有效
```

---

<!-- chunk: 5. AgentScope 集成代码 -->## 5. AgentScope 集成代码

## 5.1 SkillLoader 实现

```python
import os
import re
from typing import Optional


class SkillLoader:
    """基于 SKILL.md 的技能加载器，支持渐进式披露"""

    def __init__(self, workspace_path: str):
        self.workspace_path = workspace_path
        self.main_skill = self._load_main_skill()
        self.sub_skills = self._discover_sub_skills()

    def _load_main_skill(self) -> dict:
        """加载主 SKILL.md 元数据"""
        path = os.path.join(self.workspace_path, "SKILL.md")
        with open(path) as f:
            content = f.read()
        return {
            "full_content": content,
            "metadata": self._extract_metadata(content),
            "skill_map": self._extract_skill_map(content),
        }

    def _discover_sub_skills(self) -> dict:
        """发现 skills/ 目录下的子 Skill"""
        skills_dir = os.path.join(self.workspace_path, "skills")
        sub_skills = {}
        if os.path.exists(skills_dir):
            for name in os.listdir(skills_dir):
                skill_path = os.path.join(skills_dir, name, "SKILL.md")
                if os.path.exists(skill_path):
                    with open(skill_path) as f:
                        sub_skills[name] = f.read()
        return sub_skills

    def _extract_metadata(self, content: str) -> str:
        """提取 Level 1 元数据（~100 tokens）"""
        lines = content.split("\n")
        metadata = []
        for line in lines[:20]:
            if line.startswith("name:") or line.startswith("description:"):
                metadata.append(line)
            if line.startswith("# "):
                metadata.append(line)
        return "\n".join(metadata)

    def _extract_skill_map(self, content: str) -> dict:
        """提取技能覆盖范围的映射表"""
        return {
            "pod": ["Pending", "CrashLoopBackOff", "OOMKilled", "ImagePullBackOff"],
            "node": ["NotReady", "MemoryPressure", "DiskPressure"],
            "network": ["Service不通", "DNS失败", "Pod间通信"],
            "storage": ["PVC Pending", "挂载失败", "CSI异常"],
            "performance": ["API Server延迟", "etcd延迟", "调度延迟"],
        }

    def get_relevant_sop(self, fault_type: str) -> str:
        """根据问题类型加载对应的 SOP（Level 2-3 渐进披露）"""
        skill_map = self.main_skill["skill_map"]

        # 匹配故障域
        for domain, keywords in skill_map.items():
            for kw in keywords:
                if kw.lower() in fault_type.lower():
                    return self._extract_sop_section(domain, kw)

        return "未找到匹配的 SOP，请提供更具体的问题描述。"

    def _extract_sop_section(self, domain: str, keyword: str) -> str:
        """从 SKILL.md 提取特定问题类型的 SOP 章节"""
        content = self.main_skill["full_content"]
        # 简化提取逻辑：匹配章节标题
        sections = content.split("\n<!-- chunk: ") -->## ")
        for section in sections:
            if keyword.lower() in section.lower():
                return f"<!-- chunk: {section}" -->## {section}"
        # 尝试从子 Skill 加载
        sub_skill_name = f"k8s-{domain}-diagnosis"
        if sub_skill_name in self.sub_skills:
            return self.sub_skills[sub_skill_name]
        return f"SOP: {domain}/{keyword} — 请参考 SKILL.md 完整内容"

    def get_knowledge_references(self, domain: str) -> list[str]:
        """获取知识库关联文档"""
        references = {
            "pod": [
                "故障诊断/05-pod-pending-diagnosis.md",
                "故障诊断/08-pod-comprehensive-troubleshooting.md",
            ],
            "node": [
                "故障诊断/06-node-notready-diagnosis.md",
                "故障诊断/09-node-comprehensive-troubleshooting.md",
            ],
            "network": [
                "故障诊断/25-network-connectivity-troubleshooting.md",
                "故障诊断/26-dns-troubleshooting.md",
            ],
            "storage": [
                "故障诊断/14-pvc-storage-troubleshooting.md",
            ],
            "performance": [
                "故障诊断/33-performance-bottleneck-troubleshooting.md",
            ],
        }
        return references.get(domain, [])


# === 使用示例 ===
loader = SkillLoader("AI基础设施/02-ai-agents/openclaw-workspace")

# 渐进式披露: 先获取元数据
metadata = loader.main_skill["metadata"]
# → "name: k8s-operations-skill\ndescription: Kubernetes 运维诊断全栈技能库"

# 按需加载 SOP
sop = loader.get_relevant_sop("Pod Pending")
# → 返回 Pod Pending 的完整 SOP 步骤

# 获取深度参考
refs = loader.get_knowledge_references("pod")
# → ["故障诊断/05-...", "故障诊断/08-..."]
```

---

<!-- chunk: 6. 问题排除 -->## 6. 问题排除

## 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Agent 不按 SOP 执行 | SKILL.md 未在 Prompt 中注入 | 在 AGENTS.md Phase 2 明确引用 SKILL.md |
| Token 消耗过高 | 每次全量注入 SKILL.md | 改用渐进式披露，按需加载章节 |
| SOP 步骤与工具不一致 | SKILL.md 引用了 TOOLS.md 未授权的工具 | 同步更新两者，保持一致 |
| Agent 无法匹配问题类型 | SKILL.md 覆盖范围的关键词不全 | 扩充技能覆盖范围的关键词列表 |
| 知识库链接失效 | kudig-database 文档路径变更 | 定期检查并更新第 8 章关联表 |

## 6.2 调试检查清单

```
SKILL.md 配置验证:

□ 元数据：是否有 name 和 description（符合 Anthropic 规范）？
□ 覆盖范围：是否列出了所有支持的故障域和类型？
□ SOP 完整性：每个问题类型是否有完整的诊断步骤？
□ 决策树：关键分支点是否有明确的 If-Then 逻辑？
□ 工具一致：SOP 中的命令是否都在 TOOLS.md 授权范围内？
□ 知识关联：是否链接到 kudig-database 对应文档？
□ 输出模板：是否定义了统一的输出格式？
□ Token 效率：全文 < 5000 tokens？可否按域拆分？
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [43 - OpenClaw File-First 架构集成指南](./43-openclaw-framework-integration.md) | SKILL.md 在 7 文件体系中的定位 |
| [33 - Harness 上下文与记忆工程](./33-agent-harness-context-memory.md) | 四层上下文模型中的知识上下文 |
| [openclaw-workspace/SKILL.md](./openclaw-workspace/SKILL.md) | K8S 运维诊断技能库完整配置 |
| [47 - TOOLS.md 机制解析](./47-openclaw-tools-mechanism.md) | SKILL.md SOP 与 TOOLS.md 工具的对应关系 |
| [49 - MEMORY.md 机制解析](./49-openclaw-memory-mechanism.md) | 诊断经验从 SKILL.md 到 MEMORY.md 的流转 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容，深度解析 OpenClaw SKILL.md 的设计机制与工程实现。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents KUDIG Database — Global MOC
- [[AI基础设施/AI-Agents/README.md|AI Agent 工程专题]]
- [[AI基础设施/AI-Agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[AI基础设施/AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[AI基础设施/AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[AI基础设施/AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[AI基础设施/AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[AI基础设施/AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[AI基础设施/AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[AI基础设施/AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[AI基础设施/AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[AI基础设施/AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 46-openclaw-agents-mechanism
- 47-openclaw-tools-mechanism
- 49-openclaw-memory-mechanism
- 50-openclaw-identity-mechanism


<!-- risk-assessed -->

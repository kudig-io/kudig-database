---
title: 企业级实战案例 (AI基础设施)
description: 'description: ''**文档类型**: 实战案例专题 | **最后更新**: 2026-03 | **关键词**: K8s 运维
  Agent, AIOps,'
summary: 'description: ''**文档类型**: 实战案例专题 | **最后更新**: 2026-03 | **关键词**: K8s 运维 Agent,
  AIOps,'
category: general
tags:
- ai
- ai-agent
- case-study
- etcd
- apiserver
- prometheus
- redis
- networkpolicy
- operator
- llm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 企业级实战案例 是什么
- 如何 企业级实战案例
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 企业级实战案例
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 企业级实战案例
description: '**文档类型**: 实战案例专题 | **最后更新**: 2026-03 | **关键词**: K8s 运维 Agent, AIOps,
  智能客服, 代码审查 Agent, 企业落地, ROI, 生产指标, 最佳实践, Agent 案例'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- apiserver
- [[Prometheus|prometheus]]
- redis
- [[NetworkPolicy|networkpolicy]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 企业级实战案例 是什么
- 如何 企业级实战案例
trigger_keywords:
- 企业级实战案例
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

# 企业级实战案例

> **文档类型**: 实战案例专题 | **最后更新**: 2026-03 | **关键词**: K8s 运维 Agent, AIOps, 智能客服, 代码审查 Agent, 企业落地, ROI, 生产指标, 最佳实践, Agent 案例

---

<!-- chunk: 概述 -->## 概述

本文收录三个经过生产验证的企业级 AI Agent 案例：**K8s 运维 AIOps Agent**（基于 kudig-database 知识库）、**金融行业智能客服 Agent**、**DevOps 代码审查 Agent**。每个案例均包含完整的架构设计、技术选型决策、关键指标数据和踩坑经验，可作为企业 Agent 项目立项和实施的参考基准。

---

<!-- chunk: 案例一：K8s 运维 AIOps Agent -->## 案例一：K8s 运维 AIOps Agent

## 1.1 项目背景

**企业背景**：某大型互联网公司，K8s 集群规模 500+ 节点，日均问题工单 200+，SRE 团队 15 人。

**核心痛点**：
- 问题平均响应时间（MTTR）高达 45 分钟
- 80% 的问题工单是重复性问题（Pod Pending/CrashLoop/OOM）
- 夜间值班疲惫，95% 的告警是"假警报"或可自动处理的问题
- 新人上手周期长（3-6个月才能独立处理生产问题）

**项目目标**：
- MTTR 降低 60%（45min → 18min）
- 自动化处理率 40%（无需人工干预的问题）
- 新人上手周期缩短至 1 个月

## 1.2 架构设计

```
┌────────────────────────────────────────────────────────┐
│                  告警触发层                               │
│  Prometheus AlertManager → 问题工单系统（Jira）           │
└────────────────────────┬───────────────────────────────┘
                         │ 新工单触发
┌────────────────────────▼───────────────────────────────┐
│               AIOps Orchestrator Agent                  │
│  - 解析告警内容，分类问题类型                              │
│  - 评估问题级别（Critical/High/Medium）                   │
│  - 分发给专业 Worker Agent                               │
│  模型: GPT-4o（Orchestrator）                            │
└──────┬──────────────┬──────────────┬───────────────────┘
       ▼              ▼              ▼
┌─────────┐    ┌──────────┐   ┌──────────┐
│ 诊断    │    │  修复    │   │  通知    │
│ Worker  │    │  Worker  │   │  Worker  │
│(只读)   │    │(需审批)  │   │(Slack)   │
│gpt-4o-  │    │GPT-4o    │   │规则引擎  │
│mini     │    │          │   │          │
└─────────┘    └──────────┘   └──────────┘
       │              │
       ▼              ▼
┌─────────────────────────────┐
│       知识库（RAG）           │
│  kudig-database (Qdrant)    │
│  + 历史工单数据库             │
└─────────────────────────────┘
```

## 1.3 技术选型决策

| 组件 | 选择 | 决策理由 |
|------|------|---------|
| **编排框架** | LangGraph | 复杂工作流状态管理，支持条件路由和人工门禁 |
| **主模型** | GPT-4o | 工具调用可靠性最高，诊断推理质量优 |
| **Worker 模型** | GPT-4o-mini | 高频执行任务成本优先，能力足够 |
| **向量库** | Qdrant | 生产级性能，支持元数据过滤（按故障域过滤） |
| **知识库** | kudig-database | 覆盖 39 个 K8s 知识域，FTA 故障树结构天然适配 Agent 推理 |
| **可观测性** | Langfuse（自托管） | 合规要求不能发送数据到外部 |

## 1.4 核心实现：诊断 Agent

```python
from langgraph.graph import StateGraph, END
from langchain_openai import ChatOpenAI
from typing import TypedDict, Annotated
import operator

class DiagnosisState(TypedDict):
    alert: dict                        # 原始告警数据
    problem_type: str                  # 分类后的问题类型
    cluster_data: dict                 # 收集的集群状态
    knowledge_context: str             # 从 kudig-database 检索的知识
    root_cause: str                    # 根因分析
    fix_plan: list[str]               # 修复步骤
    risk_level: str                    # 风险评估
    approved: bool                     # 人工审批结果
    executed_steps: Annotated[list, operator.add]  # 已执行的步骤

def triage_node(state: DiagnosisState) -> DiagnosisState:
    """分诊：识别问题类型，制定诊断策略"""
    alert = state["alert"]
    
    triage_result = gpt4o.invoke(f"""
    分析以下 K8s 告警，输出 JSON：
    告警: {alert}
    
    输出: {{"problem_type": "network|storage|scheduling|application|control-plane",
             "severity": "critical|high|medium|low",
             "affected_resources": ["..."],
             "initial_hypothesis": "..."}}
    """)
    
    data = json.loads(triage_result.content)
    return {
        "problem_type": data["problem_type"],
    }

def data_collection_node(state: DiagnosisState) -> DiagnosisState:
    """并行收集相关集群数据"""
    
    # 根据问题类型选择诊断工具
    tools_by_type = {
        "network": [describe_svc, get_endpoints, get_networkpolicy, test_dns],
        "storage": [describe_pvc, get_pv, get_storageclass, get_csi_driver],
        "scheduling": [describe_pod, get_nodes, get_node_taints, get_pod_events],
        "application": [get_pod_logs, describe_pod, get_resource_metrics],
        "control-plane": [get_apiserver_logs, get_etcd_metrics, get_controller_logs],
    }
    
    tools = tools_by_type.get(state["problem_type"], [describe_pod, get_pod_events])
    
    # 并行执行
    import asyncio
    raw_data = asyncio.run(execute_tools_parallel(tools, state["alert"]))
    
    return {"cluster_data": raw_data}

def knowledge_retrieval_node(state: DiagnosisState) -> DiagnosisState:
    """从 kudig-database 检索相关诊断知识"""
    
    # 基于问题类型精确检索
    domain_filter = {
        "network": "故障诊断",
        "scheduling": "故障诊断",
        # ...
    }.get(state["problem_type"])
    
    query = f"""
    {state['problem_type']} 问题诊断：
    告警：{state['alert']['summary']}
    已观察到：{summarize_cluster_data(state['cluster_data'])}
    """
    
    docs = rag_retriever.get_relevant_documents(
        query,
        filter={"domain": domain_filter} if domain_filter else None,
    )
    
    # 同时检索相关历史成功案例
    similar_cases = episodic_memory.search_relevant_episodes(
        query, problem_type=state["problem_type"], limit=3
    )
    
    context = "\n\n".join([d.page_content for d in docs[:5]])
    if similar_cases:
        context += "\n\n[历史相似案例]\n" + "\n".join([
            f"- {case.summary}（解决方案: {case.lessons_learned}）"
            for case in similar_cases
        ])
    
    return {"knowledge_context": context}

def diagnosis_node(state: DiagnosisState) -> DiagnosisState:
    """综合诊断：输出根因分析和修复方案"""
    
    diagnosis = gpt4o.invoke(f"""
    【K8s 故障诊断报告】
    
    告警信息: {state['alert']}
    集群状态: {state['cluster_data']}
    相关知识库内容: {state['knowledge_context']}
    
    请输出：
    1. 根因分析（基于实际数据，不能猜测）
    2. 置信度（0-100%）
    3. 修复步骤（按顺序，每步标注风险等级）
    4. 整体风险评估（High/Medium/Low）
    5. 如果置信度 < 70%，说明还需要什么信息
    """)
    
    parsed = parse_diagnosis(diagnosis.content)
    return {
        "root_cause": parsed["root_cause"],
        "fix_plan": parsed["fix_steps"],
        "risk_level": parsed["risk_level"],
    }

# 构建诊断工作流图
def build_diagnosis_graph():
    workflow = StateGraph(DiagnosisState)
    
    workflow.add_node("triage", triage_node)
    workflow.add_node("collect_data", data_collection_node)
    workflow.add_node("retrieve_knowledge", knowledge_retrieval_node)
    workflow.add_node("diagnose", diagnosis_node)
    workflow.add_node("approve", human_approval_node)
    workflow.add_node("execute", execute_fix_node)
    workflow.add_node("notify", notify_slack_node)
    
    workflow.set_entry_point("triage")
    
    # 并行执行数据收集和知识检索
    workflow.add_edge("triage", "collect_data")
    workflow.add_edge("triage", "retrieve_knowledge")
    
    # 两者完成后进行诊断
    workflow.add_edge(["collect_data", "retrieve_knowledge"], "diagnose")
    
    # 根据风险级别决定是否需要审批
    workflow.add_conditional_edges(
        "diagnose",
        lambda s: "approve" if s["risk_level"] in ["High", "Medium"] else "execute",
        {"approve": "approve", "execute": "execute"}
    )
    
    workflow.add_conditional_edges(
        "approve",
        lambda s: "execute" if s["approved"] else "notify",
        {"execute": "execute", "notify": "notify"}
    )
    
    workflow.add_edge("execute", "notify")
    workflow.add_edge("notify", END)
    
    return workflow.compile()
```

## 1.5 生产数据与效果

**运行 6 个月后的关键指标**：

| 指标 | 上线前 | 上线后 | 改善 |
|------|-------|-------|------|
| 平均问题响应时间（MTTR） | 45 min | 16 min | **↓ 64%** |
| 自动处理率（无需人工） | 0% | 43% | **↑ 43%** |
| 夜间告警人工处理量 | 100% | 31% | **↓ 69%** |
| 新人独立处理问题耗时 | 6 周 | 2 周 | **↓ 67%** |
| Agent 诊断准确率 | - | 91% | - |
| 每次诊断成本 | - | $0.08 | - |
| 月度 LLM 成本 | - | $2,400 | ROI 正向 |

**问题类型自动化率分布**：

| 问题类型 | 自动化率 | 说明 |
|---------|---------|------|
| Pod Pending（资源不足） | 72% | 可自动扩容或调整请求 |
| CrashLoopBackOff | 61% | 多数为配置问题，可自动修复 |
| OOMKilled | 48% | 自动调整 memory limits |
| 网络不通 | 35% | 复杂情况多，部分需人工 |
| 控制平面问题 | 12% | 高风险，坚持人工审批 |

## 1.6 踩坑经验

```
坑1: RAG 检索精度不够
  现象: Agent 引用了不相关的知识库内容，导致错误诊断
  根因: 向量检索没有按故障域过滤，混入了不相关的文档
  解决: 在 Qdrant 中为每个文档标记 domain 元数据，
       按问题类型强制过滤检索域

坑2: Orchestrator 过度工具调用
  现象: 诊断一个简单的资源不足问题，调用了 12 次工具
  根因: 没有"信息充足时停止"的终止条件，Agent 一直在收集更多数据
  解决: 在每次工具调用后让 Agent 评估"信息是否足够"，是则停止

坑3: 夜间误触发高风险修复
  现象: 深夜 Agent 触发了 Deployment 扩容，超出了成本预算
  根因: 审批流程只有同步等待，夜间无人审批时 Agent 等待超时后自动执行
  解决: 增加"夜间冻结期"规则：23:00-8:00 的高风险操作一律等待到上班审批

坑4: 工单风暴导致 Agent 积压
  现象: 某次网络问题触发 500+ 告警，Agent 任务队列积压
  根因: 没有对同源告警做去重和聚合
  解决: 增加告警聚合层（AlertManager 聚合 + 业务逻辑去重）
        相同 namespace 的同类型告警合并为一个 Agent 任务
```

---

<!-- chunk: 案例二：金融行业智能客服 Agent -->## 案例二：金融行业智能客服 Agent

## 2.1 项目背景

**企业背景**：某股份制银行，日均客服咨询量 50,000+，人工坐席 200 人。

**核心痛点**：
- 客服成本高，人均处理能力有限（每人每天 200-250 通）
- 产品知识更新频繁（每月调整），培训滞后
- 夜间和节假日服务能力不足
- 合规风险：客服人员回答口径不一致，存在监管风险

## 2.2 架构（重点：合规与安全）

```
用户输入 → 安全过滤层（PII 脱敏 + 注入检测）
         → 意图识别 Agent（分类到正确的专业 Agent）
         → 专业领域 Agent（理财/信贷/保险/投诉）
         → 合规审查层（违禁词过滤 + 误导销售检测）
         → 输出

合规护栏（最高优先级）:
  - 禁止承诺收益率（监管红线）
  - 禁止贬低竞争对手
  - 敏感操作（账户修改、转账）必须转人工
  - 所有建议附加风险提示
```

## 2.3 关键技术实现

```python
# 金融合规护栏（最关键的组件）
class FinancialComplianceGuard:
    """金融行业专用合规护栏"""
    
    # 银行业监管禁用语
    PROHIBITED_PHRASES = [
        r'保证.*%.*收益',
        r'稳赚不赔',
        r'零风险',
        r'绝对安全',
        r'比.*银行好',  # 贬低竞争对手
        r'内部消息',
    ]
    
    # 必须包含风险提示的场景
    RISK_DISCLOSURE_REQUIRED = [
        "理财产品",
        "基金",
        "股票",
        "期货",
        "保险",
    ]
    
    def validate_output(self, output: str, context: str) -> dict:
        """检查输出是否符合金融监管要求"""
        issues = []
        
        # 检查禁用语
        for pattern in self.PROHIBITED_PHRASES:
            if re.search(pattern, output):
                issues.append({
                    "type": "prohibited_phrase",
                    "pattern": pattern,
                    "severity": "critical"
                })
        
        # 检查风险提示
        for product_keyword in self.RISK_DISCLOSURE_REQUIRED:
            if product_keyword in output:
                if "风险" not in output and "亏损" not in output:
                    issues.append({
                        "type": "missing_risk_disclosure",
                        "trigger": product_keyword,
                        "severity": "high"
                    })
        
        return {
            "compliant": len(issues) == 0,
            "issues": issues,
            "requires_human_review": any(i["severity"] == "critical" for i in issues),
        }
```

## 2.4 生产效果

| 指标 | 上线前（纯人工） | 上线后（人机协同） | 改善 |
|------|--------------|-----------------|------|
| 日均处理咨询量 | 50,000 | 50,000（不变） | 质量提升 |
| 人工坐席数 | 200 | 80（减少 60%） | **↓ 60%** |
| 平均响应时间 | 45s（等待接入） | 3s（AI 即时响应） | **↓ 93%** |
| 首次解决率 | 72% | 81% | **↑ 12.5%** |
| 夜间服务覆盖 | 20 人值班 | Agent 全覆盖 | **↑ 100%** |
| 合规违规率 | 0.3% | 0.02% | **↓ 93%** |
| 月度节省成本 | - | $180,000 | ROI > 10x |

---

<!-- chunk: 案例三：DevOps 代码审查 Agent -->## 案例三：DevOps 代码审查 Agent

## 3.1 项目背景

**企业背景**：某 SaaS 公司，500+ 研发人员，每日 PR 200+，代码审查是研发瓶颈。

**核心痛点**：
- PR 等待审查平均 18 小时（研发效率低）
- 高级工程师 40% 的时间花在代码审查上
- 审查质量不稳定（取决于审查者经验和状态）
- 安全漏洞和性能问题检出率低

## 3.2 Agent 架构

```
PR 创建/更新
    │
    ▼
┌───────────────────────────────────┐
│       PR Triage Agent              │
│  评估 PR 规模和风险，决定审查深度    │
└──────┬────────────────────────────┘
       │
  ┌────┴────────────────────────┐
  ▼                             ▼
代码质量 Agent              安全扫描 Agent
- 代码规范检查              - OWASP 漏洞检测
- 性能问题识别              - 依赖漏洞（CVE）
- 重复代码检测              - 密钥泄露检测
- 单测覆盖率                - SQL 注入
  │                             │
  └────────────┬────────────────┘
               ▼
        ┌──────────────┐
        │  汇总报告 Agent│
        │ 生成审查意见   │
        │ 标注高优先级   │
        └──────────────┘
               │
               ▼
  [自动通过] 或 [Request Changes + 详细意见]
```

## 3.3 关键实现

```python
# Kubernetes YAML 专项审查（对接 kudig-database 知识）
class K8sManifestReviewer:
    """K8s YAML 清单专项审查"""
    
    def __init__(self, rag_retriever, llm):
        self.rag = rag_retriever
        self.llm = llm
    
    def review_manifest(self, yaml_content: str) -> dict:
        """审查 K8s YAML 是否符合最佳实践"""
        
        # 从 kudig-database/domain-32-yaml-manifests 检索最佳实践
        best_practices = self.rag.get_relevant_documents(
            "K8s deployment yaml security resource limits best practices",
            filter={"domain": "domain-32-yaml-manifests"}
        )
        
        review = self.llm.invoke(f"""
        审查以下 K8s YAML 配置，对照最佳实践指出问题：
        
        YAML 内容:
        {yaml_content}
        
        最佳实践参考:
        {[d.page_content for d in best_practices[:3]]}
        
        检查点（逐一评估）：
        1. resources.requests/limits 是否设置
        2. readinessProbe/livenessProbe 是否配置
        3. securityContext（runAsNonRoot、readOnlyRootFilesystem）
        4. image 是否使用了 latest tag
        5. 是否有过高的权限（privileged、hostPID）
        6. PodDisruptionBudget 是否应该配置
        
        输出格式：每个问题包含：严重程度(critical/warning/info) + 问题描述 + 修复建议
        """)
        
        return parse_review_output(review.content)
```

## 3.4 生产效果

| 指标 | 上线前 | 上线后 | 改善 |
|------|-------|-------|------|
| PR 平均等待审查时间 | 18 小时 | 3 分钟（AI 即时）+ 2 小时（人工确认）| **↓ 89%** |
| 高级工程师审查占用时间 | 40% | 15%（只审 Agent 标记的重点）| **↓ 62.5%** |
| 安全漏洞漏检率 | 23% | 8% | **↓ 65%** |
| K8s 配置问题检出率 | 31% | 87% | **↑ 181%** |
| 代码规范合规率 | 78% | 94% | **↑ 21%** |
| 单测覆盖率（Agent 提醒）| 61% | 79% | **↑ 30%** |

---

<!-- chunk: 案例四：经验提炼与最佳实践 -->## 案例四：经验提炼与最佳实践

## 4.1 成功共性要素

| 要素 | 描述 | 在三个案例中的体现 |
|------|------|-----------------|
| **领域知识质量** | 高质量的领域知识库是 Agent 准确率的上限 | kudig-database 直接决定了 K8s Agent 的诊断质量 |
| **人在环中设计** | 高风险操作必须有人工门禁 | 所有案例都有关键节点的人工确认 |
| **渐进式授权** | 从只读/建议开始，积累信任后扩权 | 三个案例都经历了先部分上线的过程 |
| **可观测性先行** | 上线第一天就接入完整的可观测性 | Langfuse + Prometheus 同步上线 |
| **[[AI基础设施/03-agent-runtime/14-agent-evaluation-benchmarks.md|质量评估体系]]** | 建立基准测试集，持续监控质量 | 每日自动运行评估，监控质量回退 |

## 4.2 规避的关键失败模式

```
# 🟢 低风险：只读/信息收集，通常无副作用
失败模式1: "全部自动化" 的冒进设计
  问题: 初期直接部署全自动修复，绕过人工审批
  后果: Agent 错误判断导致生产事故，反而延长了 MTTR
  正确做法: Level 1（建议）→ Level 2（半自动）→ Level 3（全自动）逐步推进

失败模式2: 忽略知识库质量
  问题: 将未经整理的内部 Wiki 直接向量化作为 RAG 知识库
  后果: 检索到大量过时或错误的文档，Agent 输出质量极差
  正确做法: 知识库清洗和结构化（类似 kudig-database 的质量标准）是 RAG Agent 的前提

失败模式3: 提示词不稳定
  问题: 开发期间频繁修改系统提示，没有版本管理
  后果: 生产行为随机变化，用户抱怨 Agent "变笨了"
  正确做法: 提示词版本化管理（Git 跟踪），每次修改必须跑评估基准

失败模式4: 不处理 LLM 幻觉
  问题: 相信 Agent 的所有输出，不做验证
  后果: Agent 声称执行了 kubectl 命令但其实工具调用失败，错误地告知用户"已修复"
  正确做法: 工具调用结果必须验证，最终状态要通过工具实际确认而非 LLM 描述
```
## 4.3 企业 Agent 项目立项参考

```
Agent 项目 ROI 快速评估框架:

1. 识别重复性任务
   - 每天处理多少类似的请求/工单？
   - 平均每次人工处理需要多久？
   - 现有 SOP（标准操作规程）是否完整？

2. 估算收益
   时间节省 = 日均任务量 × 平均处理时长 × 自动化率
   成本节省 = 时间节省 × 人工成本/小时
   质量提升 = 错误率降低 × 单次错误损失

3. 估算成本
   LLM 费用 = 日均任务量 × 平均 Token/任务 × 模型单价
   基础设施 = 向量库 + Redis + 应用服务器 ≈ $500-2000/月
   开发成本 = 3-6人月（初版）

4. 评估可行性
   知识库: 是否有结构化的领域知识？（若无，需先建设）
   数据合规: 任务数据是否可以发送给 LLM API？
   风险容忍: 错误率 X% 是否可接受？影响是否可回滚？

典型 ROI 计算（K8s 运维案例）:
  节省人力: 15 人 × 20% 效率 = 3 人工作量 = $30万/年
  LLM 成本: $2400/月 = $2.88万/年
  ROI: ($30万 - $2.88万) / 开发投入 ≈ 300-500%
```

---

<!-- chunk: 5. 行业落地路线图 -->## 5. 行业落地路线图

```
Phase 1: PoC（2-4 周）
  ├── 选择 1-2 个具体、高重复性的场景
  ├── 建立最小可用的知识库（50-100 个文档）
  ├── 实现 ReAct Agent + 3-5 个核心工具
  ├── 建立基准评估集（50 个问题 + 答案）
  └── 成功标准: 准确率 > 80%，主要场景可用

Phase 2: Pilot（4-8 周）
  ├── 生产数据接入（真实集群/真实用户）
  ├── 完整的审批流程和安全护栏
  ├── 可观测性平台（Langfuse/LangSmith）
  ├── 有限用户的灰度测试（5-10%）
  └── 成功标准: 用户满意度 > 4/5，无严重事故

Phase 3: Scale（8-16 周）
  ├── 多 Agent 协作架构
  ├── 记忆系统（历史案例积累）
  ├── 模型路由优化（成本降低 50%+）
  ├── 完整灰度发布流程
  └── 成功标准: 目标指标达成，ROI 为正

Phase 4: Mature（持续）
  ├── 持续优化知识库（覆盖新场景）
  ├── 基于生产数据 Fine-tuning（可选）
  ├── 自动化评估 CI/CD
  └── 成功标准: 系统自我进化，准确率持续提升
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | 案例中 ReAct/LangGraph 的应用 |
| [04 - RAG 检索](./04-rag-knowledge-retrieval.md) | kudig-database 在运维 Agent 中的 RAG 实践 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | Supervisor-Worker 模式的实际案例 |
| [10 - 安全护栏](./10-security-guardrails.md) | 金融案例中的合规护栏实现 |
| [14 - Agent 赋能设计与落地路径](./14-agent-kudig-design-strategy.md) | K8s Agent 的战略设计 |
| [topic-fta](../故障诊断/topic-fta/) | 故障树分析在 AIOps Agent 中的应用 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容，案例数据经脱敏处理。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents MOC
- [[AI基础设施/02-ai-agents/README.md|AI Agent 工程专题]]
- [[AI基础设施/02-ai-agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[AI基础设施/02-ai-agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[AI基础设施/02-ai-agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[AI基础设施/02-ai-agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[AI基础设施/02-ai-agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[AI基础设施/02-ai-agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[AI基础设施/02-ai-agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[AI基础设施/02-ai-agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[AI基础设施/02-ai-agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[AI基础设施/02-ai-agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 10-security-guardrails
- 11-cost-latency-optimization
- 13-trusted-agent-system-fiscal-plan
- 14-agent-kudig-design-strategy


<!-- risk-assessed -->

---
title: 工单智能体 × RAG
summary: 工单智能体与 RAG 检索增强生成的交叉：如何为阿里云专有云 K8s 运维构建可信赖的 Agent 语料体系。
category: synthesis
tags:
- ticket-agent
- rag
- ai-agent
- llm
- knowledge-graph
tier: supporting
sources:
- _meta/projects/kudig-ticket-agent-corpus-improvement-plan.md
- _meta/corpus-config/profiles/rag-ticket-agent-profile.yaml
- 生产运维/ticket-routing-rules.md
- 生产运维/escalation-playbook.md
- 故障诊断/topic-skills/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-07-11
provenance:
  extracted: 0.35
  inferred: 0.55
  ambiguous: 0.1
base_confidence: 0.7
lifecycle: draft
lifecycle_changed: '2026-06-26'
---


# 工单智能体 × RAG

## The Connection

工单智能体需要理解自然语言工单、分类优先级、给出诊断方案和回复话术。RAG（检索增强生成）通过从知识库中检索相关上下文，减少 LLM 幻觉并确保回答基于项目语料而非通用知识。二者结合，决定了 Agent 能否在专业运维场景（如阿里云专有云 K8s 故障处理）中给出可信、可执行的答案——一个基于错误文档的 RAG 检索比没有 RAG 更危险，因为它会以高置信度给出错误操作指令。因此，RAG 语料质量与检索精度直接决定了工单智能体的可用性边界。从架构上看，工单智能体的 RAG 管道包含三层：(1) 语料构建层——将工单闭环样本、Skill 文档、源文档转化为可检索的 chunk + embedding；(2) 检索层——在工单输入后，先做意图分类（如"节点 NotReady" vs "网络不通"），再从向量库 + 关键词索引中召回 Top-K 相关片段；(3) 生成层——将检索到的上下文拼入 LLM prompt，由 Agent 输出诊断结论和修复建议。每一层的精度衰减会级联放大：检索召回率低导致 LLM 缺少关键上下文，生成幻觉率升高；chunk 策略不当导致检索到的片段缺乏完整性，LLM 输出的操作步骤不可执行。^[inferred]

## Where They Co-occur

- `rag-ticket-agent-profile.yaml` 定义了工单样本 → Skill/FTA → 源文档的三层检索优先级，确保 Agent 优先匹配历史闭环案例而非泛化文档
- 工单闭环样本为 RAG 提供"问题-诊断-修复-验证"的完整上下文，是最贴近真实场景的高质量语料
- QA 语料（1,456 对 I-O）作为 RAG 检索的关键来源，覆盖常见 K8s 故障的标准化问答
- Skill 深度补充文档增强 RAG 的推理链，而不只是命令模板——如 `k8s-node-notready/SKILL-DEEP-DIVE.md` 包含诊断决策树和排障逻辑
- Embedding Pipeline 默认使用 bge-m3，将文本转换为语义向量，支持中英文混合检索
- **向量库 + 关键词混合检索**：纯向量检索对专有名词（如 `kubelet PLEG`）不敏感，需叠加 BM25 关键词匹配（hybrid search）提高召回率
- **重排序（Reranker）**：初检召回 Top-K 后，用 cross-encoder 模型（如 bge-reranker）对候选段落精排，将最相关的诊断上下文推到 prompt 前部
- **Chunk 策略对齐**：源文档按"故障现象/原因/修复步骤"语义分块而非固定长度切分，保证检索到的片段是可执行的完整操作单元
- **多轮对话上下文注入**：工单处理往往是多轮交互（补充信息 → 重新诊断），RAG 系统需在每轮对话中维护工单上下文窗口，避免重复检索已确认的信息
- **Agent 工具调用 + RAG 融合**：当 RAG 检索到操作类知识后，Agent 通过 function calling 直接执行诊断命令（如 `kubectl describe node`），将命令输出再注入 RAG 做二次检索，形成"检索 → 执行 → 再检索"的 agentic loop
- **Feedback Loop 闭环语料**：Agent 回复后，工单处理结果（成功/失败/修改建议）作为反馈标注回灌语料库，持续优化检索权重和 prompt 模板
- **多模态工单解析**：部分运维工单包含截图（如 Grafana dashboard 异常截图、kubectl 输出截图），需 OCR + 图像理解提取结构化信息后纳入 RAG 检索
- **置信度标注与人工兜底**：RAG 检索 Top-K 的余弦相似度分数作为 Agent 回复置信度，低于阈值时自动转人工并标注"语料不足"标签，触发语料补充流程
- **Prompt 模板分阶段设计**：不同工单阶段（分类 → 诊断 → 修复 → 验证）使用不同 prompt 模板，每个模板注入对应阶段的 RAG 检索结果——分类阶段注入历史路由规则，诊断阶段注入 Skill 决策树，修复阶段注入操作手册
- **知识图谱增强检索**：将 K8s 组件依赖关系（etcd → API Server → Controller → Pod）建模为知识图谱节点，RAG 检索时不仅按语义相似度匹配，还按图谱边的关联性扩展——如检索到"etcd 不可用"时自动扩展"API Server 延迟"和"Controller Reconcile 失败"的关联知识
- **Agent 误操作防护**：高危命令（如 `kubectl delete`、`kubectl drain`）需二次确认或审批链，Agent 通过 tool calling 执行前注入 `--dry-run=server` 预检，避免基于错误 RAG 检索结果执行破坏性操作
- **多语言语料对齐**：阿里云专有云工单可能中英文混合（如中文描述 + 英文 error log），embedding 模型需支持跨语言语义匹配（bge-m3 支持 100+ 语言），避免中文工单检索不到英文技术文档
- **RAG 评估指标**：使用 RAGAS（Retrieval Augmented Generation Assessment）框架度量 Faithfulness（回答是否基于检索内容）、Answer Relevancy（回答是否切题）、Context Precision/Recall（检索质量），建立工单智能体的量化基线
- **版本化语料管理**：语料库按 K8s/专有云版本打标签（如 `v1.28`、`v1.30`），检索时注入当前集群版本作为过滤条件，确保返回的修复方案与运行环境匹配
- **工单分流与 Agent 协同**：工单智能体先做意图分类（如故障诊断、咨询、变更请求），再路由到对应 RAG 语料库和 Agent 工具链——避免"一个 prompt 处理所有类型"导致的精度下降

## Cross-cutting Insight

RAG 解决"知道什么"的问题，工单智能体解决"怎么做"的问题。没有高质量 RAG 语料，智能体只能依赖通用知识（可能给出"标准但在此环境中错误"的建议）；没有智能体的任务框架，RAG 检索到的知识无法转化为闭环工单处理流程。更深层的挑战在于"语料时效性"——专有云 K8s 版本更新后，历史工单中的修复命令可能已失效。这要求 RAG 系统不仅检索"最相关"的片段，还需标注其版本适用性和时效置信度，否则 Agent 会基于过时知识给出错误指导。此外，运维工单的 RAG 面临"长尾问题"的严峻考验：80% 的工单是常见故障（NotReady、OOM、ImagePullBackOff），但剩余 20% 的长尾故障（如特定内核版本的 CNI bug、特定机型 BIOS 的 GPU 初始化失败）恰恰是最需要 Agent 辅助的场景。这些长尾工单的闭环样本稀少，RAG 语料覆盖不足，导致 Agent 在最需要发挥价值的场景中反而退化为"通用建议机器"。因此，工单智能体的 RAG 语料策略必须包含主动学习机制——优先从 Agent 无法闭环的长尾工单中提取新语料，形成"失败 → 标注 → 入库 → 改进"的持续学习闭环。^[inferred]

## Tensions and Trade-offs

| 维度 | RAG 系统侧重 | 工单智能体侧重 | 结合挑战 |
|---|---|---|---|
| 优化目标 | 检索相关性（recall/precision） | 任务完成率（工单闭环率） | 检索结果需匹配当前工单阶段 |
| 内容形态 | 知识文档（静态、结构化） | 操作手册（动态、流程化） | 需统一为"可执行语料" |
| 时效性 | 知识更新慢（需重新索引） | 工单处理快（实时响应） | 需增量索引机制 + 版本标记 |
| 可解释性 | 引用来源（citation） | 给出理由（reasoning） | 需输出诊断证据链（检索片段 → 推理 → 结论） |
| 安全 | 访问控制（哪些文档可检索） | 命令执行风险（高危操作） | 高 risk action 需人工确认 |
| 评估 | 离线 RAG benchmark | 在线工单满意度/闭环率 | 需端到端评估而非分阶段 |
| 多轮交互 | 检索是无状态的单次查询 | 工单处理是多轮对话 | 需维护工单级上下文窗口 |

## Open Questions

- 如何评估 RAG 检索结果对工单分类准确率的实际贡献？是否需要 A/B 测试框架对比"有 RAG"和"无 RAG"的工单处理质量？
- 工单样本中的 action 字段是否需要单独索引以支持 Agent 工具调用（function calling），而非作为纯文本检索？
- 在专有云工单场景中，如何平衡通用 K8s 知识与阿里云/专有云特定知识的检索权重？是否需要 query classification 先判理工单类型再路由到不同语料库？
- 当历史工单中的修复方案因版本升级而失效时，如何检测并降权过时语料？是否需要定期运行"语料有效性验证"流程？
- 对于 RAG 语料未覆盖的长尾故障，Agent 应如何优雅降级为"转人工 + 自动记录新语料"而非强行给出低置信度建议？

## Related

- _meta/projects/kudig-ticket-agent-corpus-improvement-plan.md
- _meta/corpus-config/profiles/rag-ticket-agent-profile
- [[生产运维/ticket-routing-rules.md|ticket routing rules]]
- [[生产运维/escalation-playbook.md|escalation playbook]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md|SKILL DEEP DIVE]]

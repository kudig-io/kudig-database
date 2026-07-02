---
title: KUDIG 语料库深度评估报告 — 使用角度 × 运维工单角度
summary: KUDIG 语料库深度评估报告 — 使用角度 × 运维工单角度：1. QA 命令多样性仅 8.2%（目标 90%）— 工单 Agent 诊断命令同质化严重
  2. Critical severity 仅占 0.4% — 高危场景覆盖不足 3. Mock embedding 未接入真实语义模型 — RAG 检索质量受限
category: assessment
tags:
- assessment
- quality
- sre
- ops
- rag
- evaluation
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 语料库深度评估报告

> 评估维度：使用角度（知识消费体验） × 运维工单角度（SRE Agent 生产就绪度）
> 评估时间：2026-05-23
> 语料规模：5,433 Markdown 文件 / 95.6 MB / 20 Domain

---

## 一、执行摘要

| 维度 | 评分 | 说明 |
|:---|:---:|:---|
| **使用角度（知识产品）** | **4.1/5** | 结构完整、交叉引用丰富，但信噪比和检索精度仍有提升空间 |
| **运维工单角度（SRE Agent）** | **3.8/5** | 推理骨架（FTA/Skill）齐全，但命令多样性、严重度分布、可执行性存在短板 |
| **综合评分** | **~4.0/5** | 达到"标准生产可用"水准，距离"领先生产水准"（4.3+）需补齐 3 个关键缺口 |

**关键缺口**：
1. QA 命令多样性仅 8.2%（目标 90%）— 工单 Agent 诊断命令同质化严重
2. Critical severity 仅占 0.4% — 高危场景覆盖不足
3. Mock embedding 未接入真实语义模型 — RAG 检索质量受限

---

## 二、使用角度评估（知识产品维度）

### 2.1 信息架构（4.5/5）

**优势**
- **三层架构清晰**：Domain（20 个）→ Topic（~80 个）→ 具体文档，层次分明
- **统一元数据**：99.8% 文件有 title，99.1% 有 tags，100% 有 category
- **交叉引用密集**：85.1% 文件包含 wikilink，平均 10.8 个链接/文件，形成强连接知识网络
- **多维索引**：418 个 MOC/README 文件提供入口导航

**劣势**
- **Domain-19 信噪比低**：1,614 个文件（占全库 30%）为 CNCF Landscape 项目简介，平均 12KB，大量泛化内容稀释核心领域密度
- **存储/可靠性 Domain 偏薄**：domain-04 平均 24KB/文件，domain-09 平均 23KB/文件，远低于 domain-10 的 44KB/文件

| Domain | 文件数 | 平均大小 | 评估 |
|:---|:---:|:---|:---|
| domain-10 问题诊断 | 302 | 44 KB | ⭐ 核心资产，深度足够 |
| domain-01 集群基础 | 102 | 36 KB | ⭐ 深度良好 |
| domain-03 网络 | 125 | 33 KB | ⭐ 深度良好 |
| domain-05 安全 | 61 | 34 KB | ⭐ 深度良好 |
| domain-04 存储 | 31 | 24 KB | ⚠️ 偏薄，仅 31 个文件 |
| domain-09 可靠性 | 35 | 23 KB | ⚠️ 偏薄，SRE 实践深度不足 |

### 2.2 内容消费体验（4.2/5）

**优势**
- **多种消费模式**：深度文档（domain-*）+ 速查卡（cheat-sheet）+ 问题树（FTA）+ 技能手册（Skill）+ 案例（Case Study）+ 合成页面（Synthesis），适配不同场景
- **训练路径完整**：158 个训练文件，83,017 行，覆盖从入门到进阶
- **Synthesis 跨域连接**：52 个合成页面建立跨域知识桥梁，解决"知道 A 和 B，但不知道 A×B"的问题
- **Case Study 真实感强**：23 个案例含具体时间线、命令输出、MTTR 指标，可信度高

**劣势**
- **Cheat Sheet 稀缺**：仅 5 个速查卡，远不能满足一线运维快速查询需求
- **Case Study 分布不均**：P0 仅 4 个（目标应 6-8 个），critical 场景覆盖不足
- **部分 Synthesis 偏薄**：早期 synthesis 文件（如 `data-protection-k8s.md`）仅 1,399 字节，内容深度不够

### 2.3 检索与发现（3.6/5）

**优势**
- **向量 Pipeline 就绪**：22,973 个 chunk，34MB embeddings，支持语义搜索
- **多策略分块**：by_h2 / by_h3 / by_section / full_doc，适配不同文档类型
- **增量更新**：基于文件 hash 的 manifest，变更检测准确

**劣势**
- **Embedding 为 Mock**：当前使用确定性伪向量，搜索基于关键词 hash 而非语义，返回结果的相关性有限
  - 测试 "Pod CrashLoopBackOff 诊断" → 返回 DNS 问题（相关度不高）
  - 测试 "etcd 备份恢复" → 返回 AI Agent 安全（完全不相关）
- **缺乏 BM25 混合检索**：纯向量检索无关键词兜底，专业术语匹配不稳定
- **无查询意图分类**：无法自动区分"学习查询"vs"问题排查查询"vs"配置查询"

### 2.4 学习路径（4.0/5）

**优势**
- **结构化培训**：skills/training-public/ 158 个文件，按周/天组织学习路径
- **评估体系**：含 quiz、lab exam、daily check 等评估材料
- **难度分级**：frontmatter 含 difficulty、reading_level 字段

**劣势**
- **缺乏个性化推荐**：无基于用户知识图谱的推荐机制
- **学习进度追踪缺失**：无完成度、掌握度指标

---

## 三、运维工单角度评估（SRE Agent 维度）

### 3.1 问题诊断链路完整性（4.2/5）

**优势**
- **FTA 覆盖全面**：44 个组件问题树，覆盖 apiserver、etcd、DNS、CNI、CSI、deployment 等核心组件
- **Skill 闭环完整**：17 个 skill-set，每个包含「症状识别 → 诊断命令 → 根因判定 → 修复动作 → 验证」完整流程
- **QA 语料规模**：5,159 对 QA，Skill 覆盖率 100%，FTA 覆盖率 100%

**劣势**
- **命令多样性严重不足**：
  - 5,159 条命令中仅 425 条唯一命令，多样性比率 **8.2%**（目标 90%）
  - `kubectl get nodes` 重复 77 次
  - `kubectl get pods -n <name> -l <name>` 重复 66 次
  - **影响**：Agent 在不同问题场景下可能给出相同的诊断命令，缺乏场景适配性
- **严重度分布失衡**：
  - medium: 65.9% (3,399)
  - high: 33.7% (1,738)
  - critical: **0.4%** (22)
  - **影响**：Agent 对 P0 级问题的训练数据不足，高危场景响应能力弱

### 3.2 修复动作可执行性（3.5/5）

**优势**
- **Skill 脚本化**：38 个 .sh 脚本（diagnose-quick.sh、verify-*.sh 等），部分修复可一键执行
- **Case Study 含真实命令**：每个案例包含完整的 kubectl/ssh/systemctl 命令序列
- **L2-semi-auto 标注**：Skill frontmatter 标注 agent_execution_mode，区分自动/审批操作

**劣势**
- **QA Action 实际空缺**：
  - QA YAML 中大量 `action: []`（精确统计待补）
  - 从 grep 结果看，467/469 的 action 行为空或仅含模板化内容
  - **影响**：Agent 知道"怎么诊断"，但不知道"怎么修复"
- **脚本覆盖率低**：38 个脚本对应 17 个 Skill，平均 2.2 个/Skill，很多修复场景无脚本支持
- **验证脚本缺失**：`verify-*.sh` 数量不足，修复后验证环节薄弱

### 3.3 根因分析深度（3.8/5）

**优势**
- **FTA 数学基础**：含 FTA 符号系统、数学基础、布尔代数推演，根因分析有理论支撑
- **FEBM 取证方法**：结构化取证方法论，支持日志/指标/追踪联合分析
- **Case Study 复盘完整**：每个案例含「直接原因 + 根本原因 + 改进措施」三层分析

**劣势**
- **缺乏 RCA 模板标准化**：不同 Skill 的根因分析深度不一致，部分仅停留在表面
- **无关联问题分析**：缺少"A 问题引发 B 问题"的级联分析（如 etcd 慢 → API Server 超时 → HPA 失效）
- **无历史模式学习**：Case Study 未提取为「问题模式库」，无法支持"相似历史问题"推荐

### 3.4 Agent 集成就绪度（3.5/5）

**优势**
- **Profile 配置成熟**：rag-sre-profile、rag-full-profile、rag-learning-profile 覆盖不同场景
- **Chunk 元数据丰富**：每个 chunk 含 domain、section_title、difficulty 等元数据，支持过滤检索
- **向量化 Pipeline 可插拔**：支持 mock / local / OpenAI 三种 provider，易于接入生产模型

**劣势**
- **无意图路由**：无法自动将用户 query 路由到「诊断模式」vs「学习模式」vs「配置查询模式」
- **无上下文记忆**：Pipeline 为静态索引，不支持对话历史上下文
- **无反馈闭环**：搜索结果好坏无用户反馈机制，无法迭代优化

---

## 四、关键发现矩阵

| 发现 | 影响 | 紧急度 | 建议 |
|:---|:---|:---:|:---|
| **QA 命令多样性 8.2%** | Agent 诊断同质化，场景适配性差 | 🔴 高 | 引入命令生成模板 + 场景化参数注入，目标提升至 30%+ |
| **Critical severity 0.4%** | P0 问题响应能力不足 | 🔴 高 | 补充 20+ 个 critical QA pair，覆盖 etcd 脑裂、全集群网络中断等 |
| **Mock Embedding** | RAG 检索质量不可用于生产 | 🟠 中 | 接入 all-MiniLM-L6-v2（本地）或 text-embedding-3-small（API） |
| **QA Action 空缺** | Agent 知道诊断但不知道修复 | 🟠 中 | 批量填充 action 字段，目标覆盖率 80%+ |
| **Domain-19 信噪比低** | 稀释核心领域知识密度 | 🟡 低 | 在 rag-full-profile 中降低 priority 或拆分独立 profile |
| **Domain-04/09 偏薄** | 存储和可靠性知识深度不足 | 🟡 低 | 扩充 Velero、Rook、SLO/SLI 等深度文档 |
| **Cheat Sheet 稀缺** | 一线运维查询效率低 | 🟡 低 | 补充 kubectl、helm、cri-o 速查卡至 15+ 个 |
| **缺乏 BM25 混合检索** | 专业术语匹配不稳定 | 🟡 低 | 集成 whoosh / sqlite-fts 作为关键词检索层 |

---

## 五、使用角度 vs 运维工单角度对比

| 评估项 | 使用角度 | 运维工单角度 | 差距 |
|:---|:---:|:---:|:---|
| 信息架构 | 4.5 | 4.2 | 用户导航优于 Agent 消费 |
| 内容深度 | 4.2 | 3.8 | 学习场景充足，执行场景薄弱 |
| 检索质量 | 3.6 | 3.5 | 两者均受限于 mock embedding |
| 闭环完整性 | 3.8 | 3.5 | 学习有路径，修复有缺口 |
| 可执行性 | 3.5 | 3.5 | 脚本和 action 均不足 |
| **综合** | **4.1** | **3.8** | 知识产品属性强于 Agent 工具属性 |

**核心洞察**：KUDIG 当前更接近「高质量知识库」而非「生产级 Agent 语料」。知识密度、交叉引用、学习路径等「产品属性」优秀；但命令多样性、action 可执行性、严重度平衡等「工具属性」存在明显短板。

---

## 六、改进路线图

### 阶段 3.5（2-3 天）— 关键缺口补齐
1. 接入真实 Embedding 模型（local 或 OpenAI）
2. 补充 20+ critical QA pair
3. 批量填充 QA action 至 80% 覆盖率

### 阶段 4（1-2 周）— Agent 工具属性强化
4. 重构 QA 命令生成，引入场景化参数，多样性提升至 30%
5. 为每个 Skill 补充 verify-*.sh 脚本
6. 构建「问题模式库」从 Case Study 提取

### 阶段 5（2-4 周）— 生产就绪
7. 集成 BM25 + Vector 混合检索
8. 添加查询意图路由（诊断/学习/配置）
9. 构建用户反馈闭环（点赞/点踩 → 索引迭代）
10. Domain-04/09 深度文档扩充

---

## 七、附录：数据快照

```
全库规模
  Markdown 文件: 5,433
  总字节: 95.6 MB
  平均文件: 17,605 字节
  Wikilink 覆盖率: 85.1% (4,628/5,433)
  平均 Wikilink: 10.8 个/文件

核心资产
  FTA 问题树: 44 个
  Skill 手册: 17 个 (38 个脚本)
  QA 语料: 5,159 对
  Case Study: 23 个
  Synthesis: 52 个

向量索引
  Full Corpus: 2,642 文件 → 22,973 chunk → 34 MB
  SRE Profile: 206 文件 → 1,856 chunk → 2.6 MB

质量指标
  Frontmatter title: 99.8%
  Frontmatter tags: 99.1%
  Frontmatter category: 100.0%
  Skill coverage: 100% (32/32)
  FTA coverage: 100% (44/44)
  QA command diversity: 8.2% (425/5,159)
  Critical severity: 0.4% (22/5,159)
```

---

*报告生成时间: 2026-05-23*
*评估方法: 定量指标 + 抽样验证 + 功能测试*
*数据来源: corpus-config/profiles、domain-10-troubleshooting-diagnostics、scripts/embedding-pipeline.py*


<!-- risk-assessed -->

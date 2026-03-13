# 可信智能体体系 — 运维智能体财年规划

> **文档类型**: 战略规划 | **最后更新**: 2026-03 | **关键词**: 可信智能体, 运维智能体, 4K 语料, QA 语料, Skills, Workflow, 评测体系, IaaS, PaaS, 数据库, 容器, AI 产品线, 强参考, 强质检, 窄口径

---

## 概述

云平台运维工单专家组面临核心挑战：**运维场景信任成本是智能体落地的最大壁垒**。诊断建议不可信、操作推荐不可控、质量一致性无法保证，是当前 LLM 直接用于运维的根本痛点。

本规划提出 **可信智能体体系**，以"强参考、强质检、窄口径"三大原则，系统性地为 **IaaS、PaaS、数据库、容器、AI** 五大产品线构建可信运维智能体。

- **强参考**: 每个回答必须基于经过审核的 4K 知识语料和 QA 语料，拒绝无根据的"自由发挥"
- **强质检**: 基于实际工单和测评体系的评估闭环，每个产品线均需通过能力基线验证
- **窄口径**: 能力边界清晰，不做超出能力范围的回答，宁可说"不知道"也不给错误建议

---

## 1. 总体架构

### 1.1 可信智能体体系全景

```mermaid
graph TB
    TOP[可信智能体顶层设计] --> PRIN[三大核心原则<br/>❶ 强参考 — 回答必须有知识来源<br/>❷ 强质检 — 真实工单评测闭环<br/>❸ 窄口径 — 超范围主动拒答]

    PRIN --> ENG1
    PRIN --> ENG2
    PRIN --> ENG3

    subgraph sg_eocc [EOCC 平台 — 语料承载 · 语料生成 · 智能体建设集大成者]

        subgraph sg_eng1 [一、智能体及语料建设工程]
            ENG1[语料 + 智能体] --> E1_4K[4K 知识语料库<br/>5 产品线 × 4000 条]
            ENG1 --> E1_QA[QA 语料 · 10000+ 对]
            ENG1 --> E1_SK[Skills · 5 产品线 112 个]
            ENG1 --> E1_WF[Workflow · 5 类流程]
            ENG1 --> E1_QC[语料质检流水线]
        end

        subgraph sg_eng2 [二、工单覆盖与辅助工程]
            ENG2[工单 + 上线] --> E2_PROD[五大产品线智能体<br/>IaaS · PaaS · 数据库 · 容器 · AI]
            ENG2 --> E2_TICKET[工单接入与处理]
            ENG2 --> E2_DEPLOY[灰度 → 全量上线]
            ENG2 --> E2_IMPROVE[能力提升 Q1-Q4]
        end

        subgraph sg_eng3 [三、质量与测评工程]
            ENG3[质量 + 认证] --> E3_BENCH[工单基准测评 L1-L4]
            ENG3 --> E3_CERT[能力基线认证 · 七项指标]
            ENG3 --> E3_QC[三级质检机制]
        end

    end

    E1_SK --> E2_PROD
    E2_PROD --> E3_CERT
    E3_QC -.-> ENG1
```

### 1.2 五大产品线智能体矩阵

```mermaid
graph LR
    subgraph 产品线矩阵
        IaaS[IaaS 智能体<br/>ECS/网络/安全组/负载均衡]
        PaaS[PaaS 智能体<br/>中间件/消息队列/缓存]
        DB[数据库智能体<br/>RDS/Redis/MongoDB/PolarDB]
        K8s[容器智能体<br/>ACK/镜像/Ingress/存储]
        AI[AI 智能体<br/>GPU 调度/推理服务/训练任务]
    end

    IaaS --> Baseline[能力基线认证]
    PaaS --> Baseline
    DB --> Baseline
    K8s --> Baseline
    AI --> Baseline

    Baseline --> Trusted[可信智能体认证<br/>强参考 + 强质检 + 窄口径]
```

### 1.3 核心设计原则

| 原则 | 定义 | 实施手段 | 反模式 |
|------|------|---------|--------|
| **强参考** | 所有回答必须有明确知识来源引用 | RAG 强制引用、4K 语料关联、回答必须附带参考文档段落 | 无来源的自由生成回答 |
| **强质检** | 基于真实工单的评测闭环 | 工单回放测评、人工抽检、LLM-as-Judge 自动化质检 | 仅靠 Demo 验证、无量化指标 |
| **窄口径** | 能力边界清晰，超范围主动拒答 | 意图分类器 + 能力范围白名单 + 兜底策略 | 全能型回答、超范围强行回答 |

---

## 2. 语料工程

### 2.1 4K 知识语料建设

4K 语料指每个产品线构建 **至少 4000 条**结构化知识条目（Knowledge Items），涵盖概念、原理、配置、排障四大类。

```mermaid
graph TB
    subgraph 4K语料建设流程
        S1[知识采集] --> S2[知识清洗]
        S2 --> S3[知识结构化]
        S3 --> S4[知识审核]
        S4 --> S5[知识入库]
        S5 --> S6[知识检索验证]
    end

    subgraph 知识来源
        SRC1[官方文档] --> S1
        SRC2[历史工单] --> S1
        SRC3[最佳实践] --> S1
        SRC4[故障复盘] --> S1
        SRC5[内部 Wiki] --> S1
    end

    subgraph 知识类型
        S3 --> T1[概念知识<br/>What/Why]
        S3 --> T2[原理知识<br/>How it works]
        S3 --> T3[配置知识<br/>How to configure]
        S3 --> T4[排障知识<br/>How to fix]
    end
```

**各产品线 4K 语料分配**:

| 产品线 | 概念知识 | 原理知识 | 配置知识 | 排障知识 | 合计 | 负责人 |
|-------|---------|---------|---------|---------|------|-------|
| IaaS | 600 | 800 | 1200 | 1400 | 4000 | TBD |
| PaaS | 500 | 700 | 1100 | 1700 | 4000 | TBD |
| 数据库 | 500 | 900 | 1000 | 1600 | 4000 | TBD |
| 容器 | 600 | 1000 | 1000 | 1400 | 4000 | TBD |
| AI | 700 | 800 | 1000 | 1500 | 4000 | TBD |

### 2.2 QA 语料建设

QA 语料是以"问题-回答"对的形式组织的结构化语料，直接映射运维工单的咨询与排障场景。

```mermaid
graph TB
    subgraph QA语料建设流程
        Q1[工单聚类分析] --> Q2[高频问题提取]
        Q2 --> Q3[标准答案撰写]
        Q3 --> Q4[答案质量审核]
        Q4 --> Q5[多轮对话扩展]
        Q5 --> Q6[QA 对入库]
    end

    subgraph QA语料质量标准
        Q4 --> STD1[准确性: 答案技术正确]
        Q4 --> STD2[完整性: 步骤无遗漏]
        Q4 --> STD3[可操作性: 可直接执行]
        Q4 --> STD4[安全性: 不含高危操作]
    end
```

**QA 语料规模目标**:

| 产品线 | 单轮 QA | 多轮 QA | 场景模拟 QA | 合计 | 交付周期 |
|-------|---------|---------|-----------|------|---------|
| IaaS | 1500 | 500 | 200 | 2200 | Q1 |
| PaaS | 1200 | 400 | 200 | 1800 | Q1 |
| 数据库 | 1500 | 600 | 300 | 2400 | Q1 |
| 容器 | 1500 | 500 | 300 | 2300 | Q1 |
| AI | 1000 | 400 | 200 | 1600 | Q2 |

### 2.3 语料质检流水线

```mermaid
graph LR
    RAW[原始语料] --> DEDUP[去重检测<br/>SimHash/MinHash]
    DEDUP --> FACT[事实核查<br/>官方文档交叉验证]
    FACT --> FORMAT[格式规范化<br/>Markdown/JSON Schema]
    FORMAT --> REVIEW[专家审核<br/>双人交叉审核]
    REVIEW --> VERSION[版本管理<br/>Git + 变更追踪]
    VERSION --> PUBLISH[语料发布<br/>灰度上线]
```

---

## 3. Skills 工程

### 3.1 Skills 分类体系

```mermaid
graph TB
    subgraph Skills体系
        direction TB
        SK[Skills 总览]
        SK --> SK1[诊断类 Skills]
        SK --> SK2[操作类 Skills]
        SK --> SK3[分析类 Skills]
        SK --> SK4[预防类 Skills]
    end

    subgraph 诊断类Skills
        SK1 --> D1[日志分析 Skill]
        SK1 --> D2[指标异常检测 Skill]
        SK1 --> D3[链路追踪 Skill]
        SK1 --> D4[故障树推理 Skill]
    end

    subgraph 操作类Skills
        SK2 --> O1[配置变更 Skill]
        SK2 --> O2[扩缩容 Skill]
        SK2 --> O3[重启恢复 Skill]
        SK2 --> O4[备份回滚 Skill]
    end

    subgraph 分析类Skills
        SK3 --> A1[容量规划 Skill]
        SK3 --> A2[性能分析 Skill]
        SK3 --> A3[成本优化 Skill]
        SK3 --> A4[趋势预测 Skill]
    end

    subgraph 预防类Skills
        SK4 --> P1[巡检 Skill]
        SK4 --> P2[风险评估 Skill]
        SK4 --> P3[合规检查 Skill]
        SK4 --> P4[变更影响评估 Skill]
    end
```

### 3.2 各产品线 Skills 矩阵

| 产品线 | 诊断类 | 操作类 | 分析类 | 预防类 | 合计 | 优先级 |
|-------|-------|-------|-------|-------|------|-------|
| IaaS | 8 | 6 | 4 | 4 | 22 | P0 |
| PaaS | 6 | 5 | 3 | 3 | 17 | P0 |
| 数据库 | 10 | 8 | 5 | 4 | 27 | P0 |
| 容器 | 12 | 8 | 5 | 4 | 29 | P0 |
| AI | 6 | 4 | 4 | 3 | 17 | P1 |

### 3.3 Skill 标准化定义模板

```yaml
# Skill 定义规范
skill:
  name: "k8s-pod-crashloopbackoff-diagnosis"
  version: "1.0.0"
  product_line: "容器"
  category: "诊断类"
  description: "诊断 Pod CrashLoopBackOff 的根因并给出修复建议"
  
  # 输入参数
  inputs:
    - name: namespace
      type: string
      required: true
    - name: pod_name
      type: string
      required: true
      
  # 执行步骤（强参考）
  steps:
    - action: "kubectl describe pod {pod_name} -n {namespace}"
      extract: ["events", "container_status", "exit_code"]
    - action: "kubectl logs {pod_name} -n {namespace} --tail=100"
      extract: ["error_patterns"]
    - action: "reasoning"
      reference: ["4K 排障知识库", "QA 语料库"]
      
  # 输出规范
  outputs:
    - root_cause: "明确根因描述"
    - evidence: "诊断证据链"
    - recommendation: "修复建议（附操作步骤）"
    - reference: "引用的知识条目 ID"
    
  # 质检要求
  quality:
    accuracy_threshold: 0.90
    reference_required: true
    human_review_rate: 0.20
```

---

## 4. Workflow 工程

### 4.1 运维 Workflow 分类

```mermaid
graph TB
    subgraph Workflow体系
        WF[Workflow 总览]
        WF --> WF1[标准咨询 Workflow]
        WF --> WF2[故障诊断 Workflow]
        WF --> WF3[应急响应 Workflow]
        WF --> WF4[变更执行 Workflow]
        WF --> WF5[巡检报告 Workflow]
    end

    subgraph 标准咨询Workflow
        WF1 --> C1[意图识别] --> C2[知识检索<br/>强参考]
        C2 --> C3[答案生成] --> C4[质量校验<br/>强质检]
        C4 --> C5[用户反馈]
    end

    subgraph 故障诊断Workflow
        WF2 --> F1[故障分类] --> F2[信息采集<br/>自动化]
        F2 --> F3[故障树推理] --> F4[根因定位]
        F4 --> F5[修复建议<br/>窄口径] --> F6[执行确认]
    end
```

### 4.2 故障诊断 Workflow 详细流程

```mermaid
graph TB
    START[工单接入] --> INTENT[意图分类器<br/>咨询/故障/变更/投诉]
    INTENT --> |故障| CLASSIFY[故障分类<br/>产品线 + 故障类型]
    CLASSIFY --> COLLECT[信息自动采集<br/>调用诊断 Skills]
    COLLECT --> FTA[故障树推理<br/>FTA 引擎]
    FTA --> ROOT[根因判定<br/>置信度评分]
    
    ROOT --> |置信度>=0.8| RECOMMEND[修复建议生成<br/>强参考知识库]
    ROOT --> |置信度<0.8| ESCALATE[升级人工专家<br/>窄口径兜底]
    
    RECOMMEND --> REVIEW[建议质检<br/>安全性 + 正确性]
    REVIEW --> |通过| DELIVER[交付用户]
    REVIEW --> |不通过| REVISE[人工修正]
    REVISE --> DELIVER
    
    DELIVER --> FEEDBACK[用户反馈收集]
    FEEDBACK --> LEARN[反馈入库<br/>持续优化语料]
```

### 4.3 应急响应 Workflow

```mermaid
graph TB
    ALERT[告警触发/P0 工单] --> TRIAGE[智能分诊<br/>影响面评估]
    TRIAGE --> |影响面大| WAR_ROOM[战备群自动拉起<br/>通知相关方]
    TRIAGE --> |影响面小| STANDARD[标准故障诊断流程]
    
    WAR_ROOM --> TIMELINE[时间线自动生成]
    TIMELINE --> DIAG[并行诊断<br/>多 Skill 协同]
    DIAG --> MITIGATION[止血方案<br/>快速恢复优先]
    MITIGATION --> CONFIRM[人工确认执行]
    CONFIRM --> MONITOR[恢复监控<br/>指标回归检测]
    MONITOR --> POSTMORTEM[复盘报告自动生成]
```

---

## 5. 评测工程

### 5.1 基于工单的评测体系

```mermaid
graph TB
    subgraph 评测数据来源
        SRC1[历史工单<br/>已解决] --> DATASET[评测数据集]
        SRC2[人工构造<br/>边界 Case] --> DATASET
        SRC3[线上采样<br/>实时工单] --> DATASET
    end

    subgraph 评测流程
        DATASET --> REPLAY[工单回放<br/>智能体独立处理]
        REPLAY --> COMPARE[结果比对<br/>智能体 vs 人工]
        COMPARE --> SCORE[多维度评分]
        SCORE --> REPORT[评测报告生成]
    end

    subgraph 评分维度
        SCORE --> S1[准确性<br/>诊断是否正确]
        SCORE --> S2[完整性<br/>步骤是否遗漏]
        SCORE --> S3[安全性<br/>建议是否安全]
        SCORE --> S4[效率<br/>平均处理时长]
        SCORE --> S5[引用率<br/>强参考落实率]
    end
```

### 5.2 能力基线定义

每个产品线智能体必须通过以下能力基线认证，方可上线：

| 指标 | 基线要求 | 测量方式 | 不达标处理 |
|------|---------|---------|-----------|
| **诊断准确率** | >= 85% | 工单回放 + 专家盲评 | 阻断上线，补充语料 |
| **建议安全率** | >= 98% | 安全审查 + 高危操作检测 | 一票否决 |
| **知识引用率** | >= 90% | 强参考检测器 | 回退至强制引用模式 |
| **边界拒答率** | >= 95% | 超范围测试集 | 收紧意图分类器阈值 |
| **工单解决率** | >= 60% | 端到端工单闭环 | 分析 Gap，迭代语料 |
| **用户满意度** | >= 4.0/5.0 | 工单结束后评分 | 分析 Bad Case |
| **平均处理时长** | <= 人工的 50% | 工单时间戳对比 | 优化 Workflow 路径 |

### 5.3 评测流程 — 三级质检机制

```mermaid
graph LR
    subgraph 一级质检-自动化
        L1[LLM-as-Judge<br/>自动评分]
        L1A[格式合规检查]
        L1B[引用完整性检查]
        L1C[安全敏感词检测]
    end

    subgraph 二级质检-抽样
        L2[人工抽检<br/>20%采样率]
        L2A[专家标注]
        L2B[Bad Case 归因]
    end

    subgraph 三级质检-定期
        L3[能力基线回归<br/>月度测评]
        L3A[全量评测集回放]
        L3B[能力趋势报告]
    end

    L1 --> L2 --> L3
```

### 5.4 工单分级测评集

| 难度 | 描述 | 占比 | 示例 |
|------|------|------|------|
| **L1 - 基础** | 常见问题、标准配置查询 | 40% | "如何配置 ECS 安全组规则" |
| **L2 - 中等** | 典型故障诊断、需多步推理 | 35% | "Pod 处于 CrashLoopBackOff 且日志显示 OOM" |
| **L3 - 高难** | 复杂关联故障、跨组件排查 | 20% | "Service Mesh 下 gRPC 间歇性超时" |
| **L4 - 边界** | 超范围/模糊/对抗性问题 | 5% | "帮我写一个黑客工具" |

---

## 6. 能力提升工程

### 6.1 三阶段能力提升路线

```mermaid
graph LR
    subgraph 短期-基线达标-Q1-Q2
        P1[4K 语料建设]
        P2[QA 语料建设]
        P3[核心 Skills 开发]
        P4[标准 Workflow 搭建]
        P5[基线测评通过]
    end

    subgraph 中期-能力优化-Q3
        P6[语料动态更新机制]
        P7[Skills 自动发现]
        P8[Workflow 自适应优化]
        P9[多轮对话增强]
        P10[跨产品线关联诊断]
    end

    subgraph 长期-自主进化-Q4
        P11[工单反馈自动入库]
        P12[新故障模式自动学习]
        P13[Skills 自动编排]
        P14[知识图谱驱动推理]
        P15[人机协同进化闭环]
    end

    P1 --> P6 --> P11
    P2 --> P7 --> P12
    P3 --> P8 --> P13
    P4 --> P9 --> P14
    P5 --> P10 --> P15
```

### 6.2 短期目标: 基线达标（Q1-Q2）

**目标**: 五大产品线智能体全部通过能力基线认证。

```mermaid
graph TB
    subgraph Q1-语料与Skills
        Q1A[4K 知识语料建设<br/>各产品线 4000 条] --> Q1B[QA 语料建设<br/>总计 10000+ 对]
        Q1B --> Q1C[核心 Skills 开发<br/>诊断类 + 操作类 优先]
        Q1C --> Q1D[标准 Workflow 搭建<br/>咨询 + 诊断 优先]
    end

    subgraph Q2-测评与上线
        Q2A[评测数据集构建<br/>各产品线 500+ 用例] --> Q2B[基线测评执行<br/>五大产品线]
        Q2B --> |达标| Q2C[灰度上线<br/>10% 工单流量]
        Q2B --> |不达标| Q2D[Gap 分析<br/>语料补充]
        Q2D --> Q2A
        Q2C --> Q2E[灰度效果监控<br/>2 周观察期]
        Q2E --> Q2F[全量上线]
    end

    Q1D --> Q2A
```

**关键里程碑**:

| 里程碑 | 时间 | 交付物 | 验收标准 |
|-------|------|-------|---------|
| M1: 语料基础就绪 | Q1-W4 | 各产品线 4K + QA 语料初版 | 语料质检通过率 >= 90% |
| M2: Skills V1 完成 | Q1-W8 | 核心诊断/操作 Skills | Skill 单测通过率 >= 95% |
| M3: Workflow 联调 | Q2-W2 | 标准咨询 + 诊断 Workflow | 端到端流程可跑通 |
| M4: 基线认证 | Q2-W4 | 五大产品线基线评测报告 | 全部指标达标 |
| M5: 灰度上线 | Q2-W6 | 灰度流量开启 | 线上无 P0 事故 |
| M6: 全量上线 | Q2-W8 | 全量流量切换 | 用户满意度 >= 4.0 |

### 6.3 中期目标: 能力优化（Q3）

**目标**: 提升解决率到 75%+，支持跨产品线关联诊断。

| 方向 | 措施 | 预期收益 |
|------|------|---------|
| 语料动态更新 | 新工单自动抽取知识、语料版本月度迭代 | 知识时效性提升 |
| Skills 自动发现 | 分析工单 Gap 自动建议新 Skill | 覆盖率提升 20% |
| Workflow 自适应 | 根据诊断路径历史优化 Workflow 编排 | 处理效率提升 30% |
| 多轮对话增强 | 上下文记忆、追问引导、进度跟踪 | 用户体验显著提升 |
| 跨产品线关联 | 构建产品线间依赖关系图谱 | 复杂故障诊断能力突破 |

### 6.4 长期目标: 自主进化（Q4）

**目标**: 智能体具备自我学习和持续进化能力。

```mermaid
graph TB
    subgraph 自主进化闭环
        TICKET[运维工单] --> AGENT[智能体处理]
        AGENT --> FEEDBACK[用户反馈]
        FEEDBACK --> ANALYZE[反馈归因分析]
        ANALYZE --> |知识缺失| CORPUS[自动语料补充]
        ANALYZE --> |Skill 不足| SKILL_GEN[Skill 自动生成]
        ANALYZE --> |流程不优| WF_OPT[Workflow 自动优化]
        CORPUS --> VALIDATE[质检验证]
        SKILL_GEN --> VALIDATE
        WF_OPT --> VALIDATE
        VALIDATE --> DEPLOY[灰度部署]
        DEPLOY --> AGENT
    end
```

---

## 7. 产品线分板块规划

### 7.1 IaaS 智能体

```mermaid
graph TB
    subgraph IaaS智能体
        IAAS[IaaS 运维智能体]
        IAAS --> ECS[ECS 诊断]
        IAAS --> NET[网络诊断<br/>VPC/SLB/NAT]
        IAAS --> SEC[安全组诊断]
        IAAS --> DISK[磁盘/存储诊断]
    end

    subgraph IaaS语料
        ECS_K[ECS 知识 1200 条]
        NET_K[网络知识 1000 条]
        SEC_K[安全组知识 800 条]
        DISK_K[存储知识 1000 条]
    end

    subgraph IaaS-Skills
        ECS_S[ECS 重启/迁移/扩容]
        NET_S[连通性检测/路由分析]
        SEC_S[规则审计/冲突检测]
        DISK_S[IO 分析/空间清理]
    end

    ECS --> ECS_K --> ECS_S
    NET --> NET_K --> NET_S
    SEC --> SEC_K --> SEC_S
    DISK --> DISK_K --> DISK_S
```

### 7.2 PaaS 智能体

```mermaid
graph TB
    subgraph PaaS智能体
        PAAS[PaaS 运维智能体]
        PAAS --> MQ[消息队列诊断<br/>RocketMQ/Kafka]
        PAAS --> CACHE[缓存诊断<br/>Redis/Memcached]
        PAAS --> MW[中间件诊断<br/>Nginx/Envoy]
        PAAS --> APP[应用运行时诊断<br/>JVM/Node.js]
    end

    MQ --> MQ_K[MQ 知识 1000 条]
    CACHE --> CACHE_K[缓存知识 1000 条]
    MW --> MW_K[中间件知识 1000 条]
    APP --> APP_K[应用知识 1000 条]
```

### 7.3 数据库智能体

```mermaid
graph TB
    subgraph 数据库智能体
        DB[数据库运维智能体]
        DB --> RDS[关系型数据库<br/>MySQL/PostgreSQL]
        DB --> NOSQL[NoSQL 数据库<br/>MongoDB/DynamoDB]
        DB --> CACHE_DB[缓存数据库<br/>Redis Cluster]
        DB --> POLAR[云原生数据库<br/>PolarDB/TDSQL]
    end

    subgraph 数据库核心Skills
        RDS --> SLOW_SQL[慢 SQL 诊断 Skill]
        RDS --> LOCK[锁等待分析 Skill]
        RDS --> REPLI[主从复制诊断 Skill]
        NOSQL --> SHARD[分片诊断 Skill]
        CACHE_DB --> HOTKEY[热 Key 检测 Skill]
        POLAR --> SCALE[弹性扩缩 Skill]
    end
```

### 7.4 容器智能体

```mermaid
graph TB
    subgraph 容器智能体
        K8S[容器运维智能体]
        K8S --> POD[Pod 生命周期诊断]
        K8S --> NODE[Node 状态诊断]
        K8S --> SVC[Service/Ingress 诊断]
        K8S --> STORE[存储 PV/PVC 诊断]
        K8S --> CTRL[控制面诊断<br/>APIServer/etcd/Scheduler]
    end

    subgraph 容器Workflow
        POD --> POD_WF[Pod 故障诊断 Workflow<br/>CrashLoop/Pending/OOM]
        NODE --> NODE_WF[Node 故障诊断 Workflow<br/>NotReady/DiskPressure]
        SVC --> SVC_WF[Service 故障诊断 Workflow<br/>连通性/DNS/Ingress]
        STORE --> STORE_WF[存储故障诊断 Workflow<br/>挂载失败/容量不足]
        CTRL --> CTRL_WF[控制面故障诊断 Workflow<br/>API 超时/etcd 延迟]
    end
```

### 7.5 AI 智能体

```mermaid
graph TB
    subgraph AI智能体
        AIA[AI 运维智能体]
        AIA --> GPU[GPU 调度诊断<br/>分配/碎片/驱动]
        AIA --> TRAIN[训练任务诊断<br/>OOM/失败/慢训练]
        AIA --> INFER[推理服务诊断<br/>延迟/吞吐/模型加载]
        AIA --> DATA[数据管道诊断<br/>预处理/存储/管道]
    end

    GPU --> GPU_S[GPU Skill: nvidia-smi 分析/DCGM 指标]
    TRAIN --> TRAIN_S[训练 Skill: 分布式训练排障/资源分析]
    INFER --> INFER_S[推理 Skill: vLLM/TGI 调优/模型热加载]
    DATA --> DATA_S[数据 Skill: 数据管道监控/存储分析]
```

---

## 8. 财年整体时间线

```mermaid
graph LR
    subgraph FY整体规划
        Q1[Q1<br/>基础建设期] --> Q2[Q2<br/>基线达标期]
        Q2 --> Q3[Q3<br/>能力优化期]
        Q3 --> Q4[Q4<br/>自主进化期]
    end

    subgraph Q1交付
        Q1 --> Q1D1[4K 语料 V1]
        Q1 --> Q1D2[QA 语料 V1]
        Q1 --> Q1D3[核心 Skills V1]
        Q1 --> Q1D4[标准 Workflow V1]
        Q1 --> Q1D5[评测框架搭建]
    end

    subgraph Q2交付
        Q2 --> Q2D1[五产品线基线认证]
        Q2 --> Q2D2[灰度上线]
        Q2 --> Q2D3[全量上线]
        Q2 --> Q2D4[运营数据看板]
    end

    subgraph Q3交付
        Q3 --> Q3D1[语料动态更新机制]
        Q3 --> Q3D2[Skills 自动发现]
        Q3 --> Q3D3[跨产品线关联诊断]
        Q3 --> Q3D4[解决率 75%+]
    end

    subgraph Q4交付
        Q4 --> Q4D1[自主进化闭环]
        Q4 --> Q4D2[知识图谱推理]
        Q4 --> Q4D3[人机协同优化]
        Q4 --> Q4D4[解决率 85%+]
    end
```

---

## 9. 组织保障与资源规划

### 9.1 组织架构

| 角色 | 职责 | 人力需求 |
|------|------|---------|
| **项目 Owner** | 整体进度把控、跨团队协调 | 1 人 |
| **语料工程师** | 4K/QA 语料建设、质检、维护 | 5 人（每产品线 1 人） |
| **Skills 工程师** | Skills 开发、测试、上线 | 3 人 |
| **Workflow 工程师** | Workflow 设计、搭建、优化 | 2 人 |
| **评测工程师** | 评测数据集、评测平台、质量报告 | 2 人 |
| **平台工程师** | RAG 平台、Agent 框架、基础设施 | 3 人 |
| **各产品线 SME** | 知识审核、测评标注、Bad Case 分析 | 5 人（兼职） |

### 9.2 风险与应对

| 风险 | 影响 | 概率 | 应对措施 |
|------|------|------|---------|
| 语料建设进度滞后 | 基线认证推迟 | 中 | 引入外包语料标注、简化低频知识 |
| 产品线 SME 投入不足 | 语料质量下降 | 高 | 与各产品线 TL 签署 SLA、纳入 OKR |
| 评测标准争议 | 上线标准不一 | 中 | 提前对齐评测标准、成立评审委员会 |
| LLM 基座能力限制 | 复杂场景准确率不足 | 中 | 细化 Skills 粒度、增加人工兜底比例 |
| 用户信任建立慢 | 使用率低 | 高 | 灰度渐进、强调"参考建议"定位、显示信心值 |

---

## 10. 关键指标看板

### 10.1 运营看板指标

```mermaid
graph TB
    subgraph 运营指标看板
        KPI[可信智能体 KPI]
        KPI --> K1[工单接入率<br/>目标: 80%]
        KPI --> K2[自动解决率<br/>Q2:60% Q3:75% Q4:85%]
        KPI --> K3[诊断准确率<br/>目标: >= 85%]
        KPI --> K4[用户满意度<br/>目标: >= 4.0]
        KPI --> K5[知识引用率<br/>目标: >= 90%]
        KPI --> K6[安全事件数<br/>目标: 0]
        KPI --> K7[平均处理时长<br/>目标: 人工的 50%]
    end
```

### 10.2 各产品线达标跟踪

| 产品线 | 语料进度 | Skills 进度 | Workflow 进度 | 基线认证 | 上线状态 |
|-------|---------|-----------|-------------|---------|---------|
| IaaS | 🔲 0% | 🔲 0% | 🔲 0% | ⏳ 待评测 | 🔲 未上线 |
| PaaS | 🔲 0% | 🔲 0% | 🔲 0% | ⏳ 待评测 | 🔲 未上线 |
| 数据库 | 🔲 0% | 🔲 0% | 🔲 0% | ⏳ 待评测 | 🔲 未上线 |
| 容器 | 🔲 0% | 🔲 0% | 🔲 0% | ⏳ 待评测 | 🔲 未上线 |
| AI | 🔲 0% | 🔲 0% | 🔲 0% | ⏳ 待评测 | 🔲 未上线 |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [08 - Agent 评测体系与可观测性](./08-agent-evaluation-observability.md) | 评测方法论基础 |
| [04 - RAG 检索增强生成深度指南](./04-rag-knowledge-retrieval.md) | 语料检索技术方案 |
| [05 - Tool Use & Function Calling](./05-tool-use-function-calling.md) | Skills 技术实现基础 |
| [06 - 多 Agent 编排与协作架构](./06-multi-agent-orchestration.md) | Workflow 编排方案参考 |
| [09 - 生产部署指南](./09-production-deployment-guide.md) | 智能体部署方案 |
| [10 - 安全护栏与合规](./10-security-guardrails.md) | 安全质检标准参考 |
| [topic-fta](../topic-fta/) | 故障树分析（FTA）方法论 |
| [domain-12-troubleshooting](../domain-12-troubleshooting/) | 容器故障排障知识源 |

---

*本文档为可信智能体体系财年规划，由云平台运维工单专家组制定，以"强参考、强质检、窄口径"为核心原则，所有方案设计面向生产环境落地。*

---
title: "模板标题"
category: templates
tags: ["templates", "visibility/public"]
sources: ["auto-generated"]
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# {{组件名称}} 故障树分析 (FTA)

> **模板版本**: 2.0
> **最后更新**: 2026-05
> **文档类型**: FTA 故障树分析
> **适用版本**: Kubernetes v1.28 - v1.32

---

## YAML Front Matter

```yaml
---
fta_id: "FTA-{COMPONENT}-{SEQ}"
title: "{{组件名称}} 故障树分析"
component: "{{组件名称}}"
severity: "P{0-3}"
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
top_event_id: "TE-{序号}"
last_updated: "{{YYYY-MM}}"
authors:
  - name: "{{姓名}}"
    role: "{{角色}}"
reviewers: []
tags: [fta, troubleshooting, {{component}}]

# ---- 阅读体验增强字段 ----
difficulty: "intermediate"              # beginner | intermediate | advanced | expert
reading_level: "intermediate"          # beginner | intermediate | advanced | expert (同 difficulty)
audience: ["SRE", "Ops Engineer"]      # 目标读者
estimated_read_time: "20min"             # 预计阅读时间
prerequisites:                         # 前置知识依赖
  - "domain-01-cluster-fundamentals"
  - "basic-kubectl"

# ---- 统一 cross_refs ----
cross_refs:
  - type: "skill"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/{{NN}}-{{scenario}}.md"
    label: "{{技能名称}}"
  - type: "domain"
    path: "../domain-{{N}}-{{name}}/{{doc}}.md"
    label: "{{文档名称}}"
  - type: "structural"
    path: "../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/{{component}}-*.md"
    label: "结构化故障排查"

related_skills:
  - "../domain-10-troubleshooting-diagnostics/topic-skills/{{NN}}-{{scenario}}.md"
knowledge_refs:
  - "../domain-{{N}}-{{name}}/{{doc}}.md"
  - "../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/{{component}}-*.md"
---
```

---

## 顶事件 (Top Event)

**TE: {{组件名称}}服务异常/不可用**

### 顶事件定义模板

```yaml
top_event:
  id: "TE-{序号}"
  name: "{描述性名称}"
  severity: "P{0-3}"
  description: "{详细描述}"

  slo_mapping:
    indicator: "{SLI 名称}"
    target: "{SLO 目标值}"
    consequence: "{SLO 违约后果}"

  impact:
    users: "{受影响用户范围}"
    services: "{受影响服务列表}"
    business: "{业务影响描述}"

  response:
    sla: "{响应时间要求}"
    notification: "{通知方式和对象}"
    escalation: "{升级路径}"
```

### SLO 映射表

| SLO 指标 | 当前目标值 | 违约后果 | 影响用户 |
|:---|:---|:---|:---|
| {{SLI名称}} | {{目标值}} | {{后果}} | {{范围}} |

---

## 故障树结构

> 符号说明：
> - **OR 门**：任一子事件成立即可触发父事件
> - **AND 门**：所有子事件同时成立才触发父事件

```mermaid
graph TD
    TE[{{组件名称}}异常] --> OR0{{OR}}
    OR0 --> IE1[中间事件1]
    OR0 --> IE2[中间事件2]
    IE1 --> BE1[底事件1: {{具体问题}}]
    IE1 --> BE2[底事件2: {{具体问题}}]
    IE2 --> BE3[底事件3: {{具体问题}}]
    IE2 --> BE4[底事件4: {{具体问题}}]
    IE2 --> BE5[底事件5: {{具体问题}}]

    style TE fill:#ff6b6b,stroke:#c92a2a,color:#fff
    style OR0 fill:#f59e0b,stroke:#b45309,color:#fff
    style IE1 fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style IE2 fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style BE1 fill:#22c55e,stroke:#166534,color:#fff
    style BE2 fill:#22c55e,stroke:#166534,color:#fff
    style BE3 fill:#22c55e,stroke:#166534,color:#fff
    style BE4 fill:#22c55e,stroke:#166534,color:#fff
    style BE5 fill:#22c55e,stroke:#166534,color:#fff
```

---

## 底事件分析

### BE-1: {{问题名称}}

| 属性 | 详情 |
|:---|:---|
| **TE 编号** | TE-{{序号}} |
| **BE 编号** | BE-{{序号}}.1 |
| **问题现象** | {{问题现象描述}} |
| **根因** | {{根本原因}} |
| **影响** | {{对业务/系统的影响}} |
| **概率** | {{发生频率：高/中/低}} |

#### 诊断命令

```bash
# 步骤1：检查状态
{{诊断命令1}}

# 步骤2：查看日志
{{诊断命令2}}

# 步骤3：深入排查
{{诊断命令3}}

# 步骤4：关联分析
{{诊断命令4}}
```

#### 修复方案

| 风险等级 | 方案 | 操作 |
|:---:|:---|:---|
| 🟢 低风险 | {{安全修复方案}} | `{{命令}}` |
| 🟡 中风险 | {{需要评估的方案}} | `{{命令}}` |
| 🔴 高风险 | {{可能影响服务的方案}} | `{{命令}}` |

#### 预防措施

- {{预防措施1}}
- {{预防措施2}}
- {{预防措施3}}

#### Prometheus 告警规则

```yaml
groups:
- name: {{component}}-alerts
  rules:
  - alert: {{AlertName}}
    expr: {{PromQL表达式}}
    for: 5m
    labels:
      severity: {{critical/warning}}
    annotations:
      summary: "{{告警摘要}}"
      description: "{{详细描述}}"
      runbook_url: "{{链接到本文档}}"
```

#### 证据易失性

> 采集此底事件相关证据时应优先采集高优先级证据。

| 证据类型 | 易失性等级 | 采集窗口 | 存储要求 |
|:---|:---:|:---:|:---|
| 内存/运行时状态 | L1 | 问题发生后分钟级 | 加密存储 |
| 日志/Events | L5-L6 | 1h+ | 标准存储 |
| 审计记录 | L7 | 天-月级 | 长期归档 |

---

### BE-2: {{问题名称}}

> 按 BE-1 格式继续添加，每个底事件独立章节。

---

## 底事件 YAML 定义模板

> 以下模板用于以结构化 YAML 格式定义底事件，便于工具解析和自动化处理。

```yaml
basic_event:
  id: "BE-{顶事件序号}.{底事件序号}"
  name: "{描述性名称}"
  description: "{详细描述}"

  observability:
    metrics:
      - expression: "{PromQL 表达式}"
        threshold: "{阈值}"
        severity: "{告警级别}"
    logs:
      - pattern: "{日志匹配模式}"
        component: "{来源组件}"
    events:
      - type: "{K8s Event 类型}"
        reason: "{Event Reason}"

  probability:
    annual_rate: {年问题率}
    mttr_minutes: {平均修复时间}
    data_source: "{数据来源}"

  root_causes:
    - "{可能原因 1}"
    - "{可能原因 2}"

  diagnosis_commands:
    - "{诊断命令 1}"
    - "{诊断命令 2}"

  healing_actions:
    - id: "HA-{关联底事件}.{序号}"
      name: "{动作名称}"
      risk_level: "{low|medium|high}"
      auto_healable: {true|false}
      command: "{执行命令}"
      verification: "{验证命令}"
```

---

## 问题统计

> 汇总所有底事件的问题频率、影响度和 MTTR，用于优先级排序和资源分配。

| 底事件 | 问题类型 | 频率 | 影响度 | MTTR | 优先修复 |
|:---|:---|:---:|:---:|:---:|:---:|
| BE-1 | {{类型}} | 高/中/低 | 高/中/低 | {{时间}} | ✅/❌ |
| BE-2 | {{类型}} | 高/中/低 | 高/中/低 | {{时间}} | ✅/❌ |
| BE-3 | {{类型}} | 高/中/低 | 高/中/低 | {{时间}} | ✅/❌ |

---

## 问题统计图

```mermaid
pie title 底事件频率分布
    "BE-1: {{问题名称}}" : {{比例}}
    "BE-2: {{问题名称}}" : {{比例}}
    "BE-3: {{问题名称}}" : {{比例}}
    "其他" : {{比例}}
```

---

## FTA 评审检查表

> 完成 FTA 文档后，必须通过以下检查项。每一项都必须满足。

### 结构完整性

- [ ] 顶事件定义清晰，与 SLO 关联
- [ ] 所有中间事件都有子事件
- [ ] 所有底事件都是叶子节点（无可继续分解的下游事件）
- [ ] 没有悬挂的孤立事件
- [ ] 没有循环依赖

### 逻辑正确性

- [ ] 逻辑门类型选择正确（OR vs AND）
- [ ] 同一门下的子事件满足 MECE 原则（相互独立，完全穷尽）
- [ ] 同一门下的子事件满足独立性原则（不相互包含）
- [ ] 层数在 3-5 层之间（过深影响可维护性，过浅表示分解不充分）

### 可观测性

- [ ] 每个底事件至少有 1 个指标监控
- [ ] 每个底事件至少有 1 种诊断命令
- [ ] 每个底事件有明确的判定条件
- [ ] 告警规则与 FTA 事件正确关联
- [ ] 告警规则的 PromQL 表达式经过实测验证

### 可维护性

- [ ] 编号遵循规范（TE-/IE-/BE- 前缀）
- [ ] 命名专业准确，无歧义
- [ ] 每个子树有明确的 Owner
- [ ] 概率数据有标注来源
- [ ] 修复动作有风险分级（🟢/🟡/🔴）
- [ ] 修复操作包含回滚方案

### Agent 友好性

- [ ] 每个底事件有结构化的修复动作
- [ ] 修复动作标注了自动化程度（L1/L2/L3）
- [ ] 高风险操作有审批标记
- [ ] 验证条件可自动化判定
- [ ] 诊断步骤可脚本化（提供 bash 示例）

### 文档质量

- [ ] 所有 `{{PLACEHOLDER}}` 均已替换为实际内容
- [ ] Mermaid 图可正常渲染
- [ ] 代码块语法正确（bash/yaml/json）
- [ ] 章节编号连续，无遗漏
- [ ] 文档长度适中（单个底事件不超过 200 行为佳）

---

## 相关文档

| 类型 | 文档 | 说明 |
|:---|:---|:---|
| 深度排查 | [../domain-{{N}}-{{name}}/{{doc}}.md](../domain-{{N}}-{{name}}/{{doc}}.md) | 完整故障排查指南 |
| Skill | [../domain-10-troubleshooting-diagnostics/topic-skills/{{NN}}-{{scenario}}.md](../domain-10-troubleshooting-diagnostics/topic-skills/{{NN}}-{{scenario}}.md) | 自动化修复技能 |
| FEBM | [../domain-10-troubleshooting-diagnostics/topic-febm/{{NN}}-{{scenario}}.md](../domain-10-troubleshooting-diagnostics/topic-febm/{{NN}}-{{scenario}}.md) | 取证分析方法 |
| 速查卡 | [../domain-17-system-foundation/topic-cheat-sheet/k8s.md](../domain-17-system-foundation/topic-cheat-sheet/k8s.md) | 命令速查 |
| 学习计划 | [../domain-11-production-operations/topic-learn/{{path}}/README.md](../domain-11-production-operations/topic-learn/{{path}}/README.md) | 相关学习路径 |

---

## 版本历史

| 日期 | 版本 | 变更 | 作者 |
|:---:|:---:|:---|:---:|
| YYYY-MM | v1.0 | 初始版本 | {{姓名}} |
| YYYY-MM | v2.0 | {{变更描述}} | {{姓名}} |

---

> **关联文档**: [domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md](../domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md)（FTA 方法论与 AI Agent 智能运维实践）

## Related

- Wiki Lint Report — 2026-05-21 — Cross-reference

# Skill 运维技能文档模板

> 本模板为简化版。完整 Skill Schema 参见 [topic-skills/skill-schema.md](../topic-skills/skill-schema.md)

---

## Section 1: 故障概述

**Skill 名称**: {{XX-故障名称}}
**分类**: {{类别，如 Node/Pod/Network/Storage/Security/Observability/Scaling}}

### 故障现象
{{故障现象列表，≥10 个症状}}

### 影响范围
{{对业务和系统的影响描述}}

---

## Section 2: 快速诊断

```bash
# 一键诊断脚本
{{诊断命令}}
```

---

## Section 3: 根因分析

| # | 根因 | 概率 | 验证命令 |
|:---:|:---|:---:|:---|
| 1 | {{根因1}} | 高 | `{{命令}}` |
| 2 | {{根因2}} | 中 | `{{命令}}` |

---

## Section 4: 修复方案

### 方案1: {{低风险修复}}

```bash
{{修复命令}}
```

### 方案2: {{中风险修复}}

```bash
{{修复命令}}
```

---

## Section 5: 预防措施

- {{预防措施1}}
- {{预防措施2}}

---

> 完整 Skill 应包含 Section 1-12，详见 [skill-schema.md](../topic-skills/skill-schema.md)

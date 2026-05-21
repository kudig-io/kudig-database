#!/usr/bin/env python3
"""
Skill 批量生成器 —— 从 FTA/Structural 文档快速派生新 Skill

用法:
  python3 skill-generator.py --id SKILL-HELM-001 \
    --name "Helm Chart 部署与回滚故障" \
    --source-fta skills/helm-fta.md \
    --source-structural domain-10-troubleshooting-diagnostics/36-helm-chart-troubleshooting.md \
    --output domain-10-troubleshooting-diagnostics/topic-skills/26-helm-chart-failure.md

前置条件:
  - source-fta 和 source-structural 至少提供一个
  - 模板文件 templates/skill-template.md 必须存在
"""

import argparse
import os
import re
import sys
from datetime import datetime
from pathlib import Path


def parse_fta(fta_path: str) -> dict:
    """从 FTA 文档中提取关键信息"""
    result = {
        "symptoms": [],
        "quick_checks": [],
        "diagnosis_phases": [],
        "remediation": [],
        "risks": [],
        "related_docs": []
    }
    if not fta_path or not os.path.exists(fta_path):
        return result

    with open(fta_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 提取症状: 顶事件描述部分
    te_match = re.search(r'##?\s*顶事件.*\n(.*?)(?=##?\s*|$)', content, re.DOTALL)
    if te_match:
        result["symptoms"].append(te_match.group(1).strip()[:500])

    # 提取排查命令: 代码块中带 kubectl/describe/get 的行
    code_blocks = re.findall(r'```(?:bash|shell)?\n(.*?)```', content, re.DOTALL)
    for block in code_blocks:
        lines = [l.strip() for l in block.split('\n') if l.strip().startswith(('kubectl', 'helm', 'describe', 'get ', 'check'))]
        result["quick_checks"].extend(lines[:8])

    # 提取修复动作: HA- 开头的行
    ha_lines = re.findall(r'(HA-\d+\.\d+.*)', content)
    result["remediation"] = ha_lines[:10]

    # 提取风险关键词
    risk_keywords = ['高风险', '⚠️', '强制', 'delete', 'cordon', 'drain']
    for kw in risk_keywords:
        if kw in content:
            result["risks"].append(kw)

    return result


def parse_structural(structural_path: str) -> dict:
    """从 Structural 排障文档中提取步骤"""
    result = {
        "phase1": [],
        "phase2": [],
        "phase3": [],
        "verification": []
    }
    if not structural_path or not os.path.exists(structural_path):
        return result

    with open(structural_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 简单启发式提取各 Phase
    sections = re.split(r'##\s*\d+\.', content)
    for i, sec in enumerate(sections):
        lines = [l.strip() for l in sec.split('\n') if l.strip().startswith(('- ', '1. ', '```'))]
        if i == 1:
            result["phase1"] = lines[:10]
        elif i == 2:
            result["phase2"] = lines[:10]
        elif i == 3:
            result["phase3"] = lines[:10]
        elif '验证' in sec or 'verification' in sec.lower():
            result["verification"] = lines[:5]

    return result


def generate_skill(args) -> str:
    """生成 Skill Markdown 内容"""
    fta_data = parse_fta(args.source_fta)
    struct_data = parse_structural(args.source_structural)

    skill_num = args.skill_number or "XX"
    skill_id = args.skill_id or f"SKILL-{skill_num}"
    severity = args.severity or "P2"
    execution_mode = args.execution_mode or "L1"

    # 构建 frontmatter
    frontmatter = f"""---
title: {args.name}
description: Skill — {args.name}
category: skills
tags:
- k8s
- skills
- sop
- runbook
{chr(10).join(f'- {tag}' for tag in (args.tags or '').split(',') if tag)}
last_updated: '{datetime.now().strftime('%Y-%m-%d')}'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 15min
intent_queries:
- {args.name} 是什么
- 如何 {args.name}
trigger_keywords:
{chr(10).join(f'- {kw}' for kw in (args.keywords or args.name).split(','))}
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: {execution_mode}
skill_metadata:
  skill_id: {skill_id}
  category: {args.category or 'general'}
  severity: {severity}
  time_to_diagnosis_minutes: 10
  time_to_remediation_minutes: 30
  escalation_required: true
---

# {args.name}

> **Skill ID**: `{skill_id}`  
> **严重级别**: {severity}  
> **执行模式**: {execution_mode}  
> **来源**: FTA + Structural 文档派生

---

## 1. 概述

{args.name} 是 Kubernetes 生产环境中 **{severity} 级故障**。

**典型触发条件**:
{chr(10).join(f'- {s}' for s in (fta_data['symptoms'] or ['[请补充典型症状]']))}

**爆炸半径评估**:
- 影响范围: [请补充]
- 恢复时间目标 (RTO): [请补充]
- 是否需要人工审批: {'是' if execution_mode in ('L2', 'L3') else '视修复动作而定'}

---

## 2. 症状识别

| 症状模式 | 置信度 | 检查命令 |
|---------|--------|---------|
| [症状1] | 0.90 | `kubectl ...` |
| [症状2] | 0.75 | `kubectl ...` |

---

## 3. 快速检查（< 2 分钟）

```bash
# 1. 确认故障范围
{chr(10).join(f'# {cmd}' for cmd in (fta_data['quick_checks'][:5] or ['[请补充快速检查命令]']))}
```

**决策点**:
- 如果 [条件A] → 跳转到 Phase 2-A
- 如果 [条件B] → 跳转到 Phase 2-B
- 如果都不匹配 → 升级到 L2 人工诊断

---

## 4. Phase 1: 信息收集（2-5 分钟）

```bash
# 收集基础信息
{chr(10).join(f'# {cmd}' for cmd in (struct_data['phase1'][:5] or ['[请补充]']))}
```

---

## 5. Phase 2: 根因定位（5-10 分钟）

| 根因假设 | 验证命令 | 验证标准 |
|---------|---------|---------|
| [假设1] | `...` | [标准] |
| [假设2] | `...` | [标准] |

---

## 6. Phase 3: 修复操作

### 🟢 低风险（L0-L1，可自动执行）
| 动作 | 命令 | 预期结果 |
|------|------|---------|
| [动作1] | `...` | [结果] |

### 🟡 中风险（L2，需确认后执行）
| 动作 | 命令 | 风险说明 | 确认方式 |
|------|------|---------|---------|
| [动作2] | `...` | [风险] | [确认] |

### 🔴 高风险（L3，必须人工审批）
| 动作 | 命令 | 风险说明 | 审批人 |
|------|------|---------|--------|
| [动作3] | `...` | [风险] | [审批人] |

---

## 7. 验证

```bash
# 验证修复结果
{chr(10).join(f'# {cmd}' for cmd in (struct_data['verification'][:3] or ['[请补充验证命令]']))}
```

**验证标准**:
- [ ] 指标恢复正常
- [ ] Pod/节点状态正常
- [ ] 业务流量恢复

---

## 8. 回滚方案

如果修复后问题加剧，执行以下回滚：

```bash
# 回滚命令
[请补充]
```

---

## 9. 升级路径

如果以下情况发生，立即升级：
- [条件1] → 升级到 [Team/On-Call]
- [条件2] → 升级到 [架构师/厂商支持]

---

## 10. 相关链接

- FTA 故障树: [{args.source_fta or 'N/A'}]
- Structural 排障指南: [{args.source_structural or 'N/A'}]
- 相关 Skill: [链接]

---

*本 Skill 由 skill-generator.py 从已有文档自动生成，需人工补充 [请补充] 标记的内容后达到 GA 状态。*
"""
    return frontmatter


def main():
    parser = argparse.ArgumentParser(description='从 FTA/Structural 文档生成 Skill')
    parser.add_argument('--id', '--skill-id', dest='skill_id', required=True, help='Skill ID, e.g. SKILL-HELM-001')
    parser.add_argument('--name', required=True, help='Skill 名称')
    parser.add_argument('--number', dest='skill_number', help='Skill 编号, e.g. 26')
    parser.add_argument('--category', default='general', help='分类')
    parser.add_argument('--severity', default='P2', choices=['P0', 'P1', 'P2', 'P3'], help='严重级别')
    parser.add_argument('--mode', dest='execution_mode', default='L1', choices=['L0', 'L1', 'L2', 'L3'], help='执行模式')
    parser.add_argument('--source-fta', help='来源 FTA 文档路径')
    parser.add_argument('--source-structural', help='来源 Structural 文档路径')
    parser.add_argument('--tags', help='额外标签，逗号分隔')
    parser.add_argument('--keywords', help='触发关键词，逗号分隔')
    parser.add_argument('--output', '-o', required=True, help='输出文件路径')

    args = parser.parse_args()

    content = generate_skill(args)

    # 确保目录存在
    os.makedirs(os.path.dirname(args.output), exist_ok=True)

    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"✅ Skill 已生成: {args.output}")
    print(f"   名称: {args.name}")
    print(f"   ID: {args.skill_id}")
    print(f"   严重级别: {args.severity}")
    print(f"   执行模式: {args.execution_mode}")
    print(f"\n⚠️  注意: 文件中标记 [请补充] 的部分需要人工审核和填写")
    print(f"   建议下一步: 用模板对照现有 GA Skill (如 01-node-notready.md) 补齐内容")


if __name__ == '__main__':
    main()

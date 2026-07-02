---
title: QA Action 扩展执行摘要（2026-06-26）
description: 将 QA action 字段扩展至全部命令输出诊断语料文件的执行摘要
summary: 将 QA action 字段扩展至全部命令输出诊断语料文件的执行摘要
category: reports
tags:
- ticket-agent
- corpus
- qa-action
- command-output-diagnosis
- quality
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
status: completed
relationships:
- target: _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
  type: related_to
- target: _reports/ticket-agent-corpus-execution-summary-2026-06-26.md
  type: related_to
- target: _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# QA Action 扩展执行摘要（2026-06-26）

> **执行目标**：将 QA action 字段扩展至项目内全部命令输出诊断语料文件  
> **执行原则**：统一 action 格式为结构化对象、保持非破坏性、确保 YAML 可解析  
> **执行结果**：扫描 5 个文件，1,456 个 I-O 对，全部完成 action 填充/结构化转换

---

## 1. 执行概览

| 维度 | 成果 |
|---|---|
| 扫描文件数 | 5 个 |
| 扫描 I-O 对总数 | 1,456 对 |
| 新填充 action | 1,401 对 |
| 字符串 action 结构化转换 | 49 对 |
| 已存在结构化 action 跳过 | 6 对（第一轮） |
| 输出 *_with_actions.md 文件 | 5 个 |
| YAML 解析错误 | 0 |

---

## 2. 处理的文件清单

| 输入文件 | I-O 对数 | 处理方式 | 输出文件 |
|---|---|---|---|
| `generated/command-output-diagnosis-p0.md` | 469 | 推断填充 | `generated/command-output-diagnosis-p0.with_actions.md` |
| `generated/command-output-diagnosis-p1.md` | 469 | 推断填充 | `generated/command-output-diagnosis-p1.with_actions.md` |
| `generated/command-output-diagnosis-p2.md` | 469 | 推断填充 | `generated/command-output-diagnosis-p2.with_actions.md` |
| `command-output-diagnosis.md` | 23 | 字符串 action → 结构化对象 | `command-output-diagnosis.with_actions.md` |
| `seed/p0-core-scenarios.md` | 26 | 字符串 action → 结构化对象 | `seed/p0-core-scenarios.with_actions.md` |
| **合计** | **1,456** | — | **5 个** |

---

## 3. Action 格式统一

### 3.1 统一后的结构化格式

```yaml
action:
- command: "kubectl logs <pod-name> -n <namespace> --previous"
  description: "查看异常 Pod 的上一次容器日志"
  risk_level: low
- command: "kubectl rollout restart deployment/<name> -n <namespace>"
  description: "重启 Deployment"
  risk_level: medium

```

### 3.2 risk_level 判定规则

| 风险等级 | 触发关键词 |
|---|---|
| low | 查询、查看、describe、get、logs、auth can-i |
| medium | restart、rollout、patch、apply、scale、prune |
| high | delete、drain、cordon、evict、force、systemctl stop |
| critical | 未显式定义，保留给未来手动标注 |

---

## 4. Action 推断规则覆盖

`scripts/fill_qa_actions.py` 已扩展至覆盖以下场景：

| 场景类别 | 代表动作 |
|---|---|
| Pod 异常 | 查看日志、describe pod、调整资源限制 |
| Node 异常 | describe node、查看事件、识别资源消耗大户、cordon/drain |
| 证书/TLS | 检查证书有效期、重启 kubelet |
| DNS | 重启 CoreDNS、重新应用配置 |
| NetworkPolicy | 查看规则、临时删除恢复业务 |
| PVC/存储 | 查看 PVC/PV、查看挂载失败事件 |
| SLB/负载均衡 | 查询 SLB 属性、查询虚拟服务器组 |
| etcd | endpoint status、endpoint health、defrag |
| Deployment | rollout status、rollout undo |
| HPA | 查看 HPA 配置、重启 metrics-server |
| RBAC | auth can-i、创建 rolebinding |
| ConfigMap/Secret | 查看内容、重启应用生效 |
| 兜底 | 复现诊断命令确认状态 |

---

## 5. 质量验证

### 5.1 YAML 解析验证

对全部 5 个输出文件执行 YAML 解析验证：

| 输出文件 | YAML 块数 | 解析成功 | 结构化 action | 字符串 action | 错误 |
|---|---|---|---|---|---|
| `command-output-diagnosis-p0.with_actions.md` | 469 | 469 | 469 | 0 | 0 |
| `command-output-diagnosis-p1.with_actions.md` | 469 | 469 | 469 | 0 | 0 |
| `command-output-diagnosis-p2.with_actions.md` | 469 | 469 | 469 | 0 | 0 |
| `command-output-diagnosis.with_actions.md` | 23 | 23 | 23 | 0 | 0 |
| `seed/p0-core-scenarios.with_actions.md` | 26 | 26 | 26 | 0 | 0 |

### 5.2 未处理文件说明

以下文件虽包含相关字段，但未纳入本次处理：

| 文件 | 原因 |
|---|---|
| `skills/ts-command-output.md` | YAML 块存在语法错误（中文引号），非标准 I-O 对格式 |
| `_archives/troubleshooting-diagnostics/topic-qa-corpus/command-output-diagnosis.md` | 归档文件，按项目规范排除 |
| `domain-14-ai-ml-infra/01-ai-infra/15-llm-data-pipeline.md` | 不包含标准 I-O 对 YAML 块 |

---

## 6. 关键脚本更新

`scripts/fill_qa_actions.py` 本次升级内容：

1. **支持字符串 action 转结构化对象**：自动识别 `- kubectl logs ... # 查看日志` 格式并拆分 command/description/risk_level
2. **扩展推断规则库**：从 10+ 条增至 20+ 条，覆盖更多故障场景
3. **支持多源文件**：同时处理 generated、command-output-diagnosis、seed 三类语料
4. **增强 risk_level 判定**：基于命令关键词自动判定风险等级
5. **保留已有结构化 action**：避免重复处理，防止覆盖人工精心编写的 action

---

## 7. 对语料配置的更新建议

在 `_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml` 中，建议将 `*_with_actions.md` 文件加入高优先级摄入列表：

```yaml
qa_corpus:
  - path: domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/*.with_actions.md
    priority: critical
    chunking: by_yaml_block
  - path: domain-10-troubleshooting-diagnostics/topic-qa-corpus/command-output-diagnosis.with_actions.md
    priority: high
    chunking: by_yaml_block
  - path: domain-10-troubleshooting-diagnostics/topic-qa-corpus/seed/p0-core-scenarios.with_actions.md
    priority: high
    chunking: by_yaml_block
```

（注：当前 profile 尚未显式包含 QA 语料，建议后续补充。）

---

## 8. 后续建议

1. **Action 质量人工抽检**：从 1,456 个 action 中随机抽取 50-100 个，验证其准确性和可执行性
2. **补充高风险 action 的 rollback 字段**：为 high/critical 级别的 action 增加 `rollback` 子字段
3. **扩展至其他语料源**：检查 `skills/`、`domain-*/topic-structural-trouble-shooting/` 中是否还有可结构化的诊断语料
4. **建立 action 禁用清单**：与安全红线（SOUL.md）联动，自动拦截禁止执行的命令

---

## 9. 相关文件

- `scripts/fill_qa_actions.py` — QA action 自动化填充脚本
- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/*.with_actions.md` — 带 action 的批量 QA 语料
- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/command-output-diagnosis.with_actions.md` — 核心命令输出语料
- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/seed/p0-core-scenarios.with_actions.md` — P0 种子语料
- `_reports/ticket-agent-corpus-round2-summary-2026-06-26.md` — 上一轮执行摘要
- `_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md` — 完整改进规划

---

*本摘要记录 2026-06-26 执行的 QA action 扩展成果。*

## Related

- _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
- _reports/ticket-agent-corpus-execution-summary-2026-06-26.md
- _reports/ticket-agent-corpus-round2-summary-2026-06-26.md

```

<!-- risk-assessed -->

---
title: Skill Template
summary: 每个 Skill 文档以 YAML front matter 开头，用 --- 包围。所有字段均为必填。
category: general
tags:
- skill-template
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Skill 运维技能文档模板

> **模板版本**: 2.0
> **最后更新**: 2026-05
> **关联 Schema**: 本模板是 Skill 文档的完整规范，所有新建 Skill 必须严格遵循此结构

---

## YAML Front Matter 规范

每个 Skill 文档以 YAML front matter 开头，用 `---` 包围。**所有字段均为必填**。

```yaml
---
# === 基本信息 ===
skill_id: "SKILL-{CATEGORY}-{SEQ}"   # 唯一标识符
                                       # CATEGORY: NODE | POD | NET | STORE | CP | SEC | WORK | SCALE
                                       # SEQ: 三位数字，如 001
skill_name: "中文名称 / English Name"  # 双语名称，斜杠分隔
version: "1.0"                         # 文档版本，语义化版本
category: "node"                       # 分类枚举: node | pod | network | storage |
                                       #          control-plane | security | workload | scaling
severity_range: "P0-P2"               # 此 Skill 覆盖的严重性范围
k8s_versions:                          # 兼容的 [[实体/kubernetes.md|kubernetes]] 版本
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "5-30min"   # 预计修复时间范围
risk_level: "high"                     # 修复操作的整体风险: low | medium | high | critical
agent_execution_mode: "L1-advisory"    # 推荐的 Agent 自动化级别
                                       # L1-advisory:   仅建议，人工执行
                                       # L2-semi-auto:  低风险自动，其余审批
                                       # L3-full-auto:  中风险以下自动，高风险审批

# === 路由匹配 ===
trigger_keywords:                      # NLP 匹配关键词（中英文）
  - "NotReady"
  - "节点不可用"
trigger_events:                        # Kubernetes Event Reason
  - "NodeNotReady"
trigger_metrics:                       # Prometheus 指标模式
  - 'kube_node_status_condition{condition="Ready",status="false"}'

# === 阅读体验增强字段 ===
difficulty: "intermediate"             # beginner | intermediate | advanced | expert
reading_level: "intermediate"          # beginner | intermediate | advanced | expert (同 difficulty)
audience: ["SRE", "Ops Engineer"]      # 目标读者: SRE / DevOps / Developer
estimated_read_time: "10min"            # 预计阅读时间
prerequisites:                         # 前置知识依赖
  - "故障诊断"
  - "kubectl-basics"

# === 关联引用 ===
related_skills:                        # 关联的其他 Skill ID
  - "SKILL-POD-001"
fta_refs:                              # 对应的 FTA 文件
  - "故障诊断/topic-fta/list/node-fta.md"
knowledge_refs:                        # 深度知识参考
  - "故障诊断/topic-structural-trouble-shooting/node-*.md"
  - "故障诊断/"

# === 统一 cross_refs ===
cross_refs:
  - type: "fta"
    path: "../故障诊断/topic-fta/list/node-fta.md"
    label: "Node 故障树分析"
  - type: "domain"
    path: "../故障诊断/02-node-notready-troubleshooting.md"
    label: "Node NotReady 深度诊断"
---
```

---

## Skill 分类体系

| 类别 | 前缀 | 说明 | 示例 |
|------|------|------|------|
| Node | SKILL-NODE-xxx | 节点级问题 | Node NotReady |
| Pod | SKILL-POD-xxx | Pod 生命周期问题 | CrashLoop、Pending |
| Network | SKILL-NET-xxx | 网络与连通性问题 | DNS、Service、Ingress |
| Storage | SKILL-STORE-xxx | 存储与持久化问题 | PVC、CSI |
| Security | SKILL-SEC-xxx | 安全与权限问题 | RBAC、Certificate、Incident |
| Workload | SKILL-WORK-xxx | 工作负载管理问题 | Deployment Rollout |
| Image | SKILL-IMAGE-xxx | 镜像管理问题 | ImagePull |
| ControlPlane | SKILL-CP-xxx | 控制平面问题 | etcd、API Server |
| Scaling | SKILL-SCALE-xxx | 弹性伸缩问题 | HPA、VPA、CA |
| Configuration | SKILL-CONFIG-xxx | 配置管理问题 | ConfigMap、Secret |
| Observability | SKILL-MONITOR-xxx / SKILL-LOG-xxx | 可观测性问题 | Prometheus、日志 |
| Performance | SKILL-PERF-xxx | 性能瓶颈 | CPU/Memory/IO |

---

## Section 1: 概述

## 1. 概述

简要说明此 Skill 的覆盖范围、典型场景和使用前提。内容控制在 5-10 行以内。

包含：
- 此 Skill 解决什么问题
- 典型触发场景（1-3 个）
- 前置条件（需要的权限、工具）

---

## Section 2: 症状识别

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 描述（中文） / Description (English) | `kubectl get ...` 或指标/日志模式 | 0.0-1.0 | 何时不应匹配此 Skill |
| S2 | ... | ... | ... | ... |

### 2.2 工单关键词映射

中英文常见工单描述示例，帮助 Agent 进行 NLP 意图匹配：
- "节点状态异常，显示 NotReady"
- "Node is not ready, pods being evicted"

### 2.3 排除标准

明确列出此 Skill **不适用** 的场景：
- 条件 A → 应使用 SKILL-XXX-NNN
- 条件 B → 不属于此 Skill 范围

**编写要求**:
- 症状模式表至少包含 5 个症状
- 置信度基于该症状对此 Skill 的特异性（0.95 = 几乎确定，0.5 = 可能）
- 排除条件必须明确指向正确的 Skill 或说明不在范围内

---

## Section 3: 快速分级

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: [目的]
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl ...
```
> **判断规则**: 如果输出中... → 影响范围为...

**Step T2**: [目的]
...

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| [条件A] | P0 | [说明] |
| [条件B] | P1 | [说明] |
| [条件C] | P2 | [说明] |
| 其他 | P3 | [说明] |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工**：
- 条件 1
- 条件 2

**编写要求**:
- 影响评估命令不超过 5 条
- 所有命令均为只读操作
- 严重性分级必须包含 P0-P3 四级

---

## Section 4: 诊断工作流

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: [目的]
- **命令**:
  ```bash
  kubectl get ...
  ```
- **超时**: 10s
- **预期输出模式**: `pattern` 或关键字
- **判断规则**:
  - 如果输出匹配 `X` → 根因为 RC-001，跳转 Section 5
  - 如果输出匹配 `Y` → 继续 Step D1.2
  - 如果无异常 → 继续 Step D1.2
- **版本差异**: **[v1.30+]** 新增字段 `xxx`，需额外检查...

**Step D1.2**: [目的]
...

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: [目的]
...

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: [目的]
...

**编写要求**:
- 每个 Step 必须有唯一 ID（格式: `D{Phase}.{Seq}`）
- 必须标注超时时间
- 判断规则使用明确的条件 → 动作格式
- 版本差异使用 `**[vX.XX+]**` 或 `**[vX.XX-vX.XX]**` 标记
- Phase 3 的每个步骤必须标注风险级别

---

## Section 5: 根因分类

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | 根因描述 | 高/中/低 | 需要的证据（Step ID） | FTA 底事件 ID |
| RC-002 | ... | ... | ... | ... |

**编写要求**:
- 至少覆盖 8 个根因
- 概率分三级：高（>30% 工单为此根因）、中（10-30%）、低（<10%）
- 诊断证据引用诊断工作流中的 Step ID
- FTA 映射引用 故障诊断/topic-fta/list/ 中对应的底事件

---

## Section 6: 修复操作

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: [操作名称]
- **适用根因**: RC-XXX
- **前置检查**:
  ```bash
  kubectl ...  # 确认条件
  ```
- **执行命令**:
  ```bash
  kubectl ...
  ```
- **后置验证**:
  ```bash
  kubectl ...  # 预期: ...
  ```
- **回滚命令**:
  ```bash
  kubectl ...
  ```

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-002: [操作名称]
- **适用根因**: RC-XXX
- **影响说明**: [此操作的影响范围和风险]
- **审批提示**: "建议执行 [操作]，影响范围 [X]，是否批准？"
- **前置检查**: ...
- **执行命令**: ...
- **后置验证**: ...
- **回滚命令**: ...

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-003: [操作名称]
- **适用根因**: RC-XXX
- **影响说明**: [此操作的影响范围和风险]
- **操作步骤**:
  1. [步骤1]
  2. [步骤2]
- **安全检查**: ...
- **回滚方案**: ...

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-004: [操作名称]
- **适用根因**: RC-XXX
- **审批要求**: [需要的审批级别]
- **数据备份**: [备份要求]
- **操作步骤**: ...
- **回滚方案**: ...

**编写要求**:
- 每个修复操作必须有唯一 ID（格式: `REM-{SEQ}`）
- 必须关联到具体根因 (RC-XXX)
- 所有操作必须包含回滚命令或回滚方案
- 🟢 低风险操作必须包含前置检查和后置验证的完整命令
- 🟡🔴⚫ 操作必须包含影响说明

---

## Section 7: 验证确认

## 7. 验证确认

### 7.1 即时验证（修复后 1 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: [验证项]
kubectl ...
# 预期: ...

# V2: [验证项]
kubectl ...
# 预期: ...
```
### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| [项目] | `kubectl ...` 或 PromQL | [预期] | [异常] |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：
- [ ] 条件 1
- [ ] 条件 2
- [ ] 条件 3

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| [项目] | [方法] | [频率] | [行动] |

---

## Section 8: 升级协议

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 X 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 严重性升级 | 初始分级为 P2 但影响面扩大到 P0 级别 |
| 未知根因 | 诊断完成但无法匹配任何已知根因 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 问题概述: {summary}
- 影响范围: {impact}
- 已完成诊断: {completed_steps}
- 初步发现: {findings}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. 已排除的根因及原因
3. 可能的根因假设
4. 相关资源的 YAML 快照
5. 最近 30 分钟的关键事件时间线

---

## Section 9: 版本兼容矩阵

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| [功能1] | [状态] | [状态] | [状态] | [状态] | [状态] |
| [功能2] | ... | ... | ... | ... | ... |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| [命令1] | [语法] | [语法] | [语法] | [语法] | [语法] |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| [资源] | [API版本] | ... | ... | ... | ... |

---

## Section 10: 知识进化

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| [场景] | [现象] | [根因] | [方法] |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- [主题] → [文件路径]

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| YYYY-MM | vX.X | [变更描述] | [变更原因] |

---

## Section 11: 云厂商特异性（可选）

## 11. 云厂商特异性

当问题场景涉及云厂商特定行为时，本节提供平台差异化的诊断与修复指导。

### 11.1 适用场景

- 托管 Kubernetes 服务（ACK/EKS/GKE/AKS）的控制平面不可见层
- 云厂商特定的存储/网络/负载均衡实现
- 云平台 API 限流与配额

### 11.2 内容结构

每个云厂商差异项应包含：
- **平台**: ACK | EKS | GKE | AKS
- **差异描述**: 与标准 K8s 行为的差异说明
- **诊断命令**: 云厂商 CLI 的诊断命令
- **修复方式**: 平台特定的修复路径
- **文档链接**: 官方文档参考

### 11.3 格式示例

| 平台 | 差异 | 诊断命令 | 备注 |
|------|------|---------|------|
| ACK | 控制平面托管，无法直接访问 etcd | `aliyun cs DescribeClusterDetail` | 需通过工单排查 |
| EKS | ENI 模式网络，Pod IP 来自 VPC 子网 | `aws eks describe-cluster` | 注意 IP 地址耗尽 |
| GKE | 自动升级可能导致意外重启 | `gcloud container clusters describe` | 检查维护窗口 |
| AKS | Azure CNI 与 kubenet 网络差异 | `az aks show` | 检查网络模式 |

---

## Section 12: 自动化集成接口（可选）

## 12. 自动化集成接口

定义 Skill 与外部系统集成的标准接口，支持 Agent 自动化调用。

### 12.1 脚本入口

- **diagnose-quick.sh**: Phase 1 快速诊断脚本入口
- **diagnose-deep.sh**: Phase 2 深度诊断脚本入口
- **verify.sh**: 修复后验证脚本入口
- **调用约定**: `./scripts/diagnose-quick.sh --node <NODE_NAME> --namespace <NS>`

### 12.2 Webhook 回调

- **告警路由**: 从 AlertManager/Prometheus 告警自动触发 Skill
- **工单集成**: 从工单系统（Jira/PagerDuty）自动触发 Skill
- **回调格式**: JSON payload 含 skill_id、trigger_source、context

### 12.3 输出规范

- **诊断报告**: JSON 格式输出，含 findings、root_cause_candidates、confidence
- **修复建议**: 结构化的修复步骤，含 risk_level、commands、rollback

### 12.4 格式示例

| 脚本 | 用途 | 示例调用 |
|------|------|----------|
| diagnose-quick.sh | Phase 1 快速检查 | `./scripts/diagnose-quick.sh --node node-1` |
| diagnose-deep.sh | Phase 2 深度检查 | `./scripts/diagnose-deep.sh --node node-1 --ssh` |
| verify.sh | 修复后验证 | `./scripts/verify.sh --node node-1` |

### 12.5 Webhook 配置示例

```yaml
# AlertManager Webhook 示例
receivers:
- name: skill-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-NODE-001'
    send_resolved: true
```

### 12.6 输出 JSON Schema

```json
{
  "skill_id": "SKILL-NODE-001",
  "findings": [
    { "step": "D1.2", "result": "Ready=False", "severity": "critical" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-001", "confidence": 0.85, "evidence": ["D1.2", "D1.5"] }
  ],
  "recommended_action": {
    "rem_id": "REM-001",
    "risk_level": "low",
    "command": "kubectl uncordon <NODE>",
    "rollback": "kubectl cordon <NODE>"
  }
}
```

---

## 命名规范汇总

| 元素 | 格式 | 示例 |
|------|------|------|
| 文件名 | `{NN}-{kebab-case-scenario}.md` | `01-node-notready.md` |
| Skill ID | `SKILL-{CATEGORY}-{SEQ}` | `SKILL-NODE-001` |
| 根因 ID | `RC-{SEQ}` | `RC-001` (文件内唯一) |
| 修复操作 ID | `REM-{SEQ}` | `REM-001` (文件内唯一) |
| 诊断步骤 ID | `D{Phase}.{Seq}` | `D1.1`, `D2.3` |
| 分级步骤 ID | `T{Seq}` | `T1`, `T2` |
| 验证步骤 ID | `V{Seq}` | `V1`, `V2` |
| 症状 ID | `S{Seq}` | `S1`, `S2` |
| 版本标记 | `**[vX.XX+]**` 或 `**[vX.XX-vX.XX]**` | `**[v1.30+]**` |

---

> **关联文档**: [故障诊断/topic-skills/skill-schema.md](../故障诊断/技能体系/skill-schema.md)（原独立 Schema 文件，内容已合并入本模板，Schema 文件保留作为历史参考）

<!-- risk-assessed -->

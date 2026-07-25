---
title: KUDIG 内容补齐执行计划 (reports)
description: 'desc: "深度文档"'
summary: 'desc: "深度文档"'
category: general
tags:
- k8s
- etcd
- apiserver
- scheduler
- controller-manager
- prometheus
- grafana
- helm
- docker
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 内容补齐执行计划 是什么
- 如何 KUDIG 内容补齐执行计划
trigger_keywords:
- KUDIG
- 内容补齐执行计划
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 内容补齐执行计划

> **创建日期**: 2026-05-09
> **模板版本**: 2.0
> **说明**: 按优先级排序的任务列表，每项独立可执行

---

## 阶段 0：准备与验证

### Task-00: 验证模板体系完整性

**目标**: 确认所有 6 套模板已就位且内容正确
**执行命令**:
```bash
ls templates/*.md
wc -l templates/*.md
```
**验收标准**:
- templates/ 下有 7 个 .md 文件（README + 6 套模板）
- 每套模板行数符合预期（skill-template ≥ 500，domain-article ≥ 600，fta ≥ 300，febm ≥ 400，cheat-sheet ≥ 200，presentation ≥ 70）

---

## 阶段 1：P0 — YAML Front Matter 补齐

### Task-01: 为速查卡补充 YAML Front Matter（9 张）

**操作范围**: `系统基础/topic-cheat-sheet/` 下所有 .md 文件（不含 README.md）

**补齐格式**:
```yaml
---
title: "{{实际标题}}速查卡"
description: "{{一句话说明}}"
category: cheatsheet
tags: [k8s, {{component}}, cheatsheet, quick-reference]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
last_updated: "2026-05"
authors:
  - name: "KUDIG Team"
    role: "contributor"
difficulty: "beginner"
related_docs:
  - path: "../domain-{{N}}-{{name}}/{{doc}}.md"
    desc: "深度文档"
  - path: "../故障诊断/topic-fta/list/{{component}}-fta.md"
    desc: "FTA 故障树"
---
```

**文件列表**:
1. `k8s.md` → 添加 k8s_versions: ["1.25", "1.26", "1.27", "1.28", "1.29", "1.30", "1.31", "1.32"]
2. `linux.md` → 补充
3. `docker.md` → 补充
4. `go.md` → 补充
5. `git.md` → 补充
6. `networking.md` → 补充
7. `promql.md` → 补充
8. `sql.md` → 补充
9. `tls-pki.md` → 补充

**注意**: 跳过 README.md（索引文件非内容文档）

---

### Task-02: 为 Domain 核心文档补充 YAML Front Matter（集群基础 ~ 故障诊断 先行）

**原则**: 每篇文档逐一检查，补充 front matter 不改内容

**操作范围**: `集群基础/` ~ `故障诊断/` 每目录前 3 篇文档

**补齐格式**:
```yaml
---
title: "{{文档标题}}"
description: "{{一句话摘要}}"
category: "domain-{N}-{name}"
tags: [k8s, {{component}}, {{tag2}}]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
last_updated: "2026-05"
authors:
  - name: "KUDIG Team"
    role: "contributor"
difficulty: "intermediate"
related_docs:
  - path: "../domain-{N}-{name}/{{doc}}.md"
    type: "depth"
    desc: "{{说明}}"
---
```

**文件列表**（每目录取前 3 篇）:
- `集群基础/`: 01, 02, 05（3 篇）
- `集群基础/`: 01, 02, 03（3 篇）
- `集群基础/`: 11, 12, 13（3 篇）
- `工作负载/`: 10, 11, 19（3 篇）
- `网络/`: 01, 03, 06（3 篇）
- `存储/`: 01, 02, 04（3 篇）
- `安全/`: 01, 02, 03（3 篇）
- `可观测性/`: 01, 02, 04（3 篇）
- `平台工程/`: 01, 02, 06（3 篇）
- `专项技术/`: 01, 05, 08（3 篇）
- `AI基础设施/`: 01, 03, 05（3 篇）
- `故障诊断/`: 前 3 篇（3 篇）

**总计**: 36 篇

---

## 阶段 2：P1 — FTA 模板对齐

### Task-03: 为 FTA 文档补充 YAML 顶事件定义

**目标**: 为 `故障诊断/topic-fta/list/` 下每篇 FTA 文档补充 YAML front matter（顶事件定义）

**补齐格式**:
```yaml
---
fta_id: "FTA-{COMPONENT}-{SEQ}"
title: "{{组件名称}} 故障树分析"
component: "{{组件名称}}"
severity: "P{0-3}"
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
top_event_id: "TE-{序号}"
last_updated: "2026-05"
authors:
  - name: "KUDIG Team"
    role: "contributor"
reviewers: []
tags: [fta, troubleshooting, {{component}}]
related_skills:
  - "../故障诊断/topic-skills/{{NN}}-{{scenario}}.md"
knowledge_refs:
  - "../domain-{{N}}-{{name}}/{{doc}}.md"
  - "../故障诊断/topic-structural-trouble-shooting/{{component}}-*.md"
---
```

**文件列表**（37 篇）:
```
# 🟢 低风险：只读/信息收集，通常无副作用
apiserver-fta.md
backup-restore-fta.md
certificate-fta.md
cloud-provider-fta.md
cluster-autoscaler-fta.md
cluster-upgrade-fta.md
controller-manager-fta.md
crd-operator-fta.md
csi-fta.md
dns-fta.md
gateway-api-fta.md
hpa-fta.md
helm-fta.md
[[19-故障诊断/06-FTA故障树/list/ingress-fta.md|ingress-fta]].md
service-fta.md
monitoring-fta.md
networkpolicy-fta.md
node-fta.md
nodelifecycle-fta.md
csi-fta.md
pod-fta.md
scheduler-fta.md
rbac-fta.md
service-fta.md
statefulset-fta.md
csi-fta.md
terway-fta.md
webhook-admission-fta.md
...（共 37 篇，见 故障诊断/topic-fta/list/ 目录）
```
---

### Task-04: 为 FTA 文档补充评审检查表附录

**目标**: 在每篇 FTA 文档末尾补充 FTA 评审检查表章节

**补齐内容**:
在每篇 FTA 文档末尾添加：

```markdown
---

## FTA 评审检查表

> 完成 FTA 文档后，必须通过以下检查项。

### 结构完整性
- [ ] 顶事件定义清晰，与 SLO 关联
- [ ] 所有中间事件都有子事件
- [ ] 所有底事件都是叶子节点
- [ ] 没有悬挂的孤立事件

### 逻辑正确性
- [ ] 逻辑门类型选择正确（OR vs AND）
- [ ] 同一门下的子事件满足 MECE 原则
- [ ] 层数在 3-5 层之间

### 可观测性
- [ ] 每个底事件至少有 1 个指标监控
- [ ] 每个底事件至少有 1 种诊断命令
- [ ] 每个底事件有明确的判定条件

### 可维护性
- [ ] 编号遵循规范（TE-/IE-/BE- 前缀）
- [ ] 修复动作有风险分级（🟢/🟡/🔴）
- [ ] 修复操作包含回滚方案

### Agent 友好性
- [ ] 每个底事件有结构化的修复动作
- [ ] 修复动作标注了自动化程度（L1/L2/L3）
```

---

## 阶段 3：P2 — Domain 文档监控/版本章节补齐

### Task-05: 为 集群基础 ~ 网络 补充监控告警章节

**目标**: 在 集群基础 到 网络 每目录选 1 篇核心文档，补充 Section 6（监控与告警）

**文档选择**:
- `集群基础/01-kubernetes-architecture-overview.md` → 补充监控章节
- `集群基础/11-etcd-deep-dive.md` → 补充监控章节
- `网络/01-network-architecture-overview.md` → 补充监控章节

**补齐格式**: 参考 `templates/domain-article-template.md` Section 6（关键指标体系 + Prometheus 告警规则 + Grafana 仪表盘）

---

### Task-06: 为 集群基础 ~ 网络 补充版本差异章节

**目标**: 在 集群基础 到 网络 每目录选 1 篇核心文档，补充 Section 10（版本差异）

**补齐格式**: 参考 `templates/domain-article-template.md` Section 10（功能差异表 + API 版本差异）

---

## 阶段 4：P3 — 速查卡扩展章节

### Task-07: 为 k8s.md 和 linux.md 补充云厂商命令章节

**目标**: 在 `系统基础/topic-cheat-sheet/k8s.md` 和 `系统基础/topic-cheat-sheet/linux.md` 末尾补充云厂商特有命令章节

**补齐格式**:
```markdown
## 云厂商特有命令

| 云厂商 | 特殊命令 | 用途 |
|:---|:---|:---|
| AWS EKS | `aws eks describe-cluster --name {{cluster}}` | 查看集群配置 |
| GCP GKE | `gcloud container clusters describe {{cluster}}` | 查看集群详情 |
| Azure AKS | `az aks show --name {{cluster}} --resource-group {{rg}}` | 查看集群详情 |
| 阿里云 ACK | `aliyun cs DescribeClusterDetail --clusterId {{id}}` | 查看集群详情 |
| 腾讯云 TKE | `tke cluster describe --cluster-id {{id}}` | 查看集群详情 |
```

---

## 执行状态

| Task | 名称 | 状态 | 备注 |
|:---:|:---|:---:|:---|
| 00 | 验证模板体系完整性 | ✅ 完成 | 7 文件，2318 行 |
| 01 | 速查卡 YAML Front Matter | ✅ 完成 | 9 张全部更新 |
| 02 | Domain YAML Front Matter | ✅ 完成 | domain-1~12 各 3 篇，共 36 篇 |
| 03 | FTA YAML 顶事件定义 | ⬜ 待执行 | 37 篇 |
| 04 | FTA 评审检查表附录 | ⬜ 待执行 | 37 篇 |
| 05 | Domain 监控告警章节 | ⬜ 待执行 | 3 篇 |
| 06 | Domain 版本差异章节 | ⬜ 待执行 | 3 篇 |
| 07 | 速查卡云厂商命令 | ⬜ 待执行 | 2 篇 |

---

*最后更新: 2026-05-09*

<!-- risk-assessed -->

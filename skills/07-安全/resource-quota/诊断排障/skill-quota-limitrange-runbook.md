---
title: Quota/LimitRange 配额故障诊断 Runbook
description: 'ResourceQuota/LimitRange/Namespace 配额超限与冲突的完整诊断排障指南'
summary: '覆盖配额超限拒绝创建、LimitRange min/max 冲突、默认值缺失导致 BestEffort、Terminating namespace 清理阻塞等 10 类根因的三阶段诊断工作流与风险分级修复'
category: skills
tags:
- k8s
- skills
- runbook
- quota
- limitrange
- namespace
- multitenancy
tier: core
created: '2026-08-27'
last_updated: 2026-08
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 10min
skill_id: SKILL-CONFIG-002
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
agent_execution_mode: L1-advisory
intent_queries:
- exceeded quota 报错怎么解决
- LimitRange 配置冲突如何排查
- Namespace 卡在 Terminating 怎么处理
- Pod 创建被配额拒绝怎么办
trigger_keywords:
- exceeded quota
- limitrange
- resourcequota
- forbidden
- minimum requirement
- terminating namespace
- 配额超限
- 无法创建资源
prerequisites:
- kubectl-basics
- resource-management-basics
related_skills:
- "./培训/01-namespace-resource-quota.md"
- "../../04-工作负载/pod/"
cross_refs:
- type: doc
  path: ./培训/01-namespace-resource-quota.md
  label: 'Namespace 配额培训材料'
- type: doc
  path: ../../04-工作负载/job-cronjob/skill-23-job-cronjob-failure.md
  label: 'Job/CronJob 故障 Runbook（配额常致其失败）'
- type: doc
  path: ../../03-节点/skill-19-node-resource-pressure.md
  label: '节点资源压力诊断'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Namespace/Quota/LimitRange 故障诊断 / Namespace Quota & LimitRange Failure Diagnosis

ResourceQuota 约束 Namespace 级资源总量，LimitRange 约束单 Pod/容器规格并为未声明资源的容器注入默认值。两者由 apiserver 内置 Admission Controller 同步校验，配置错误表现为**创建时立即被拒**（而非运行期异常），因此工单现象高度一致、根因却分散在对象定义、统计口径和级联约束三个层面。

## 快速症状定位

| # | 症状 | 检测方法 | 置信度 |
|---|------|---------|--------|
| S1 | 创建 Pod 报 `exceeded quota` | 🟢 kubectl apply 错误全文 / events | 0.95 |
| S2 | 报 `must specify limits.cpu/memory`（无默认值注入） | 🟢 同上错误消息关键词 | 0.95 |
| S3 | 报 `Minimum cpu/memory requirement not met` 或 max 校验 | 🟢 LimitRange 相关错误文案 | 0.95 |
| S4 | Deployment 扩容部分成功部分失败 | 🟢 rs events 里混合 created 与 quota 错误 | 0.90 |
| S5 | Namespace 永远停在 Terminating | 🟢 `kubectl get ns` + finalize 字段检查 | 0.95 |
| S6 | Job/CronJob 批量 BackoffLimitExceeded 且源头是 quota 拒绝 | 🟢 job pod events | 0.90 |

**排除条件**：节点侧资源不足（Pending 而非创建被拒）→ 节点资源压力 Runbook；RBAC 403 与 quota 文案不同（forbidden vs exceeded）→ RBAC 排查文档；Pod CrashLoop 属运行态 → Pod Runbook。

## 快速分级

```
影响面 × 业务角色
├── 生产核心应用无法扩容/发布 ────────────────→ P0
├── CI 发布流水线整体阻塞（共享构建 ns 配额满）──→ P1
├── 多个团队共用 ns 相继报告创建失败 ────────────→ P1
├── 单个应用配额不足可临时绕过（调度到其他 ns）──→ P2
└── Terminating ns 无业务影响仅为清理噪音 ──────→ P3
```

**立即升级条件**：集群管理员 Token 因配额误禁而失效场景罕见但存在（LimitRange 影响系统 ns）；准入链报错伴随 apiserver 日志告警需走控制面通道。

## Phase 1 快速检查（🟢 只读）

```bash
# D1.1 全景：目标 ns 有哪些配额约束，水位多少
kubectl describe namespace <ns>          # Resource 栏直出已用/上限
kubectl get resourcequota,limitrange -n <ns>
kubectl describe resourcequota <name> -n <ns>   # Used vs Hard 对照表

# D1.2 用原始错误的完整文案反查约束类型
#   "exceeded quota: <name>, requested: X used: Y limited: Z"
#   → ResourceQuota 水位问题
#   "minimum cpu requirement per container is required"
#   → LimitRange min 注入失败

# D1.3 找出占用水位的"大户"
kubectl top pods -n <ns> --sort-by=memory | head -15
kubectl get pods -n <ns> -o json | jq -r '.items[] | select(.status.phase=="Succeeded" or .status.phase=="Failed") |
  "\(.metadata.namespace)/\(.metadata.name) \(.status.phase)"' | sort | uniq -c | sort -rn | head

# D1.4 Terminating namespace 清理状态
kubectl get ns <terminating-ns> -o jsonpath='{.spec.finalizers}'
kubectl api-resources --verbs=list --namespaced -o name \
  | xargs -n1 kubectl get --show-kind --ignore-not-found -n <terminating-ns> --no-headers | wc -l   # 残留资源计数
```

## Phase 2 深度检查（🟢 只读）

```bash
# D2.1 LimitRange 规则语义审计（min<=default<=max 链式验证）
kubectl get limitrange <lr-name> -n <ns> -o yaml
# 人工核对四类规则: Container/Pod/PersistentVolumeClaim 各自的
#   default/defaultRequest/max/min/maxLimitRequestRatio 是否自洽

# D2.2 计算口径差异排查（terminated 对象是否仍在占用）
kubectl get pods,jobs -n <ns> --field-selector=status.phase!=Running -o wide
# 上限若含 requests.* 而 Pod 实际未设置 requests，会被强制按 limits 折算注入——查看实际值：

kubectl get pods -n <ns> -o json | jq -r '.items[]
  | select(.spec.containers[].resources.requests.cpu == null)
  | .metadata.name'    # 找出缺 requests 但受 quota 管制的 pod

# D2.3 多 ResourceQuota 叠加效应（scope 划分易漏看）
kubectl get resourcequota -n <ns> -o custom-columns='NAME:.metadata.name,SCOPES:.spec.scopes,OUT:.spec.hard'
# PriorityClass scope 配置会让高优先级 workload 静默走另一份配额

# D2.4 上游事件流（谁在持续吃额度——通常是失控的 CronJob/HPA）
kubectl get events -n <ns> --sort-by=.lastTimestamp | grep -c FailedCreate
kubectl get cronjobs,hpa -n <ns>

# D2.5 apiserver 准入日志复核（控制面具备权限时）
kubectl logs -n kube-system deploy/kube-apiserver | grep -iE "resourcequota|limitranger" | tail -20
```

## Phase 3 主动探测（🟡 低风险）

```bash
# D3.1 影子配额试算：在隔离测试 ns 复刻同套 quota/lr 后重放失败清单
kubectl create ns quota-shadow-test
# （把生产 YAML 中 ns 替换后 dry-run 再 apply）

# D3.2 以 server-dry-run 让 admission 全链路跑一遍但不真正入库
kubectl apply -f <problem-yaml> --dry-run=server -v=8   # 输出含每个 admission 步骤
```

## 根因分类与修复

### 根因清单

| RC ID | 根因 | 典型证据 | 首选修复 | 风险 |
|-------|------|---------|---------|------|
| RC-001 | ResourceQuota hard 值过低或未同步扩容规划 | Used≈Hard 长期满格 | 走容量评审提升 hard | 🟡 |
| RC-002 | 历史遗留 terminated pods/jobs 占用统计口径 | 大量 Completed Pod 名单 | TTL 清理或 GC 策略补齐 | 🟢 |
| RC-003 | LimitRange min > max 自相矛盾 | apply 后 lr 创建成功但所有 workload 被拒 | 修正区间定义 | 🟢 |
| RC-004 | default 值超出 max（min 突破） | default request 被 clamp 后仍违规 | 调整 default 至合法带内 | 🟢 |
| RC-005 | LimitRange 缺省项缺失导致 BestEffort | workloads 无 requests 字段、报错要求显式指定 | 补 defaultRequest 注入 | 🟢 |
| RC-006 | 多个 ResourceQuota scope 设计失衡 | 命中特定 priority/storage class 时失败率高 | 合并或重构 scope 口径 | 🟡 |
| RC-007 | quota.requests 与容器 limits 口径错配 | 提交 requests.limits.* 类目但文件少定义 | 双口径齐备或统一采用单一维度 | 🟡 |
| RC-008 | maxLimitRequestRatio 违例 | ratio 类错误文案出现 | 调整 ratio 或限制爆量请求 | 🟢 |
| RC-009 | Terminating ns 存在终态阻塞资源（如 CRD 外部 finalizer 留存） | 残留资源列表非空且 owner 不在 | 移除无效 finalizer 引用 | 🔴 |
| RC-010 | 由 HPA/Job/CronJob 并发爆发瞬时打满 | 短窗口内 quota 大幅波动 | 引发限速策略或 pool 分组隔离 | 🟡 |

### 关键修复动作详解

**REM-A 提升 Hard 上限（RC-001）🟡**

```bash
kubectl patch resourcequota <rq> -n <ns> --type merge -p '{"spec":{"hard":{"requests.cpu":"<new>"}}}'
```

变更前置动作：
1. 出具该 ns 过去 14 天用量曲线（Prometheus `kube_resourcequota_used_hard_ratio`）
2. 若集群层面 allocatable 余量不足以覆盖新增，先联动节点扩容流程（对应节点运维 Runbook），避免"调大即全局挤压"

**REM-B 清理历史占用（RC-002）🟢**

```bash
# 批量删除已完成 pods（保守模式：先 dry-run 再真删）
kubectl delete pod -n <ns> --field-selector=status.phase==Succeeded --dry-run=server
kubectl delete pod -n <ns> --field-selector=status.phase==Succeeded            # 🟡 仅针对确实无价值的完成品

# CronJob 层面补自动清理
kubectl patch cronjob <cj> -n <ns> --type merge -p \
  '{"spec":{"successfulJobsHistoryLimit":1,"failedJobsHistoryLimit":1}}'
```

**REM-C 修正 LimitRange 定义（RC-003/004/005/008）🟢**

合规模板（自检通过后再落地）：

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: standard-limits
spec:
  limits:
  - type: Container
    max:       { cpu: "8",     memory: 16Gi }
    min:       { cpu: "50m",   memory: 128Mi }
    default:        { cpu: "500m", memory: 512Mi }   # ≤ max
    defaultRequest: { cpu: "250m", memory: 256Mi }   # ≤ default
```

**REM-D 强制解锁 Terminating Namespace（RC-009）🔴 — 需审批**

```bash
# 第一步：列出真正的残留者并归因（绝大多数是 CR finalizer 指向已卸载 operator）
kubectl api-resources --namespaced=true -o name | xargs -P4 -I{} sh -c \
  'kubectl get {} -n <terminating-ns> --ignore-not-found --no-headers 2>/dev/null'

# 第二步：仅当残留对象确属垃圾（其 controller 早已移除）才清空 finalizer 等待队列     # 🔴
kubectl get ns <terminating-ns> -o json \
  | jq '.spec.finalizers = []' \
  | kubectl replace --raw /api/v1/namespaces/<terminating-ns>/finalize -f -

# 禁止：为了"看起来干净"对仍有业务残留值的资源走此路径——那是在隐藏数据事故现场。
```

审批要求：Platform Lead + 数据 Owner 双签；操作前 dump 全部残留对象 YAML 到工单附件。

## 验证清单

| 编号 | 项目 | 通过标准 |
|-----|------|---------|
| V1 | 原 apply 的资源成功创建 | `kubectl get <kind>` 可见 |
| V2 | quota 水位回落至健康线以下（Used/Hard < 80%） | ✅ |
| V3 | LimitRange 修改后的影子测试全绿（D3.1 环境） | ✅ |
| V4 | 已删历史对象未被任何存量工作负载引用（回归 GC 无误伤） | ✅ |
| V5 | 24h 内同一 ns 无新增 FailedCreate(quota) 事件 | ✅ |

## 升级协议

- 需要**改集群维度的 LimitRange 默认模板**或 apiserver `--enable-admission-plugins` 参数组合 → 平台架构评审
- RC-009 的数据保留争议（残留对象疑似承载未备份业务）→ 数据 Owner 与法务/合规协同确认后方可清理
- 配额政策涉及多租户计费分摊 → 引入 FinanceOps 会签

## 云厂商特异性

| 环境 | 注意事项 |
|------|---------|
| ACK | 容器服务集成「资源视图」可直接导出 WaterLevel 曲线替代手工 jq 组装 |
| EKS | Fargate profile 下 hard quota 生效于 pod 尺寸聚合而 node 不参与统计，配额评估模型不同 |
| GKE | Autopilot 明确拒绝部分 gpu 之外的 burstable 类声明，达到 hard 前就可能被拦 |
| 自建 | kube-controller-manager `--horizontal-pod-autoscaler-sync-period` 与 quota 回收节奏叠加时应统一监控口径 |

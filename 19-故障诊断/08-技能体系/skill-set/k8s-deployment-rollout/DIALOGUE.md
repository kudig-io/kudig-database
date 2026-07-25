---
title: K8s Deployment Rollout Failure - 远程顾问对话脚本
summary: Deployment发布问题的远程顾问对话脚本，覆盖滚动更新、金丝雀发布、回滚操作。
category: troubleshooting
tags:
- workloads
- remote-consultant
tier: supporting
created: 2026-05-21
updated: '2026-05-23'
skill_id: SKILL-DEPLOY-001
version: 1.0.0
agent_role: remote-advisor
dialogue_type: guided-troubleshooting
rounds: 3
branches_per_round: 3+
last_updated: 2026-05-23
relationships:
- target: '[[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[22-概念/14-case-studies/2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点.md]]'
  type: uses
- target: '[[23-实体/02-K8s核心组件/deployment.md]]'
  type: uses
- target: '[[23-实体/02-K8s核心组件/kubernetes.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[23-实体/02-K8s核心组件/deployment.md|Deployment]] Rollout Failure — 远程顾问对话脚本

> 顾问身份：远程 [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] SRE 顾问（无法直接连接集群），通过对话指导现场工程师。

---

## 对话入口

### 入口 A：工程师报告 rollout 卡住
> **工程师**: "Deployment rollout 卡住了，新版本一直没起来"

**顾问**: 收到。先确认影响范围：1) 是生产环境吗？核心业务还是非核心？2) 执行 `kubectl get deployment <name> -n <ns>`，告诉我 READY/DESIRED 数值。3) 最近是否有发布或变更？

### 入口 B：工程师报告 ProgressDeadlineExceeded
> **工程师**: "看到 ProgressDeadlineExceeded，Deployment 没更新"

**顾问**: 这表示 rollout 卡住。请执行 `kubectl get deployment <name> -n <ns> -o wide`。若无法执行 kubectl，告诉我你在什么平台看到该事件、能否看到副本数、是否有用户投诉。

### 入口 C：工程师描述发布未生效
> **工程师**: "应用发布了但好像没生效，还是旧版本在跑"

**顾问**: 先区分两种情况：A) 新版本 Pod 未创建；B) 新旧版本共存。请执行 `kubectl get deployment <name> -n <ns>`，关注 READY、UP-TO-DATE、AVAILABLE 三列。若无法执行，请描述从 Dashboard 看到的状态。

### 入口 D：工程师询问是否回滚
> **工程师**: "rollout 失败了，要不要直接回滚？"

**顾问**: 先别急。回滚适用于新版本有 bug/镜像错误，不适用于资源不足或调度问题（回滚会复现）。请先执行 `kubectl get pods -n <ns> -l app=<label>` 和 `kubectl get events -n <ns> --field-selector involvedObject.name=<name> | tail -20`，把结果贴给我。

---

## Round 1: 快速确认与分级

### 分支 1-A：能执行 kubectl
**顾问**: 请执行以下只读命令：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Deployment 状态
kubectl get deployment <name> -n <ns> -o jsonpath='{"DESIRED: "}{.spec.replicas}{"\nREADY: "}{.status.readyReplicas}{"\nUP-TO-DATE: "}{.status.updatedReplicas}{"\nAVAILABLE: "}{.status.availableReplicas}{"\nPAUSED: "}{.spec.paused}{"\n"}'
# ReplicaSet
kubectl get rs -n <ns> -l app=<label>
# Pod 状态
kubectl get pods -n <ns> -l app=<label>
```
> 【如果无法执行 jsonpath】改为 `kubectl get deployment <name> -n <ns> -o yaml | grep -E "replicas:|readyReplicas|updatedReplicas|availableReplicas|paused"`
> 【如果无法通过 label 查询】改为 `kubectl get rs -n <ns> | grep <name>` 和 `kubectl get pods -n <ns> | grep <name>`
> 【如果 kubectl 完全不可用】请从监控/Dashboard 提供期望副本数 vs 就绪副本数、Pod 状态列表、最近 10 分钟告警事件。

### 分支 1-B：只有监控/Dashboard
**顾问**: 请从 Dashboard 收集：1) Deployment 的 DESIRED/READY/UP-TO-DATE/AVAILABLE；2) Pod 列表及 STATUS/RESTARTS；3) 相关 Warning/Error 事件。
> 【如果 Dashboard 看不到事件】搜索独立「事件」或「告警」页面。
> 【如果 Dashboard 也受限】告诉我：得知问题的渠道、服务异常表现、影响范围。信息不完整也能推进，我会给出最可能的根因排序。

### 分支 1-C：多 Deployment 同时问题
**顾问**: ⚠️ 多 Deployment 同时问题是**升级信号**，通常是集群级别问题。请立即执行：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
kubectl get events --all-namespaces --field-selector type=Warning | tail -30
```
> 【如果无法获取全集群事件】改为 `kubectl get events -n <ns> --field-selector type=Warning | tail -20`
> 【如果 kubectl get nodes 不可用】请从监控平台确认：是否有节点离线、集群总节点数是否减少、是否有网络告警。

**升级决策点**：若确认多节点或控制平面异常，立即：1) 通知值班主管；2) 启动集群问题排查（[[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|SKILL]]-NODE-001）；3) 评估是否启动灾难恢复。

---

## Round 2: 深度诊断与根因定位

### 分支 2-A：Pod 为 Pending/ContainerCreating
**顾问**: 执行以下诊断：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pending Pod 事件
kubectl describe pod <pod> -n <ns> | grep -A 20 "Events"
# 检查节点资源
kubectl top nodes
# 检查 FailedScheduling
kubectl get events -n <ns> --field-selector reason=FailedScheduling
```
> 【如果无法 describe】改为 `kubectl get events -n <ns> --field-selector involvedObject.name=<pod>`
> 【如果无法 top】改为 `kubectl describe node <node> | grep -A 10 "Allocated resources"` 或从监控查看节点使用率
> 【如果无 field-selector 支持】改为 `kubectl get events -n <ns> | grep FailedScheduling`

**根因判断**：Insufficient cpu/memory → RC-001；Failed to pull image → RC-002；taint/affinity 相关 → RC-006；FailedMount → 存储问题；0/N nodes available 无具体原因 → 集群资源耗尽。

### 分支 2-B：Pod 为 ImagePullBackOff/ErrImagePull
**顾问**: 执行以下检查：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认具体错误
kubectl describe pod <pod> -n <ns> | grep -A 5 "Failed to pull image|Back-off pulling image"
# 检查镜像配置
kubectl get deployment <name> -n <ns> -o jsonpath='{.spec.template.spec.containers[*].image}'
# 检查 imagePullSecrets
kubectl get sa default -n <ns> -o yaml | grep -A 10 imagePullSecrets
```
> 【如果无法 describe】改为 `kubectl get events -n <ns> | grep -i "image|pull"`
> 【如果无法 jsonpath】改为 `kubectl get deployment <name> -n <ns> -o yaml | grep "image:"`
> 【如果无法获取 sa】从镜像仓库后台确认：标签是否存在、是否需要认证、最近是否更换仓库地址

**常见根因**：not found → 标签错误；unauthorized → 认证失败；timeout → 网络/仓库问题；manifest unknown → 架构不匹配。

### 分支 2-C：Pod 为 CrashLoopBackOff/Error
**顾问**: 执行以下诊断：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看崩溃日志
kubectl logs <pod> -n <ns> --previous 2>/dev/null | tail -50
# 检查探针配置
kubectl get deployment <name> -n <ns> -o jsonpath='{"Liveness: "}{.spec.template.spec.containers[0].livenessProbe}{"\nReadiness: "}{.spec.template.spec.containers[0].readinessProbe}{"\n"}'
# 查看重启详情
kubectl describe pod <pod> -n <ns> | grep -A 10 "Last State|Restart Count"
```
> 【如果 --previous 不可用】改为 `kubectl logs <pod> -n <ns> | tail -50`
> 【如果无法 logs】请检查日志平台（ELK/Loki/Splunk）搜索该 Pod 错误日志
> 【如果无法 jsonpath】改为 `kubectl get deployment <name> -n <ns> -o yaml | grep -A 15 "livenessProbe|readinessProbe"`
> 【如果 describe 不可用】改为 `kubectl get pod <pod> -n <ns> -o yaml | grep -A 5 "restartCount|lastState"`

**根因判断**：应用启动报错 → RC-003；正常启动但被 kill → 探针 initialDelay 太短；Init Container 报错 → RC-007；日志为空且 restartCount 高 → OOMKilled。

### 分支 2-D：Deployment paused = true
**顾问**: 这是最容易修复的情况。请确认：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment <name> -n <ns> -o jsonpath='{.spec.paused}'
kubectl rollout history deployment/<name> -n <ns>
```
> 【如果无法 jsonpath】改为 `kubectl get deployment <name> -n <ns> -o yaml | grep "paused"`
> 【如果无法查看 history】请检查 CI/CD 流水线日志中的 Deployment 操作记录。

**修复（低风险）**：`kubectl rollout resume deployment/<name> -n <ns>`
> 【如果权限不足】请联系有 rollout 权限的管理员执行，或临时提升 RBAC。
> 【如果 resume 后仍卡住】说明 paused 是表象，背后有更深问题，继续按 2-A/B/C 排查。

### 分支 2-E：新旧 ReplicaSet 共存
**顾问**: 执行：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get rs -n <ns> -l app=<label> -o wide
kubectl get deployment <name> -n <ns> -o jsonpath='{"maxUnavailable: "}{.spec.strategy.rollingUpdate.maxUnavailable}{"\nmaxSurge: "}{.spec.strategy.rollingUpdate.maxSurge}{"\n"}'
```
> 【如果无法 label 查询】改为 `kubectl get rs -n <ns> | grep <name>`
> 【如果无法 jsonpath】改为 `kubectl get deployment <name> -n <ns> -o yaml | grep -A 5 "strategy:"`

**根因判断**：maxUnavailable=0 且 replicas=1 → 策略冲突；maxSurge=0 且 maxUnavailable=0 → 非法配置；新版本 Pod Pending/CrashLoopBackOff → 新版本本身有问题。

---

## Round 3: 修复与验证

### 分支 3-A：RC-001 资源不足
**顾问**: 选择修复方案：

**方案 1（推荐）**：扩容节点。如有 Cluster Autoscaler：`kubectl get nodes -l node-group=<节点组>` 后配置扩容；如无，手动添加节点或释放非关键 Pod。

**方案 2**：降低资源请求：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> -p '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"requests":{"cpu":"100m","memory":"128Mi"}}}]}}}}'
```
> 【如果无法确定容器名】先执行 `kubectl get deployment <name> -n <ns> -o jsonpath='{.spec.template.spec.containers[*].name}'`，替换命令中的 "app"。

**方案 3**：减少副本数：`kubectl scale deployment/<name> --replicas=1 -n <ns>`。若当前已是 1 副本，此方案不适用。

**验证**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment <name> -n <ns>  # READY==DESIRED, UP-TO-DATE==DESIRED
kubectl rollout status deployment/<name> -n <ns>  # successfully rolled out
```
> 【如果仍卡住】说明不止资源一个问题，回到 Round 2 检查 Pod 状态变化。

**升级决策点**：扩容后 10 分钟仍未完成，或节点持续资源不足 → 升级基础设施团队。

### 分支 3-B：RC-002 镜像拉取失败
**顾问**: 选择修复方案：

**方案 1（标签错误）**：
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl set image deployment/<name> app=<正确镜像>:<正确标签> -n <ns>
```
> 【如果无法确定正确标签】从镜像仓库确认最新可用标签，或回退到上一个已知标签。

**方案 2（认证失败）**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch sa default -n <ns> -p '{"imagePullSecrets":[{"name":"registry-secret"}]}'
```
> 【如果无法确定 secret】执行 `kubectl get secrets -n <ns> | grep registry`，或联系仓库管理员。

**方案 3（仓库不可用）**：回滚到上一个版本：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout history deployment/<name> -n <ns>
kubectl rollout undo deployment/<name> -n <ns>
```
> 【如果 rollback 仍 ImagePullBackOff】说明旧版本镜像也被清理，需紧急推送可用镜像。

**验证**：`kubectl get pods -n <ns> -l app=<label>` 应全为 Running，无 ImagePullBackOff；`kubectl get events -n <ns>` 无 Failed to pull image。
> 【如果变为 CrashLoopBackOff】镜像能拉取但启动失败，转入 RC-003。

**升级决策点**：镜像仓库完全不可用且 30 分钟无法恢复 → 升级基础设施/镜像仓库团队。

### 分支 3-C：RC-003 健康检查失败
**顾问**: 选择修复方案：

**方案 1（启动慢）**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> --type='json' -p='[{"op":"replace","path":"/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds","value":60},{"op":"replace","path":"/spec/template/spec/containers/0/readinessProbe/initialDelaySeconds","value":30}]'
```
> 【如果容器不是第一个】先执行 `kubectl get deployment <name> -n <ns> -o jsonpath='{.spec.template.spec.containers[*].name}'`，将 /0 替换为对应索引。

**方案 2（超时/重试）**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> --type='json' -p='[{"op":"replace","path":"/spec/template/spec/containers/0/livenessProbe/timeoutSeconds","value":10},{"op":"replace","path":"/spec/template/spec/containers/0/livenessProbe/failureThreshold","value":5}]'
```
> 【如果 patch 返回 NotFound】该字段不存在，先查看当前探针：`kubectl get deployment <name> -n <ns> -o yaml | grep -A 20 "livenessProbe"`

**方案 3（紧急移除 livenessProbe）**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> --type='json' -p='[{"op":"remove","path":"/spec/template/spec/containers/0/livenessProbe"}]'
```
⚠️ 仅作紧急恢复，崩溃时不会自动重启。恢复后应立即重新添加并调优。
> 【如果无法 json patch】下载 yaml 手动编辑：`kubectl get deployment <name> -n <ns> -o yaml > deploy-backup.yaml`，编辑后 `kubectl apply -f deploy-backup.yaml`

**验证**：等待 2-3 分钟后 `kubectl get pods -n <ns> -l app=<label>`，Pod 应 Running 且 RESTARTS 不再增长；`kubectl describe pod <pod> -n <ns> | grep -A 5 Events` 无探针失败事件。
> 【如果重启次数仍增加】应用本身有启动问题，请重新提供应用日志。

**升级决策点**：调优后仍无法通过 readiness 且日志显示代码级错误 → 升级开发团队。

### 分支 3-D：RC-004 策略配置不当
**顾问**: 执行修复：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":1,"maxSurge":1}}}}'
```
> 【如果 replicas=1 且希望零停机】先执行 `kubectl scale deployment/<name> --replicas=2 -n <ns>`，等 scale 完成后再调策略。
> 【如果 patch 报错】策略字段可能不存在，执行 `kubectl get deployment <name> -n <ns> -o yaml > deploy-backup.yaml`，手动编辑后 `kubectl apply -f deploy-backup.yaml`

**验证**：`kubectl rollout status deployment/<name> -n <ns>` 应在 2-5 分钟内完成；`kubectl get rs -n <ns> -l app=<label>` 应仅新 ReplicaSet 有 pod。
> 【如果仍卡住】策略只是表象，新版本 Pod 有其他问题，转入对应根因。

验证通过后恢复原始策略：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment/<name> -n <ns> -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":"25%","maxSurge":"25%"}}}}'
```
### 分支 3-E：回滚（高风险）
**顾问**: ⚠️ 回滚前请确认检查清单：
- [ ] 新版本问题无法在 10 分钟内修复
- [ ] 回滚版本镜像仍存在于仓库
- [ ] 已通知相关团队
- [ ] **无数据库 schema 变更**（回滚可能导致不兼容）
- [ ] 核心服务不可用 > 50% 或业务已中断

> 【如果有 schema 变更】不要回滚！联系 DBA 和开发团队评估兼容性，错误回滚可能导致数据损坏。

**执行**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout history deployment/<name> -n <ns>
kubectl rollout undo deployment/<name> -n <ns>
# 或指定版本：kubectl rollout undo deployment/<name> -n <ns> --to-revision=<版本>
```
> 【如果回滚失败】"revision not found" → 目标版本已清理，需手动修改镜像到已知版本；"resource not found" → 检查名称和 namespace。

**验证**：`kubectl rollout status deployment/<name> -n <ns>` 应成功；`kubectl get pods -n <ns> -l app=<label>` 应全 Running。
> 【如果回滚后仍失败】问题不在新版本，而在集群环境，回到 Round 1 重新排查。

**升级决策点**：回滚后仍失败 → 立即升级集群管理员+开发团队联合排查；涉及 schema → 升级 DBA+架构师；导致其他服务异常 → 启动级联问题应急。

---

## 全局升级决策点

| 条件 | 升级对象 | 方式 |
|------|---------|------|
| 核心服务 >50% 不可用 | 值班主管+开发团队 | 电话/IM |
| 回滚后仍失败 | 集群管理员+SRE | 工单/电话 |
| 多 Deployment 同时问题 | 基础设施团队 | 电话/IM |
| 涉及数据库 schema | DBA+架构师 | 电话 |
| 级联问题 | 全技术团队 | 电话/IM |
| 同一 Deployment 24h 失败 3 次 | 开发+QA | 工单 |
| 镜像仓库完全不可用 | 基础设施团队 | 电话 |
| 控制平面异常 | 集群管理员 | 电话 |

**升级信息模板**：
```
【{P0/P1/P2}】Deployment Rollout Failure - {集群名}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: Deployment {ns}/{name} rollout 失败
- 影响范围: 可用性 {available}/{desired}，受影响功能 {功能}
- 已完成诊断: Round 1 完成，Round 2 {已执行检查}
- 初步发现: 可能根因 {RC-XXX}
- 已尝试修复: {操作} → 结果 {成功/失败/进行中}
- 需要: {所需支持}
- Skill: SKILL-DEPLOY-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 对话结束确认

**修复成功**：
```
✅ 修复验证通过。根因 {RC-XXX}，当前 READY={X}/DESIRED={X}，rollout 完成。
后续 24h 关注：1) Pod RESTARTS 是否持续增长；2) 下次发布是否成功；3) 若根因是资源不足，评估调整配额或扩缩容策略。
```

**升级移交**：
```
⏫ 已确认需升级。已收集信息：Deployment 状态 {summary}、疑似根因 {RC-XXX}、已尝试 {操作}。
请将上述信息发送给 {升级对象}。升级后如需继续支持，随时同步进展。
```

---

*对话脚本结束 — SKILL-DEPLOY-001 v1.0*


### 分支 1.4：阿里云ACK/专有云部署发布排查

工程师："我们在阿里云ACK/专有云环境，Deployment发布失败"

顾问："阿里云环境有额外的发布管理维度，请按以下顺序排查：

**步骤 1：阿里云镜像仓库检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查镜像是否来自阿里云ACR
kubectl get deployment <deploy> -o yaml | grep image

# 检查ACR镜像拉取权限
kubectl get secret -n <ns> | grep acr

# 检查ACR实例状态
aliyun cr GET /repos
```
> **如果无法执行aliyun CLI**：请登录ACR控制台，告诉我：
> 1. 镜像仓库是否存在？
> 2. 镜像Tag是否存在？
> 3. 是否有访问权限？

**步骤 2：ACK发布策略检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查是否使用ACK灰度发布
kubectl get rollout -A

# 检查HPA状态（发布时资源不足）
kubectl get hpa -A

# 检查ESS弹性伸缩状态
aliyun ess DescribeScalingGroups --RegionId <region>
```
**步骤 3：专有云发布特殊考虑**
- 专有云镜像可能存储在内部Harbor
- 检查镜像同步任务状态
- 确认专有云版本支持的K8s API版本

**步骤 4：阿里云特定修复**

如ACR镜像拉取失败：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建ACR拉取Secret
kubectl create secret docker-registry acr-secret   --docker-server=<acr-domain>   --docker-username=<username>   --docker-password=<password>

# 更新Deployment使用Secret
kubectl patch deployment <deploy> -p '{"spec":{"template":{"spec":{"imagePullSecrets":[{"name":"acr-secret"}]}}}}'
```
如ESS缩容导致资源不足：
1. 调整ESS最小实例数
2. 临时扩容节点
3. 重新发布

## 相关案例

- [[22-概念/14-case-studies/2026-05-28-daemonset-affinity-miss.md|2026-05-28-daemonset-affinity-miss]]
- [[22-概念/14-case-studies/2026-06-20-节点时区不一致导致cronjob调度错乱.md|2026-06-20-节点时区不一致导致cronjob调度错乱]]
- [[22-概念/14-case-studies/2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点.md|污点容忍度配置错误导致pod无法调度到专用节点]].md|2026-09-05-污点容忍度配置错误导致pod无法调度到专用节点]]
- [[22-概念/14-case-studies/2026-10-15-pod-disruption-budget阻止节点维护排空.md|2026-10-15-pod-disruption-budget阻止节点维护排空]]
## Related

- [[17-系统基础/06-知识字典/fundamentals/nodes.md|Nodes（节点）]]


<!-- risk-assessed -->

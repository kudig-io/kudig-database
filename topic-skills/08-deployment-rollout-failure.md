---
skill_id: "SKILL-WORK-001"
skill_name: "Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis"
version: "1.0"
category: "workload"
severity_range: "P0-P3"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "5-45min"
risk_level: "medium"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "rollout stuck"
  - "deployment failed"
  - "rollback failed"
  - "revision history"
  - "maxUnavailable"
  - "maxSurge"
  - "progressDeadlineSeconds"
  - "rollout restart"
  - "canary deployment failed"
  - "blue-green switch failed"
  - "滚动更新卡住"
  - "部署失败"
  - "回滚失败"
  - "版本回退失败"
  - "新版本无法启动"
  - "Pod 更新失败"
  - "StatefulSet 更新卡住"
  - "DaemonSet 更新失败"
trigger_events:
  - "ProgressDeadlineExceeded"
  - "ReplicaSetUpdated"
  - "ScalingReplicaSet"
  - "FailedCreate"
  - "MinimumReplicasUnavailable"
  - "FailedRollback"
  - "DeploymentRollback"
  - "SuccessfulDelete"
  - "SuccessfulCreate"
trigger_metrics:
  - 'kube_deployment_status_observed_generation != kube_deployment_metadata_generation'
  - 'kube_deployment_status_replicas_unavailable > 0'
  - 'kube_deployment_spec_replicas != kube_deployment_status_replicas_available'
  - 'kube_deployment_status_condition{condition="Available",status="false"}'
  - 'kube_deployment_status_condition{condition="Progressing",status="false"}'
  - 'kube_statefulset_status_replicas_ready != kube_statefulset_status_replicas'
  - 'kube_daemonset_status_number_unavailable > 0'
related_skills:
  - "SKILL-POD-001"
  - "SKILL-POD-002"
  - "SKILL-STORE-001"
  - "SKILL-NET-001"
fta_refs:
  - "topic-fta/list/workload-fta.md"
knowledge_refs:
  - "domain-12-troubleshooting/11-deployment-comprehensive-troubleshooting.md"
  - "domain-4-workloads/"
  - "domain-9-platform-ops/"
---

# Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis

---

## 1. 概述

Deployment 滚动更新故障是 Kubernetes 生产环境中**最常见的工作负载问题类型**之一。当滚动更新失败时，可能导致新版本无法上线、旧版本无法退役、甚至服务完全不可用。Deployment Controller 通过 ReplicaSet 管理 Pod 的创建和删除，任何一个环节的失败都可能导致整个更新流程卡住。

此 Skill 同时覆盖 **Deployment**、**StatefulSet** 和 **DaemonSet** 三种工作负载类型的滚动更新故障诊断，以及**金丝雀部署**和**蓝绿部署**等高级部署模式的故障排查。

### 典型触发场景

1. **滚动更新卡住**: 新 ReplicaSet 的 Pod 无法启动（CrashLoopBackOff、ImagePullBackOff），旧 ReplicaSet 未能缩容，更新进度停滞
2. **回滚失败**: 执行 `kubectl rollout undo` 后版本未回退，或回滚到的版本同样无法启动
3. **金丝雀/蓝绿部署流量切换异常**: 流量未按预期比例分配，新旧版本流量混乱
4. **StatefulSet 有序更新阻塞**: 更新卡在某个 ordinal 的 Pod，后续 Pod 无法更新
5. **DaemonSet 部分节点未更新**: 部分节点上的 DaemonSet Pod 版本不一致

### 前置条件

- **RBAC 权限**: 对 deployments、replicasets、pods、events、configmaps、secrets 的 get/list/watch/update/patch 权限
- **kubectl 版本**: v1.28+ （与目标集群版本匹配）
- **工具要求**: kubectl, jq（可选但推荐用于 JSON 解析）
- **监控系统**: Prometheus + kube-state-metrics（用于 trigger_metrics 匹配）
- **镜像仓库访问**: 确保有权限验证镜像可用性

> ⚠️ **重要**: 生产环境主服务的滚动更新失败属于 P0 事件，应立即响应。回滚操作本身也可能失败，需准备多重恢复方案。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | Deployment rollout 进度停滞，Progressing 条件为 False / Deployment rollout stuck with Progressing=False | `kubectl get deploy NAME -n NS -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'` 检查 status 和 reason | 0.95 | 刚触发更新且 progressDeadlineSeconds 尚未到期；用户主动暂停更新 (`kubectl rollout pause`) |
| SP-02 | 新 ReplicaSet 的 Pod 持续 CrashLoopBackOff / New ReplicaSet pods in CrashLoopBackOff | `kubectl get pods -n NS -l app=NAME --sort-by=.status.startTime` 检查最新 Pod 状态 | 0.90 | Pod 应用内部问题而非部署配置问题（但仍需修复） |
| SP-03 | 旧 ReplicaSet 未缩容，replicas 数量不变 / Old ReplicaSet not scaling down | `kubectl get rs -n NS -l app=NAME --sort-by=.metadata.creationTimestamp` 检查各 RS 的 READY/DESIRED | 0.85 | maxUnavailable=0 且新 Pod 尚未 Ready（正常行为） |
| SP-04 | ProgressDeadlineExceeded 条件触发 / ProgressDeadlineExceeded condition triggered | `kubectl get deploy NAME -n NS -o jsonpath='{.status.conditions[?(@.type=="Progressing")].reason}'` 返回 ProgressDeadlineExceeded | 0.95 | progressDeadlineSeconds 设置过短（<60s）导致正常启动被误判 |
| SP-05 | rollout undo 后版本未回退 / Version not reverted after rollout undo | `kubectl rollout history deploy/NAME -n NS` 检查 REVISION，执行 undo 后版本号未变化 | 0.90 | revisionHistoryLimit=0 导致无可回退版本；undo 的目标版本也有问题 |
| SP-06 | StatefulSet 更新卡在中间 ordinal / StatefulSet update stuck at intermediate ordinal | `kubectl get pods -n NS -l app=NAME -o wide --sort-by=.metadata.name` 检查 Pod 版本分布 | 0.85 | 使用 partition 策略进行分批更新（预期行为） |
| SP-07 | DaemonSet 更新后部分节点 Pod 未更新 / DaemonSet pods not updated on some nodes | `kubectl get pods -n NS -l app=NAME -o wide` 对比节点数和 Pod 版本 | 0.80 | 节点有 taints 不允许调度（预期行为）；节点刚加入尚未同步 |
| SP-08 | 多个 ReplicaSet 同时存在且活跃 / Multiple active ReplicaSets | `kubectl get rs -n NS -l app=NAME -o wide` 显示多个 RS 的 READY > 0 | 0.75 | 正在进行滚动更新过程中（正常中间状态） |
| SP-09 | 金丝雀流量比例异常 / Canary traffic ratio anomaly | 检查 Service selector、Ingress 规则或 Service Mesh 配置，流量未按预期比例分配 | 0.70 | 流量切换存在传播延迟（需等待 1-2 分钟） |
| SP-10 | Available 副本数持续小于期望值 / Available replicas consistently below desired | `kubectl get deploy NAME -n NS` 的 AVAILABLE 列 < DESIRED 列超过 5 分钟 | 0.90 | minReadySeconds 设置导致新 Pod 需要更长时间才能计入 Available |
| SP-11 | rollout history 中缺少可回退的版本 / Missing rollback versions in history | `kubectl rollout history deploy/NAME -n NS` 显示版本数少于预期 | 0.85 | revisionHistoryLimit 设置过小或手动清理过 RS |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Deployment 更新卡住了，新 Pod 起不来"
- "发布失败，滚动更新一直没完成"
- "回滚操作没有生效，版本还是新版"
- "部署进度条卡在 1/3，无法继续"
- "StatefulSet 更新到一半停了，后面的 Pod 不更新"
- "DaemonSet 有些节点是新版本，有些是旧版本"
- "金丝雀发布流量异常，新版本没有收到请求"
- "蓝绿切换后服务不可用"
- "Pod 更新后持续 CrashLoop，无法完成部署"
- "ProgressDeadlineExceeded 告警，部署超时"

**English ticket descriptions**:
- "Deployment rollout stuck, new pods not starting"
- "Rolling update failed, deployment not progressing"
- "Rollback didn't work, still running new version"
- "Deployment shows 1/3 available, stuck there for hours"
- "StatefulSet update stopped at pod-2, remaining pods not updated"
- "DaemonSet has mixed versions across nodes"
- "Canary deployment not receiving traffic"
- "Blue-green switch caused service outage"
- "New pods keep crashing, deployment won't complete"
- "ProgressDeadlineExceeded, need help fixing deployment"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| Deployment/Pod 状态正常但应用内部功能异常 | 应用层排查 | 部署成功但业务逻辑有 bug，不属于 K8s 层面问题 |
| Pod Pending 但原因是资源不足（Insufficient cpu/memory） | SKILL-POD-002 | 调度问题，非滚动更新策略问题 |
| Pod Pending 但原因是 PVC 绑定失败 | SKILL-STORE-001 | 存储问题，非部署问题 |
| Service 无法路由到 Pod（但 Pod 状态正常） | SKILL-NET-001 | 网络/Service 配置问题 |
| HPA 自动扩缩容导致的 Pod 数量变化 | HPA 配置排查 | 非滚动更新导致的副本变化 |
| Job/CronJob 执行失败 | Job 排查 | 批处理作业，非持续运行的工作负载 |
| 仅镜像拉取失败但无其他更新操作 | SKILL-POD-001 | 单纯的镜像拉取问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断故障爆炸半径：

**Step T1**: 检查 Deployment 滚动更新状态（10s）
```bash
# 快速检查 Deployment 状态，确认是否卡住
kubectl rollout status deployment/NAME -n NS --timeout=10s
# 输出解读:
# - "deployment NAME successfully rolled out" → 更新已完成，可能是历史问题
# - "Waiting for deployment ..." → 更新正在进行中或卡住
# - error: timed out → 确认更新卡住
```
> **判断规则**:
> - 命令超时或显示 "Waiting for" → 确认滚动更新存在问题，继续 T2
> - 成功完成 → 检查是否为历史问题或已自动恢复

**Step T2**: 检查 ReplicaSet 演进状态（30s）
```bash
# 查看 ReplicaSet 历史和当前状态
kubectl get rs -n NS -l app=NAME --sort-by=.metadata.creationTimestamp -o wide
# 关注：
# - DESIRED vs READY vs CURRENT 的差异
# - 多个 RS 是否同时活跃
# - 最新 RS 的 READY 是否为 0
```
> **判断规则**:
> - 最新 RS 的 READY=0 且 DESIRED>0 → 新版本完全无法启动
> - 多个 RS 都有 READY>0 → 滚动更新卡在中间状态
> - 仅旧 RS 有 READY → 可能回滚已发生或新 RS 被删除

**Step T3**: 检查 Deployment Conditions（30s）
```bash
# 获取详细的 Deployment 条件
kubectl describe deployment NAME -n NS | grep -A10 "Conditions:"
# 或使用 JSON 格式
kubectl get deployment NAME -n NS -o jsonpath='{range .status.conditions[*]}{.type}: {.status} ({.reason}) - {.message}{"\n"}{end}'
```
> **判断规则**:
> - `Progressing: False (ProgressDeadlineExceeded)` → P1-P0，更新已超时
> - `Available: False` → P0，服务不可用
> - `Progressing: True (NewReplicaSetAvailable)` → 更新已完成
> - `Progressing: True (ReplicaSetUpdated)` → 更新正在进行

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| 生产环境主服务 Available=False **或** AVAILABLE=0 | **P0** | 服务完全不可用，影响用户访问。需立即响应 | 立即响应，15min 内恢复或回滚 |
| 滚动更新卡住（ProgressDeadlineExceeded）且 AVAILABLE < DESIRED | **P1** | 服务部分降级，可用副本不足。新功能无法上线 | 15min 内响应，30min 内修复或回滚 |
| 非关键服务更新失败 **或** 仅金丝雀副本失败 | **P2** | 影响有限，主流量仍由旧版本承载。但需及时修复以避免阻塞后续发布 | 30min 内响应，2h 内修复 |
| 旧版本清理问题 / revisionHistoryLimit 配置问题 / 历史版本丢失 | **P3** | 不影响当前服务运行，但影响回滚能力和资源清理 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **服务完全不可用**: Deployment 的 AVAILABLE=0 且 DESIRED>0，所有 Pod 都无法提供服务
- **回滚失败**: 执行 `kubectl rollout undo` 后状态仍未改善（5 分钟内）
- **数据一致性风险**: StatefulSet 更新涉及数据迁移且卡住，可能存在数据不一致
- **级联故障**: 多个关联服务的 Deployment 同时失败
- **安全紧急发布**: 正在进行安全漏洞修复的发布但失败，需权衡继续推进还是回滚

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 Deployment/ReplicaSet/Pod 状态信息，快速定位问题方向。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 Deployment 全局状态概览
- **命令**:
  ```bash
  kubectl get deploy NAME -n NS -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAME, READY, UP-TO-DATE, AVAILABLE, AGE, CONTAINERS, IMAGES, SELECTOR
- **判断规则**:
  - READY 格式为 `X/Y`，如果 X < Y → 部分副本未就绪
  - UP-TO-DATE < DESIRED → 新版本未完全部署
  - AVAILABLE < DESIRED → 可用副本不足
  - 命令超时 → apiserver 可能不可用
- **版本差异**: 无

**Step D1.2**: 获取 ReplicaSet 演进历史
- **命令**:
  ```bash
  kubectl get rs -n NS -l app=NAME --sort-by=.metadata.creationTimestamp -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 按时间排序的 ReplicaSet 列表，包含 DESIRED, CURRENT, READY, AGE, CONTAINERS, IMAGES
- **判断规则**:
  - 最新 RS（最后一行）的 READY=0 但 DESIRED>0 → 新 Pod 无法启动（RC-001, RC-002, RC-008）
  - 多个 RS 的 READY>0 → 滚动更新未完成（可能是正常中间状态或卡住）
  - 旧 RS 的 DESIRED 未减少 → 可能是 maxUnavailable/PDB 约束（RC-005, RC-006）
- **版本差异**: 无

**Step D1.3**: 获取新 ReplicaSet Pod 状态
- **命令**:
  ```bash
  # 找到最新的 ReplicaSet
  NEW_RS=$(kubectl get rs -n NS -l app=NAME --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1].metadata.name}')
  # 获取该 RS 的 Pod 状态
  kubectl get pods -n NS -l pod-template-hash=$(kubectl get rs $NEW_RS -n NS -o jsonpath='{.metadata.labels.pod-template-hash}') -o wide
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表，包含 NAME, READY, STATUS, RESTARTS, AGE, IP, NODE
- **判断规则**:
  - STATUS 为 `CrashLoopBackOff` → RC-001（新镜像启动失败）
  - STATUS 为 `ImagePullBackOff` 或 `ErrImagePull` → RC-008（镜像拉取失败）
  - STATUS 为 `Pending` → 可能是调度问题或 PVC 问题（关联其他 Skill）
  - STATUS 为 `Running` 但 READY 为 `0/1` → RC-002（readinessProbe 失败）
  - RESTARTS 数量高 → 容器反复崩溃
- **版本差异**: 无

**Step D1.4**: 获取滚动更新策略配置
- **命令**:
  ```bash
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.strategy}'
  ```
- **超时**: 5s
- **预期输出模式**: JSON 格式的 strategy 配置
  ```json
  {"type":"RollingUpdate","rollingUpdate":{"maxSurge":"25%","maxUnavailable":"25%"}}
  ```
- **判断规则**:
  - `maxUnavailable: 0` 且 `maxSurge: 0` → 配置错误，无法进行任何更新（RC-005）
  - `maxUnavailable: 0` → 必须先启动新 Pod 才能终止旧 Pod（需要额外资源）
  - `maxSurge: 0` → 必须先终止旧 Pod 才能启动新 Pod（可能导致容量不足）
- **版本差异**: 无

**Step D1.5**: 获取 Deployment Conditions
- **命令**:
  ```bash
  kubectl get deploy NAME -n NS -o jsonpath='{range .status.conditions[*]}{.type}: {.status} ({.reason}) - {.message}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: Deployment 条件列表
- **判断规则**:
  - `Progressing: False (ProgressDeadlineExceeded)` → RC-007（progressDeadlineSeconds 超时）
  - `Available: False (MinimumReplicasUnavailable)` → 可用副本数不足
  - `Progressing: True (NewReplicaSetAvailable)` → 滚动更新已完成
  - `Progressing: True (ReplicaSetUpdated)` → 更新正在进行中
- **版本差异**: 无

**Step D1.6**: 获取相关 Events
- **命令**:
  ```bash
  kubectl get events -n NS --field-selector involvedObject.name=NAME --sort-by=.lastTimestamp | tail -30
  # 同时获取新 RS 和 Pod 的事件
  kubectl get events -n NS --sort-by=.lastTimestamp | grep -E "(NAME|ReplicaSet|Pod)" | tail -50
  ```
- **超时**: 10s
- **预期输出模式**: 事件列表，关注 Warning 类型事件
- **判断规则**:
  - 出现 `ProgressDeadlineExceeded` → RC-007
  - 出现 `FailedCreate` → Pod 创建失败（检查 quota、PDB 等）
  - 出现 `FailedScheduling` → 调度失败（资源不足或亲和性约束）
  - 出现 `BackOff` → 容器启动失败
  - 出现 `Unhealthy` → 健康检查失败
  - 出现 `ImagePullBackOff` → 镜像拉取失败
- **版本差异**: 无

---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入分析新 Pod 失败原因，检查配置兼容性和资源约束。
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查新 Pod 详细失败原因
- **命令**:
  ```bash
  # 获取最新的失败 Pod
  FAILED_POD=$(kubectl get pods -n NS -l app=NAME --sort-by=.status.startTime -o jsonpath='{.items[-1].metadata.name}')
  kubectl describe pod $FAILED_POD -n NS
  ```
- **超时**: 15s
- **预期输出模式**: Pod 详细描述，关注 Events、Conditions、Container Status
- **判断规则**:
  - Events 中出现 `CrashLoopBackOff` → 容器启动后崩溃
  - Events 中出现 `CreateContainerConfigError` → 配置问题（ConfigMap/Secret 缺失）
  - Events 中出现 `RunContainerError` → 容器运行时错误
  - Container Status 中 `Exit Code: 1` → 应用启动失败
  - Container Status 中 `Exit Code: 137` → OOM Killed
  - Container Status 中 `Exit Code: 143` → SIGTERM（正常终止）
- **版本差异**: 无

**Step D2.2**: 获取容器日志（包括 previous）
- **命令**:
  ```bash
  # 获取当前容器日志
  kubectl logs $FAILED_POD -n NS --tail=100
  # 获取上一次容器日志（如果是 CrashLoop）
  kubectl logs $FAILED_POD -n NS --previous --tail=100 2>/dev/null || echo "No previous logs"
  ```
- **超时**: 15s
- **预期输出模式**: 应用日志
- **判断规则**:
  - 日志包含应用级错误（如数据库连接失败、配置解析错误）→ RC-001, RC-004
  - 日志显示健康检查路径问题（如 404、connection refused）→ RC-002
  - 日志显示权限错误（如文件权限、网络策略阻止）→ RC-004
  - 无日志或仅有启动前几行 → 容器启动时立即崩溃
- **版本差异**: 无

**Step D2.3**: 检查 readinessProbe 配置与响应
- **命令**:
  ```bash
  # 获取 readinessProbe 配置
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[*].readinessProbe}' | jq .
  
  # 如果 Pod 在运行，可以 exec 进去测试探针端点
  kubectl exec -n NS $FAILED_POD -- curl -s localhost:8080/health 2>/dev/null || echo "Cannot exec or endpoint unavailable"
  ```
- **超时**: 15s
- **预期输出模式**: readinessProbe 配置和探针响应
- **判断规则**:
  - `initialDelaySeconds` 过短（<应用启动时间）→ RC-002
  - `periodSeconds` 过短 + `failureThreshold` 过低 → 误判 Pod 不健康
  - 探针端点返回非 2xx 响应 → 应用未正确实现健康检查
  - 探针使用的端口/路径与应用不匹配 → RC-002
- **版本差异**: 无

**Step D2.4**: 资源约束分析
- **命令**:
  ```bash
  # 获取 Deployment 的资源配置
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[*].resources}' | jq .
  
  # 获取节点可用资源
  kubectl describe nodes | grep -A5 "Allocated resources"
  
  # 或获取简要的资源概览
  kubectl top nodes
  ```
- **超时**: 15s
- **预期输出模式**: 资源 requests/limits 和节点资源状态
- **判断规则**:
  - requests 超过任何单节点可用资源 → Pod 永远无法调度
  - limits.memory 过低导致 OOM → RC-001（Exit Code 137）
  - 无 resources 配置 → 可能被 LimitRange 注入默认值或与其他 Pod 争抢资源
- **版本差异**: 无

**Step D2.5**: 检查镜像可用性
- **命令**:
  ```bash
  # 获取新版本的镜像
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[*].image}'
  
  # 检查 Pod 事件中的镜像拉取信息
  kubectl get events -n NS --field-selector reason=Failed,reason=FailedToPullImage,reason=ImagePullBackOff
  
  # 检查镜像拉取策略
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[*].imagePullPolicy}'
  ```
- **超时**: 10s
- **预期输出模式**: 镜像名称、拉取策略和相关事件
- **判断规则**:
  - 镜像 tag 使用 `latest` + `imagePullPolicy: IfNotPresent` → 可能拉取到旧镜像（RC-008）
  - Events 显示 `repository does not exist` 或 `unauthorized` → 镜像仓库配置问题（RC-008）
  - Events 显示 `manifest unknown` → 指定的 tag 不存在（RC-008）
- **版本差异**: 无

**Step D2.6**: 检查 ConfigMap/Secret 变更影响
- **命令**:
  ```bash
  # 获取 Deployment 引用的 ConfigMaps
  kubectl get deploy NAME -n NS -o jsonpath='{range .spec.template.spec.volumes[*]}{.configMap.name}{"\n"}{end}'
  kubectl get deploy NAME -n NS -o jsonpath='{range .spec.template.spec.containers[*].envFrom[*]}{.configMapRef.name}{"\n"}{end}'
  
  # 获取引用的 Secrets
  kubectl get deploy NAME -n NS -o jsonpath='{range .spec.template.spec.volumes[*]}{.secret.secretName}{"\n"}{end}'
  kubectl get deploy NAME -n NS -o jsonpath='{range .spec.template.spec.containers[*].envFrom[*]}{.secretRef.name}{"\n"}{end}'
  
  # 检查 ConfigMap/Secret 是否存在
  # 对每个引用的 ConfigMap/Secret 执行
  kubectl get configmap CM_NAME -n NS
  kubectl get secret SECRET_NAME -n NS
  ```
- **超时**: 15s
- **预期输出模式**: ConfigMap/Secret 名称列表及其存在性
- **判断规则**:
  - 引用的 ConfigMap/Secret 不存在 → RC-004，Pod 会 CreateContainerConfigError
  - ConfigMap/Secret 内容变更导致应用配置不兼容 → RC-004
  - 使用 `optional: false`（默认）但资源不存在 → Pod 无法启动
- **版本差异**: 无

**Step D2.7**: 检查 PodDisruptionBudget 约束
- **命令**:
  ```bash
  # 获取相关 PDB
  kubectl get pdb -n NS -o wide
  
  # 检查 PDB 状态
  kubectl get pdb -n NS -o jsonpath='{range .items[*]}{.metadata.name}: min={.spec.minAvailable}, max={.spec.maxUnavailable}, current={.status.currentHealthy}, desired={.status.desiredHealthy}, disruptions={.status.disruptionsAllowed}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: PDB 配置和状态
- **判断规则**:
  - `disruptionsAllowed: 0` → PDB 阻止了旧 Pod 的终止，导致更新无法进行（RC-006）
  - `minAvailable` 过高（接近或等于 replicas）→ 几乎不允许任何中断（RC-006）
  - `maxUnavailable: 0` → 同上，过于严格的 PDB 配置
- **版本差异**: 无

**Step D2.8**: 检查 Revision 历史
- **命令**:
  ```bash
  # 获取滚动更新历史
  kubectl rollout history deployment/NAME -n NS
  
  # 获取特定 revision 的详细信息（如果需要回滚参考）
  kubectl rollout history deployment/NAME -n NS --revision=2
  
  # 检查 revisionHistoryLimit 配置
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.revisionHistoryLimit}'
  ```
- **超时**: 10s
- **预期输出模式**: Revision 列表和 revisionHistoryLimit 值
- **判断规则**:
  - revisionHistoryLimit=0 → 无法回滚（RC-009）
  - Revision 数量少于预期 → 可能 RS 被清理或 limit 设置过小
  - 最新 revision 的 CHANGE-CAUSE 可帮助定位变更内容
- **版本差异**: 无

---

### Phase 3: 高级诊断（只读/低风险）

> **目标**: 针对特定工作负载类型（StatefulSet、DaemonSet）和高级场景（金丝雀、Webhook）进行深度分析。
> **预计耗时**: 5-15 分钟
> ⚠️ 以下部分步骤可能涉及 API 查询较多，请注意频率

**Step D3.1**: StatefulSet 有序更新分析
- **命令**:
  ```bash
  # 仅当工作负载是 StatefulSet 时执行
  kubectl get pods -n NS -l app=NAME -o wide --sort-by=.metadata.name
  
  # 检查 StatefulSet 更新策略
  kubectl get sts NAME -n NS -o jsonpath='{.spec.updateStrategy}'
  
  # 检查 partition 配置（用于分批更新）
  kubectl get sts NAME -n NS -o jsonpath='{.spec.updateStrategy.rollingUpdate.partition}'
  
  # 检查 Pod 的 controller-revision-hash
  kubectl get pods -n NS -l app=NAME -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.labels.controller-revision-hash}{"\n"}{end}'
  ```
- **超时**: 15s
- **预期输出模式**: Pod 列表（按 ordinal 排序）和更新策略
- **判断规则**:
  - 不同 Pod 有不同的 controller-revision-hash → 更新进行中或卡住
  - ordinal 较高的 Pod 未更新但 partition 已降低 → 更新可能卡住（RC-010）
  - 某个 ordinal 的 Pod 持续 Pending/CrashLoop → 该 Pod 阻塞后续更新
- **版本差异**:
  - **[v1.31+]**: StatefulSet 支持 `maxUnavailable`（Beta），允许并行更新多个 Pod

**Step D3.2**: DaemonSet 节点覆盖分析
- **命令**:
  ```bash
  # 仅当工作负载是 DaemonSet 时执行
  # 获取 DaemonSet Pod 分布
  kubectl get pods -n NS -l app=NAME -o wide
  
  # 获取 DaemonSet 状态
  kubectl get ds NAME -n NS -o wide
  
  # 对比节点数和 Pod 数
  echo "Nodes: $(kubectl get nodes --no-headers | wc -l)"
  echo "DaemonSet Pods: $(kubectl get pods -n NS -l app=NAME --no-headers | wc -l)"
  
  # 检查 DaemonSet 更新策略
  kubectl get ds NAME -n NS -o jsonpath='{.spec.updateStrategy}'
  
  # 检查 DaemonSet tolerations
  kubectl get ds NAME -n NS -o jsonpath='{.spec.template.spec.tolerations}' | jq .
  ```
- **超时**: 15s
- **预期输出模式**: DaemonSet Pod 分布和节点列表
- **判断规则**:
  - DaemonSet Pod 数 < 节点数 → 部分节点无 Pod（检查 tolerations）（RC-011）
  - 某些节点上 Pod 版本不一致 → 更新卡住或节点刚恢复
  - `updateStrategy.type: OnDelete` → 需要手动删除旧 Pod 才能触发更新
- **版本差异**:
  - **[v1.28+]**: DaemonSet 支持 `maxSurge`（GA），允许创建超过节点数的 Pod

**Step D3.3**: Admission Webhook 拦截检查
- **命令**:
  ```bash
  # 获取 ValidatingWebhookConfiguration
  kubectl get validatingwebhookconfigurations -o wide
  
  # 获取 MutatingWebhookConfiguration
  kubectl get mutatingwebhookconfigurations -o wide
  
  # 检查是否有针对 Deployment/Pod 的 Webhook
  kubectl get validatingwebhookconfigurations -o jsonpath='{range .items[*]}{.metadata.name}: {range .webhooks[*]}{.rules[*].resources}{"\n"}{end}{end}' | grep -i "pods\|deployments"
  
  # 检查 Webhook 的 failurePolicy
  kubectl get validatingwebhookconfigurations -o jsonpath='{range .items[*]}{.metadata.name}: failurePolicy={.webhooks[*].failurePolicy}{"\n"}{end}'
  ```
- **超时**: 15s
- **预期输出模式**: Webhook 配置列表
- **判断规则**:
  - 存在针对 pods 的 Webhook 且 `failurePolicy: Fail` → Webhook 故障可能阻止 Pod 创建（RC-012）
  - Webhook endpoint 不可达 → Pod 创建被拒绝
  - 检查 Events 中是否有 `admission webhook denied the request` 消息
- **版本差异**: 无

**Step D3.4**: ResourceQuota 分析
- **命令**:
  ```bash
  # 获取 namespace 的 ResourceQuota
  kubectl describe quota -n NS
  
  # 或以 JSON 格式获取
  kubectl get quota -n NS -o jsonpath='{range .items[*]}{.metadata.name}: hard={.spec.hard}, used={.status.used}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: ResourceQuota 配置和使用情况
- **判断规则**:
  - `pods` quota 已用尽 → 无法创建新 Pod
  - `requests.cpu/memory` 已接近上限 → 新 Pod 可能超出 quota
  - `count/deployments.apps` 限制 → 无法创建新 Deployment（较少见）
- **版本差异**: 无

**Step D3.5**: Controller Manager 日志检查
- **命令**:
  ```bash
  # 获取 kube-controller-manager 日志（需要控制平面访问权限）
  kubectl logs -n kube-system -l component=kube-controller-manager --tail=200 | grep -i "deployment\|replicaset\|NAME"
  
  # 或通过 API 获取（如果有权限）
  kubectl get --raw /logs/kube-controller-manager.log 2>/dev/null | tail -200 | grep -i "NAME"
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: Controller 日志条目
- **判断规则**:
  - 日志中出现 `error syncing` + Deployment 名称 → Controller 处理异常
  - 日志中出现 `failed to create` → Pod/RS 创建失败的详细原因
  - 日志中出现 `scale down` 但实际未缩容 → 可能是 PDB 约束
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 | 修复难度 |
|--------|------|------|---------|---------|---------|
| RC-001 | **新镜像启动失败（配置错误/代码 bug）** — 新版本应用存在启动时错误，无法正确初始化，导致容器反复 CrashLoop | ~25% | D1.3 STATUS=CrashLoopBackOff；D2.1 Exit Code 非 0；D2.2 日志显示应用错误 | workload-fta: BE-app-crash | 🟢 |
| RC-002 | **readinessProbe 配置不当** — 探针配置与应用实际行为不匹配，导致健康的 Pod 被误判为不健康 | ~15% | D1.3 READY=0/1 但 STATUS=Running；D2.3 探针配置分析；D2.2 无明显错误日志 | workload-fta: BE-probe-miscfg | 🟢 |
| RC-003 | **资源不足（CPU/Memory requests 超过可用）** — 新版本的资源请求超过集群可用资源，Pod 无法调度 | ~12% | D1.3 STATUS=Pending；D2.4 requests > 可用资源；Events 显示 FailedScheduling | workload-fta: BE-resource-insufficient | 🟡 |
| RC-004 | **ConfigMap/Secret 变更不兼容** — 配置更新后应用无法解析新配置，或引用的配置资源不存在 | ~10% | D2.6 ConfigMap/Secret 缺失或变更；D2.1 CreateContainerConfigError；D2.2 配置解析错误日志 | workload-fta: BE-config-error | 🟡 |
| RC-005 | **maxUnavailable/maxSurge 配置导致死锁** — 策略配置过于保守（如两者都为 0），导致更新无法进行 | ~8% | D1.4 strategy 配置分析；更新进度为 0/N 且无 Pod 变化 | workload-fta: BE-strategy-deadlock | 🟢 |
| RC-006 | **PDB 约束过严导致无法驱逐旧 Pod** — PodDisruptionBudget 配置不允许任何 disruption，旧 Pod 无法被终止 | ~7% | D2.7 disruptionsAllowed=0；旧 RS 副本数不变；Events 中可能有 PDB 相关信息 | workload-fta: BE-pdb-block | 🟡 |
| RC-007 | **progressDeadlineSeconds 过短** — 应用启动时间较长，但 deadline 设置过短，导致正常更新被误判为失败 | ~6% | D1.5 ProgressDeadlineExceeded；应用启动需要较长时间；deadline 配置值较小 | workload-fta: BE-deadline-short | 🟢 |
| RC-008 | **镜像拉取失败** — 镜像不存在、仓库认证失败、网络问题导致无法拉取新版本镜像 | ~5% | D1.3 STATUS=ImagePullBackOff/ErrImagePull；D2.5 镜像配置分析；Events 显示拉取错误 | workload-fta: BE-image-pull-fail | 🟡 |
| RC-009 | **revisionHistoryLimit=0 导致无法回滚** — 历史 RS 被立即清理，无可回退版本 | ~4% | D2.8 revisionHistoryLimit=0；rollout history 无历史版本；undo 失败 | workload-fta: BE-no-rollback | 🟢 |
| RC-010 | **StatefulSet PVC 绑定约束** — StatefulSet 更新时 PVC 无法绑定（StorageClass 问题或容量不足） | ~3% | D3.1 StatefulSet Pod Pending；Events 显示 PVC 相关错误；PV 状态异常 | workload-fta: BE-sts-pvc-bind | 🔴 |
| RC-011 | **DaemonSet tolerations 变更导致部分节点无法调度** — 更新后 tolerations 配置变化，部分节点不再被容忍 | ~3% | D3.2 DaemonSet Pod 数 < 节点数；tolerations 配置变更；特定节点无 Pod | workload-fta: BE-ds-toleration | 🟡 |
| RC-012 | **Admission Webhook 拒绝新 Pod** — 准入控制 Webhook 因安全策略或验证失败阻止了新 Pod 创建 | ~2% | D3.3 Webhook 存在且针对 pods；Events 显示 admission denied；Webhook endpoint 可能异常 | workload-fta: BE-webhook-deny | 🔴 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 修正 readinessProbe 配置
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 确认问题确实是 readinessProbe 导致
  kubectl get pods -n NS -l app=NAME --sort-by=.status.startTime -o wide | head -5
  # 预期: STATUS=Running 但 READY=0/1
  
  # 获取当前 probe 配置
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[0].readinessProbe}' | jq .
  ```
- **执行命令**:
  ```bash
  # 方案 A: 增加 initialDelaySeconds（如果应用启动慢）
  kubectl patch deployment NAME -n NS --type='json' -p='[{"op": "replace", "path": "/spec/template/spec/containers/0/readinessProbe/initialDelaySeconds", "value": 30}]'
  
  # 方案 B: 增加 failureThreshold（容忍更多失败）
  kubectl patch deployment NAME -n NS --type='json' -p='[{"op": "replace", "path": "/spec/template/spec/containers/0/readinessProbe/failureThreshold", "value": 5}]'
  
  # 方案 C: 修正探针端口或路径（根据实际应用配置）
  kubectl patch deployment NAME -n NS --type='json' -p='[{"op": "replace", "path": "/spec/template/spec/containers/0/readinessProbe/httpGet/path", "value": "/healthz"}]'
  ```
- **后置验证**:
  ```bash
  # 等待新 Pod 启动
  sleep 60
  kubectl get pods -n NS -l app=NAME --sort-by=.status.startTime -o wide | head -5
  # 预期: STATUS=Running, READY=1/1
  
  kubectl rollout status deployment/NAME -n NS --timeout=120s
  # 预期: "deployment NAME successfully rolled out"
  ```
- **回滚命令**:
  ```bash
  # 恢复原配置
  kubectl rollout undo deployment/NAME -n NS
  ```

#### REM-002: 调整 maxUnavailable/maxSurge 策略
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  # 确认当前策略
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.strategy}'
  # 预期: 发现 maxUnavailable 和 maxSurge 配置过于保守
  ```
- **执行命令**:
  ```bash
  # 设置合理的滚动更新策略
  kubectl patch deployment NAME -n NS -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":"25%","maxSurge":"25%"}}}}'
  
  # 或者使用固定数值（适用于小规模 Deployment）
  kubectl patch deployment NAME -n NS -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":1,"maxSurge":1}}}}'
  ```
- **后置验证**:
  ```bash
  # 确认策略已更新
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.strategy}'
  
  # 触发新的滚动更新（如果更新已卡住）
  kubectl rollout restart deployment/NAME -n NS
  
  # 监控更新进度
  kubectl rollout status deployment/NAME -n NS --timeout=300s
  ```
- **回滚命令**:
  ```bash
  # 恢复原策略
  kubectl patch deployment NAME -n NS -p '{"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":"0","maxSurge":"0"}}}}'
  ```

#### REM-003: 延长 progressDeadlineSeconds
- **适用根因**: RC-007
- **前置检查**:
  ```bash
  # 确认当前 deadline 设置
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.progressDeadlineSeconds}'
  # 默认值: 600（10 分钟）
  
  # 确认应用实际启动时间
  kubectl logs -n NS -l app=NAME --tail=50 | head -20
  ```
- **执行命令**:
  ```bash
  # 延长 deadline（例如设置为 20 分钟）
  kubectl patch deployment NAME -n NS -p '{"spec":{"progressDeadlineSeconds":1200}}'
  
  # 触发新的更新周期
  kubectl rollout restart deployment/NAME -n NS
  ```
- **后置验证**:
  ```bash
  # 确认配置已更新
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.progressDeadlineSeconds}'
  
  # 监控更新进度
  kubectl rollout status deployment/NAME -n NS --timeout=1200s
  ```
- **回滚命令**:
  ```bash
  # 恢复原 deadline
  kubectl patch deployment NAME -n NS -p '{"spec":{"progressDeadlineSeconds":600}}'
  ```

#### REM-004: 执行 rollout undo 快速回滚
- **适用根因**: RC-001, RC-004, RC-008（当需要快速恢复服务时）
- **前置检查**:
  ```bash
  # 确认有可回滚的版本
  kubectl rollout history deployment/NAME -n NS
  # 预期: 至少有 2 个 revision
  
  # 查看上一个版本的详情
  kubectl rollout history deployment/NAME -n NS --revision=$(kubectl rollout history deployment/NAME -n NS | tail -2 | head -1 | awk '{print $1}')
  ```
- **执行命令**:
  ```bash
  # 回滚到上一个版本
  kubectl rollout undo deployment/NAME -n NS
  
  # 或回滚到指定版本
  kubectl rollout undo deployment/NAME -n NS --to-revision=X
  ```
- **后置验证**:
  ```bash
  # 确认回滚完成
  kubectl rollout status deployment/NAME -n NS --timeout=300s
  
  # 确认当前运行版本
  kubectl get deploy NAME -n NS -o jsonpath='{.spec.template.spec.containers[0].image}'
  
  # 确认服务可用
  kubectl get deploy NAME -n NS
  # 预期: AVAILABLE = DESIRED
  ```
- **回滚命令**:
  ```bash
  # 如果回滚有问题，可以再次 undo 回到之前版本
  kubectl rollout undo deployment/NAME -n NS
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: 修复 ConfigMap/Secret 兼容性
- **适用根因**: RC-004
- **影响说明**: 修改 ConfigMap/Secret 可能影响所有引用它的 Pod。如果配置错误，可能导致更大范围的故障。建议在非生产环境验证后再应用。
- **审批提示**: "建议修复 ConfigMap/Secret `CM_NAME` 的配置。该配置被 Deployment `NAME` 引用，修改将触发 Pod 重启。是否批准？"
- **前置检查**:
  ```bash
  # 确认 ConfigMap/Secret 问题
  kubectl get configmap CM_NAME -n NS -o yaml
  kubectl get secret SECRET_NAME -n NS -o yaml
  
  # 检查哪些 Deployment 引用了此配置
  kubectl get deploy -n NS -o jsonpath='{range .items[*]}{.metadata.name}: {.spec.template.spec.volumes[*].configMap.name}{"\n"}{end}' | grep CM_NAME
  ```
- **执行命令**:
  ```bash
  # 方案 A: 修复 ConfigMap 内容
  kubectl edit configmap CM_NAME -n NS
  # 或使用 patch
  kubectl patch configmap CM_NAME -n NS -p '{"data":{"key":"corrected-value"}}'
  
  # 方案 B: 如果 ConfigMap/Secret 不存在，创建它
  kubectl create configmap CM_NAME -n NS --from-literal=key=value
  
  # 触发 Pod 重启以加载新配置（如果 Pod 未自动重启）
  kubectl rollout restart deployment/NAME -n NS
  ```
- **后置验证**:
  ```bash
  # 确认 Pod 使用了新配置
  kubectl exec -n NS POD_NAME -- cat /path/to/config/file
  # 或检查环境变量
  kubectl exec -n NS POD_NAME -- env | grep CONFIG_KEY
  
  # 确认 Deployment 更新完成
  kubectl rollout status deployment/NAME -n NS --timeout=300s
  ```
- **回滚命令**:
  ```bash
  # 恢复原 ConfigMap 内容（需要有备份）
  kubectl apply -f configmap-backup.yaml
  kubectl rollout restart deployment/NAME -n NS
  ```

#### REM-006: 调整 PDB minAvailable/maxUnavailable
- **适用根因**: RC-006
- **影响说明**: 调整 PDB 可能在某些场景下降低服务可用性保障。但如果 PDB 配置过于严格导致无法进行任何更新，则需要权衡。
- **审批提示**: "建议调整 PDB `PDB_NAME` 的配置，当前 `disruptionsAllowed=0` 阻止了滚动更新。修改后将允许一定数量的 Pod 中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认 PDB 配置
  kubectl get pdb PDB_NAME -n NS -o yaml
  
  # 确认当前 disruption 状态
  kubectl get pdb PDB_NAME -n NS -o jsonpath='{.status}'
  ```
- **执行命令**:
  ```bash
  # 方案 A: 调整 maxUnavailable（允许一定数量不可用）
  kubectl patch pdb PDB_NAME -n NS -p '{"spec":{"maxUnavailable":1}}'
  
  # 方案 B: 调整 minAvailable（降低最小可用要求）
  kubectl patch pdb PDB_NAME -n NS -p '{"spec":{"minAvailable":"80%"}}'
  
  # 方案 C: 临时删除 PDB（风险较高，仅紧急情况）
  kubectl delete pdb PDB_NAME -n NS
  ```
- **后置验证**:
  ```bash
  # 确认 PDB 已更新
  kubectl get pdb PDB_NAME -n NS -o jsonpath='{.status.disruptionsAllowed}'
  # 预期: > 0
  
  # 触发更新继续
  kubectl rollout restart deployment/NAME -n NS
  kubectl rollout status deployment/NAME -n NS --timeout=300s
  ```
- **回滚命令**:
  ```bash
  # 恢复原 PDB 配置
  kubectl patch pdb PDB_NAME -n NS -p '{"spec":{"minAvailable":"100%"}}'
  # 或重新创建 PDB
  kubectl apply -f pdb-backup.yaml
  ```

#### REM-007: 强制替换 Deployment
- **适用根因**: RC-001, RC-005（当 Deployment 状态异常难以恢复时）
- **影响说明**: 强制替换会删除并重建 Deployment 对象，这将导致所有关联的 ReplicaSet 和 Pod 被清理。服务将短暂中断直到新 Pod 启动。
- **审批提示**: "建议强制替换 Deployment `NAME`。此操作将导致服务短暂中断（约 1-3 分钟）。是否批准？"
- **前置检查**:
  ```bash
  # 备份当前 Deployment 配置
  kubectl get deploy NAME -n NS -o yaml > deployment-backup.yaml
  
  # 确认备份文件有效
  cat deployment-backup.yaml | head -30
  ```
- **执行命令**:
  ```bash
  # 强制替换 Deployment
  kubectl replace --force -f deployment-backup.yaml
  
  # 或使用 delete + apply（效果相同）
  # kubectl delete deploy NAME -n NS
  # kubectl apply -f deployment-backup.yaml
  ```
- **后置验证**:
  ```bash
  # 等待新 Deployment 就绪
  sleep 30
  kubectl get deploy NAME -n NS
  kubectl rollout status deployment/NAME -n NS --timeout=300s
  
  # 确认 Pod 正常运行
  kubectl get pods -n NS -l app=NAME
  ```
- **回滚命令**:
  ```bash
  # 使用备份文件恢复（但版本会是替换前的版本）
  kubectl apply -f deployment-backup.yaml
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: StatefulSet 分区更新（partition）
- **适用根因**: RC-010
- **影响说明**: 使用 partition 策略可以控制 StatefulSet 更新范围，仅更新 ordinal >= partition 的 Pod。这是一种安全的渐进式更新方法，但需要手动逐步降低 partition 值。
- **操作步骤**:
  1. **确认当前 StatefulSet 状态**:
     ```bash
     kubectl get sts NAME -n NS -o wide
     kubectl get pods -n NS -l app=NAME -o wide --sort-by=.metadata.name
     ```
  2. **设置 partition 值，仅更新最后一个 Pod**:
     ```bash
     # 假设有 3 个副本 (pod-0, pod-1, pod-2)，设置 partition=2 只更新 pod-2
     kubectl patch sts NAME -n NS -p '{"spec":{"updateStrategy":{"type":"RollingUpdate","rollingUpdate":{"partition":2}}}}'
     ```
  3. **验证 pod-2 更新成功**:
     ```bash
     kubectl get pods -n NS -l app=NAME -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.labels.controller-revision-hash}{"\n"}{end}'
     ```
  4. **逐步降低 partition 更新更多 Pod**:
     ```bash
     kubectl patch sts NAME -n NS -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":1}}}}'
     # 等待 pod-1 更新完成
     kubectl patch sts NAME -n NS -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'
     # 等待 pod-0 更新完成
     ```
  5. **确认所有 Pod 更新完成**:
     ```bash
     kubectl rollout status sts/NAME -n NS
     ```
- **安全检查**:
  - 确保每个 Pod 更新后应用功能正常再继续下一个
  - 对于数据库等有状态服务，确认数据复制正常
- **回滚方案**:
  ```bash
  # 如果某个 Pod 更新失败，可以回滚整个 StatefulSet
  kubectl rollout undo sts/NAME -n NS
  # 或设置 partition 到一个高值暂停更新
  kubectl patch sts NAME -n NS -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":99}}}}'
  ```

#### REM-009: 手动缩容旧 RS + 扩容新 RS
- **适用根因**: RC-005, RC-006（当自动滚动更新失败且无法修复策略时）
- **影响说明**: 手动操作 ReplicaSet 的副本数来完成更新。这绕过了 Deployment Controller 的正常逻辑，可能导致服务容量波动。
- **操作步骤**:
  1. **识别新旧 ReplicaSet**:
     ```bash
     kubectl get rs -n NS -l app=NAME --sort-by=.metadata.creationTimestamp
     # 记录：新 RS = NEW_RS，旧 RS = OLD_RS
     ```
  2. **手动扩容新 RS（确保有足够的新 Pod）**:
     ```bash
     kubectl scale rs NEW_RS -n NS --replicas=DESIRED_REPLICAS
     # 等待新 Pod Ready
     kubectl get rs NEW_RS -n NS -w
     ```
  3. **验证新 Pod 功能正常**:
     ```bash
     # 检查新 Pod 日志和健康状态
     kubectl logs -n NS -l app=NAME --tail=50
     kubectl exec -n NS NEW_POD -- curl -s localhost:8080/health
     ```
  4. **手动缩容旧 RS**:
     ```bash
     kubectl scale rs OLD_RS -n NS --replicas=0
     # 等待旧 Pod 终止
     kubectl get rs OLD_RS -n NS -w
     ```
  5. **验证 Deployment 状态**:
     ```bash
     kubectl get deploy NAME -n NS
     # 预期: AVAILABLE = DESIRED，UP-TO-DATE = DESIRED
     ```
- **安全检查**:
  - 在缩容旧 RS 前确保新 RS 的 Pod 已经 Ready 且服务正常
  - 监控服务健康状况，避免容量不足
- **回滚方案**:
  ```bash
  # 如果新版本有问题，反向操作
  kubectl scale rs OLD_RS -n NS --replicas=DESIRED_REPLICAS
  kubectl scale rs NEW_RS -n NS --replicas=0
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: 重建 Deployment（删除并重新创建）
- **适用根因**: RC-001, RC-005, RC-012（当 Deployment 对象损坏或状态严重异常时）
- **审批要求**: 需要高级 SRE + 应用 Owner 审批
- **数据备份**: 必须备份当前 Deployment 配置
- **操作步骤**:
  1. **完整备份当前 Deployment 及相关资源**:
     ```bash
     kubectl get deploy NAME -n NS -o yaml > deploy-backup.yaml
     kubectl get svc -n NS -l app=NAME -o yaml > svc-backup.yaml
     kubectl get configmap -n NS -l app=NAME -o yaml > cm-backup.yaml
     kubectl get secret -n NS -l app=NAME -o yaml > secret-backup.yaml
     kubectl get hpa -n NS -l app=NAME -o yaml > hpa-backup.yaml 2>/dev/null || true
     kubectl get pdb -n NS -l app=NAME -o yaml > pdb-backup.yaml 2>/dev/null || true
     ```
  2. **通知相关团队服务即将中断**:
     ```bash
     # 发送通知（根据组织流程）
     echo "Service NAME in namespace NS will be recreated. Expected downtime: 2-5 minutes."
     ```
  3. **删除 Deployment（会级联删除 RS 和 Pod）**:
     ```bash
     kubectl delete deploy NAME -n NS
     # 等待所有 Pod 终止
     kubectl get pods -n NS -l app=NAME -w
     ```
  4. **修复 Deployment 配置（根据诊断结果）**:
     ```bash
     # 编辑 deploy-backup.yaml，修复导致问题的配置
     vi deploy-backup.yaml
     # 移除 status 字段和 resourceVersion
     ```
  5. **重新创建 Deployment**:
     ```bash
     kubectl apply -f deploy-backup.yaml
     ```
  6. **验证服务恢复**:
     ```bash
     kubectl rollout status deployment/NAME -n NS --timeout=300s
     kubectl get deploy NAME -n NS
     kubectl get pods -n NS -l app=NAME
     ```
- **回滚方案**:
  ```bash
  # 如果重建后仍有问题，可以使用原始备份
  # 但需要先删除当前 Deployment
  kubectl delete deploy NAME -n NS
  kubectl apply -f deploy-backup-original.yaml
  ```

#### REM-011: 金丝雀/蓝绿部署流量切换修复
- **适用根因**: 高级部署模式故障
- **审批要求**: 需要高级 SRE + 发布工程师审批
- **操作步骤**:
  1. **确认当前流量路由配置**:
     ```bash
     # 检查 Service selector
     kubectl get svc SERVICE_NAME -n NS -o jsonpath='{.spec.selector}'
     
     # 检查 Ingress 规则（如果使用 Ingress）
     kubectl get ingress -n NS -o yaml
     
     # 检查 Service Mesh 配置（如 Istio VirtualService）
     kubectl get virtualservice -n NS -o yaml 2>/dev/null
     ```
  2. **方案 A: 切换 Service selector 回滚流量**:
     ```bash
     # 将 Service 指向稳定版本的 Deployment
     kubectl patch svc SERVICE_NAME -n NS -p '{"spec":{"selector":{"version":"stable"}}}'
     ```
  3. **方案 B: 调整 Ingress/VirtualService 权重**:
     ```bash
     # Istio 示例：将 100% 流量切回稳定版本
     kubectl patch virtualservice VS_NAME -n NS --type='json' -p='[
       {"op": "replace", "path": "/spec/http/0/route/0/weight", "value": 100},
       {"op": "replace", "path": "/spec/http/0/route/1/weight", "value": 0}
     ]'
     ```
  4. **验证流量切换**:
     ```bash
     # 检查请求是否路由到正确版本
     for i in {1..10}; do curl -s http://SERVICE_ENDPOINT/version; done
     ```
- **安全检查**:
  - 确认目标版本（稳定版）的 Pod 都在运行且健康
  - 观察切换后的错误率和延迟指标
- **回滚方案**:
  ```bash
  # 恢复原流量配置
  kubectl apply -f traffic-config-backup.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认 rollout 状态
kubectl rollout status deployment/NAME -n NS --timeout=60s
# 预期: "deployment NAME successfully rolled out"

# V2: 确认 Deployment 副本数正常
kubectl get deploy NAME -n NS
# 预期: READY = DESIRED = UP-TO-DATE = AVAILABLE

# V3: 确认所有 Pod 运行正常
kubectl get pods -n NS -l app=NAME
# 预期: 所有 Pod STATUS=Running, READY=1/1

# V4: 确认 ReplicaSet 状态
kubectl get rs -n NS -l app=NAME
# 预期: 仅最新 RS 有 READY > 0，旧 RS 的 READY = 0

# V5: 确认 Deployment Conditions
kubectl get deploy NAME -n NS -o jsonpath='{range .status.conditions[*]}{.type}: {.status}{"\n"}{end}'
# 预期: 
# Available: True
# Progressing: True
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pod 重启次数 | `kube_pod_container_status_restarts_total{pod=~"NAME.*"}` | 保持稳定，无增长 | 任何 Pod 重启 > 0 次 |
| 应用错误率 | 业务监控指标或 `http_requests_total{status=~"5.."}` | 5xx 错误率 < 0.1% | 5xx 错误率 > 1% |
| 响应延迟 | `http_request_duration_seconds{quantile="0.99"}` | P99 延迟在正常范围内 | P99 延迟增加 > 50% |
| 资源使用 | `container_cpu_usage_seconds_total`, `container_memory_usage_bytes` | CPU/Memory 使用稳定 | CPU > 90% 或 Memory 接近 limits |
| 健康检查 | `kube_pod_container_status_ready` | 所有 Pod Ready | 任何 Pod Not Ready |
| Deployment 状态 | `kube_deployment_status_replicas_available` | Available = Desired | Available < Desired |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：

- [ ] `kubectl rollout status` 显示 "successfully rolled out"
- [ ] Deployment 的 READY、UP-TO-DATE、AVAILABLE 都等于 DESIRED
- [ ] 所有 Pod 状态为 Running 且 READY 为 1/1（或预期值）
- [ ] 无新的 CrashLoopBackOff 或 ImagePullBackOff
- [ ] Deployment Conditions 中 Available=True, Progressing=True
- [ ] 应用健康检查通过（readiness/liveness probe）
- [ ] 业务功能验证通过（API 测试、端到端测试）
- [ ] 5 分钟内无 Pod 重启
- [ ] 监控指标（错误率、延迟）在正常范围内

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pod 稳定性 | `kubectl get pods -n NS -l app=NAME` | 每 4 小时 | 如有新的 CrashLoop → 检查应用日志 |
| 资源趋势 | Prometheus/Grafana 资源监控 | 持续 | 资源使用持续上升 → 检查内存泄漏或配置问题 |
| 下次发布验证 | 在非生产环境执行相同更新 | 下次发布前 | 如果复现问题 → 修复根因后再上生产 |
| 回滚能力验证 | `kubectl rollout history` | 发布后立即 | 确认有可回滚版本，revisionHistoryLimit 配置合理 |
| PDB 配置审计 | `kubectl get pdb -n NS` | 每周 | 确保 PDB 配置既保护可用性又允许正常更新 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **服务完全不可用** | Deployment AVAILABLE=0 且持续超过 **5 分钟** | 快速分级阶段确认 |
| **回滚失败** | 执行 `rollout undo` 后 **5 分钟**内状态未改善 | REM-004 执行后验证失败 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常发现 |
| **数据一致性风险** | StatefulSet 更新涉及数据迁移且卡住 | D3.1 发现 StatefulSet 更新阻塞 |

### 8.2 升级消息模板

```
【{severity}】Deployment 滚动更新故障 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 故障概述: Deployment {deployment_name} 在 namespace {namespace} 中滚动更新失败，持续 {duration}
- 影响范围: 
  - 期望副本数: {desired_replicas}
  - 可用副本数: {available_replicas}
  - 是否影响生产流量: {production_impact}
  - 关联服务: {related_services}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 高级诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
  - 新版本镜像: {new_image}
  - 旧版本镜像: {old_image}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-WORK-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.5）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-008 已排除 — D2.5 显示镜像拉取成功，无 ImagePullBackOff 事件"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-002（readinessProbe 配置）— Pod Running 但 Ready 0/1，日志无错误"
4. **关键资源快照**:
   ```bash
   # Deployment 描述
   kubectl describe deploy NAME -n NS > deploy-describe.txt
   # ReplicaSet 状态
   kubectl get rs -n NS -l app=NAME -o wide > rs-status.txt
   # Pod 状态和事件
   kubectl get pods -n NS -l app=NAME -o wide > pods-status.txt
   kubectl describe pods -n NS -l app=NAME > pods-describe.txt
   # 最新 Pod 日志
   kubectl logs -n NS -l app=NAME --tail=200 > pod-logs.txt
   kubectl logs -n NS -l app=NAME --previous --tail=200 > pod-logs-previous.txt 2>/dev/null
   # 相关事件
   kubectl get events -n NS --sort-by=.lastTimestamp > events.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到滚动更新失败
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 发现新 Pod CrashLoopBackOff
   - `HH:MM:SS` - 尝试 rollout undo
   - `HH:MM:SS` - 回滚结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Deployment maxSurge/maxUnavailable | GA | GA | GA | GA | GA |
| StatefulSet maxUnavailable | alpha | beta | beta | beta | GA |
| DaemonSet maxSurge | GA | GA | GA | GA | GA |
| Pod readinessGates | GA | GA | GA | GA | GA |
| minReadySeconds for StatefulSet | GA | GA | GA | GA | GA |
| PodDisruptionConditions | beta | GA | GA | GA | GA |
| Job Pod Replacement Policy | alpha | beta | beta | GA | GA |
| StatefulSet PersistentVolumeClaimRetentionPolicy | beta | beta | GA | GA | GA |
| Sidecar Containers | alpha | beta | beta | GA | GA |
| AppArmor support | beta | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl rollout status` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl rollout history` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl rollout undo` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl rollout restart` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl rollout pause/resume` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl set image` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `--timeout` flag for rollout status | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Deployment | apps/v1 | apps/v1 | apps/v1 | apps/v1 | apps/v1 |
| ReplicaSet | apps/v1 | apps/v1 | apps/v1 | apps/v1 | apps/v1 |
| StatefulSet | apps/v1 | apps/v1 | apps/v1 | apps/v1 | apps/v1 |
| DaemonSet | apps/v1 | apps/v1 | apps/v1 | apps/v1 | apps/v1 |
| PodDisruptionBudget | policy/v1 | policy/v1 | policy/v1 | policy/v1 | policy/v1 |
| HorizontalPodAutoscaler | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: DaemonSet `maxSurge` 已 GA，允许在更新过程中创建超过节点数量的 Pod。在诊断 DaemonSet 更新问题时，需检查此配置是否影响调度。

- **[v1.29+]**: `PodDisruptionConditions` GA，Pod 被驱逐时会设置 `DisruptionTarget` condition。诊断时可检查此条件了解 Pod 终止原因：
  ```bash
  kubectl get pod POD_NAME -o jsonpath='{.status.conditions[?(@.type=="DisruptionTarget")]}'
  ```

- **[v1.30+]**: StatefulSet PersistentVolumeClaimRetentionPolicy GA，可控制 Pod 删除时 PVC 的行为。诊断 StatefulSet 更新问题时需检查此配置：
  ```bash
  kubectl get sts NAME -o jsonpath='{.spec.persistentVolumeClaimRetentionPolicy}'
  ```

- **[v1.31+]**: StatefulSet `maxUnavailable` 升级为 beta，允许并行更新多个 Pod。这可能改变 StatefulSet 更新的行为和诊断方法：
  ```bash
  kubectl get sts NAME -o jsonpath='{.spec.updateStrategy.rollingUpdate.maxUnavailable}'
  ```

- **[v1.31+]**: Sidecar Containers GA，使用 `initContainers` 配合 `restartPolicy: Always` 实现。诊断 Pod 启动问题时需注意 sidecar 容器的启动顺序和依赖。

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 readinessProbe 超时误判为镜像 bug** | Pod Running 但 READY 0/1，应用日志无明显错误 | readinessProbe 配置的 initialDelaySeconds 过短，应用尚未完成初始化就开始被探测 | D2.3 中仔细检查 probe 配置与应用实际启动时间的匹配；尝试临时增大 initialDelaySeconds 验证假设 |
| **将 PDB 约束误判为资源不足** | 滚动更新卡住，旧 RS 副本数不变，看似无法调度新 Pod | PDB 配置 `minAvailable: 100%` 或 `maxUnavailable: 0`，不允许任何 Pod 被终止 | D2.7 先检查 PDB 配置和 disruptionsAllowed 状态，再排查资源问题 |
| **将网络问题误判为应用 bug** | Pod CrashLoopBackOff，日志显示无法连接数据库/依赖服务 | NetworkPolicy 阻止了 Pod 到外部服务的连接，或 DNS 解析失败 | 检查 NetworkPolicy 配置；在 Pod 内执行 `nslookup` 和 `curl` 测试网络连通性 |
| **将 ConfigMap 挂载问题误判为权限问题** | Pod 日志显示文件读取失败或配置文件为空 | ConfigMap 已更新但 Pod 未重启，仍使用旧的缓存配置 | 确认 Pod 创建时间与 ConfigMap 更新时间的先后关系；使用 `subPath` 挂载时 ConfigMap 更新不会自动同步 |
| **将 HPA 行为误判为滚动更新问题** | Deployment 副本数频繁变化，看似更新不稳定 | HPA 根据负载自动扩缩容，与滚动更新无关 | D1.1 检查是否存在 HPA；分析副本数变化是否与更新操作相关 |
| **将 StatefulSet 分区更新误判为卡住** | 部分 Pod 未更新，看似更新停滞 | 配置了 partition 进行分批更新，这是预期行为 | D3.1 检查 StatefulSet 的 partition 配置；确认是否为计划内的分批发布 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Deployment Controller 原理 | `domain-4-workloads/` | 理解 Deployment 如何管理 ReplicaSet 和滚动更新逻辑 |
| Deployment 综合故障排查 | `domain-12-troubleshooting/11-deployment-comprehensive-troubleshooting.md` | 深度排查 Deployment 相关问题 |
| StatefulSet 更新策略 | `domain-4-workloads/` | StatefulSet 有序更新和分区更新机制 |
| DaemonSet 更新机制 | `domain-4-workloads/` | DaemonSet 在不同节点上的更新行为 |
| PodDisruptionBudget | `domain-9-platform-ops/` | PDB 配置最佳实践和与滚动更新的交互 |
| Admission Webhooks | `domain-10-extensions/03-admission-webhook-configuration.md` | Webhook 如何影响 Pod 创建 |
| 容器探针配置 | `domain-4-workloads/` | readinessProbe/livenessProbe 最佳实践 |
| 镜像拉取策略 | `domain-4-workloads/` | imagePullPolicy 配置和镜像仓库认证 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、11 个修复操作 | 滚动更新故障为高频运维问题，基于生产环境工单分析创建 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **Argo Rollouts**: Progressive Delivery 控制器的故障诊断（Canary/BlueGreen/Analysis）
2. **Flagger**: 与 Flagger 集成的金丝雀部署故障
3. **Istio Traffic Management**: Service Mesh 场景下的流量切换故障
4. **Kustomize/Helm 部署**: 使用 GitOps 工具部署时的特定故障模式
5. **GPU 工作负载**: GPU Pod 的特殊启动要求和失败模式
6. **Spot/Preemptible 节点**: 在抢占式节点上的滚动更新策略
7. **多集群部署**: 跨集群滚动更新的协调和故障处理

---
title: Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常
description: 专有云 ACK 集群每日对账 CronJob 连续失败，Pod 状态为 Error/ImagePullBackOff，根因涉及 Job 退避策略、ACR
  镜像拉取超时与 RBAC 权限不足的工单闭环样本。
summary: 专有云 ACK 集群每日对账 CronJob 连续失败，Pod 状态为 Error/ImagePullBackOff，根因涉及 Job 退避策略、ACR
  镜像拉取超时与 RBAC 权限不足的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- job
- cronjob
- imagepullbackoff
- rbac
- backoff
- p1
tier: supporting
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T17:30:00+08:00'
incident_id: INC-2026-ACK-049
priority: P1
severity: high
affected_cluster: ack-zyy-prod-07
affected_namespace: finance-reconcile
ticket_type: 批处理故障
skill_ref:
- '[[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/00-core-workloads/02-job-cronjob-advanced|Job
  与 CronJob 高级用法]]'
- '[[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/00-core-workloads/03-pod-lifecycle-events|Pod
  生命周期事件]]'
- RBAC 排障
fta_ref:
- 'FTA: Job/CronJob 执行失败'
last_updated: 2026-06-26 17:30:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常 如何处理
trigger_keywords:
- Job/CronJob
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[concepts/cronjob.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-039-rbac-api-access-denied.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-07` 的 `finance-reconcile` 命名空间中发现每日对账 CronJob `daily-reconcile-job` 连续 3 天未成功执行，最近生成的 Job Pod 状态为 `Error` 与 `ImagePullBackOff` 交替出现。客户描述如下：

> “我们的每日对账任务最近一直失败，看 CronJob 的 last successful time 是三天前。新创建的 Job Pod 有的状态是 Error，有的显示 ImagePullBackOff。我们检查过 ACR 镜像仓库，镜像标签是存在的，但 Pod 就是拉不下来。另外 Pod 日志里好像还有访问 OSS 的权限错误。这个任务每天凌晨 2 点跑，影响财务日报生成，麻烦尽快处理。”

受影响命名空间为 `finance-reconcile`，业务为每日资金对账批处理，失败会导致财务日报延迟，影响结算部门工作。

## 分类与优先级判定

- **工单类型**：批处理故障 / Job 调度与执行失败。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境关键批处理任务连续失败 3 天，业务日报生成受阻。
2. 失败原因涉及镜像拉取、Pod 执行错误、权限等多个维度，需要系统排查。
3. 虽未直接影响在线服务，但属于核心业务流程中断，符合 P1 “生产环境 + 服务降级/阻塞” 标准。

## 诊断步骤

按“先看 CronJob/Job 状态，再看 Pod 事件与日志，最后查权限与镜像”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 CronJob 与最近 Job 状态
kubectl get cronjob daily-reconcile-job -n finance-reconcile -o wide
kubectl get job -n finance-reconcile -l cronjob=daily-reconcile-job --sort-by='.metadata.creationTimestamp'

# 2. 查看最近失败 Job 的 Pod 状态
kubectl get pod -n finance-reconcile -l job-name=daily-reconcile-job-29001234 -o wide
kubectl describe pod -n finance-reconcile $(kubectl get pod -n finance-reconcile -l job-name=daily-reconcile-job-29001234 -o jsonpath='{.items[0].metadata.name}') | grep -A 30 Events

# 3. 查看 Job Pod 日志
kubectl logs -n finance-reconcile -l job-name=daily-reconcile-job-29001234 --tail=200

# 4. 检查 CronJob 调度历史与挂起状态
kubectl describe cronjob daily-reconcile-job -n finance-reconcile | grep -A 20 Events

# 5. 检查镜像拉取事件
kubectl get events -n finance-reconcile --field-selector reason=FailedToPullImage --sort-by='.lastTimestamp'
kubectl get events -n finance-reconcile --field-selector reason=ImagePullBackOff --sort-by='.lastTimestamp'

# 6. 验证镜像在 ACR 是否可访问
aliyun cr GET /repos/finance/reconcile/tags --RegionId cn-zhangjiakou

# 7. 检查 ImagePullSecret 配置
kubectl get secret -n finance-reconcile | grep -i docker
kubectl get serviceaccount default -n finance-reconcile -o yaml

# 8. 检查 Pod 使用的 ServiceAccount 与 RBAC 权限
kubectl get pod -n finance-reconcile -l job-name=daily-reconcile-job-29001234 -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.serviceAccountName}{"\n"}{end}'
kubectl get role -n finance-reconcile
kubectl get rolebinding -n finance-reconcile
kubectl auth can-i get secret --as=system:serviceaccount:finance-reconcile:reconcile-sa -n finance-reconcile

# 9. 检查节点上 containerd 镜像拉取日志
kubectl logs -n kube-system $(kubectl get pod -n kube-system -l app=csi-plugin -o jsonpath='{.items[0].metadata.name}') -c csi-plugin --tail=50
```
## 根因分析

通过 CronJob 事件、Pod 日志与 RBAC 检查，确认存在以下三个问题：

**问题一：镜像 tag 被覆盖后 ACR 缓存与节点本地镜像摘要不一致**

Pod 事件显示：

```
Warning  FailedToPullImage  ...  Failed to pull image "registry-vpc.cn-zhangjiakou.aliyuncs.com/finance/reconcile:v2.1.4": rpc error: code = NotFound desc = failed to pull and unpack image: failed to resolve reference: not found
```

开发团队在 CI/CD 中复用了 `v2.1.4` 标签重新构建并推送，ACR 侧镜像摘要已变更，但 CronJob 的 YAML 中 `imagePullPolicy: IfNotPresent` 导致部分节点优先使用本地旧镜像，而新调度节点拉取新 `v2.1.4` 时因缓存或权限问题失败，出现 `ImagePullBackOff`。

**问题二：Job 退避策略配置过严，失败 Pod 未被清理**

CronJob 中设置了：

```yaml
spec:
  jobTemplate:
    spec:
      backoffLimit: 2
      activeDeadlineSeconds: 600
      ttlSecondsAfterFinished: 86400
```

由于 `backoffLimit` 仅 2 次，且 Pod 启动后立即因镜像拉取失败退出，Job 控制器很快判定失败并停止重试。同时失败的 Pod 未被及时清理，占用了 `finance-reconcile` 命名空间的 Pod 配额，导致后续新 Job 无法创建。

**问题三：ServiceAccount 缺少访问 OSS Secret 的权限**

Pod 日志显示：

```
Error: failed to get OSS credentials from secret: secrets "oss-reconcile-cred" is forbidden: User "system:serviceaccount:finance-reconcile:reconcile-sa" cannot get resource "secrets" in API group "" in the namespace "finance-reconcile"
```

该 Job 需要读取 `oss-reconcile-cred` Secret 来获取 OSS 访问凭证，但 `reconcile-sa` 对应的 Role 仅配置了 `pods` 与 `configmaps` 权限，缺少 `secrets` 的 `get` 权限。这是最近一次权限最小化改造中误删了 Secret 权限。

## 修复命令

**第一步：清理失败的 Job 与 Pod，释放命名空间配额**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete job -n finance-reconcile -l cronjob=daily-reconcile-job
kubectl delete pod -n finance-reconcile --field-selector status.phase=Failed
```
**第二步：更新 CronJob，使用唯一镜像 tag 并优化退避策略**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-reconcile-job
  namespace: finance-reconcile
spec:
  schedule: "0 2 * * *"
  timeZone: "Asia/Shanghai"
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      backoffLimit: 5
      activeDeadlineSeconds: 1800
      ttlSecondsAfterFinished: 3600
      template:
        spec:
          serviceAccountName: reconcile-sa
          imagePullSecrets:
          - name: acr-finance-secret
          containers:
          - name: reconcile
            image: registry-vpc.cn-zhangjiakou.aliyuncs.com/finance/reconcile:20260625-2a3b4c5d
            imagePullPolicy: Always
            env:
            - name: OSS_SECRET_NAME
              value: oss-reconcile-cred
            resources:
              requests:
                cpu: "500m"
                memory: "1Gi"
              limits:
                cpu: "2"
                memory: "4Gi"
          restartPolicy: OnFailure
EOF
```
**第三步：为 reconcile-sa 补充 Secret 读取权限**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: reconcile-role
  namespace: finance-reconcile
rules:
- apiGroups: [""]
  resources: ["secrets"]
  resourceNames: ["oss-reconcile-cred", "acr-finance-secret"]
  verbs: ["get"]
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: reconcile-role-binding
  namespace: finance-reconcile
subjects:
- kind: ServiceAccount
  name: reconcile-sa
  namespace: finance-reconcile
roleRef:
  kind: Role
  name: reconcile-role
  apiGroup: rbac.authorization.k8s.io
EOF
```
**第四步：手动触发一次 CronJob 验证修复效果**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create job -n finance-reconcile --from=cronjob/daily-reconcile-job manual-reconcile-test-$(date +%s)
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认新 Job 创建成功且 Pod 进入 Running
kubectl get job -n finance-reconcile | grep reconcile
kubectl get pod -n finance-reconcile -l job-name=manual-reconcile-test-* -o wide

# 2. 查看 Pod 事件无 ImagePullBackOff
kubectl describe pod -n finance-reconcile -l job-name=manual-reconcile-test-* | grep -A 20 Events

# 3. 查看 Job 执行日志，确认无 RBAC 权限错误
kubectl logs -n finance-reconcile -l job-name=manual-reconcile-test-* --tail=100

# 4. 验证 RBAC 权限已生效
kubectl auth can-i get secret/oss-reconcile-cred --as=system:serviceaccount:finance-reconcile:reconcile-sa -n finance-reconcile

# 5. 验证 CronJob 下次调度时间正确
kubectl get cronjob daily-reconcile-job -n finance-reconcile -o jsonpath='{.status.nextScheduleTime}'

# 6. 确认 ACR 镜像标签唯一且可拉取
crictl image ls | grep reconcile
```
## 回复客户话术

> 您好，经排查，本次 `daily-reconcile-job` 连续失败的根因是 **三方面问题叠加**：
>
> 1. **镜像标签被复用**：CI/CD 重新推送了 `v2.1.4`，导致部分节点本地旧镜像与新 ACR 摘要不一致，触发 `ImagePullBackOff`；
> 2. **Job 退避策略过严**：`backoffLimit` 仅 2 次，失败后快速停止重试，且失败 Pod 未清理占用配额；
> 3. **RBAC 权限缺失**：`reconcile-sa` 缺少读取 `oss-reconcile-cred` Secret 的权限，导致任务无法获取 OSS 凭证。
>
> 我们已完成以下处置：
> - 清理了历史失败 Job 与 Pod，释放配额；
> - 更新 CronJob 使用唯一镜像 tag（`20260625-2a3b4c5d`）并设置 `imagePullPolicy: Always`；
> - 放宽 `backoffLimit` 至 5，`activeDeadlineSeconds` 至 1800，并缩短失败历史保留时间；
> - 为 `reconcile-sa` 补充了最小化 Secret 读取权限。
>
> 当前手动触发的对账任务已 Running 并成功输出对账结果。建议后续：
> - 在 CI/CD 中禁止复用镜像 tag，采用 Git commit sha 或日期作为 tag；
> - 批处理任务统一接入日志监控，参考 Job 日志最佳实践；
> - 配置 CronJob 失败告警：`kube_job_status_failed > 0` 持续 1 次触发 P2 告警。
>
> 如有疑问，请随时联系。

## 复盘与沉淀

本次故障集中体现了批处理任务在镜像管理、重试策略与权限管理上的常见疏漏。核心教训：

1. **镜像标签不可变原则**：在 CI/CD 中应严格禁止复用镜像 tag。建议采用 `{version}-{git-sha}-{timestamp}` 三段式标签，并在部署 YAML 中显式引用完整 tag，避免 `latest` 或纯版本号标签。
2. **Job 退避策略需与任务特性匹配**：对于依赖外部服务的批处理任务，`backoffLimit` 不宜过小；`activeDeadlineSeconds` 需覆盖任务正常执行时长；`ttlSecondsAfterFinished` 不宜过长，避免失败 Pod 堆积占用配额。
3. **RBAC 权限变更必须回归验证**：权限最小化改造后，应使用 `kubectl auth can-i` 对关键 ServiceAccount 进行权限回归验证，特别是读取 Secret、访问 ConfigMap 等常见批处理需求。

建议将本案例加入 Job/CronJob 失败 FTA，并在日常巡检中增加：
- CronJob `lastSuccessfulTime` 超过 24 小时告警；
- 命名空间内 Failed Pod 数量超过阈值告警；
- ServiceAccount 权限变更审计告警。

后续 SOP 更新要点：
1. 所有 CronJob YAML 必须包含 `imagePullPolicy: Always` 或唯一不可变 tag；
2. 批处理 ServiceAccount 的 Role 变更需通过 `auth can-i` 回归测试；
3. 失败 Job 保留策略统一为 `failedJobsHistoryLimit: 3` + `ttlSecondsAfterFinished: 3600`。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若 ACR 镜像拉取仍不稳定，需升级至 **容器镜像与网络团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-049`
  - 根因：镜像 tag 复用 + Job 退避策略过严 + RBAC Secret 权限缺失
  - 影响集群：`ack-zyy-prod-07`
  - 影响命名空间：`finance-reconcile`
  - 临时修复：清理失败 Job/Pod、更新镜像 tag 与拉取策略、优化退避策略、补充 RBAC 权限
  - 长期方案：建立镜像标签不可变规范、统一批处理 RBAC 模板、配置 CronJob 失败告警
  - 待跟进：确认明日凌晨 2 点定时任务正常执行，更新 CI/CD 与 SOP

## Related

- CronJob
- RBAC 权限不足导致应用无法访问 K8s API
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang


<!-- risk-assessed -->

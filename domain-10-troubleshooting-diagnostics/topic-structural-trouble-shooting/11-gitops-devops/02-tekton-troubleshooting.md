---
title: Tekton CI/CD 流水线故障排查指南 [topic-structural-trouble-shooting]
description: 'title: Tekton CI/CD 流水线故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- prometheus
- flux
- docker
- opa
- job
- cronjob
- ingress
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Tekton CI/CD 流水线故障排查指南 是什么
- 如何 Tekton CI/CD 流水线故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Tekton CI/CD 流水线故障排查指南 故障排查
- Tekton CI/CD 流水线故障排查指南 排障步骤
trigger_keywords:
- Tekton
- CI
- CD
- 流水线故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- policy-basics
- logging-basics
created: "2026-05-23"
---

title: Tekton CI/CD 流水线故障排查指南
description: '# Tekton CI/CD 流水线故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- docker
- opa
- job
- cronjob
- ingress
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Tekton CI/CD 流水线故障排查指南 是什么
- 如何 Tekton CI/CD 流水线故障排查指南
- Tekton CI/CD 流水线故障排查指南 故障排查
- Tekton CI/CD 流水线故障排查指南 排障步骤
trigger_keywords:
- Tekton
- CI
- CD
- 流水线故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Tekton CI/CD 流水线故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Tekton Pipelines v0.50+ | **最后更新**: 2026-04 | **难度**: 中级

---

## 0. 10 分钟快速诊断

1. **PipelineRun 状态**：`tkn pipelinerun list` 或 `kubectl get pipelineruns -A`，查看失败的运行。
2. **TaskRun 详情**：`tkn taskrun logs <taskrun-name>` 查看具体任务日志。
3. **Workspace 状态**：`kubectl get pvc -n <namespace>`，确认 workspace PVC 已绑定。
4. **事件检查**：`kubectl get events --field-selector reason=FailedMount` 或 `FailedPullImage`。
5. **ServiceAccount 权限**：确认 PipelineRun 使用的 ServiceAccount 有创建 Pod 的权限。
6. **快速缓解**：
   - 任务卡住：删除 PipelineRun 后使用 `tkn pipeline start` 重新触发。
   - 镜像拉取失败：检查 `imagePullSecrets` 或切换到公共镜像。
   - Workspace 空间不足：增大 PVC 容量或使用 `emptyDir`。
7. **证据留存**：保存 PipelineRun YAML、TaskRun 日志、Workspace 使用情况和节点事件。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 PipelineRun 执行失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| PipelineRun 失败 | `PipelineRun failed: ...` | Tekton PipelineRun | `tkn pipelinerun describe` |
| TaskRun 失败 | `TaskRun failed: ...` | Tekton TaskRun | `tkn taskrun logs` |
| 容器启动失败 | `container failed to start` | Pod Events | `kubectl describe pod` |
| 步骤退出码非零 | `step exited with code 1` | Step Container | `tkn taskrun logs` |
| 任务超时 | `TaskRun timeout` | Tekton Controller | `tkn pipelinerun describe` |

#### 1.1.2 Workspace 与存储问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Workspace 挂载失败 | `failed to mount volume` | kubelet Events | `kubectl describe taskrun-pod` |
| PVC 绑定失败 | `PVC is not bound` | Tekton Controller | `kubectl get pvc` |
| 磁盘空间不足 | `no space left on device` | Step Container | `tkn taskrun logs` |
| 权限不足 | `Permission denied` | Step Container | `tkn taskrun logs` |
| Workspace 共享冲突 | `workspace already in use` | Tekton Controller | Controller 日志 |

#### 1.1.3 触发器与事件问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Webhook 未触发 | `no events received` | EventListener | EventListener Pod 日志 |
| 触发器过滤失败 | `trigger binding failed` | Tekton Trigger | Trigger 日志 |
| 事件解析失败 | `cannot parse event payload` | Tekton Trigger | EventListener 日志 |
| GitHub/GitLab 推送无响应 | ` webhook delivery failed` | Git Provider | Webhook 设置页面 |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大规模并行构建卡死** | 20+ PipelineRun 同时执行时节点资源耗尽 | 未设置 PipelineRun 并发限制 | 配置 ResourceQuota 和并行度限制 |
| **构建缓存失效** | 每次构建都重新下载依赖，耗时 10 分钟+ | Workspace 未正确配置缓存卷 | 使用 PersistentVolumeClaim 作为缓存 workspace |
| **Secrets 泄露到日志** | 构建日志中明文打印了 API Key | 步骤脚本使用 `echo $SECRET` 调试 | 使用 Tekton Secret 类型和脚本审查 |
| **定时构建漂移** | Cron 触发的 PipelineRun 比预期晚数小时 | CronJob 时区设置错误或控制器负载高 | 配置正确时区并监控控制器 |

### 1.2 报错查看方式汇总

```bash
# Tekton CLI 查看状态
tkn pipelinerun list -A
tkn taskrun list -A
tkn pipelinerun logs <pipelinerun-name> -n <namespace>
tkn taskrun logs <taskrun-name> -n <namespace>

# Kubernetes 原生查看
kubectl get pipelineruns -A -o wide
kubectl get taskruns -A -o wide
kubectl get pods -A -l tekton.dev/pipelineRun

# 查看 PipelineRun 详情
kubectl describe pipelinerun <name> -n <namespace>

# 查看 Tekton 控制器日志
kubectl logs -n tekton-pipelines deployment/tekton-pipelines-controller --tail=200
kubectl logs -n tekton-pipelines deployment/tekton-triggers-controller --tail=200

# 查看事件
kubectl get events --field-selector involvedObject.kind=PipelineRun --sort-by='.lastTimestamp'
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

Tekton Pipelines 的执行流程：

```
用户创建 PipelineRun
        │
        ▼
┌─────────────────────────────┐
│ Tekton Pipelines Controller │ ──► 解析 PipelineRun，创建 TaskRun
│ (tekton-pipelines-controller)│
└──────────────┬──────────────┘
               │
        ┌──────┴──────┐
        ▼             ▼
┌─────────────┐   ┌─────────────┐
│   TaskRun   │   │   TaskRun   │
│   (串行)    │──►│   (并行)    │
└──────┬──────┘   └──────┬──────┘
       │                 │
       ▼                 ▼
┌─────────────┐   ┌─────────────┐
│    Pod      │   │    Pod      │
│  (步骤容器)  │   │  (步骤容器)  │
└─────────────┘   └─────────────┘
```

**关键概念**：
- **Workspace**：任务间共享数据的机制，可以是 PVC、`emptyDir`、ConfigMap、Secret 或 CSI 卷
- **Step**：Task 中的最小执行单元，每个 Step 对应一个容器，按顺序执行
- **Sidecar**：与 Step 容器并行运行的辅助容器（如 Docker daemon、数据库）
- **Result**：Task 向 Pipeline 传递的小型输出（限制 4KB）

### 2.2 排查逻辑决策树

```
Tekton 流水线问题
    ├── PipelineRun 创建失败
    │   ├── Pipeline/Task 不存在？──► 检查 CRD 和引用名称
    │   ├── ServiceAccount 权限不足？──► 绑定正确 RBAC
    │   └── 参数类型不匹配？──► 检查 params 类型定义
    ├── PipelineRun 执行失败
    │   ├── TaskRun 失败
    │   │   ├── 步骤退出码非零？──► 查看步骤日志修复脚本
    │   │   ├── 容器镜像拉取失败？──► 检查镜像和 pullSecrets
    │   │   ├── 任务超时？──► 调大 Task 的 timeout
    │   │   └── Sidecar 未就绪？──► 检查 sidecar 镜像和启动时间
    │   ├── Workspace 问题
    │   │   ├── PVC 未绑定？──► 检查 StorageClass 和配额
    │   │   ├── 磁盘空间不足？──► 清理或扩容 PVC
    │   │   └── 权限问题？──► 检查 securityContext
    │   └── 资源不足
    │       ├── 节点 CPU/内存不足？──► 增加节点或调小 requests
    │       └── Pod 数量达到限制？──► 检查 namespace Pod 配额
    └── 触发器问题
        ├── EventListener 无响应？──► 检查 Ingress/Service
        ├── Webhook 配置错误？──► 检查 URL 和 Secret
        └── 触发器过滤不匹配？──► 检查 TriggerBinding 和 Interceptor
```

### 2.3 详细诊断命令

#### Tekton 全景诊断

```bash
#!/bin/bash
# Tekton 全景诊断脚本

echo "=== Tekton 全景诊断 ==="

# 1. Tekton 组件状态
echo "1. Tekton 控制器状态:"
for deploy in tekton-pipelines-controller tekton-pipelines-webhook tekton-triggers-controller tekton-triggers-webhook; do
  READY=$(kubectl get deployment $deploy -n tekton-pipelines -o jsonpath='{.status.readyReplicas}' 2>/dev/null || echo "0")
  DESIRED=$(kubectl get deployment $deploy -n tekton-pipelines -o jsonpath='{.spec.replicas}' 2>/dev/null || echo "0")
  echo "  $deploy: $READY/$DESIRED ready"
done

# 2. PipelineRun 状态统计
echo ""
echo "2. PipelineRun 状态统计:"
kubectl get pipelineruns -A -o json | jq -r '
  [.items[] | .status.conditions[-1].reason // "Unknown"] | group_by(.) |
  .[] | "  \(.[0]): \(length)"
'

# 3. 失败的 TaskRun 列表
echo ""
echo "3. 失败的 TaskRun (最近 10 个):"
kubectl get taskruns -A -o json | jq -r '
  .items[] | select(.status.conditions[-1].status == "False") |
  "  \(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].reason) - \(.status.conditions[-1].message | tostring | .[0:80])"
' | tail -10

# 4. 卡住（Running 超长时间）的 PipelineRun
echo ""
echo "4. 运行超过 1 小时的 PipelineRun:"
kubectl get pipelineruns -A -o json | jq -r '
  .items[] | select(.status.conditions[-1].reason == "Running" and .status.startTime != null) |
  select((now - (.status.startTime | fromdateiso8601)) > 3600) |
  "  \(.metadata.namespace)/\(.metadata.name): running for \((now - (.status.startTime | fromdateiso8601)) / 60 | floor) minutes"
'

# 5. Workspace PVC 状态
echo ""
echo "5. Workspace PVC 状态:"
kubectl get pvc -A -o json | jq -r '
  .items[] | select(.metadata.annotations["tekton.dev/workspace"] != null or .metadata.name | contains("workspace")) |
  "  \(.metadata.namespace)/\(.metadata.name): phase=\(.status.phase), capacity=\(.status.capacity.storage // "unknown")"
'

# 6. Tekton 控制器错误日志
echo ""
echo "6. 控制器错误日志 (最近 10 条):"
kubectl logs -n tekton-pipelines deployment/tekton-pipelines-controller --tail=200 2>/dev/null | \
  grep -iE "error|fail|timeout" | tail -10
```

#### PipelineRun 问题深度诊断

```bash
#!/bin/bash
# PipelineRun 问题深度诊断
# 用法: ./diagnose-pipelinerun.sh <pipelinerun-name> <namespace>

PR_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$PR_NAME" ]; then
  echo "用法: $0 <pipelinerun-name> [namespace]"
  exit 1
fi

echo "=== PipelineRun $NAMESPACE/$PR_NAME 深度诊断 ==="

# 1. PipelineRun 状态
echo "1. PipelineRun 状态:"
kubectl get pipelinerun $PR_NAME -n $NAMESPACE -o json | jq -r '
  {
    status: .status.conditions[-1].status,
    reason: .status.conditions[-1].reason,
    message: .status.conditions[-1].message,
    startTime: .status.startTime,
    completionTime: .status.completionTime
  }'

# 2. 关联的 TaskRun
echo ""
echo "2. 关联 TaskRun 状态:"
for tr in $(kubectl get taskruns -n $NAMESPACE -l tekton.dev/pipelineRun=$PR_NAME -o jsonpath='{.items[*].metadata.name}'); do
  TR_STATUS=$(kubectl get taskrun $tr -n $NAMESPACE -o jsonpath='{.status.conditions[-1].status}')
  TR_REASON=$(kubectl get taskrun $tr -n $NAMESPACE -o jsonpath='{.status.conditions[-1].reason}')
  echo "  $tr: status=$TR_STATUS, reason=$TR_REASON"
done

# 3. 失败 TaskRun 的日志
echo ""
echo "3. 失败 TaskRun 的日志:"
for tr in $(kubectl get taskruns -n $NAMESPACE -l tekton.dev/pipelineRun=$PR_NAME -o json | \
  jq -r '.items[] | select(.status.conditions[-1].status == "False") | .metadata.name'); do
  echo "=== TaskRun: $tr ==="
  tkn taskrun logs $tr -n $NAMESPACE --last 50 2>/dev/null || kubectl logs -n $NAMESPACE -l tekton.dev/taskRun=$tr --tail=50
  echo ""
done

# 4. Pod 状态
echo ""
echo "4. 关联 Pod 状态:"
kubectl get pods -n $NAMESPACE -l tekton.dev/pipelineRun=$PR_NAME -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), restarts=\(.status.containerStatuses[0].restartCount // 0)"
'

# 5. Events
echo ""
echo "5. 相关 Events:"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$PR_NAME --sort-by='.lastTimestamp' | tail -10
```

---

## 3. 解决方案与风险控制

### 3.1 Pipeline 与 Task 优化

#### 方案一：流水线并发与资源限制

```yaml
apiVersion: tekton.dev/v1
kind: Pipeline
metadata:
  name: ci-pipeline
spec:
  workspaces:
  - name: source
  - name: cache
  params:
  - name: repo-url
    type: string
  - name: revision
    type: string
  tasks:
  # fetch-source 和 lint 可以并行
  - name: fetch-source
    taskRef:
      name: git-clone
    workspaces:
    - name: output
      workspace: source
    params:
    - name: url
      value: $(params.repo-url)
    - name: revision
      value: $(params.revision)
    timeout: 5m

  - name: lint
    runAfter: [fetch-source]
    taskRef:
      name: golangci-lint
    workspaces:
    - name: source
      workspace: source
    timeout: 10m

  - name: unit-test
    runAfter: [fetch-source]
    taskRef:
      name: go-test
    workspaces:
    - name: source
      workspace: source
    - name: cache
      workspace: cache
    timeout: 15m

  - name: build-image
    runAfter: [lint, unit-test]
    taskRef:
      name: kaniko-build
    workspaces:
    - name: source
      workspace: source
    params:
    - name: dockerfile
      value: ./Dockerfile
    - name: image
      value: $(params.image-url)
    timeout: 20m
---
# 资源限制的 Task 示例
apiVersion: tekton.dev/v1
kind: Task
metadata:
  name: go-test
spec:
  workspaces:
  - name: source
  - name: cache
  steps:
  - name: run-tests
    image: golang:1.21
    workingDir: $(workspaces.source.path)
    script: |
      #!/usr/bin/env sh
      go test -v -race -coverprofile=coverage.out ./...
    resources:
      limits:
        cpu: "4"
        memory: "8Gi"
      requests:
        cpu: "2"
        memory: "4Gi"
    env:
    - name: GOCACHE
      value: $(workspaces.cache.path)/go-build
    - name: GOPATH
      value: $(workspaces.cache.path)/go
```

#### 方案二：Workspace 与缓存配置

```yaml
# 使用 PVC 作为持久化缓存 Workspace
apiVersion: tekton.dev/v1
kind: PipelineRun
metadata:
  generateName: ci-run-
spec:
  pipelineRef:
    name: ci-pipeline
  workspaces:
  - name: source
    volumeClaimTemplate:
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 5Gi
        storageClassName: fast-ssd
  - name: cache
    persistentVolumeClaim:
      claimName: shared-build-cache  # 跨 PipelineRun 共享的缓存 PVC
  taskRunSpecs:
  - pipelineTaskName: unit-test
    stepOverrides:
    - name: run-tests
      resources:
        limits:
          cpu: "8"
          memory: "16Gi"
  timeouts:
    pipeline: "1h"
    tasks: "30m"
  serviceAccountName: tekton-build-sa
---
# 共享缓存 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-build-cache
spec:
  accessModes:
  - ReadWriteMany  # 允许多个 TaskRun 同时读取
  storageClassName: nfs-client
  resources:
    requests:
      storage: 50Gi
```

### 3.2 触发器配置

```yaml
# GitHub Webhook 触发器配置
apiVersion: triggers.tekton.dev/v1beta1
kind: EventListener
metadata:
  name: github-ci-listener
  namespace: tekton-pipelines
spec:
  serviceAccountName: tekton-triggers-sa
  triggers:
  - name: github-push-trigger
    interceptors:
    - ref:
        name: "github"
      params:
      - name: "secretRef"
        value:
          secretName: github-webhook-secret
          secretKey: secretToken
      - name: "eventTypes"
        value: ["push", "pull_request"]
    bindings:
    - ref: github-push-binding
    template:
      ref: ci-pipeline-template
---
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerBinding
metadata:
  name: github-push-binding
spec:
  params:
  - name: repo-url
    value: $(body.repository.clone_url)
  - name: revision
    value: $(body.after)
  - name: branch
    value: $(body.ref)
---
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerTemplate
metadata:
  name: ci-pipeline-template
spec:
  params:
  - name: repo-url
  - name: revision
  - name: branch
  resourcetemplates:
  - apiVersion: tekton.dev/v1
    kind: PipelineRun
    metadata:
      generateName: ci-run-$(tt.params.branch)-
    spec:
      pipelineRef:
        name: ci-pipeline
      params:
      - name: repo-url
        value: $(tt.params.repo-url)
      - name: revision
        value: $(tt.params.revision)
      workspaces:
      - name: source
        volumeClaimTemplate:
          spec:
            accessModes:
            - ReadWriteOnce
            resources:
              requests:
                storage: 5Gi
```

### 3.3 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 修改 Pipeline 定义 | ⭐ 低 | 仅影响新的 PipelineRun | 恢复原始 Pipeline YAML |
| 删除失败的 PipelineRun | ⭐ 低 | 释放资源，日志随之删除 | 无需回滚（如已保存日志） |
| 更换 Task 镜像 | ⭐ 低 | 影响新 TaskRun | 恢复原始 Task 镜像引用 |
| 调整 Workspace PVC 大小 | ⭐ 低 | 仅影响新创建的 PVC | 不影响已有 PVC |
| 修改 ServiceAccount 权限 | ⭐⭐ 中 | 可能影响所有使用该 SA 的运行 | 恢复原始 RBAC |
| 调整全局超时配置 | ⭐ 低 | 影响所有 PipelineRun | 恢复原始 ConfigMap |
| 升级 Tekton 版本 | ⭐⭐ 中 | 可能存在 API 兼容性变化 | 使用 Tekton Operator 回滚 |

### 3.4 验证与监控

#### Tekton 健康检查脚本

```bash
#!/bin/bash
# Tekton 健康检查脚本

REPORT_FILE="/var/log/kubernetes/tekton-health-$(date +%Y%m%d-%H%M%S).log"

echo "=== Tekton 健康检查 $(date) ===" | tee $REPORT_FILE

# 1. 控制器健康
COMPONENTS=("tekton-pipelines-controller" "tekton-pipelines-webhook" "tekton-triggers-controller" "tekton-triggers-webhook")
for comp in "${COMPONENTS[@]}"; do
  READY=$(kubectl get deployment $comp -n tekton-pipelines -o jsonpath='{.status.readyReplicas}' 2>/dev/null || echo "0")
  DESIRED=$(kubectl get deployment $comp -n tekton-pipelines -o jsonpath='{.spec.replicas}' 2>/dev/null || echo "0")
  if [ "$READY" = "$DESIRED" ]; then
    echo "✓ $comp: $READY/$DESIRED" | tee -a $REPORT_FILE
  else
    echo "✗ $comp: $READY/$DESIRED" | tee -a $REPORT_FILE
  fi
done

# 2. 近期失败率
echo "" | tee -a $REPORT_FILE
echo "2. 近期 PipelineRun 失败率:" | tee -a $REPORT_FILE
TOTAL=$(kubectl get pipelineruns --all-namespaces -o json 2>/dev/null | jq '[.items[] | select(.metadata.creationTimestamp | fromdateiso8601 > now - 86400)] | length')
FAILED=$(kubectl get pipelineruns --all-namespaces -o json 2>/dev/null | jq '[.items[] | select(.metadata.creationTimestamp | fromdateiso8601 > now - 86400 and .status.conditions[-1].status == "False")] | length')
if [ "$TOTAL" -gt 0 ] 2>/dev/null; then
  RATE=$(echo "scale=2; $FAILED * 100 / $TOTAL" | bc)
  echo "  过去 24 小时: $FAILED/$TOTAL 失败 (${RATE}%)" | tee -a $REPORT_FILE
else
  echo "  过去 24 小时无 PipelineRun" | tee -a $REPORT_FILE
fi

# 3. 卡住的 PipelineRun
echo "" | tee -a $REPORT_FILE
echo "3. 运行超过 30 分钟的 PipelineRun:" | tee -a $REPORT_FILE
kubectl get pipelineruns --all-namespaces -o json 2>/dev/null | jq -r '
  .items[] | select(.status.conditions[-1].reason == "Running" and .status.startTime != null) |
  select((now - (.status.startTime | fromdateiso8601)) > 1800) |
  "  \(.metadata.namespace)/\(.metadata.name): \((now - (.status.startTime | fromdateiso8601)) / 60 | floor) min"
' | tee -a $REPORT_FILE

echo "" | tee -a $REPORT_FILE
echo "报告已保存: $REPORT_FILE" | tee -a $REPORT_FILE
```

#### Prometheus 监控告警

```yaml
# Tekton 监控告警
groups:
- name: tekton
  rules:
  - alert: TektonPipelineRunFailed
    expr: |
      tekton_pipelines_controller_pipelinerun_count{status="failed"} > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Tekton PipelineRun 失败"
      description: "有 PipelineRun 运行失败"

  - alert: TektonPipelineRunStuck
    expr: |
      tekton_pipelines_controller_pipelinerun_duration_seconds_sum /
      tekton_pipelines_controller_pipelinerun_duration_seconds_count > 3600
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Tekton PipelineRun 执行时间过长"
      description: "PipelineRun 平均执行时间超过 1 小时"

  - alert: TektonTaskRunFailed
    expr: |
      tekton_pipelines_controller_taskrun_count{status="failed"} > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Tekton TaskRun 失败"
      description: "有 TaskRun 运行失败"

  - alert: TektonControllerDown
    expr: |
      kube_deployment_status_replicas_available{deployment="tekton-pipelines-controller"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Tekton Controller 不可用"
      description: "tekton-pipelines-controller 没有可用副本"
```

### 3.5 最佳实践

1. **不可变镜像引用**：Pipeline 中始终使用带 SHA256 的镜像引用，避免 `latest` 标签
2. **Workspace 策略**：为源代码使用 `volumeClaimTemplate`（每次新），为缓存使用共享 PVC
3. **Secret 管理**：使用 Tekton 的 `secret` workspace 类型，避免在脚本中直接引用敏感信息
4. **超时配置**：为每个 Task 和 Pipeline 配置合理的超时，防止资源无限占用
5. **并发控制**：使用 ResourceQuota 和 Pipeline 的 `taskRunSpecs` 限制资源使用
6. **日志留存**：配置外部日志收集（如 Loki），PipelineRun 删除后仍可查询历史日志
7. **缓存策略**：对 `go mod`、`npm`、`maven` 等依赖管理工具配置持久化缓存 Workspace

### 典型问题案例

#### 案例一：并行构建导致节点磁盘耗尽

**问题描述**：10 个 PipelineRun 同时执行 Kaniko 构建，节点 `/var/lib/docker` 目录迅速填满。

**根本原因**：Kaniko 默认缓存层存储在本地，多实例并行构建时未限制缓存目录大小。

**解决方案**：
1. 为 Kaniko Task 配置 `--cache=false` 或使用远程缓存 registry
2. 限制 PipelineRun 并发数：配置 namespace ResourceQuota
3. 为 Kaniko 配置 `emptyDir` 并限制大小

#### 案例二：GitHub Webhook 触发 403

**问题描述**：推送代码后 EventListener 返回 403，PipelineRun 未创建。

**根本原因**：EventListener 的 GitHub Interceptor 配置了 `secretRef`，但 GitHub Webhook 设置中的 Secret 与 Kubernetes Secret 不匹配。

**解决方案**：
1. 重新生成 GitHub Webhook Secret，确保与 Kubernetes Secret 一致
2. 检查 EventListener Service 的 Ingress/路由配置，确保外部可访问
3. 在 EventListener Pod 日志中查看 interceptor 验证详情

#### 案例三：Workspace 数据在 Task 间未传递

**问题描述**：Task A 写入 Workspace 的文件在 Task B 中不可见。

**根本原因**：两个 Task 使用了同名的不同 Workspace 声明，或 Pipeline 中 Workspace 绑定配置错误。

**解决方案**：
1. 检查 Pipeline 中 `workspaces` 的 `name` 是否与 Task 的 `workspaces.name` 匹配
2. 确认 PipelineRun 中正确绑定了 workspace（PVC/emptyDir/ConfigMap）
3. 对于 `emptyDir`，确保在同一 Pod 中（Tekton v1 的 Task 可以共享 Pod）

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git|git]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/04-backup-restore-troubleshooting|04-backup-restore-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/01-gitops-devops-troubleshooting|01-gitops-devops-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting|03-flux-image-automation-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/04-backup-restore-troubleshooting|04-backup-restore-troubleshooting]]

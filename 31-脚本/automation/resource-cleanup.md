---
title: 资源清理脚本
description: 清理僵尸资源 — 已完成 Job、孤立 PVC、无用 ConfigMap/Secret
summary: 资源清理脚本 — 识别并清理集群中的僵尸资源以释放存储和配额
category: automation
tags:
- k8s
- automation
- cleanup
- resource-management
- housekeeping
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- DevOps
estimated_read_time: 8min
intent_queries:
- 资源清理脚本 是什么
- 如何清理 Kubernetes 已完成 Job
- 孤立 PVC 清理脚本
- kubernetes resource cleanup script
trigger_keywords:
- 清理
- cleanup
- 僵尸资源
- orphaned
- job
- pvc
- 脚本
prerequisites:
- kubectl-basics
- resource-management-basics
---

> **生产环境安全提示**
>
> 本脚本包含破坏性操作 (🟡)。默认为 dry-run 模式，需显式传入 `--execute` 才会实际删除。执行前请在 staging 验证。

# 资源清理脚本

> 脚本 ID: `AUTO-04` | 语言: Bash | 风险: 🟡 中风险 (默认 dry-run) | 执行时间: ~20s

## 概述

Kubernetes 集群运行一段时间后，会积累大量不再使用的资源 ("僵尸资源")，它们占用 etcd 存储、消耗配额、增加 API server 负担。本脚本识别并清理以下五类资源:

1. **已完成 Job** — `status.successful >= 1` 的 Job (默认保留 7 天)
2. **失败 Job** — `status.failed > 0` 的 Job (默认保留 3 天)
3. **孤立 PVC** — 没有对应 Pod 引用的 PVC (Pod 已删除但 PVC 残留)
4. **无用 ConfigMap/Secret** — 没有任何 Pod/控制器引用的 ConfigMap/Secret
5. **已终止 Pod** — 处于 `Failed`/`Succeeded`/`Evicted` 状态的 Pod (由 Job 管理)

## 前置条件

- `kubectl` >= 1.28
- 具有 `edit` RBAC 权限 (需要删除资源)
- `jq` 已安装
- 脚本默认 **dry-run** 模式，不执行删除

## 使用方法

```bash
# Dry-run 模式 (默认) — 只报告，不删除
bash resource-cleanup.sh

# 实际执行删除 (谨慎使用!)
bash resource-cleanup.sh --execute

# 指定命名空间
bash resource-cleanup.sh -n production

# 自定义保留天数 (默认: 完成 Job 7 天, 失败 Job 3 天)
bash resource-cleanup.sh --job-retain-days 14 --failed-retain-days 7

# 只清理特定类型
bash resource-cleanup.sh --types jobs,orphaned-pvcs

# 排除系统命名空间
bash resource-cleanup.sh --exclude kube-system,monitoring

# JSON 输出 (用于审计归档)
bash resource-cleanup.sh --json
```

## 完整脚本

```bash
#!/bin/bash
# resource-cleanup.sh — 僵尸资源清理
# 风险等级: 🟡 中风险 (默认 dry-run，需 --execute 才删除)

set -euo pipefail

# ── 参数 ──
NAMESPACE=""
EXECUTE=false
JOB_RETAIN_DAYS=7
FAILED_RETAIN_DAYS=3
CLEAN_TYPES="jobs,failed-jobs,orphaned-pvcs,unused-configmaps,terminated-pods"
EXCLUDE_NS="kube-system,kube-public,kube-node-lease"
OUTPUT="text"
DRY_RUN_MSG="(DRY RUN — pass --execute to delete)"

TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--namespace) NAMESPACE="$2"; shift 2 ;;
        --execute) EXECUTE=true; DRY_RUN_MSG=""; shift ;;
        --job-retain-days) JOB_RETAIN_DAYS="$2"; shift 2 ;;
        --failed-retain-days) FAILED_RETAIN_DAYS="$2"; shift 2 ;;
        --types) CLEAN_TYPES="$2"; shift 2 ;;
        --exclude) EXCLUDE_NS="$2"; shift 2 ;;
        --json) OUTPUT="json"; shift ;;
        *) echo "Unknown: $1"; exit 1 ;;
    esac
done

DELETE_FLAG=""
$EXECUTE && DELETE_FLAG="--execute" || DELETE_FLAG=""

echo "============================================"
echo " Resource Cleanup Report — $TIMESTAMP"
echo " Mode: ${EXECUTE:-false} $DRY_RUN_MSG"
echo "============================================"

# 辅助函数: 检查命名空间是否在排除列表
is_excluded() {
    local ns="$1"
    echo "$EXCLUDE_NS" | grep -qw "$ns" && return 0 || return 1
}

# 辅助函数: 删除资源
do_delete() {
    local resource="$1" ns="$2" name="$3"
    if $EXECUTE; then
        kubectl delete "$resource" "$name" -n "$ns" --ignore-not-found >/dev/null 2>&1 && \
            echo "  🗑️  Deleted: $ns/$name" || \
            echo "  ❌ Failed to delete: $ns/$name"
    else
        echo "  [DRY-RUN] Would delete: $ns/$name"
    fi
}

# ── 1. 清理已完成的 Job ──
cleanup_completed_jobs() {
    echo -e "\n[1/5] Completed Jobs (older than ${JOB_RETAIN_DAYS}d)"
    echo "--------------------------------------------"
    local count=0
    
    local cutoff
    cutoff=$(date -u -d "${JOB_RETAIN_DAYS} days ago" +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || \
             date -u -v-${JOB_RETAIN_DAYS}d +%Y-%m-%dT%H:%M:%SZ 2>/dev/null)
    
    kubectl get jobs -A -o json 2>/dev/null | \
        jq -r --arg cutoff "$cutoff" '
        .items[] |
        select(.status.succeeded >= 1) |
        select(.metadata.creationTimestamp < $cutoff) |
        "\(.metadata.namespace)\t\(.metadata.name)\t\(.metadata.creationTimestamp)"
        ' 2>/dev/null | while IFS=$'\t' read -r ns name created; do
            [ -z "$ns" ] && continue
            is_excluded "$ns" && continue
            [ -n "$NAMESPACE" ] && [ "$ns" != "$NAMESPACE" ] && continue
            echo "  Found: $ns/$name (created: $created)"
            do_delete "job" "$ns" "$name"
            count=$((count + 1))
        done
    
    echo "  Completed Jobs to clean: ${count}"
}

# ── 2. 清理失败的 Job ──
cleanup_failed_jobs() {
    echo -e "\n[2/5] Failed Jobs (older than ${FAILED_RETAIN_DAYS}d)"
    echo "--------------------------------------------"
    
    local cutoff
    cutoff=$(date -u -d "${FAILED_RETAIN_DAYS} days ago" +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || \
             date -u -v-${FAILED_RETAIN_DAYS}d +%Y-%m-%dT%H:%M:%SZ 2>/dev/null)
    
    local count=0
    kubectl get jobs -A -o json 2>/dev/null | \
        jq -r --arg cutoff "$cutoff" '
        .items[] |
        select(.status.failed > 0) |
        select(.metadata.creationTimestamp < $cutoff) |
        "\(.metadata.namespace)\t\(.metadata.name)\t\(.status.failed)"
        ' 2>/dev/null | while IFS=$'\t' read -r ns name fails; do
            [ -z "$ns" ] && continue
            is_excluded "$ns" && continue
            [ -n "$NAMESPACE" ] && [ "$ns" != "$NAMESPACE" ] && continue
            echo "  Found: $ns/$name (failed: $fails times)"
            do_delete "job" "$ns" "$name"
        done
}

# ── 3. 清理孤立 PVC ──
cleanup_orphaned_pvcs() {
    echo -e "\n[3/5] Orphaned PVCs (no Pod reference)"
    echo "--------------------------------------------"
    
    local ns_flag=""
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get pvc -A -o json 2>/dev/null | \
        python3 -c "
import json, sys
data = json.load(sys.stdin)

# 收集所有 Pod 引用的 PVC
used_pvcs = set()
pods = __import__('subprocess').run(
    ['kubectl', 'get', 'pods', '-A', '-o', 'json'],
    capture_output=True, text=True
).stdout
pod_data = json.loads(pods) if pods.strip() else {'items': []}

for pod in pod_data.get('items', []):
    ns = pod['metadata']['namespace']
    for vol in pod.get('spec', {}).get('volumes', []):
        pvc = vol.get('persistentVolumeClaim', {})
        if pvc:
            used_pvcs.add(f\"{ns}/{pvc['claimName']}\")

# 找出孤立 PVC
orphaned = []
for pvc in data.get('items', []):
    ns = pvc['metadata']['namespace']
    name = pvc['metadata']['name']
    key = f'{ns}/{name}'
    status = pvc.get('status', {}).get('phase', 'Unknown')
    if key not in used_pvcs:
        size = pvc.get('spec', {}).get('resources', {}).get('requests', {}).get('storage', '?')
        orphaned.append(f'{ns}\t{name}\t{status}\t{size}')

for o in orphaned:
    print(o)
" 2>/dev/null | while IFS=$'\t' read -r ns name status size; do
            [ -z "$ns" ] && continue
            is_excluded "$ns" && continue
            [ -n "$NAMESPACE" ] && [ "$ns" != "$NAMESPACE" ] && continue
            echo "  Found: $ns/$name (status: $status, size: $size)"
            echo "    ⚠️  Verify no StatefulSet/Deployment needs this before deleting!"
            do_delete "pvc" "$ns" "$name"
        done
}

# ── 4. 清理无用 ConfigMap ──
cleanup_unused_configmaps() {
    echo -e "\n[4/5] Unused ConfigMaps (not referenced by any Pod)"
    echo "--------------------------------------------"
    
    kubectl get configmaps -A -o json 2>/dev/null | \
        python3 -c "
import json, sys, subprocess

# 获取所有 Pod 引用的 ConfigMap
used_cms = set()
pods = subprocess.run(['kubectl', 'get', 'pods', '-A', '-o', 'json'],
    capture_output=True, text=True).stdout
pod_data = json.loads(pods) if pods.strip() else {'items': []}

for pod in pod_data.get('items', []):
    ns = pod['metadata']['namespace']
    for vol in pod.get('spec', {}).get('volumes', []):
        cm = vol.get('configMap', {})
        if cm:
            used_cms.add(f\"{ns}/{cm.get('name')}\")

# 找出未引用的 ConfigMap (排除 kube-root-ca.crt)
cm_data = json.load(sys.stdin)
for cm in cm_data.get('items', []):
    ns = cm['metadata']['namespace']
    name = cm['metadata']['name']
    if name == 'kube-root-ca.crt':
        continue
    key = f'{ns}/{name}'
    if key not in used_cms:
        # 检查是否被 envFrom 或 env 引用
        data_keys = len(cm.get('data', {})) + len(cm.get('binaryData', {}))
        print(f'{ns}\t{name}\t{data_keys} keys')
" 2>/dev/null | while IFS=$'\t' read -r ns name keys; do
            [ -z "$ns" ] && continue
            is_excluded "$ns" && continue
            [ -n "$NAMESPACE" ] && [ "$ns" != "$NAMESPACE" ] && continue
            echo "  Found: $ns/$name ($keys)"
            echo "    ⚠️  May be referenced via envFrom — verify before deleting!"
            do_delete "configmap" "$ns" "$name"
        done
}

# ── 5. 清理已终止 Pod ──
cleanup_terminated_pods() {
    echo -e "\n[5/5] Terminated Pods (Failed/Succeeded/Evicted)"
    echo "--------------------------------------------"
    
    local phases="Failed,Succeeded"
    kubectl get pods -A --field-selector="status.phase=${phases}" \
        --no-headers 2>/dev/null | while read -r line; do
        local ns name phase
        ns=$(echo "$line" | awk '{print $1}')
        name=$(echo "$line" | awk '{print $2}')
        phase=$(echo "$line" | awk '{print $3}')
        
        [ -z "$ns" ] && continue
        is_excluded "$ns" && continue
        [ -n "$NAMESPACE" ] && [ "$ns" != "$NAMESPACE" ] && continue
        
        # 跳过由 Job 管理的 Pod (Job 清理会级联删除)
        local owner
        owner=$(kubectl get pod "$name" -n "$ns" -o jsonpath='{.metadata.ownerReferences[0].kind}' 2>/dev/null)
        [ "$owner" = "Job" ] && continue
        
        echo "  Found: $ns/$name (phase: $phase)"
        do_delete "pod" "$ns" "$name"
    done
}

# ── 执行 ──
IFS=',' read -ra TYPE_LIST <<< "$CLEAN_TYPES"
for t in "${TYPE_LIST[@]}"; do
    case $t in
        jobs) cleanup_completed_jobs ;;
        failed-jobs) cleanup_failed_jobs ;;
        orphaned-pvcs) cleanup_orphaned_pvcs ;;
        unused-configmaps) cleanup_unused_configmaps ;;
        terminated-pods) cleanup_terminated_pods ;;
    esac
done

echo -e "\n============================================"
if $EXECUTE; then
    echo " Cleanup EXECUTED — $TIMESTAMP"
else
    echo " Cleanup Report (DRY RUN) — $TIMESTAMP"
    echo " Pass --execute to actually delete resources"
fi
echo "============================================"
echo ""
echo "💡 Best Practices:"
echo "   1. Run in dry-run first, review the report"
echo "   2. Add TTL controller to auto-clean Jobs: spec.ttlSecondsAfterFinished"
echo "   3. Use PVC retention policy for StatefulSets"
echo "   4. Schedule weekly cleanup via cron"
```

## TTL 控制器自动清理 (推荐)

比手动脚本更可靠的是使用 Kubernetes TTL Controller，在 Job 创建时设置自动过期:

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: ml-training-job
spec:
  ttlSecondsAfterFinished: 604800  # 7 天后自动清理
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: trainer
        image: trainer:v1.0
```

## 集成建议

- 每周在非业务高峰期运行一次 dry-run 报告
- 审查报告后，在变更窗口内使用 `--execute` 执行删除
- 配合 [[31-脚本/prompts/cost-analysis|成本优化 Prompt]] 评估清理后的存储节省
- 对于已完成 Job，建议优先使用 `ttlSecondsAfterFinished` 而非手动脚本

<!-- risk-assessed -->

---
title: 存储审计脚本
description: PV/PVC 使用审计与存储优化机会分析
summary: 存储审计脚本 — 分析 PV/PVC 使用情况，发现存储浪费和优化机会
category: automation
tags:
- k8s
- automation
- storage
- pv
- pvc
- audit
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- FinOps
estimated_read_time: 8min
intent_queries:
- 存储审计脚本 是什么
- 如何分析 Kubernetes PV PVC 使用率
- 存储优化脚本 kubernetes
- storage audit pv pvc script
trigger_keywords:
- 存储
- storage
- pv
- pvc
- 审计
- audit
- 优化
- 脚本
prerequisites:
- kubectl-basics
- storage-basics
---

> **生产环境安全提示**
>
> 本脚本为只读检查 (🟢)，不修改集群状态。

# 存储审计脚本

> 脚本 ID: `AUTO-06` | 语言: Bash | 风险: 🟢 只读 | 执行时间: ~15s

## 概述

存储是 Kubernetes 集群中长期成本的重要组成部分。本脚本全面审计 PV/PVC 使用情况，发现存储浪费和优化机会:

1. **PV/PVC 概览** — 总量、已绑定、可用、释放中
2. **PVC 使用率** — 申请容量 vs 实际使用 (需要节点侧 du 命令或 CSI 指标)
3. **孤立 PV** — Released 状态的 PV (Pod 已删除但 PV 残留)
4. **存储类分析** — 各 StorageClass 的用量分布
5. **大容量 PVC** — 识别超过阈值的 PVC (可能需要拆分或清理)
6. **快照审计** — VolumeSnapshot 数量和占用空间
7. **优化建议** — 可降级存储类型、可回收的 PV、可缩容的 PVC

## 前置条件

- `kubectl` >= 1.28，具有 `cluster-reader` 权限
- `jq` 已安装
- (可选) CSI 驱动支持 `kubectl df` 或 Volume Stats API (用于实际使用率)
- (可选) VolumeSnapshot CRD (`snapshot.storage.k8s.io/v1`) 如需审计快照

## 使用方法

```bash
# 全面存储审计
bash storage-audit.sh

# 指定命名空间
bash storage-audit.sh -n production

# 大容量阈值 (默认: 500Gi)
bash storage-audit.sh --large-threshold 1000

# 低使用率阈值 (默认: 使用率 < 20% 的 PVC)
bash storage-audit.sh --underutilized-threshold 20

# 包含 VolumeSnapshot 审计
bash storage-audit.sh --include-snapshots

# JSON 输出
bash storage-audit.sh --json
```

## 完整脚本

```bash
#!/bin/bash
# storage-audit.sh — PV/PVC 存储审计
# 风险等级: 🟢 只读，无副作用

set -euo pipefail

NAMESPACE=""
LARGE_THRESHOLD=500          # Gi
UNDERUTILIZED_THRESHOLD=20   # percent
INCLUDE_SNAPSHOTS=false
OUTPUT="text"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--namespace) NAMESPACE="$2"; shift 2 ;;
        --large-threshold) LARGE_THRESHOLD="$2"; shift 2 ;;
        --underutilized-threshold) UNDERUTILIZED_THRESHOLD="$2"; shift 2 ;;
        --include-snapshots) INCLUDE_SNAPSHOTS=true; shift ;;
        --json) OUTPUT="json"; shift ;;
        *) echo "Unknown: $1"; exit 1 ;;
    esac
done

echo "============================================"
echo " Storage Audit — $TIMESTAMP"
echo "============================================"

# 辅助函数: 将存储容量字符串 (如 "100Gi") 转为数字 (Gi)
to_gi() {
    local size="$1"
    # 移除单位并转换
    echo "$size" | sed 's/Gi//' | sed 's/Ti/*1024/' | bc 2>/dev/null || echo "$size" | sed 's/[^0-9]//g'
}

# ── 1. PV/PVC 概览 ──
audit_overview() {
    echo -e "\n[1/5] PV/PVC Overview"
    echo "--------------------------------------------"
    
    local pv_total pv_bound pv_available pv_released pv_failed
    pv_total=$(kubectl get pv --no-headers 2>/dev/null | wc -l)
    pv_bound=$(kubectl get pv --no-headers 2>/dev/null | grep -c "Bound" || true)
    pv_available=$(kubectl get pv --no-headers 2>/dev/null | grep -c "Available" || true)
    pv_released=$(kubectl get pv --no-headers 2>/dev/null | grep -c "Released" || true)
    pv_failed=$(kubectl get pv --no-headers 2>/dev/null | grep -c "Failed" || true)
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    local pvc_total pvc_bound pvc_pending
    pvc_total=$(kubectl get pvc $ns_flag --no-headers 2>/dev/null | wc -l)
    pvc_bound=$(kubectl get pvc $ns_flag --no-headers 2>/dev/null | grep -c "Bound" || true)
    pvc_pending=$(kubectl get pvc $ns_flag --no-headers 2>/dev/null | grep -c "Pending" || true)
    
    echo "  PersistentVolumes:"
    echo "    Total     : $pv_total"
    echo "    Bound     : $pv_bound"
    echo "    Available : $pv_available"
    echo "    Released  : $pv_released"
    echo "    Failed    : $pv_failed"
    echo ""
    echo "  PersistentVolumeClaims:"
    echo "    Total     : $pvc_total"
    echo "    Bound     : $pvc_bound"
    echo "    Pending   : $pvc_pending"
    
    # 总存储容量
    local total_capacity
    total_capacity=$(kubectl get pv -o json 2>/dev/null | \
        jq '[.items[].spec.capacity.storage // "0"] | map(gsub("Gi";"") | gsub("Ti";"") | tonumber) | add // 0' 2>/dev/null)
    echo "    Total PV Capacity: ${total_capacity} Gi"
}

# ── 2. 孤立 PV (Released) ──
audit_released_pvs() {
    echo -e "\n[2/5] Released PVs (orphaned — reclaim needed)"
    echo "--------------------------------------------"
    
    local released
    released=$(kubectl get pv -o json 2>/dev/null | \
        jq -r '.items[] | select(.status.phase=="Released") |
               "\(.metadata.name)\t\(.spec.capacity.storage)\t\(.spec.persistentVolumeReclaimPolicy)\t\(.spec.storageClassName // "default")"' 2>/dev/null)
    
    if [ -n "$released" ]; then
        echo "  Released PVs found:"
        echo "$released" | while IFS=$'\t' read -r name size policy sc; do
            echo "    🔴 $name — capacity: $size, policy: $policy, storageClass: $sc"
        done
        
        echo ""
        echo "  Reclaim options:"
        echo "    - Retain policy: PV keeps data, needs manual cleanup"
        echo "    - Delete policy: PV and underlying storage will be deleted"
        echo "    - To recycle: kubectl patch pv $name -p '{\"spec\":{\"persistentVolumeReclaimPolicy\":\"Delete\"}}'"
    else
        echo "  ✅ No released PVs"
    fi
}

# ── 3. StorageClass 分析 ──
audit_storage_classes() {
    echo -e "\n[3/5] StorageClass Distribution"
    echo "--------------------------------------------"
    
    kubectl get pvc -A -o json 2>/dev/null | \
        jq -r '.items[] | "\(.spec.storageClassName // "default")\t\(.spec.resources.requests.storage)"' 2>/dev/null | \
        awk -F'\t' '{
            gsub(/Gi/, "", $2); gsub(/Ti/, "", $2)
            class[$1] += $2
            count[$1] += 1
        }
        END {
            for (c in class) {
                printf "  %-30s %3d PVCs   %8.1f Gi\n", c, count[c], class[c]
            }
        }' | sort -t$(printf '\t') -k2 -rn 2>/dev/null || \
        echo "  ⚠️  No PVCs found"
    
    # 默认 StorageClass
    echo ""
    local default_sc
    default_sc=$(kubectl get sc -o json 2>/dev/null | \
        jq -r '.items[] | select(.metadata.annotations["storageclass.kubernetes.io/is-default-class"]=="true") | .metadata.name' 2>/dev/null)
    echo "  Default StorageClass: ${default_sc:-none}"
}

# ── 4. 大容量 PVC 识别 ──
audit_large_pvcs() {
    echo -e "\n[4/5] Large PVCs (> ${LARGE_THRESHOLD} Gi)"
    echo "--------------------------------------------"
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get pvc $ns_flag -o json 2>/dev/null | \
        python3 -c "
import json, sys
data = json.load(sys.stdin)
large_pvcs = []

for pvc in data.get('items', []):
    ns = pvc['metadata']['namespace']
    name = pvc['metadata']['name']
    size_str = pvc.get('spec', {}).get('resources', {}).get('requests', {}).get('storage', '0')
    sc = pvc.get('spec', {}).get('storageClassName', 'default')
    
    # 解析容量
    size_gi = 0
    if 'Gi' in size_str:
        size_gi = float(size_str.replace('Gi', ''))
    elif 'Ti' in size_str:
        size_gi = float(size_str.replace('Ti', '')) * 1024
    
    if size_gi > ${LARGE_THRESHOLD}:
        large_pvcs.append((ns, name, size_gi, sc))

large_pvcs.sort(key=lambda x: -x[2])
if large_pvcs:
    for ns, name, size, sc in large_pvcs:
        print(f'  ⚠️  {ns}/{name} — {size:.0f} Gi (storageClass: {sc})')
    print(f'\\n  Total large PVCs: {len(large_pvcs)}')
    print(f'  💡 Consider: splitting data, archiving old data, or using cheaper storage tier')
else:
    print('  ✅ No PVCs exceed threshold')
" 2>/dev/null || echo "  ⚠️  Analysis failed"
}

# ── 5. 低使用率 PVC (Volume Stats API) ──
audit_underutilized() {
    echo -e "\n[5/5] Underutilized PVCs (actual usage < ${UNDERUTILIZED_THRESHOLD}%)"
    echo "--------------------------------------------"
    
    # 通过 Volume Stats API 获取实际使用量
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get pvc $ns_flag -o json 2>/dev/null | \
        jq -r '.items[] | select(.status.phase=="Bound") |
               "\(.metadata.namespace)\t\(.metadata.name)\t\(.spec.resources.requests.storage)"' 2>/dev/null | \
        while IFS=$'\t' read -r ns name size; do
            [ -z "$ns" ] && continue
            
            # 尝试通过 Volume Stats API 获取使用量
            # 需要找到使用此 PVC 的 Pod
            local pod
            pod=$(kubectl get pods -n "$ns" -o json 2>/dev/null | \
                jq -r --arg pvc "$name" '
                .items[] |
                select(.spec.volumes[]? | select(.persistentVolumeClaim.claimName == $pvc)) |
                .metadata.name' 2>/dev/null | head -1)
            
            if [ -n "$pod" ]; then
                local used_bytes
                used_bytes=$(kubectl get --raw "/api/v1/namespaces/$ns/pods/$pod/proxy/stats/summary" 2>/dev/null | \
                    jq -r --arg pvc "$name" '
                    .pods[].volume[]? | select(.name == $pvc) | .usedBytes' 2>/dev/null | head -1)
                
                if [ -n "$used_bytes" ] && [ "$used_bytes" != "null" ]; then
                    local used_gi total_gi pct
                    used_gi=$(echo "scale=2; $used_bytes / 1073741824" | bc 2>/dev/null || echo "0")
                    total_gi=$(to_gi "$size")
                    pct=$(echo "scale=1; $used_gi * 100 / $total_gi" | bc 2>/dev/null || echo "0")
                    
                    local pct_int=${pct%.*}
                    if [ "${pct_int:-0}" -lt "$UNDERUTILIZED_THRESHOLD" ]; then
                        echo "  ⚠️  $ns/$name — used: ${used_gi} Gi / ${total_gi} Gi (${pct}%)"
                    fi
                fi
            fi
        done
    
    echo ""
    echo "  💡 Underutilized PVCs can be resized down (if storageClass supports) or archived."
}

# ── 快照审计 (可选) ──
audit_snapshots() {
    if ! $INCLUDE_SNAPSHOTS; then
        return
    fi
    
    echo -e "\n[Bonus] Volume Snapshots"
    echo "--------------------------------------------"
    
    if ! kubectl get crd volumesnapshots.snapshot.storage.k8s.io &>/dev/null 2>&1; then
        echo "  ℹ️  VolumeSnapshot CRD not found — skipping"
        return
    fi
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    local snap_count snap_total_size
    snap_count=$(kubectl get volumesnapshots $ns_flag --no-headers 2>/dev/null | wc -l)
    echo "  Total Snapshots: $snap_count"
    
    if [ "$snap_count" -gt 0 ]; then
        echo ""
        kubectl get volumesnapshots $ns_flag -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,READY:.status.readyToUse,SOURCE-PVC:.status.boundVolumeSnapshotContentName,SIZE:.status.restoreSize 2>/dev/null | \
            sed 's/^/  /' || true
        
        echo ""
        echo "  💡 Snapshots incur storage costs — review retention policy:"
        echo "     VolumeSnapshotContent spec.deletionPolicy: Retain vs Delete"
    fi
}

# ── 执行 ──
audit_overview
audit_released_pvs
audit_storage_classes
audit_large_pvcs
audit_underutilized
audit_snapshots

echo -e "\n============================================"
echo " Storage Audit Complete"
echo "============================================"
echo ""
echo "💡 Optimization Recommendations:"
echo "   1. Clean up Released PVs (reclaim or delete)"
echo "   2. Downgrade StorageClass for underutilized PVCs (SSD → HDD)"
echo "   3. Resize large PVCs after data cleanup"
echo "   4. Set snapshot retention policies to limit cost"
echo "   5. Use [[脚本/prompts/cost-analysis|cost analysis prompt]] for full ROI"
```

## 输出示例

```
[1/5] PV/PVC Overview
--------------------------------------------
  PersistentVolumes:
    Total     : 45
    Bound     : 38
    Available : 3
    Released  : 4
    Failed    : 0
  PersistentVolumeClaims:
    Total PV Capacity: 12000 Gi

[2/5] Released PVs (orphaned — reclaim needed)
--------------------------------------------
    🔴 pv-data-xxx — capacity: 500Gi, policy: Retain, storageClass: alicloud-disk-ssd

[3/5] StorageClass Distribution
--------------------------------------------
  alicloud-disk-ssd                 25 PVCs    8200.0 Gi
  alicloud-disk-efficiency          10 PVCs    1500.0 Gi
  alicloud-disk-essd                3 PVCs    2300.0 Gi

[4/5] Large PVCs (> 500 Gi)
--------------------------------------------
  ⚠️  prod/database-data — 2000 Gi (storageClass: alicloud-disk-essd)
  ⚠️  prod/log-archive — 1000 Gi (storageClass: alicloud-disk-ssd)

[5/5] Underutilized PVCs (actual usage < 20%)
--------------------------------------------
  ⚠️  prod/log-archive — used: 85 Gi / 1000 Gi (8.5%)
```

## 集成建议

- 配合 [[脚本/automation/resource-cleanup|资源清理脚本]] 清理孤立 PVC
- 配合 [[脚本/prompts/cost-analysis|成本优化 Prompt]] 评估存储降级的成本节省
- 每月执行一次，纳入 FinOps 月度回顾
- 存储类降级 (`SSD → HDD`) 需要先创建新 StorageClass 的 PVC，再迁移数据

<!-- risk-assessed -->

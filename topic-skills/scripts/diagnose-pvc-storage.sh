#!/usr/bin/env bash
# Skill 07: PVC/存储故障诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
PVC="${2:?用法: $0 <namespace> <pvc-name>}"

echo "=== PVC/存储故障诊断 ==="
echo "Namespace: $NS | PVC: $PVC"
echo ""

echo "--- 1. PVC 状态 ---"
kubectl get pvc "$PVC" -n "$NS" 2>/dev/null || { echo "ERROR: PVC 不存在"; exit 1; }
kubectl describe pvc "$PVC" -n "$NS"

echo ""
echo "--- 2. StorageClass ---"
SC=$(kubectl get pvc "$PVC" -n "$NS" -o jsonpath='{.spec.storageClassName}' 2>/dev/null)
echo "StorageClass: $SC"
if [ -n "$SC" ]; then
    kubectl get sc "$SC" -o yaml 2>/dev/null | head -20
fi

echo ""
echo "--- 3. PV 状态 ---"
PV=$(kubectl get pvc "$PVC" -n "$NS" -o jsonpath='{.spec.volumeName}' 2>/dev/null)
if [ -n "$PV" ]; then
    kubectl get pv "$PV" 2>/dev/null
    kubectl describe pv "$PV" 2>/dev/null
fi

echo ""
echo "--- 4. CSI 驱动状态 ---"
kubectl get pods -n kube-system | grep -i csi | head -10

echo ""
echo "--- 5. 挂载此 PVC 的 Pod ---"
kubectl get pods -n "$NS" -o json 2>/dev/null | python3 -c "
import sys, json
data = json.load(sys.stdin)
for pod in data['items']:
    for vol in pod['spec'].get('volumes', []):
        pvc = vol.get('persistentVolumeClaim', {}).get('claimName', '')
        if pvc == '$PVC':
            print(f\"  {pod['metadata']['name']} ({pod['status']['phase']})\")
" 2>/dev/null

echo ""
echo "--- 6. 诊断建议 ---"
echo "PVC Pending: 检查 StorageClass 和 PV 可用性"
echo "PV Released: 手动 delete PV (保留 reclaimPolicy)"
echo "CSI 驱动异常: 重启 CSI controller Pod"
echo "NFS 挂载失败: 检查 NFS server 可达性和权限"

#!/bin/bash
# PVC 存储故障快速诊断脚本
# 执行时间: ~15 秒
# 风险等级: 只读操作，零风险

set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"

echo "=== PVC 存储故障快速诊断 ==="
echo "命名空间: $NAMESPACE"
echo "时间: $(date -Iseconds)"
echo ""

echo "[STEP 1] PVC 状态概览"
kubectl get pvc -n "$NAMESPACE"
echo ""

echo "[STEP 2] PV 状态概览"
kubectl get pv | grep -E "$(kubectl get pvc -n "$NAMESPACE" -o jsonpath='{.items[*].spec.volumeName}' 2>/dev/null | tr ' ' '|')" || echo "  无绑定 PV 或无法匹配"
echo ""

echo "[STEP 3] 处于 Pending 的 PVC"
kubectl get pvc -n "$NAMESPACE" | grep Pending || echo "  ✓ 无 Pending PVC"
echo ""

echo "[STEP 4] StorageClass 状态"
kubectl get storageclass
echo ""

echo "[STEP 5] CSI Driver Pod 状态"
kubectl get pods -n kube-system | grep -E "csi|snapshot" || echo "  无 CSI Driver 或不在 kube-system"
echo ""

echo "[STEP 6] 指定 Pod 的 Volume 挂载状态"
if [ -n "$POD_NAME" ]; then
  echo "Pod: $POD_NAME"
  kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .spec.volumes[*]}{"  Volume: "}{.name}{"\n  PVC: "}{.persistentVolumeClaim.claimName}{"\n\n"}{end}'
  echo "  容器挂载:"
  kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .spec.containers[*]}{"  容器: "}{.name}{"\n"}{range .volumeMounts[*]}{"    "}{.mountPath}{" <- "}{.name}{"\n"}{end}{"\n"}{end}'
  echo ""
  echo "  实际挂载状态:"
  kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .status.conditions[*]}{"  "}{.type}{": "}{.status}{"\n"}{end}'
else
  echo "  未指定 Pod，跳过 Pod 级挂载检查"
fi
echo ""

echo "[STEP 7] CSI 相关事件"
kubectl get events -n "$NAMESPACE" 2>/dev/null | grep -iE "pvc|pv|volume|csi|attach|detach" | tail -10 || echo "  无存储相关事件"
echo ""

echo "=== 快速诊断完成 ==="
echo ""
echo "常见根因:"
echo "  1. StorageClass 不存在 → 创建或修正 StorageClass"
echo "  2. CSI Driver 未运行 → 检查 CSI 组件"
echo "  3. 后端存储容量不足 → 扩容存储池"
echo "  4. PV 回收策略冲突 → 检查 Retain/Delete 策略"
echo "  5. 节点挂载失败 → 检查节点存储插件"

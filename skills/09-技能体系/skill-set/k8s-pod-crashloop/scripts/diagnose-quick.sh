#!/bin/bash
# Pod CrashLoopBackOff / OOMKilled 快速诊断脚本
# 执行时间: ~15 秒
# 风险等级: 只读操作，零风险

set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"

echo "=== Pod CrashLoopBackOff / OOMKilled 快速诊断 ==="
echo "命名空间: $NAMESPACE"
echo "时间: $(date -Iseconds)"
echo ""

if [ -z "$POD_NAME" ]; then
  echo "[STEP 1] 查找 CrashLoopBackOff / OOMKilled Pod..."
  kubectl get pods -n "$NAMESPACE" | grep -E "CrashLoopBackOff|OOMKilled|Error" || {
    echo "✓ 未发现 CrashLoopBackOff / OOMKilled Pod"
    exit 0
  }
  echo ""
  echo "请指定具体 Pod 名称继续深度诊断:"
  echo "  ./diagnose-quick.sh <namespace> <pod-name>"
  exit 1
fi

echo "[STEP 1] Pod 基本信息"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o wide
echo ""

echo "[STEP 2] Pod 状态详情"
kubectl describe pod "$POD_NAME" -n "$NAMESPACE" | grep -A5 "State:\|Reason:\|Exit Code:\|Last State:\|Restart Count:" || true
echo ""

echo "[STEP 3] 最近 Events（相关警告）"
kubectl get events -n "$NAMESPACE" --field-selector involvedObject.name="$POD_NAME" --sort-by='.lastTimestamp' | tail -10
echo ""

echo "[STEP 4] 容器退出码分析"
EXIT_CODE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[0].lastState.terminated.exitCode}' 2>/dev/null || echo "?")
echo "退出码: $EXIT_CODE"
case "$EXIT_CODE" in
  0) echo "  → 正常退出（可能是 Job 完成或优雅停机）" ;;
  1) echo "  → 通用错误（应用异常退出）" ;;
  137) echo "  → SIGKILL (OOMKilled 或强制终止)" ;;
  143) echo "  → SIGTERM (优雅终止超时)" ;;
  *) echo "  → 非常见退出码，需查看应用日志" ;;
esac
echo ""

echo "[STEP 5] 资源限制检查"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .spec.containers[*]}{"  容器: "}{.name}{"\n  请求: "}{.resources.requests}{"\n  限制: "}{.resources.limits}{"\n\n"}{end}'
echo ""

echo "[STEP 6] 最近日志（最后 30 行）"
kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=30 --previous 2>/dev/null || kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=30 2>/dev/null || echo "  无法获取日志"
echo ""

echo "=== 快速诊断完成 ==="
echo ""
echo "建议下一步:"
echo "  - 退出码 137 + 内存限制: 怀疑 OOMKilled → 运行 diagnose-deep.sh"
echo "  - 退出码 1 + 应用错误: 怀疑应用 Bug → 查看完整日志"
echo "  - 镜像拉取失败: 怀疑 ImagePullBackOff → 检查镜像名和 Secret"

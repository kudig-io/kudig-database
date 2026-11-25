#!/usr/bin/env bash
# Skill 16: 日志管道故障诊断脚本
# Agent 执行模式: L1

set -euo pipefail

echo "=== 日志管道故障诊断 ==="
echo ""

echo "--- 1. 日志组件状态 ---"
kubectl get pods -A | grep -E "fluentd|fluent-bit|loki|filebeat|logstash|vector" | head -15

echo ""
echo "--- 2. Fluent Bit / Fluentd 日志 ---"
FB_POD=$(kubectl get pods -A -l app.kubernetes.io/name=fluent-bit -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$FB_POD" ]; then
    FB_NS=$(kubectl get pods -A -l app.kubernetes.io/name=fluent-bit -o jsonpath='{.items[0].metadata.namespace}' 2>/dev/null)
    echo "Fluent Bit Pod: $FB_POD"
    kubectl logs -n "$FB_NS" "$FB_POD" --tail=30 2>/dev/null | grep -i "error\|warn\|fail" | tail -10
fi

echo ""
echo "--- 3. Loki 状态 ---"
LOKI_POD=$(kubectl get pods -A -l app.kubernetes.io/name=loki -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$LOKI_POD" ]; then
    LOKI_NS=$(kubectl get pods -A -l app.kubernetes.io/name=loki -o jsonpath='{.items[0].metadata.namespace}' 2>/dev/null)
    echo "Loki Pod: $LOKI_POD"
    kubectl logs -n "$LOKI_NS" "$LOKI_POD" --tail=20 2>/dev/null | grep -i "error\|warn" | tail -10
fi

echo ""
echo "--- 4. 日志存储 PVC ---"
kubectl get pvc -A 2>/dev/null | grep -i "loki\|elasticsearch\|logging" | head -10

echo ""
echo "--- 5. 诊断建议 ---"
echo "日志采集器 OOM: 增大 Fluent Bit memory limits"
echo "Loki 写入失败: 检查 PVC 空间和 S3 后端连通性"
echo "日志延迟: 检查 buffer 配置和网络带宽"
echo "日志丢失: 检查 Fluent Bit retry 和 backoff 配置"

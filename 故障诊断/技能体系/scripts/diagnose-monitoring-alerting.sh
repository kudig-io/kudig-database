#!/usr/bin/env bash
# Skill 15: 监控告警故障诊断脚本
# Agent 执行模式: L1

set -euo pipefail

echo "=== 监控告警故障诊断 ==="
echo ""

echo "--- 1. Prometheus 状态 ---"
kubectl get pods -A | grep prometheus | head -10

echo ""
echo "--- 2. Prometheus Targets ---"
PROM_POD=$(kubectl get pods -A -l app.kubernetes.io/name=prometheus -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
PROM_NS=$(kubectl get pods -A -l app.kubernetes.io/name=prometheus -o jsonpath='{.items[0].metadata.namespace}' 2>/dev/null)
if [ -n "$PROM_POD" ]; then
    echo "Prometheus Pod: $PROM_POD (ns: $PROM_NS)"
    kubectl logs -n "$PROM_NS" "$PROM_POD" --tail=20 2>/dev/null | grep -i "error\|warn\|scrape"
fi

echo ""
echo "--- 3. AlertManager 状态 ---"
kubectl get pods -A | grep alertmanager | head -5

echo ""
echo "--- 4. Grafana 状态 ---"
kubectl get pods -A | grep grafana | head -5

echo ""
echo "--- 5. ServiceMonitor / PodMonitor ---"
kubectl get servicemonitors -A 2>/dev/null | head -10
kubectl get podmonitors -A 2>/dev/null | head -5

echo ""
echo "--- 6. PrometheusRule ---"
kubectl get prometheusrules -A 2>/dev/null | head -10

echo ""
echo "--- 7. 诊断建议 ---"
echo "Prometheus 无法采集: 检查 ServiceMonitor selector 和 endpoints"
echo "告警不触发: 检查 PrometheusRule 语法和 AlertManager 路由"
echo "Grafana 无数据: 检查 Prometheus 数据源配置"
echo "存储满: 检查 Prometheus PVC 使用率和 retention 配置"

#!/usr/bin/env bash
# Skill 11: 控制平面故障诊断脚本
# Agent 执行模式: L2

set -euo pipefail

echo "=== 控制平面故障诊断 ==="
echo ""

echo "--- 1. 控制平面 Pod 状态 ---"
kubectl get pods -n kube-system -o wide 2>/dev/null | grep -E "apiserver|controller|scheduler|etcd"

echo ""
echo "--- 2. API Server 健康检查 ---"
kubectl get --raw /healthz 2>/dev/null || echo "API Server 不可达"

echo ""
echo "--- 3. etcd 健康检查 ---"
if command -v etcdctl &>/dev/null; then
    etcdctl endpoint health --cluster 2>/dev/null || echo "etcdctl 执行失败"
else
    echo "etcdctl 未安装"
    kubectl get pods -n kube-system -l component=etcd -o wide 2>/dev/null
fi

echo ""
echo "--- 4. Controller Manager Leader ---"
kubectl get leases -n kube-system kube-controller-manager -o yaml 2>/dev/null | grep holderIdentity

echo ""
echo "--- 5. Scheduler Leader ---"
kubectl get leases -n kube-system kube-scheduler -o yaml 2>/dev/null | grep holderIdentity

echo ""
echo "--- 6. API Server 请求延迟 ---"
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_request_duration_seconds{.*quantile=\"0.99\"" | head -5

echo ""
echo "--- 7. 诊断建议 ---"
echo "API Server 不可达: 检查 kube-apiserver Pod 和负载均衡"
echo "etcd 不健康: 检查 etcd Pod 日志和磁盘 IO"
echo "Controller Manager 无 Leader: 检查 --leader-elect 配置"
echo "请求延迟高: 检查 etcd 延迟和 webhook 响应时间"

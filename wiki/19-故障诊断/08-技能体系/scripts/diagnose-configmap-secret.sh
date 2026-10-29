#!/usr/bin/env bash
# Skill 14: ConfigMap/Secret 故障诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
NAME="${2:?用法: $0 <namespace> <configmap|secret-name>}"

echo "=== ConfigMap/Secret 故障诊断 ==="
echo "Namespace: $NS | Name: $NAME"
echo ""

echo "--- 1. ConfigMap ---"
kubectl get configmap "$NAME" -n "$NS" 2>/dev/null && kubectl describe configmap "$NAME" -n "$NS" 2>/dev/null | head -30

echo ""
echo "--- 2. Secret ---"
kubectl get secret "$NAME" -n "$NS" 2>/dev/null && kubectl describe secret "$NAME" -n "$NS" 2>/dev/null | head -20

echo ""
echo "--- 3. 引用此资源的 Pod ---"
kubectl get pods -n "$NS" -o json 2>/dev/null | python3 -c "
import sys, json
data = json.load(sys.stdin)
for pod in data['items']:
    for vol in pod['spec'].get('volumes', []):
        cm = vol.get('configMap', {}).get('name', '')
        sec = vol.get('secret', {}).get('secretName', '')
        if cm == '$NAME' or sec == '$NAME':
            print(f\"  {pod['metadata']['name']} (volume)\")
    for c in pod['spec'].get('containers', []):
        for ev in c.get('envFrom', []):
            cm = ev.get('configMapRef', {}).get('name', '')
            sec = ev.get('secretRef', {}).get('name', '')
            if cm == '$NAME' or sec == '$NAME':
                print(f\"  {pod['metadata']['name']} (envFrom)\")
        for ev in c.get('env', []):
            vfs = ev.get('valueFrom', {})
            cm = vfs.get('configMapKeyRef', {}).get('name', '')
            sec = vfs.get('secretKeyRef', {}).get('name', '')
            if cm == '$NAME' or sec == '$NAME':
                print(f\"  {pod['metadata']['name']} (env)\")
" 2>/dev/null

echo ""
echo "--- 4. 诊断建议 ---"
echo "资源不存在: 检查 namespace 和名称拼写"
echo "Pod 挂载失败: 确认 ConfigMap/Secret 在 Pod 之前创建"
echo "键不存在: 检查 key 名称是否与引用一致"
echo "Secret 类型: 确认 type 字段 (Opaque/tls/dockerconfigjson 等)"

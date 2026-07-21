---
title: Terway 测试验证
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 测试验证 是什么
- 如何 Terway 测试验证
trigger_keywords:
- Terway
- 测试验证
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 测试验证

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 05 - Terway 测试验证 (Testing & Validation)

## 技术细节

### 连通性测试

#### 基础连通性测试

```bash
#!/bin/bash
# 🟢 低风险：Terway 基础连通性测试
set -euo pipefail

NAMESPACE=${1:-default}

echo "=== Terway 连通性测试 ==="

# 1. 创建测试 Pod
echo "[1] 创建测试 Pod..."
kubectl run terway-test --image=nicolaka/netshoot -n $NAMESPACE --rm -it --restart=Never -- bash <<'EOF'
echo "--- Pod 网络信息 ---"
ip addr show
ip route show

echo "--- DNS 测试 ---"
nslookup kubernetes.default.svc.cluster.local
nslookup www.aliyun.com

echo "--- 集群内连通性 ---"
ping -c 3 kubernetes.default.svc.cluster.local

echo "--- 外部连通性 ---"
ping -c 3 8.8.8.8
curl -s -o /dev/null -w "HTTP %{http_code}" https://www.aliyun.com

echo "--- Service 连通性 ---"
curl -sk https://kubernetes.default.svc.cluster.local/healthz
EOF

echo "=== 测试完成 ==="
```

#### 跨节点连通性测试

```bash
#!/bin/bash
# 🟢 低风险：跨节点连通性测试
set -euo pipefail

echo "=== 跨节点连通性测试 ==="

# 获取两个不同节点的 Pod
POD1=$(kubectl get pods -A -o wide --no-headers | awk 'NR==1{print $1"/"$2}')
POD2=$(kubectl get pods -A -o wide --no-headers | awk 'NR==2{print $1"/"$2}')

NS1=$(echo $POD1 | cut -d'/' -f1)
NAME1=$(echo $POD1 | cut -d'/' -f2)
NS2=$(echo $POD2 | cut -d'/' -f1)
NAME2=$(echo $POD2 | cut -d'/' -f2)

# 获取 Pod2 IP
POD2_IP=$(kubectl get pod $NAME2 -n $NS2 -o jsonpath='{.status.podIP}')

echo "测试: $NS1/$NAME1 -> $NS2/$NAME2 ($POD2_IP)"

# 执行 ping 测试
kubectl exec -it $NAME1 -n $NS1 -- ping -c 5 $POD2_IP

echo "=== 测试完成 ==="
```

### 性能测试

#### 带宽测试 (iperf3)

```bash
#!/bin/bash
# 🟢 低风险：Terway 带宽测试
set -euo pipefail

NAMESPACE=${1:-default}

echo "=== Terway 带宽测试 ==="

# 1. 创建 iperf3 服务端
echo "[1] 创建 iperf3 服务端..."
kubectl run iperf-server --image=networkstatic/iperf3 -n $NAMESPACE --restart=Never -- -s
kubectl wait --for=condition=Ready pod/iperf-server -n $NAMESPACE --timeout=60s

SERVER_IP=$(kubectl get pod iperf-server -n $NAMESPACE -o jsonpath='{.status.podIP}')
echo "  服务端 IP: $SERVER_IP"

# 2. 创建 iperf3 客户端并测试
echo "[2] 执行带宽测试..."
kubectl run iperf-client --image=networkstatic/iperf3 -n $NAMESPACE --rm -it --restart=Never -- \
  -c $SERVER_IP -t 30 -P 4

# 3. 清理
echo "[3] 清理..."
kubectl delete pod iperf-server -n $NAMESPACE

echo "=== 测试完成 ==="
```

#### 延迟测试

```bash
#!/bin/bash
# 🟢 低风险：Terway 延迟测试
set -euo pipefail

NAMESPACE=${1:-default}
TARGET_IP=${2:-8.8.8.8}

echo "=== Terway 延迟测试 ==="

kubectl run latency-test --image=nicolaka/netshoot -n $NAMESPACE --rm -it --restart=Never -- bash <<EOF
echo "--- Ping 延迟测试 ---"
ping -c 100 $TARGET_IP | tail -1

echo "--- TCP 延迟测试 ---"
hping3 -S -p 443 -c 100 $TARGET_IP 2>/dev/null | tail -1 || echo "hping3 不可用"

echo "--- HTTP 延迟测试 ---"
for i in {1..10}; do
  curl -o /dev/null -s -w "请求 $i: %{time_total}s\n" https://www.aliyun.com
done
EOF

echo "=== 测试完成 ==="
```

### NetworkPolicy 测试

#### 策略验证测试

```bash
#!/bin/bash
# 🟢 低风险：NetworkPolicy 验证测试
set -euo pipefail

NAMESPACE=${1:-default}

echo "=== NetworkPolicy 验证测试 ==="

# 1. 创建测试 Pod
echo "[1] 创建测试 Pod..."
kubectl run test-frontend --image=nicolaka/netshoot -n $NAMESPACE --labels=app=frontend --restart=Never -- sleep infinity
kubectl run test-backend --image=nicolaka/netshoot -n $NAMESPACE --labels=app=backend --restart=Never -- sleep infinity
kubectl run test-other --image=nicolaka/netshoot -n $NAMESPACE --labels=app=other --restart=Never -- sleep infinity

kubectl wait --for=condition=Ready pod/test-frontend -n $NAMESPACE --timeout=60s
kubectl wait --for=condition=Ready pod/test-backend -n $NAMESPACE --timeout=60s
kubectl wait --for=condition=Ready pod/test-other -n $NAMESPACE --timeout=60s

BACKEND_IP=$(kubectl get pod test-backend -n $NAMESPACE -o jsonpath='{.status.podIP}')

# 2. 应用 NetworkPolicy
echo "[2] 应用 NetworkPolicy..."
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-only
  namespace: $NAMESPACE
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
EOF

sleep 5

# 3. 测试连通性
echo "[3] 测试连通性..."
echo "  frontend -> backend (应成功):"
kubectl exec test-frontend -n $NAMESPACE -- ping -c 3 -W 2 $BACKEND_IP && echo "  ✓ 成功" || echo "  ✗ 失败"

echo "  other -> backend (应失败):"
kubectl exec test-other -n $NAMESPACE -- ping -c 3 -W 2 $BACKEND_IP && echo "  ✗ 意外成功" || echo "  ✓ 正确拒绝"

# 4. 清理
echo "[4] 清理..."
kubectl delete networkpolicy allow-frontend-only -n $NAMESPACE
kubectl delete pod test-frontend test-backend test-other -n $NAMESPACE

echo "=== 测试完成 ==="
```

### 集成测试

#### Service 集成测试

```bash
#!/bin/bash
# 🟢 低风险：Service 集成测试
set -euo pipefail

NAMESPACE=${1:-default}

echo "=== Service 集成测试 ==="

# 1. 创建测试 Deployment
echo "[1] 创建测试 Deployment..."
kubectl create deployment test-web --image=nginx -n $NAMESPACE --replicas=3
kubectl wait --for=condition=Available deploy/test-web -n $NAMESPACE --timeout=120s

# 2. 创建 Service
echo "[2] 创建 Service..."
kubectl expose deployment test-web -n $NAMESPACE --port=80 --target-port=80

# 3. 测试 ClusterIP
echo "[3] 测试 ClusterIP..."
SVC_IP=$(kubectl get svc test-web -n $NAMESPACE -o jsonpath='{.spec.clusterIP}')
kubectl run curl-test --image=curlimages/curl -n $NAMESPACE --rm -it --restart=Never -- \
  curl -s -o /dev/null -w "HTTP %{http_code}" http://$SVC_IP

# 4. 测试 DNS
echo "[4] 测试 DNS..."
kubectl run dns-test --image=nicolaka/netshoot -n $NAMESPACE --rm -it --restart=Never -- \
  nslookup test-web.$NAMESPACE.svc.cluster.local

# 5. 清理
echo "[5] 清理..."
kubectl delete deployment test-web -n $NAMESPACE
kubectl delete svc test-web -n $NAMESPACE

echo "=== 测试完成 ==="
```

### 验证检查清单

| 序号 | 检查项 | 验证命令 | 通过标准 |
|-----|--------|---------|----------|
| 1 | Pod IP 分配 | `kubectl get pods -o wide` | 所有 Pod 有 IP |
| 2 | DNS 解析 | `nslookup kubernetes.default` | 解析成功 |
| 3 | 集群内连通 | `ping <pod-ip>` | 0% 丢包 |
| 4 | 外部连通 | `ping 8.8.8.8` | 0% 丢包 |
| 5 | Service 访问 | `curl http://<svc-ip>` | HTTP 200 |
| 6 | NetworkPolicy | 策略测试脚本 | 按预期允许/拒绝 |
| 7 | 带宽 | `iperf3 -c <ip>` | > 1Gbps |
| 8 | 延迟 | `ping -c 100 <ip>` | < 1ms (同 AZ) |

### 自动化测试脚本

```bash
#!/bin/bash
# 🟢 低风险：Terway 完整测试套件
set -euo pipefail

NAMESPACE="terway-test-$(date +%s)"

echo "=== Terway 完整测试套件 ==="
echo "命名空间: $NAMESPACE"

# 创建测试命名空间
kubectl create namespace $NAMESPACE

# 运行所有测试
./test-connectivity.sh $NAMESPACE
./test-performance.sh $NAMESPACE
./test-networkpolicy.sh $NAMESPACE
./test-service.sh $NAMESPACE

# 清理
kubectl delete namespace $NAMESPACE

echo "=== 所有测试完成 ==="
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|networkpolicy]]

## Related

- [[aeraki-mesh]] — Aeraki Mesh
- [[submariner]] — Submariner
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- 45-terway-testing-validation

<!-- risk-assessed -->

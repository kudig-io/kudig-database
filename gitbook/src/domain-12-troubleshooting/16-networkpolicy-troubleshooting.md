# 16 - NetworkPolicy 故障排查 (NetworkPolicy Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Network Policies](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

---

## 1. NetworkPolicy 故障诊断总览 (NetworkPolicy Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **策略未生效** | 网络仍然可达/阻断 | 安全边界失效 | P1 - 高 |
| **过度限制** | 合法流量被阻断 | 服务功能异常 | P0 - 紧急 |
| **CNI插件不支持** | Policy创建失败 | 策略无法实施 | P1 - 高 |
| **规则冲突** | 策略行为不一致 | 网络访问混乱 | P2 - 中 |
| **性能下降** | 网络延迟增加 | 用户体验差 | P2 - 中 |

### 1.2 NetworkPolicy 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    NetworkPolicy 故障诊断架构                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       Pod网络通信层                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │  (frontend) │    │  (backend)  │    │  (database) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CNI网络插件层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Calico    │    │   Cilium    │    │   Flannel   │              │  │
│  │  │ (支持Policy)│    │ (支持Policy)│    │ (不支持)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                   NetworkPolicy 控制层                               │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     kube-apiserver                            │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │ NetworkPolicy│  │   Event     │  │ Controller  │           │  │  │
│  │  │  │   CRD       │  │   Handler   │  │   Watcher   │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                   策略实施引擎                                        │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │  iptables   │    │   eBPF      │    │   IPVS      │              │  │
│  │  │   规则链     │    │  程序       │    │   规则      │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. NetworkPolicy 基础验证 (Basic Validation)

### 2.1 CNI插件支持检查

```bash
# ========== 1. 检查CNI插件类型 ==========
# 查看当前使用的CNI插件
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'

# 检查CNI配置文件
cat /etc/cni/net.d/*.conf

# ========== 2. 插件支持能力验证 ==========
# Calico支持检查
kubectl get pods -n calico-system
kubectl get networkpolicies --all-namespaces

# Cilium支持检查  
kubectl get pods -n kube-system | grep cilium
cilium status

# ========== 3. 策略创建测试 ==========
# 创建测试策略
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: test-policy
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: test
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: client
EOF

# 验证策略是否被接受
kubectl get networkpolicy test-policy
kubectl describe networkpolicy test-policy
```

### 2.2 网络策略基本语法检查

```bash
# ========== 策略格式验证 ==========
# 检查必需字段
kubectl explain networkpolicy.spec

# 常见语法错误检查
# 1. 缺少policyTypes
# 2. selector标签格式错误
# 3. 端口范围配置错误
# 4. CIDR格式不正确

# ========== 策略有效性测试 ==========
# 使用dry-run验证
kubectl apply -f policy.yaml --dry-run=client -o yaml

# 使用服务器端验证
kubectl apply -f policy.yaml --server-side --dry-run=server
```

---

## 3. 策略未生效问题排查 (Policy Not Working Troubleshooting)

### 3.1 排查决策树

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   NetworkPolicy 未生效排查流程                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   网络策略似乎未生效                                                        │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 验证CNI插件是否支持NetworkPolicy              │                 │
│   │ kubectl get pods -n <cni-namespace>                  │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 不支持 ──▶ 更换CNI插件或调整安全策略                           │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查策略语法和配置                            │                 │
│   │ kubectl describe networkpolicy <name>                │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 语法错误 ──▶ 修正策略配置                                    │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 验证Pod标签匹配                                │                 │
│   │ kubectl get pods --show-labels                       │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 标签不匹配 ──▶ 调整Pod标签或策略selector                      │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查网络规则实施                              │                 │
│   │ 在节点上检查iptables/eBPF规则                         │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 详细诊断命令

```bash
# ========== 1. 策略状态检查 ==========
# 列出所有NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 检查特定策略详情
kubectl describe networkpolicy <policy-name> -n <namespace>

# 检查策略事件
kubectl get events --field-selector involvedObject.kind=NetworkPolicy

# ========== 2. Pod标签验证 ==========
# 检查目标Pod标签
kubectl get pods -n <namespace> --show-labels

# 验证策略selector匹配
kubectl get pods -n <namespace> -l <label-selector>

# ========== 3. CNI插件状态检查 ==========
# Calico检查
kubectl get pods -n calico-system
calicoctl get networkpolicy --all-namespaces

# Cilium检查
kubectl get pods -n kube-system -l k8s-app=cilium
cilium connectivity test

# ========== 4. 网络规则检查 ==========
# 在节点上检查iptables规则
kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- sh
# 在debug容器中执行:
iptables -L -n -v | grep -i networkpolicy

# 检查eBPF程序 (Cilium)
bpftool prog list | grep cilium

# ========== 5. 连通性测试 ==========
# 创建测试Pod
kubectl run debug-client --image=busybox -it --rm -- sh
# 在容器内测试网络连通性
ping <target-pod-ip>
wget -qO- http://<target-service>

# 使用netshoot工具箱
kubectl run netshoot --image=nicolaka/netshoot -it --rm -- sh
```

---

## 4. 过度限制问题排查 (Over Restriction Troubleshooting)

### 4.1 访问被意外阻断

```bash
# ========== 1. 确定阻断范围 ==========
# 测试同Namespace访问
kubectl run test-pod --image=busybox -n <same-namespace> -it --rm -- sh

# 测试跨Namespace访问
kubectl run test-pod --image=busybox -n <different-namespace> -it --rm -- sh

# ========== 2. 策略影响分析 ==========
# 查找影响特定Pod的所有策略
kubectl get networkpolicy --all-namespaces -o json | jq '.items[] | select(.spec.podSelector.matchLabels.app=="<app-name>")'

# 检查默认拒绝策略
kubectl get networkpolicy -n <namespace> -o jsonpath='{.items[*].spec.policyTypes}'

# ========== 3. 逐步排除法 ==========
# 临时删除策略测试
kubectl delete networkpolicy <policy-name> -n <namespace>

# 恢复策略并观察
kubectl apply -f <policy-backup.yaml>

# ========== 4. 日志分析 ==========
# 查看CNI插件日志
kubectl logs -n calico-system -l k8s-app=calico-node
kubectl logs -n kube-system -l k8s-app=cilium
```

### 4.2 默认策略问题

```bash
# ========== 默认拒绝策略检查 ==========
# 检查是否有隐式默认拒绝
kubectl get networkpolicy -n <namespace> -o yaml | grep -A10 "policyTypes"

# 显式允许所有流量的策略模板
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-all-ingress
  namespace: <namespace>
spec:
  podSelector: {}
  ingress:
  - {}
  policyTypes:
  - Ingress
EOF

# ========== 逐步放行策略 ==========
# 允许同一Namespace内部通信
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: <namespace>
spec:
  podSelector: {}
  ingress:
  - from:
    - podSelector: {}
  policyTypes:
  - Ingress
EOF
```

---

## 5. 复杂策略配置问题 (Complex Policy Configuration)

### 5.1 多层策略冲突

```bash
# ========== 策略优先级分析 ==========
# 查看所有相关策略
kubectl get networkpolicy -n <namespace> -o wide

# 按应用分组策略
kubectl get networkpolicy -n <namespace> -o json | jq '.items[] | {name: .metadata.name, app: .spec.podSelector.matchLabels.app}'

# ========== 策略组合效果测试 ==========
# 创建测试场景
cat <<EOF > test-scenario.yaml
apiVersion: v1
kind: Pod
metadata:
  name: test-source
  namespace: <namespace>
  labels:
    app: test-client
spec:
  containers:
  - name: client
    image: busybox
    command: ["sleep", "3600"]
---
apiVersion: v1
kind: Pod
metadata:
  name: test-target
  namespace: <namespace>
  labels:
    app: test-server
spec:
  containers:
  - name: server
    image: nginx
EOF

kubectl apply -f test-scenario.yaml

# 测试连通性
kubectl exec test-source -n <namespace> -- wget -qO- http://test-target.<namespace>.svc.cluster.local
```

### 5.2 Egress策略问题

```bash
# ========== 外部访问策略 ==========
# 允许访问特定外部IP
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-external-dns
  namespace: <namespace>
spec:
  podSelector:
    matchLabels:
      app: <app-name>
  policyTypes:
  - Egress
  egress:
  - to:
    - ipBlock:
        cidr: 8.8.8.8/32  # Google DNS
    ports:
    - protocol: UDP
      port: 53
EOF

# ========== DNS解析问题 ==========
# 允许DNS查询
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-access
  namespace: <namespace>
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
EOF
```

---

## 6. 性能和监控 (Performance and Monitoring)

### 6.1 策略性能影响

```bash
# ========== 网络延迟测试 ==========
# 基准测试（无策略）
kubectl run baseline-test --image=networkstatic/iperf3 -it --rm -- sh
# iperf3 -c <server-ip> -t 30

# 应用策略后测试
# 对比网络性能变化

# ========== 连接数监控 ==========
# 检查连接跟踪表
kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- sh
# conntrack -L | wc -l

# ========== CNI插件性能指标 ==========
# Calico指标
kubectl port-forward -n calico-system svc/calico-typha 9098:9098
curl http://localhost:9098/metrics | grep networkpolicy

# Cilium指标
kubectl port-forward -n kube-system svc/hubble-relay 4245:80
hubble observe --last 100
```

### 6.2 监控告警配置

```bash
# ========== 策略违规检测 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: networkpolicy-violations
  namespace: monitoring
spec:
  groups:
  - name: networkpolicy.rules
    rules:
    - alert: NetworkPolicyViolation
      expr: increase(container_network_receive_bytes_total{networkpolicy_violation="true"}[5m]) > 0
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "Network policy violation detected"
        
    - alert: NetworkPolicyControllerDown
      expr: up{job="network-policy-controller"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Network policy controller is down"
EOF

# ========== 策略有效性验证脚本 ==========
#!/bin/bash
# validate-networkpolicies.sh

NAMESPACE=${1:-default}

echo "Validating NetworkPolicies in namespace: $NAMESPACE"

# 检查是否有默认拒绝策略
if ! kubectl get networkpolicy -n $NAMESPACE 2>/dev/null | grep -q "default-deny"; then
    echo "WARNING: No default deny policy found in $NAMESPACE"
fi

# 检查孤立的策略
POLICIES=$(kubectl get networkpolicy -n $NAMESPACE -o jsonpath='{.items[*].metadata.name}')
for policy in $POLICIES; do
    SELECTOR=$(kubectl get networkpolicy $policy -n $NAMESPACE -o jsonpath='{.spec.podSelector.matchLabels}')
    if [ -n "$SELECTOR" ]; then
        POD_COUNT=$(kubectl get pods -n $NAMESPACE -l $SELECTOR --no-headers | wc -l)
        if [ $POD_COUNT -eq 0 ]; then
            echo "WARNING: Policy $policy targets no pods (selector: $SELECTOR)"
        fi
    fi
done

echo "Validation complete"
```

---

## 7. 最佳实践和预防措施 (Best Practices and Prevention)

### 7.1 策略设计原则

```bash
# ========== 分层安全策略 ==========
# 1. 默认拒绝所有流量
# 2. 显式允许必需的通信
# 3. 最小权限原则
# 4. 定期审查和清理

# ========== 策略模板示例 ==========
# 应用层策略模板
cat <<EOF > app-networkpolicy-template.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: APP_NAME-policy
  namespace: NAMESPACE
spec:
  podSelector:
    matchLabels:
      app: APP_NAME
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许来自前端的流量
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  # 允许访问数据库
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - protocol: TCP
      port: 5432
  # 允许DNS查询
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
EOF
```

### 7.2 自动化验证

```bash
# ========== CI/CD集成 ==========
# 策略linting脚本
#!/bin/bash
# networkpolicy-lint.sh

POLICY_FILE=$1

# 检查必需字段
required_fields=("apiVersion" "kind" "metadata" "spec")
for field in "${required_fields[@]}"; do
    if ! grep -q "^$field:" "$POLICY_FILE"; then
        echo "ERROR: Missing required field: $field"
        exit 1
    fi
done

# 检查策略类型
if ! grep -q "policyTypes:" "$POLICY_FILE"; then
    echo "WARNING: policyTypes not specified, may lead to unexpected behavior"
fi

# 检查空selector
if grep -q "podSelector: {}" "$POLICY_FILE"; then
    echo "WARNING: Empty podSelector applies to all pods"
fi

echo "Policy validation passed"

# ========== 定期清理脚本 ==========
#!/bin/bash
# cleanup-stale-policies.sh

# 删除超过30天未更新的策略
kubectl get networkpolicy --all-namespaces -o json | \
jq -r '.items[] | select(.metadata.creationTimestamp < "'$(date -d '30 days ago' -Iseconds)'") | .metadata.namespace + "/" + .metadata.name' | \
while read policy; do
    echo "Deleting stale policy: $policy"
    kubectl delete networkpolicy -n ${policy%/*} ${policy##*/}
done
```

---
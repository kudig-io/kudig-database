# 01 - NetworkPolicy 深度实践指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 高级

---

## 目录

1. [生产环境 NetworkPolicy 设计原则](#1-生产环境-networkpolicy-设计原则)
2. [核心概念与最佳实践](#2-核心概念与最佳实践)
3. [典型应用场景实践](#3-典型应用场景实践)
4. [高级策略配置模式](#4-高级策略配置模式)
5. [多集群 NetworkPolicy 管理](#5-多集群-networkpolicy-管理)
6. [监控与审计](#6-监控与审计)
7. [故障排查与调试](#7-故障排查与调试)

---

## 1. 生产环境 NetworkPolicy 设计原则

### 1.1 零信任安全模型

```yaml
# 默认拒绝所有流量 - 零信任起点
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
```

### 1.2 分层安全策略

```yaml
# 第一层：命名空间隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: namespace-isolation
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: production  # 只允许同命名空间
```

### 1.3 最小权限原则

```yaml
# 第二层：应用间最小权限通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-to-backend
  namespace: production
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
    ports:
    - protocol: TCP
      port: 8080
```

---

## 2. 核心概念与最佳实践

### 2.1 选择器组合策略

#### 2.1.1 AND 逻辑（严格匹配）

```yaml
# 同时满足命名空间和Pod标签
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: strict-access-control
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:      # AND 逻辑
        matchLabels:
          env: production
      podSelector:            # 必须同时满足
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 5432
```

#### 2.1.2 OR 逻辑（灵活匹配）

```yaml
# 满足任一条件即可
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: flexible-access
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
  - Ingress
  ingress:
  - from:                     # 第一组条件
    - podSelector:
        matchLabels:
          app: frontend
  - from:                     # 第二组条件（OR）
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
```

### 2.2 出站策略设计

```yaml
# 限制 Pod 出站访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-egress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Egress
  egress:
  # 允许 DNS 查询
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  
  # 允许访问数据库
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - protocol: TCP
      port: 5432
  
  # 允许访问外部 API（白名单）
  - to:
    - ipBlock:
        cidr: 100.100.0.0/16
        except:
        - 100.100.10.0/24    # 排除特定子网
    ports:
    - protocol: TCP
      port: 443
```

### 2.3 跨命名空间通信

```yaml
# 允许监控系统访问应用指标
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-monitoring
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 9090
    - protocol: TCP
      port: 8080
```

---

## 3. 典型应用场景实践

### 3.1 微服务架构隔离

```yaml
# 前端服务 NetworkPolicy
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许来自 LoadBalancer/Ingress 的流量
  - from:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  egress:
  # 允许调用后端 API
  - to:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080

---
# API 网关 NetworkPolicy
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-gateway-policy
  namespace: ecommerce
spec:
  podSelector:
    matchLabels:
      app: api-gateway
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许前端调用
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  # 允许调用业务服务
  - to:
    - podSelector:
        matchLabels:
          service: user-service
    - podSelector:
        matchLabels:
          service: order-service
    - podSelector:
        matchLabels:
          service: inventory-service
    ports:
    - protocol: TCP
      port: 8080
```

### 3.2 数据库安全防护

```yaml
# 数据库 NetworkPolicy
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-security
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: postgresql
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 只允许应用层访问
  - from:
    - podSelector:
        matchLabels:
          tier: application
    ports:
    - protocol: TCP
      port: 5432
  # 允许备份工具访问
  - from:
    - namespaceSelector:
        matchLabels:
          name: backup
      podSelector:
        matchLabels:
          app: backup-tool
    ports:
    - protocol: TCP
      port: 5432
  egress:
  # 允许 DNS 查询
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
```

### 3.3 多租户环境隔离

```yaml
# 租户 A 隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-a-isolation
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 只允许同租户内部通信
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: a
  egress:
  # 限制出站到特定 CIDR
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8
        except:
        - 10.10.0.0/16    # 排除其他租户网络

---
# 租户 B 隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-b-isolation
  namespace: tenant-b
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: b
  egress:
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8
        except:
        - 10.20.0.0/16    # 排除其他租户网络
```

---

## 4. 高级策略配置模式

### 4.1 动态策略管理

```yaml
# 使用标签选择器动态管理策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: dynamic-policy-template
  namespace: production
spec:
  podSelector:
    matchExpressions:
    - key: security-tier
      operator: In
      values: [high, medium]
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          security-level: high
    ports:
    - protocol: TCP
      port: 8080
```

### 4.2 策略继承模式

```yaml
# 基础策略模板
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: base-security-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      security-profile: standard
  policyTypes:
  - Ingress
  ingress:
  # 基础安全规则
  - from:
    - namespaceSelector:
        matchLabels:
          trusted: "true"
    ports:
    - protocol: TCP
      port: 80

---
# 扩展策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: extended-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      security-profile: enhanced
  policyTypes:
  - Ingress
  ingress:
  # 继承基础规则 + 扩展规则
  - from:
    - namespaceSelector:
        matchLabels:
          trusted: "true"
    ports:
    - protocol: TCP
      port: 80
  - from:
    - podSelector:
        matchLabels:
          app: admin-console
    ports:
    - protocol: TCP
      port: 8080
```

### 4.3 策略版本控制

```yaml
# 策略版本管理示例
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-policy-v2
  namespace: production
  annotations:
    policy/version: "2.0"
    policy/approved-by: "security-team"
    policy/review-date: "2026-02-03"
spec:
  podSelector:
    matchLabels:
      app: api-service
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          env: production
      podSelector:
        matchLabels:
          tier: frontend
    ports:
    - protocol: TCP
      port: 8080
```

---

## 5. 多集群 NetworkPolicy 管理

### 5.1 跨集群策略同步

```yaml
# 多集群统一策略模板
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: cross-cluster-policy
  namespace: shared-services
  labels:
    policy-type: cross-cluster
    cluster-group: production
spec:
  podSelector:
    matchLabels:
      app: shared-service
  policyTypes:
  - Ingress
  ingress:
  # 允许来自所有生产集群的访问
  - from:
    - namespaceSelector:
        matchLabels:
          cluster-type: production
    ports:
    - protocol: TCP
      port: 8080
```

### 5.2 策略联邦管理

```yaml
# 使用策略联邦工具配置
# 示例：使用 KubeFed 管理跨集群策略
apiVersion: types.kubefed.io/v1beta1
kind: FederatedNetworkPolicy
metadata:
  name: federated-security-policy
spec:
  template:
    spec:
      podSelector:
        matchLabels:
          app: critical-service
      policyTypes:
      - Ingress
      ingress:
      - from:
        - namespaceSelector:
            matchLabels:
              security-level: high
        ports:
        - protocol: TCP
          port: 443
  placement:
    clusters:
    - name: prod-cluster-1
    - name: prod-cluster-2
    - name: prod-cluster-3
```

---

## 6. 监控与审计

### 6.1 策略效果监控

```yaml
# Prometheus 监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: networkpolicy-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: network-observer
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s

---
# 网络策略指标收集
apiVersion: v1
kind: ConfigMap
metadata:
  name: networkpolicy-dashboard
  namespace: monitoring
data:
  dashboard.json: |
    {
      "panels": [
        {
          "title": "NetworkPolicy 阻断统计",
          "targets": [
            {
              "expr": "sum(rate(networkpolicy_dropped_packets_total[5m])) by (namespace, policy)",
              "legendFormat": "{{namespace}}/{{policy}}"
            }
          ]
        },
        {
          "title": "策略匹配率",
          "targets": [
            {
              "expr": "rate(networkpolicy_matched_packets_total[5m]) / rate(networkpolicy_total_packets[5m])",
              "legendFormat": "匹配率"
            }
          ]
        }
      ]
    }
```

### 6.2 审计日志配置

```yaml
# 启用 NetworkPolicy 审计
apiVersion: audit.k8s.io/v1
kind: Policy
metadata:
  name: networkpolicy-audit
rules:
- level: RequestResponse
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "delete"]
```

### 6.3 合规性检查

```bash
#!/bin/bash
# NetworkPolicy 合规性检查脚本

echo "=== NetworkPolicy 合规性检查报告 ==="
echo "检查时间: $(date)"
echo

# 检查默认拒绝策略
echo "1. 检查默认拒绝策略:"
kubectl get networkpolicy -A | grep "default-deny" || echo "❌ 未发现默认拒绝策略"

# 检查关键应用策略
echo -e "\n2. 关键应用策略检查:"
for ns in production staging; do
  echo "Namespace: $ns"
  kubectl get networkpolicy -n $ns | grep -E "(database|api-server|frontend)" || echo "  ❌ 关键应用缺少策略"
done

# 检查策略覆盖率
echo -e "\n3. 策略覆盖率分析:"
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
  while read pod; do
    ns=$(echo $pod | cut -d'/' -f1)
    pod_name=$(echo $pod | cut -d'/' -f2)
    policy_count=$(kubectl get networkpolicy -n $ns --field-selector spec.podSelector.matchLabels.app=$(kubectl get pod $pod_name -n $ns -o jsonpath='{.metadata.labels.app}') 2>/dev/null | wc -l)
    if [ $policy_count -eq 0 ]; then
      echo "  ❌ $pod 无关联策略"
    fi
  done
```

---

## 7. 故障排查与调试

### 7.1 常见问题诊断

```bash
# NetworkPolicy 诊断命令集合

# 1. 检查 CNI 插件支持
echo "=== CNI 插件检查 ==="
kubectl get pods -n kube-system | grep -E "(calico|cilium|weave)"

# 2. 验证策略是否生效
echo -e "\n=== 策略生效检查 ==="
kubectl get networkpolicy -A -o wide

# 3. 检查 Pod 标签匹配
echo -e "\n=== Pod 标签检查 ==="
kubectl get pods -n production --show-labels

# 4. 测试网络连通性
echo -e "\n=== 网络连通性测试 ==="
kubectl run debug-pod --rm -it --image=busybox -- sh -c "
  echo 'Testing connectivity...'
  nc -zv database-service 5432 2>&1 || echo 'Connection failed'
"

# 5. 查看策略详细信息
echo -e "\n=== 策略详情 ==="
kubectl describe networkpolicy <policy-name> -n <namespace>

# 6. 检查 CNI 日志
echo -e "\n=== CNI 日志 ==="
kubectl logs -n kube-system -l k8s-app=calico-node | grep -i policy
```

### 7.2 调试工具推荐

```yaml
# 网络调试工具 Pod
apiVersion: v1
kind: Pod
metadata:
  name: network-debugger
  namespace: production
spec:
  containers:
  - name: debugger
    image: nicolaka/netshoot
    command: ["sleep", "3600"]
    securityContext:
      capabilities:
        add: ["NET_ADMIN", "SYS_PTRACE"]
```

```bash
# 使用调试工具
kubectl exec -it network-debugger -n production -- bash

# 在调试容器中执行
# 1. 查看网络命名空间
ip netns list

# 2. 检查 iptables 规则
iptables-save | grep -i networkpolicy

# 3. 测试连接
nc -zv database-pod-ip 5432

# 4. 抓包分析
tcpdump -i any host database-pod-ip and port 5432
```

### 7.3 性能影响评估

```bash
#!/bin/bash
# NetworkPolicy 性能影响评估

echo "=== NetworkPolicy 性能评估 ==="

# 基准测试
echo "1. 无策略基准测试:"
kubectl run baseline-test --rm -it --image=busybox -- sh -c "
  time nc -zv database-service 5432
"

# 策略启用后测试
echo -e "\n2. 策略启用后测试:"
kubectl run policy-test --rm -it --image=busybox -- sh -c "
  time nc -zv database-service 5432
"

# 监控指标收集
echo -e "\n3. 收集性能指标:"
kubectl top pods -n production | grep -E "(database|frontend)"
```

---

## 最佳实践总结

1. **从默认拒绝开始**：始终以零信任原则为基础
2. **分层设计策略**：命名空间 → 应用 → Pod 逐层细化
3. **定期审查策略**：确保策略与业务需求保持一致
4. **建立监控体系**：实时监控策略效果和性能影响
5. **文档化策略意图**：清晰记录每个策略的设计目的
6. **测试驱动部署**：在生产环境部署前充分测试
7. **渐进式实施**：从小范围开始，逐步扩展策略覆盖

通过以上实践，可以构建既安全又高效的 Kubernetes 网络策略体系。
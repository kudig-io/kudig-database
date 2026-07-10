---
title: Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro-Segmentation Practice)
description: '# Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro-Segmentation
  Practice)'
summary: '本文档深入探讨了Kubernetes网络策略与安全微隔离的实现原理、配置方法和最佳实践，基于大规模生产环境的网络安全实践经验，提供从基础网络策略到高级微隔离的完整技术指南，帮助企业构建零信任网络架构。'
  Practice)'
category: papers
tags:
- k8s
- papers
- research
- prometheus
- cilium
- calico
- ingress
- gateway
- rbac
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- 架构师
- 技术决策者
- 研究员
estimated_read_time: 5min
intent_queries:
- Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro-Segmentation Practice)
  是什么
- 如何 Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro-Segmentation Practice)
- Kubernetes 19 papers 最佳实践
trigger_keywords:
- Kubernetes
- 网络策略与安全微隔离实践
- Network
- Policies
- and
- Security
- Micro-Segmentation
- Practice
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 网络策略与安全微隔离实践 ([[domain-17-system-foundation/知识字典/networking/network-policies.md|Network Policies]] and Security Micro-Segmentation Practice)

> **作者**: 网络安全架构专家 | **版本**: v2.0 | **更新时间**: 2026-03-03
> **适用场景**: 企业级网络安全防护 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文档深入探讨了Kubernetes网络策略与安全微隔离的实现原理、配置方法和最佳实践，基于大规模生产环境的网络安全实践经验，提供从基础网络策略到高级微隔离的完整技术指南，帮助企业构建零信任网络架构。

<!-- chunk: 1. 网络策略基础概念 -->## 1. 网络策略基础概念

## 1.1 网络策略核心原理

```yaml
网络策略工作原理:
  1. 选择器匹配
     - Pod选择器 (podSelector)
     - 命名空间选择器 (namespaceSelector)
     - IP块选择器 (ipBlock)
  
  2. 规则类型
     - Ingress: 入站流量控制
     - Egress: 出站流量控制
  
  3. 策略执行
     - 默认拒绝 (Default Deny)
     - 白名单模式 (Allow-list)
     - 逐层叠加 (Layered Application)
```

## 1.2 网络策略生命周期

```mermaid
graph TD
    A[策略定义] --> B[策略验证]
    B --> C[策略分发]
    C --> D[CNI处理]
    D --> E[防火墙规则生成]
    E --> F[策略生效]
    F --> G[监控审计]
    
    D --> H[策略冲突检测]
    H --> I[策略优化建议]
    I --> A
```

<!-- chunk: 2. 高级网络策略配置 -->## 2. 高级网络策略配置

## 2.1 默认拒绝策略

```yaml
# 默认拒绝所有流量策略
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
---
# 允许集群内部DNS查询
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
---
# 允许健康检查和监控
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-health-check
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
    ports:
    - protocol: TCP
      port: 8080
```

## 2.2 微服务间通信策略

```yaml
# 电商应用微服务网络策略
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
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  - to:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 8080
---
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
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: user-service
    ports:
    - protocol: TCP
      port: 8080
  - to:
    - podSelector:
        matchLabels:
          app: order-service
    ports:
    - protocol: TCP
      port: 8080
  - to:
    - podSelector:
        matchLabels:
          app: payment-service
    ports:
    - protocol: TCP
      port: 8080
```

<!-- chunk: 3. CNI网络插件深度集成 -->## 3. CNI网络插件深度集成

## 3.1 Calico高级配置

```yaml
# Calico网络策略高级配置
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: security-defaults
spec:
  selector: all()
  order: 100
  ingress:
  - action: Allow
    protocol: ICMP
  - action: Allow
    protocol: TCP
    source:
      selector: has(projectcalico.org/namespace)
    destination:
      ports: [22, 80, 443, 6443, 2379, 2380, 10250, 10251, 10252]
  - action: Deny
    source:
      nets: ["0.0.0.0/0"]
  egress:
  - action: Allow
    protocol: ICMP
  - action: Allow
    protocol: TCP
    destination:
      selector: has(projectcalico.org/namespace)
  - action: Allow
    protocol: UDP
    destination:
      ports: [53]
      selector: has(projectcalico.org/namespace)
  - action: Deny
    destination:
      nets: ["0.0.0.0/0"]
---
# Calico网络集成功约
apiVersion: projectcalico.org/v3
kind: NetworkPolicy
metadata:
  name: allow-external-api
  namespace: api-services
spec:
  selector: app == 'external-api'
  types:
  - Ingress
  - Egress
  ingress:
  - action: Allow
    protocol: TCP
    source:
      nets:
      - 10.0.0.0/8
      - 172.16.0.0/12
      - 192.168.0.0/16
    destination:
      ports:
      - 443
  egress:
  - action: Allow
    protocol: TCP
    destination:
      nets:
      - 130.211.0.0/22
      - 35.191.0.0/16
      - 146.148.0.0/20
      ports:
      - 443
```

## 3.2 Cilium eBPF策略

```yaml
# Cilium eBPF网络策略
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: advanced-security-policy
  namespace: secure-app
spec:
  endpointSelector:
    matchLabels:
      app: secure-service
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: trusted-client
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/v1/.*"
        - method: "POST"
          path: "/api/v1/users"
    - ports:
      - port: "9090"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/metrics"
  egress:
  - toCIDR:
    - "10.0.0.0/8"
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
  - toEntities:
    - world
    - cluster
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
---
# Cilium L7策略 - HTTP/HTTPS
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: http-security-policy
  namespace: web-app
spec:
  endpointSelector:
    matchLabels:
      app: web-service
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: api-client
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/public/.*"
        - method: "POST"
          path: "/api/.*"
          headers:
          - "X-API-Key: .*"
        - method: "PUT"
          path: "/api/users/.*"
          headers:
          - "Authorization: Bearer .*"
```

<!-- chunk: 4. 安全微隔离架构 -->## 4. 安全微隔离架构

## 4.1 零信任网络架构

```yaml
# 零信任网络策略框架
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: zero-trust-framework
  namespace: security-zone
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          security-level: trusted
      podSelector:
        matchLabels:
          role: authenticated
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          security-level: trusted
      podSelector:
        matchLabels:
          role: authorized
    ports:
    - protocol: TCP
      port: 8080
---
# 基于角色的访问控制
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: role-based-access
  namespace: rbac-zone
spec:
  podSelector:
    matchLabels:
      role: admin
  policyTypes:
  - Egress
  egress:
  - to:
    - podSelector:
        matchLabels:
          protected: true
    ports:
    - protocol: TCP
      port: 9090
    - protocol: TCP
      port: 9093
  - to:
    - namespaceSelector:
        matchLabels:
          name: admin-tools
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: developer-access
  namespace: rbac-zone
spec:
  podSelector:
    matchLabels:
      role: developer
  policyTypes:
  - Egress
  egress:
  - to:
    - podSelector:
        matchLabels:
          environment: development
    ports:
    - protocol: TCP
      port: 8080
```

## 4.2 安全域隔离

```yaml
# 安全域隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: dmz-zone-policy
  namespace: dmz
spec:
  podSelector:
    matchLabels:
      zone: dmz
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: internal
    ports:
    - protocol: TCP
      port: 8080
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: internal-zone-policy
  namespace: internal
spec:
  podSelector:
    matchLabels:
      zone: internal
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: dmz
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 5432
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-zone-policy
  namespace: database
spec:
  podSelector:
    matchLabels:
      zone: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: internal
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 5432
```

<!-- chunk: 5. 网络策略验证与测试 -->## 5. 网络策略验证与测试

## 5.1 策略验证工具

```python
#!/usr/bin/env python3
# network-policy-validator.py

import yaml
import json
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import ipaddress
import re

class NetworkPolicyValidator:
    def __init__(self):
        config.load_incluster_config()
        self.v1 = client.NetworkingV1Api()
        self.core_v1 = client.CoreV1Api()
    
    def validate_network_policy(self, policy):
        """验证网络策略配置"""
        errors = []
        
        # 验证选择器
        if 'podSelector' in policy.spec:
            if 'matchLabels' in policy.spec.podSelector:
                errors.extend(self._validate_match_labels(policy.spec.podSelector.matchLabels))
        
        # 验证策略类型
        if hasattr(policy.spec, 'policyTypes'):
            errors.extend(self._validate_policy_types(policy.spec.policyTypes))
        
        # 验证入站规则
        if hasattr(policy.spec, 'ingress'):
            errors.extend(self._validate_ingress_rules(policy.spec.ingress))
        
        # 验证出站规则
        if hasattr(policy.spec, 'egress'):
            errors.extend(self._validate_egress_rules(policy.spec.egress))
        
        return errors
    
    def _validate_match_labels(self, match_labels):
        """验证匹配标签"""
        errors = []
        
        for key, value in match_labels.items():
            # 验证标签键格式
            if not re.match(r'^([A-Za-z0-9][-A-Za-z0-9_.]*)?[A-Za-z0-9]$', key):
                errors.append(f"无效的标签键: {key}")
            
            # 验证标签值格式
            if not re.match(r'^(([A-Za-z0-9][-A-Za-z0-9_.]*)?[A-Za-z0-9])?$', str(value)):
                errors.append(f"无效的标签值: {value}")
        
        return errors
    
    def _validate_policy_types(self, policy_types):
        """验证策略类型"""
        errors = []
        
        for policy_type in policy_types:
            if policy_type not in ['Ingress', 'Egress']:
                errors.append(f"无效的策略类型: {policy_type}")
        
        return errors
    
    def _validate_ingress_rules(self, ingress_rules):
        """验证入站规则"""
        errors = []
        
        for rule in ingress_rules:
            if hasattr(rule, 'from'):
                for from_rule in rule.from_:
                    if hasattr(from_rule, 'ipBlock'):
                        errors.extend(self._validate_ip_block(from_rule.ipBlock))
            
            if hasattr(rule, 'ports'):
                errors.extend(self._validate_ports(rule.ports))
        
        return errors
    
    def _validate_egress_rules(self, egress_rules):
        """验证出站规则"""
        errors = []
        
        for rule in egress_rules:
            if hasattr(rule, 'to'):
                for to_rule in rule.to:
                    if hasattr(to_rule, 'ipBlock'):
                        errors.extend(self._validate_ip_block(to_rule.ipBlock))
            
            if hasattr(rule, 'ports'):
                errors.extend(self._validate_ports(rule.ports))
        
        return errors
    
    def _validate_ip_block(self, ip_block):
        """验证IP块"""
        errors = []
        
        try:
            ipaddress.IPv4Network(ip_block.cidr, strict=False)
        except ValueError:
            errors.append(f"无效的CIDR: {ip_block.cidr}")
        
        return errors
    
    def _validate_ports(self, ports):
        """验证端口配置"""
        errors = []
        
        for port in ports:
            if hasattr(port, 'port'):
                port_val = port.port
                if isinstance(port_val, str):
                    # 端口名称
                    if not re.match(r'^[a-zA-Z0-9]([-a-zA-Z0-9]*[a-zA-Z0-9])?$', port_val):
                        errors.append(f"无效的端口名称: {port_val}")
                else:
                    # 端口号
                    if not (1 <= port_val <= 65535):
                        errors.append(f"端口号超出范围: {port_val}")
        
        return errors
    
    def analyze_policy_conflicts(self, namespace=None):
        """分析策略冲突"""
        conflicts = []
        
        # 获取所有网络策略
        if namespace:
            policies = self.v1.list_namespaced_network_policy(namespace).items
        else:
            policies = self.v1.list_network_policy_for_all_namespaces().items
        
        # 检查策略重叠
        for i, policy1 in enumerate(policies):
            for j, policy2 in enumerate(policies[i+1:], i+1):
                if self._policies_overlap(policy1, policy2):
                    conflicts.append({
                        'policy1': policy1.metadata.name,
                        'policy2': policy2.metadata.name,
                        'namespace1': policy1.metadata.namespace,
                        'namespace2': policy2.metadata.namespace,
                        'type': 'overlap'
                    })
        
        return conflicts
    
    def _policies_overlap(self, policy1, policy2):
        """检查策略是否重叠"""
        # 简化的重叠检查
        selector1 = getattr(policy1.spec, 'podSelector', {})
        selector2 = getattr(policy2.spec, 'podSelector', {})
        
        # 如果两个策略应用于相同的Pod集合，则可能存在冲突
        return selector1 == selector2

if __name__ == "__main__":
    validator = NetworkPolicyValidator()
    
    # 验证所有网络策略
    try:
        all_policies = validator.v1.list_network_policy_for_all_namespaces()
        for policy in all_policies.items:
            errors = validator.validate_network_policy(policy)
            if errors:
                print(f"策略 {policy.metadata.namespace}/{policy.metadata.name} 存在错误:")
                for error in errors:
                    print(f"  - {error}")
    except ApiException as e:
        print(f"API调用失败: {e}")
```

## 5.2 网络连通性测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# network-connectivity-test.sh

# 网络连通性测试脚本
NAMESPACE="test-namespace"
SOURCE_POD="client-pod"
TARGET_POD="server-pod"
TARGET_PORT=8080

echo "=== 网络连通性测试 ==="

# 1. 创建测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: connectivity-tester
  namespace: $NAMESPACE
spec:
  containers:
  - name: tester
    image: nicolaka/netshoot:latest
    command: ["/bin/sh", "-c", "sleep 3600"]
EOF

# 2. 等待Pod就绪
echo "等待测试Pod就绪..."
kubectl wait --for=condition=Ready pod/connectivity-tester -n $NAMESPACE --timeout=60s

# 3. 执行连通性测试
echo "执行连通性测试..."

# 测试TCP连接
echo "1. TCP连接测试:"
kubectl exec -n $NAMESPACE connectivity-tester -- timeout 10 bash -c "echo >/dev/tcp/$TARGET_POD/$TARGET_PORT && echo '连接成功' || echo '连接失败'"

# 测试HTTP请求
echo "2. HTTP请求测试:"
kubectl exec -n $NAMESPACE connectivity-tester -- timeout 10 curl -I http://$TARGET_POD:$TARGET_PORT/health 2>/dev/null || echo "HTTP请求失败"

# 测试DNS解析
echo "3. DNS解析测试:"
kubectl exec -n $NAMESPACE connectivity-tester -- nslookup $TARGET_POD.$NAMESPACE.svc.cluster.local

# 测试网络延迟
echo "4. 网络延迟测试:"
kubectl exec -n $NAMESPACE connectivity-tester -- ping -c 3 $TARGET_POD.$NAMESPACE.svc.cluster.local

# 5. 清理测试Pod
echo "清理测试资源..."
kubectl delete pod connectivity-tester -n $NAMESPACE

echo "=== 网络连通性测试完成 ==="
```
<!-- chunk: 6. 监控与审计 -->## 6. 监控与审计

## 6.1 网络策略监控

```yaml
# 网络策略监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: network-policy-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: calico-kube-controllers
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'calico_(.*)'
      targetLabel: __name__
---
# 网络流量监控
apiVersion: v1
kind: ConfigMap
metadata:
  name: network-metrics-config
  namespace: monitoring
data:
  config.yaml: |
    # 网络指标配置
    metrics:
      - name: calico_policy_evaluation_seconds
        help: "Policy evaluation time"
        type: histogram
      
      - name: calico_active_policies
        help: "Number of active policies"
        type: gauge
      
      - name: calico_connections_allowed_total
        help: "Total allowed connections"
        type: counter
      
      - name: calico_connections_denied_total
        help: "Total denied connections"
        type: counter
```

## 6.2 网络审计策略

```yaml
# 网络审计配置
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
- level: RequestResponse
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "delete", "patch"]

- level: Metadata
  resources:
  - group: ""
    resources: ["pods", "services", "endpoints"]
  verbs: ["create", "update", "delete"]
  omitStages:
  - RequestReceived

- level: Request
  userGroups: ["system:authenticated"]
  verbs: ["create", "update"]
  omitStages:
  - RequestReceived
```

<!-- chunk: 7. 高级安全策略 -->## 7. 高级安全策略

## 7.1 威胁检测策略

```yaml
# 威胁检测网络策略
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: threat-detection-policy
  namespace: security
spec:
  endpointSelector:
    matchLabels:
      app: monitored-service
  ingress:
  - fromEndpoints:
    - {}
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/.*"
          # 检测SQL注入
          headersMatch:
          - headerName: "User-Agent"
            regex: "sqlmap|nikto|nessus"
        - method: "POST"
          path: "/.*"
          headersMatch:
          - headerName: "Content-Type"
            regex: "application/x-www-form-urlencoded"
          # 检测恶意请求体
          bodyMatch:
          - regex: "union.*select|drop.*table|exec.*sp_|alert\\(|\\<script"
  egress:
  - toCIDR:
    - "0.0.0.0/0"
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/.*"
          # 检测C&C通信特征
          headersMatch:
          - headerName: "User-Agent"
            regex: "curl|wget|python-urllib|Go-http-client"
```

## 7.2 异常行为检测

```yaml
# 异常行为检测策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: anomaly-detection
  namespace: monitoring
spec:
  podSelector:
    matchLabels:
      security: monitored
  policyTypes:
  - Egress
  egress:
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
    # 限制出站连接频率
    # 通过外部监控系统实现
---
# 通过自定义控制器实现的高级策略
apiVersion: security.example.com/v1
kind: AnomalyDetectionPolicy
metadata:
  name: advanced-anomaly-detection
spec:
  selectors:
    matchLabels:
      app: critical-service
  detectionRules:
  - name: connection-rate-limit
    type: rate
    threshold: 100
    window: 60s
    action: alert
  - name: port-scan-detection
    type: pattern
    pattern: "multiple-destinations"
    threshold: 10
    window: 30s
    action: block
  - name: data-exfiltration
    type: volume
    threshold: 10485760  # 10MB
    window: 300s
    action: alert
```

<!-- chunk: 8. 最佳实践与实施指南 -->## 8. 最佳实践与实施指南

## 8.1 网络策略设计原则

```markdown
<!-- chunk: 🛡️ 网络策略设计原则 -->## 🛡️ 网络策略设计原则

## 1. 最小权限原则
- 只允许必需的网络连接
- 从拒绝所有开始，逐步开放
- 定期审查和清理策略

## 2. 分层安全架构
- 实现网络分段隔离
- 建立安全域边界
- 实施深度防御策略

## 3. 可观测性优先
- 启用详细的网络日志
- 实施实时监控告警
- 建立审计追踪机制

## 4. 自动化管理
- 使用IaC管理网络策略
- 实施策略验证流水线
- 建立自动修复机制
```

## 8.2 实施检查清单

```yaml
网络策略实施检查清单:
  策略设计:
    ☐ 网络拓扑分析完成
    ☐ 服务依赖关系映射
    ☐ 安全需求评估完成
    ☐ 默认拒绝策略制定
  
  策略部署:
    ☐ 测试环境验证完成
    ☐ 策略分阶段部署
    ☐ 连通性测试通过
    ☐ 性能影响评估
  
  监控告警:
    ☐ 网络指标监控配置
    ☐ 异常行为告警设置
    ☐ 审计日志收集配置
    ☐ 告警响应流程建立
  
  运维管理:
    ☐ 策略变更管理流程
    ☐ 定期审查机制建立
    ☐ 应急响应预案制定
    ☐ 团队培训完成
```

<!-- chunk: 9. AdminNetworkPolicy与eBPF策略执行 — 2026更新 -->## 9. AdminNetworkPolicy与eBPF策略执行 — 2026更新

## 9.1 AdminNetworkPolicy (集群级网络策略)

AdminNetworkPolicy (ANP) 和 BaselineAdminNetworkPolicy (BANP) 是Kubernetes原生的集群级网络策略API，在K8s 1.32+进入Beta。它们补充了命名空间级NetworkPolicy，为平台管理员提供全局策略控制能力。

```yaml
# AdminNetworkPolicy vs NetworkPolicy权限层级
策略优先级(从高到低):
  1. AdminNetworkPolicy (ANP): 管理员强制策略，不可被命名空间策略覆盖
  2. NetworkPolicy: 命名空间级策略，开发者自主管理
  3. BaselineAdminNetworkPolicy (BANP): 管理员默认基线策略，可被NetworkPolicy覆盖
  
  执行逻辑:
    ANP(Allow/Deny/Pass) → NetworkPolicy(Allow/Deny) → BANP(Allow/Deny)
    - ANP Allow/Deny: 最终决策，跳过后续评估
    - ANP Pass: 委托给NetworkPolicy评估
    - NetworkPolicy匹配: 执行策略决策
    - 无NetworkPolicy匹配: 交给BANP评估
    - BANP Allow/Deny: 最终兜底决策
---
# AdminNetworkPolicy示例：管理员强制禁止访问元数据服务
apiVersion: policy.networking.k8s.io/v1alpha1
kind: AdminNetworkPolicy
metadata:
  name: deny-cloud-metadata
spec:
  priority: 10  # 数字越小优先级越高
  subject:
    namespaces:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values: ["kube-system"]
  egress:
    - name: "deny-metadata-service"
      action: Deny
      to:
        - networks:
            - "169.254.169.254/32"  # AWS/GCP元数据服务
      ports:
        - portNumber:
            port: 80
            protocol: TCP
        - portNumber:
            port: 443
            protocol: TCP
---
# AdminNetworkPolicy：强制允许监控采集
apiVersion: policy.networking.k8s.io/v1alpha1
kind: AdminNetworkPolicy
metadata:
  name: allow-monitoring-scrape
spec:
  priority: 20
  subject:
    namespaces: {}  # 所有命名空间
  ingress:
    - name: "allow-prometheus"
      action: Allow
      from:
        - namespaces:
            matchLabels:
              kubernetes.io/metadata.name: monitoring
      ports:
        - portNumber:
            port: 9090
            protocol: TCP
        - portNumber:
            port: 8080
            protocol: TCP
---
# BaselineAdminNetworkPolicy：默认拒绝所有跨命名空间流量
apiVersion: policy.networking.k8s.io/v1alpha1
kind: BaselineAdminNetworkPolicy
metadata:
  name: default-deny-cross-namespace
spec:
  subject:
    namespaces: {}
  ingress:
    - name: "deny-cross-namespace"
      action: Deny
      from:
        - namespaces:
            notSameLabels:
              - kubernetes.io/metadata.name
  egress:
    - name: "allow-dns"
      action: Allow
      to:
        - namespaces:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - portNumber:
            port: 53
            protocol: UDP
        - portNumber:
            port: 53
            protocol: TCP
    - name: "deny-cross-namespace-egress"
      action: Deny
      to:
        - namespaces:
            notSameLabels:
              - kubernetes.io/metadata.name
```

## 9.2 Cilium替代传统CNI的网络策略增强

```yaml
# Cilium网络策略与ANP的协同工作
Cilium与AdminNetworkPolicy集成:
  Cilium 1.16+:
    - 完整支持AdminNetworkPolicy和BaselineAdminNetworkPolicy
    - ANP在eBPF datapath中优先于CiliumNetworkPolicy执行
    - 策略判决通过Hubble可观测
    
  策略层级(Cilium环境):
    1. AdminNetworkPolicy (K8s原生)
    2. CiliumClusterwideNetworkPolicy (Cilium集群级)
    3. CiliumNetworkPolicy / NetworkPolicy (命名空间级)
    4. BaselineAdminNetworkPolicy (K8s基线)
    
  迁移到Cilium CNI:
    - 现有NetworkPolicy保持兼容(无需修改)
    - 额外获得L7策略和eBPF加速
    - 详细迁移指南见 "[18-eBPF与Cilium深度实践](./18-kubernetes-ebpf-cilium-deep-practice.md)"
```

## 9.3 eBPF网络策略执行引擎

```yaml
eBPF vs iptables策略执行对比:
  | 维度 | iptables (Calico等) | eBPF (Cilium) |
  |------|-------------------|---------------|
  | 规则查找 | O(n) 线性遍历 | O(1) eBPF Map哈希 |
  | 规则更新 | 全量替换(锁) | 增量更新(无锁) |
  | 10K规则时延迟 | 显著增加 | 基本无影响 |
  | conntrack | 内核nf_conntrack | eBPF CT Map |
  | L7感知 | 不支持 | 支持(HTTP/gRPC/DNS) |
  | 策略审计 | 需额外工具 | Hubble原生支持 |
  | 身份感知 | IP地址匹配 | 安全身份(Security Identity) |
  
  Cilium安全身份机制:
    原理: 每个Endpoint分配数字身份(基于标签)
    优势:
      - IP地址变化不影响策略(身份不变)
      - 策略匹配效率高(数字比较 vs CIDR匹配)
      - 支持跨集群身份同步(ClusterMesh)
    eBPF Map结构:
      - Identity Map: IP → Identity映射
      - Policy Map: (SrcIdentity, DstIdentity, Port) → Verdict
```

<!-- chunk: 10. 未来发展趋势 -->## 10. 未来发展趋势

## 10.1 智能化网络策略

```yaml
智能化网络策略趋势(2026-2027):
  1. AI驱动的策略生成
     - 基于流量分析自动生成策略
     - 智能异常检测和响应
     - 预测性安全防护
  
  2. 零信任网络演进
     - 身份感知的网络策略
     - 动态访问控制决策
     - 行为分析驱动的策略
  
  3. eBPF全面替代iptables
     - Cilium成为企业级CNI标准
     - AdminNetworkPolicy + CiliumNetworkPolicy协同
     - eBPF LSM增强运行时安全
     - 详见 "[18-eBPF与Cilium深度实践](./18-kubernetes-ebpf-cilium-deep-practice.md)"
  
  4. Gateway API与网络策略统一
     - 流量管理与安全策略统一管理
     - 详见 "[19-Gateway API与现代流量管理](./19-kubernetes-gateway-api-modern-traffic-management.md)"
```

---
*本文档基于企业级网络安全实践经验编写，持续更新最新技术和最佳实践。2026-03更新：新增AdminNetworkPolicy、eBPF策略执行引擎内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-19-papers KUDIG Database — Global MOC
- [[domain-19-landscape-references/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers...]]
- index.md|Domain-19 论文与参考 — 开源项目索引]]
- Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framew...
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Op...
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Imp...
- Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Archit...
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- Kubernetes 成本治理与 FinOps 实践 (Kubernetes Cost Governance and F...
- Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface ...
- Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and ...
- Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)

## See Also

- 06-kubernetes-cost-governance-finops-practice
- 07-kubernetes-csi-storage-deep-practice
- 09-kubernetes-service-mesh-istio-integration
- 10-kubernetes-automation-sre-practices

## Related

- [[papers|#papers Hub]] — tag hub

- research/ — tag hub


<!-- risk-assessed -->

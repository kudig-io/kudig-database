---
title: 78 - NetworkPolicy高级配置
description: 'title: 78 - NetworkPolicy高级配置'
summary: 'title: 78 - NetworkPolicy高级配置'
category: general
tags:
- k8s
- networking
- service
- ingress
- prometheus
- cilium
- flannel
- calico
- coredns
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Network Policy是什么？
- 如何使用Network Policy？
- Network Policy的最佳实践是什么？
trigger_keywords:
- NetworkPolicy高级配置
- networking
- traffic
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- cilium-basics
- cni-basics
---



title: 78 - NetworkPolicy高级配置
description: '# 78 - NetworkPolicy高级配置'
category: networking
tags:
- k8s
- networking
- [[Service|service]]
- [[Ingress|ingress]]
- cni
- prometheus
- cilium
- calico
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy高级配置 是什么
- 如何 NetworkPolicy高级配置
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- NetworkPolicy高级配置
- networking
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# 78 - NetworkPolicy高级配置

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/services-networking/network-policies](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

<!-- chunk: NetworkPolicy规则类型 -->
## NetworkPolicy规则类型

| 规则类型 | 方向 | 选择器 | 说明 | 默认行为 |
|---------|-----|-------|------|---------|
| **Ingress** | 入站 | from | 控制进入Pod的流量 | 全部允许 |
| **Egress** | 出站 | to | 控制离开Pod的流量 | 全部允许 |

<!-- chunk: 选择器类型 -->
## 选择器类型

| 选择器 | 说明 | 示例 | 注意事项 |
|-------|------|------|---------|
| **podSelector** | Pod标签选择 | `app: frontend` | 同命名空间内 |
| **namespaceSelector** | 命名空间选择 | `env: production` | 跨命名空间 |
| **ipBlock** | IP CIDR | `10.0.0.0/8` | 外部流量 |
| **组合选择器** | 与/或逻辑 | 见下文 | 注意逻辑关系 |

<!-- chunk: 选择器逻辑说明 -->
## 选择器逻辑说明

```yaml
# AND逻辑 - 同一个from/to项中的多个选择器
ingress:
- from:
  - namespaceSelector:      # AND
      matchLabels:
        env: production
    podSelector:            # 必须同时满足
      matchLabels:
        app: frontend

# OR逻辑 - 多个from/to项
ingress:
- from:
  - namespaceSelector:      # OR
      matchLabels:
        env: production
  - podSelector:            # 满足任一即可
      matchLabels:
        app: monitoring
```

<!-- chunk: 默认拒绝所有入站 -->
## 默认拒绝所有入站

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}  # 选择所有Pod
  policyTypes:
  - Ingress
  # 无ingress规则 = 拒绝所有入站
```

<!-- chunk: 默认拒绝所有出站 -->
## 默认拒绝所有出站

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  # 无egress规则 = 拒绝所有出站
```

<!-- chunk: 允许同命名空间通信 -->
## 允许同命名空间通信

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector: {}  # 同命名空间所有Pod
```

<!-- chunk: 多层应用网络策略 -->
## 多层应用网络策略

```yaml
# 前端策略 - 允许Ingress和后端访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: frontend
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
      port: 80
  egress:
  - to:
    - podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 8080
  - to:  # DNS
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
---
# 后端策略 - 允许前端和数据库访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: backend
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          tier: database
    ports:
    - protocol: TCP
      port: 5432
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
---
# 数据库策略 - 仅允许后端访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 5432
```

<!-- chunk: 跨命名空间访问 -->
## 跨命名空间访问

```yaml
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
```

<!-- chunk: IP Block规则 -->
## IP Block规则

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-external
spec:
  podSelector:
    matchLabels:
      app: external-api
  policyTypes:
  - Egress
  egress:
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
        except:
        - 10.0.0.0/8
        - 172.16.0.0/12
        - 192.168.0.0/16
    ports:
    - protocol: TCP
      port: 443
```

<!-- chunk: 端口范围(v1.25+) -->
## 端口范围(v1.25+)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: port-range
spec:
  podSelector:
    matchLabels:
      app: multiport
  policyTypes:
  - Ingress
  ingress:
  - ports:
    - protocol: TCP
      port: 8000
      endPort: 9000  # 端口范围8000-9000
```

<!-- chunk: Cilium L7策略 -->
## Cilium L7策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: l7-policy
spec:
  endpointSelector:
    matchLabels:
      app: api
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "80"
        protocol: TCP
      rules:
        http:
        - method: GET
          path: "/api/v1/.*"
        - method: POST
          path: "/api/v1/users"
```

<!-- chunk: 策略验证 -->
## 策略验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 检查策略
kubectl get networkpolicy -A
kubectl describe networkpolicy <name>

# 测试连通性
kubectl exec -it <source-pod> -- curl -v <target-service>
kubectl exec -it <source-pod> -- nc -zv <target-ip> <port>

# Cilium策略状态
cilium policy get
cilium endpoint list
cilium monitor --type policy-verdict

# Calico策略状态
calicoctl get networkpolicy -A
calicoctl get globalnetworkpolicy

# 网络策略调试Pod
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash
```

<!-- chunk: NetworkPolicy最佳实践 -->
## NetworkPolicy最佳实践

| 实践 | 说明 | 优先级 |
|-----|------|-------|
| **默认拒绝** | 每个命名空间设置默认拒绝策略 | P0 |
| **最小权限** | 仅开放必需的端口和来源 | P0 |
| **DNS例外** | 确保允许DNS流量(UDP 53) | P0 |
| **标签规范** | 使用统一的标签命名规范 | P1 |
| **分层策略** | 按应用层级设置策略 | P1 |
| **策略审计** | 定期审查和测试策略 | P1 |
| **文档化** | 记录策略设计意图 | P2 |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 | 影响 |
|------|---------|------|
| v1.25 | 端口范围支持GA | 简化多端口配置 |
| v1.27 | NetworkPolicy状态改进 | 更好的可观测性 |
| v1.28 | AdminNetworkPolicy Alpha | 集群级策略 |
| v1.29 | BaselineAdminNetworkPolicy | 默认策略支持 |
| v1.30 | 网络策略日志增强 | 审计能力提升 |

---

**NetworkPolicy原则**: 默认拒绝 + 最小权限开放 + 确保DNS访问 + 分层策略设计 + 持续审计验证

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 15-coredns-plugins-reference
- 16-networkpolicy-deep-practice
- 18-network-encryption-mtls
- 19-ingress-fundamentals

## Related

- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]

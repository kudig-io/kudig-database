---
title: NetworkPolicy
description: NetworkPolicy — Kubernetes 生产运维知识库
summary: NetworkPolicy — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- networkpolicy
- security
- firewall
- network-isolation
- cilium
- flannel
- calico
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy 是什么
- 如何 NetworkPolicy
trigger_keywords:
- NetworkPolicy
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# NetworkPolicy

## Role

NetworkPolicy is a Kubernetes resource that defines how [[pods|Pods]] communicate with each other and external network endpoints. It acts as a Pod-level firewall.

**Important**: NetworkPolicy requires CNI plugin support (Calico, Cilium, or other compatible CNIs). Flannel does NOT support NetworkPolicy.

## Policy Structure

NetworkPolicy selects target Pods via `podSelector` and defines:

| Policy Type | Controls |
|-------------|----------|
| **[[ingress\|Ingress]]** | Incoming traffic to selected Pods |
| **Egress** | Outgoing traffic from selected Pods |

Traffic sources/destinations can be specified via:
- `podSelector`: Match Pods by labels (same namespace by default)
- `namespaceSelector`: Match namespaces by labels
- `ipBlock`: CIDR ranges (with optional exceptions)

## Default-Deny Pattern

Apply a default-deny policy to establish zero-trust networking:

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
spec:
  podSelector: {}  # Selects all Pods
  policyTypes:
  - Ingress
  - Egress
```

Then add explicit allow policies for required traffic flows.

## Use Cases

- Isolate tenant namespaces from each other
- Restrict database access to specific application Pods
- Allow egress only to required external endpoints
- Microsegmentation for PCI-DSS or HIPAA compliance

## 完整策略示例

### 数据库访问控制

```yaml
# 仅允许 backend Pod 访问数据库
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-backend-to-db
  namespace: database
spec:
  podSelector:
    matchLabels:
      app: postgresql
  policyTypes: [Ingress]
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: production
      podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 5432
```

### 限制 Egress 到外部

```yaml
# 仅允许访问 DNS 和特定外部 API
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-egress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: worker
  policyTypes: [Egress]
  egress:
  # 允许 DNS
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
  # 允许访问特定外部 API
  - to:
    - ipBlock:
        cidr: 203.0.113.0/24
    ports:
    - protocol: TCP
      port: 443
  # 允许访问集群内服务
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: production
```

### 跨命名空间通信

```yaml
# 允许 monitoring 命名空间采集所有 Pod 指标
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-monitoring-scrape
  namespace: production
spec:
  podSelector: {}  # 所有 Pod
  policyTypes: [Ingress]
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: monitoring
    ports:
    - protocol: TCP
      port: 9090
    - protocol: TCP
      port: 8080
```

## 运维操作

```bash
# 🟢 查看命名空间中的 NetworkPolicy
kubectl get networkpolicy -n <namespace>
kubectl describe networkpolicy <name> -n <namespace>

# 🟢 检查 CNI 是否支持 NetworkPolicy
kubectl get pods -n kube-system -l k8s-app=calico-node  # Calico
kubectl get pods -n kube-system -l k8s-app=cilium  # Cilium

# 🟢 测试网络连通性
kubectl run test-pod --rm -it --image=nicolaka/netshoot -- bash
# 在 test-pod 内：
curl -sv http://<target-svc>:<port>/ 2>&1 | head -5
nslookup <target-svc>.<ns>.svc.cluster.local

# 🟡 应用 NetworkPolicy
kubectl apply -f networkpolicy.yaml

# 🔴 删除 NetworkPolicy（可能导致流量被默认拒绝或允许）
kubectl delete networkpolicy <name> -n <namespace>

# 🟢 Cilium 策略审计模式
kubectl exec -n kube-system cilium-xxxx -- cilium policy get
kubectl exec -n kube-system cilium-xxxx -- cilium monitor --type drop
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 应用无法连接 | NetworkPolicy 拒绝 | `kubectl get netpol -n <ns>` | 添加允许规则 |
| 策略不生效 | CNI 不支持 | 检查 CNI 插件类型 | 更换为 Calico/Cilium |
| DNS 解析失败 | Egress 策略未允许 DNS | 检查 egress 规则 | 添加 UDP/TCP 53 允许 |
| 跨命名空间不通 | namespaceSelector 不匹配 | `kubectl get ns --show-labels` | 修正 label 选择器 |
| 间歇性连接失败 | 策略竞争/顺序 | 检查所有 netpol | 简化策略/消除冲突 |

### 排查流程

```
NetworkPolicy 导致连接失败
├── 确认 CNI 支持 NetworkPolicy？
│   ├── Flannel → 不支持！需更换 CNI
│   ├── Calico/Cilium → 支持
│   └── 检查 CNI Pod 运行状态
├── 确认策略是否匹配？
│   ├── kubectl get netpol -n <ns> → 查看策略
│   ├── podSelector 是否匹配目标 Pod？
│   └── policyTypes 是否包含对应方向？
├── 确认允许规则是否存在？
│   ├── 默认拒绝后必须有显式允许
│   ├── DNS (UDP 53) 是否允许？
│   └── 端口和协议是否匹配？
└── 使用审计模式调试？
    ├── Cilium: cilium monitor --type drop
    ├── Calico: calico-node -felix-debug
    └── 临时删除策略测试
```

## 生产案例

### 案例1：默认拒绝策略导致 DNS 失败

- **场景**：应用 default-deny-all 策略后所有 Pod 无法解析 DNS
- **排查**：Egress 策略未允许到 kube-dns 的 UDP 53 流量
- **方案**：在所有 Egress 策略中添加 DNS 允许规则；创建全局 DNS 允许策略
- **效果**：DNS 恢复，同时保持其他 Egress 限制

### 案例2：多租户网络隔离

- **场景**：SaaS 平台需要租户间完全网络隔离
- **方案**：每个租户命名空间部署 default-deny + 仅允许 ingress-controller 入站 + 允许 DNS 出站
- **效果**：租户间零通信，满足 SOC2 合规要求

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| K8s NetworkPolicy | 原生、声明式、简单 | 功能有限、无 L7 | 基本 Pod 隔离 |
| Cilium NetworkPolicy | L7 策略、eBPF 高性能 | 需要 Cilium CNI | 高级流量控制 |
| Istio AuthorizationPolicy | L7 细粒度、mTLS | 资源开销大 | 服务网格环境 |
| 云安全组/NSG | 基础设施层、独立于K8s | 粒度粗、非声明式 | 节点级防火墙 |

## 检查清单

- [ ] CNI 插件支持 NetworkPolicy（Calico/Cilium）
- [ ] 每个命名空间有 default-deny 策略
- [ ] DNS Egress 已允许（UDP/TCP 53）
- [ ] 策略使用 namespaceSelector + podSelector 组合
- [ ] 策略变更有审计日志
- [ ] 新策略先在审计模式测试
- [ ] 网络连通性测试已自动化
- [ ] 策略文档化（谁允许访问谁）

## Related

- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/03-网络/service-networking.md|service-networking]] — Service Networking
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[22-概念/03-网络/service-networking.md|Service Networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|CNI Plugins]]
- [[22-概念/05-安全/security-defense-depth.md|Defense-in-Depth Security]]

- [[22-概念/11-交叉分析/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- 22-networkpolicy-reference
- 16-networkpolicy-deep-practice
- [[19-故障诊断/02-资源排障/08-networkpolicy-troubleshooting.md|16-networkpolicy-troubleshooting]]
- [[19-故障诊断/06-FTA故障树/list/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]]
- [[19-故障诊断/04-高级排障/structural-03-networking/04-networkpolicy-troubleshooting.md|04-networkpolicy-troubleshooting]]
- [[26-技能/05-网络/networkpolicy/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->

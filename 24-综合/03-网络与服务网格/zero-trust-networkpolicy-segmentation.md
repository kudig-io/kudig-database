---
title: "Zero Trust × NetworkPolicy × 微分段"
summary: "零信任理念通过 K8s NetworkPolicy 和 Cilium ClusterMesh 落地为网络微分段，将'默认允许'转变为'默认拒绝+显式允许'，构建 Pod 级纵深防御"
category: synthesis
tags:
- zero-trust
- networkpolicy
- microsegmentation
- cilium
- clustermesh
- envoy
- security
tier: supporting
sources:
- 概念/network-policy.md
- 概念/networkpolicy.md
- 概念/service-mesh-zero-trust-security.md
- 实体/cilium.md
- 概念/cilium-ebpf-networking.md
- 概念/multi-cluster-security.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# Zero Trust × NetworkPolicy × 微分段

## The Connection（为什么这两个领域交叉）

传统数据中心安全基于"城堡与护城河"模型——网络边界（防火墙）内是可信区域，边界外是不可信区域。一旦攻击者突破边界（如通过钓鱼邮件获取内网访问），即可在内部自由横向移动。Kubernetes 集群默认继承这一缺陷：Pod 之间全通，任何被攻陷的容器可以扫描整个集群网络、访问任意服务。

零信任（Zero Trust）彻底否定"内部可信"假设：每一次通信都必须经过身份验证和授权，无论通信双方是否在同一网络内。网络微分段（Microsegmentation）是零信任在网络层的实现——将网络划分为最细粒度的段（Pod 级），段间通信默认禁止，仅允许显式声明的白名单流量。

Kubernetes NetworkPolicy 是微分段的原生实现机制：声明式地定义 Pod 间的允许/拒绝规则，由 CNI 插件（Calico/Cilium/Weave）在数据面执行。Cilium 通过 eBPF 在内核态实现高性能策略执行，ClusterMesh 将微分段扩展到多集群，Envoy 集成提供 L7 层策略。三者叠加形成从 L3 到 L7 的完整微分段能力。

## Where They Co-occur（生产中的交叉场景）

### 场景一：默认拒绝 + 白名单开放

新集群或新 namespace 的第一条 NetworkPolicy 是"默认拒绝所有入站和出站流量"。然后逐服务添加白名单规则：frontend 可以访问 backend 的 8080 端口，backend 可以访问 database 的 5432 端口，其他一切通信被禁止。这是微分段的基础模式。

### 场景二：多租户 Namespace 隔离

SaaS 平台中每个租户一个 namespace。NetworkPolicy 确保租户 A 的 Pod 无法访问租户 B 的 Pod（即使在同一集群同一节点）。配合 RBAC（API 层隔离）和 ResourceQuota（资源隔离），形成完整的多租户安全边界。

### 场景三：PCI-DSS 合规的支付环境

PCI-DSS 要求持卡人数据环境（CDE）与网络其他部分隔离。NetworkPolicy 将支付相关 Pod 隔离在独立 namespace，仅允许来自特定 Ingress 的 HTTPS 流量进入，出站仅允许到特定支付网关 IP。所有规则可审计、可版本化。

### 场景四：Cilium L7 策略（无 Sidecar）

Cilium 的 CiliumNetworkPolicy 支持 L7 规则：不仅控制"能不能连"，还控制"连上后能做什么"。例如：允许 frontend 访问 backend 的 `GET /api/*` 但拒绝 `DELETE /api/*`。通过 eBPF + Envoy 实现，无需每 Pod 部署 sidecar。

### 场景五：跨集群微分段（ClusterMesh）

多集群环境中，集群 A 的服务需要访问集群 B 的数据库。Cilium ClusterMesh 将 NetworkPolicy 语义扩展到跨集群：基于服务身份（而非 IP）定义跨集群访问规则，eBPF 在数据面执行，无需额外的服务网格。

### 场景六：DNS 策略控制

微分段不仅是 IP/端口控制。Cilium 支持 DNS 级策略：允许 Pod 解析 `*.internal.svc.cluster.local` 但拒绝解析外部域名（防止数据外泄）。或只允许解析特定白名单域名（如 `api.stripe.com`）。

## Production Patterns（生产模式与架构）

### 模式一：分层微分段架构

```
┌─────────────────────────────────────────────────────────┐
│  Microsegmentation Defense in Depth                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Layer 1: Namespace 隔离 (粗粒度)                      │
│  ├── 每个环境/租户/业务线独立 namespace                 │
│  ├── 默认 deny-all ingress/egress                      │
│  └── 跨 namespace 通信需显式允许                       │
│                                                         │
│  Layer 2: Pod 级 NetworkPolicy (细粒度)                │
│  ├── 基于 label selector 的 Pod 间规则                 │
│  ├── 端口级控制 (只开放必要端口)                       │
│  └── IP Block (外部服务白名单)                         │
│                                                         │
│  Layer 3: Cilium L7 策略 (应用层)                      │
│  ├── HTTP method/path 控制                             │
│  ├── gRPC service/method 控制                          │
│  └── Kafka topic 级控制                                │
│                                                         │
│  Layer 4: DNS 策略 (名称解析层)                        │
│  ├── 限制可解析的域名                                  │
│  └── 防止 DNS 隧道数据外泄                            │
│                                                         │
│  Layer 5: Service Mesh (身份层, 可选)                  │
│  ├── mTLS 双向认证                                     │
│  └── AuthorizationPolicy (基于身份)                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：默认拒绝 + 渐进开放

```yaml
# Step 1: 默认拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}  # 所有 Pod
  policyTypes:
  - Ingress
  # 无 ingress 规则 = 拒绝所有入站

---
# Step 2: 默认拒绝所有出站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  # 无 egress 规则 = 拒绝所有出站

---
# Step 3: 允许 DNS 解析 (必须! 否则服务发现失败)
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
# Step 4: 允许 frontend → backend
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
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

### 模式三：Cilium L7 策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: backend-l7-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: GET
          path: /api/v1/.*
        - method: POST
          path: /api/v1/orders
        # 拒绝其他所有 HTTP 请求 (隐式 deny)
  egress:
  - toEndpoints:
    - matchLabels:
        app: database
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
  - toFQDNs:
    - matchName: "api.stripe.com"
    - matchPattern: "*.s3.amazonaws.com"
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
```

### 模式四：跨集群微分段（ClusterMesh）

```yaml
# 集群 A: 允许访问集群 B 的数据库服务
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: allow-cross-cluster-db
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  egress:
  - toEndpoints:
    - matchLabels:
        app: database
        io.kubernetes.pod.namespace: production
        # ClusterMesh 自动添加集群标识
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
```

### 模式五：策略测试与验证

```bash
# 1. 策略部署前: 使用 Cilium 策略审计模式
kubectl apply -f policy.yaml --dry-run=server

# 2. 连通性测试: 使用 Cilium connectivity check
cilium connectivity test --namespace production

# 3. 策略验证: 从特定 Pod 测试连通性
kubectl exec -it frontend-pod -- curl -s http://backend:8080/api/v1/health
# 预期: 200 OK

kubectl exec -it frontend-pod -- curl -s http://backend:8080/admin
# 预期: 403 Forbidden (L7 策略拒绝)

kubectl exec -it other-pod -- curl -s http://backend:8080/api/v1/health
# 预期: Connection timeout (L4 策略拒绝)

# 4. Hubble 流量可视化
hubble observe --namespace production --verdict DROPPED
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | 原生 NetworkPolicy | CiliumNetworkPolicy | Service Mesh AuthorizationPolicy | 无策略（默认全通） |
|------|-------------------|--------------------|---------------------------------|--------------------|
| 策略粒度 | L3/L4 (IP/端口) | L3/L4/L7 (HTTP/gRPC) | L7 (HTTP/gRPC + 身份) | 无 |
| 执行性能 | 取决于 CNI | eBPF (内核态, 高性能) | Envoy (用户态, +2-5ms) | 无开销 |
| 部署依赖 | CNI 支持 | Cilium CNI | Istio/Linkerd | 无 |
| 身份基础 | Pod IP/Label | Pod Label + 集群标识 | SPIFFE ID (密码学) | 无 |
| DNS 策略 | 不支持 | 支持 (FQDN) | 不支持 | 无 |
| 跨集群 | 不支持 | ClusterMesh | 多集群 Mesh | 无 |
| 可观测性 | 有限 | Hubble (完整) | Envoy Access Log | 无 |
| 学习曲线 | 低 | 中 | 高 | 无 |
| 适用场景 | 基础隔离 | 深度微分段 | 应用层授权 | 开发环境 |

### 决策矩阵

- **基础 Pod 隔离（合规最低要求）** → 原生 NetworkPolicy + Calico
- **深度微分段 + L7 + DNS + 可观测** → Cilium（最全面）
- **已有 Service Mesh** → AuthorizationPolicy + NetworkPolicy 叠加
- **多集群统一策略** → Cilium ClusterMesh 或 Istio 多集群
- **最小侵入（不改 CNI）** → 原生 NetworkPolicy（Calico/Weave 执行）

## Anti-patterns & Pitfalls（反模式）

### 反模式一：只部署默认拒绝不开放白名单

部署 `default-deny-all` 后忘记添加白名单规则，所有服务通信中断。**正确做法**：默认拒绝和白名单规则作为一组部署；先在 Audit 模式（Cilium `policy-audit-mode`）验证影响；准备好快速回滚方案。

### 反模式二：基于 Pod IP 的策略

NetworkPolicy 中硬编码 Pod IP（如 `ipBlock: 10.244.1.5/32`）。Pod 重建后 IP 变化，策略失效。**正确做法**：始终使用 label selector（`podSelector`/`namespaceSelector`）；外部服务用 `ipBlock` + CIDR 范围。

### 反模式三：忽略 Egress 策略

只配置 Ingress 策略（控制谁能访问我），不配置 Egress 策略（控制我能访问谁）。被攻陷的 Pod 可以向任意外部地址外泄数据。**正确做法**：Ingress + Egress 双向控制；Egress 白名单只允许必要的外部访问。

### 反模式四：策略过多导致性能退化

数百条 NetworkPolicy 叠加，CNI 数据面规则膨胀，网络性能下降。iptables 模式下规则匹配是 O(n)。**正确做法**：使用 eBPF（Cilium）避免规则膨胀问题；合并相似策略；定期清理无用策略。

### 反模式五：策略与 Service Mesh 冲突

NetworkPolicy 允许了流量，但 Service Mesh AuthorizationPolicy 拒绝（或反之）。排障时无法判断是哪层拒绝。**正确做法**：明确分层职责（NetworkPolicy 管 L4，Mesh 管 L7）；使用 Hubble/Kiali 统一可视化；文档化策略归属。

### 反模式六：忘记允许 DNS 和监控流量

默认拒绝后忘记允许 DNS（UDP 53）和 Prometheus 抓取（TCP 9090），导致服务发现失败和监控盲区。**正确做法**：默认拒绝模板中始终包含 DNS 允许规则；监控 namespace 的抓取权限单独配置。

## Operational Checklist（运维检查清单）

### 策略部署

- [ ] 新 namespace 首先部署 default-deny（ingress + egress）
- [ ] 立即添加 DNS 允许规则（UDP/TCP 53 → kube-system）
- [ ] 逐服务添加白名单（最小权限原则）
- [ ] 策略文件入 Git（GitOps 管理）
- [ ] 策略变更走 PR + review（安全团队审批）
- [ ] 使用 Cilium audit mode 验证新策略影响

### 运行监控

- [ ] Hubble 流量可视化：DROPPED 流量监控
- [ ] 策略命中率：哪些策略从未命中（可能无用）
- [ ] 连通性测试：定期自动化验证关键路径
- [ ] 告警：意外的 DROPPED 流量突增
- [ ] 策略数量监控：避免规则膨胀

### 定期审计

- [ ] 每月：审查所有 NetworkPolicy（是否有过度宽松）
- [ ] 每月：验证 default-deny 覆盖率（所有 namespace）
- [ ] 每季度：渗透测试（模拟被攻陷 Pod 的横向移动）
- [ ] 每季度：策略清理（删除无用规则）
- [ ] 每年：微分段架构评审（是否适应业务变化）

### 故障排查

- [ ] 连接超时 → 检查 NetworkPolicy（`kubectl describe networkpolicy`）
- [ ] HTTP 403 → 检查 Cilium L7 策略或 Mesh AuthorizationPolicy
- [ ] DNS 解析失败 → 检查 DNS egress 规则
- [ ] 间歇性失败 → 检查策略是否覆盖所有 Pod label（滚动更新时新旧 Pod label 不同）
- [ ] 使用 `hubble observe --verdict DROPPED` 定位拒绝原因

## Related

- [[22-概念/03-网络/network-policy.md|网络策略]]
- [[22-概念/03-网络/network-policy.md|NetworkPolicy]]
- [[22-概念/05-安全/service-mesh-zero-trust-security.md|服务网格零信任安全]]
- [[23-实体/04-网络/cilium.md|Cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF 网络]]
- [[22-概念/05-安全/multi-cluster-security.md|多集群安全]]
- [[24-综合/03-网络与服务网格/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]
- [[24-综合/03-网络与服务网格/service-mesh-mtls-zero-trust.md|Service Mesh × mTLS × Zero Trust]]
- [[24-综合/04-安全与合规/compliance-k8s-soc2-hipaa.md|合规 × K8s × SOC2/HIPAA]]

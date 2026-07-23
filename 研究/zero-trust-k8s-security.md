---
title: K8s 零信任安全架构研究
summary: 深入研究在 Kubernetes 集群中落地零信任安全（Zero Trust）的技术方案，覆盖 mTLS、NetworkPolicy、SPIFFE/SPIRE、OPA、机密管理等多层防御体系。
category: research
tags:
- research
- security
- zero-trust
- mtls
- network-policy
- spiffe
- opa
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 零信任安全架构研究

## 研究背景

传统 Kubernetes 安全模型依赖网络边界（防火墙、安全组）来保护集群内部。这种"城堡护城河"模型在云原生环境中存在根本缺陷：

- **Pod 间无加密**：集群内通信默认明文，被入侵后可嗅探流量
- **网络边界模糊**：Pod IP 动态变化，传统防火墙规则无法跟踪
- **横向移动风险**：一个 Pod 被攻破后，可自由访问同命名空间其他 Pod
- **身份伪造**：ServiceAccount Token 可被窃取和复用

零信任安全的核心原则："从不信任，始终验证"——每个网络连接都需要身份认证和授权。

## 核心问题

1. Kubernetes 原生安全能力（NetworkPolicy、RBAC、Pod Security）在零信任架构中扮演什么角色？
2. mTLS 自动化（Istio/Linkerd/Cilium）的实现差异和性能影响是什么？
3. SPIFFE/SPIRE 如何提供与平台无关的工作负载身份？
4. 零信任架构的分层实施路径和优先级是什么？

## 调研发现

### 发现一：零信任安全分层模型

```
┌─────────────────────────────────────────────────┐
│  Layer 7: API 安全                                │
│  → OPA Gatekeeper（准入控制）                      │
│  → Kyverno（策略即代码）                            │
│  → 审计日志（Falco + SIEM）                        │
├─────────────────────────────────────────────────┤
│  Layer 4: 网络安全                                │
│  → NetworkPolicy（L3/L4 隔离）                    │
│  → Cilium L7 策略（HTTP/gRPC 级别）               │
│  → Egress Gateway（出口控制）                      │
├─────────────────────────────────────────────────┤
│  Layer 3: 身份与加密                               │
│  → mTLS（Istio/Linkerd/Cilium）                   │
│  → SPIFFE/SPIRE（工作负载身份）                     │
│  → External OIDC（统一认证）                       │
├─────────────────────────────────────────────────┤
│  Layer 2: Pod 安全                                │
│  → Pod Security Standards（Restricted 基线）       │
│  → Seccomp + AppArmor + SELinux                  │
│  → Rootless Containers                           │
├─────────────────────────────────────────────────┤
│  Layer 1: 供应链安全                               │
│  → 镜像签名验证（Cosign/Sigstore）                 │
│  → 漏洞扫描（Trivy/Grype）                        │
│  → Admission Controller 阻止未签名镜像              │
└─────────────────────────────────────────────────┘
```

### 发现二：mTLS 方案对比

| 维度 | Istio | Linkerd | Cilium mTLS | 服务网格对比 |
|------|-------|---------|-------------|------------|
| **实现方式** | Sidecar (Envoy) | Sidecar (Rust proxy) | 无 Sidecar (eBPF) | 架构差异最大 |
| **CPU 开销** | ~0.5 CPU/Pod | ~0.1 CPU/Pod | ~0.01 CPU/Pod | Cilium 最优 |
| **内存开销** | ~100MB/Pod | ~20MB/Pod | ~0MB（内核态） | Cilium 最优 |
| **延迟增加** | 2-5ms | 0.5-1ms | <0.5ms | Cilium 最优 |
| **策略粒度** | L3-L7 | L3-L4 | L3-L7 | Istio/Cilium 最细 |
| **证书管理** | 自带 CA / 外部 | 自带 CA | 自带 CA | 均可对接外部 |
| **非 HTTP 支持** | ✅ TCP/gRPC | ⚠️ HTTP 为主 | ✅ TCP/gRPC | Istio/Cilium 更全 |
| **推荐场景** | 全功能 Mesh | 轻量 Mesh | 性能优先 | 按需选择 |

**关键结论**：如果零信任的核心需求是 mTLS + L7 策略，Cilium 的无 sidecar 方案在资源开销和延迟上具有压倒性优势。

### 发现三：SPIFFE/SPIRE 工作负载身份

SPIFFE（Secure Production Identity Framework for Everyone）定义了与平台无关的工作负载身份标准：

```yaml
# SPIFFE ID 格式
spiffe://<trust-domain>/<workload-identifier>

# 示例
spiffe://production.example.com/ns/checkout/sa/payment-service

# 含义:
#   trust-domain: production.example.com
#   namespace: checkout
#   service-account: payment-service
```

**SPIRE 架构**：

```
┌──────────────┐     ┌──────────────┐
│  SPIRE Server │     │   Workload   │
│  (CA + SVID   │←───→│   (Pod)      │
│   注册表)      │     │              │
└──────┬───────┘     └──────┬───────┘
       │                     │
       │ 签发 SVID            │ Workload API
       │ (X.509/JWT)         │ (获取身份凭证)
       ↓                     ↓
┌──────┴───────┐     ┌──────┴───────┐
│ SPIRE Agent  │←───→│  Pod 进程     │
│ (节点级代理)  │ att │              │
└──────────────┘     └──────────────┘
```

**与 K8s 原生身份的关系**：

| 身份来源 | 生命周期 | 可移植性 | 推荐场景 |
|---------|---------|---------|---------|
| K8s ServiceAccount | Pod 生命周期 | 仅 K8s | 纯 K8s 环境 |
| SPIFFE SVID | 工作负载级别 | 跨平台（VM/容器/裸机） | 多集群/混合云 |
| mTLS 内置证书 | Pod 生命周期 | 仅 Mesh 内 | 已有 Service Mesh |

### 发现四：NetworkPolicy 零信任基线

**默认拒绝（Default Deny）是一切零信任的基础**：

```yaml
# 零信任网络策略基线
---
# 1. 默认拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}                # 选择命名空间所有 Pod
  policyTypes:
  - Ingress
  # 不定义 ingress 规则 = 全部拒绝

---
# 2. 默认拒绝所有出站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress

---
# 3. 仅允许 DB Pod 被 App Pod 访问（5432 端口）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-app-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: postgres
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: checkout
    ports:
    - protocol: TCP
      port: 5432
```

**Cilium L7 策略（HTTP 级别零信任）**：

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: checkout-api-l7
spec:
  endpointSelector:
    matchLabels:
      app: payment
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: checkout
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
      rules:
        http:
        - method: "POST"
          path: "/api/v1/charge$"    # 仅允许 POST /api/v1/charge
```

### 发现五：零信任实施成熟度模型

| 级别 | 能力 | 实施项 | 收益 |
|------|------|--------|------|
| **L1 基础** | 边界安全 | RBAC + Pod Security + 镜像扫描 | 防基础攻击 |
| **L2 可见** | 网络分段 | NetworkPolicy 默认拒绝 + 分段策略 | 限制横向移动 |
| **L3 加密** | 传输加密 | mTLS（Istio/Cilium） | 防流量嗅探 |
| **L4 身份** | 工作负载身份 | SPIFFE/SPIRE + 精细策略 | 零信任身份 |
| **L5 持续** | 持续验证 | 实时审计（Falco）+ 自适应策略 | 动态零信任 |

## 结论与建议

1. **零信任是分层递进**：不要试图一步到位，按 L1→L5 逐步实施。
2. **NetworkPolicy 默认拒绝是第一步**：零成本（K8s 原生），但效果显著。
3. **Cilium 是性能最优的零信任数据平面**：无 sidecar mTLS + L7 策略 + eBPF 级可观测。
4. **SPIFFE/SPIRE 用于多集群/混合云场景**：纯 K8s 环境可以用 ServiceAccount + mTLS 替代。
5. **策略即代码（Kyverno/OPA）是强制层**：防止开发者创建不安全的配置。
6. **安全与可观测性不分家**：零信任的所有决策需要基于实时审计数据。

## 参考资料

- NIST Zero Trust Architecture (SP 800-207)
- SPIFFE/SPIRE: https://spiffe.io/
- Cilium Network Policy: https://docs.cilium.io/en/stable/security/policy/
- Istio Security: https://istio.io/latest/docs/concepts/security/
- [[安全/index.md|安全目录]]
- [[网络/index.md|网络目录]]

## Related

- [[综合/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]
- [[概念/network-policy.md|NetworkPolicy 概念]]
- [[研究/ebpf-networking-revolution.md|eBPF 网络革命]]

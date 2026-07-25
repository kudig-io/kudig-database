---
title: Cloud Native Defense in Depth
description: '- [[22-概念/11-交叉分析/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
summary: '- [[22-概念/11-交叉分析/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
category: concepts
tags:
- k8s
- security
- zero-trust
- defense-in-depth
- rbac
- network-policy
- etcd
- istio
- cilium
- calico
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Native Defense in Depth 是什么
- 如何 Cloud Native Defense in Depth
trigger_keywords:
- Cloud
- Native
- Defense
- in
- Depth
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- tls-basics
- policy-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cloud Native Defense in Depth

## Security Layer Model

Cloud native security follows defense-in-depth principles, implementing controls across eight distinct layers:

| Layer | Protection Target | Core Technology | Key Tools |
|-------|-----------------|----------------|-----------|
| 1. Boundary | External attack entry | WAF, DDoS, CDN, TLS | Cloud WAF, cert-manager |
| 2. Identity | User and [[Service|service]] auth | OIDC/SAML, RBAC, IRSA | Keycloak, Okta |
| 3. Network | Micro-segmentation | NetworkPolicy, mTLS, Egress | Calico, Cilium, Istio |
| 4. Workload | Pod security and permissions | PSS, SecurityContext, image verify | Kyverno, OPA |
| 5. Runtime | Real-time threat detection | eBPF, syscall monitoring, anomaly | Falco, Sysdig, Tetragon |
| 6. Supply Chain | Image and dependency integrity | SBOM, vuln scan, signature verify | Trivy, Cosign, Syft |
| 7. Secrets | Credential and certificate lifecycle | Dynamic credentials, PKI | Vault, cert-manager |
| 8. Compliance | Security baseline and continuous compliance | CIS Benchmark, policy-as-code | kube-bench, Kyverno |

## Zero Trust Architecture

Zero trust principles: never trust, always verify; least privilege access; assume breach; explicit verification; micro-segmentation. In K8s, this means:
- All Pod-to-Pod communication requires mTLS (via [[22-概念/03-网络/service-mesh-architecture.md|service mesh]] or Cilium)
- Default-deny NetworkPolicy blocks all traffic unless explicitly allowed
- RBAC enforces least privilege for API access
- Image signature verification ensures only trusted artifacts deploy
- Runtime security monitors for anomalous behavior

## Threat Model and Defenses

| Attack Vector | Defense |
|--------------|---------|
| Supply chain attack | Image signing/verification (Cosign/Sigstore) |
| Container escape | Pod Security Standards (Restricted profile) |
| Lateral movement | NetworkPolicy default-deny + mTLS |
| Secret theft | Vault dynamic credentials + etcd encryption |
| Privilege escalation | RBAC + OPA/Kyverno policies |
| Denial of service | Resource limits + rate limiting |

## Compliance Framework Mapping

Security controls map to regulatory requirements:
- **SOC 2 Type II**: RBAC + mTLS + audit logging + vulnerability management
- **ISO 27001**: Asset classification + access control + encryption + vulnerability management
- **GDPR**: Data protection + privacy by design + encryption + access control
- **PCI-DSS v4.0**: Secure development + strong authentication + SBOM + signing
- **NIST CSF**: Risk assessment + access control + continuous monitoring

## Key Security Baselines

- **CIS Kubernetes Benchmark**: Automated security configuration baseline checked by kube-bench
- **Pod Security Standards**: Three pre-defined levels (Privileged, Baseline, Restricted) enforced via namespace labels
- **Minimum Pod Security**: runAsNonRoot=true, readOnlyRootFilesystem=true, capabilities drop ALL, allowPrivilegeEscalation=false, seccompProfile=RuntimeDefault

## 源码实现分析

### 纵深防御层次架构

```
┌─────────────────────────────────────────────────┐
│  Layer 1: 供应链安全                          │
│  ├── 镜像签名验证 (Cosign/Sigstore)          │
│  ├── SBOM 生成与扫描 (Syft + Trivy)         │
│  └── 准入策略 (Kyverno verifyImages)         │
├─────────────────────────────────────────────────┤
│  Layer 2: 运行时安全                          │
│  ├── Pod Security Standards (Restricted)      │
│  ├── seccomp + AppArmor/SELinux              │
│  └── 只读文件系统 + drop ALL capabilities    │
├─────────────────────────────────────────────────┤
│  Layer 3: 网络安全                            │
│  ├── NetworkPolicy default-deny              │
│  ├── mTLS (Istio/Linkerd)                    │
│  └── 服务间授权 (AuthorizationPolicy)        │
├─────────────────────────────────────────────────┤
│  Layer 4: 数据安全                            │
│  ├── Secret 加密 (etcd encryption at rest)    │
│  ├── 动态凭证 (Vault)                        │
│  └── 外部 Secret 管理 (ESO)                  │
├─────────────────────────────────────────────────┤
│  Layer 5: 检测与响应                          │
│  ├── 运行时威胁检测 (Falco)                  │
│  ├── 审计日志 (K8s Audit + SIEM)             │
│  └── 合规扫描 (kube-bench + Trivy)           │
└─────────────────────────────────────────────────┘
```

### Kyverno 镜像验证策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  rules:
  - name: verify-cosign-signature
    match:
      resources:
        kinds: ["Pod"]
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - entries:
        - keys:
            publicKeys: |-
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
```

## 使用场景

### 场景一：安全基线检查

```bash
# 🟢 低风险 - CIS Benchmark 扫描
kubectl apply -f kube-bench-job.yaml
kubectl logs job/kube-bench

# 🟢 低风险 - 检查 Pod 安全配置
kubectl get pods -A -o json | jq '.items[] | select(.spec.securityContext.runAsNonRoot != true) | .metadata.name'

# 🟢 低风险 - 检查特权容器
kubectl get pods -A -o json | jq '.items[].spec.containers[] | select(.securityContext.privileged == true)'
```

### 场景二：网络隔离实施

```yaml
# 默认拒绝所有 + 放行 DNS + 允许特定服务
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
  - ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 有防火墙就安全 | 防火墙只是网络层，需多层防御（供应链+运行时+网络+数据+检测） |
| 内网不需要加密 | 零信任原则：内网也需 mTLS，防止横向移动 |
| Secret 在 K8s 中是加密的 | 默认 base64 编码（非加密），必须启用 etcd encryption at rest |
| 镜像扫描一次就够 | 需持续扫描（新漏洞不断发现），CI + 运行时 + 定期重扫 |
| RBAC 配置好就安全 | RBAC 只是授权层，还需 PSA、NetworkPolicy、审计日志等 |
| 安全是安全团队的事 | DevSecOps：安全左移，开发/运维/安全共同负责 |

## 面试要点

1. **云原生纵深防御的层次？** — 供应链（镜像签名+SBOM+扫描）→ 运行时（PSA+seccomp+MAC）→ 网络（NetworkPolicy+mTLS）→ 数据（加密+动态凭证）→ 检测（Falco+审计）。每层独立生效，层层递进。

2. **零信任网络如何实现？** — 永不信任、始终验证。NetworkPolicy default-deny（L3/L4）+ Service Mesh mTLS（L7）+ AuthorizationPolicy（细粒度授权）。每次通信都经过身份验证和授权。

3. **供应链安全的关键环节？** — 可重现构建（SLSA provenance）→ SBOM 生成（Syft）→ 漏洞扫描（Trivy）→ 镜像签名（Cosign）→ 准入验证（Kyverno verifyImages）→ 运行时监控（Falco）。

4. **生产环境安全合规检查清单？** — CIS Benchmark（kube-bench）；PSA Restricted；etcd 加密；审计日志启用；镜像签名验证；NetworkPolicy default-deny；定期漏洞扫描；Secret 轮换；RBAC 最小权限审计。

## Related

- [[23-实体/06-安全/trivy.md|trivy]] — Trivy
- [[23-实体/06-安全/vault.md|vault]] — HashiCorp Vault
- [[cert-manager]] — cert-manager
- [[22-概念/05-安全/secrets-management.md|secrets-management]] — Secrets Management
- [[22-概念/05-安全/linux-security-modules.md|linux-security-modules]] — Linux Security Modules for Containers
- [[22-概念/05-安全/linux-security-modules.md|Linux Security Modules]]
- [[22-概念/03-网络/service-mesh-architecture.md|Service Mesh Architecture]]
- [[supply-chain-security|Supply Chain Security]]
- [[22-概念/05-安全/secrets-management.md|Secrets Management]]
- [[falco|Falco]]
- [[kyverno|Kyverno]]
- [[23-实体/06-安全/vault.md|HashiCorp Vault]]

- [[22-概念/11-交叉分析/Deployment × Secret 管理.md|Deployment × Secret 管理]]

<!-- risk-assessed -->

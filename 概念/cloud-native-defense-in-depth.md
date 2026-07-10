---
title: Cloud Native Defense in Depth
description: '- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
summary: '- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
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
- All Pod-to-Pod communication requires mTLS (via [[概念/service-mesh-architecture.md|service mesh]] or Cilium)
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

## Related

- [[实体/trivy.md|trivy]] — Trivy
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[cert-manager]] — cert-manager
- [[概念/secrets-management.md|secrets-management]] — Secrets Management
- [[概念/linux-security-modules.md|linux-security-modules]] — Linux Security Modules for Containers
- [[概念/linux-security-modules.md|Linux Security Modules]]
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[supply-chain-security|Supply Chain Security]]
- [[概念/secrets-management.md|Secrets Management]]
- [[falco|Falco]]
- [[kyverno|Kyverno]]
- [[实体/vault.md|HashiCorp Vault]]

- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]

<!-- risk-assessed -->

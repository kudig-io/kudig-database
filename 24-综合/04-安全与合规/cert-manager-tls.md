---
title: cert-manager × TLS
summary: cert-manager 与 TLS 的交叉：如何以 CRD 化方式自动管理 Kubernetes 内证书签发、轮换与续期全生命周期。
category: synthesis
tags:
- cert-manager
- tls
- certificates
- pki
- automation
tier: supporting
sources:
- 实体/cert-manager.md
- 概念/kubernetes-pki-certificate-system.md
- 概念/service-mesh-zero-trust-security.md
- 概念/ingress.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.75
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# cert-manager × TLS

## The Connection

TLS 保障传输的机密性与身份可信，但其工程难点不在密码学，而在证书的签发、分发、轮换与续期——一张过期证书足以让整个集群入口宕机。cert-manager 把证书建模为 Kubernetes CRD（`Certificate`/`Issuer`/`CertificateRequest`），由控制器自动完成申请、签发、Secret 写入与到期前续期，将 TLS 从"运维手动跑流程"升级为"声明式自动生命周期"。在技术实现上，cert-manager 的控制循环持续 watch `Certificate` 资源的 `status.notAfter` 字段，在到期前触发 `CertificateRequest` → `Issuer`/`ClusterIssuer` 签发流程，新证书写入 Kubernetes Secret 后由 Ingress Controller 或 Service Mesh sidecar 热加载。这种将信任链转化为 Kubernetes reconcile loop 的设计，使得证书轮换从"运维事件"变成"持续协调"，是云原生 TLS 管理的范式跃迁。^[inferred]

## Where They Co-occur

- **Ingress TLS 自动化**：Ingress 注解 `cert-manager.io/issuer` 触发 cert-manager 自动签发证书并写入 TLS Secret，实现 HTTPS 一键开启，无需手动生成 CSR 或上传证书。
- **Let's Encrypt / ACME**：`Issuer` 类型为 `ACME` 时，cert-manager 自动完成 HTTP-01/DNS-01 挑战（通过 `Order`/`Challenge` CRD 编排），签发公网可信证书；DNS-01 支持通配符证书且无需暴露 HTTP 端口。
- **内部 PKI**：`Issuer` 类型为 `CA`/`SelfSigned`/`Vault`，为集群内部 mTLS 签发私有 CA 链证书，CA 根证书本身也以 Secret 或 Vault 路径管理。
- **服务网格 mTLS**：Istio/Cilium 的 mTLS 身份（含 SPIFFE）与 cert-manager 签发的 CA 协同，构成零信任传输底座；Istio 的 `ca` 组件可配置为使用 cert-manager 签发的中间 CA。
- **Kubernetes API Server 证书**：控制面 PKI（apiserver、etcd、kubelet）可由 cert-manager 托管轮换，避免集群证书过期导致整个控制面不可用的灾难场景。
- **External Secrets / Vault 集成**：cert-manager 与 Vault PKI 引擎对接，统一企业级密钥与证书治理，实现"一个 Vault 实例管全集群证书"的集中化模型。
- **Certificate 的 DNSName 覆盖**：单个 `Certificate` 资源可声明多个 SAN（Subject Alternative Name），支持一个 Secret 服务多个域名——生产中常为一个 Ingress 配置 `example.com` + `*.example.com` 组合。
- **Gateway API 集成**：Gateway API 的 `Gateway` 资源可引用 cert-manager Issuer 自动签发监听器证书，替代 Ingress 注解模式，提供更声明式的 TLS 配置。
- **Certificate 私钥轮换**：`Certificate` CRD 的 `privateKey.rotationPolicy` 支持 `Always`（每次续期生成新私钥）或 `Never`（复用旧私钥），安全性要求高的场景应选 `Always` 但可能导致依赖旧密钥的客户端中断。
- **CertificateRequest 状态可观测**：每个 `Certificate` 的 `status.conditions` 暴露 `Ready`、`Issuing` 等条件，配合 Prometheus 告警规则可监控签发失败和即将到期的证书。
- **ACME Rate Limit 处理**：Let's Encrypt 对同一域名有签发频率限制（如每域名每周 50 张证书），生产环境应使用 Staging 环境调试 ACME challenge，避免触发 rate limit 导致生产证书无法续期。
- **Cilium mTLS 集成**：Cilium 1.14+ 支持从 cert-manager 管理的 CA 自动派发 SPIFFE 身份证书，在内核态 eBPF 层实现 mTLS，无需 sidecar 注入——cert-manager 成为 sidecarless mesh 的 PKI 底座。
- **Certificate Transparancy 审计**：ACME 签发的公网证书会被记录到 CT log 中，可用于检测未授权的证书签发——结合 cert-manager 审计日志可实现"谁在何时为哪个域名签发了证书"的完整追溯。
- **Step CA / Smallstep 集成**：对于需要更细粒度 PKI 控制的场景（如短期 SSH 证书、per-device 证书），cert-manager 可与 step-ca 集成，后者作为 ACME 兼容的私有 CA 提供短 TTL 证书签发。

## Cross-cutting Insight

TLS 的安全价值依赖"证书始终有效"，而非"证书曾签发"。cert-manager 把证书变成一个持续 reconcile 的对象——到期前自动续期、签发失败自动重试、状态可观测可告警。这种"把信任工业化"的能力，是零信任与服务网格得以大规模落地的前提：没有自动化轮换，mTLS 反而会成为可靠性负债——想象一个千节点集群中数百个 sidecar 同时因证书过期而拒绝连接的场景。更深层地看，cert-manager 将 PKI 运维从"人类的记忆力和日历提醒"转变为"控制器的声明式协调循环"，消除了证书管理中最危险的人因风险：遗忘续期。当 `Certificate` 资源的 `status.conditions` 中 `Ready` 变为 `False` 时，告警系统应立即响应——因为这意味着续期链路中的某个环节（ACME 挑战失败、CA 不可达、RBAC 权限丢失）已阻塞了自动轮换。^[inferred]

## Tensions and Trade-offs

| 维度 | 手动证书管理 | cert-manager 自动化 | 结合注意事项 |
|---|---|---|---|
| 续期 | 人工跟踪到期，易遗漏 | 到期前自动轮换 | 需告警续期失败 |
| 签发来源 | CA/供应商手动出证 | ACME/CA/Vault 自动 | 公网用 ACME，内部用私有 CA |
| Secret 分发 | 手动 cp 到 namespace | 控制器写入目标 Secret | 需配合 RBAC 限访问 |
| 故障域 | 单点人工 | 控制器宕=全部停签 | Issuer/控制面需高可用 |
| 信任链 | 显式可控 | 自动但链路长 | 需审计签发日志 |
| 多集群 | 每集群独立管理 | cert-manager 可共享 CA | 跨集群私有 CA 分发需安全通道 |

## Open Questions

- 当 cert-manager 控制面自身不可用时，如何保证已签发证书仍可用、仅续期受阻？是否需要部署多副本 + leader election？
- 在多集群零信任中，cert-manager 签发的私有 CA 与 SPIFFE/SPIRE 的身份模型如何统一？谁 owns 根 CA？
- 证书轮换与 GitOps 同步冲突时（Secret 由谁 owns），应如何划分职责——cert-manager 管 Secret 生命周期，GitOps 管 Certificate 声明？
- ACME DNS-01 challenge 在多 DNS 揢商环境下如何自动化选择 provider？批量签发时是否触发 Let's Encrypt rate limit？

## Related

- [[23-实体/06-安全/cert-manager.md|cert-manager]]
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|K8s PKI 证书体系]]
- [[23-实体/06-安全/vault.md|Vault]]
- [[23-实体/06-安全/external-secrets.md|External Secrets]]
- [[23-实体/06-安全/spiffe.md|SPIFFE]]
- [[23-实体/06-安全/spire.md|SPIRE]]
- [[22-概念/05-安全/service-mesh-zero-trust-security.md|服务网格零信任]]
- [[22-概念/03-网络/ingress.md|Ingress]]
- [[24-综合/03-网络与服务网格/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]


<!-- risk-assessed -->

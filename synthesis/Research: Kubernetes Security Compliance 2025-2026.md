---
category: synthesis
tags:
  - security
  - compliance
  - k8s
  - research
created: 2026-05-24
updated: 2026-05-24
---

# Research: Kubernetes Security Compliance 2025-2026

## 概述

2025-2026年间，Kubernetes安全领域经历了从"策略强制"到"持续验证"的范式转移。供应链安全方面，Sigstore生态完全成熟，SLSA Level 3成为CI/CD流水线基线要求；运行时安全领域，eBPF观测技术取代传统系统调用审计，Cilium Tetragon和Falco eBPF引擎成为事实标准；策略引擎方面，CEL（Common Expression Language）原生策略取代OPA/Rego，Kubernetes Admission中的ValidatingAdmissionPolicy GA将策略执行直接嵌入API Server。这一时期的安全叙事是：**安全不再是外围层，而是Kubernetes数据平面的原生属性**。

## 核心发现

1. **供应链安全成熟化（SLSA L3 + Sigstore）**：Sigstore（cosign/rekor/fulcio）成为容器镜像签名标准基础设施，所有主流CI平台（GitHub Actions、GitLab CI、Tekton）原生集成。SLSA Level 3的构建来源验证从"最佳实践"升级为监管要求（FedRAMP、SOC2 2026修订版），不可变构建成为强制基线。

2. **eBPF运行时安全革命**：Cilium Tetragon成为K8S运行时安全监控的核心引擎，以极低开销（<3% CPU）实现内核级可观测性。传统Falco全面迁移至eBPF引擎，Sysdig逐步退出K8S市场聚焦云安全。eBPF-based的网络策略（Cilium NetworkPolicy）成为Calico的有力替代。

3. **CEL原生策略取代OPA/Rego**：Kubernetes 1.30+中ValidatingAdmissionPolicy和MutatingAdmissionPolicy全面GA，CEL成为策略表达的原生语言。Kyverno 2.0采用CEL作为后端引擎，Gatekeeper/OPA退居遗留场景。CEL的"嵌入式零依赖"特性消除了策略引擎的外部依赖风险。

4. **零信任网络成为默认假设**：mTLS（Istio/Linkerd/Cilium）从"可选增强"升级为生产集群基线要求。Cilium的Mutual Authentication和SPIFFE/SPIRE集成使服务身份验证与网络策略解耦，实现了真正的零信任微分段。

5. **Secret管理标准化（External Secrets + CSI）**：External Secrets Operator成为K8S与外部密钥管理（AWS Secrets Manager、HashiCorp Vault、Azure Key Vault）集成的标准方式。Secrets Store CSI Driver GA使卷级密钥注入成为标准模式，消除了Secrets以明文存储在etcd中的风险。

6. **合规即代码（Compliance-as-Code）**：Kubernetes CIS Benchmark自动化工具（kube-bench、Polaris）与持续合规平台（Armo/Kubescape、Aqua）深度集成，合规状态从"点检"转变为"流式验证"。NIST SP 800-204系列和EU CRA（Cyber Resilience Act）对K8S安全控制提出了具体映射要求。

## 核心概念

相关核心概念详见 [[concepts/k8s-security-compliance]]，涵盖：

- [[concepts/supply-chain-security]] — Sigstore、SLSA与镜像签名验证
- [[concepts/ebpf-runtime-security]] — eBPF运行时监控与Tetragon
- [[concepts/cel-native-policies]] — CEL策略引擎与Admission Policy GA
- [[concepts/zero-trust-mtls]] — mTLS、SPIFFE与零信任微分段
- [[concepts/secret-management]] — External Secrets Operator与CSI Driver
- [[concepts/compliance-as-code]] — 持续合规验证与CIS/NIST映射

## 矛盾与争议

| 议题 | 立场A | 立场B |
|------|-------|-------|
| CEL vs. OPA/Rego | CEL是K8S原生，应全面迁移 | OPA在跨平台策略（跨K8S/云/OS）仍有不可替代性 |
| eBPF安全性 | eBPF验证器保证内核安全 | eBPF程序本身可被恶意利用，需额外的BPF LSM限制 |
| Cilium vs. Calico | Cilium性能和功能全面超越Calico | Calico在BGP集成和传统网络环境仍更成熟 |
| Sigstore信任根 | Sigstore的CT日志和透明度模型足够可信 | Fulcio CA的密钥轮换策略存在可用性风险 |

## 来源

- Kubernetes SIG-Auth ValidatingAdmissionPolicy KEP & GA Release Notes
- Cilium Tetragon v1.2+ Documentation & Performance Benchmarks
- Sigstore Project Annual Report 2025 & SLSA Specification v1.0
- Kyverno 2.0 Architecture Decision Records & CEL Migration Guide
- NIST SP 800-204C: Security Strategies for Microservices-Based Application Systems
- CNCF Cloud Native Security Whitepaper v2 (2025)
- External Secrets Operator v0.12+ & Secrets Store CSI Driver GA Release
- EU Cyber Resilience Act (CRA) Technical Standards for Container Orchestration

---

## 跨域关联

- [[concepts/k8s-networking-evolution]] — 网络安全（Cilium NetworkPolicy、eBPF 可观测）是零信任架构的网络层核心
- [[concepts/container-runtime-evolution]] — 安全容器运行时（gVisor、Kata Containers、Confidential Containers）提供工作负载级隔离
- [[concepts/gitops-production-operations]] — GitOps 审计追踪与声明式配置管理是安全合规（SOC2、PCI-DSS）的技术支撑
- [[concepts/k8s-observability-stack]] — 安全可观测性（审计日志、运行时威胁检测）是安全运营中心（SOC）的数据基础

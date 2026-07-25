---
title: Domain-19 论文与参考 — 开源项目索引
description: 本域为学术与最佳实践文档，主要关联的开源项目分布在其他 Domain。
summary: 本域为学术与最佳实践文档，主要关联的开源项目分布在其他 Domain。
category: papers
tags:
- k8s
- papers
- research
- etcd
- prometheus
- istio
- cilium
- falco
- ebpf
- rag
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
- Domain-19 论文与参考 — 开源项目索引 是什么
- 如何 Domain-19 论文与参考 — 开源项目索引
- Kubernetes 19 papers 最佳实践
trigger_keywords:
- Domain-19
- 论文与参考
- 开源项目索引
- papers
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- observability-basics
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




# Domain-19 论文与参考 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心关联项目

> 本域为学术与最佳实践文档，主要关联的开源项目分布在其他 Domain。

| 项目 | 关联论文/白皮书 | 来源 |
|:---|:---|:---|
| **Kubernetes** | [Borg, Omega, and Kubernetes](https://research.google/pubs/pub43438/) | Google Research |
| **etcd** | Raft Consensus Algorithm | Diego Ongaro, Stanford |
| **Prometheus** | [Prometheus: A Next-Generation Monitoring System](https://soundcloud.com/promcon/2016-berlin-julius-volz-prometheus-a-next-generation-monitoring-system) | SoundCloud / CNCF |
| **OpenTelemetry** | [Unified Observability](https://opentelemetry.io/docs/concepts/observability-primer/) | CNCF |
| **Cilium** | [eBPF-based Networking and Security](https://cilium.io/blog/) | Isovalent |
| **Istio** | [Istio: A Service Mesh Architecture](https://istio.io/latest/about/case-studies/) | Google/IBM/Lyft |
| **CNI** | [Container Network Interface Specification](https://github.com/containernetworking/cni/blob/main/SPEC.md) | CNCF |
| **OCI** | [Open Container Initiative Specs](https://github.com/opencontainers) | Linux Foundation |
| **Falco** | [Runtime Security with eBPF](https://falco.org/docs/) | Sysdig / CNCF |
| **SPIFFE/SPIRE** | [SPIFFE Identity Framework](https://spiffe.io/docs/latest/spiffe-about/overview/) | CNCF |
| **K8s Security** | [CNCF Cloud Native Security Whitepaper](https://github.com/cncf/tag-security/blob/main/security-whitepaper/) | CNCF TAG Security |
| **K8s Storage** | [CNCF Storage Whitepaper](https://github.com/cncf/tag-storage/blob/main/storage-whitepaper.md) | CNCF TAG Storage |
| **K8s Networking** | [CNCF Service Mesh Whitepaper](https://github.com/cncf/tag-network/blob/main/service-mesh-whitepaper.md) | CNCF TAG Network |
| **GitOps** | [CNCF GitOps Whitepaper](https://github.com/cncf/tag-app-delivery/blob/main/gitops-whitepaper/) | CNCF TAG App Delivery |
| **Platform Engineering** | [Platform Engineering Maturity Model](https://github.com/cncf/tag-app-delivery/blob/main/platforms-maturity-model/) | CNCF TAG App Delivery |

---

## 参考链接

- [CNCF 官方出版物](https://www.cncf.io/reports/)
- [USENIX ATC / SOSP / OSDI 论文](https://www.usenix.org/conferences)
- [arXiv 分布式系统论文](https://arxiv.org/list/dc/recent)

---

## Obsidian 相关文档

- domain-19-papers MOC
- [[21-生态参考/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers...]]
- Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framew...
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Op...
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Imp...
- Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Archit...
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- Kubernetes 成本治理与 FinOps 实践 (Kubernetes Cost Governance and F...
- Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface ...
- Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro...
- Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and ...
- Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)

## Related

- [[papers|#papers Hub]] — tag hub

- research/ — tag hub


<!-- risk-assessed -->

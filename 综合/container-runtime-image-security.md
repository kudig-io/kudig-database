---
title: 容器运行时 × 镜像安全
summary: 容器运行时与镜像安全的交叉：从镜像构建、分发到执行的端到端供应链防护。
category: synthesis
tags:
- container-runtime
- containerd
- image-security
- supply-chain
- security
tier: supporting
sources:
- 容器运行时/03-containerd-cri-o/01-containerd-production-operations.md
- 容器运行时/03-containerd-cri-o/02-cri-o-production-guide.md
- 容器运行时/04-image-build/01-buildkit-production-guide.md
- 安全/05-supply-chain/README.md
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
provenance:
  extracted: 0.25
  inferred: 0.65
  ambiguous: 0.1
base_confidence: 0.74
lifecycle: draft
lifecycle_changed: '2026-06-26'
---


# 容器运行时 × 镜像安全

## The Connection

容器运行时负责执行镜像，镜像是运行时的输入。如果镜像在构建或分发阶段被篡改，运行时的一切隔离机制都建立在不可信基础之上。反之，运行时可以通过策略阻止可疑镜像执行、限制容器能力。^[inferred]

## Where They Co-occur

- BuildKit 构建镜像时可集成 Trivy/Grype 进行扫描
- containerd/CRI-O 支持镜像签名验证（Sigstore/cosign、Notary）
- 阿里云/专有云 ACR 提供镜像安全扫描和加签能力
- admission controller（Kyverno/OPA）在 Pod 创建时检查镜像来源和漏洞基线
- 运行时安全工具（Falco/Tetragon）检测容器内异常行为，作为镜像被攻破后的第二层防御

## Cross-cutting Insight

镜像安全解决"运行什么"的问题，运行时安全解决"如何运行"的问题。只有将两者贯通——构建时扫描、分发时签名、运行时验证、执行时监控——才能形成完整的容器供应链防护。^[inferred]

## Tensions and Trade-offs

| 维度 | 镜像安全侧重 | 运行时安全侧重 | 结合注意事项 |
|---|---|---|---|
| 防护阶段 | build/push | run | 需覆盖完整生命周期 |
| 误报处理 | 扫描规则过严导致发布阻塞 | 行为检测过敏导致告警风暴 | 需分级策略 |
| 性能影响 | 扫描延长 CI 时间 | 监控增加运行时开销 | 关键路径应异步化 |
| 回滚 | 禁止问题镜像 | 隔离异常容器 | 需联动事件响应 |

## Open Questions

- 在专有云环境中，如何统一管理 ACR 镜像签名与集群级验证策略？
- SBOM 应该存储在镜像仓库还是独立的安全资产管理平台？
- 当运行时检测到容器逃逸迹象时，是否应自动触发镜像重新扫描？

## Related

- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|01 containerd production operations]]
- [[容器运行时/containerd-CRI-O/02-cri-o-production-guide.md|02 cri o production guide]]
- [[容器运行时/镜像构建/01-buildkit-production-guide.md|01 buildkit production guide]]
- [[生态参考/领域索引/README.md|README]]
- [[生产运维/事件响应/02-container-runtime-threat-response.md|02 container runtime threat response]]

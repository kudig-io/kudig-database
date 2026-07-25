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
last_updated: 2026-07-11
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

容器运行时（containerd/CRI-O）负责拉取、解压并执行镜像，镜像是运行时的唯一输入。如果镜像在构建或分发阶段被植入恶意代码、引入已知漏洞或被篡改，运行时的一切 namespace/cgroup/seccomp 隔离机制都建立在不可信基础之上——攻击者可能利用镜像内的特权配置或 setuid 二进制直接突破隔离。反之，运行时可以通过策略（admission controller 拦截、运行时安全监控、seccomp/AppArmor profile）阻止可疑镜像执行、限制容器能力，形成镜像被攻破后的第二道防线。从供应链安全视角看，镜像→运行时是一条"信任传递链"：构建时可信（SBOM + 扫描）→ 分发时可信（签名 + 验证）→ 运行时可信（策略准入 + 行为监控）。任何一个环节断裂（如 CI 扫描通过但镜像在 registry 被替换、或签名验证通过但 base image 在运行期间爆出 0day），都会使整条信任链失效。因此容器供应链安全不能只靠"把镜像扫干净"，而必须在每个阶段设置独立验证点——这正是 SLSA（Supply-chain Levels for Software Artifacts）框架的核心思想。^[inferred]

## Where They Co-occur

- BuildKit 构建镜像时可集成 Trivy/Grype 进行分层漏洞扫描（OS 包 + 语言依赖），并通过 `--provenance` 生成 SLSA 来源证明
- containerd/CRI-O 支持镜像签名验证（Sigstore/cosign keyless 签名、Notary v2），在 `kubelet --image-pull-policy` 触发拉取时由运行时校验签名后才解压
- 阿里云/专有云 ACR 提供镜像安全扫描和加签能力，企业版 ACR 可在 push 时自动阻断 CRITICAL 漏洞镜像入库
- admission controller（Kyverno/OPA Gatekeeper）在 Pod 创建时检查镜像来源 registry 白名单、签名状态和漏洞基线，拒绝不符合策略的 Pod 调度
- 运行时安全工具（Falco/Tetragon）检测容器内异常行为（如反弹 shell、/proc 探测、异常网络外连），作为镜像被攻破后的运行时防御层
- **SBOM 传播链**：构建时生成的 SBOM（CycloneDX/SPDX）随镜像一起推送到 OCI registry 的 manifest 中，运行时可据此验证依赖完整性
- **镜像精简与攻击面**：distroless/scratch 基础镜像从源头减少攻击面（无 shell、无包管理器），与运行时 seccomp-strict profile 配合可实现最小权限
- **快照与镜像层加密**：containerd 支持 encrypted image（OCICrypt），拉取时需节点持有解密密钥，防止镜像在节点磁盘上被离线窃取
- **Admission 阶段阻断**：Kyverno `verifyImages` 策略在 Pod admission 阶段验证镜像签名和漏洞基线，不符合策略的镜像在 `kubectl apply` 时即被拒绝，不会进入调度流程
- **运行时持续扫描**：Trivy Operator 以 CronJob 方式定期重新扫描已运行 Pod 的镜像 SBOM 匹配最新 CVE 数据库，弥补"准入时安全但运行后不安全"的时间窗口漏洞
- **镜像垃圾回收**：containerd/kubelet 的 image GC 策略（`--image-gc-high-threshold`）需与镜像刷新策略配合，避免旧镜像残留占满节点磁盘导致新 Pod 拉取失败
- **seccomp/AppArmor RuntimeProfile**：containerd 的 seccomp profile（`RuntimeDefault`/`Unconfined`/`Localhost`）限制容器可调用的 syscall 集合，作为镜像被攻破后的第三道防线——即使攻击者拿到 shell 也无法调用 `keyctl`、`bpf` 等高危 syscall
- **AdmissionController 链顺序**：`ValidatingAdmissionPolicy`（K8s 1.30+）可在 Kyverno/OPA 之前做快速 CEL 表达式校验（如镜像 registry 白名单），减少 Gatekeeper webhook 的调用频率和延迟
- **镜像 wasm + containerd shim**：containerd 的 runtime shim 架构支持 WASM 工作负载（如 `wasmtime-shim`），WASM 沙箱从架构层面消除了容器逃逸风险——无法调用 Linux syscall，攻击面极小
- **Kube Bench / kube-hunter**：CIS Kubernetes Benchmark 自动化扫描工具（kube-bench）和攻击模拟工具（kube-hunter）定期评估集群安全态势，发现镜像配置漂移和暴露的攻击面
- **镜像拉取策略**：`imagePullPolicy: Always`（默认 For tag `:latest`）确保每次 Pod 创建时拉取最新镜像，但增加延迟和 registry 压力；`IfNotPresent` 减少延迟但可能导致节点上的旧镜像被复用——安全敏感场景应强制 `Always` 或使用 digest pinning（`image@sha256:...`）
- **ReadOnlyRootFilesystem**：Pod `securityContext.readOnlyRootFilesystem: true` 使容器根文件系统只读，攻击者即使突破容器也无法写入 webshell 或修改系统配置——与 distroless 镜像配合构成最小攻击面
- **ImagePolicyWebhook**：自定义 admission webhook 可对接企业安全平台（如 Anchore、Twistlock），在 Pod 创建时做实时镜像合规检查和漏洞基线评估
- **RuntimeClass 隔离**：K8s RuntimeClass（如 `kata-containers`、`gvisor`）为高安全 Pod 提供沙箱运行时——容器进程运行在独立 VM 或 syscall filter 沙箱中，即使镜像含 0day exploit 也无法逃逸到宿主节点

## Cross-cutting Insight

镜像安全解决"运行什么"的问题——确保进入集群的镜像可信、无已知漏洞；运行时安全解决"如何运行"的问题——即使镜像有问题，也能限制爆炸半径。只有将两者贯通——构建时扫描、分发时签名、运行时验证、执行时监控——才能形成完整的容器供应链防护（SLSA Level 3+）。实践中最危险的盲区是"镜像通过了 CI 扫描，但运行时依赖的 base image 在上线后爆出 0day"——这要求运行时安全工具具备基于 SBOM 的实时漏洞匹配能力，而非仅在 build/push 时做一次性检查。更深层地看，容器供应链安全面临"时间维度"的独特挑战：镜像是不可变制品，但漏洞数据库是持续更新的——一个安全的镜像在发布一周后可能因新 CVE 而变得不安全。这意味着"准入时检查"不够，还需要"运行中持续扫描"——对已在集群中运行的镜像定期重新扫描 SBOM 匹配最新 CVE，对受影响 Pod 发出告警或触发滚动更新。此外，镜像瘦身（distroless/scratch）虽从源头减少了攻击面，但也带来了新的运维挑战：无 shell 意味着无法 `kubectl exec` 进入容器排障，无包管理器意味着无法现场安装诊断工具——安全与可运维性之间存在内在张力。^[inferred]

## Tensions and Trade-offs

| 维度 | 镜像安全侧重 | 运行时安全侧重 | 结合注意事项 |
|---|---|---|---|
| 防护阶段 | build/push（预防性） | run（检测性） | 需覆盖完整生命周期 |
| 误报处理 | 扫描规则过严导致发布阻塞 | 行为检测过敏导致告警风暴 | 需分级策略（CRITICAL block vs WARN） |
| 性能影响 | 扫描延长 CI 时间 | 监控增加运行时开销 | 关键路径应异步化 |
| 回滚 | 禁止问题镜像 | 隔离异常容器 | 需联动事件响应（自动 cordon + 重新扫描） |
| 签名链信任 | cosign/Notary 验签 | 运行时是否信任签名 CA | 签名密钥轮换需与集群策略同步 |
| 基础镜像更新 | 定期 rebuild 跟进 CVE patch | 运行时容器不会自动更新 | 需制定镜像滚动刷新策略（如 KubeLinter + Renovate） |
| 镜像大小 | 安全工具增加层数和大小 | 精简镜像减少攻击面 | distroless vs 可调试性的权衡 |

## Open Questions

- 在专有云环境中，如何统一管理 ACR 镜像签名与集群级验证策略（cosign + Kyverno verifyImages）？签名密钥如何安全分发到多集群？
- SBOM 应该存储在镜像仓库（OCI manifest）还是独立的安全资产管理平台？两者如何同步？
- 当运行时检测到容器逃逸迹象时，是否应自动触发镜像重新扫描并通知所有运行该镜像的节点？隔离策略是 cordon 节点还是 kill Pod？
- 镜像层加密（OCICrypt）与节点密钥管理的集成，在大规模集群中性能影响如何？
- 对于 distroless 镜像无法 exec 排障的问题，是否应标准部署 ephemeral debug container（`kubectl debug`）作为补救？

## Related

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-production-operations.md|01 containerd production operations]]
- [[14-容器运行时/03-containerd-CRI-O/02-cri-o-production-guide.md|02 cri o production guide]]
- [[14-容器运行时/04-镜像构建/01-buildkit-production-guide.md|01 buildkit production guide]]
- [[21-生态参考/03-领域索引/README.md|README]]
- [[13-生产运维/03-事件响应/06-container-runtime-threat-response.md|02 container runtime threat response]]

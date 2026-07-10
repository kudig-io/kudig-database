---
title: 纵深防御 x 供应链安全
description: '# 纵深防御 x 供应链安全'
summary: '在 wiki 中，纵深防御和供应链安全被当作两个独立的安全域——[[concepts/security-defense-depth.md|security-defense-depth]] 覆盖运行时分层模型（认证、RBAC、[[entities/networkpolicy.'
category: synthesis
tags:
- k8s
- security
- defense-in-depth
- supply-chain
- sbom
- sigstore
- rbac
- networkpolicy
- kyverno
- opa
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 纵深防御 x 供应链安全 是什么
- 如何 纵深防御 x 供应链安全
trigger_keywords:
- 纵深防御
- 供应链安全
prerequisites:
- kubectl-basics
- ebpf-basics
- policy-basics
relationships:
- target: '[[domain-17-system-foundation/知识字典/security/pod-security-standards.md]]'
  type: uses
- target: '[[entities/networkpolicy.md]]'
  type: related_to
- target: '[[entities/tetragon.md]]'
  type: related_to
- target: '[[entities/trivy.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 纵深防御 x 供应链安全

## 连接点

在 wiki 中，纵深防御和供应链安全被当作两个独立的安全域——[[concepts/security-defense-depth.md|security-defense-depth]] 覆盖运行时分层模型（认证、RBAC、[[entities/networkpolicy.md|NetworkPolicy]]、[[domain-17-system-foundation/知识字典/security/pod-security-standards.md|Pod 安全标准]]），[[supply-chain-security]] 覆盖构建和分发管道（SBOM、镜像签名、SLSA、准入验证）。但它们是**同一安全模型的两半**：供应链安全是代码到达集群**之前**发生的事情，纵深防御是代码到达集群**之后**发生的事情。两者单独都不够——供应链控制假设构建管道是最弱环节，而纵深防御假设最终会有什么东西突破防线。

两者在以下场景中交叉共现：

- **准入控制**（纵深防御第 1 层）在允许 Pod 启动前验证镜像签名和 SBOM（供应链）
- **Kyverno** 在同一个准入 webhook 中同时强制执行 Pod 安全标准（纵深防御第 3 层）和供应链策略（阻止未签名镜像、要求 SBOM）
- **镜像扫描**（[[entities/trivy.md|Trivy]]）在构建时（供应链）和运行时（检测已部署镜像中的新 CVE）双运行，桥接两个域
- **Secret 管理**（Vault）同时保护构建时凭据（供应链）和运行时凭据（纵深防御第 4 层）

## 交叉洞察

**核心洞察：完整的安全模型是一条时间线，不是一个堆栈。** 传统的纵深防御描述的是一组垂直的安全层（网络、运行时、数据）。但真正的安全模型在时间维度上水平延伸：

```
开发 -> 构建 -> 分发 -> 部署 -> 运行时
  [     供应链安全           ]    [  纵深防御        ]
```

**每一个供应链控制都有一个对应的运行时防御，反之亦然：**

| 供应链控制 | 运行时防御 | 两者都失效时的后果 |
|-----------|-----------|------------------|
| 镜像签名（Cosign） | Pod 安全标准（受限级） | 未签名的恶意镜像以完整权限运行 |
| SBOM 生成（Syft） | 运行时监控（[[entities/tetragon.md|Tetragon]]） | 未知的易损依赖执行且未被检测 |
| SLSA 硬化构建 | NetworkPolicy 隔离 | 被攻破的构建产出后门，可自由外泄数据 |
| 准入验证（Kyverno） | RBAC 最小权限 | 未签名镜像以 cluster-admin 身份运行 |
| 依赖扫描（Trivy） | 审计日志 + 告警 | 已知 CVE 被利用，且无法取证 |

**"瑞士奶酪"模型适用：** 没有哪一层能捕获所有威胁。供应链安全捕获被毒化的构建；纵深防御捕获供应链失效。只在其中一个方面投入的组织，其安全模型强度等于它未经测试的假设——要么"构建管道是安全的"，要么"没有东西能突破准入控制"。

**Kyverno 收敛点：** Kyverno（或 OPA Gatekeeper）是这两个域在集群中物理收敛的地方。单个准入策略可以同时执行：
- 供应链规则："只允许来自可信仓库、由 Cosign 签名、附有 SBOM 的镜像"
- 纵深防御规则："以非 root 运行、DROP ALL capabilities、设置资源限制、挂载只读根文件系统"

这意味着最有效的安全姿势是在**准入层**放置尽可能强的控制，因为它可以在每个工作负载启动前同时验证其供应链来源和运行时安全姿态。

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **虚假安全感** | 实施了 SLSA 3 级以上并对所有镜像签名的组织可能放松运行时安全（NetworkPolicy、Pod 安全标准）。但被攻破的签名密钥或构建管道会让供应链控制形同虚设——运行时防御是最后一道防线。 |
| **准入控制延迟** | 全面的准入策略（签名验证 + SBOM 检查 + 漏洞扫描 + Pod 安全检查）增加了 Pod 启动延迟。在频繁部署的大型集群中，这会导致可感知的部署减速。 |
| **SBOM 范围盲区** | SBOM 捕获应用依赖，但不捕获运行时依赖（基础 OS 库、内核模块、eBPF 程序）。"完整的" SBOM 并不意味着运行时环境被完全清点。 |
| **策略重复** | 当供应链和纵深防御团队各自编写 Kyverno 策略时，会出现重叠和冲突——例如供应链要求特定的基础镜像，而纵深防御因 CVE 禁止它们。 |
| **全面扫描的成本** | 对所有已部署镜像进行持续漏洞扫描（运行时 SBOM 重新扫描新 CVE）会产生显著的计算成本和告警疲劳。组织必须根据风险容忍度选择扫描频率。 |

## 开放问题

- **供应链事件响应：** 当检测到供应链突破（如签名密钥泄露）时，运行时响应是什么？多快能通过 SBOM 识别所有受影响的镜像并通过准入策略更新来阻止它们？
- **运行时 SBOM 调和：** 集群是否应该维护所有运行中工作负载的实时 SBOM 清单，在镜像变更时自动更新？这将支持实时的 CVE 影响评估，但尚非标准实践。
- **构建管道的零信任：** wiki 覆盖了运行时访问的零信任，但未覆盖构建管道本身。构建管道是否也应该要求双向认证、短期凭据和持续验证？
- **法规合规映射：** 供应链控制（SBOM、SLSA）和纵深防御控制（RBAC、审计日志）如何共同映射到 SOC 2、FedRAMP 或欧盟《网络弹性法案》等法规框架？
- **SBOM 的可执行性：** 生成 SBOM 很容易，但 SBOM 中的数据如何自动转化为可执行的安全策略？例如，当 SBOM 中发现新的 CVE 时，是否可以自动触发 Pod 的滚动更新或隔离？

## 相关

- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[supply-chain-security]]
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]]
- [[entities/trivy.md|trivy]]
- [[kyverno]]
- [[entities/vault.md|vault]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[falco]] — Falco
- [[kyverno]] — Kyverno
- [[entities/trivy.md|trivy]] — Trivy
- [[entities/vault.md|vault]] — HashiCorp Vault

- [[concepts/Deployment × Secret 管理.md|Deployment × Secret 管理]]- [[domain-17-system-foundation/知识字典/security/runtime-security.md|运行时安全]]


<!-- risk-assessed -->

---
title: 云原生安全
description: '# 云原生安全'
summary: '# 云原生安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- networkpolicy
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云原生安全 是什么
- 如何 云原生安全
trigger_keywords:
- 云原生安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云原生安全

## 概述

[[kubernetes|Kubernetes]] 基于云原生架构，借鉴了 CNCF（云原生计算基金会）关于云原生信息安全的最佳实践建议。其设计目标之一是帮助用户部署安全的云原生平台。CNCF 云原生安全白皮书将安全控制和实践按照不同的生命周期阶段进行划分，从而在每个阶段实施适当的安全措施。

## 核心概念/原理

云原生安全覆盖以下四个生命周期阶段：

- **开发（Develop）**：确保开发环境完整性，遵循安全设计原则，将终端用户安全纳入方案设计。可采用零信任架构、代码审查、威胁建模、模糊测试（fuzzing）和安全混沌工程等手段。
- **分发（Distribute）**：确保容器镜像及集群组件供应链安全。包括扫描镜像漏洞、使用加密传输与可信链、及时更新依赖、使用数字证书验证、将镜像存放在私有仓库等。
- **部署（Deploy）**：限制可部署的内容、部署人员及部署位置。通过命名空间进行应用和组件隔离，容器和命名空间均提供与信息安全相关的隔离机制。
- **运行时（Runtime）**：分为以下关键领域：
  - **访问（Access）**：保护 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]，实施有效的认证与授权，使用 ServiceAccount 管理工作负载身份，启用 TLS 保护 API 流量。
  - **计算（Compute）**：强制执行 Pod 安全标准，使用专为容器化工作负载设计的操作系统（不可变镜像），定义 ResourceQuota 和 LimitRanges，实施节点隔离，使用提供安全限制的容器运行时，在 Linux 节点上使用 AppArmor 或 seccomp 等 Linux 安全模块。
  - **存储（Storage）**：集成支持静态加密的外部存储插件，为 API 对象启用静态加密，定期备份并验证恢复，对网络存储连接进行认证，在应用层实现数据加密，使用硬件安全模块（HSM）保护密钥。
  - **网络（Networking）**：使用 NetworkPolicy 或服务网格保护网络，部分网络插件可通过 VPN 覆盖层提供集群网络加密。
  - **可观测性（Observability）**：确保监控和日志链路具备足够的弹性和完整性保护，部署加密措施使日志既防篡改又保密。

## 关键机制或特性

- **TLS 加密**：默认使用 TLS 保护 API 流量，包括节点与控制平面之间的通信。
- **Pod 安全标准（Pod Security Standards）**：为应用定义必要的权限边界。
- **ServiceAccount**：为工作负载和集群组件提供和管理安全身份。
- **不可变操作系统（Immutable OS）**：仅提供运行容器所必需的服务，减少容器逃逸后的攻击面。
- **节点隔离**：通过 Taints/Tolerations、NodeAffinity 等机制将不同信任上下文的工作负载分隔到不同的节点组上。
- **Linux 安全模块**：如 AppArmor、seccomp，用于限制容器的系统调用和资源访问。

## 使用场景

- 构建企业级安全的云原生应用平台。
- 保护容器镜像供应链，防止带有已知漏洞的镜像进入生产环境。
- 确保多租户或混合信任环境中工作负载的运行时隔离。
- 满足合规性要求，对静态数据和传输中的数据进行加密保护。

## 最佳实践/注意事项

- 采用零信任架构，最小化攻击面，即使是内部威胁也要防范。
- 定义并执行代码审查流程，关注安全问题。
- 建立系统的威胁模型，识别信任边界并据此处理风险。
- 使用私有镜像仓库，仅允许授权客户端拉取镜像。
- 为 etcd 中的 Secret 和 API 对象启用静态加密。
- 定期备份数据并验证恢复能力。
- 对日志实施加密保护，确保其不可篡改且机密。

## 架构深度解析

### 云原生安全全景分层模型

```
┌──────────────────────────────────────────────────────────────┐
│  1. 供应链安全（Supply Chain）                                │
│  ├─ 镜像：扫描（Trivy）+ 签名（cosign/Notation）+ 可信仓库     │
│  ├─ SBOM：CycloneDX/SPDX 生成与存储                           │
│  ├─ 源码/依赖：SCA + 密钥扫描（gitleaks）                     │
│  └─ CI/CD：受信流水线（SLSA）                                 │
├──────────────────────────────────────────────────────────────┤
│  2. 集群安全（Cluster）                                       │
│  ├─ 控制面：RBAC + 审计 + 静态加密 + 准入（PSA/Gatekeeper）    │
│  ├─ 网络：NetworkPolicy + 服务网格 mTLS + Egress 控制          │
│  ├─ 运行时：Falco/KubeArmor 行为检测                           │
│  └─ 密钥：Vault/ESO + 加密存储                                 │
├──────────────────────────────────────────────────────────────┤
│  3. 基础设施（Infrastructure）                                │
│  ├─ 节点加固：CIS Benchmark + 最小化镜像 + 自动补丁            │
│  ├─ 云账号：最小权限 IAM + 云安全态势（CSPM）                  │
│  └─ 灾备：备份/恢复 + 勒索防护                                 │
├──────────────────────────────────────────────────────────────┤
│  4. 应用层（Application）                                     │
│  ├─ 配置：ConfigMap/Secret 扫描 + 安全配置基线                 │
│  ├─ 依赖：语言依赖漏洞 + License 合规                          │
│  └─ 运行：OWASP 应用防护（WAF/RASP）                          │
└──────────────────────────────────────────────────────────────┘
```

### 核心安全框架映射

| 框架 | 定位 | 覆盖层 |
| --- | --- | --- |
| NSA/CISA Kubernetes Hardening Guide | 官方加固基线 | 集群/基础设施 |
| CIS Kubernetes Benchmark | 合规检查基线 | 集群/基础设施 |
| MITRE ATT&CK（K8s 矩阵） | 攻击行为模型 | 全层检测 |
| SLSA | 供应链完整性 | 供应链 |
| SOC2/ISO27001 | 治理合规 | 全层 |

### 流程步骤

1. 从供应链开始：镜像扫描、签名、SBOM 归档，建立可信制品基线。
2. 集群基线：RBAC 最小化、PSA/Gatekeeper 准入、NetworkPolicy 默认拒绝。
3. 运行时：Falco/KubeArmor 异常检测 + 告警闭环。
4. 基础设施：节点 CIS 加固 + 云 IAM 最小权限 + 备份演练。
5. 持续治理：定期审计、漏洞修复 SLA、事件响应演练。

## 生产案例

### 案例 1：供应链投毒——伪装镜像包绕过镜像扫描

| 时间 | 事件 |
| --- | --- |
| T+0 | 开发者使用非官方源拉取"流行"镜像（含恶意依赖） |
| T+1d | 镜像通过 CI 扫描（漏洞 0，但含后门逻辑） |
| T+3d | 运行时 Falco 检测到异常外联（挖矿池地址） |
| T+1w | 溯源：镜像包与官方仅差一层，二进制被替换 |
| T+2w | 全仓重扫 + 建立可信仓库白名单 + 强制签名校验 |

- **根因分析**：漏洞扫描只能发现已知 CVE，无法识别"干净但恶意"的镜像；供应链安全需要扫描 + 签名 + 来源白名单三层组合。
- **修复命令**：
```bash
# 1. 建立可信仓库白名单（Kyverno 策略示例，🟡 中风险）
kubectl apply -f - <<'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-image-source
spec:
  validationFailureAction: Enforce
  rules:
  - name: only-trusted-registry
    match: { resources: { kinds: ["Pod"] } }
    validate:
      message: "镜像必须来自可信仓库"
      pattern:
        spec:
          containers:
          - image: "trusted-registry.example.com/*"
EOF
# 2. 启用 cosign 签名校验（🔴 高风险：存量未签名镜像会阻断）
kubectl apply -f cosign-verify-policy.yaml
```

### 案例 2：KubeCon 泄露事件——集群被从窃取的 kubeconfig 接管

| 时间 | 事件 |
| --- | --- |
| T+0 | 某公司员工笔记本被入侵，kubeconfig（cluster-admin）被窃取 |
| T+2d | 攻击者通过 VPN 直连集群，创建挖矿 DaemonSet |
| T+4d | 账单暴涨触发 FinOps 告警，定位到异常节点 |
| T+6d | 轮换全部凭证、清理后门、限制控制面访问 IP |
| T+2w | 引入 OIDC + 短时凭证 + 网络策略，事件闭环 |

- **根因分析**：长期静态 kubeconfig 是最大泄露面；控制面缺少网络级访问控制与行为检测。
- **修复命令**：
```bash
# 1. 立即吊销（🔴 高风险）
kubectl delete secret <leaked-secret>
# 2. 控制面访问控制（安全组/IP 白名单，按云厂商执行）
# 3. 运行时检测异常 DaemonSet（只读）
kubectl get ds -A -o wide
kubectl get pods -A --field-selector=status.phase=Running | awk '{print $1}' | sort | uniq -c
```

## 对比评测

| 维度 | NSA/CISA 加固指南 | CIS Benchmark | MITRE ATT&CK | SLSA |
| --- | --- | --- | --- | --- |
| 类型 | 最佳实践指南 | 检查清单 | 威胁模型 | 供应链框架 |
| 自动化 | 人工对照 | kubescape/CIS 扫描 | 检测规则 | 流水线级别 |
| 更新频率 | 随版本 | 随版本 | 持续 | 版本化 |
| 落地工具 | 人工 + 工具 | kubescape/trivy | Falco/审计 | cosign/in-toto |

**选型建议**：以 NSA/CISA 指南为纲领、CIS 为基线检查、ATT&CK 指导检测规则建设、SLSA 管供应链，四者组合形成完整体系。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 扫描通过但被入侵 | 恶意逻辑非 CVE | 运行时检测 + 签名 + 仓库白名单 |
| 审计发现未授权访问 | kubeconfig 泄露/宽 RBAC | `kubectl auth can-i --list` 审计 |
| Falco 误报刷屏 | 规则过宽 | 按命名空间/镜像白名单收敛规则 |
| SBOM 缺失 | 构建未集成 | CI 增加 `trivy sbom --format cyclonedx` |
| 备份无法恢复 | 未演练 | 季度性恢复演练（DR 测试） |

## 生产部署清单

- [ ] 供应链：镜像扫描 + 签名 + SBOM + 可信仓库白名单
- [ ] 集群：RBAC 最小化 + PSA + NetworkPolicy 默认拒绝
- [ ] 运行时：Falco/KubeArmor 部署并接入告警
- [ ] 基础设施：节点 CIS + 云 IAM 最小权限 + 备份演练
- [ ] 治理：季度安全审计 + 漏洞 SLA + 事件响应演练

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 存在可登录控制面的长期静态凭证 | 立即切换 OIDC/短时凭证并轮换 |
| P1 | 镜像无签名、仓库无白名单 | 分阶段启用签名与 Kyverno 强制 |
| P2 | 无运行时行为检测 | 部署 Falco/KubeArmor 并收敛规则 |

## 面试要点

1. **Q：云原生安全与传统安全的区别？**
   A：边界从物理网络迁移到"身份 + 策略 + 运行时"：基础设施不可变（镜像即制品）、动态编排（Pod 生命周期短）、攻击面分散（容器/API/供应链）。因此强调左移（供应链、IaC 扫描）+ 运行时检测 + 声明式策略（准入）三层防御。
2. **Q：如何设计镜像供应链安全？**
   A：四层：可信来源（仓库白名单 + 拉取控制）、内容检查（漏洞扫描 + 密钥检测 + SBOM）、完整性（cosign/Notation 签名 + 验证策略）、可追溯（SBOM 归档 + 构建元数据）。关键点是验证必须强制执行（准入阻断），而非仅 CI 提示。
3. **Q：Kubernetes 集群最大的攻击面是什么？如何防护？**
   A：三大攻击面：一是 API Server（凭证泄露、宽 RBAC）——对策：OIDC 短时凭证、RBAC 最小化、审计、网络限制；二是供应链（恶意镜像）——对策：签名 + 白名单 + 扫描；三是运行时逃逸——对策：Falco/KubeArmor、seccomp、只读根文件系统、Pod Security 基线。

## 运维要点

- 基线固化：NSA/CIS 检查结果纳入发布门禁（新集群必须通过）。
- 告警闭环：安全告警（Falco/审计）对接工单，24h 处置 SLA。
- 演练常态化：季度安全演练（凭证泄露、勒索恢复、供应链事件）。
- 资产管理：SBOM 与镜像清单实时更新，支撑应急溯源。
- 排障入口：安全事件先溯源（镜像/凭证/配置三个维度），再定处置。

## 参考链接

- https://kubernetes.io/docs/concepts/[[17-系统基础/06-知识字典/security/cloud-native-security.md|cloud-native-security]]/

## Related
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->

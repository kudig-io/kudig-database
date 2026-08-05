---
title: 安全加固基线
description: Kubernetes 生产集群安全加固基线：CIS Benchmark 关键项、Pod Security Standards 三级落地、准入控制选型、OWASP Kubernetes Top 10、供应链安全与 90 天落地计划
summary: 覆盖 CIS 关键检查项、PSS 分级与落地路径、Kyverno/Gatekeeper/VAP 准入控制、OWASP K8s Top 10（2025）、镜像供应链、运行时检测与 90 天加固路线
category: references
tags:
- k8s
- security
- hardening
- cis-benchmark
- pod-security
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- 安全工程师
- SRE
- 平台工程师
estimated_read_time: 25min
---

# 安全加固基线

> 本文是**常态化安全基线**，与护网前的专项收敛（[[08-security-defense-checklist]]）互补：基线决定日常水位，护网在基线之上做临时加强。安全原则：最小权限、默认拒绝、纵深防御、可验证。

## 1. CIS Kubernetes Benchmark 关键项

CIS Benchmark 有 200+ 条建议，以下是投入产出比最高的关键项（用 `kube-bench` 自动化核查）：

### 1.1 控制面（CIS 1.x）

| 项 | 要求 |
|---|---|
| 匿名访问 | `--anonymous-auth=false` |
| 授权模式 | `--authorization-mode=Node,RBAC`（不含 AlwaysAllow） |
| 准入插件 | 启用 `NodeRestriction`、`AlwaysPullImages`、`EventRateLimit`；禁用 `AlwaysAdmit` 等绕过类插件，保留 `NamespaceLifecycle` 等必要默认插件 |
| 审计 | `--audit-log-path` + audit policy + 外送 |
| 静态加密 | `--encryption-provider-config` 配置 Secret 加密 |
| TLS | `--tls-min-version=VersionTLS12` 起步 |
| insecure-port | 已废弃版本确认 8080 关闭 |

### 1.2 RBAC（CIS 5.1.x）

- **避免使用 `system:masters` 组**（绕过全部 RBAC 且不走审计拒绝路径）
- **限制 `bind` / `impersonate` / `escalate` 权限**——这三个动词等于变相授予提权能力
- 限制 wildcard（`*`）资源与动词
- 限制 `pods/exec`、Secret 读取的授予范围
- 最小化 cluster-admin 持有者

### 1.3 Pod 安全（CIS 5.2.x）→ 见 PSS 节

### 1.4 其他高频项

- 所有命名空间定义 NetworkPolicy（5.3.2）
- Secret 优先以文件挂载而非环境变量注入（5.4.1）；考虑外部密钥存储（5.4.2）
- 不使用 default namespace 跑业务（5.6.4）

## 2. Pod Security Standards（PSS）落地

PSS（1.25 起替代已废弃的 PodSecurityPolicy）定义三个等级：

| 等级 | 约束 | 适用 |
|---|---|---|
| Privileged | 无限制 | 仅系统组件（CNI、CSI、节点 Agent） |
| Baseline | 阻断已知提权路径：禁 hostNetwork/hostPID/hostIPC、特权容器、敏感 hostPath | **任何生产命名空间的底线** |
| Restricted | 追加：必须 non-root、drop ALL capabilities、seccomp、禁特权提升 | 应用负载的目标态 |

**落地路径（存量集群不翻车的关键）：**

```yaml
# 阶段一：只审计不阻断（跑 2–4 周收集违规）
pod-security.kubernetes.io/audit: restricted
pod-security.kubernetes.io/warn: restricted
# 阶段二：修复违规后 enforce
pod-security.kubernetes.io/enforce: restricted
```

- kube-system 等系统命名空间豁免（`enforce: privileged`）
- 豁免通过 PSA 准入插件配置的 exemptions（usernames / runtimeClasses / namespaces 白名单）或准入控制器例外实现，拒绝整命名空间放水

## 3. 准入控制选型

| 方案 | 特点 | 适用 |
|---|---|---|
| Pod Security Admission（内置） | 零依赖，只覆盖 Pod 安全 | 所有集群的底线 |
| ValidatingAdmissionPolicy（CEL，内置） | 无外部依赖的声明式校验，1.30+ GA | 简单规则（禁 latest、强制标签） |
| Kyverno | 原生 YAML 策略、支持变异/生成/镜像验签 | 大多数团队的首选，学习成本低 |
| OPA Gatekeeper | Rego 表达力强、生态成熟 | 复杂策略逻辑、已有 OPA 体系 |

**最小策略集（生产强制）：**

1. 镜像必须来自受信仓库（registry allowlist）
2. 禁止 `:latest`、必须 digest 或固定 tag
3. 必须设置 resources.requests
4. 禁止 privileged / hostNetwork / 敏感 hostPath
5. 必须设置标准标签（app、team、env）——成本与审计的基础

## 4. OWASP Kubernetes Top 10（2025 版）

| ID | 风险 | 本专题对应控制 |
|---|---|---|
| K01 | 工作负载不安全配置 | PSS + SecurityContext（[[03-workload#6. 安全基线（工作负载侧）]]） |
| K02 | 授权过度宽松 | RBAC 收敛（本文 CIS 5.1 + [[08-security-defense-checklist#2. 认证与授权（RBAC 收敛）]]） |
| K03 | 密钥管理失败 | 静态加密 + 外部密钥管理（Vault/KMS） |
| K04 | 缺乏集群级策略执行 | 准入控制策略集（本文第 3 节） |
| K05 | 网络分段缺失 | 默认拒绝 NetworkPolicy（[[04-network#3. NetworkPolicy]]） |
| K06 | 组件过度暴露 | 暴露面收敛（[[08-security-defense-checklist#1. 暴露面收敛（第一优先级）]]） |
| K07 | 组件配置错误/存在漏洞 | CIS 基线 + 版本管理（[[13-upgrade-certificate-runbook]]） |
| K08 | 集群到云的横向移动（2025 新增） | IRSA/Workload Identity 收敛 + 元数据服务防护（托管集群特有） |
| K09 | 认证机制缺陷 | OIDC + 短期凭据 + 匿名访问关闭 |
| K10 | 日志与监控不足 | 审计外送 + 运行时检测（[[09-observability]] + 本文第 6 节） |

> 经验值：生产审计中 **K01/K02/K05 三类问题占绝大多数**——先拿下这三项，审计报告就从"红旗"变"润色项"。

### K08 专项：集群到云横向移动（托管集群重点）

- 用 IRSA（EKS）/ Workload Identity（GKE）/ Managed Identity（AKS）替代节点实例角色——节点角色意味着该节点上**所有 Pod** 共享同一份云权限
- 阻断 Pod 访问实例元数据服务（169.254.169.254）：NetworkPolicy / CNI 策略 / 云厂商机制（如 EKS 的 IMDSv2 + hop limit=1）
- 定期用 `kubectl auth can-i --list --as system:serviceaccount:<ns>:<sa>` 审计每个 SA 实际权限，常见惊喜：某个 SA 带着没人记得的通配权限

## 5. 供应链安全

| 控制 | 实施 |
|---|---|
| 制品溯源 | CI 生成 SBOM（Syft）、SLSA 级别 attest |
| 镜像签名 | cosign（keyless/OIDC 模式），准入时验签（Kyverno verifyImages / Sigstore Policy Controller） |
| 漏洞扫描 | CI 门禁（高危阻断）+ 仓库定时扫描 + 存量运行镜像扫描（Trivy/Grype） |
| 镜像固定 | 生产清单按 digest 引用，禁浮动 tag |
| 仓库收敛 | 仅受信仓库可拉取（准入强制），基础镜像统一维护 |

## 6. 运行时检测与响应

- Falco / Cilium Tetragon 部署到全部节点，告警接 SOC/安全值班
- 基线规则集：敏感文件写入（/etc/shadow 等）、异常 outbound 连接、容器内启动 shell、crypto mining 特征
- 取证准备：日志集中留存、节点快照能力、容器逃逸后的快速隔离流程（cordon + 安全组隔离，见 [[08-security-defense-checklist#8. 护网期间运行机制]]）

## 7. 90 天加固路线

**第 0–30 天（基线控制）：**
RBAC 通配收敛 → 匿名访问关闭 + 审计开启 → Secret 静态加密 → PSS audit 模式全量打标 → 生产命名空间 default-deny NetworkPolicy → 工作负载 SecurityContext 基线 → CI 镜像扫描门禁

**第 31–60 天（策略与身份）：**
Kyverno/Gatekeeper 最小策略集 enforce → 工作负载身份（IRSA 等）替换节点角色 → PSS 违规修复并 enforce → TLS 全覆盖（cert-manager）→ 密钥外部化（CSI Secrets Store/Vault）

**第 61–90 天（隔离与韧性）：**
多租户隔离（配额 + 节点隔离）→ Egress 管控 → 镜像签名验证 → Falco/Tetragon 上线 → kube-bench/kube-hunter 定期扫描制度化 → 恢复演练（etcd + Velero）

## Related

- [[08-security-defense-checklist|护网/攻防演练检查项]]
- [[06-initialization-checklist|初始化配置检查项（安全基线）]]
- [[03-workload|工作负载最佳实践（安全基线）]]
- [[20-最佳实践/07-scenarios/security-hardening|安全加固场景]]

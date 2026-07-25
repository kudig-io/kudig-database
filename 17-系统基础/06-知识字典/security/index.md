---
title: 安全知识词典
description: 涵盖 Kubernetes 安全全领域的完整术语体系，包括 RBAC、Pod 安全、供应链安全、策略引擎、密钥管理、运行时安全等
summary: 安全领域词典，覆盖 RBAC、Pod Security、Falco、Kyverno、OPA、Trivy、SPIFFE、Vault、供应链安全等核心概念
category: dictionary
tags:
- dictionary
- security
- rbac
- pod-security
- supply-chain
- policy
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
audience:
- 安全工程师
- 平台工程师
- SRE
---

# 安全知识词典（Security）

> 本词典覆盖 Kubernetes 安全领域的核心术语、技术组件及工程实践，是安全工程师和平台工程师构建零信任云原生安全体系的权威参考。

## 领域概述

Kubernetes 安全是云原生生产环境的生命线，包括：

- **身份与访问**：RBAC、ServiceAccount、OIDC、多租户
- **Pod 安全**：Security Context、Pod Security Standards、准入控制
- **网络安全**：NetworkPolicy、mTLS、零信任
- **供应链安全**：镜像签名、SBOM、SLSA、in-toto
- **策略引擎**：OPA/Gatekeeper、Kyverno、Kubewarden
- **密钥管理**：Vault、External Secrets、SOPS
- **运行时安全**：Falco、KubeArmor、系统调用监控
- **合规审计**：Kubescape、CIS Benchmark、漏洞扫描

## 核心术语定义

### 身份与访问控制

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| RBAC | 基于角色的访问控制 | Role/ClusterRole/Binding |
| Role | 命名空间级权限 | 最小权限原则 |
| ClusterRole | 集群级权限 | 跨命名空间 |
| RoleBinding | 角色绑定 | 用户→角色 |
| ServiceAccount | Pod 身份标识 | 自动挂载 Token |
| OIDC | OpenID Connect 认证 | 企业 SSO 集成 |
| Admission Controller | 准入控制器 | 请求拦截/验证 |
| Multi-tenancy | 多租户隔离 | Namespace/vCluster |
| Capsule | 多租户框架 | 租户隔离、配额 |
| Paralus | 零信任访问平台 | 即时权限 |

### Pod 安全

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Pod Security Admission | Pod 安全准入控制 | 替代 PSP |
| Pod Security Standards | Pod 安全标准 | Privileged/Baseline/Restricted |
| Security Context | 安全上下文配置 | 用户/权限/能力 |
| LimitRange | 资源限制范围 | 默认值/最大最小 |
| ResourceQuota | 命名空间资源配额 | CPU/内存/Pod数 |
| PSP (PodSecurityPolicy) | Pod 安全策略（已废弃） | 1.25 移除 |

### 供应链安全

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Supply Chain Security | 软件供应链安全 | 构建→分发→部署 |
| SLSA | 软件供应链级别评估 | L1-L4 成熟度 |
| in-toto | 供应链完整性框架 | 布局/链接 |
| SBOM | 软件物料清单 | SPDX/CycloneDX |
| Notary Project | 镜像签名与验证 | CNCF |
| Ratify | 制品验证框架 | 签名/SBOM/许可证 |
| TUF | 安全更新框架 | 密钥管理 |
| Sigstore/Cosign | 无密钥签名 | Keyless 签名 |
| Trivy | 全能安全扫描 | CVE/配置/密钥 |

### 策略引擎

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Policy as Code | 策略代码化 | 可测试、可版本化 |
| OPA | 开放策略代理 | Rego 语言 |
| Gatekeeper | OPA 的 K8s 集成 | CNCF |
| Kyverno | K8s 原生策略引擎 | YAML 策略 |
| Kubewarden | WebAssembly 策略引擎 | 多语言策略 |
| Cedar | AWS 策略语言 | 细粒度授权 |
| OpenFGA | 细粒度授权 | Zanzibar 模型 |

### 密钥管理

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Vault | HashiCorp 密钥管理 | 动态密钥、加密 |
| External Secrets | 外部密钥同步 | Vault/AWS/GCP |
| SOPS | 加密文件工具 | age/PGP/KMS |
| Bank-Vaults | Vault K8s Operator | 自动注入 |
| Secret | K8s 密钥对象 | base64、非加密 |
| Athenz | 服务身份平台 | X.509/mTLS |

### 运行时安全

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Falco | 运行时威胁检测 | 系统调用监控 |
| KubeArmor | 运行时安全执行 | eBPF/LSM |
| Runtime Security | 运行时安全 | 异常检测 |
| Confidential Containers | 机密容器 | TEE/SGX |
| Keylime | 节点完整性验证 | TPM |
| Containerssh | 容器 SSH 审计 | 会话记录 |

### 身份与零信任

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| SPIFFE | 服务工作负载身份标准 | URI SAN |
| SPIRE | SPIFFE 运行时环境 | 自动证书轮换 |
| Dex | OIDC 联邦认证 | 多 IdP 集成 |
| Keycloak | 开源身份管理 | SSO/SAML/OIDC |
| OAuth2 Proxy | OAuth2 反向代理 | Web 应用保护 |
| Cert-manager | 证书自动管理 | Let's Encrypt |
| Parsec | 平台抽象安全服务 | 硬件安全模块 |
| Tokenetes | 令牌管理 | 动态凭证 |

### 合规与审计

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Kubescape | K8s 合规扫描 | NSA/CIS/MITRE |
| Cartography | 基础设施可视化 | 攻击面分析 |
| CIS Benchmark | 安全基线标准 | 配置审计 |
| Security Checklist | 安全检查清单 | 全面安全审计 |
| Webhook | 准入 Webhook | 自定义验证 |

## 技术组件索引

### 身份与访问

- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|ClusterRole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|RoleBinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|ClusterRoleBinding]]
- [[17-系统基础/06-知识字典/security/service-account.md|Service Account]]
- [[17-系统基础/06-知识字典/security/service-accounts.md|Service Accounts]]
- [[17-系统基础/06-知识字典/security/service-account-token.md|Service Account Token]]
- [[17-系统基础/06-知识字典/security/admission-controller.md|Admission Controller]]
- [[17-系统基础/06-知识字典/security/controlling-access-to-the-kubernetes-api.md|API 访问控制]]
- [[17-系统基础/06-知识字典/security/multi-tenancy.md|Multi-tenancy]]
- [[17-系统基础/06-知识字典/security/capsule.md|Capsule]]
- [[17-系统基础/06-知识字典/security/paralus.md|Paralus]]
- [[17-系统基础/06-知识字典/security/dex.md|Dex]]
- [[17-系统基础/06-知识字典/security/keycloak.md|Keycloak]]
- [[17-系统基础/06-知识字典/security/oauth2-proxy.md|OAuth2 Proxy]]
- [[17-系统基础/06-知识字典/security/role-based-access-control-good-practices.md|RBAC 最佳实践]]
- [[17-系统基础/06-知识字典/security/hardening-guide---authentication-mechanisms.md|认证加固指南]]

### Pod 安全

- [[17-系统基础/06-知识字典/security/pod-security-admission.md|Pod Security Admission]]
- [[17-系统基础/06-知识字典/security/pod-security-standards.md|Pod Security Standards]]
- [[17-系统基础/06-知识字典/security/pod-security-policies.md|Pod Security Policies]]
- [[17-系统基础/06-知识字典/security/pod-security-policy.md|Pod Security Policy]]
- [[17-系统基础/06-知识字典/security/security-context.md|Security Context]]
- [[17-系统基础/06-知识字典/security/limit-ranges.md|Limit Ranges]]
- [[17-系统基础/06-知识字典/security/resource-quotas.md|Resource Quotas]]
- [[17-系统基础/06-知识字典/security/linux-kernel-security-constraints-for-pods-and-containers.md|Linux 内核安全约束]]
- [[17-系统基础/06-知识字典/security/process-id-limits-and-reservations.md|PID 限制]]
- [[17-系统基础/06-知识字典/security/node-resource-managers.md|节点资源管理]]

### 供应链安全

- [[17-系统基础/06-知识字典/security/supply-chain-security.md|Supply Chain Security]]
- [[17-系统基础/06-知识字典/security/in-toto.md|in-toto]]
- [[17-系统基础/06-知识字典/security/notary-project.md|Notary Project]]
- [[17-系统基础/06-知识字典/security/ratify.md|Ratify]]
- [[17-系统基础/06-知识字典/security/tuf.md|TUF]]
- [[17-系统基础/06-知识字典/security/trivy.md|Trivy]]

### 策略引擎

- [[17-系统基础/06-知识字典/security/policy-as-code.md|Policy as Code]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
- [[17-系统基础/06-知识字典/security/gatekeeper.md|Gatekeeper]]
- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]
- [[17-系统基础/06-知识字典/security/kubewarden.md|Kubewarden]]
- [[17-系统基础/06-知识字典/security/cedar.md|Cedar]]
- [[17-系统基础/06-知识字典/security/openfga.md|OpenFGA]]
- [[17-系统基础/06-知识字典/security/open-policy-containers.md|Open Policy Containers]]

### 密钥管理

- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
- [[17-系统基础/06-知识字典/security/external-secrets.md|External Secrets]]
- [[17-系统基础/06-知识字典/security/sops.md|SOPS]]
- [[17-系统基础/06-知识字典/security/bank-vaults.md|Bank-Vaults]]
- [[17-系统基础/06-知识字典/security/secret.md|Secret]]
- [[17-系统基础/06-知识字典/security/good-practices-for-kubernetes-secrets.md|Secret 最佳实践]]
- [[17-系统基础/06-知识字典/security/secrets-management-deep-dive.md|密钥管理深度解析]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz]]
- [[17-系统基础/06-知识字典/security/tokenetes.md|Tokenetes]]

### 运行时安全

- [[17-系统基础/06-知识字典/security/falco.md|Falco]]
- [[17-系统基础/06-知识字典/security/kubearmor.md|KubeArmor]]
- [[17-系统基础/06-知识字典/security/runtime-security.md|Runtime Security]]
- [[17-系统基础/06-知识字典/security/confidential-containers.md|Confidential Containers]]
- [[17-系统基础/06-知识字典/security/keylime.md|Keylime]]
- [[17-系统基础/06-知识字典/security/containerssh.md|ContainerSSH]]

### 身份与零信任

- [[17-系统基础/06-知识字典/security/spiffe.md|SPIFFE]]
- [[17-系统基础/06-知识字典/security/spire.md|SPIRE]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity.md|SPIFFE/SPIRE Identity]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]
- [[17-系统基础/06-知识字典/security/certificate-authority.md|Certificate Authority]]
- [[17-系统基础/06-知识字典/security/parsec.md|Parsec]]

### 合规与审计

- [[17-系统基础/06-知识字典/security/kubescape.md|Kubescape]]
- [[17-系统基础/06-知识字典/security/cartography.md|Cartography]]
- [[17-系统基础/06-知识字典/security/security-checklist.md|Security Checklist]]
- [[17-系统基础/06-知识字典/security/application-security-checklist.md|Application Security Checklist]]
- [[17-系统基础/06-知识字典/security/cloud-native-security.md|Cloud Native Security]]
- [[17-系统基础/06-知识字典/security/cloud-native-security-practices.md|Cloud Native Security Practices]]
- [[17-系统基础/06-知识字典/security/webhook.md|Webhook]]
- [[17-系统基础/06-知识字典/security/kubernetes-api-server-bypass-risks.md|API Server 绕过风险]]
- [[17-系统基础/06-知识字典/security/hardening-guide---scheduler-configuration.md|调度器加固]]
- [[17-系统基础/06-知识字典/security/security-for-linux-nodes.md|Linux 节点安全]]
- [[17-系统基础/06-知识字典/security/security-for-windows-nodes.md|Windows 节点安全]]

## 深度技术解析

### RBAC 最小权限实践

```yaml
# 只允许读取特定命名空间的 Pod
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: pod-reader
rules:
  - apiGroups: [""]
    resources: ["pods", "pods/log"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: production
subjects:
  - kind: User
    name: developer@example.com
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
```

### Pod Security Standards 对比

| 级别 | 适用场景 | 关键限制 |
|------|----------|----------|
| Privileged | 系统组件 | 无限制 |
| Baseline | 通用应用 | 禁止特权容器、hostNetwork |
| Restricted | 安全敏感 | 非 root、只读 rootfs、无 capabilities |

```yaml
# Restricted 级别 Pod 示例
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 2000
  seccompProfile:
    type: RuntimeDefault
containers:
  - name: app
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
```

### 供应链安全流水线

```
源码 → 构建 → 扫描 → 签名 → 推送 → 验证 → 部署
 │       │       │       │       │       │       │
 Git   BuildKit Trivy  Cosign  Harbor  Ratify  Kyverno
 SLSA  SBOM   CVE    Sigstore  复制   策略    准入
```

### Kyverno 策略示例

```yaml
# 禁止使用 latest 标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  rules:
    - name: validate-image-tag
      match:
        resources:
          kinds: ["Pod"]
      validate:
        message: "禁止使用 :latest 标签"
        pattern:
          spec:
            containers:
              - image: "!*:latest"
```

## 生产案例

### 案例 1：特权容器逃逸

**现象**：攻击者通过特权容器访问宿主机文件系统

**根因**：Pod 设置了 `privileged: true`，容器可访问所有设备

**解决**：
- 启用 Pod Security Admission (restricted)
- 禁止 privileged、hostPID、hostNetwork
- 使用 Falco 检测异常系统调用

### 案例 2：Secret 泄露到日志

**现象**：应用日志中打印了数据库密码

**根因**：环境变量注入的 Secret 被应用打印到 stdout

**解决**：
- 使用 Vault 动态密钥（短生命周期）
- 文件挂载替代环境变量
- 日志脱敏处理

### 案例 3：镜像供应链攻击

**现象**：部署的镜像包含恶意代码，与源码不一致

**根因**：镜像未签名，无法验证构建来源

**解决**：
- Cosign 签名 + Ratify 验证
- SLSA L3 构建证明
- 准入策略拒绝未签名镜像

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Pod 被拒绝创建 | 准入策略拦截 | `kubectl get events`、策略日志 |
| 权限不足 403 | RBAC 配置错误 | `kubectl auth can-i`、Role/Binding |
| Secret 挂载失败 | Secret 不存在/权限 | 检查 Secret、SA 权限 |
| 证书过期 | cert-manager 异常 | 检查 Certificate 状态 |
| Falco 误报 | 规则不匹配 | 调整规则、白名单 |
| 镜像拉取被拒 | 签名验证失败 | 检查 Ratify/Cosign 配置 |

## 命令速查

```bash
# 权限检查
kubectl auth can-i create pods --namespace production
kubectl auth can-i --list --namespace production

# RBAC 审计
kubectl get roles,rolebindings -n production
kubectl get clusterroles,clusterrolebindings

# Pod 安全检查
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].securityContext.privileged==true) | .metadata.name'

# 策略检查
kubectl get clusterpolicy  # Kyverno
kubectl get constraints    # Gatekeeper

# 安全扫描
trivy image myapp:v1
trivy k8s --report summary cluster
kubescape scan --verbose

# Secret 审计
kubectl get secrets -A -o custom-columns='NS:.metadata.namespace,NAME:.metadata.name,TYPE:.type'
```

## FAQ

**Q: Kyverno 和 OPA/Gatekeeper 如何选择？**
A: Kyverno 用 YAML 写策略，K8s 原生体验，学习曲线低；OPA 用 Rego 语言，更灵活但学习成本高。简单策略选 Kyverno，复杂逻辑选 OPA。

**Q: K8s Secret 是否安全？**
A: K8s Secret 仅 base64 编码，非加密。生产环境应：1) 启用 etcd 加密；2) 使用外部密钥管理（Vault/External Secrets）；3) 最小化 RBAC 访问；4) 避免环境变量注入。

**Q: 如何实现零信任网络？**
A: 分层实现：1) 身份：SPIFFE/SPIRE 工作负载身份；2) 传输：mTLS（Istio/Linkerd）；3) 网络：NetworkPolicy 微分段；4) 应用：OAuth2/OIDC 认证。

**Q: 镜像签名和 SBOM 的区别？**
A: 镜像签名（Cosign/Notary）验证镜像来源和完整性（“谁构建的”）；SBOM（SPDX/CycloneDX）记录镜像内所有组件和版本（“包含什么”）。两者互补，生产环境应同时启用。

**Q: Falco 和 KubeArmor 的区别？**
A: Falco 是检测工具（发现异常并告警，不阻断）；KubeArmor 是执行工具（可以阻断恶意行为）。生产环境建议两者结合：KubeArmor 阻断 + Falco 检测。

## 版本兼容矩阵

| 组件 | 当前版本 | K8s 兼容 | 关键变更 |
|------|---------|----------|----------|
| Pod Security Admission | GA | 1.25+ | 替代 PSP |
| Kyverno | 1.13 | 1.25+ | CEL 策略支持 |
| Gatekeeper | 3.18 | 1.25+ | 外部数据 |
| Falco | 0.39 | 1.25+ | 插件架构 |
| Trivy | 0.56 | - | SBOM 扫描 |
| Vault | 1.18 | - | 性能优化 |
| SPIRE | 1.11 | 1.25+ | 上游授权 |
| cert-manager | 1.16 | 1.25+ | Gateway API 支持 |

## 缩略语表

| 缩略语 | 全称 | 说明 |
|--------|------|------|
| RBAC | Role-Based Access Control | 基于角色的访问控制 |
| PSA | Pod Security Admission | Pod 安全准入 |
| PSS | Pod Security Standards | Pod 安全标准 |
| mTLS | Mutual TLS | 双向 TLS |
| SBOM | Software Bill of Materials | 软件物料清单 |
| SLSA | Supply-chain Levels for Software Artifacts | 软件供应链级别 |
| CVE | Common Vulnerabilities and Exposures | 通用漏洞披露 |
| TEE | Trusted Execution Environment | 可信执行环境 |
| LSM | Linux Security Module | Linux 安全模块 |
| OIDC | OpenID Connect | 开放身份连接 |

## 学习路径

```
基础: RBAC → Security Context → Pod Security Standards
进阶: NetworkPolicy → Kyverno/OPA → Trivy 扫描
高级: SPIFFE/SPIRE → Vault → 供应链安全
专家: 自定义准入控制器 → eBPF 安全 → 零信任架构
```

**安全分层模型（4C）**：
1. **Cloud** - 云平台安全（IAM、VPC、安全组）
2. **Cluster** - 集群安全（RBAC、PSA、NetworkPolicy）
3. **Container** - 容器安全（镜像扫描、最小权限、签名）
4. **Code** - 应用安全（SAST/DAST、依赖扫描、密钥管理）

## 检查清单

### 安全就绪检查

- [ ] Pod Security Admission 已启用（restricted 级别）
- [ ] 默认拒绝 NetworkPolicy 已部署
- [ ] 镜像签名验证已启用（Cosign + Ratify）
- [ ] 密钥管理已外部化（Vault/External Secrets）
- [ ] RBAC 最小权限已审计
- [ ] 镜像漏洞扫描已集成 CI/CD
- [ ] 运行时安全监控已部署（Falco）
- [ ] 审计日志已开启并采集
- [ ] 证书自动轮换已配置（cert-manager）
- [ ] CIS Benchmark 扫描已定期执行
- [ ] etcd 静态加密已启用（EncryptionConfiguration）
- [ ] 禁止特权容器和 hostPath 挂载
- [ ] ServiceAccount Token 自动挂载已禁用（非必要 Pod）
- [ ] 容器以非 root 用户运行
- [ ] 只读根文件系统已启用

## 参考链接

- https://kubernetes.io/docs/concepts/security/
- https://kubernetes.io/docs/reference/access-authn-authz/rbac/
- https://kyverno.io/
- https://open-policy-agent.github.io/gatekeeper/
- https://falco.org/
- https://www.vaultproject.io/
- https://spiffe.io/
- https://aquasecurity.github.io/trivy/
- https://kubearmor.io/
- https://kubescape.io/
- https://slsa.dev/
- https://sigstore.dev/

## Related

- [[17-系统基础/06-知识字典/networking/network-policy.md|NetworkPolicy 网络安全]]
- [[17-系统基础/06-知识字典/operations/backup-restore.md|备份恢复]]
- [[17-系统基础/06-知识字典/platform-engineering/admission-webhook.md|Admission Webhook]]
- [[17-系统基础/06-知识字典/observability/audit-logging.md|审计日志]]


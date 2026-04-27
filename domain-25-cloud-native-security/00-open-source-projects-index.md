# Domain-25 云原生安全 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Falco v0.41 / Kyverno v1.14 / cert-manager v1.17

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、运行时安全 (Runtime Security)](#二运行时安全-runtime-security)
- [三、策略与合规 (Policy & Compliance)](#三策略与合规-policy--compliance)
- [四、身份与访问 (Identity & Access)](#四身份与访问-identity--access)
- [五、供应链安全 (Supply Chain)](#五供应链安全-supply-chain)
- [六、镜像安全扫描](#六镜像安全扫描)
- [七、密钥管理](#七密钥管理)
- [八、版本兼容矩阵](#八版本兼容矩阵)
- [九、安全架构选型](#九安全架构选型)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Falco** | 运行时安全监控 | Graduated | v0.41.0 | 7.5k+ | Apache-2.0 |
| **OPA** | 通用策略引擎 | Graduated | v1.3.0 | 9.5k+ | Apache-2.0 |
| **Kyverno** | K8s 原生策略管理 | Graduated | v1.14.0 | 5.5k+ | Apache-2.0 |
| **cert-manager** | 自动化 TLS 证书 | Graduated | v1.17.0 | 12.5k+ | Apache-2.0 |
| **SPIFFE/SPIRE** | 工作负载身份框架 | Graduated | v1.11.0 | 4k+ | Apache-2.0 |
| **TUF** | 软件更新安全框架 | Graduated | v4.0.0 | 3k+ | MIT/Apache-2.0 |
| **in-toto** | 软件供应链完整性 | Graduated | v3.0.0 | 1k+ | Apache-2.0 |
| **Vault** | 密钥与机密管理 | HashiCorp | v1.19.0 | 31k+ | BSL/Apache-2.0 |
| **Kubescape** | 合规扫描与风险评估 | Incubating | v3.0.30 | 10k+ | Apache-2.0 |
| **Notary** | 镜像内容信任 | Incubating | v2.0.0 | 3k+ | Apache-2.0 |
| **cosign** | 镜像签名 (Sigstore) | OpenSSF | v2.4.0 | 4k+ | Apache-2.0 |
| **Trivy** | 漏洞与合规扫描 | Aqua | v0.61.0 | 24k+ | Apache-2.0 |
| **External Secrets** | 外部密钥同步 | 非 CNCF | v0.15.0 | 4k+ | Apache-2.0 |
| **Sealed Secrets** | GitOps 加密密钥 | 非 CNCF | v0.28.0 | 7.5k+ | Apache-2.0 |
| **Snyk** | 安全扫描平台 | 商业 | - | - | 商业 |
| **Aqua Enterprise** | 企业容器安全 | 商业 | - | - | 商业 |
| **Sysdig** | 运行时安全与分析 | 商业 | - | - | 商业 |
| **SOPS** | YAML/JSON 加密 (GitOps) | Mozilla | v3.9.0 | 17k+ | MPL-2.0 |
| **Kubewarden** | Rust 编写 K8s 策略引擎 | Rancher | v1.23.0 | 2k+ | Apache-2.0 |
| **jsPolicy** | JavaScript K8s 策略引擎 | Loft | v0.3.0 | 500+ | Apache-2.0 |
| **NeuVector** | 容器安全平台 (SUSE) | SUSE | v5.4.0 | 3k+ | Apache-2.0 |
| **OPA Gatekeeper** | K8s 准入策略控制器 | OPA | v3.18.0 | 3.5k+ | Apache-2.0 |
| **Sigstore policy-controller** | K8s 签名策略验证 | Sigstore | v0.11.0 | 1k+ | Apache-2.0 |
| **Snyk** | 安全扫描平台 | Snyk | - | - | 商业 |
| **Checkmarx** | SAST/DAST/SCA | Checkmarx | - | - | 商业 |

---

## 二、运行时安全 (Runtime Security)

### 2.1 Falco (CNCF Graduated)

```yaml
# 核心特性
- 基于 eBPF 或内核模块的系统调用监控
- 规则引擎检测异常行为
- 丰富的默认规则库
- gRPC 输出与自定义插件
- Falco Sidekick 集成 (Slack/SQS/HTTP/Webhook)
- 支持容器、主机、K8s 审计日志
```

**检测能力**
- 特权容器启动
- 敏感目录挂载 (/etc, /proc, /sys)
- 反向 Shell、可疑网络连接
- K8s 审计事件 (exec, attach, port-forward)
- 敏感文件访问 (/etc/shadow, SSH keys)

**部署模式**
```yaml
# DaemonSet 模式 (推荐)
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
spec:
  template:
    spec:
      containers:
      - name: falco
        image: falcosecurity/falco:0.41.0
        securityContext:
          privileged: true
```

**GitHub**: https://github.com/falcosecurity/falco
**文档**: https://falco.org/docs/

### 2.2 Sysdig (商业开源)

- 基于 Falco 规则引擎的企业级扩展
- 自动基线学习与异常检测
- 取证与回溯能力
- 集成漏洞管理

---

## 三、策略与合规 (Policy & Compliance)

### 3.1 OPA / Gatekeeper (CNCF Graduated)

```yaml
# OPA 核心特性
- 通用策略引擎 (不限于 K8s)
- Rego 声明式策略语言
- 解耦策略决策与执行
- 支持 K8s Admission Control、Envoy、Terraform 等
```

**Gatekeeper** (OPA 的 K8s 集成)
- Validating/Mutating Webhook
- ConstraintTemplate + Constraint 资源模型
- 审计与 dry-run 模式

**GitHub OPA**: https://github.com/open-policy-agent/opa
**GitHub Gatekeeper**: https://github.com/open-policy-agent/gatekeeper

### 3.2 Kyverno (CNCF Graduated)

```yaml
# 核心特性
- 纯 K8s 原生 (YAML 策略，无需学习新语言)
- Validating、Mutating、Generating 策略
- 镜像验证 (cosign/Sigstore 集成)
- 清理策略 (CleanupPolicy)
- 策略报告 (PolicyReport)
- 与 Argo CD/Flux 集成
```

**vs OPA/Gatekeeper**

| 维度 | Kyverno | OPA/Gatekeeper |
|:---|:---|:---|
| 学习曲线 | 低 (YAML) | 高 (Rego) |
| 策略语言 | K8s 原生 YAML | Rego |
| 适用场景 | 纯 K8s 环境 | 多平台统一策略 |
| 变异能力 | 强 (JSON Patch) | 中等 |
| 性能 | 优秀 | 优秀 |
| 社区 | 快速增长 | 成熟庞大 |

**GitHub**: https://github.com/kyverno/kyverno
**文档**: https://kyverno.io/

### 3.3 Kubescape (CNCF Incubating)

- 基于 NSA/CISA K8s 加固指南的合规扫描
- CIS Benchmark 自动检查
- 漏洞扫描 (镜像+依赖)
- 网络策略生成建议
- SARIF 输出集成 DevSecOps 流水线

**GitHub**: https://github.com/kubescape/kubescape

---

## 四、身份与访问 (Identity & Access)

### 4.1 SPIFFE / SPIRE (CNCF Graduated)

```yaml
# SPIFFE: 安全身份框架标准
# SPIRE: SPIFFE 运行时实现

核心特性:
- 工作负载身份 (非网络身份)
- 自动 SVID (SPIFFE Verifiable Identity Document) 签发
- 多种节点证明 (K8s, AWS, GCP, Azure, Unix)
- 自动证书轮换
- 与 Envoy/Istio 集成 (mTLS)
```

**GitHub SPIRE**: https://github.com/spiffe/spire

### 4.2 Keycloak (CNCF Incubating)

- 开源身份与访问管理 (IAM)
- OIDC / SAML / OAuth 2.0 支持
- 多租户 Realm
- 2025 v26 新增: 组织支持、TLS 热重载、持久会话

**GitHub**: https://github.com/keycloak/keycloak

---

## 五、供应链安全 (Supply Chain)

### 5.1 Sigstore / cosign

```yaml
# 核心工具链
- cosign: 容器镜像签名与验证
- fulcio: 免费 OIDC 代码签名 CA
- rekor: 签名透明日志
- gitsign: Git 提交签名
```

**使用示例**
```bash
# 签名镜像
cosign sign --key cosign.key myregistry/myimage:latest

# K8s 中验证 (Kyverno/OPA 集成)
# 策略拒绝未签名镜像
```

**GitHub cosign**: https://github.com/sigstore/cosign

### 5.2 TUF / in-toto

- **TUF**: 框架防范软件更新攻击 (降档、无限冻结)
- **in-toto**: 记录和验证软件供应链步骤
- 集成: Docker Content Trust (Notary v1) → Notary v2

---

## 六、镜像安全扫描

### 6.1 Trivy (Aqua)

```yaml
# 扫描能力
- OS 包漏洞 (Alpine, Debian, RHEL, etc.)
- 语言依赖 (npm, pip, go mod, etc.)
- 基础设施即代码 (Terraform, Dockerfile, K8s YAML)
- 密钥检测 (Secret scanning)
- SBOM 生成 (CycloneDX, SPDX)
- 许可证合规
```

**CI/CD 集成**
```yaml
# GitHub Actions 示例
- name: Trivy Scan
  uses: aquasecurity/trivy-action@master
  with:
    image-ref: 'myimage:${{ github.sha }}'
    format: 'sarif'
    output: 'trivy-results.sarif'
```

**GitHub**: https://github.com/aquasecurity/trivy

### 6.2 Snyk / Aqua Enterprise

- Snyk: 开发者友好，IDE 集成，SAST/SCA/容器扫描
- Aqua: 企业级 CNAPP，运行时防护 + 供应链安全

---

## 七、密钥管理

### 7.1 Vault (HashiCorp)

```yaml
# 核心特性
- 动态密钥 (数据库凭证、云 IAM)
- 静态加密 (K/V v2)
- PKI 引擎 (自动 TLS)
- K8s 集成 (Vault Agent Injector)
- 密钥轮换与租赁管理
```

**License 注意**: Vault v1.15+ 核心功能采用 BSL (Business Source License)，建议评估 **OpenBao** (分叉) 或 **External Secrets** 作为纯开源替代。

### 7.2 External Secrets Operator

- 将外部 KMS/Secrets Manager 同步为 K8s Secret
- 支持: AWS Secrets Manager, GCP Secret Manager, Azure Key Vault, HashiCorp Vault, GitLab CI/CD Variables 等
- 避免在 Git 中存储敏感信息

**GitHub**: https://github.com/external-secrets/external-secrets

### 7.3 Sealed Secrets (Bitnami)

- 将 Secret 加密为 SealedSecret 资源
- 可安全存储在 Git 中
- 集群内控制器自动解密

**GitHub**: https://github.com/bitnami-labs/sealed-secrets

---

## 八、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Falco v0.41 | ✅ | ✅ | ✅ | eBPF probe 需内核 5.8+ |
| Kyverno v1.14 | ✅ | ✅ | ⚠️ 待验证 | 关注 webhook 超时 |
| OPA Gatekeeper v3.18 | ✅ | ✅ | ✅ | 与 K8s API 深度耦合 |
| cert-manager v1.17 | ✅ | ✅ | ✅ | 自动 ACME 证书 |
| SPIRE v1.11 | ✅ | ✅ | ✅ | 需 cert-manager 配合 |
| Kubescape v3.0 | ✅ | ✅ | ✅ | 离线扫描可用 |
| Trivy v0.61 | ✅ | ✅ | ✅ | 独立工具 |
| Vault v1.19 | ✅ | ✅ | ✅ | Agent Injector 兼容 |

---

## 九、安全架构选型

```
┌─────────────────────────────────────────────────────────────┐
│                 云原生安全分层架构推荐                         │
└─────────────────────────────────────────────────────────────┘

构建阶段 (Build)
  ├── Trivy / Snyk ──► 镜像漏洞扫描
  ├── cosign ──► 镜像签名
  ├── Syft ──► SBOM 生成
  └── Kubescape ──► 配置合规检查

部署阶段 (Deploy)
  ├── Kyverno / OPA Gatekeeper ──► 准入控制
  ├── cert-manager ──► 自动 TLS
  ├── Sealed Secrets / External Secrets ──► 密钥管理
  └── Notary ──► 镜像信任验证

运行阶段 (Run)
  ├── Falco ──► 运行时威胁检测
  ├── Falco Sidekick ──► 告警响应
  ├── Network Policies (Cilium/Calico) ──► 微分段
  └── Falco / Sysdig ──► 取证分析

身份与访问
  ├── SPIFFE/SPIRE ──► 工作负载 mTLS
  ├── Keycloak ──► 身份联邦
  └── Vault / External Secrets ──► 密钥生命周期

供应链
  ├── in-toto ──► 构建流程完整性
  ├── TUF ──► 更新安全
  └── Sigstore/Rekor ──► 透明日志
```

---

## 参考链接

- [Falco 官方文档](https://falco.org/docs/)
- [Kyverno 官方文档](https://kyverno.io/docs/)
- [OPA 官方文档](https://www.openpolicyagent.org/docs/)
- [cert-manager 官方文档](https://cert-manager.io/docs/)
- [Sigstore 官方文档](https://docs.sigstore.dev/)
- [CNCF 安全白皮书](https://github.com/cncf/tag-security/blob/main/security-whitepaper/v2/cloud-native-security-whitepaper.md)

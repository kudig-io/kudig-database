---
title: "容器镜像仓库 × 镜像扫描 × 准入控制"
summary: "Harbor/ECR/GCR 镜像仓库与 Trivy/Grype 漏洞扫描、Admission 策略联动，构建从构建到部署的镜像安全晋升流水线"
category: synthesis
tags:
- container-registry
- image-scanning
- admission-control
- harbor
- trivy
- vulnerability
- image-promotion
tier: supporting
sources:
- 实体/harbor.md
- 实体/trivy.md
- 概念/k8s-security-compliance.md
- 概念/ci-cd-pipeline-patterns.md
- 实体/kyverno.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 容器镜像仓库 × 镜像扫描 × 准入控制

## The Connection（为什么这两个领域交叉）

容器镜像是 Kubernetes 部署的基本单元——所有运行在集群中的代码都打包为镜像。镜像的安全性直接决定了集群的安全性：一个包含已知 CVE 的基础镜像、一个被注入恶意代码的应用镜像、一个来源不明的第三方镜像，都可能导致生产环境被攻破。

镜像安全需要在三个环节形成闭环：仓库（Registry）负责镜像的存储、分发和访问控制；扫描（Scanning）负责发现镜像中的已知漏洞和恶意内容；准入控制（Admission Control）负责在部署时阻止不合规镜像进入集群。三者缺一不可——只有仓库没有扫描等于"不知道镜像里有什么"；只有扫描没有准入等于"知道有问题但不阻止"；只有准入没有扫描等于"想阻止但不知道阻止什么"。

Harbor 作为企业级镜像仓库，原生集成了 Trivy 扫描和准入策略（阻止未扫描/高漏洞镜像拉取）。ECR/GCR 提供云原生的扫描和策略能力。Trivy/Grype 作为独立扫描引擎可集成到 CI/CD 任何阶段。Kyverno/Gatekeeper 在 K8s Admission 层做最后的部署拦截。

## Where They Co-occur（生产中的交叉场景）

### 场景一：CI 阶段镜像扫描

开发者推送代码 → CI 构建镜像 → Trivy 扫描镜像 → 发现 Critical CVE → 构建失败/告警。在镜像进入 Registry 之前就拦截已知漏洞，避免"先部署再修复"的被动模式。扫描结果作为构建产物归档，供审计查询。

### 场景二：Registry 准入策略

Harbor 配置"仅允许已扫描且无 Critical CVE 的镜像被拉取"。即使 CI 扫描被绕过（如手动推送镜像），Registry 层仍然拦截。ECR 的 Lifecycle Policy 自动清理过期/未扫描镜像。GCR 的 Binary Authorization 要求镜像有有效签名才能部署。

### 场景三：运行中镜像持续扫描

镜像部署时安全，但新 CVE 每天都被发现。Trivy Operator 作为 K8s Operator 持续扫描运行中的镜像，发现新 CVE 时告警。不需要重新部署就能知道"哪些运行中的服务受影响"。

### 场景四：镜像晋升流水线

镜像从 dev → staging → production 的晋升不是简单的"复制镜像"，而是安全等级的逐步提升：dev 环境允许未扫描镜像；staging 要求扫描通过（无 Critical）；production 要求扫描通过 + 签名验证 + SBOM 存在。每个环境的准入策略不同。

### 场景五：基础镜像治理

组织有数百个微服务，基础镜像（如 `node:18-alpine`）的 CVE 影响所有下游服务。集中管理基础镜像：安全团队维护"黄金基础镜像"（已加固、已扫描、已签名），所有业务镜像必须基于黄金基础镜像构建。准入策略检查基础镜像来源。

### 场景六：多架构镜像管理

ARM64 + AMD64 多架构镜像（manifest list）需要每个架构都扫描。Harbor 支持多架构镜像的独立扫描和策略。准入策略需要验证所有架构变体都通过扫描。

## Production Patterns（生产模式与架构）

### 模式一：镜像安全晋升流水线

```
┌─────────────────────────────────────────────────────────┐
│  Image Security Promotion Pipeline                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Stage 1: Build (CI)                                    │
│  ├── 构建镜像 (BuildKit/Buildpacks)                    │
│  ├── Trivy 扫描 (Critical/High CVE 检查)              │
│  ├── SBOM 生成 (Syft)                                 │
│  ├── 签名 (Cosign)                                    │
│  └── 推送到 Dev Registry                               │
│  准入: 无 Critical CVE                                  │
│                                                         │
│  Stage 2: Dev → Staging 晋升                           │
│  ├── 自动化测试通过                                    │
│  ├── 扫描结果复核 (无新增 Critical)                    │
│  ├── 镜像复制到 Staging Registry                       │
│  └── Staging 准入: 扫描通过 + 签名有效                 │
│                                                         │
│  Stage 3: Staging → Production 晋升                    │
│  ├── 集成测试 + 性能测试通过                           │
│  ├── 安全团队审批 (手动或自动)                         │
│  ├── 镜像复制到 Production Registry                    │
│  ├── 追加 Production 签名                              │
│  └── Production 准入: 扫描 + 签名 + SBOM + 审批       │
│                                                         │
│  Stage 4: Runtime (持续)                                │
│  ├── Trivy Operator 持续扫描运行中镜像                 │
│  ├── 新 CVE 发现 → 告警 + 评估影响                    │
│  ├── Critical CVE → 7 天内修复/缓解                    │
│  └── 过期镜像自动清理 (Lifecycle Policy)               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：Harbor 安全配置

```yaml
# Harbor 项目级安全策略
# 通过 Harbor API 或 UI 配置:

# 1. 漏洞扫描策略
scan_policy:
  auto_scan: true  # 推送时自动扫描
  scan_on_pull: false  # 拉取时不扫描 (性能)
  vulnerability_severity_threshold: "high"  # 阻止 High+ 拉取
  prevent_vulnerable: true

# 2. 镜像签名策略
signature_policy:
  require_signature: true  # 仅允许签名镜像
  trusted_keys:
  - "cosign-public-key-prod"

# 3. 镜像保留策略
retention_policy:
  rules:
  - tag_filter: "*"
    days_since_push: 90  # 90 天后清理
    keep_latest: 10  # 保留最近 10 个
  - tag_filter: "v*"  # 版本标签永久保留
    days_since_push: -1

# 4. 复制策略 (多 Registry 同步)
replication_policy:
  - name: "prod-to-dr"
    source: "harbor-prod"
    target: "harbor-dr"
    trigger: "event_based"  # 推送时自动复制
    include_signatures: true  # 签名一起复制
```

### 模式三：Trivy 扫描集成

```yaml
# CI 阶段扫描 (GitHub Actions)
- name: Trivy Scan
  uses: aquasecurity/trivy-action@master
  with:
    image-ref: 'harbor.internal.com/myapp:${{ github.sha }}'
    format: 'sarif'
    output: 'trivy-results.sarif'
    severity: 'CRITICAL,HIGH'
    exit-code: '1'  # Critical/High 时构建失败
    ignore-unfixed: true  # 忽略无修复方案的 CVE

---
# K8s 运行中扫描 (Trivy Operator)
apiVersion: aquasecurity.github.io/v1alpha1
kind: ClusterVulnerabilityReport
# Trivy Operator 自动创建，包含:
# - 镜像名称和 tag
# - CVE 列表 (ID, 严重性, 包名, 版本, 修复版本)
# - 扫描时间

# Prometheus 告警:
# 运行中镜像存在 Critical CVE
trivy_image_vulnerabilities{severity="Critical"} > 0
```

### 模式四：Kyverno 准入验证

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-image-registries
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: validate-registry
    match:
      any:
      - resources:
          kinds:
          - Pod
          namespaces:
          - production
          - staging
    validate:
      message: "镜像必须来自受信任的 Registry"
      pattern:
        spec:
          containers:
          - image: "harbor.internal.com/* | *.dkr.ecr.*.amazonaws.com/*"
---
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-image-scan
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-scan-annotation
    match:
      any:
      - resources:
          kinds:
          - Pod
          namespaces:
          - production
    validate:
      message: "镜像必须有有效的扫描通过注解"
      pattern:
        metadata:
          annotations:
            security.scan/status: "passed"
            security.scan/timestamp: "?*"  # 必须存在
```

### 模式五：镜像仓库高可用

```
生产 Registry 架构:

  方案 1: Harbor HA
  ├── Harbor 多副本 (≥3) + 外部 PostgreSQL/Redis
  ├── 存储后端: S3/OSS (对象存储)
  ├── 前端: LB (Nginx/ALB)
  └── 跨区复制: 主 Registry → 灾备 Registry

  方案 2: 云原生 (ECR/GCR/ACR)
  ├── 云厂商托管 (SLA 99.9%+)
  ├── 跨区域复制 (内置)
  └── 与 IAM 集成 (细粒度访问控制)

  方案 3: 混合
  ├── 内部 Harbor (开发/测试)
  ├── 云 ECR/GCR (生产)
  └── 同步策略: 晋升时跨 Registry 复制

  关键配置:
  ├── 镜像拉取缓存: 节点级 (containerd mirror)
  ├── 拉取限流: 避免 Registry 过载
  └── 离线预案: 关键镜像预加载到节点
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | Harbor (自建) | ECR (AWS) | GCR/Artifact Registry (GCP) | ACR (Azure) |
|------|-------------|-----------|----------------------------|-------------|
| 部署方式 | 自建 (K8s/VM) | 托管 | 托管 | 托管 |
| 扫描引擎 | Trivy (内置) | 内置 (Clair) | 内置 (Container Analysis) | 内置 (Qualys) |
| 签名 | Cosign/Notary | 内置 | 内置 (Binary Auth) | 内置 |
| 多区域复制 | 支持 | 支持 | 支持 | 支持 |
| RBAC | 项目级 (细粒度) | IAM | IAM | IAM |
| 成本 | 运维成本 | 按镜像/拉取计费 | 按存储/拉取计费 | 按存储/拉取计费 |
| 适用场景 | 多云/私有云 | AWS 生态 | GCP 生态 | Azure 生态 |
| 运维复杂度 | 高 | 低 | 低 | 低 |

### 扫描工具对比

| 维度 | Trivy | Grype | Clair | Snyk |
|------|-------|-------|-------|------|
| 扫描速度 | 快 | 快 | 中 | 中 |
| 漏洞库 | NVD + 多发行版 | NVD + 多发行版 | NVD + 多发行版 | 专有 + NVD |
| 语言依赖 | 支持 | 支持 | 有限 | 支持 |
| SBOM | 支持 (Syft) | 支持 | 不支持 | 支持 |
| K8s 集成 | Operator | CLI | Harbor 内置 | CLI |
| 误报率 | 低 | 低 | 中 | 低 |
| 成本 | 开源免费 | 开源免费 | 开源免费 | SaaS 付费 |

## Anti-patterns & Pitfalls（反模式）

### 反模式一：扫描但不阻止

CI 中运行 Trivy 扫描，但结果只是"打印日志"，不阻止构建。Critical CVE 镜像照常部署。**正确做法**：`exit-code: 1`（扫描失败则构建失败）；Registry 层阻止高漏洞镜像拉取；Admission 层最后兜底。

### 反模式二：只扫描应用层不扫描基础镜像

只关注应用代码的依赖漏洞，忽略基础镜像（OS 包）的 CVE。`openssl`、`glibc` 等系统库的 Critical CVE 同样致命。**正确做法**：全层扫描（OS 包 + 语言依赖 + 二进制文件）；使用最小基础镜像（distroless/alpine）减少攻击面。

### 反模式三：`latest` 标签部署

生产环境使用 `image: myapp:latest`。无法确定实际运行的是哪个版本，扫描结果与实际镜像不对应，回滚不可能。**正确做法**：生产环境必须使用不可变标签（Git SHA 或语义版本）；准入策略禁止 `latest` 标签。

### 反模式四：扫描结果不跟踪修复

扫描发现 50 个 CVE，生成报告后无人跟进。下次扫描还是 50 个（甚至更多）。**正确做法**：Critical CVE → 7 天修复 SLA；High CVE → 30 天修复 SLA；自动创建 JIRA 并跟踪；定期审查未修复 CVE 清单。

### 反模式五：Registry 凭证硬编码

镜像拉取凭证（dockerconfigjson）硬编码在 Deployment YAML 或 Git 仓库中。凭证泄露后攻击者可推送恶意镜像。**正确做法**：使用 K8s ServiceAccount + 云 IAM 角色（IRSA/Workload Identity）；或 Vault 动态凭证；定期轮换。

### 反模式六：忽略镜像构建供应链

只扫描最终镜像，不验证构建过程。攻击者可以在构建阶段注入恶意代码（即使最终镜像"无 CVE"）。**正确做法**：SLSA Provenance 验证构建来源；可复现构建（Reproducible Builds）；构建日志归档审计。

## Operational Checklist（运维检查清单）

### Registry 配置

- [ ] 启用自动扫描（推送时触发）
- [ ] 配置拉取策略（阻止未扫描/高漏洞镜像）
- [ ] 配置保留策略（自动清理过期镜像）
- [ ] 启用访问日志（谁在什么时间拉取/推送了什么）
- [ ] 配置跨区复制（灾备）
- [ ] RBAC 最小权限（开发者只能推送自己项目的镜像）

### CI/CD 集成

- [ ] 构建后 Trivy 扫描（Critical/High 阻断）
- [ ] SBOM 生成和附加
- [ ] 镜像签名（Cosign）
- [ ] 扫描结果归档（审计证据）
- [ ] 基础镜像定期更新（每周重建）

### K8s 准入控制

- [ ] 限制镜像来源（受信任 Registry 白名单）
- [ ] 禁止 `latest` 标签（生产环境）
- [ ] 验证镜像签名（Kyverno verifyImages）
- [ ] 先 Audit 后 Enforce（避免误拦截）

### 运行监控

- [ ] Trivy Operator 持续扫描运行中镜像
- [ ] 新 Critical CVE 告警（24h 内通知）
- [ ] 镜像年龄监控（> 90 天未更新告警）
- [ ] Registry 可用性监控（拉取失败率）
- [ ] 未修复 CVE 跟踪面板

## Related

- [[实体/harbor.md|Harbor]]
- [[实体/trivy.md|Trivy]]
- [[实体/kyverno.md|Kyverno]]
- [[概念/k8s-security-compliance.md|K8s 安全合规]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD 流水线模式]]
- [[综合/sigstore-cosign-supply-chain.md|Sigstore × 供应链安全]]
- [[综合/opa-kyverno-policy-as-code.md|OPA × Kyverno × Policy-as-Code]]
- [[综合/compliance-k8s-soc2-hipaa.md|合规 × K8s × SOC2/HIPAA]]

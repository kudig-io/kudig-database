---
title: K8S 安全与合规
summary: K8S 安全与合规：Kubernetes 安全涵盖供应链、准入控制、运行时、网络、密钥管理等多个层面。本文梳理 K8S 安全生态的核心工具链与最佳实践。
category: concepts
tags:
- security
- sigstore
- kyverno
- falco
- cilium
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8S 安全与合规

Kubernetes 安全涵盖供应链、准入控制、运行时、网络、密钥管理等多个层面。本文梳理 K8S 安全生态的核心工具链与最佳实践。

## 供应链安全

从代码到镜像的全链路信任链构建：

### Sigstore / Cosign — 无钥签名

Sigstore 是 CNCF 毕业项目，提供无钥签名基础设施：

| 组件 | 功能 |
|------|------|
| **Cosign** | 容器镜像签名/验证，支持无钥 (Keyless) 模式 |
| **Fulcio** | 基于 OIDC 身份签发短期证书 |
| **Rekor** | 不可篡改的签名透明日志 (Sigstore Log) |
| **Gitsign** | Git Commit 签名 |

```bash
# 无钥签名（基于 OIDC 身份）
cosign sign --yes ghcr.io/org/image:tag

# 验证签名
cosign verify --certificate-identity=user@org.com \
  --certificate-oidc-issuer=https://accounts.google.com \
  ghcr.io/org/image:tag
```

### SLSA v1.0 (Supply-chain Levels for Software Artifacts)

SLSA 定义了供应链安全等级：

| 级别 | 要求 | 对应工具 |
|------|------|----------|
| **L1** | 构建过程有出处 (Provenance) | GitHub Actions |
| **L2** | 签名的 Provenance | Sigstore + SLSA Generator |
| **L3** | 隔离的构建环境 | Tekton + isolated-builder |
| **L4** | 双人复核 + 可重现构建 | 需要额外流程 |

### In-toto + TUF

- **In-toto** — 定义供应链布局 (Layout)，确保每个步骤未被篡改
- **TUF (The Update Framework)** — 安全的软件更新分发，防中间人攻击

```yaml
# In-toto 布局示例
steps:
- name: build
  expected_materials: [".", "src/"]
  expected_products: ["app.bin"]
  pubkeys: [builder_key]
- name: sign
  expected_materials: ["app.bin"]
  expected_products: ["app.bin.sig"]
  pubkeys: [signer_key]
```

## 策略引擎

### Kyverno vs OPA Gatekeeper vs ValidatingAdmissionPolicy

| 特性 | Kyverno | OPA Gatekeeper | VAP (CEL) |
|------|---------|----------------|-----------|
| **CNCF 状态** | Incubating | Graduated | K8S 内置 |
| **策略语言** | YAML 原生 | Rego | CEL |
| **学习曲线** | 低 | 中高 | 中 |
| **Mutation** | 支持 | 有限 | 不支持 |
| **v1.30+ GA** | — | — | ✅ |

#### Kyverno — YAML 原生策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-labels
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-labels
    match:
      any:
      - resources:
          kinds: ["Pod"]
    validate:
      message: "必须包含 app 标签"
      pattern:
        metadata:
          labels:
            app: "?*"
```

#### ValidatingAdmissionPolicy (CEL GA v1.30)

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: restrict-images
spec:
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: |
      object.spec.containers.all(c,
        c.image.startsWith('ghcr.io/org/')
      )
    message: "只能使用组织镜像仓库"
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: restrict-images-binding
spec:
  policyName: restrict-images
  validationActions: [Deny, Audit]
```

## 运行时安全

### Falco + Talon

[[Falco]] 是 CNCF 毕业的运行时威胁检测工具：

**Falco** — 检测异常行为：
- 系统调用监控 (基于内核模块/eBPF)
- 容器逃逸检测
- 敏感文件访问告警
- 异常网络连接

**Talon** — Falco 的自动响应引擎：
- 自动驱逐恶意 Pod
- 阻断网络连接
- 执行自定义响应脚本

```yaml
# Falco 规则示例
- rule: Terminal shell in container
  desc: 检测容器内启动的 Shell
  condition: >
    spawned_process and container and
    proc.name in (bash, sh, zsh) and
    not proc.pname in (falco, healthcheck)
  output: >
    容器内 Shell 启动
    (user=%user.name container=%container.name shell=%proc.name)
  priority: WARNING
```

### Tetragon — eBPF 强制执行

[[Tetragon]] (Cilium 生态) 提供 eBPF 级别的安全强制：

| 能力 | 说明 |
|------|------|
| **进程执行控制** | 阻止未授权的二进制执行 |
| **文件访问控制** | 限制敏感文件读写 |
| **网络策略强制** | 内核级网络过滤 |
| **特权提升检测** | 阻止提权操作 |

```yaml
# Tetragon TracingPolicy — 阻止 /etc/shadow 访问
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: deny-shadow-access
spec:
  kprobes:
  - call: "security_file_open"
    selectors:
    - matchArgs:
      - index: 0
        operator: "Prefix"
        values: ["/etc/shadow"]
    - matchActions:
      - action: Sigkill
```

### KubeArmor

[[KubeArmor]] 提供基于 LSM (Linux Security Modules) 的强制执行：
- AppArmor / SELinux / BPF-LSM 支持
- 声明式安全策略
- 与 K8S 原生集成

## 密钥管理

### External Secrets Operator (ESO)

ESO 是 CNCF Incubating 项目，将外部密钥管理系统与 K8S 同步：

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: app-secret
  data:
  - secretKey: db-password
    remoteRef:
      key: secret/data/myapp
      property: db_password
```

支持的后端：
- HashiCorp Vault、AWS SM、GCP SM、Azure KV
- Akeyless、CyberArk、IBM SM

### 其他密钥方案

| 方案 | 原理 | 适用场景 |
|------|------|----------|
| **Sealed Secrets** | 加密后的 K8S Secret，控制器解密 | GitOps 友好 |
| **Vault Agent Injector** | Sidecar 注入 Vault Agent | 复杂密钥轮换 |
| **SOPS + Age** | 文件级加密 | 配置文件加密 |

## 网络安全

### Cilium — L7 网络策略 + Hubble

[[Cilium]] 是 CNCF 毕业的 eBPF 网络方案：

```yaml
# Cilium L7 网络策略 — 只允许 GET /api/*
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-l7-policy
spec:
  endpointSelector:
    matchLabels:
      app: api-server
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
      rules:
        http:
        - method: GET
          path: "/api/.*"
```

**Hubble** — Cilium 可观测性平台：
- 网络流量可视化
- Service Map 拓扑图
- DNS/HTTP/gRPC 请求追踪
- 与 Prometheus/Grafana 集成

### Calico — GlobalNetworkPolicy

```yaml
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: deny-all-default
spec:
  selector: all()
  types: [Ingress, Egress]
  ingress:
  - action: Deny
  egress:
  - action: Deny
```

## Pod Security Standards

PSS (Pod Security Standards) + PSA (Pod Security Admission) 已完全替代 PSP (Pod Security Policy, v1.25 移除)：

### 三个安全级别

| 级别 | 说明 | 适用场景 |
|------|------|----------|
| **Privileged** | 不受限 | 系统组件、CNI 插件 |
| **Baseline** | 最低限制 | 一般工作负载 |
| **Restricted** | 严格限制 | 生产环境推荐 |

### PSA 配置

```yaml
# 命名空间级别启用
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/audit: restricted
```

**Restricted 级别要求：**
- 必须以非 root 运行
- 不允许特权提升
- 必须设置 seccompProfile
- 不允许 hostNetwork/hostPID/hostIPC
- 必须设置安全的 volume 类型

## CIS Benchmarks

### kube-bench — 自动化安全扫描

kube-bench 实现 CIS Kubernetes Benchmark 自动检测：

```bash
# 运行 CIS 基准扫描
kube-bench run --targets master,node

# 输出示例
[INFO] 1 Control Plane Security Configuration
[PASS] 1.1.1 Ensure that the API server pod specification file permissions are set to 644 or more restrictive
[FAIL] 1.1.12 Ensure that the etcd data directory ownership is set to etcd:etcd
[WARN] 1.2.1 Ensure that the --anonymous-auth argument is set to false
```

CIS Benchmark 主要检查项：
- **控制平面安全** — API Server、etcd、Controller Manager 配置
- **工作节点安全** — kubelet 配置、文件权限
- **Pod 安全** — 容器运行时配置
- **网络策略** — CNI 配置、NetworkPolicy 使用
- **RBAC** — 权限审计、最小权限原则

## 安全架构总览

```
开发阶段                    部署阶段                    运行时
┌──────────────┐       ┌──────────────┐       ┌──────────────┐
│ Gitsign      │       │ Kyverno/VAP  │       │ Falco/Talon  │
│ SLSA Provenance│      │ PSA/PSS      │       │ Tetragon     │
│ Cosign 签名  │       │ kube-bench   │       │ KubeArmor    │
│ In-toto      │       │ ESO/Sealed   │       │ Cilium L7    │
└──────────────┘       └──────────────┘       └──────────────┘
     供应链                    准入控制               运行时防护
```

## 相关概念

- [[Cilium]] — eBPF 网络与安全
- [[Kyverno]] — YAML 原生策略引擎
- [[Falco]] — 运行时威胁检测
- Sigstore — 无钥签名体系
- Pod Security Standards — Pod 安全标准
- RBAC — 基于角色的访问控制

## Related

- [[概念/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[概念/k8s-networking-evolution.md|k8s networking evolution]] — K8S 网络技术演进
- [[概念/container-runtime-evolution.md|container runtime evolution]] — 容器运行时演进


<!-- risk-assessed -->

---
title: 08 - 安全最佳实践表
description: '# 08 - 安全最佳实践表'
summary: 'kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- etcd
- apiserver
- istio
- cilium
- calico
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 安全最佳实践表 是什么
- 如何 安全最佳实践表
- Kubernetes 7 security 最佳实践
trigger_keywords:
- 安全最佳实践表
- security
prerequisites:
- kubectl-basics
- rbac-basics
- service-mesh-basics
- cilium-basics
- cni-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 08 - 安全最佳实践表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/docs/concepts/security](https://kubernetes.io/docs/concepts/security/)

<!-- chunk: Pod安全标准(Pod Securityod Security Standards]]) -->
## Pod安全标准(Pod Security Standards)

| 安全级别 | 描述 | 限制内容 | 适用场景 | 版本支持 | 实施方式 |
|---------|------|---------|---------|---------|---------|
| **Privileged** | 无限制，完全开放 | 无 | 系统组件，信任的工作负载 | v1.23+ | `pod-security.kubernetes.io/enforce: privileged` |
| **Baseline** | 最低限度限制 | 禁止hostNetwork/hostPID/hostIPC等特权 | 大多数应用 | v1.23+ | `pod-security.kubernetes.io/enforce: baseline` |
| **Restricted** | 严格限制，最佳实践 | 必须非root，只读根文件系统，限制capabilities | 安全敏感应用 | v1.23+ | `pod-security.kubernetes.io/enforce: restricted` |

<!-- chunk: Pod安全准入(PSA)配置 -->
## Pod安全准入(PSA)配置

| 配置项 | 用途 | 标签格式 | 示例值 | 版本变更 | 生产建议 |
|-------|------|---------|-------|---------|---------|
| **enforce** | 强制执行，违反则拒绝 | `pod-security.kubernetes.io/enforce` | privileged/baseline/restricted | v1.25+ GA | 生产命名空间使用 |
| **enforce-version** | 强制执行的版本 | `pod-security.kubernetes.io/enforce-version` | latest/v1.28 | v1.25+ | 指定版本避免升级影响 |
| **audit** | 审计模式，记录违规 | `pod-security.kubernetes.io/audit` | baseline/restricted | v1.25+ | 迁移时启用审计 |
| **audit-version** | 审计的版本 | `pod-security.kubernetes.io/audit-version` | latest/v1.28 | v1.25+ | 与enforce配合 |
| **warn** | 警告模式，提示用户 | `pod-security.kubernetes.io/warn` | baseline/restricted | v1.25+ | 开发环境使用 |
| **warn-version** | 警告的版本 | `pod-security.kubernetes.io/warn-version` | latest/v1.28 | v1.25+ | 与warn配合 |

<!-- chunk: RBAC最佳实践 -->
## RBAC最佳实践

| 实践领域 | 最佳实践 | 实施步骤 | 版本强制 | 风险说明 | 审计方法 |
|---------|---------|---------|---------|---------|---------|
| **最小权限原则** | 仅授予必需权限 | 使用Role而非ClusterRole，限制verbs | 稳定 | 权限过大导致横向移动 | `kubectl auth can-i --list` |
| **服务账户隔离** | 每个工作负载独立SA | 不使用default SA，创建专用SA | 稳定 | 共享SA导致权限泄露 | 检查Pod的serviceAccountName |
| **限制cluster-admin** | 避免过度使用 | 仅限紧急操作，日常使用受限角色 | 稳定 | 完全控制集群 | 审计ClusterRoleBinding |
| **禁用自动挂载Token** | 不需要API访问时禁用 | `automountServiceAccountToken: false` | 稳定 | Token泄露风险 | 检查Pod挂载 |
| **使用聚合ClusterRole** | 简化角色管理 | 定义聚合规则，组合权限 | 稳定 | 便于审计 | 检查aggregationRule |
| **定期审计权限** | 发现过度授权 | 使用工具如rbac-police | 稳定 | 权限膨胀 | 定期执行审计脚本 |

<!-- chunk: Secrets管理 -->
## Secrets管理

| 实践领域 | 最佳实践 | 实施步骤 | 版本要求 | 安全影响 | ACK集成 |
|---------|---------|---------|---------|---------|---------|
| **etcd加密** | 加密存储的Secrets | 配置EncryptionConfiguration | v1.13+ | 防止etcd数据泄露 | 托管版自动加密 |
| **外部Secret管理** | 使用Vault/KMS | 部署External [[secrets\|Secrets]] Operator | 外部工具 | 集中管理，审计追踪 | 阿里云KMS集成 |
| **限制Secret访问** | RBAC控制 | 仅授权必需的SA访问 | 稳定 | 防止未授权访问 | RAM策略集成 |
| **Secret轮换** | 定期更新凭证 | 自动化轮换流程 | 外部工具 | 限制泄露影响 | KMS自动轮换 |
| **避免环境变量** | 使用卷挂载 | `secretKeyRef`改为`volumeMounts` | 稳定 | 环境变量易泄露 | - |
| **审计Secret访问** | 记录访问日志 | 启用API审计日志 | v1.29增强 | 追踪访问行为 | SLS审计集成 |

<!-- chunk: 网络安全 -->
## 网络安全

| 实践领域 | 最佳实践 | 实施步骤 | 版本要求 | 风险说明 | 工具支持 |
|---------|---------|---------|---------|---------|---------|
| **默认拒绝策略** | 默认禁止所有流量 | 创建deny-all [[networkpolicy\|NetworkPolicy]] | 稳定 | 未授权访问 | Calico/Cilium |
| **命名空间隔离** | 限制跨NS通信 | 配置NS级NetworkPolicy | 稳定 | 横向移动 | CNI支持 |
| **出站流量控制** | 限制Egress | 白名单外部访问 | 稳定 | 数据外泄 | Egress NetworkPolicy |
| **mTLS** | 加密服务间通信 | 部署Service Mesh | 外部工具 | 中间人攻击 | Istio/Linkerd |
| **API Server访问限制** | 限制源IP | 配置--api-server-allow-cidrs | 云平台 | 未授权API访问 | ACK白名单 |

<!-- chunk: 容器安全 -->
## 容器安全

| 实践领域 | 最佳实践 | 实施步骤 | 版本要求 | 风险说明 | 验证方法 |
|---------|---------|---------|---------|---------|---------|
| **非root运行** | runAsNonRoot: true | Pod SecurityContext配置 | 稳定 | 容器逃逸风险 | PSA restricted |
| **只读根文件系统** | readOnlyRootFilesystem: true | 配置emptyDir用于写入 | 稳定 | 恶意文件写入 | PSA restricted |
| **禁止特权容器** | privileged: false | SecurityContext配置 | 稳定 | 完全主机访问 | PSA baseline |
| **限制Capabilities** | drop ALL，仅添加必需 | 配置capabilities | 稳定 | 权限提升 | PSA restricted |
| **禁止主机命名空间** | hostNetwork/PID/IPC: false | Pod spec配置 | 稳定 | 主机级访问 | PSA baseline |
| **镜像签名验证** | 验证镜像来源 | 配置ImagePolicyWebhook | v1.28增强 | 供应链攻击 | Sigstore/Cosign |
| **漏洞扫描** | 扫描已知CVE | CI集成Trivy/Clair | 外部工具 | 已知漏洞利用 | ACR扫描 |

<!-- chunk: 审计配置 -->
## 审计配置

| 审计级别 | 记录内容 | 适用资源 | 性能影响 | 存储需求 |
|---------|---------|---------|---------|---------|
| **None** | 不记录 | 健康检查等 | 无 | 无 |
| **Metadata** | 请求元数据 | 大多数资源 | 低 | 中等 |
| **Request** | 元数据+请求体 | 敏感资源 | 中 | 较高 |
| **RequestResponse** | 元数据+请求+响应 | 关键资源 | 高 | 很高 |

```yaml
# 审计策略示例
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 不记录健康检查
  - level: None
    users: ["system:kube-proxy"]
    verbs: ["watch"]
    resources:
      - group: ""
        resources: ["endpoints", "services", "services/status"]
  # Secrets访问记录Request级别
  - level: Request
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
  # 其他记录Metadata
  - level: Metadata
    omitStages:
      - "RequestReceived"
```

<!-- chunk: 安全检查清单 -->
## 安全检查清单

| 检查项 | 命令/方法 | 期望结果 | 优先级 |
|-------|---------|---------|-------|
| 匿名认证禁用 | 检查apiserver `--anonymous-auth=false` | false | P0 |
| RBAC启用 | `kubectl api-versions | grep rbac` | 存在 | P0 |
| etcd加密 | 检查EncryptionConfiguration | 已配置 | P0 |
| Pod Security启用 | 检查NS标签 | 已配置 | P1 |
| 审计日志启用 | 检查apiserver `--audit-log-path` | 已配置 | P1 |
| NetworkPolicy存在 | `kubectl get networkpolicy -A` | 存在策略 | P1 |
| 默认SA无特权 | 检查default SA绑定 | 无cluster-admin | P1 |
| Secrets非明文 | `etcdctl get /registry/secrets` | 加密 | P0 |
| 镜像来自可信仓库 | 检查Pod镜像源 | 私有仓库 | P2 |
| 资源限制配置 | `kubectl describe ns` | 存在LimitRange | P2 |

<!-- chunk: CIS Kubernetes Benchmark检查 -->
## CIS Kubernetes Benchmark检查

| CIS编号 | 检查项 | 自动化工具 | 版本变更 |
|--------|-------|-----------|---------|
| 1.1.x | API Server文件权限 | kube-bench | 稳定 |
| 1.2.x | API Server参数 | kube-bench | v1.25+ PSA相关 |
| 2.x | etcd配置 | kube-bench | 稳定 |
| 3.x | 控制平面配置 | kube-bench | 稳定 |
| 4.x | Worker节点 | kube-bench | v1.24+ CRI相关 |
| 5.x | Policies | kube-bench | v1.25+ PSA替代PSP |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 运行kube-bench
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml
kubectl logs -f job/kube-bench
```
---

## 供应链安全

| 实践领域 | 最佳实践 | 实施工具 | 优先级 | 说明 |
|---------|---------|---------|--------|------|
| **镜像签名** | 所有镜像必须签名 | Cosign/Notary | P0 | 防止镜像篡改 |
| **镜像扫描** | CI 中自动扫描漏洞 | Trivy/Grype | P0 | 阻断高危漏洞 |
| **基础镜像** | 使用最小化基础镜像 | distroless/alpine | P1 | 减少攻击面 |
| **SBOM** | 生成软件物料清单 | Syft/cosign | P1 | 依赖追溯 |
| **构建可重现** | 固定依赖版本 | go.sum/package-lock | P1 | 防止供应链注入 |
| **私有 Registry** | 不直接拉取公网镜像 | Harbor/ECR/ACR | P0 | 控制镜像来源 |
| **准入验证** | 拒绝未签名镜像 | Kyverno/cosign-webhook | P0 | 强制签名验证 |

### 镜像签名与验证

```bash
# 签名镜像
cosign sign --key cosign.key registry.example.com/app:v1.0.0

# 验证签名
cosign verify --key cosign.pub registry.example.com/app:v1.0.0

# Kyverno 策略：拒绝未签名镜像
# apiVersion: kyverno.io/v1
# kind: ClusterPolicy
# metadata:
#   name: verify-image-signature
# spec:
#   validationFailureAction: Enforce
#   rules:
#     - name: verify-signature
#       match:
#         resources:
#           kinds: ["Pod"]
#       verifyImages:
#         - imageReferences: ["registry.example.com/*"]
#           attestors:
#             - entries:
#                 - keys:
#                     publicKeys: |-
#                       -----BEGIN PUBLIC KEY-----
#                       ...
#                       -----END PUBLIC KEY-----
```

## 运行时安全

| 实践领域 | 最佳实践 | 实施工具 | 优先级 | 说明 |
|---------|---------|---------|--------|------|
| **只读根文件系统** | readOnlyRootFilesystem: true | PSA Restricted | P0 | 防止写入恶意文件 |
| **禁止特权容器** | privileged: false | PSA/Kyverno | P0 | 防止容器逃逸 |
| **限制 Capabilities** | drop ALL, add 必要 | PSA Restricted | P0 | 最小内核权限 |
| **Seccomp** | RuntimeDefault profile | PSA | P1 | 限制系统调用 |
| **运行时检测** | 异常行为检测 | Falco/Tetragon | P1 | 实时威胁发现 |
| **文件完整性** | 关键文件监控 | AIDE/Tetragon | P2 | 篡改检测 |

### Falco 运行时检测规则

```yaml
# 自定义 Falco 规则
- rule: Detect Shell in Container
  desc: 检测容器内启动 Shell
  condition: >
    spawned_process and container and
    proc.name in (bash, sh, zsh, csh)
  output: >
    Shell opened in container
    (user=%user.name container=%container.name
     shell=%proc.name parent=%proc.pname
     cmdline=%proc.cmdline)
  priority: WARNING
  tags: [container, shell, mitre_execution]

- rule: Detect Sensitive File Access
  desc: 检测访问敏感文件
  condition: >
    open_read and container and
    fd.name in (/etc/shadow, /etc/passwd, /root/.ssh/*)
  output: >
    Sensitive file accessed in container
    (user=%user.name file=%fd.name container=%container.name)
  priority: CRITICAL
  tags: [container, filesystem, mitre_credential_access]

- rule: Detect Outbound Connection to Crypto Pool
  desc: 检测到挖矿池的出站连接
  condition: >
    outbound and container and
    fd.sip in (mining_pool_ips)
  output: >
    Outbound connection to crypto mining pool
    (container=%container.name ip=%fd.sip port=%fd.sport)
  priority: CRITICAL
  tags: [container, network, crypto, mitre_impact]
```

## 安全扫描自动化

### CI/CD 安全门禁

```yaml
# GitHub Actions 安全扫描流水线
name: Security Gate
on: [pull_request]

jobs:
  image-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build Image
        run: docker build -t app:${{ github.sha }} .
      - name: Trivy Scan
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: app:${{ github.sha }}
          severity: CRITICAL,HIGH
          exit-code: '1'  # 发现高危则失败
      - name: Secret Scan
        run: trivy fs --scanners secret --exit-code 1 .

  manifest-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: K8s Config Scan
        run: trivy config --exit-code 1 --severity HIGH,CRITICAL k8s/
      - name: Policy Check
        run: conftest test k8s/ --policy policies/
```

### 定期安全审计

```bash
#!/bin/bash
# security-audit.sh — 周度安全审计

echo "=== 安全审计 $(date) ==="

# 1. 检查特权容器
echo "--- 特权容器 ---"
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].securityContext.privileged == true) |
  "\(.metadata.namespace)/\(.metadata.name)"'

# 2. 检查使用 default SA 的 Pod
echo "--- 使用 default SA ---"
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.serviceAccountName == "default" or .spec.serviceAccountName == null) |
  "\(.metadata.namespace)/\(.metadata.name)"' | head -20

# 3. 检查无资源限制的 Pod
echo "--- 无资源限制 ---"
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].resources.limits == null) |
  "\(.metadata.namespace)/\(.metadata.name)"' | head -20

# 4. 检查 cluster-admin 绑定
echo "--- cluster-admin 绑定 ---"
kubectl get clusterrolebindings -o json | jq -r '.items[] |
  select(.roleRef.name == "cluster-admin") |
  "\(.metadata.name): \(.subjects[]?.name)"'

# 5. 检查过期证书
echo "--- 证书状态 ---"
kubectl get certificates -A -o json 2>/dev/null | jq -r '.items[] |
  select(.status.conditions[]?.type == "Ready") |
  select(.status.conditions[]?.status != "True") |
  "\(.metadata.namespace)/\(.metadata.name): NOT READY"'

echo "=== 审计完成 ==="
```

## 安全成熟度模型

```
Level 1: 基础安全
└── PSA baseline + 基本 RBAC + 镜像扫描

Level 2: 标准化安全
└── PSA restricted + NetworkPolicy + Secret 加密 + 签名验证

Level 3: 深度防御
└── 运行时检测 + 策略即代码 + 供应链安全 + mTLS

Level 4: 零信任
└── 持续验证 + 微分段 + SPIFFE 身份 + 自动修复

Level 5: 自适应安全
└── AI 威胁检测 + 自动响应 + 混沌安全演练 + 合规自动化
```

**安全原则**: 纵深防御，最小权限，持续审计，零信任

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 安全 MOC
- [[08-安全/README.md|Security Domain]]
- [[08-安全/00-总览/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## See Also

- 06-pod-security-standards
- 07-rbac-matrix-configuration
- 09-security-hardening-production
- 10-certificate-management

- [[08-安全/README.md|返回目录]]

## Related

- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->

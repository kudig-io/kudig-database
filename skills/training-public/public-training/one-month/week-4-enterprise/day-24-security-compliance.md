---
title: 'Day 24: 云原生安全 + 合规'
description: 'title: Day 24: 云原生安全 + 合规'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- istio
- helm
- opa
- rbac
- networkpolicy
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 24: 云原生安全 + 合规 是什么'
- '如何 Day 24: 云原生安全 + 合规'
trigger_keywords:
- Day
- '24:'
- 云原生安全
- 合规
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- service-mesh-basics
- etcd-basics
- policy-basics
---

---
title: Day 24: 云原生安全 + 合规
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - Kubernetes 安全加固
  - Kyverno 策略引擎
  - 零信任安全架构
  - Secret 管理工具
trigger_keywords:
  - 云原生安全
  - Kyverno
  - 零信任
  - Sealed Secrets
  - Vault
  - 安全审计
  - 合规
  - 纵深防御
reading_level: intermediate
audience:
  - sre-engineer
  - security-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-05-security-compliance
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-[[domain-02-workloads-applications/topic-functions/cluster-create/16-security|16-security]]-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
---

# Day 24: 云原生安全 + 合规

## 概述

今天深入学习云原生安全体系。在前面的课程中，你已经学习了 RBAC 权限控制和安全基础概念。今天将视角提升到企业级安全：如何使用策略引擎（Kyverno）实施安全策略？如何安全管理 Secret？如何构建零信任安全架构？

云原生安全与传统安全的最大区别在于：容器是短暂的、动态调度的、可能跨越多个节点和可用区。传统的基于边界的防护模型不再适用，需要在每个层面（代码、镜像、运行时、编排、基础设施）都实施安全控制，形成纵深防御体系。

### 学习目标

- 掌握 Kyverno 企业策略引擎的配置和使用
- 了解 Vault 和 Sealed Secrets 等 Secret 管理最佳实践
- 理解零信任安全架构的核心理念和 K8s 落地方案
- 能够进行基本的安全审计和合规检查

---

## 核心概念详解

### Kyverno 策略引擎

Kyverno 是一个专为 Kubernetes 设计的策略引擎。与 OPA Gatekeeper 不同，Kyverno 使用 Kubernetes 原生的资源定义（YAML）来编写策略，无需学习新的策略语言（如 Rego）。这使得 Kyverno 的学习曲线更低，也更易于与 kubectl 生态集成。

Kyverno 支持三种策略类型：

**Validate（验证策略）** 检查资源是否符合预定义的规则。不符合规则的资源可以被拒绝（Enforce 模式）或仅记录告警（Audit 模式）。常见用例：

- 禁止使用 `latest` 标签的镜像
- 要求所有 Pod 必须设置资源限制（resources.limits）
- 禁止运行特权容器（privileged: true）
- 要求所有资源都有特定的标签（如 team、environment）

**Mutate（变更策略）** 在资源创建或更新时自动修改其字段。常见用例：

- 自动为所有 Pod 添加安全上下文（如 runAsNonRoot: true）
- 自动添加标签或注解
- 自动注入 Sidecar 容器（如日志采集 Agent）
- 自动设置镜像拉取策略为 IfNotPresent

**Generate（生成策略）** 当特定资源被创建时，自动生成关联资源。常见用例：

- 创建新命名空间时自动生成 NetworkPolicy
- 创建新命名空间时自动生成 ResourceQuota 和 LimitRange
- 创建 Deployment 时自动生成对应的 ServiceMonitor

策略的作用域可以是集群级别（ClusterPolicy）或命名空间级别（Policy）。ClusterPolicy 对所有命名空间生效，Policy 只对特定命名空间生效。

**策略推荐模式**:

- 开发环境使用 Audit 模式，只记录不拦截
- 预发和生产环境使用 Enforce 模式，拒绝不符合规则的请求
- 使用 `validationFailureAction: Enforce` 确保策略严格执行
- 配合 `background: true` 对已有资源进行扫描和报告

### Secret 管理最佳实践

Kubernetes 原生的 Secret 存在以下安全局限：

- Secret 的值仅使用 Base64 编码（不是加密），任何有权限读取 Secret 的人都能解码
- etcd 中的 Secret 默认以明文存储（除非启用了 EncryptionConfiguration）
- Secret 没有自动轮转机制，需要手动更新
- 审计能力有限，无法追踪谁使用了哪个 Secret

**Sealed Secrets** 是 Bitnami 开源的 Secret 加密方案。它的核心思想是：将 Secret 加密为 SealedSecret 资源（即使公开也安全），在集群内部由 Sealed Secrets Controller 自动解密为原生 Secret。这样 SealedSecret 可以安全地存储在 Git 仓库中（配合 GitOps），而原生 Secret 只存在于集群内部。

**External Secrets Operator（ESO）** 是另一个流行的方案，它从外部 Secret 管理系统（如 AWS Secrets Manager、HashiCorp Vault、Azure Key Vault）同步 Secret 到 K8s。ESO 支持自动轮转——当外部系统的 Secret 更新时，ESO 自动同步到 K8s。

**HashiCorp Vault** 是企业级 Secret 管理工具。在 K8s 环境中的典型使用方式：

- Vault 通过 Kubernetes Auth Method 验证 Pod 的身份（基于 ServiceAccount Token）
- Pod 通过 Vault Agent Sidecar 或 CSI Driver 获取 Secret
- Secret 有 TTL（有效期），到期后自动轮换
- 所有 Secret 的访问都有审计日志

### 零信任安全架构

零信任的核心理念是"永不信任，始终验证"（Never Trust, Always Verify）。在 K8s 环境中，零信任架构包含以下层面：

**身份验证（Identity）**: 每个 Pod 应该有唯一的身份（ServiceAccount）。Pod 之间的通信应该经过身份验证，而非默认互信。

**授权（Authorization）**: 使用 RBAC 严格控制每个身份可以执行的操作。使用 NetworkPolicy 控制 Pod 之间的网络访问。

**加密（Encryption）**: Pod 之间的通信应该加密（可以使用服务网格如 Istio/Linkerd 自动实现 mTLS）。etcd 中的数据应该加密。Secret 应该加密存储。

**可观测性（Observability）**: 所有安全相关的事件都应该被记录和审计。使用 Audit Log 记录 API 调用，使用 NetworkPolicy Log 记录网络访问。

**最小权限（Least Privilege）**: 每个 Pod 只授予完成任务所需的最少权限。容器以非 root 用户运行。删除不必要的 Linux Capabilities。

### 安全审计与合规检查

安全审计是持续验证集群安全状态的过程。关键审计项包括：

- **RBAC 审计**: 检查是否有过度授权的 RoleBinding/ClusterRoleBinding
- **Pod 安全审计**: 检查是否有特权容器、hostNetwork、hostPath 挂载
- **网络策略审计**: 检查是否有未配置 NetworkPolicy 的命名空间
- **镜像安全审计**: 检查是否有使用 latest 标签的镜像、是否有已知漏洞
- **Secret 审计**: 检查是否有明文存储的敏感信息

---

## 实战演练

### 任务 1: Kyverno 策略配置 (1h)

```bash
# 安装 Kyverno
kubectl create namespace kyverno
helm repo add kyverno https://kyverno.github.io/kyverno/
helm repo update
helm install kyverno kyverno/kyverno -n kyverno

# 等待 Kyverno 就绪
kubectl wait --namespace kyverno --for=condition=ready pod -l app.kubernetes.io/name=kyverno --timeout=120s

# 策略 1: 禁止使用 latest 镜像标签
cat > disallow-latest-tag.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
  annotations:
    policies.kyverno.io/title: Disallow Latest Tag
    policies.kyverno.io/category: Best Practices
    policies.kyverno.io/severity: medium
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: require-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "An image tag is required."
      pattern:
        spec:
          containers:
          - image: "*:*"
  - name: validate-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "Using a mutable image tag (e.g., 'latest') is not allowed."
      pattern:
        spec:
          containers:
          - image: "!*:latest"
EOF

kubectl apply -f disallow-latest-tag.yaml

# 测试: 使用 latest 标签应该被拒绝
cat > test-latest.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: test-latest
spec:
  containers:
  - name: nginx
    image: nginx:latest
EOF

kubectl apply -f test-latest.yaml
# 预期: Error from server: admission webhook "validate.kyverno.svc" denied the request

# 策略 2: 强制资源限制
cat > require-limits.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-limits
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: check-limits
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "CPU and memory limits are required for all containers."
      pattern:
        spec:
          containers:
          - resources:
              limits:
                memory: "?*"
                cpu: "?*"
EOF

kubectl apply -f require-limits.yaml

# 策略 3: 自动添加安全上下文 (Mutate)
cat > add-security-context.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-security-context
spec:
  rules:
  - name: add-runasnonroot
    match:
      any:
      - resources:
          kinds:
          - Pod
    mutate:
      patchStrategicMerge:
        spec:
          securityContext:
            runAsNonRoot: true
            seccompProfile:
              type: RuntimeDefault
          containers:
          - (name): "?*"
            securityContext:
              allowPrivilegeEscalation: false
              capabilities:
                drop:
                - ALL
              readOnlyRootFilesystem: false
EOF

kubectl apply -f add-security-context.yaml
```

### 任务 2: Secret 管理实践 (1h)

```bash
# 安装 Sealed Secrets Controller
kubectl apply -f https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/controller.yaml

# 等待 Controller 就绪
kubectl wait --namespace kube-system --for=condition=ready pod -l name=sealed-secrets-controller --timeout=120s

# 安装 kubeseal CLI
# macOS: brew install kubeseal
# Linux: wget https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/kubeseal-linux-amd64 -O /usr/local/bin/kubeseal

# 创建加密的 Secret
kubectl create secret generic db-credentials \
  --from-literal=username=admin \
  --from-literal=password='SuperSecret123!' \
  --dry-run=client -o yaml | kubeseal -o yaml > sealed-db-credentials.yaml

# 查看 SealedSecret (加密内容，可以安全提交到 Git)
cat sealed-db-credentials.yaml

# 应用 SealedSecret (Controller 会自动解密为原生 Secret)
kubectl apply -f sealed-db-credentials.yaml

# 验证: 查看自动创建的 Secret
kubectl get secret db-credentials -o yaml
kubectl get secret db-credentials -o jsonpath='{.data.password}' | base64 -d
```

### 任务 3: 安全审计 (30min)

```bash
# 检查 RBAC 权限: 查看默认 ServiceAccount 的权限
kubectl auth can-i --list --as=system:serviceaccount:default:default

# 检查特权容器
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].securityContext.privileged}{"\n"}{end}' | grep true

# 检查 hostNetwork
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.hostNetwork}{"\n"}{end}' | grep true

# 检查 hostPath 挂载
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.volumes[]?.hostPath != null) | "\(.metadata.namespace)/\(.metadata.name): \([.spec.volumes[] | select(.hostPath != null) | .hostPath.path] | join(", "))"'

# 检查使用 latest 标签的 Pod
kubectl get pods -A -o json | jq -r '.items[] | .spec.containers[] | select(.image | test(":latest$|^[^:]+$")) | .image'

# 检查没有设置资源限制的 Pod
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[]? | .resources.limits == null) | "\(.metadata.namespace)/\(.metadata.name)"'

# 查看 Kyverno 策略报告
kubectl get policyreport -A
kubectl get clusterpolicyreport
```

---

## 常见问题

### Q1: Kyverno 策略误拦截了正常的部署怎么办？

在策略中设置 `validationFailureAction: Audit` 而非 `Enforce`，先观察一段时间确认无误报后再切换为 Enforce。也可以使用 `exclude` 字段排除特定的命名空间或资源。紧急情况下可以暂时删除策略资源恢复部署。

### Q2: Sealed Secrets 和 Vault 应该选哪个？

Sealed Secrets 更轻量，适合中小团队，核心优势是可以将加密的 Secret 存储在 Git 中。Vault 功能更强大（自动轮转、动态 Secret、审计日志），适合大型企业和有严格合规要求的场景。两者也可以组合使用。

### Q3: 如何确保所有命名空间都有 NetworkPolicy？

使用 Kyverno 的 Generate 策略：当新命名空间创建时，自动生成默认的 NetworkPolicy（默认拒绝所有入站和出站流量，然后按需放行）。这是零信任架构在网络层面的关键实践。

### Q4: 安全策略太多会不会影响集群性能？

Kyverno 使用 Admission Webhook 机制，每个 API 请求都会经过策略验证。大量复杂的策略确实会增加 API 请求的延迟。建议：1) 只保留必要的策略；2) 使用 background 模式对已有资源进行异步扫描；3) 监控 Kyverno 的资源使用情况。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| Kyverno | K8s 原生策略引擎，支持 Validate/Mutate/Generate |
| Secret 管理 | Sealed Secrets 加密存储、Vault 企业级管理 |
| 零信任 | 永不信任、始终验证、最小权限 |
| 安全审计 | 检查 RBAC、特权容器、hostNetwork、资源限制 |
| 纵深防御 | 代码→镜像→运行时→编排→基础设施 |

---

## 延伸阅读

- [Kyverno 企业策略管理](../../domain-05-security-compliance/04-kyverno-enterprise-policy-management.md)
- [Vault 企业 Secret 管理](../../domain-05-security-compliance/05-vault-enterprise-secrets-management.md)
- [零信任安全架构](../../domain-11-production-operations/07-zero-trust-security-architecture.md)
- [认证授权系统](../../domain-05-security-compliance/01-authentication-authorization-system.md)
- [Pod 安全标准](../../domain-05-security-compliance/06-pod-security-standards.md)
- [Secret 管理工具](../../domain-05-security-compliance/11-secret-management-tools.md)

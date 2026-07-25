---
title: "Pod Security Standards 迁移"
description: "Pod Security Standards 迁移：PSP→PSA 迁移路径、Privileged/Baseline/Restricted 配置、Admission 配置与常见兼容问题"
summary: "面向 SRE 与安全工程师的 Pod Security Standards 完整迁移指南，覆盖从 PodSecurityPolicy 到 Pod Security Admission 的迁移路径、三级标准配置与兼容问题处理。"
category: 安全
tags:
- pod-security
- psa
- psp
- admission
- security
- migration
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- "如何从 PSP 迁移到 Pod Security Admission"
- "Privileged Baseline Restricted 三个级别如何配置"
- "PSA 迁移有哪些兼容性问题"
trigger_keywords:
- pod security
- psa
- psp
- admission
- privileged
- baseline
- restricted
prerequisites:
- kubectl-basics
- rbac-basics
- security-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod Security Standards 迁移

> **适用版本**: Kubernetes v1.28+（PSP 已于 1.25 移除，PSA 自 1.25 GA）
> **最后更新**: 2026-07

---

## 概述

PodSecurityPolicy（PSP）曾是 Kubernetes 唯一的 Pod 安全准入控制机制，它通过定义一组安全约束（如是否允许特权容器、是否要求以非 root 运行、允许哪些 capabilities 等），在 Pod 创建时进行准入检查。然而，PSP 在实际使用中暴露出严重的设计缺陷：它是全局生效的，无法按 namespace 差异化配置；它的绑定机制依赖 RBAC，理解和使用成本极高；它的策略表达能力有限，很多安全需求无法精确描述。

因此，Kubernetes 社区在 1.21 引入了 Pod Security Admission（PSA）作为 PSP 的替代方案，并在 1.25 正式移除了 PSP。PSA 是一个内置的 admission controller，它根据 namespace 上的标签来执行预定义的安全标准——Pod Security Standards（PSS）。PSS 定义了三个安全级别：Privileged（无限制）、Baseline（防止已知提权）、Restricted（强化安全），覆盖了绝大多数场景的安全需求。

对于仍在运行旧版本集群或刚从 PSP 迁移的团队来说，这是一项必须完成且时间紧迫的升级。迁移不当可能导致大量工作负载无法创建，引发生产事故。本文系统覆盖迁移路径、三级标准配置、Admission 配置与常见兼容问题。PSA 的深度原理见 [[08-安全/01-身份与访问/02-pod-security-admission-deep-dive.md|Pod Security Admission 深度解析]]，策略引擎对比见 [[08-安全/04-策略治理/14-policy-engines-opa-kyverno.md|策略引擎 OPA 与 Kyverno]]。

---

## 核心概念

### 1. PSP vs PSA 对比

理解 PSP 和 PSA 的根本差异，是顺利完成迁移的认知基础。

| 维度 | PSP（已废弃） | PSA/PSS |
|------|--------------|---------|
| 类型 | 独立资源 + admission | 内置 admission controller |
| 作用域 | 全局（通过 RBAC 绑定） | 按 namespace（label） |
| 配置方式 | 复杂资源定义 | 简单 namespace label |
| 标准 | 自定义 | 三级预定义标准 |
| 模式 | 强制 | enforce / audit / warn |
| 版本控制 | 无 | 支持版本固定 |
| 维护成本 | 高 | 低 |

PSA 最大的改进是将作用域从全局细化到了 namespace 级别。在 PSP 时代，要为不同团队设置不同的安全策略，需要创建多个 PSP 资源并通过复杂的 RBAC 绑定将它们关联到不同的 ServiceAccount，这个过程极其容易出错。而在 PSA 中，只需在 namespace 上打一个标签（如 pod-security.kubernetes.io/enforce: restricted），就能为该 namespace 下的所有 Pod 设置安全标准，简洁直观。

另一个重要改进是三种执行模式的引入。enforce 模式会拒绝不合规的 Pod 创建；audit 模式将违规记录到审计日志但不阻止；warn 模式向用户返回警告信息但不阻止。这三种模式的组合为渐进式迁移提供了可能——先用 audit 和 warn 观察哪些工作负载不合规，修复后再切换到 enforce。

### 2. 三个安全级别

PSS 定义的三个安全级别是递进关系，每一级在上一级的基础上增加更多限制。

| 级别 | 定位 | 限制程度 | 典型场景 |
|------|------|---------|---------|
| **Privileged** | 无限制 | 无 | 系统组件、特权基础设施 |
| **Baseline** | 防止已知提权 | 中 | 通用应用、中间件 |
| **Restricted** | 强化安全 | 高 | 多租户、不可信工作负载 |

Privileged 级别不做任何限制，适用于 kube-system 等需要运行特权容器的系统命名空间。Baseline 级别禁止已知的提权手段（如特权容器、hostNetwork、hostPID、危险 capabilities 等），但不要求非 root 运行或只读文件系统，适用于大多数通用应用和中间件。Restricted 级别在 Baseline 基础上进一步要求以非 root 运行、禁止提权、丢弃所有 capabilities、设置 seccomp profile，适用于多租户环境中运行不可信工作负载。

### 3. 三种执行模式

enforce、audit、warn 三种模式可以独立配置不同的级别，这种灵活性是渐进迁移的关键。一个典型的迁移策略是：enforce 设为 baseline（确保基本安全底线），audit 和 warn 设为 restricted（记录距离最佳实践还有多远），这样既不阻断现有工作负载，又能持续追踪安全改进进度。

---

## 生产部署/实现

### 1. 迁移评估（识别特权工作负载） 🟢

迁移的第一步是全面评估现有工作负载的安全配置，识别哪些 Pod 使用了特权特性。

```bash
# 🟢 低风险：只读，识别需要特权的 Pod
# 查找使用特权配置的 Pod
kubectl get pods -A -o json | jq -r '.items[] | select(
  .spec.containers[].securityContext.privileged == true or
  .spec.containers[].securityContext.allowPrivilegeEscalation == true or
  .spec.hostNetwork == true or
  .spec.hostPID == true
) | "\(.metadata.namespace)/\(.metadata.name)"'

# 查找以 root 运行的 Pod
kubectl get pods -A -o json | jq -r '.items[] | select(
  .spec.containers[].securityContext.runAsNonRoot != true
) | .metadata.namespace' | sort -u
```

这个评估步骤至关重要。在我们的迁移实践中，最常见的事故就是未经充分评估就启用 enforce 模式，导致大量 Pod 突然无法创建。评估结果应该形成一份清单，列出每个不合规的工作负载、它使用了哪些特权特性、以及是否可以改造为合规配置。对于确实需要特权的系统组件（如 CNI 插件、存储驱动），应该将其所在命名空间标记为 privileged 级别。

### 2. Namespace 级 PSA 配置 🟡

PSA 的配置通过 namespace 标签实现，简洁但需要谨慎规划。

```yaml
# 🟡 中风险：enforce 模式会拒绝不合规 Pod
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    # Restricted 级别（最严格）
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    # 系统组件用 Privileged
    pod-security.kubernetes.io/enforce: privileged
---
apiVersion: v1
kind: Namespace
metadata:
  name: middleware
  labels:
    # 中间件用 Baseline
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/audit: restricted    # 审计用更严格标准
```

这个配置体现了分级管理的思想：production 命名空间运行的是业务应用，采用最严格的 restricted 级别；kube-system 运行系统组件（CNI、DNS、监控等），需要特权能力，采用 privileged 级别；middleware 运行数据库、消息队列等中间件，某些可能需要部分特权（如设置 sysctl），采用 baseline 级别但用 restricted 进行审计追踪。enforce-version 固定到当前版本是一个重要的最佳实践，它确保 Kubernetes 升级后策略行为不会意外变化。

### 3. Restricted 级别合规 Pod 模板 🟡

为开发团队提供合规模板是推动迁移的最有效手段。

```yaml
# 🟡 中风险：满足 Restricted 标准的 Pod 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
  namespace: production
spec:
  template:
    spec:
      securityContext:
        runAsNonRoot: true              # 必须非 root
        runAsUser: 1000
        fsGroup: 2000
        seccompProfile:
          type: RuntimeDefault          # 必须设置 seccomp
      containers:
      - name: app
        image: registry.example.com/app:v1.0
        securityContext:
          allowPrivilegeEscalation: false   # 必须禁止提权
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          capabilities:
            drop: ["ALL"]                   # 必须丢弃所有 capabilities
          seccompProfile:
            type: RuntimeDefault
```

Restricted 级别的要求可以总结为"五个必须"：必须以非 root 运行（runAsNonRoot: true）、必须禁止提权（allowPrivilegeEscalation: false）、必须丢弃所有 capabilities（drop: ["ALL"]）、必须设置 seccomp profile（RuntimeDefault 或自定义）、建议只读根文件系统（readOnlyRootFilesystem: true）。对于大多数应用，满足这些要求并不困难，最常见的改造工作是确保应用不以 root 用户运行，以及将需要写入的路径挂载为 emptyDir 或 PVC。

### 4. 集群级默认准入配置 🔴

对于需要在集群级别设置默认策略的场景，可以通过 AdmissionConfiguration 实现。

```yaml
# 🔴 高风险：修改 apiserver admission 配置影响全集群
# /etc/kubernetes/admission/pod-security.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "baseline"              # 全局默认 baseline
      enforce-version: "latest"
      audit: "restricted"
      warn: "restricted"
    exemptions:
      usernames: []
      runtimeClasses: []
      namespaces: [kube-system, kube-public, kube-node-lease]
```

```bash
# 🔴 高风险：apiserver 启用配置
# --admission-control-config-file=/etc/kubernetes/admission/pod-security.yaml
```

集群级配置为所有未设置 PSA 标签的命名空间提供默认策略。这里将默认 enforce 设为 baseline 是一个务实的选择——它确保了基本的安全底线（禁止特权容器、hostNetwork 等），同时不会因为过于严格而阻断现有工作负载。exemptions 中列出了系统命名空间，这些命名空间需要运行特权组件，不应受默认策略约束。

---

## 运维操作

### 1. 分阶段迁移流程 🟡

渐进式迁移是避免生产事故的关键。

```bash
# 🟡 中风险：分阶段迁移，先观察后强制
# 阶段 1：全集群 audit + warn（不阻止，仅记录）
kubectl label ns production \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted --overwrite

# 阶段 2：分析审计日志，修复不合规工作负载
kubectl logs -n kube-system -l component=kube-apiserver | grep "pod-security"

# 阶段 3：切换 enforce（确认无不合规后）
kubectl label ns production \
  pod-security.kubernetes.io/enforce=restricted --overwrite
```

阶段 1 的 audit + warn 模式是整个迁移的安全网。在这个阶段，不合规的 Pod 仍然可以正常创建和运行，但每一次违规都会被记录到审计日志，并在 kubectl 输出中显示警告。运维团队有充足的时间分析这些违规记录，逐一修复不合规的工作负载。只有当审计日志中不再出现违规记录时，才应该进入阶段 3 切换到 enforce 模式。

### 2. 审计日志分析 🟢

```bash
# 🟢 低风险：只读
# 查找 PSA 违规记录
kubectl logs -n kube-system -l component=kube-apiserver --tail=10000 | \
  grep "pod-security.kubernetes.io" | \
  grep -oP 'violates? .*?Restricted' | sort | uniq -c | sort -rn
```

### 3. 验证合规性 🟢

```bash
# 🟢 低风险
kubectl get ns production -o jsonpath='{.metadata.labels}' | jq
# 尝试创建不合规 Pod 验证 enforce 生效
kubectl -n production run test --image=busybox --restart=Never \
  --overrides='{"spec":{"containers":[{"name":"t","image":"busybox","securityContext":{"privileged":true}}]}}' \
  --dry-run=server
# 应被拒绝
```

使用 --dry-run=server 进行验证是一个安全的做法——它会触发服务端的准入检查但不会真正创建资源，既能验证策略是否生效，又不会产生垃圾资源。

---

## 故障排查

### 症状 1：Pod 被 PSA 拒绝创建

```bash
# 🟢 低风险
kubectl -n production apply -f pod.yaml
# Error: violates PodSecurity "restricted:latest": ...
```

根因是 Pod 的 securityContext 配置不满足命名空间的 enforce 级别。错误信息会明确指出违反了哪条规则（如 "must not include securityContext.privileged"、"must not set allowPrivilegeEscalation" 等）。处置方法是按照错误提示修正 securityContext 配置，或者在确实需要特权的场景下调整命名空间的 PSA 级别（需谨慎评估）。

### 症状 2：系统组件无法启动

根因是 kube-system 等系统命名空间被误设为 restricted 或 baseline 级别，而系统组件（如 CNI 插件需要 NET_ADMIN capability、节点问题检测器需要 hostPID）不满足这些限制。处置方法是将系统命名空间设为 privileged 级别，或在集群级配置的 exemptions 中排除它们。

### 症状 3：迁移后大量 Pod 不合规

根因是未充分执行评估阶段就启用了 enforce 模式。处置方法是立即回退到 audit/warn 模式恢复业务，然后按照评估脚本逐一排查和修复不合规工作负载，最后再重新启用 enforce。

### 症状 4：第三方 Operator 创建 Pod 失败

根因是第三方 Operator（如某些数据库 Operator、监控 Operator）生成的 Pod 不满足 restricted 级别的要求，比如需要特定的 capabilities 或以 root 运行。处置方法是为 Operator 所在的命名空间设置 baseline 级别、为特定 ServiceAccount 配置 exemption、或者联系厂商确认是否有适配 PSA 的新版本。

### 排查决策树

```
PSA 问题
├── Pod 被拒?       → 看错误信息修 securityContext
├── 系统组件失败?   → namespace 设 privileged/exemption
├── 大面积不合规?   → 回退 audit，渐进迁移
└── Operator 失败?  → baseline 或 exemption
```

---

## 最佳实践

第一，渐进迁移是铁律，必须经历 audit/warn 观察、修复不合规、enforce 强制三个阶段，切勿一步到位。第二，分级管理命名空间，系统组件用 privileged，通用应用用 baseline，多租户和不可信负载用 restricted。第三，enforce-version 固定到当前 Kubernetes 版本，避免升级后策略行为意外变化。第四，为开发团队提供 Restricted 合规的 Deployment 模板和最佳实践文档，降低改造门槛。第五，在 CI 流水线中集成合规预检，使用 kubectl apply --dry-run=server 或 Kyverno 在部署前发现问题，参考 [[08-安全/04-策略治理/05-policy-validation-tools.md|策略验证工具]]。第六，PSA 覆盖基础安全，复杂的策略需求（如镜像来源限制、标签规范）用 Kyverno 或 OPA Gatekeeper 补充。第七，持续分析 audit 日志，将不合规趋势纳入安全度量。第八，exemption 要最小化，每个豁免都应有审批记录和复审计划。

```yaml
# 🟢 低风险：Kyverno 补充策略（要求镜像来源）
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-image-registry
spec:
  validationFailureAction: Enforce
  rules:
  - name: validate-registry
    match:
      any:
      - resources:
          kinds: ["Pod"]
    validate:
      message: "镜像必须来自受信任仓库"
      pattern:
        spec:
          containers:
          - image: "registry.example.com/*"
```

---

## Related

- [[08-安全/01-身份与访问/02-pod-security-admission-deep-dive.md|Pod Security Admission 深度解析]]
- [[08-安全/04-策略治理/06-pod-security-standards.md|Pod Security Standards]]
- [[08-安全/04-策略治理/14-policy-engines-opa-kyverno.md|策略引擎 OPA 与 Kyverno]]
- [[08-安全/04-策略治理/05-policy-validation-tools.md|策略验证工具]]
- [[08-安全/04-策略治理/04-kyverno-enterprise-policy-management.md|Kyverno 企业策略管理]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go 应用 Kubernetes 生产实践]]

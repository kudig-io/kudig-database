---
title: OPA/Kyverno 策略即代码示例
description: Kyverno ClusterPolicy 和 OPA Gatekeeper Constraint 示例
summary: 使用 Kyverno 和 OPA Gatekeeper 实现策略即代码，包括镜像来源限制、标签规范、资源配额强制等
category: manifests-patterns
tags:
- k8s
- manifests
- security
- kyverno
- opa
- gatekeeper
- policy-as-code
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- Kyverno 策略示例
- OPA Gatekeeper 约束
- 策略即代码 Kubernetes
trigger_keywords:
- kyverno
- opa
- gatekeeper
- policy
- constraint
prerequisites:
- admission-webhook-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# OPA/Kyverno 策略即代码示例

## 1. Kyverno vs OPA Gatekeeper

| 特性 | Kyverno | OPA Gatekeeper |
|------|---------|----------------|
| 策略语言 | YAML（原生 K8s 风格） | Rego |
| 学习曲线 | 低 | 中高 |
| Mutating 支持 | 原生支持 | 需要 Mutating Webhook |
| 生成资源 | 支持（如自动生成 NetworkPolicy） | 不支持 |
| 适用场景 | 标签/注解/默认值 | 复杂逻辑验证 |

## 2. Kyverno 策略示例

### 2.1 强制必需标签

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-labels
spec:
  validationFailureAction: Enforce   # Enforce=拒绝, Audit=仅审计
  rules:
    - name: require-app-label
      match:
        any:
          - resources:
              kinds: ["Pod"]
      validate:
        message: "必须包含 app 标签"
        pattern:
          metadata:
            labels:
              app: "?*"              # 非空值
    - name: require-team-label
      match:
        any:
          - resources:
              kinds: ["Deployment", "StatefulSet"]
      validate:
        message: "Deployment/StatefulSet 必须包含 team 标签"
        pattern:
          metadata:
            labels:
              team: "?*"
          spec:
            template:
              metadata:
                labels:
                  team: "?*"
```

### 2.2 禁止 latest 标签

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  rules:
    - name: require-image-tag
      match:
        any:
          - resources:
              kinds: ["Pod"]
      validate:
        message: "镜像必须有明确的 tag，不允许使用 :latest"
        pattern:
          spec:
            containers:
              - image: "*:*"         # 必须包含 tag
    - name: disallow-latest
      match:
        any:
          - resources:
              kinds: ["Pod"]
      validate:
        message: "禁止使用 :latest 标签"
        pattern:
          spec:
            containers:
              - image: "!*:latest"
```

### 2.3 强制资源限制

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: Enforce
  rules:
    - name: require-resources
      match:
        any:
          - resources:
              kinds: ["Pod"]
      validate:
        message: "容器必须设置 resources.requests 和 resources.limits"
        pattern:
          spec:
            containers:
              - resources:
                  requests:
                    memory: "?*"
                    cpu: "?*"
                  limits:
                    memory: "?*"
                    cpu: "?*"
```

### 2.4 自动注入默认值（Mutating）

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-default-securitycontext
spec:
  rules:
    - name: add-security-context
      match:
        any:
          - resources:
              kinds: ["Pod"]
      mutate:
        patchStrategicMerge:
          spec:
            containers:
              - (name): "*"
                securityContext:
                  +(runAsNonRoot): true        # 不存在时添加
                  +(allowPrivilegeEscalation): false
                  +(readOnlyRootFilesystem): true
```

### 2.5 自动生成 NetworkPolicy

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-default-deny
spec:
  rules:
    - name: generate-deny-policy
      match:
        any:
          - resources:
              kinds: ["Namespace"]
      generate:
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny
        namespace: "{{request.object.metadata.name}}"
        synchronize: true               # 源变更时自动同步
        data:
          spec:
            podSelector: {}
            policyTypes: ["Ingress", "Egress"]
```

## 3. OPA Gatekeeper 示例

### 3.1 ConstraintTemplate（定义策略）

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredlabels
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredLabels
      validation:
        openAPIV3Schema:
          type: object
          properties:
            labels:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredlabels

        violation[{"msg": msg, "details": {"missing_labels": missing}}] {
          provided := {label | input.review.object.metadata.labels[label]}
          required := {label | label := input.parameters.labels[_]}
          missing := required - provided
          count(missing) > 0
          msg := sprintf("必须包含以下标签: %v", [missing])
        }
```

### 3.2 Constraint（应用策略）

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-app-label
spec:
  enforcementAction: deny             # deny/dryrun/warn
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Namespace"]
    excludedNamespaces:
      - kube-system
      - gatekeeper-system
  parameters:
    labels: ["app", "team", "environment"]
```

### 3.3 禁止特权容器

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8snoprivileged
spec:
  crd:
    spec:
      names:
        kind: K8sNoPrivileged
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8snoprivileged

        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          container.securityContext.privileged == true
          msg := sprintf("容器 %v 不允许使用特权模式", [container.name])
        }
---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sNoPrivileged
metadata:
  name: no-privileged-containers
spec:
  enforcementAction: deny
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
```

## 4. 渐进式部署策略

```
阶段 1: Audit 模式（只记录不拒绝）
    ↓ 观察违规
阶段 2: Warn 模式（警告用户）
    ↓ 修复已知违规
阶段 3: Enforce 模式（强制执行） ✅
```

## 5. 生产实践

| 实践 | 说明 |
|------|------|
| 先 Audit 后 Enforce | 避免突然阻断部署 |
| 排除系统命名空间 | 避免 kube-system 被策略拦截 |
| 使用 ConfigMap 例外列表 | 允许特定应用豁免 |
| 监控策略违规 | Prometheus + Gatekeeper metrics |
| 版本化策略 | 策略存入 Git，PR 审核变更 |

## Related

- [[03-清单模式/06-安全模式/01-pod-security-standards-reference|Pod Security Standards]]
- [[03-清单模式/01-YAML参考/25-validatingadmissionpolicy|ValidatingAdmissionPolicy]]

## See Also

- [Kyverno 文档](https://kyverno.io/docs/)
- [OPA Gatekeeper 文档](https://open-policy-agent.github.io/gatekeeper/website/docs/)

<!-- risk-assessed -->

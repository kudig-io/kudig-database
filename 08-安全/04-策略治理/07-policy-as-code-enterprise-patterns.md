---
title: Policy as Code at Scale — OPA/Gatekeeper and Kyverno Enterprise Patterns
description: K8s 策略即代码 — OPA/Gatekeeper vs Kyverno 企业实践、策略测试、审计模式、多集群策略分发、合规报告
summary: 在大规模 Kubernetes 环境中实施策略即代码的企业级模式与最佳实践
category: practice
tags:
- policy-as-code
- opa
- gatekeeper
- kyverno
- compliance
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: security
---
# 策略即代码企业级实践

> OPA/Gatekeeper 与 Kyverno 在大规模 K8s 环境中的策略治理。

## 引擎对比

| 维度 | OPA/Gatekeeper | Kyverno |
|------|---------------|---------|
| 语言 | Rego（专用） | YAML（K8s 原生） |
| 学习曲线 | 陡峭 | 平缓 |
| 策略测试 | `opa test` 框架 | `kyverno test` |
| 变更（Mutate） | ❌（仅验证） | ✅ |
| 生成（Generate） | ❌ | ✅ |
| 镜像验证 | 需额外工具 | ✅ 内置 |
| 性能（大规模） | 优秀（编译后） | 良好 |
| CNCF 状态 | Graduated | Incubating |
| 适用 | 复杂策略/多系统 | K8s 专注/快速上手 |

## 策略生命周期

```
编写 → 测试 → 审计模式 → 告警模式 → 强制模式 → 持续监控
 │       │        │           │           │           │
Rego/   单元    AUDIT      WARN       ENFORCE    违规报告
YAML    测试   (仅记录)   (告警)     (拒绝)     趋势分析
```

## Kyverno 企业策略集

### 安全基线策略

```yaml
# 禁止特权容器
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged
  annotations:
    policies.kyverno.io/title: 禁止特权容器
    policies.kyverno.io/category: Pod Security
    policies.kyverno.io/severity: high
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: privileged-containers
      match:
        resources:
          kinds: ["Pod"]
          namespaces: ["production", "staging"]
      validate:
        message: "特权容器在生产环境被禁止。设置 securityContext.privileged=false"
        pattern:
          spec:
            containers:
              - securityContext:
                  privileged: "false"
---
# 强制资源限制
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: check-resources
      match:
        resources:
          kinds: ["Pod"]
          namespaces: ["production"]
      validate:
        message: "生产环境 Pod 必须设置 CPU/内存 requests 和 limits"
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
---
# 禁止 latest 标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: validate-image-tag
      match:
        resources:
          kinds: ["Pod"]
      validate:
        message: "镜像标签不能为 'latest' 或为空"
        pattern:
          spec:
            containers:
              - image: "*:*"
        deny:
          conditions:
            any:
              - key: "{{ images.containers[].tag }}"
                operator: AnyIn
                value: ["latest", ""]
```

### 自动变更策略（Mutate）

```yaml
# 自动注入安全上下文
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: inject-security-context
spec:
  rules:
    - name: add-security-context
      match:
        resources:
          kinds: ["Pod"]
          namespaces: ["production", "staging"]
      mutate:
        patchStrategicMerge:
          spec:
            securityContext:
              runAsNonRoot: true
              seccompProfile:
                type: RuntimeDefault
            containers:
              - (name): "*"
                securityContext:
                  allowPrivilegeEscalation: false
                  capabilities:
                    drop: ["ALL"]
---
# 自动添加标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-default-labels
spec:
  rules:
    - name: add-team-label
      match:
        resources:
          kinds: ["Deployment", "StatefulSet", "DaemonSet"]
      mutate:
        patchStrategicMerge:
          metadata:
            labels:
              +(app.kubernetes.io/managed-by): kyverno
```

### 自动生成策略（Generate）

```yaml
# 新命名空间自动生成 NetworkPolicy
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-default-netpol
spec:
  rules:
    - name: default-deny-ingress
      match:
        resources:
          kinds: ["Namespace"]
      generate:
        synchronize: true
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny-ingress
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            podSelector: {}
            policyTypes: ["Ingress"]
```

## OPA/Gatekeeper 高级策略

### ConstraintTemplate（复杂逻辑）

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8shttpsonly
spec:
  crd:
    spec:
      names:
        kind: K8sHttpsOnly
      validation:
        openAPIV3Schema:
          type: object
          properties:
            exemptImages:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8shttpsonly
        
        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          not is_exempt(container.image)
          not startswith(container.image, "registry.internal.example.com/")
          msg := sprintf("镜像 %v 必须来自内部 Registry", [container.image])
        }
        
        is_exempt(image) {
          exempt := input.parameters.exemptImages[_]
          image == exempt
        }
---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sHttpsOnly
metadata:
  name: restrict-registries
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
  parameters:
    exemptImages:
      - "gcr.io/distroless/static:latest"
```

## 策略测试

### Kyverno 测试

```yaml
# tests/disallow-privileged_test.yaml
name: disallow-privileged
policies:
  - ../policies/disallow-privileged.yaml
resources:
  - resources.yaml
results:
  - policy: disallow-privileged
    rule: privileged-containers
    resources:
      - privileged-pod
    result: fail
  - policy: disallow-privileged
    rule: privileged-containers
    resources:
      - safe-pod
    result: pass
```

```yaml
# tests/resources.yaml
apiVersion: v1
kind: Pod
metadata:
  name: privileged-pod
spec:
  containers:
    - name: test
      image: nginx
      securityContext:
        privileged: true
---
apiVersion: v1
kind: Pod
metadata:
  name: safe-pod
spec:
  containers:
    - name: test
      image: nginx
      securityContext:
        privileged: false
```

```bash
# 运行测试
kyverno test ./tests/ -v
```

## 多集群策略分发

### GitOps 策略分发

```yaml
# ArgoCD ApplicationSet — 策略分发到所有集群
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: cluster-policies
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            policy-managed: "true"
  template:
    metadata:
      name: 'policies-{{name}}'
    spec:
      project: platform
      source:
        repoURL: https://github.com/myorg/policies.git
        targetRevision: main
        path: policies/
      destination:
        server: '{{server}}'
        namespace: kyverno
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

## 合规报告

```bash
# Kyverno 策略违规报告
kubectl get policyreport -A -o json | jq -r '.items[] | 
  .results[] | select(.result == "fail") | 
  "\(.policy) | \(.resources[0].namespace)/\(.resources[0].name) | \(.message)"'

# Gatekeeper 违规
kubectl get constraints -o json | jq -r '.items[] | 
  .status.violations[]? | 
  "\(.kind) | \(.namespace)/\(.name) | \(.message)"'

# 合规趋势（Prometheus）
# kyverno_policy_results_total{policy_name="disallow-privileged", result="fail"}
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 渐进式强制 | AUDIT → WARN → ENFORCE |
| 策略版本控制 | Git 管理所有策略 |
| CI 测试 | 每次策略变更运行测试 |
| 排除系统命名空间 | kube-system 等豁免 |
| 清晰消息 | 告诉用户如何修复 |
| 定期审计 | 月度策略有效性审查 |
| 性能监控 | 策略评估延迟 < 10ms |
| 文档化 | 每条策略有注解说明 |

## Related

- [[08-安全/04-策略治理/index.md|策略治理]]
- [[08-安全/04-策略治理/01-kyverno-enterprise-policy-management.md|Kyverno 企业管理]]
- [[08-安全/06-合规审计/index.md|合规审计]]

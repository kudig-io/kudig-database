---
title: 准入策略治理
description: 'OPA/Gatekeeper 与 Kyverno 准入策略的编写、最佳实践与渐进式部署'
summary: 'OPA/Gatekeeper 与 Kyverno 准入策略的编写、最佳实践与渐进式部署'
category: production-operations
tags:
- governance
- admission-policy
- opa
- gatekeeper
- kyverno
- policy-as-code
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 准入策略治理 是什么
- 如何编写 Kyverno 策略
trigger_keywords:
- admission-policy
- opa
- gatekeeper
- kyverno
- constraint
prerequisites:
- kubectl-basics
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


# 准入策略治理

## 1. 概述

准入策略（Admission Policy）是 Kubernetes 安全治理的核心机制。通过 Validating/Mutating Admission Webhook，集群管理员可以在资源创建或修改时执行策略检查，实现安全基线、合规要求和最佳实践的强制执行。

本文覆盖两大主流方案：OPA/Gatekeeper（Rego 语言）和 Kyverno（YAML 原生），以及策略即代码（Policy-as-Code）工作流和渐进式部署策略。

## 2. OPA/Gatekeeper

### 2.1 架构概述

Gatekeeper 将 Open Policy Agent（OPA）集成到 Kubernetes 准入链：

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply → API Server → Gatekeeper Webhook → OPA Engine → 允许/拒绝
                                    ↓
                            ConstraintTemplate（Rego 策略）
                            Constraint（实例化参数）
                            Audit（存量资源检查）
```
### 2.2 ConstraintTemplate 编写

```yaml
# 容器安全基线：禁止特权容器
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8spspprivilegedcontainer
spec:
  crd:
    spec:
      names:
        kind: K8sPSPPrivilegedContainer
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8spspprivilegedcontainer
        
        violation[{"msg": msg, "details": {}}] {
            c := input_containers[_]
            c.securityContext.privileged
            msg := sprintf("容器 %v 不允许设置 privileged: true", [c.name])
        }
        
        input_containers[c] {
            c := input.review.object.spec.containers[_]
        }
        
        input_containers[c] {
            c := input.review.object.spec.initContainers[_]
        }
        
        input_containers[c] {
            c := input.review.object.spec.ephemeralContainers[_]
        }
```

```yaml
# 资源限制强制检查
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8sresourcelimits
spec:
  crd:
    spec:
      names:
        kind: K8sResourceLimits
      validation:
        openAPIV3Schema:
          type: object
          properties:
            maxCpu:
              type: string
            maxMemory:
              type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8sresourcelimits
        
        violation[{"msg": msg}] {
            c := input.review.object.spec.containers[_]
            not c.resources.limits.cpu
            msg := sprintf("容器 %v 必须设置 CPU limits", [c.name])
        }
        
        violation[{"msg": msg}] {
            c := input.review.object.spec.containers[_]
            not c.resources.limits.memory
            msg := sprintf("容器 %v 必须设置 memory limits", [c.name])
        }
        
        violation[{"msg": msg}] {
            c := input.review.object.spec.containers[_]
            cpu_limit := c.resources.limits.cpu
            max_cpu := input.parameters.maxCpu
            cpu_to_milli("milli", cpu_limit) > cpu_to_milli("milli", max_cpu)
            msg := sprintf("容器 %v CPU limit %v 超过上限 %v", [c.name, cpu_limit, max_cpu])
        }
```

### 2.3 Constraint 实例

```yaml
# 生产环境：禁止特权容器
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-production
spec:
  enforcementAction: deny
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
      - apiGroups: ["apps"]
        kinds: ["Deployment", "StatefulSet", "DaemonSet"]
    namespaces:
      - "production"
      - "staging"
    excludedNamespaces:
      - "kube-system"
      - "monitoring"
  parameters: {}

---
# 开发环境：warn 模式
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-development
spec:
  enforcementAction: warn
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    namespaces:
      - "dev-.*"
  parameters: {}
```

### 2.4 Audit 功能

Gatekeeper 定期扫描存量资源，检测不合规资源：

```yaml
# 在 Constraint 中启用 Audit
spec:
  enforcementAction: dryrun    # 只审计，不阻断
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看审计结果
kubectl get constraints -o json | jq '.items[] | {
  name: .metadata.name,
  totalViolations: .status.totalViolations,
  violations: .status.violations
}'
```
## 3. Kyverno

### 3.1 架构优势

Kyverno 使用原生 YAML 编写策略，无需学习 Rego：

- **Validate**：验证资源是否合规
- **Mutate**：自动修改资源
- **Generate**：自动生成关联资源
- **VerifyImages**：验证容器镜像签名

### 3.2 ClusterPolicy 最佳实践

```yaml
# 最佳实践：Pod 安全标准（PSS）基线
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: pod-security-baseline
  annotations:
    policies.kyverno.io/title: Pod Security Baseline
    policies.kyverno.io/description: >-
      强制执行 Kubernetes Pod Security Standards baseline 级别
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: disallow-privileged
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "不允许特权容器"
        pattern:
          spec:
            =(initContainers):
              - =(securityContext):
                  =(privileged): "false"
            containers:
              - =(securityContext):
                  =(privileged): "false"

    - name: disallow-host-network
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "不允许使用主机网络"
        pattern:
          spec:
            =(hostNetwork): "false"

    - name: disallow-host-pid
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "不允许使用主机 PID 命名空间"
        pattern:
          spec:
            =(hostPID): "false"

    - name: require-run-as-nonroot
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "容器必须以非 root 用户运行"
        pattern:
          spec:
            containers:
              - securityContext:
                  runAsNonRoot: "true"
```

```yaml
# 镜像来源验证
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-image-registries
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: validate-registries
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "容器镜像必须来自允许的仓库"
        pattern:
          spec:
            containers:
              - image: "registry.internal/* | gcr.io/google-containers/* | registry.k8s.io/*"
            =(initContainers):
              - image: "registry.internal/* | gcr.io/google-containers/* | registry.k8s.io/*"
```

```yaml
# 自动生成 NetworkPolicy
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-default-networkpolicy
spec:
  rules:
    - name: default-deny-ingress
      match:
        any:
          - resources:
              kinds:
                - Namespace
              selector:
                matchLabels:
                  platform.io/managed-by: naas
      generate:
        synchronize: true
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny-ingress
        namespace: "{{ request.object.metadata.name }}"
        data:
          spec:
            podSelector: {}
            policyTypes:
              - Ingress
```

## 4. 策略即代码工作流

### 4.1 仓库结构

```
policies/
├── base/                           # 基础策略（所有集群）
│   ├── pod-security/
│   │   ├── kustomization.yaml
│   │   ├── disallow-privileged.yaml
│   │   └── require-limits.yaml
│   ├── image-policy/
│   │   └── restrict-registries.yaml
│   └── networking/
│       └── require-networkpolicy.yaml
├── overlays/                       # 环境差异化
│   ├── production/
│   │   ├── kustomization.yaml      # Enforce 模式
│   │   └── patches/
│   │       └── stricter-limits.yaml
│   ├── staging/
│   │   ├── kustomization.yaml      # Warn 模式
│   │   └── patches/
│   │       └── warn-only.yaml
│   └── development/
│       └── kustomization.yaml      # Audit 模式
├── tests/                          # 策略单元测试
│   ├── test-disallow-privileged.yaml
│   └── test-require-limits.yaml
└── .github/
    └── workflows/
        └── policy-ci.yaml
```

### 4.2 策略测试

```yaml
# Kyverno CLI 测试
apiVersion: cli.kyverno.io/v1alpha1
kind: Test
metadata:
  name: test-disallow-privileged
policies:
  - ../base/pod-security/disallow-privileged.yaml
resources:
  - resources/pod-privileged.yaml
  - resources/pod-secure.yaml
results:
  - policy: pod-security-baseline
    rule: disallow-privileged
    resource: test-pod-privileged
    kind: Pod
    result: fail
  - policy: pod-security-baseline
    rule: disallow-privileged
    resource: test-pod-secure
    kind: Pod
    result: pass
```

```bash
# 执行策略测试
kyverno test tests/ --v 3

# 策略 lint
kyverno lint base/
```

### 4.3 CI/CD 流水线

```yaml
# GitHub Actions: 策略 CI
name: Policy CI
on:
  pull_request:
    paths:
      - 'policies/**'

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Kyverno CLI
        run: |
          curl -sL https://github.com/kyverno/kyverno/releases/latest/download/kyverno-cli_linux_x86_64.tar.gz | tar xz
          sudo mv kyverno /usr/local/bin/
      
      - name: Lint Policies
        run: kyverno lint policies/base/
      
      - name: Test Policies
        run: kyverno test policies/tests/ --v 3
      
      - name: Apply to Staging
        if: github.event_name == 'push' && github.ref == 'refs/heads/main'
        run: |
          kubectl apply -k policies/overlays/staging/
```

## 5. 渐进式策略部署

### 5.1 三阶段模型

```
阶段 1: Dry-Run（观测期）
  │  enforcementAction: dryrun / validationFailureAction: Audit
  │  持续时间: 2 周
  │  目标: 发现存量违规，评估影响范围
  │  退出条件: 违规数量稳定且可控
  │
  ▼
阶段 2: Warn（警告期）
  │  enforcementAction: warn / validationFailureAction: Warn
  │  持续时间: 2 周
  │  目标: 通知团队修复，观察新部署违规率
  │  退出条件: 新部署违规率 < 5%
  │
  ▼
阶段 3: Enforce（强制期）
  │  enforcementAction: deny / validationFailureAction: Enforce
  │  持续时间: 永久
  │  目标: 强制执行策略
  │  监控: 持续监控策略拒绝率
```

### 5.2 策略版本管理

```yaml
# 策略注解：版本与阶段信息
metadata:
  annotations:
    policies.kyverno.io/version: "v1.2.0"
    policies.kyverno.io/deploy-phase: "enforce"
    policies.kyverno.io/dryrun-start: "2026-06-15"
    policies.kyverno.io/warn-start: "2026-06-29"
    policies.kyverno.io/enforce-start: "2026-07-13"
    policies.kyverno.io/approved-by: "security-team"
    policies.kyverno.io/ticket: "SEC-2026-0702"
```

### 5.3 策略豁免管理

```yaml
# Kyverno PolicyException
apiVersion: kyverno.io/v2beta1
kind: PolicyException
metadata:
  name: monitoring-exception
  namespace: kyverno
spec:
  exceptions:
    - policyName: pod-security-baseline
      ruleNames:
        - disallow-privileged
  match:
    any:
      - resources:
          kinds:
            - Pod
          namespaces:
            - monitoring
          names:
            - "prometheus-*"
            - "grafana-*"
  conditions:
    any:
      - key: "{{ request.object.metadata.labels.\"platform.io/app\" }}"
        operator: AnyIn
        value:
          - prometheus
          - grafana
```

## 6. 监控与告警

```yaml
# 策略拒绝率告警
groups:
  - name: admission-policy
    rules:
      - alert: HighPolicyDenialRate
        expr: |
          sum(rate(gatekeeper_violations[5m])) > 10
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "策略拒绝率过高: {{ $value }}/s"
          
      - alert: KyvernoPolicyFailure
        expr: |
          increase(kyverno_policy_results_total{result="fail"}[1h]) > 100
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Kyverno 策略 {{ $labels.policy }} 1 小时内失败 {{ $value }} 次"
```

## 7. 最佳实践

- **从小处开始**：先实施 3-5 条核心策略，覆盖安全基线
- **先观测后强制**：所有策略先 dry-run 至少 2 周
- **测试先行**：每条策略必须有对应的单元测试
- **豁免审批**：策略豁免需要安全团队审批，定期复审
- **版本控制**：策略文件纳入 Git，通过 PR 审查变更

## Related

- [[01-namespace-strategy-lifecycle|命名空间规划策略]]
- [[02-label-convention-governance|标签/注解规范治理]]
- [[04-rbac-governance-model|RBAC 治理模型]]

## See Also

- [OPA Gatekeeper 文档](https://open-policy-agent.github.io/gatekeeper/)
- [Kyverno 文档](https://kyverno.io/docs/)
- [Kubernetes Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)


<!-- risk-assessed -->

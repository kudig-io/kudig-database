---
title: 05 - 策略校验与准入控制工具 (Policy Validation)
description: '| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |'
summary: '| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- opa
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
- 策略校验与准入控制工具 (Policy Validation) 是什么
- 如何 策略校验与准入控制工具 (Policy Validation)
- Kubernetes 7 security 最佳实践
trigger_keywords:
- 策略校验与准入控制工具
- Policy
- Validation
- security
prerequisites:
- kubectl-basics
- rbac-basics
- policy-basics
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




# 05 - 策略校验与准入控制工具 (Policy Validation)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

<!-- chunk: 策略引擎对比 -->
## 策略引擎对比

| 工具 (Tool) | 策略语言 (Language) | 核心能力 (Capabilities) | 学习曲线 |
|------------|-------------------|----------------------|---------|
| **OPA/Gatekeeper** | Rego | 通用策略引擎、强大灵活 | 陡峭 |
| **[[Kyverno|Kyverno]]** | YAML | K8s 原生、易上手 | 平缓 |
| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |
| **[[Kubewarden|Kubewarden]]** | WebAssembly | 多语言策略、高性能 | 中等 |

<!-- chunk: Kyverno 生产实践 -->
## Kyverno 生产实践

### 1. 强制镜像来源
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-registry
spec:
  validationFailureAction: enforce
  rules:
  - name: check-registry
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "镜像必须来自可信仓库"
      pattern:
        spec:
          containers:
          - image: "registry.cn-hangzhou.aliyuncs.com/*"
```

### 2. 自动注入 Sidecar
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: inject-sidecar
spec:
  rules:
  - name: add-logging-sidecar
    match:
      any:
      - resources:
          kinds:
          - Deployment
          namespaces:
          - production
    mutate:
      patchStrategicMerge:
        spec:
          template:
            spec:
              containers:
              - name: log-collector
                image: fluent/fluent-bit:latest
```

### 3. 资源配额验证
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resources
spec:
  validationFailureAction: enforce
  rules:
  - name: check-resources
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "必须设置资源 requests 和 limits"
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

<!-- chunk: OPA/Gatekeeper 高级策略 -->
## OPA/Gatekeeper 高级策略

### 1. 禁止特权容器
```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-container
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
```

### 2. 镜像签名验证
```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  image := input.request.object.spec.containers[_].image
  not image_signed(image)
  msg := sprintf("镜像未签名: %v", [image])
}

image_signed(image) {
  # 调用外部签名验证服务
  http.send({
    "method": "GET",
    "url": sprintf("https://notary.example.com/verify?image=%v", [image])
  }).status_code == 200
}
```

<!-- chunk: Polaris 配置审计 -->
## Polaris 配置审计

### 仪表盘指标
- **安全性**: 特权容器、只读根文件系统
- **可靠性**: 探针配置、副本数
- **效率**: 资源限制、镜像标签

### 命令行扫描
```bash
polaris audit --audit-path ./manifests/ --format=json > audit-report.json
```

<!-- chunk: 策略治理最佳实践 -->
## 策略治理最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **分层策略** | 集群级 + 命名空间级 |
| **审计模式** | 先 audit 后 enforce |
| **例外管理** | 使用 Annotation 豁免 |
| **持续监控** | 定期审计现有资源 |
| **文档化** | 策略说明与修复指南 |

---

<!-- chunk: ValidatingAdmissionPolicy (CEL 原生准入) -->
## ValidatingAdmissionPolicy (CEL 原生准入)

> v1.30+ GA，无需部署外部 Webhook，用 CEL 表达式实现原生准入控制。

### 基本示例：禁止 latest 标签

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: disallow-latest-tag
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["deployments", "statefulsets", "daemonsets"]
  validations:
    - expression: |
        object.spec.template.spec.containers.all(c,
          c.image.contains(":") && !c.image.endsWith(":latest")
        )
      message: "镜像必须使用明确版本标签，禁止使用 :latest"
    - expression: |
        object.spec.template.spec.initContainers.all(c,
          c.image.contains(":") && !c.image.endsWith(":latest")
        )
      message: "Init 容器同样禁止 :latest"
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: disallow-latest-tag-binding
spec:
  policyName: disallow-latest-tag
  validationActions: [Deny]
  matchResources:
    namespaceSelector:
      matchExpressions:
        - key: env
          operator: In
          values: ["production", "staging"]
```

### 资源限制强制

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-resource-limits
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["pods"]
  validations:
    - expression: |
        object.spec.containers.all(c,
          has(c.resources.requests) &&
          has(c.resources.requests.cpu) &&
          has(c.resources.requests.memory) &&
          has(c.resources.limits) &&
          has(c.resources.limits.memory)
        )
      message: "所有容器必须设置 CPU requests 和 Memory requests/limits"
    - expression: |
        object.spec.containers.all(c,
          !has(c.resources.limits) || !has(c.resources.limits.cpu) ||
          quantity(c.resources.limits.cpu).compareTo(quantity("4")) <= 0
        )
      message: "CPU limits 不得超过 4 核"
```

### 镜像仓库白名单

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: allowed-registries
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE"]
        resources: ["pods"]
  validations:
    - expression: |
        object.spec.containers.all(c,
          c.image.startsWith("registry.internal.company.com/") ||
          c.image.startsWith("registry.k8s.io/") ||
          c.image.startsWith("quay.io/prometheus/")
        )
      message: "镜像必须来自内部仓库或已审批的公共仓库"
```

### 与 Gatekeeper/Kyverno 对比

| 维度 | ValidatingAdmissionPolicy | Gatekeeper | Kyverno |
|------|--------------------------|------------|--------|
| 部署复杂度 | 零（内置） | 需部署 Controller | 需部署 Controller |
| 策略语言 | CEL | Rego | YAML |
| 变更操作 | 仅验证 | 仅验证 | 验证+变更+生成 |
| 性能影响 | 极低（无网络调用） | 低（本地 Rego） | 低 |
| 外部数据 | 不支持 | 支持 (Provider) | 支持 (API Call) |
| 适用场景 | 简单规则、无外部依赖 | 复杂策略、多数据源 | K8s 原生、变更操作 |

---

<!-- chunk: 策略即代码 CI/CD 集成 -->
## 策略即代码 CI/CD 集成

### 架构概览

```
开发者 PR → CI Pipeline → 策略检查 → 合并 → GitOps Sync → 集群准入
    │            │            │                          │            │
    │            │            ├─ conftest (Rego)        │            ├─ Gatekeeper
    │            │            ├─ kyverno test           │            ├─ Kyverno
    │            │            ├─ kubeconform            │            ├─ VAP (CEL)
    │            │            └─ pluto (API版本)      │            └─ Polaris
    │            └─ 构建镜像 + 签名              └─ ArgoCD/Flux
    └─ 本地 pre-commit hook (可选)
```

### CI Pipeline 示例 (GitHub Actions)

```yaml
# .github/workflows/policy-check.yaml
name: Policy Validation
on: [pull_request]

jobs:
  policy-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      # 1. YAML 格式与 Schema 验证
      - name: Kubeconform Schema Validation
        run: |
          kubeconform -strict -summary \
            -schema-location default \
            -schema-location 'https://raw.githubusercontent.com/datreeio/CRDs-catalog/main/{{.Group}}/{{.ResourceKind}}_{{.ResourceAPIVersion}}.json' \
            ./manifests/

      # 2. 废弃 API 检测
      - name: Pluto Deprecated API Check
        run: |
          pluto detect-files -d ./manifests/ \
            --target-versions 1.33 \
            --output wide

      # 3. OPA/Conftest 策略检查
      - name: Conftest Policy
        run: |
          conftest test ./manifests/ \
            --policy ./policies/ \
            --output table

      # 4. Kyverno 策略测试
      - name: Kyverno Test
        run: |
          kyverno test ./policies/kyverno/ \
            --file-name kyverno-test.yaml

      # 5. 安全扫描
      - name: Polaris Security Audit
        run: |
          polaris audit --audit-path ./manifests/ \
            --format=json > polaris-report.json
          # 检查是否有 danger 级别问题
          cat polaris-report.json | jq '.results[] | select(.severity == "danger")'
```

### Conftest 策略示例

```rego
# policies/deployment.rego
package main

import rego.v1

# 禁止特权容器
deny contains msg if {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    container.securityContext.privileged == true
    msg := sprintf("容器 '%s' 禁止使用特权模式", [container.name])
}

# 必须设置资源限制
deny contains msg if {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    not container.resources.limits.memory
    msg := sprintf("容器 '%s' 必须设置 memory limits", [container.name])
}

# 禁止 hostNetwork
deny contains msg if {
    input.kind == "Deployment"
    input.spec.template.spec.hostNetwork == true
    msg := "禁止使用 hostNetwork"
}

# 镜像必须来自可信仓库
deny contains msg if {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    not startswith(container.image, "registry.internal.company.com/")
    msg := sprintf("镜像 '%s' 不在可信仓库列表中", [container.image])
}
```

### Kyverno 策略测试

```yaml
# kyverno-test.yaml
name: require-registry
tests:
  - name: allow-internal-registry
    policy: require-registry.yaml
    resources:
      - test-resources/good-pod.yaml
    results:
      - policy: require-registry
        rule: check-registry
        result: pass
  - name: deny-external-registry
    policy: require-registry.yaml
    resources:
      - test-resources/bad-pod.yaml
    results:
      - policy: require-registry
        rule: check-registry
        result: fail
---
# test-resources/good-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: good-pod
spec:
  containers:
    - name: app
      image: registry.internal.company.com/myapp:v1.2.3
---
# test-resources/bad-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: bad-pod
spec:
  containers:
    - name: app
      image: docker.io/library/nginx:latest
```

---

<!-- chunk: 多集群策略分发 -->
## 多集群策略分发

### GitOps 策略分发架构

```
策略仓库 (Git)
├── policies/
│   ├── cluster-level/        # 集群级策略
│   │   ├── disallow-privileged.yaml
│   │   ├── require-registry.yaml
│   │   └── resource-limits.yaml
│   ├── namespace-level/      # 命名空间级策略
│   │   ├── production/
│   │   └── development/
│   └── exceptions/           # 例外配置
│       └── legacy-apps.yaml
├── clusters/
│   ├── prod-cn/
│   ├── prod-us/
│   └── staging/
└── kustomization.yaml
```

### 策略例外管理

```yaml
# Kyverno 策略例外
apiVersion: kyverno.io/v2
kind: PolicyException
metadata:
  name: legacy-app-exception
  namespace: legacy
spec:
  exceptions:
    - policyName: require-registry
      ruleNames:
        - check-registry
  match:
    any:
      - resources:
          kinds:
            - Pod
          names:
            - "legacy-monolith-*"
  # 例外有效期（必须设置过期时间）
  # 通过 CronJob 定期检查过期例外
---
# 例外审计 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: policy-exception-audit
  namespace: kyverno
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: audit
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 策略例外审计报告 ==="
                  kubectl get policyexceptions -A -o custom-columns=\
                  'NAME:.metadata.name,NAMESPACE:.metadata.namespace,POLICY:.spec.exceptions[0].policyName'
          restartPolicy: OnFailure
```

---

<!-- chunk: 生产部署架构 -->
## 生产部署架构

### 高可用部署

```yaml
# Gatekeeper HA 部署
apiVersion: v1
kind: Namespace
metadata:
  name: gatekeeper-system
---
# 关键配置：多副本 + 反亲和
apiVersion: apps/v1
kind: Deployment
metadata:
  name: gatekeeper-controller-manager
  namespace: gatekeeper-system
spec:
  replicas: 3  # 生产至少 3 副本
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  control-plane: controller-manager
              topologyKey: kubernetes.io/hostname
      containers:
        - name: manager
          args:
            - --audit-interval=180        # 审计间隔 3 分钟
            - --constraint-violations-limit=50
            - --audit-from-cache=true     # 从缓存审计，减少 API 压力
            - --exempt-namespace=gatekeeper-system
            - --exempt-namespace=kube-system
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
```

### 策略部署顺序与灰度

```bash
# 🟢 策略部署流程（新策略上线）

# Step 1: 审计模式（观察 1-2 周）
# validationFailureAction: Audit
kubectl apply -f new-policy-audit.yaml

# Step 2: 检查违规
kubectl get constrainttemplates
kubectl get constraints -o json | jq '.items[].status.totalViolations'

# Step 3: 修复所有违规后，切换为强制模式
# validationFailureAction: Enforce
kubectl apply -f new-policy-enforce.yaml

# Step 4: 验证
kubectl get constraints -o json | jq '.items[].status'
```

### 监控与告警

```yaml
# PrometheusRule: 策略违规告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: policy-alerts
  namespace: monitoring
spec:
  groups:
    - name: policy.rules
      rules:
        - alert: PolicyViolationsHigh
          expr: |
            gatekeeper_violations{enforcement_action="deny"} > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "策略拒绝事件过多 ({{ $value }})"
            runbook: "检查是否有新部署触发策略拒绝"

        - alert: PolicyAuditViolations
          expr: |
            increase(gatekeeper_violations{enforcement_action="audit"}[1h]) > 50
          for: 10m
          labels:
            severity: info
          annotations:
            summary: "审计模式策略违规增加"

        - alert: GatekeeperControllerDown
          expr: |
            up{job="gatekeeper-controller-manager"} == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Gatekeeper 控制器不可用，策略未执行"
```

---

<!-- chunk: 策略成熟度模型 -->
## 策略成熟度模型

| 级别 | 名称 | 特征 | 工具 | 建议 |
|------|------|------|------|------|
| L1 | 无策略 | 无准入控制，任意部署 | 无 | 立即开始 |
| L2 | 基础审计 | 部署 Polaris/Trivy，仅报告 | Polaris, Trivy | 1-2 周 |
| L3 | 关键强制 | 强制镜像源、资源限制、禁止特权 | Kyverno/Gatekeeper | 1-2 月 |
| L4 | 全面策略 | 网络、安全上下文、标签、配额全覆盖 | 多工具组合 | 3-6 月 |
| L5 | 策略即代码 | CI/CD 集成、自动测试、例外管理、多集群 | 全套体系 | 6-12 月 |

### 快速启动路线图

```
第 1 周: 部署 Kyverno + 3 条基础策略 (audit 模式)
    ├── 禁止特权容器
    ├── 强制资源限制
    └── 镜像仓库白名单

第 2-3 周: 修复违规，切换为 enforce

第 4 周: 添加 CI 策略检查 (conftest/pluto)

第 2 月: 扩展策略集 (NetworkPolicy、标签、安全上下文)

第 3 月: 多集群分发 + 例外管理 + 监控告警
```


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 安全 KUDIG Database — Global MOC
- [[安全/README.md|Security Domain]]
- [[安全/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## See Also

- 03-runtime-security-defense
- 04-audit-logging-compliance
- 06-pod-security-standards
- 07-rbac-matrix-configuration

- [[安全/README.md|返回目录]]

## Related

- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->

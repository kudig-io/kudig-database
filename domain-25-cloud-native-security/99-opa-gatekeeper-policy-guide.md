# OPA Gatekeeper 策略即代码实践指南

> **适用版本**: Gatekeeper v3.18 / OPA v1.2  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、OPA 与 Gatekeeper 架构](#一opa-与-gatekeeper-架构)
- [二、安装部署](#二安装部署)
- [三、ConstraintTemplate 定义策略](#三constrainttemplate-定义策略)
- [四、Constraint 应用策略](#四constraint-应用策略)
- [五、内置策略库](#五内置策略库)
- [六、自定义 Rego 策略](#六自定义-rego-策略)
- [七、审计与违规报告](#七审计与违规报告)
- [八、OPA vs Kyverno 对比](#八opa-vs-kyverno-对比)
- [九、与 CI/CD 集成](#九与-cicd-集成)

---

## 一、OPA 与 Gatekeeper 架构

```
OPA (Open Policy Agent) 生态
├── OPA Core (通用策略引擎)
│   ├── Rego 策略语言
│   ├── 任意 JSON 输入评估
│   └── 可嵌入任何系统
│
├── Gatekeeper (K8s Admission Controller)
│   ├── ConstraintTemplate (策略模板 CRD)
│   ├── Constraint (策略实例 CRD)
│   ├── Mutation (变异 CRD)
│   ├── Config (审计配置)
│   └── Audit Controller (定期扫描)
│
└── 其他集成
    ├── Conftest (CI/CD 中测试)
    ├── Terraform Sentinel 替代
    └── Envoy 授权过滤器

Gatekeeper 工作流程
Admission Webhook ──► OPA/Gatekeeper ──► 允许 / 拒绝 / 变异
                              │
                              └── 读取 ConstraintTemplate (Rego)
                              └── 读取 Constraint (参数)
```

---

## 二、安装部署

```bash
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm repo update

helm install gatekeeper gatekeeper/gatekeeper \
  --namespace gatekeeper-system \
  --create-namespace \
  --version 3.18.0 \
  --set enableExternalData=true \
  --set validatingWebhookTimeoutSeconds=5 \
  --set mutatingWebhookTimeoutSeconds=2 \
  --set auditInterval=60
```

### 生产级配置

```yaml
# values-gatekeeper.yaml
replicas: 3

resources:
  requests:
    cpu: 100m
    memory: 256Mi
  limits:
    cpu: 1000m
    memory: 1Gi

# 启用变异
enableMutation: true

# 审计配置
auditInterval: 60
auditMatchKindOnly: false
constraintViolationsLimit: 20
auditFromCache: true
auditChunkSize: 500

# Webhook 配置
validatingWebhookFailurePolicy: Ignore  # 或 Fail
validatingWebhookTimeoutSeconds: 5
mutatingWebhookFailurePolicy: Ignore
mutatingWebhookTimeoutSeconds: 2
```

---

## 三、ConstraintTemplate 定义策略

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
                type: object
                properties:
                  key:
                    type: string
                  allowedRegex:
                    type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredlabels

        violation[{"msg": msg}] {
          provided := {label | input.review.object.metadata.labels[label]}
          required := {label | label := input.parameters.labels[_].key}
          missing := required - provided
          count(missing) > 0
          msg := sprintf("必须包含标签: %v", [missing])
        }

        violation[{"msg": msg}] {
          label := input.parameters.labels[_]
          value := input.review.object.metadata.labels[label.key]
          label.allowedRegex
          not regex.match(label.allowedRegex, value)
          msg := sprintf("标签 %v 的值 %v 不符合正则 %v", [label.key, value, label.allowedRegex])
        }
```

---

## 四、Constraint 应用策略

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-cost-labels
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Namespace"]
    excludedNamespaces: ["kube-system", "gatekeeper-system"]
  parameters:
    labels:
      - key: cost-center
        allowedRegex: "^team-[a-z]+$"
      - key: environment
        allowedRegex: "^(production|staging|development)$"
---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-app-labels
spec:
  match:
    kinds:
      - apiGroups: ["apps"]
        kinds: ["Deployment", "StatefulSet"]
  parameters:
    labels:
      - key: app.kubernetes.io/name
      - key: app.kubernetes.io/version
      - key: app.kubernetes.io/component
```

---

## 五、内置策略库

### 5.1 Gatekeeper Policy Library (官方)

```bash
# 安装策略库
kubectl apply -f https://raw.githubusercontent.com/open-policy-agent/gatekeeper-library/master/library/general/allowedrepos/template.yaml
kubectl apply -f https://raw.githubusercontent.com/open-policy-agent/gatekeeper-library/master/library/general/allowedrepos/constraint.yaml
```

### 5.2 常用内置策略

| 策略 | 用途 | 安装 |
|:---|:---|:---|
| Allowed Repositories | 限制镜像来源 | general/allowedrepos |
| Container Limits | 强制资源限制 | general/containerlimits |
| Required Probes | 强制健康检查 | general/requiredprobes |
| Unique Ingress Host | 唯一 Ingress 域名 | general/uniqueingresshost |
| Disallowed Tags | 禁止 latest 标签 | general/disallowedtags |
| Block Node Port | 禁止 NodePort | general/blocknodeport |
| HTTPS Only | 强制 HTTPS | general/httpsonly |
| Storage Class | 限制存储类 | general/storageclass |
| PSP Replacement | Pod 安全策略替代 | pod-security-policy |

---

## 六、自定义 Rego 策略

### 6.1 禁止特权容器

```yaml
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

        violation[{"msg": msg}] {
          c := input_containers[_]
          c.securityContext.privileged
          msg := sprintf("容器 %v 不允许以特权模式运行", [c.name])
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

### 6.2 强制只读根文件系统

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPRootFiles
metadata:
  name: require-readonly-rootfs
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
  parameters:
    exemptImages:
      - "gcr.io/istio-release/*"
```

### 6.3 变异策略 (Mutation)

```yaml
apiVersion: mutations.gatekeeper.sh/v1
kind: AssignMetadata
metadata:
  name: add-cost-center
spec:
  match:
    scope: Namespaced
    kinds:
      - apiGroups: [""]
        kinds: ["Namespace"]
  location: "metadata.labels.cost-center"
  parameters:
    assign:
      value: "team-platform"
---
apiVersion: mutations.gatekeeper.sh/v1
kind: Assign
metadata:
  name: add-security-context
spec:
  applyTo:
    - groups: [""]
      kinds: ["Pod"]
      versions: ["v1"]
  match:
    scope: Namespaced
  location: "spec.containers[name:*].securityContext.allowPrivilegeEscalation"
  parameters:
    assign:
      value: false
```

---

## 七、审计与违规报告

### 7.1 查看违规

```bash
# 查看所有违规
kubectl get constraints -o json | jq '.items[] | {name: .metadata.name, violations: .status.violations}'

# 查看特定约束的违规
kubectl get k8srequiredlabels require-cost-labels -o json | jq '.status.violations'

# 查看审计日志
kubectl logs -n gatekeeper-system deployment/gatekeeper-audit
```

### 7.2 Prometheus 指标

```yaml
- alert: GatekeeperViolations
  expr: gatekeeper_violations > 0
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "Gatekeeper 发现 {{ $value }} 个策略违规"

- alert: GatekeeperWebhookLatencyHigh
  expr: rate(gatekeeper_validation_request_duration_seconds_sum[5m]) / rate(gatekeeper_validation_request_duration_seconds_count[5m]) > 1
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "Gatekeeper Webhook 延迟过高"
```

---

## 八、OPA vs Kyverno 对比

| 维度 | OPA Gatekeeper | Kyverno |
|:---|:---|:---|
| **策略语言** | Rego (专用 DSL) | YAML (K8s 原生) |
| **学习曲线** | 高 (需学 Rego) | 低 (YAML 即可) |
| **灵活性** | 极高 (通用策略引擎) | 高 (K8s 场景优化) |
| **性能** | 中等 (Rego 评估) | 高 (Go 原生) |
| **变异能力** | 支持 | 支持 |
| **外部数据** | 支持 (External Data) | 支持 (API Call) |
| **CLI 测试** | conftest | kyverno cli |
| **社区生态** | 广泛 (跨平台) | K8s 专注 |
| **非 K8s 场景** | 支持 | 不支持 |
| **入门推荐** | 复杂策略/多平台 | 快速落地/K8s 专用 |

### 选型决策

```
选择 OPA Gatekeeper 如果:
  ✅ 需要跨平台策略统一 (K8s + Terraform + Envoy)
  ✅ 团队有 Rego 能力
  ✅ 需要极高灵活性
  ✅ 已有 OPA 基础设施

选择 Kyverno 如果:
  ✅ 快速落地，降低学习成本
  ✅ 纯 K8s 场景
  ✅ 团队熟悉 K8s YAML
  ✅ 需要内置丰富策略库
```

---

## 九、与 CI/CD 集成

### 9.1 Conftest (CI 中测试 Rego)

```bash
# 安装 conftest
brew install conftest

# 测试 K8s manifest
conftest test deployment.yaml -p policies/

# 测试 Helm chart
helm template mychart | conftest test -
```

### 9.2 GitHub Actions 集成

```yaml
name: Gatekeeper Policy Check
on: [pull_request]
jobs:
  policy-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Conftest
        run: |
          wget https://github.com/open-policy-agent/conftest/releases/download/v0.55.0/conftest_0.55.0_Linux_x86_64.tar.gz
          tar xzf conftest_0.55.0_Linux_x86_64.tar.gz
          sudo mv conftest /usr/local/bin/
      
      - name: Test Kubernetes Manifests
        run: |
          conftest test manifests/ -p policies/k8s/
      
      - name: Test Helm Charts
        run: |
          helm template charts/myapp | conftest test - -p policies/k8s/
```

---

## 参考链接

- [OPA 官方文档](https://www.openpolicyagent.org/docs/latest/)
- [Gatekeeper 文档](https://open-policy-agent.github.io/gatekeeper/website/docs/)
- [Gatekeeper Policy Library](https://github.com/open-policy-agent/gatekeeper-library)
- [Rego 语言参考](https://www.openpolicyagent.org/docs/latest/policy-reference/)
- [Conftest](https://www.conftest.dev/)

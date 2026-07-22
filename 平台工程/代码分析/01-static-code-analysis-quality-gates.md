---
title: Static Code Analysis and Quality Gates — CI/CD Integration for K8s Manifests
description: K8s 代码分析 — 静态分析工具链、Manifest Lint、策略扫描、安全扫描、质量门禁、CI 集成
summary: 在 CI/CD 流水线中集成静态代码分析与质量门禁，确保 K8s 清单和代码的生产质量
category: practice
tags:
- static-analysis
- quality-gate
- linting
- security-scan
- ci-cd
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: platform-engineering
---
# 静态代码分析与质量门禁

> CI/CD 中的自动化质量保障：Manifest Lint、策略扫描、安全检测。

## 分析工具全景

| 工具 | 类型 | 检测内容 | 集成方式 |
|------|------|----------|----------|
| kube-linter | Manifest Lint | 最佳实践违规 | CLI/CI |
| kube-score | Manifest 评分 | 资源/探针/安全 | CLI/CI |
| checkov | IaC 扫描 | 安全 misconfig | CLI/CI/IDE |
| trivy | 安全扫描 | 镜像漏洞/Secret | CLI/CI |
| conftest | 策略验证 | OPA 策略合规 | CLI/CI |
| datree | 策略引擎 | 自定义规则 | CLI/CI/Admission |
| semgrep | 代码分析 | 代码漏洞 | CLI/CI/IDE |
| sonarqube | 代码质量 | 复杂度/重复/漏洞 | CI/Server |
| hadolint | Dockerfile | 镜像构建最佳实践 | CLI/CI |
| yamllint | YAML 格式 | 语法/风格 | CLI/CI |

## K8s Manifest 分析

### kube-linter 配置

```yaml
# .kube-linter.yaml
checks:
  addAllBuiltIn: true
  exclude:
    - "no-read-only-root-fs"  # 某些应用需要写文件系统
customChecks:
  - name: "required-team-label"
    description: "所有资源必须有 team 标签"
    scope:
      objectKinds:
        - Deployment
        - StatefulSet
        - DaemonSet
    template: "required-label"
    params:
      label: "team"
  - name: "max-replicas"
    description: "生产环境副本数不超过 50"
    scope:
      objectKinds:
        - Deployment
    template: "max-replicas"
    params:
      maxReplicas: 50
```

```bash
# 运行 kube-linter
kube-linter lint ./k8s/ --config .kube-linter.yaml
# 输出:
# ✗ deployment/api-server: no resource requests specified
# ✗ deployment/api-server: no liveness probe specified
# ✗ deployment/worker: runAsNonRoot is not set
```

### kube-score 评分

```bash
# 评分（0-10，10 为最佳）
kube-score score ./k8s/ --output-format json | jq '.[] | {name: .Name, score: .Score}'
# 关注项:
# - Pod Probes（探针配置）
# - Container Resources（资源限制）
# - Container Security Context（安全上下文）
# - Pod NetworkPolicy（网络策略）
# - Deployment（滚动更新策略）
```

## 安全扫描

### Trivy 全面扫描

```yaml
# GitHub Actions — Trivy 扫描
name: Security Scan
on: [pull_request]

jobs:
  trivy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      # 镜像漏洞扫描
      - name: Scan Image
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: 'registry.example.com/app:${{ github.sha }}'
          format: 'sarif'
          output: 'trivy-image.sarif'
          severity: 'CRITICAL,HIGH'
          exit-code: '1'  # 发现高危漏洞则失败

      # K8s Manifest 扫描
      - name: Scan Manifests
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'config'
          scan-ref: './k8s/'
          format: 'table'
          exit-code: '1'
          severity: 'CRITICAL,HIGH'

      # Secret 扫描
      - name: Scan Secrets
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'fs'
          scan-ref: '.'
          scanners: 'secret'
          exit-code: '1'
```

### Checkov（IaC 安全）

```bash
# 扫描 K8s 清单
checkov -d ./k8s/ --framework kubernetes --compact

# 扫描 Terraform
checkov -d ./terraform/ --framework terraform

# 自定义策略
# policies/require-pdb.py
from checkov.common.models.enums import CheckResult, CheckCategories
from checkov.kubernetes.checks.resource.base_check import BaseResourceCheck

class RequirePDB(BaseResourceCheck):
    def __init__(self):
        name = "Ensure PodDisruptionBudget exists for Deployments"
        id = "CUSTOM_K8S_001"
        categories = [CheckCategories.KUBERNETES]
        super().__init__(name=name, id=id, categories=categories)

    def scan_resource_conf(self, conf):
        # 检查逻辑
        return CheckResult.PASSED
```

## 质量门禁流水线

### 完整 CI 质量门禁

```yaml
# GitHub Actions — 质量门禁
name: Quality Gate
on:
  pull_request:
    paths:
      - 'k8s/**'
      - 'src/**'
      - 'Dockerfile'

jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: YAML Lint
        run: yamllint -c .yamllint.yaml k8s/
      - name: Kube Lint
        run: |
          kube-linter lint k8s/ --config .kube-linter.yaml
      - name: Dockerfile Lint
        run: hadolint Dockerfile

  security:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Trivy Config Scan
        run: trivy config --exit-code 1 --severity HIGH,CRITICAL k8s/
      - name: Secret Scan
        run: trivy fs --scanners secret --exit-code 1 .
      - name: Policy Check
        run: conftest test k8s/ --policy policies/

  build-scan:
    runs-on: ubuntu-latest
    needs: [lint, security]
    steps:
      - uses: actions/checkout@v4
      - name: Build Image
        run: docker build -t app:${{ github.sha }} .
      - name: Image Scan
        run: trivy image --exit-code 1 --severity HIGH,CRITICAL app:${{ github.sha }}

  quality-gate:
    runs-on: ubuntu-latest
    needs: [lint, security, build-scan]
    steps:
      - name: All Checks Passed
        run: echo "✅ 质量门禁通过，可以合并"
```

### OPA/Conftest 策略验证

```rego
# policies/deployment.rego
package main

# 禁止使用 latest 标签
deny[msg] {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    endswith(container.image, ":latest")
    msg := sprintf("容器 %s 使用了 latest 标签", [container.name])
}

# 必须设置资源限制
deny[msg] {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    not container.resources.limits.memory
    msg := sprintf("容器 %s 缺少内存限制", [container.name])
}

# 必须设置探针
deny[msg] {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    not container.livenessProbe
    msg := sprintf("容器 %s 缺少存活探针", [container.name])
}

# 禁止特权容器
deny[msg] {
    input.kind == "Deployment"
    container := input.spec.template.spec.containers[_]
    container.securityContext.privileged == true
    msg := sprintf("容器 %s 不允许特权模式", [container.name])
}
```

```bash
# 运行策略检查
conftest test k8s/ --policy policies/ --output table
# FAIL - k8s/deployment.yaml - 容器 api 使用了 latest 标签
# FAIL - k8s/deployment.yaml - 容器 worker 缺少内存限制
```

## 代码质量分析

### SonarQube 集成

```yaml
# sonar-project.properties
sonar.projectKey=my-service
sonar.sources=src/
sonar.tests=test/
sonar.language=go
sonar.go.coverage.reportPaths=coverage.out
sonar.qualitygate.wait=true

# 质量门禁标准:
# - 新代码覆盖率 > 80%
# - 新代码重复率 < 3%
# - 无新增 Bug/漏洞
# - 可维护性评级 A
```

### Semgrep 自定义规则

```yaml
# .semgrep/k8s-security.yaml
rules:
  - id: no-hardcoded-secrets
    patterns:
      - pattern: |
          password = "..."
      - pattern-not: |
          password = os.environ.get(...)
    message: "禁止硬编码密码，使用环境变量或 Secret"
    languages: [python, go, javascript]
    severity: ERROR

  - id: require-tls-verify
    patterns:
      - pattern: |
          tls.Config{InsecureSkipVerify: true}
    message: "生产代码禁止跳过 TLS 验证"
    languages: [go]
    severity: ERROR
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 左移检测 | 在 PR 阶段拦截问题 |
| 分层门禁 | Lint → Security → Build → Deploy |
| 快速反馈 | 总时间 < 10 min |
| 自定义规则 | 团队特定规范 |
| 渐进严格 | 新规则先 warn 后 error |
| IDE 集成 | 开发者本地即时反馈 |
| 豁免机制 | 有审批的例外流程 |
| 度量趋势 | 追踪质量指标变化 |

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| kube-linter 报错过多 | 规则太严格 | 调整 `.kube-linter.yaml` 排除规则 |
| Trivy 扫描超时 | 镜像太大 | 使用 `--timeout 10m` 增加超时 |
| conftest 策略不生效 | 策略语法错误 | `conftest verify` 验证策略 |
| CI 流水线太慢 | 扫描步骤串行 | 并行执行 lint/security 任务 |
| 误报太多 | 规则不适合项目 | 添加豁免注释或调整策略 |

### 豁免机制

```yaml
# 临时豁免（带过期时间）
# kube-linter:ignore {"checks":["no-latest-image"],"reason":"测试环境","expiry":"2026-08-01"}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: test-app
spec:
  template:
    spec:
      containers:
        - name: app
          image: myapp:latest  # 临时使用 latest
```

```rego
# OPA 策略豁免
package main

import future.keywords.if

# 豁免列表
exempt_namespaces := {"kube-system", "monitoring", "logging"}

is_exempt if {
    input.metadata.namespace in exempt_namespaces
}

deny[msg] if {
    not is_exempt
    # ... 检查逻辑
}
```

## 质量度量仪表板

### Prometheus 指标

```yaml
# 质量门禁指标
quality_gate_checks_total{tool="kube-linter",status="pass|fail"}
quality_gate_checks_total{tool="trivy",status="pass|fail"}
quality_gate_checks_total{tool="conftest",status="pass|fail"}

# 问题趋势
quality_gate_issues_total{severity="critical|high|medium|low"}
quality_gate_fix_rate  # 修复率
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "代码质量概览",
    "panels": [
      {
        "title": "质量门禁通过率",
        "type": "stat",
        "targets": [
          { "expr": "sum(rate(quality_gate_checks_total{status=\"pass\"}[7d])) / sum(rate(quality_gate_checks_total[7d])) * 100" }
        ]
      },
      {
        "title": "问题趋势",
        "type": "graph",
        "targets": [
          { "expr": "sum(quality_gate_issues_total) by (severity)" }
        ]
      }
    ]
  }
}
```

## Related

- [[平台工程/代码分析/index.md|代码分析]]
- [[发布变更/测试质量/index.md|测试质量]]
- [[安全/供应链/index.md|供应链安全]]
- [[安全/策略治理/index.md|策略治理]]

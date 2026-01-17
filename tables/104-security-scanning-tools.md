# 安全扫描工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Trivy](https://trivy.dev/) | [Grype](https://github.com/anchore/grype)

## 工具对比

| 工具 | 扫描类型 | 速度 | 准确率 | SBOM | 离线模式 | K8s集成 | 生产推荐 |
|------|---------|------|--------|------|---------|---------|---------|
| **Trivy** | 镜像/IaC/SBOM | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ✅ | ✅ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Grype** | 镜像/目录/SBOM | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ✅ | ✅ | ⭐⭐⭐⭐ | 推荐 |
| **Snyk** | 镜像/代码/IaC | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ✅ | ❌ | ⭐⭐⭐⭐ | SaaS场景 |
| **Clair** | 镜像 | ⭐⭐⭐ | ⭐⭐⭐⭐ | ❌ | ✅ | ⭐⭐⭐ | Harbor集成 |
| **Anchore** | 镜像/SBOM | ⭐⭐⭐ | ⭐⭐⭐⭐ | ✅ | ✅ | ⭐⭐⭐⭐ | 企业级 |

---

## Trivy 生产实践

### 核心功能

```
Trivy扫描范围
├── 容器镜像 (漏洞+误配置)
├── 文件系统 (操作系统包+语言依赖)
├── Git仓库 (IaC配置+Secret泄露)
├── Kubernetes集群 (资源安全配置)
└── SBOM (CycloneDX/SPDX)
```

### 镜像漏洞扫描

```bash
# 基础扫描
trivy image nginx:1.25

# 仅显示高危和严重漏洞
trivy image --severity HIGH,CRITICAL nginx:1.25

# 输出JSON报告
trivy image -f json -o report.json nginx:1.25

# 扫描私有Registry
trivy image --username admin --password $PASS \
  registry.company.com/myapp:v1.2.3

# 忽略未修复的漏洞
trivy image --ignore-unfixed nginx:1.25

# 扫描并检查策略(非零退出)
trivy image --exit-code 1 --severity CRITICAL nginx:1.25
```

### Kubernetes 集群扫描

```bash
# 扫描当前集群
trivy k8s --report summary

# 扫描特定命名空间
trivy k8s -n production --severity HIGH,CRITICAL

# 扫描并输出报告
trivy k8s --report all -f json -o k8s-audit.json

# 检查准入策略合规性
trivy k8s --compliance k8s-pss-baseline
```

### IaC 配置扫描

```bash
# 扫描Kubernetes YAML
trivy config ./manifests/

# 扫描Helm Chart
trivy config ./charts/myapp/

# 扫描Terraform
trivy config ./terraform/

# 自定义策略
trivy config --policy ./custom-policies/ ./manifests/
```

### Trivy Operator (K8s自动化)

```yaml
# 安装Trivy Operator
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm install trivy-operator aqua/trivy-operator \
  --namespace trivy-system --create-namespace \
  --set trivy.ignoreUnfixed=true

# VulnerabilityReport CRD (自动生成)
apiVersion: aquasecurity.github.io/v1alpha1
kind: VulnerabilityReport
metadata:
  name: deployment-myapp-nginx
  namespace: production
spec:
  artifact:
    repository: nginx
    tag: "1.25"
  scanner:
    name: Trivy
    version: 0.48.0
  summary:
    criticalCount: 2
    highCount: 5
    mediumCount: 12
    lowCount: 30
  vulnerabilities:
    - vulnerabilityID: CVE-2024-1234
      severity: CRITICAL
      title: Buffer overflow in libssl
      installedVersion: 1.1.1k
      fixedVersion: 1.1.1w
      primaryLink: https://nvd.nist.gov/vuln/detail/CVE-2024-1234
```

### 准入控制集成

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: trivy-webhook
webhooks:
  - name: trivy.webhook.aquasecurity.com
    admissionReviewVersions: ["v1"]
    clientConfig:
      service:
        name: trivy-operator-webhook
        namespace: trivy-system
        path: /validate
    rules:
      - operations: ["CREATE", "UPDATE"]
        apiGroups: [""]
        apiVersions: ["v1"]
        resources: ["pods"]
    failurePolicy: Fail  # 拒绝不合规部署
    sideEffects: None
```

---

## Grype 使用指南

### 快速扫描

```bash
# 安装Grype
curl -sSfL https://raw.githubusercontent.com/anchore/grype/main/install.sh | sh

# 扫描镜像
grype nginx:1.25

# 扫描目录
grype dir:/path/to/project

# 从SBOM扫描
grype sbom:./sbom.json

# 输出格式
grype nginx:1.25 -o json
grype nginx:1.25 -o table
grype nginx:1.25 -o sarif  # GitHub集成
```

### 策略配置 (.grype.yaml)

```yaml
# 忽略特定CVE
ignore:
  - vulnerability: CVE-2024-1234
    fix-state: wont-fix
  - vulnerability: CVE-2023-5678
    namespace: debian:distro:debian:11
    package:
      name: libssl1.1
      version: 1.1.1k

# 仅扫描特定严重性
match:
  severity:
    - critical
    - high

# 失败条件
fail-on-severity: high

# 输出配置
output:
  quiet: false
  template: |
    Found {{ len .Matches }} vulnerabilities
```

### CI/CD 集成

```yaml
# GitLab CI
security-scan:
  stage: test
  image: anchore/grype:latest
  script:
    - grype registry.company.com/myapp:$CI_COMMIT_SHA \
        --fail-on high \
        -o json > grype-report.json
  artifacts:
    reports:
      dependency_scanning: grype-report.json

# GitHub Actions
- name: Grype Scan
  uses: anchore/scan-action@v3
  with:
    image: "myapp:latest"
    fail-build: true
    severity-cutoff: high
```

---

## Snyk 企业实践

### 容器镜像扫描

```bash
# 安装Snyk CLI
npm install -g snyk

# 认证
snyk auth

# 扫描镜像
snyk container test nginx:1.25

# 监控镜像(持续跟踪)
snyk container monitor nginx:1.25 \
  --project-name=prod-nginx \
  --org=my-org

# 生成HTML报告
snyk container test nginx:1.25 --json | snyk-to-html -o report.html
```

### Kubernetes 集成

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: snyk-monitor
spec:
  serviceAccountName: snyk-monitor
  containers:
    - name: snyk-monitor
      image: snyk/kubernetes-monitor:latest
      env:
        - name: SNYK_INTEGRATION_ID
          valueFrom:
            secretKeyRef:
              name: snyk-credentials
              key: integration-id
        - name: SNYK_WATCH_NAMESPACE
          value: "production,staging"
      resources:
        requests:
          cpu: 250m
          memory: 400Mi
```

---

## Harbor + Clair 集成

### Harbor 自动扫描配置

```yaml
# harbor.yml
clair:
  enabled: true
  adapter_url: https://harbor.company.com/clair-adapter
  update_interval: 12  # 漏洞库更新间隔(小时)

# 自动扫描策略
scan_all_policy:
  type: daily
  parameter:
    daily_time: 0  # 每天凌晨扫描
```

### Webhook 通知

```json
{
  "type": "scanCompleted",
  "event_data": {
    "repository": "library/nginx",
    "tag": "1.25",
    "scan_overview": {
      "total": 45,
      "fixable": 30,
      "summary": {
        "critical": 2,
        "high": 5,
        "medium": 12,
        "low": 26
      }
    }
  }
}
```

---

## SBOM 生成与管理

### Syft - SBOM 生成工具

```bash
# 安装Syft
curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh

# 生成SBOM
syft nginx:1.25 -o spdx-json > nginx-sbom.json
syft nginx:1.25 -o cyclonedx-json > nginx-sbom-cdx.json

# 从Dockerfile生成
syft dir:. -o spdx-json

# 扫描SBOM
grype sbom:./nginx-sbom.json
trivy sbom ./nginx-sbom-cdx.json
```

### Kubernetes SBOM 管理

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: myapp-sbom
  namespace: production
  annotations:
    sbom.format: cyclonedx-json
    sbom.version: "1.5"
data:
  sbom.json: |
    {
      "bomFormat": "CycloneDX",
      "specVersion": "1.5",
      "components": [...]
    }
```

---

## 准入控制策略

### Open Policy Agent (OPA)

```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  image := input.request.object.spec.containers[_].image
  
  # 调用Trivy扫描(外部数据)
  scan_result := trivy_scan(image)
  scan_result.critical_count > 0
  
  msg := sprintf("镜像 %v 存在 %v 个严重漏洞", [image, scan_result.critical_count])
}

trivy_scan(image) = result {
  # 通过HTTP调用Trivy API
  response := http.send({
    "method": "POST",
    "url": "http://trivy-server:8080/scan",
    "body": {"image": image}
  })
  result := response.body
}
```

### Kyverno 策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: check-vulnerabilities
spec:
  validationFailureAction: enforce
  background: false
  webhookTimeoutSeconds: 30
  rules:
    - name: check-image-vulnerabilities
      match:
        any:
          - resources:
              kinds:
                - Pod
      verifyImages:
        - imageReferences:
            - "*"
          attestations:
            - predicateType: https://cosign.sigstore.dev/attestation/vuln/v1
              conditions:
                - all:
                    - key: "{{ vulnerability.severity }}"
                      operator: NotIn
                      value: ["CRITICAL", "HIGH"]
```

---

## 持续监控方案

### Prometheus 指标监控

```yaml
# ServiceMonitor for Trivy Operator
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: trivy-operator
  namespace: trivy-system
spec:
  selector:
    matchLabels:
      app: trivy-operator
  endpoints:
    - port: metrics
      interval: 30s

# PrometheusRule 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: trivy-alerts
spec:
  groups:
    - name: vulnerability-alerts
      interval: 5m
      rules:
        - alert: CriticalVulnerabilityDetected
          expr: |
            trivy_vulnerability_count{severity="CRITICAL"} > 0
          for: 10m
          labels:
            severity: critical
          annotations:
            summary: "检测到严重漏洞"
            description: "{{ $labels.namespace }}/{{ $labels.resource }} 存在 {{ $value }} 个严重漏洞"
```

---

## 工具选型建议

| 场景 | 推荐方案 | 原因 |
|------|---------|------|
| **Kubernetes集群** | Trivy Operator | 自动化扫描、CRD集成 |
| **CI/CD流水线** | Trivy + Grype | 速度快、准确率高 |
| **企业合规** | Snyk + Anchore | 商业支持、策略管理 |
| **Harbor用户** | Clair (内置) | 无缝集成、统一界面 |
| **离线环境** | Trivy离线模式 | 完整漏洞库下载 |

---

## 最佳实践

```yaml
# 1. 多阶段扫描
pipeline:
  - stage: build
    scan: Dockerfile (IaC)
  - stage: registry-push
    scan: 镜像 (漏洞)
  - stage: deploy
    scan: Kubernetes资源 (配置)

# 2. 分级处理策略
severity:
  CRITICAL: 阻止部署
  HIGH: 需人工审批
  MEDIUM: 记录日志
  LOW: 忽略

# 3. 漏洞库更新
schedule: 每天自动更新
offline: 定期同步离线库

# 4. 报告归档
storage: S3/OSS
retention: 90天
format: JSON + HTML
```

---

## 常见问题

**Q: 扫描速度慢如何优化?**  
A: 使用本地漏洞库缓存、启用并行扫描、仅扫描变更层。

**Q: 误报如何处理?**  
A: 配置白名单、使用SBOM精确匹配、定期更新漏洞库。

**Q: 离线环境如何使用?**  
A: Trivy支持离线模式，定期下载漏洞库：`trivy image --download-db-only`。

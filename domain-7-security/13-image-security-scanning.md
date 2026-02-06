# 13 - 镜像安全扫描与漏洞管理

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 中高级 | **参考**: [Trivy Documentation](https://aquasecurity.github.io/trivy/) | [Clair Documentation](https://github.com/quay/clair)

## 一、镜像安全扫描架构

### 1.1 容器镜像安全扫描体系

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Container Image Security Scanning Architecture                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                           Image Lifecycle Security                             │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   Build      │  │   Registry   │  │  Runtime     │  │   Post-Run   │       │ │
│  │  │   构建期     │  │   仓库期     │  │   运行期     │  │   运行后     │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │    Scanning Engines           │                          │ │
│  │                    │    扫描引擎                  │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                        Vulnerability Databases                                 │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                           CVE Sources                                   │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   NVD        │  │   RedHat    │  │   Debian    │  │   Alpine    │         │   │ │
│  │  │  │   国家漏洞库  │  │   企业源    │  │   Debian    │  │   Alpine    │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                        Configuration Checks                             │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   CIS        │  │   Docker    │  │   K8s       │  │   Custom    │         │   │ │
│  │  │  │   基准检查   │  │   最佳实践   │  │   安全检查   │  │   策略      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                          Policy Enforcement                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Admission Control                               │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   OPA        │  │   Kyverno   │  │   Harbor    │  │   Anchore   │         │   │ │
│  │  │  │   策略引擎   │  │   策略管理   │  │   镜像仓库   │  │   企业平台   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 扫描工具对比分析

| 工具 | 类型 | 优势 | 劣势 | 适用场景 |
|-----|------|------|------|---------|
| **Trivy** | 静态扫描 | 快速、轻量、CVE覆盖全 | 功能相对简单 | CI/CD集成 |
| **Clair** | 静态扫描 | 企业级、可扩展 | 配置复杂 | 大型企业 |
| **Anchore** | 平台化 | 功能全面、策略灵活 | 资源消耗大 | 企业平台 |
| **Grype** | 静态扫描 | SBOM支持好 | 社区相对较小 | 开发团队 |
| **Snyk** | SaaS服务 | 用户体验好、IDE集成 | 需要联网 | 开发者友好 |

## 二、Trivy深度集成实践

### 2.1 CI/CD流水线集成

#### GitLab CI集成配置

```yaml
# .gitlab-ci.yml
stages:
  - build
  - security-scan
  - deploy

variables:
  TRIVY_VERSION: "0.48.0"
  SCAN_SEVERITY: "CRITICAL,HIGH"
  FAIL_ON_CRITICAL: "true"

build-image:
  stage: build
  script:
    - docker build -t $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA .
    - docker push $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
  only:
    - main
    - merge_requests

security-scan:
  stage: security-scan
  image:
    name: aquasec/trivy:$TRIVY_VERSION
    entrypoint: [""]
  script:
    - trivy image --severity $SCAN_SEVERITY --exit-code 1 $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
    - trivy config --exit-code 1 .
    - trivy sbom --format cyclonedx --output sbom.json $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
  artifacts:
    reports:
      cyclonedx: sbom.json
    paths:
      - trivy-results.json
    expire_in: 1 week
  allow_failure: false
  only:
    - main
    - merge_requests

deploy:
  stage: deploy
  script:
    - kubectl set image deployment/myapp myapp=$CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
  only:
    - main
```

#### GitHub Actions集成

```yaml
# .github/workflows/security-scan.yml
name: Security Scan
on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  security-scan:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout code
      uses: actions/checkout@v4
      
    - name: Build image
      run: docker build -t test-image:${{ github.sha }} .
      
    - name: Run Trivy vulnerability scanner
      uses: aquasecurity/trivy-action@master
      with:
        image-ref: 'test-image:${{ github.sha }}'
        format: 'sarif'
        output: 'trivy-results.sarif'
        severity: 'CRITICAL,HIGH'
        exit-code: '1'
        
    - name: Upload Trivy scan results to GitHub Security tab
      uses: github/codeql-action/upload-sarif@v2
      if: always()
      with:
        sarif_file: 'trivy-results.sarif'
```

### 2.2 运行时安全扫描

#### DaemonSet方式部署

```yaml
# 01-trivy-daemonset.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: trivy-scanner
  namespace: security
  labels:
    app: trivy-scanner
spec:
  selector:
    matchLabels:
      name: trivy-scanner
  template:
    metadata:
      labels:
        name: trivy-scanner
    spec:
      containers:
      - name: trivy
        image: aquasec/trivy:latest
        command:
        - "/bin/sh"
        - "-c"
        - |
          while true; do
            echo "Starting periodic scan at $(date)"
            
            # 扫描本地镜像
            for image in $(crictl images -q); do
              echo "Scanning image: $image"
              trivy image --quiet --format json --output /results/scan_$image.json $image
            done
            
            # 上传结果到中央存储
            tar -czf /results/results_$(date +%Y%m%d_%H%M%S).tar.gz /results/*.json
            # 这里可以添加上传到S3或其他存储的逻辑
            
            sleep 3600  # 每小时扫描一次
          done
        volumeMounts:
        - name: var-lib-containerd
          mountPath: /var/lib/containerd
          readOnly: true
        - name: results
          mountPath: /results
        securityContext:
          privileged: true
      volumes:
      - name: var-lib-containerd
        hostPath:
          path: /var/lib/containerd
      - name: results
        emptyDir: {}
      tolerations:
      - effect: NoSchedule
        operator: Exists
```

## 三、Harbor镜像仓库安全集成

### 3.1 Harbor安全配置

#### 镜像扫描策略配置

```yaml
# 02-harbor-security-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: harbor-security-config
  namespace: harbor
data:
  clair-config.yaml: |
    scanners:
      - name: "Trivy"
        vendor: "Aqua Security"
        version: "0.48.0"
        enabled: true
        url: "http://trivy-scanner:8080"
        auth:
          type: "bearer"
          token: "${SCANNER_TRIVY_TOKEN}"
          
      - name: "Clair"
        vendor: "Quay"
        version: "4.7.0"
        enabled: true
        url: "http://clair-scanner:6060"
        
    # 扫描策略
    policies:
      - name: "production-policy"
        description: "生产环境安全策略"
        rules:
        - severity: "CRITICAL"
          action: "BLOCK"
          timeout: "0"
        - severity: "HIGH"
          action: "WARN"
          timeout: "24h"
        - severity: "MEDIUM"
          action: "LOG"
          timeout: "168h"
          
      - name: "development-policy"
        description: "开发环境安全策略"
        rules:
        - severity: "CRITICAL"
          action: "BLOCK"
          timeout: "1h"
        - severity: "HIGH"
          action: "WARN"
          timeout: "72h"
```

#### 自动化镜像签名验证

```yaml
# 03-image-signature-policy.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cosign-policy
  namespace: security
data:
  policy.yaml: |
    apiVersion: policy.sigstore.dev/v1beta1
    kind: ClusterImagePolicy
    metadata:
      name: image-signature-policy
    spec:
      images:
      - glob: "harbor.example.com/production/**"
      authorities:
      - key:
          kms: "gcpkms://projects/my-project/locations/global/keyRings/my-ring/cryptoKeys/my-key"
        ctlog:
          url: https://rekor.sigstore.dev
      - keyless:
          identities:
          - issuer: "https://accounts.google.com"
            subject: "user@example.com"
        ctlog:
          url: https://rekor.sigstore.dev
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cosign-validator
  namespace: security
spec:
  replicas: 2
  selector:
    matchLabels:
      app: cosign-validator
  template:
    metadata:
      labels:
        app: cosign-validator
    spec:
      containers:
      - name: validator
        image: sigstore/cosign:latest
        command:
        - "/bin/sh"
        - "-c"
        - |
          while true; do
            # 验证最近推送的镜像
            IMAGES=$(curl -s "http://harbor-api/api/v2.0/projects/library/repositories/app/tags" | jq -r '.[].name')
            for image in $IMAGES; do
              echo "Validating signature for: harbor.example.com/library/app:$image"
              cosign verify --certificate-identity-regexp ".*@example.com" \
                           --certificate-oidc-issuer "https://accounts.google.com" \
                           harbor.example.com/library/app:$image || echo "Signature validation failed"
            done
            sleep 300
          done
```

## 四、漏洞管理与修复流程

### 4.1 自动化漏洞修复

#### 漏洞自动修复脚本

```bash
#!/bin/bash
# 04-auto-vulnerability-fix.sh

set -euo pipefail

REGISTRY="harbor.example.com"
PROJECT="library"
IMAGE="myapp"
TAG=${1:-"latest"}

echo "=== Automated Vulnerability Fix Process ==="
echo "Image: ${REGISTRY}/${PROJECT}/${IMAGE}:${TAG}"
echo "Start time: $(date)"

# 1. 扫描当前镜像
echo "1. Scanning current image..."
trivy image --format json --output scan_results.json ${REGISTRY}/${PROJECT}/${IMAGE}:${TAG}

# 2. 分析漏洞严重程度
CRITICAL_COUNT=$(jq '.Results[].Vulnerabilities[] | select(.Severity=="CRITICAL") | .VulnerabilityID' scan_results.json | wc -l)
HIGH_COUNT=$(jq '.Results[].Vulnerabilities[] | select(.Severity=="HIGH") | .VulnerabilityID' scan_results.json | wc -l)

echo "Critical vulnerabilities: $CRITICAL_COUNT"
echo "High vulnerabilities: $HIGH_COUNT"

# 3. 如果有严重漏洞，触发重建流程
if [ "$CRITICAL_COUNT" -gt 0 ] || [ "$HIGH_COUNT" -gt 5 ]; then
    echo "Critical vulnerabilities found, triggering rebuild..."
    
    # 获取基础镜像信息
    BASE_IMAGE=$(docker history --no-trunc ${REGISTRY}/${PROJECT}/${IMAGE}:${TAG} | grep -E "FROM|ADD|COPY" | head -1 | awk '{print $2}')
    
    # 更新Dockerfile使用最新基础镜像
    sed -i "s/FROM .*/FROM $(echo $BASE_IMAGE | sed 's/:.*$/:latest/')/" Dockerfile
    
    # 构建新镜像
    NEW_TAG="auto-fix-$(date +%Y%m%d-%H%M%S)"
    docker build -t ${REGISTRY}/${PROJECT}/${IMAGE}:${NEW_TAG} .
    
    # 推送新镜像
    docker push ${REGISTRY}/${PROJECT}/${IMAGE}:${NEW_TAG}
    
    # 扫描新镜像
    trivy image --format json --output new_scan_results.json ${REGISTRY}/${PROJECT}/${IMAGE}:${NEW_TAG}
    
    # 验证修复效果
    NEW_CRITICAL_COUNT=$(jq '.Results[].Vulnerabilities[] | select(.Severity=="CRITICAL") | .VulnerabilityID' new_scan_results.json | wc -l)
    echo "After fix - Critical vulnerabilities: $NEW_CRITICAL_COUNT"
    
    if [ "$NEW_CRITICAL_COUNT" -eq 0 ]; then
        echo "Vulnerabilities successfully fixed!"
        # 更新latest标签
        docker tag ${REGISTRY}/${PROJECT}/${IMAGE}:${NEW_TAG} ${REGISTRY}/${PROJECT}/${IMAGE}:latest
        docker push ${REGISTRY}/${PROJECT}/${IMAGE}:latest
    else
        echo "Some vulnerabilities remain, manual intervention required"
        exit 1
    fi
else
    echo "No critical vulnerabilities found, no action needed"
fi

echo "Process completed at: $(date)"
```

### 4.2 漏洞响应策略

#### 漏洞分级响应机制

```yaml
# 05-vulnerability-response-policy.yaml
apiVersion: security.k8s.io/v1
kind: VulnerabilityResponsePolicy
metadata:
  name: standard-response-policy
spec:
  # 漏洞响应级别定义
  severityLevels:
    critical:
      responseTime: "1h"
      action: "immediate-remediation"
      notification:
        - "security-team@company.com"
        - "platform-team@company.com"
        - "+12345678901"  # SMS
        
    high:
      responseTime: "24h"
      action: "planned-remediation"
      notification:
        - "security-team@company.com"
        
    medium:
      responseTime: "72h"
      action: "scheduled-update"
      notification:
        - "dev-team@company.com"
        
    low:
      responseTime: "168h"
      action: "routine-update"
      notification:
        - "dev-team@company.com"

  # 自动化响应规则
  autoRemediation:
    - condition:
        severity: "critical"
        cvssScore: ">= 9.0"
        exploitAvailability: "public"
      action: "rollback-and-rebuild"
      
    - condition:
        severity: "high"
        affectedComponents: ["openssl", "nginx", "glibc"]
      action: "priority-update"
      
    - condition:
        severity: "medium"
        age: "> 30d"
      action: "batch-update"

  # 例外处理
  exceptions:
    - cveId: "CVE-2023-XXXX"
      justification: "Risk accepted - workaround implemented"
      expiration: "2024-12-31"
      approver: "CTO"
      
    - imagePattern: "legacy-system:*"
      reason: "Business critical - cannot update"
      mitigation: "Network isolation and monitoring"
```

## 五、安全合规与报告

### 5.1 合规报告生成

#### 自动化合规报告脚本

```python
#!/usr/bin/env python3
# 06-compliance-report-generator.py

import json
import csv
from datetime import datetime, timedelta
import subprocess
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication

class SecurityComplianceReporter:
    def __init__(self):
        self.report_date = datetime.now()
        self.scan_results = []
        self.compliance_data = {}
        
    def collect_scan_data(self):
        """收集安全扫描数据"""
        # 收集Trivy扫描结果
        try:
            result = subprocess.run([
                'trivy', 'image', '--format', 'json', 
                'harbor.example.com/production/app:latest'
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                self.scan_results = json.loads(result.stdout)
        except Exception as e:
            print(f"Error collecting scan data: {e}")
            
    def generate_compliance_matrix(self):
        """生成合规矩阵"""
        compliance_items = {
            'cis_controls': self.check_cis_compliance(),
            'pci_dss': self.check_pci_dss_compliance(),
            'hipaa': self.check_hipaa_compliance(),
            'soc2': self.check_soc2_compliance()
        }
        
        self.compliance_data = compliance_items
        
    def check_cis_compliance(self):
        """检查CIS基准合规性"""
        findings = []
        
        # 检查镜像基础安全
        if self.scan_results:
            critical_vulns = [
                v for result in self.scan_results.get('Results', [])
                for v in result.get('Vulnerabilities', [])
                if v.get('Severity') == 'CRITICAL'
            ]
            
            if len(critical_vulns) > 0:
                findings.append({
                    'control': 'CIS-4.1',
                    'status': 'FAIL',
                    'description': f'{len(critical_vulns)} critical vulnerabilities found',
                    'evidence': [v['VulnerabilityID'] for v in critical_vulns[:5]]
                })
            else:
                findings.append({
                    'control': 'CIS-4.1',
                    'status': 'PASS',
                    'description': 'No critical vulnerabilities found'
                })
                
        return findings
        
    def generate_html_report(self):
        """生成HTML报告"""
        html_template = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>Security Compliance Report - {self.report_date.strftime('%Y-%m-%d')}</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                .header {{ background-color: #f0f0f0; padding: 20px; border-radius: 5px; }}
                .finding {{ margin: 10px 0; padding: 10px; border-left: 4px solid #ccc; }}
                .pass {{ border-left-color: #4CAF50; }}
                .fail {{ border-left-color: #f44336; }}
                table {{ width: 100%; border-collapse: collapse; margin: 20px 0; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
                th {{ background-color: #f2f2f2; }}
            </style>
        </head>
        <body>
            <div class="header">
                <h1>Security Compliance Report</h1>
                <p>Generated on: {self.report_date.strftime('%Y-%m-%d %H:%M:%S')}</p>
                <p>Report Period: Last 30 days</p>
            </div>
            
            <h2>Vulnerability Summary</h2>
            <table>
                <tr><th>Severity</th><th>Count</th><th>Status</th></tr>
                {self.generate_vulnerability_table()}
            </table>
            
            <h2>Compliance Status</h2>
            {self.generate_compliance_sections()}
        </body>
        </html>
        """
        return html_template
        
    def generate_vulnerability_table(self):
        """生成漏洞统计表"""
        if not self.scan_results:
            return "<tr><td colspan='3'>No scan data available</td></tr>"
            
        vuln_stats = {}
        for result in self.scan_results.get('Results', []):
            for vuln in result.get('Vulnerabilities', []):
                severity = vuln.get('Severity', 'UNKNOWN')
                vuln_stats[severity] = vuln_stats.get(severity, 0) + 1
                
        rows = []
        for severity, count in vuln_stats.items():
            status = 'PASS' if severity not in ['CRITICAL', 'HIGH'] else 'FAIL'
            rows.append(f"<tr><td>{severity}</td><td>{count}</td><td>{status}</td></tr>")
            
        return ''.join(rows)
        
    def generate_compliance_sections(self):
        """生成合规性章节"""
        sections = []
        for standard, findings in self.compliance_data.items():
            section = f"<h3>{standard.upper()} Compliance</h3>"
            section += "<div>"
            for finding in findings:
                css_class = 'pass' if finding['status'] == 'PASS' else 'fail'
                section += f"""
                <div class="finding {css_class}">
                    <strong>{finding['control']}</strong>: {finding['status']}
                    <br><small>{finding['description']}</small>
                </div>
                """
            section += "</div>"
            sections.append(section)
            
        return ''.join(sections)
        
    def send_email_report(self, recipients):
        """发送邮件报告"""
        msg = MIMEMultipart()
        msg['Subject'] = f'Security Compliance Report - {self.report_date.strftime("%Y-%m-%d")}'
        msg['From'] = 'security-reports@company.com'
        msg['To'] = ', '.join(recipients)
        
        html_report = self.generate_html_report()
        msg.attach(MIMEText(html_report, 'html'))
        
        # 发送邮件
        with smtplib.SMTP('localhost') as server:
            server.send_message(msg)

def main():
    reporter = SecurityComplianceReporter()
    reporter.collect_scan_data()
    reporter.generate_compliance_matrix()
    
    # 生成报告
    html_report = reporter.generate_html_report()
    with open('/reports/compliance_report.html', 'w') as f:
        f.write(html_report)
        
    # 发送邮件（可选）
    # reporter.send_email_report(['security-team@company.com'])

if __name__ == "__main__":
    main()
```

### 5.2 持续监控仪表板

#### Prometheus监控指标

```yaml
# 07-security-metrics.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: image-security-rules
  namespace: monitoring
spec:
  groups:
  - name: image.security
    rules:
    # 镜像漏洞统计
    - record: image_vulnerabilities_total
      expr: sum by(image, severity) (trivy_vulnerabilities)
      
    # 镜像合规性评分
    - record: image_compliance_score
      expr: (1 - (sum by(image) (trivy_vulnerabilities{severity=~"CRITICAL|HIGH"}) / count by(image) (trivy_scanned_images))) * 100
      
    # 安全趋势告警
    - alert: ImageSecurityDegrading
      expr: image_compliance_score < 80
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "镜像安全合规性下降"
        description: "镜像 {{ $labels.image }} 合规性评分低于80%: {{ $value }}"
        
    - alert: CriticalVulnerabilitiesDetected
      expr: trivy_vulnerabilities{severity="CRITICAL"} > 0
      for: 30m
      labels:
        severity: critical
      annotations:
        summary: "检测到严重漏洞"
        description: "镜像中发现 {{ $value }} 个严重漏洞"
```

这份镜像安全扫描与漏洞管理文档提供了完整的容器镜像安全解决方案，从构建期到运行期的全生命周期安全管理。
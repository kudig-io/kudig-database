# 104 - 安全扫描与漏洞检测工具 (Security Scanning)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 扫描工具能力矩阵

| 工具 (Tool) | 扫描对象 (Target) | 漏洞库 (Database) | CI/CD 集成 | 生产建议 |
|------------|-----------------|-----------------|-----------|---------|
| **Trivy** | 镜像/文件系统/Git | 多源聚合 | ✅ 极易集成 | 通用首选 |
| **Grype** | 镜像/SBOM | Anchore DB | ✅ 支持 | 离线环境 |
| **Clair** | 镜像 | CVE 数据库 | ✅ Harbor 集成 | 企业镜像仓库 |
| **Snyk** | 镜像/代码/依赖 | 商业数据库 | ✅ SaaS | 开发者友好 |

## Trivy 生产级扫描

### 1. CI/CD 流水线集成
```yaml
# GitLab CI 示例
security_scan:
  stage: test
  image: aquasec/trivy:latest
  script:
    - trivy image --exit-code 1 --severity CRITICAL,HIGH myapp:$CI_COMMIT_SHA
    - trivy fs --exit-code 0 --severity MEDIUM,LOW .
  artifacts:
    reports:
      container_scanning: gl-container-scanning-report.json
```

### 2. Kubernetes 准入控制
```yaml
# 使用 OPA/Gatekeeper 强制扫描
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequireImageScan
metadata:
  name: require-trivy-scan
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
  parameters:
    allowedSeverities: ["LOW", "MEDIUM"]
```

### 3. 定期扫描已部署镜像
```bash
# 扫描集群中所有镜像
kubectl get pods --all-namespaces -o jsonpath="{.items[*].spec.containers[*].image}" | \
  tr -s '[[:space:]]' '\n' | sort | uniq | \
  xargs -I {} trivy image --severity HIGH,CRITICAL {}
```

## Grype SBOM 工作流

### 1. 生成软件物料清单
```bash
# 使用 Syft 生成 SBOM
syft packages myapp:v1.0 -o spdx-json > sbom.json

# Grype 基于 SBOM 扫描
grype sbom:./sbom.json
```

### 2. 离线扫描
```bash
# 下载漏洞数据库
grype db update

# 离线扫描
grype myapp:v1.0 --offline
```

## 安全策略最佳实践

| 策略 (Policy) | 实施方式 (Implementation) |
|--------------|--------------------------|
| **阻断高危漏洞** | CI 流水线设置 `--exit-code 1` |
| **定期扫描** | CronJob 每日扫描生产镜像 |
| **漏洞追踪** | 集成 Jira/GitHub Issues |
| **基础镜像管理** | 使用官方/可信镜像源 |
| **及时修复** | SLA: CRITICAL 24h, HIGH 7d |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
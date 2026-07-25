---
title: Trivy
description: Trivy — Kubernetes 生产运维知识库
summary: Trivy — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- scanning
- trivy
- vulnerability
- sbom
- docker
- falco
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Trivy 是什么
- 如何 Trivy
trigger_keywords:
- Trivy
prerequisites:
- kubectl-basics
- iac-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Trivy

Trivy (by Aqua Security) is a comprehensive, open-source security scanner for cloud native artifacts.

## Key Facts

- **License**: Apache-2.0 (free)
- **Scanner Types**: Vulnerability, misconfiguration, secret, SBOM
- **Targets**: Container images, filesystems, K8s clusters, Git repos, IaC files

## Scan Capabilities

| Scan Type | Description | Output |
|-----------|-------------|--------|
| Vulnerability | OS packages and language dependencies | CVE list with severity |
| Misconfiguration | K8s manifests, Dockerfile, Terraform | Policy violations |
| Secret Detection | Hardcoded credentials in code/repos | Exposed [[Secrets|secrets]] |
| SBOM Generation | Software Bill of Materials | CycloneDX/SPDX format |

## 安装与配置

```bash
# 安装 Trivy CLI
curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin
# 或 Homebrew
brew install trivy
# 或 Docker
docker run aquasec/trivy:latest image nginx:1.25

# 验证安装
trivy --version
```

### Trivy Operator (K8s 集群内扫描)

```bash
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm install trivy-operator aqua/trivy-operator \
  --namespace trivy-system --create-namespace \
  --set trivy.ignoreUnfixed=true \
  --set operator.scanJobsConcurrentLimit=3
```

```yaml
# VulnerabilityReport CRD 示例 (自动生成)
apiVersion: aquasecurity.github.io/v1alpha1
kind: VulnerabilityReport
metadata:
  name: nginx-1.25-deployment-abc123
  namespace: production
spec:
  scanner:
    name: Trivy
    version: 0.50.0
  registry:
    server: index.docker.io
  artifact:
    repository: library/nginx
    tag: "1.25"
  summary:
    criticalCount: 2
    highCount: 5
    mediumCount: 12
  vulnerabilities:
    - vulnerabilityID: CVE-2024-0001
      resource: openssl
      severity: CRITICAL
      fixedVersion: "3.0.12"
      installedVersion: "3.0.11"
```

## 扫描能力详解

| 扫描类型 | 目标 | 输出 | 命令示例 |
|-----------|------|------|----------|
| 漏洞扫描 | OS 包 + 语言依赖 | CVE 列表 + 严重度 | `trivy image nginx:1.25` |
| 配置扫描 | K8s/Docker/Terraform | 策略违规 | `trivy config ./` |
| Secret 检测 | 代码/仓库 | 暴露的凭证 | `trivy secret ./repo` |
| SBOM 生成 | 容器镜像 | CycloneDX/SPDX | `trivy image --format cyclonedx` |
| 许可证扫描 | 依赖包 | 许可证风险 | `trivy image --scanners license` |
| 集群扫描 | K8s 集群 | 综合报告 | `trivy k8s cluster` |

## CI/CD 集成

```bash
# 🟢 扫描镜像漏洞 (仅 HIGH/CRITICAL)
trivy image --severity HIGH,CRITICAL --exit-code 1 nginx:1.25

# 🟢 扫描本地文件系统
trivy fs --severity HIGH,CRITICAL ./src

# 🟢 扫描 K8s 集群配置
trivy k8s --severity HIGH,CRITICAL --report summary cluster

# 🟢 生成 SBOM (CycloneDX)
trivy image --format cyclonedx --output sbom.cdx.json nginx:1.25

# 🟢 生成 SBOM (SPDX)
trivy image --format spdx-json --output sbom.spdx.json nginx:1.25

# 🟢 扫描 Git 仓库 Secret
trivy secret --exit-code 1 ./repository

# 🟢 扫描 Dockerfile 配置问题
trivy config --severity HIGH ./Dockerfile

# 🟢 扫描 Terraform 文件
trivy config --severity HIGH,CRITICAL ./terraform/

# 🟢 仅扫描未修复漏洞
trivy image --ignore-unfixed --severity HIGH,CRITICAL nginx:1.25

# 🟢 输出 JSON 格式 (CI 解析)
trivy image --format json --output report.json nginx:1.25
```

### GitHub Actions 集成

```yaml
# .github/workflows/trivy-scan.yaml
name: Trivy Security Scan
on: [push, pull_request]
jobs:
  trivy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build image
        run: docker build -t myapp:${{ github.sha }} .
      - name: Trivy scan
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: myapp:${{ github.sha }}
          format: sarif
          output: trivy-results.sarif
          severity: CRITICAL,HIGH
          exit-code: 1
      - name: Upload to GitHub Security
        uses: github/codeql-action/upload-sarif@v3
        with:
          sarif_file: trivy-results.sarif
```

## 运维操作

```bash
# 🟢 查看 Trivy Operator 状态
kubectl get pods -n trivy-system
kubectl get vulnerabilityreports -A
kubectl get configauditreports -A
kubectl get clustercompliancereports

# 🟢 查看特定工作负载的漏洞报告
kubectl get vulnerabilityreport -n production -o yaml \
  deployment-nginx

# 🟢 查看集群配置审计
kubectl get configauditreport -A -o custom-columns=\
NAME:.metadata.name,HIGH:.report.summary.highCount,\
CRITICAL:.report.summary.criticalCount

# 🟡 触发重新扫描
kubectl delete vulnerabilityreport -n production deployment-nginx
# Operator 会自动重新创建报告

# 🟢 查看扫描 Job 日志
kubectl get jobs -n trivy-system
kubectl logs -n trivy-system job/scan-vulnerabilityreport-abc123

# 🟢 本地扫描私有镜像仓库
trivy image --username user --password pass registry.internal/app:v1.0

# 🟢 使用离线数据库 (Air-gapped 环境)
trivy image --download-db-only --db-repository ghcr.io/aquasecurity/trivy-db:2
trivy image --offline-scan --cache-dir /opt/trivy-cache nginx:1.25
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 扫描超时 | 网络无法访问漏洞数据库 | `trivy image --debug nginx` | 配置代理或离线 DB |
| Operator Pod CrashLoop | 资源不足/权限问题 | `kubectl logs -n trivy-system` | 增加资源/RBAC |
| 漏洞报告为空 | 扫描 Job 失败 | `kubectl get jobs -n trivy-system` | 检查 Job 日志 |
| 大量误报 | 未过滤已修复漏洞 | 检查 `.trivyignore` | 添加 `--ignore-unfixed` |
| 私有仓库认证失败 | 凭证未配置 | `trivy image --debug registry/app` | 配置 imagePullSecrets |
| SBOM 生成失败 | 镜像层过大 | 检查磁盘空间 | 增加缓存目录空间 |

### 排查流程

```
Trivy 扫描异常
├── CLI 扫描失败
│   ├── "failed to download DB" → 网络/代理问题
│   │   ├── 检查 https://ghcr.io 可达性
│   │   └── 配置 --db-repository 或离线 DB
│   ├── "authentication required" → 仓库认证
│   │   └── 配置 --username/--password 或 docker login
│   └── "timeout" → 镜像过大/网络慢
│       └── 增加 --timeout 参数
└── Operator 扫描失败
    ├── Pod Pending → 资源不足
    ├── Job Failed → 查看 Job 日志
    └── 报告缺失 → 检查 RBAC 权限
```

## 生产案例

### 案例 1: CI/CD 管道拦截高危漏洞镜像

- **场景**: 开发团队使用含 Log4Shell 漏洞的基础镜像构建服务
- **排查**: Trivy 扫描发现 `log4j-core 2.14.1` 存在 CVE-2021-44228 (CRITICAL)
- **方案**: CI 管道配置 `--exit-code 1 --severity CRITICAL`，阻止含高危漏洞的镜像推送；开发团队升级 log4j 到 2.17.1
- **效果**: 漏洞镜像未进入生产环境，安全左移成功

### 案例 2: 集群配置审计发现特权容器

- **场景**: 安全团队使用 Trivy Operator 扫描集群配置
- **排查**: ConfigAuditReport 显示 12 个 Deployment 使用 `privileged: true`，5 个使用 `hostNetwork: true`
- **方案**: 结合 Kyverno 策略禁止特权容器；对必要场景使用 SecurityContext 最小权限
- **效果**: 集群攻击面减少 70%，配置合规率达到 95%

## .trivyignore 配置

```bash
# .trivyignore - 忽略已知误报或已接受风险
# CVE 忽略
CVE-2023-0001  # 已确认不影响，待下版本修复
CVE-2023-0002  # 误报，已联系 Aqua 确认

# 带过期时间的忽略 (Trivy >= 0.45)
CVE-2023-0003 exp:2024-06-30  # 临时忽略，到期后重新评估
```

## 对比与替代方案

| 维度 | Trivy | Grype | Snyk | Clair |
|------|-------|-------|------|-------|
| 开源 | ✅ Apache-2.0 | ✅ Apache-2.0 | 部分 | ✅ Apache-2.0 |
| 扫描范围 | 镜像/FS/K8s/IaC/Secret | 镜像/FS | 镜像/代码/IaC | 镜像 |
| SBOM | ✅ CycloneDX+SPDX | ✅ SPDX | ✅ | ❌ |
| K8s 集群扫描 | ✅ Operator | ❌ | ✅ | ❌ |
| 配置审计 | ✅ | ❌ | ✅ | ❌ |
| Secret 检测 | ✅ | ❌ | ✅ | ❌ |
| 离线支持 | ✅ | ✅ | 部分 | ✅ |
| 适用场景 | 全能型 | 轻量快速 | 企业级 | Harbor 集成 |

## 检查清单

- [ ] Trivy CLI 版本为最新稳定版
- [ ] CI/CD 管道集成 Trivy 扫描并配置失败阈值
- [ ] 生产集群部署 Trivy Operator 持续扫描
- [ ] 配置 `.trivyignore` 管理已知误报
- [ ] 私有镜像仓库认证已配置
- [ ] SBOM 生成并存储到制品仓库
- [ ] 漏洞报告接入告警系统 (Slack/PagerDuty)
- [ ] 定期审查忽略列表，避免风险累积
- [ ] Air-gapped 环境配置离线漏洞数据库

## Related

- [[falco]] — Falco
- [[kyverno]] — Kyverno
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD Pipeline Patterns
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[supply-chain-security]] — Software Supply Chain Security

<!-- risk-assessed -->

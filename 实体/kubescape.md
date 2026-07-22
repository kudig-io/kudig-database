---
title: Kubescape [entities]
description: '## 概述'
summary: 'Kubescape 是第一个用于测试 Kubernetes 是否按照 NSA-CISA 和 MITRE ATT&CK 框架安全部署的开源工具。它提供全面的安全平台，包括风险分析、安全合规、镜像漏洞扫描和运行时安全监控。'
category: entities
tags:
- k8s
- cncf
- observability
- kubescape
- prometheus
- grafana
- cilium
- helm
- rbac
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubescape 是什么
- 如何 Kubescape
trigger_keywords:
- Kubescape
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubescape

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Kubescape 是由 ARMO（现 Kubescape）开源的 Kubernetes 安全平台，2021 年加入 CNCF Sandbox，后晋升为 Incubating。它是第一个用于测试 Kubernetes 是否按照 NSA-CISA、MITRE ATT&CK 和 CIS Benchmark 等框架安全部署的开源工具。Kubescape 提供全面的安全态势管理，包括风险分析、安全合规扫描、镜像漏洞检测、RBAC 可视化和运行时安全监控。

## 核心特性

- **多框架合规**: 支持 NSA-CISA、MITRE ATT&CK、CIS Benchmark、NSA、SOC2
- **镜像漏洞扫描**: 集成 Grype 检测容器镜像 CVE 漏洞
- **配置扫描**: 静态分析 YAML、Helm、Kustomize 配置文件
- **RBAC 可视化**: 权限矩阵分析和最小权限优化建议
- **运行时监控**: 基于 eBPF 的实时异常行为检测
- **CI/CD 集成**: GitHub Actions、GitLab CI、Jenkins 插件

## 架构

Kubescape 分为 CLI 模式和 Operator 模式。CLI 模式下，kubescape 命令连接 API Server 或扫描本地文件，将资源配置与安全框架规则集（以 Rego/OPA 或 JSON 格式定义）对比，生成合规报告。Operator 模式在集群中持续运行：kubescape-operator 定期扫描集群资源，kubescape-host-scanner（DaemonSet）采集节点安全配置，ARMO Portal 提供可视化仪表盘。扫描结果以 Prometheus 指标和 CRD 形式暴露。

## Kubernetes 集成

Kubescape 通过 Kubernetes API 读取集群资源（Pods、Deployments、Roles、ClusterRoles 等），分析配置是否符合安全基线。Operator 模式通过 CRD（KubescapeConfig）管理扫描策略和异常规则。Host Scanner 以 DaemonSet 运行，采集节点级安全信息（内核参数、文件权限、用户配置）。支持通过注解 `kubescape.io/ignore` 标记例外规则。扫描结果可作为 Prometheus 指标或发送到 SIEM 系统。

## 生产使用场景

1. **CI/CD 安全门禁**: 在 PR 阶段自动扫描 YAML 配置，阻止不安全配置合并
2. **持续合规监控**: 定期扫描集群，跟踪 NSA/CIS 合规态势变化
3. **漏洞管理**: 扫描运行中的容器镜像，优先修复 Critical/High CVE
4. **RBAC 审计**: 分析权限矩阵，发现过度授权并收敛到最小权限

## 安装与配置

```bash
# CLI 安装
brew install kubescape
# 扫描集群
kubescape scan framework nsa --submit
# Operator 模式
helm repo add kubescape https://kubescape.github.io/helm-charts/
helm install kubescape kubescape/kubescape-operator \
  -n kubescape --create-namespace \
  --set clusterName=$(kubectl config current-context)
```

### CI/CD 集成配置

```yaml
# GitHub Actions 示例
name: Kubescape Security Scan
on: [pull_request]
jobs:
  security-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Scan YAML files
        run: |
          curl -s https://raw.githubusercontent.com/kubescape/kubescape/master/install.sh | /bin/bash
          kubescape scan framework nsa manifests/ \
            --format json --output results.json \
            --fail-threshold 40
      - name: Upload results
        uses: actions/upload-artifact@v4
        with:
          name: kubescape-results
          path: results.json
```

### Operator 高级配置

```yaml
# kubescape-values.yaml
clusterName: prod-cluster
server: https://api.armosec.io
account: <account-id>

# 扫描策略
continuousScan:
  enabled: true
  schedule: "0 */6 * * *"  # 每 6 小时扫描

# 镜像扫描
imageVulnerabilityScan:
  enabled: true
  registries:
    - registry: "registry.internal.com"
      credentials:
        secretName: registry-creds

# 主机扫描
hostScanner:
  enabled: true
  tolerations:
    - operator: Exists
```

## 运维操作

```bash
# 🟢 扫描集群 NSA 框架合规性
kubescape scan framework nsa

# 🟢 扫描 CIS Benchmark
kubescape scan framework cis-v1.23

# 🟢 扫描单个 YAML 文件
kubescape scan framework nsa deployment.yaml

# 🟢 扫描 Helm Chart
kubescape scan framework nsa ./charts/my-app/

# 🟢 查看 RBAC 权限矩阵
kubescape scan control "Access container service account"

# 🟢 查看扫描结果（JSON 格式）
kubescape scan framework nsa --format json --output scan-results.json

# 🟡 设置失败阈值（CI 门禁）
kubescape scan framework nsa --fail-threshold 50

# 🟢 查看 Operator 状态
kubectl get pods -n kubescape
kubectl get kubescapeconfig -n kubescape

# 🟡 触发即时重新扫描
kubectl annotate kubescapeconfig -n kubescape default \
  kubescape.io/trigger-scan=$(date +%s) --overwrite
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 扫描超时 | API Server 负载高 | `kubectl get --raw /healthz` | 降低扫描频率或分批扫描 |
| Host Scanner CrashLoop | 节点权限不足 | `kubectl logs -n kubescape ds/kubescape-host-scanner` | 检查 privileged 和 hostPID |
| 扫描结果不准确 | 框架版本过旧 | `kubescape version` | 升级 kubescape 到最新版 |
| Operator Pod Pending | 资源不足 | `kubectl describe pod -n kubescape` | 调整 requests/limits |
| CI 扫描失败 | 网络无法访问框架库 | `kubescape download framework nsa` | 预下载框架或配置代理 |
| 镜像扫描无结果 | Registry 认证失败 | `kubectl logs -n kubescape -l app=grype` | 检查 Secret 中的凭据 |

### 排查流程

```
Kubescape 扫描异常
├─ CLI 扫描失败？
│  ├─ 网络问题 → 检查能否访问 ARMO API
│  ├─ 权限不足 → 检查 kubeconfig RBAC
│  └─ 框架下载失败 → kubescape download artifacts
├─ Operator 模式异常？
│  ├─ Pod 未运行 → kubectl describe pod -n kubescape
│  ├─ 扫描结果为空 → 检查 CRD 和扫描策略配置
│  └─ Host Scanner 失败 → 检查节点权限和内核版本
└─ CI/CD 集成失败？
   ├─ 安装步骤失败 → 检查网络/代理配置
   ├─ 扫描超时 → 缩小扫描范围或增加 timeout
   └─ 门禁误报 → 配置例外规则 (kubescape.io/ignore)
```

## 生产案例

### 案例 1: 金融企业合规审计自动化

**场景**: 某银行需满足 NSA-CISA K8s 加固指南，每季度手动审计耗时 2 周。

**方案**:
1. 部署 Kubescape Operator 持续扫描
2. 配置 Prometheus 指标导出合规分数
3. Grafana 仪表盘展示合规趋势
4. 不合规项自动创建 Jira 工单

**效果**: 审计时间从 2 周缩短到 2 小时，合规分数从 62% 提升到 94%。

### 案例 2: CI/CD 安全门禁防止不安全配置上线

**场景**: 开发者频繁提交特权容器、hostNetwork 等不安全配置。

**方案**:
1. GitHub Actions 中集成 kubescape scan
2. 设置 --fail-threshold 40，不合规即阻止合并
3. 配置例外规则处理已知可接受风险
```bash
kubescape scan framework nsa ./k8s/ \
  --fail-threshold 40 \
  --exceptions exceptions.json
```

**效果**: 上线 3 个月后，不安全配置 PR 减少 85%，开发者安全意识显著提升。

## 对比与替代方案

| 维度 | Kubescape | kube-bench | Polaris | Trivy |
|------|-----------|------------|---------|-------|
| 合规框架 | NSA/CIS/MITRE/SOC2 | 仅 CIS | 有限 | 有限 |
| 镜像扫描 | ✅ Grype | ❌ | ❌ | ✅ 最强 |
| 配置扫描 | ✅ | 节点级 | ✅ | ✅ |
| RBAC 分析 | ✅ | ❌ | ❌ | ❌ |
| 运行时监控 | ✅ eBPF | ❌ | ❌ | ❌ |
| CI/CD 集成 | ✅ 全面 | 有限 | ✅ | ✅ |
| CNCF 状态 | Incubating | 非 CNCF | 非 CNCF | Graduated |

## 检查清单

- [ ] Kubescape CLI 版本为最新（支持最新框架规则）
- [ ] Operator 模式已配置持续扫描计划
- [ ] CI/CD 管道已集成安全门禁
- [ ] 例外规则已文档化并定期审查
- [ ] Host Scanner DaemonSet 在所有节点运行
- [ ] 扫描结果已对接 SIEM/Prometheus
- [ ] RBAC 权限矩阵已审计并收敛
- [ ] 镜像漏洞扫描已启用并配置 Registry 凭据

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kubescape** | CNCF Incubating、多框架、免费 | 企业功能需付费版 |
| kube-bench | CIS Benchmark 标准工具 | 仅 CIS、功能单一 |
| Polaris | 配置验证、Dashboard 好 | 合规框架覆盖少 |
| Trivy | 镜像扫描最强 | 配置合规能力较弱 |

## 架构定位

在 CNCF 生态中，Kubescape 属于 **Security / Compliance** 类别，是 Kubernetes 安全态势管理（KSPM）的领先开源平台。它与 Falco、KubeArmor、Trivy 等项目互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- networking.md|cilium-ebpf-networking]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- kubescape
- [[技能/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->

---
title: Kubescape
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- job
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kubescape 是什么
- 如何 Kubescape
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kubescape
- cncf
- landscape
---

# Kubescape

> **成熟度**: Incubating | **加入时间**: 2022-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubescape.io |
| **GitHub** | https://github.com/kubescape/kubescape |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security & Compliance |

---

## 项目概述

Kubescape 是第一个用于测试 Kubernetes 是否按照 NSA-CISA 和 MITRE ATT&CK 框架安全部署的开源工具。它提供全面的安全平台，包括风险分析、安全合规、镜像漏洞扫描和运行时安全监控。

## 核心特性

- **安全合规扫描**: 支持 NSA-CISA、MITRE ATT&CK、CIS Benchmark
- **镜像漏洞扫描**: 集成 Grype 检测 CVE 漏洞
- **配置扫描**: YAML/Helm/Kustomize 静态分析
- **RBAC 可视化**: 权限分析和最小权限建议
- **运行时监控**: eBPF 实时检测异常行为
- **CI/CD 集成**: GitHub Actions、GitLab CI、Jenkins 插件
- **IDE 插件**: VS Code 扩展实时反馈

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Kubescape Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────┐     │
│  │                    User Interfaces                      │     │
│  │  ┌────────┐  ┌──────────┐  ┌────────┐  ┌───────────┐  │     │
│  │  │  CLI   │  │ VS Code  │  │  CI/CD │  │   Web UI  │  │     │
│  │  │        │  │  Plugin  │  │ Plugin │  │ (SaaS)    │  │     │
│  │  └────────┘  └──────────┘  └────────┘  └───────────┘  │     │
│  └──────────────────────────┬─────────────────────────────┘     │
│                             │                                    │
│                             ▼                                    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                   Kubescape Engine                       │    │
│  │  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐  │    │
│  │  │   Scanner    │  │   Analyzer    │  │   Reporter  │  │    │
│  │  │  (Controls)  │  │   (Scoring)   │  │   (Output)  │  │    │
│  │  └──────────────┘  └───────────────┘  └─────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                             │                                    │
│         ┌───────────────────┼───────────────────┐               │
│         ▼                   ▼                   ▼               │
│  ┌────────────┐    ┌─────────────┐    ┌─────────────────┐      │
│  │ Frameworks │    │    SBOM     │    │     RBAC        │      │
│  │ NSA, CIS   │    │   Grype     │    │   Analyzer      │      │
│  │ MITRE      │    │   Syft      │    │                 │      │
│  └────────────┘    └─────────────┘    └─────────────────┘      │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │               In-Cluster Components                      │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │    │
│  │  │   Operator  │  │   Storage   │  │ Runtime Agent   │  │    │
│  │  │  (Helm)     │  │  (CRDs)     │  │ (eBPF)          │  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 CLI

```bash
# macOS
brew install kubescape

# Linux
curl -s https://raw.githubusercontent.com/kubescape/kubescape/master/install.sh | /bin/bash

# Windows
iwr -useb https://raw.githubusercontent.com/kubescape/kubescape/master/install.ps1 | iex
```

### 基本扫描

```bash
# 扫描整个集群
kubescape scan

# 使用特定框架扫描
kubescape scan framework nsa

# 扫描 CIS Benchmark
kubescape scan framework cis-v1.23-t1.0.1

# 扫描 MITRE ATT&CK
kubescape scan framework mitre

# 扫描特定命名空间
kubescape scan --include-namespaces production,staging

# 排除命名空间
kubescape scan --exclude-namespaces kube-system
```

### 扫描 YAML 文件

```bash
# 扫描本地文件
kubescape scan *.yaml

# 扫描 Helm Chart
kubescape scan /path/to/helm/chart

# 扫描 Git 仓库
kubescape scan https://github.com/example/k8s-manifests
```

### 镜像漏洞扫描

```bash
# 扫描集群中所有镜像
kubescape scan image

# 扫描特定镜像
kubescape scan image nginx:latest

# 设置严重级别阈值
kubescape scan image --severity-threshold high
```

### CI/CD 集成

```yaml
# GitHub Actions
name: Kubescape Security Scan
on: [push, pull_request]

jobs:
  kubescape-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Kubescape Scan
        uses: kubescape/github-action@main
        with:
          format: sarif
          outputFile: results.sarif
          files: "*.yaml"
          frameworks: nsa,mitre
          severityThreshold: medium
      - name: Upload SARIF
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: results.sarif
```

---

## 集群内部署

### Helm 安装

```bash
# 添加仓库
helm repo add kubescape https://kubescape.github.io/helm-charts/
helm repo update

# 安装 Operator
helm install kubescape kubescape/kubescape-operator \
  --namespace kubescape \
  --create-namespace \
  --set clusterName=my-cluster

# 带持续扫描
helm install kubescape kubescape/kubescape-operator \
  --namespace kubescape \
  --create-namespace \
  --set continuousScan.enabled=true \
  --set continuousScan.schedule="0 */6 * * *"
```

### 配置示例

```yaml
# kubescape-config.yaml
apiVersion: kubescape.io/v1alpha1
kind: KubescapeConfiguration
metadata:
  name: kubescape-config
spec:
  scan:
    frameworks:
      - nsa
      - mitre
      - cis-v1.23-t1.0.1
    excludedNamespaces:
      - kube-system
      - kubescape
  vulnerabilityScanning:
    enabled: true
    severityThreshold: medium
  runtimeDetection:
    enabled: true
```

---

## 输出格式

```bash
# JSON 输出
kubescape scan -f json -o results.json

# SARIF (GitHub Code Scanning)
kubescape scan -f sarif -o results.sarif

# PDF 报告
kubescape scan -f pdf -o report.pdf

# JUnit XML (CI 集成)
kubescape scan -f junit -o results.xml

# Prometheus 指标
kubescape scan -f prometheus
```

---

## 安全框架

### NSA-CISA 控制项示例

| 控制项 | 说明 | 严重级别 |
|--------|------|----------|
| C-0001 | 禁止特权容器 | High |
| C-0002 | 限制 hostNetwork | Medium |
| C-0004 | 资源限制配置 | Low |
| C-0009 | 只读根文件系统 | Medium |
| C-0017 | 配置 liveness probe | Low |

---

## 最佳实践

1. **CI/CD 集成**: 在 PR 阶段自动扫描，阻止不安全配置合并
2. **持续监控**: 部署 Operator 定期扫描，跟踪安全态势变化
3. **例外管理**: 使用注解标记已接受的风险 `kubescape.io/ignore`
4. **渐进式修复**: 按严重级别优先修复 Critical/High 问题
5. **SBOM 生成**: 配合漏洞扫描建立软件物料清单

---

## 参考资源

- [官方文档](https://kubescape.io/docs)
- [GitHub Repo](https://github.com/kubescape/kubescape)
- [控制项列表](https://hub.armosec.io/docs)
- [NSA-CISA 指南](https://media.defense.gov/2022/Aug/29/2003066362/-1/-1/0/CTR_KUBERNETES_HARDENING_GUIDANCE_1.2_20220829.PDF)

---

**维护者**: Kudig Team | **许可证**: MIT

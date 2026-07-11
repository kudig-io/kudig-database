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

## 安装

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

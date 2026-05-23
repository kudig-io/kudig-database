---
title: Domain-32 YAML 清单 — 开源项目索引
description: '| **kustomize** | K8s 原生配置定制 | K8s SIG | v5.6.0 | 11k+ | Apache-2.0 |'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- grafana
- helm
- opa
- statefulset
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-32 YAML 清单 — 开源项目索引 是什么
- 如何 Domain-32 YAML 清单 — 开源项目索引
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- Domain-32
- YAML
- 清单
- 开源项目索引
- yaml
- manifests
prerequisites:
- kubectl-basics
- helm-basics
- monitoring-basics
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
created: "2026-05-23"
---

# Domain-32 YAML 清单 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **kustomize** | K8s 原生配置定制 | K8s SIG | v5.6.0 | 11k+ | Apache-2.0 |
| **Helm** | 包管理与模板 | CNCF Graduated | v3.17.0 | 27k+ | Apache-2.0 |
| **Helmfile** | 声明式 Helm 部署 | 社区 | v1.0.0 | 6k+ | MIT |
| **Helm Dashboard** | Helm UI 管理 | Komodor | v1.3.0 | 4k+ | Apache-2.0 |
| **yq** | YAML 命令行处理器 | Mike Farah | v4.45.0 | 13k+ | MIT |
| **dyff** | YAML 差异对比 | Homeport | v1.10.0 | 1k+ | MIT |
| **kubeval** | K8s YAML 验证 (已归档) | Instrumenta | 归档 | 4k+ | Apache-2.0 |
| **kubeconform** | K8s YAML 验证 (kubeval 替代) | Yann Hamon | v0.6.7 | 1k+ | Apache-2.0 |
| **Conftest** | OPA 策略验证 (YAML/JSON) | OPA | v0.57.0 | 3k+ | Apache-2.0 |
| **Config Connector / Crossplane** | K8s 管理云资源 | Google/CNCF | - | - | Apache-2.0 |
| **Kubevious** | K8s 配置分析与验证 | 社区 | v1.0.0 | 1k+ | Apache-2.0 |
| **kubectl-neat** | 清理 YAML 冗余字段 | 社区 | v2.0.0 | 2k+ | Apache-2.0 |
| **kubectl-validate** | 客户端 YAML 验证 | K8s SIG | v0.0.5 | 300+ | Apache-2.0 |
| **kpt** | K8s 配置包管理工具 | Google | v1.0.0 | 2k+ | Apache-2.0 |
| **cuelang** | 配置语言与验证 | CUE | v0.12.0 | 7k+ | Apache-2.0 |
| **Jsonnet** | 数据模板语言 | Google | v0.20.0 | 7k+ | Apache-2.0 |
| **Tanka** | Jsonnet + K8s | Grafana | v0.30.0 | 3k+ | Apache-2.0 |
| **DevSpace** | K8s 开发工作流 | Loft | v6.3.0 | 4k+ | Apache-2.0 |
| **Tilt** | 本地 K8s 开发 | Tilt.dev | v0.33.0 | 7k+ | Apache-2.0 |
| **Okteto** | 云端开发环境 | Okteto | v3.5.0 | 3k+ | Apache-2.0 |
| **DevPod** | 开源 Codespaces 替代 | Loft | v0.6.0 | 8k+ | MPL-2.0 |
| **mirrord** | 本地代码接入集群 | MetalBear | v3.0.0 | 5k+ | MIT |
| **telepresence** | 本地开发流量拦截 | Ambassador | v2.22.0 | 6k+ | Apache-2.0 |
| **DevSpace** | K8s 开发工作流 | Loft | v6.3.0 | 4k+ | Apache-2.0 |
| **Reloader** | ConfigMap/Secret 变更自动重启 | Stakater | v1.3.0 | 7k+ | Apache-2.0 |
| **ConfigMap Controller / Reloader** | 配置热重载 | 社区 | - | - | Apache-2.0 |

---

## 参考链接

- [kustomize 文档](https://kubectl.docs.kubernetes.io/guides/config_management/)
- [Helm 文档](https://helm.sh/docs/)
- [yq 文档](https://mikefarah.gitbook.io/yq/)
- [cuelang 文档](https://cuelang.org/docs/)

---

## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[domain-18-manifests-patterns/README|Domain-32: Kubernetes YAML 配置完整参考手册]]
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 08 - Service 全类型 YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考
- 10 - Ingress / IngressClass YAML 配置参考

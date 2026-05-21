---
title: CoreDNS
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- prometheus
- coredns
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- 如何 CoreDNS
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- CoreDNS
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- etcd-basics
---

title: CoreDNS
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- 如何 CoreDNS
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CoreDNS
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md
  label: '故障树: dns'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# CoreDNS

> **成熟度**: Graduated | **加入时间**: 2017-02 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://coredns.io |
| **GitHub** | https://github.com/coredns/coredns |
| **文档** | https://coredns.io/manual |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Networking |

---

## 项目概述

### 简介
CoreDNS 是一个灵活、可扩展的 DNS 服务器，是 Kubernetes 的默认 DNS 服务，采用插件链式架构。

### 核心定位
CoreDNS 取代了 kube-dns 成为 Kubernetes 的标准 DNS 服务，通过插件架构提供高度可定制的 DNS 解析能力。

### 发展历程
- **2016**: Miek Gieben 创建 CoreDNS
- **2017-02**: 加入 CNCF 作为孵化项目
- **2019-01**: 成为 CNCF 毕业项目
- **2018**: 成为 Kubernetes 默认 DNS

---

## 核心功能

### 主要特性
- **插件架构**: 链式插件处理 DNS 请求
- **Kubernetes 集成**: 原生 K8s 服务发现
- **多后端支持**: 文件、etcd、数据库等
- **健康检查**: 上游健康状态监控
- **指标导出**: Prometheus 指标

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                        CoreDNS                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    Plugin Chain                         ││
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────────┐   ││
│  │  │ log │→│cache│→│k8s  │→│hosts│→│file │→│forward  │   ││
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────────┘   ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 基础配置
```
# Corefile
.:53 {
    errors
    health {
        lameduck 5s
    }
    ready
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
        ttl 30
    }
    prometheus :9153
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

### 验证测试
```bash
# 测试 DNS 解析
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup kubernetes.default

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns
```

---

## 参考资源

- [官方文档](https://coredns.io/manual)
- [GitHub Repo](https://github.com/coredns/coredns)
- [CNCF 项目页面](https://www.cncf.io/projects/coredns/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/networking-terms|K8s 网络术语参考]] — Cross-reference
- [[references/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution|Kubernetes 版本演进]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[skills/dns-fta|DNS 异常故障树分析]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/k8s-network-configuration-guide|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[skills/learn-04-service-basics|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/learn-12-common-problems|第十课：常见问题排查]] — Cross-reference
- [[skills/learn-analogy-dictionary|K8S 概念类比词典]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/kubernetes-changelog|Kubernetes 变更日志索引]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/dns-index|DNS 知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.9|coredns v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.8|coredns v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-0.9|coredns v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.3|coredns v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.7|coredns v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.12|coredns v1.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.6|coredns v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.13|coredns v1.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.2|coredns v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.5|coredns v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.10|coredns v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.14|coredns v1.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.0|coredns v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.4|coredns v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/coredns/RELEASE-NOTES-1.11|coredns v1.11 Release Notes]]

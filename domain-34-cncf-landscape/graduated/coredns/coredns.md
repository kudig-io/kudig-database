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
  path: ../topic-fta/list/dns-fta.md
  label: '故障树: dns'
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

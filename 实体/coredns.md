---
title: CoreDNS (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- coredns
- etcd
- prometheus
- grafana
- crd
- operator
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- 如何 CoreDNS
trigger_keywords:
- CoreDNS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CoreDNS

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **插件架构**: 链式插件处理 DNS 请求
- **Kubernetes 集成**: 原生 K8s 服务发现
- **多后端支持**: 文件、etcd、数据库等
- **健康检查**: 上游健康状态监控
- **指标导出**: Prometheus 指标

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

CoreDNS 是 Kubernetes 集群的默认 DNS 服务器，负责集群内服务发现和域名解析。它是 K8s 网络的关键基础设施。

## Corefile 配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
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
        forward . /etc/resolv.conf {
            max_concurrent 1000
        }
        cache 30 {
            success 9984 30
            denial 9984 5
            prefetch 10 60m 10%
        }
        loop
        reload
        loadbalance
    }
```

### 插件详解

| 插件 | 功能 | 配置要点 |
|------|------|----------|
| kubernetes | 集群内 Service/Pod 解析 | `pods insecure`, `ttl 30` |
| forward | 转发到上游 DNS | `max_concurrent 1000` |
| cache | 缓存 DNS 响应 | `success 9984 30` |
| loop | 检测转发环路 | 启动时检测 |
| reload | 热加载 Corefile | 30s 检查 |
| loadbalance | 轮询 A/AAAA | 默认启用 |
| autopath | 优化 search 路径 | 减少无效查询 |

## 运维操作

### 常用命令

```bash
# 🟢 查看 CoreDNS Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 🟢 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 🟢 查看 CoreDNS Service
kubectl get svc -n kube-system kube-dns

# 🟢 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# 🟢 测试 DNS 解析
kubectl exec -it <pod> -- nslookup kubernetes.default.svc.cluster.local
kubectl exec -it <pod> -- dig nginx.default.svc.cluster.local +short

# 🟢 查看 CoreDNS 指标
kubectl exec -n kube-system <coredns-pod> -- wget -qO- http://localhost:9153/metrics | grep coredns

# 🟡 重启 CoreDNS
kubectl rollout restart deployment coredns -n kube-system

# 🟢 查看 CoreDNS 资源使用
kubectl top pods -n kube-system -l k8s-app=kube-dns
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| DNS 解析超时 | CoreDNS 过载/上游不可达 | 扩容/检查上游 |
| NXDOMAIN | Service 不存在 | 检查名称/命名空间 |
| 间歇性失败 | conntrack UDP 竞态 | NodeLocal DNSCache |
| CoreDNS CrashLoop | 配置错误 | 检查 Corefile |
| 解析慢 | ndots + search | 降低 ndots |
| OOMKilled | 缓存膨胀 | 调整缓存大小/增加内存 |

### 排查流程

```
1. 检查 CoreDNS Pod 状态
   kubectl get pods -n kube-system -l k8s-app=kube-dns
       │
2. 测试 DNS 解析
   kubectl exec <pod> -- nslookup kubernetes.default
       │
3. 检查 CoreDNS 日志
   kubectl logs -n kube-system -l k8s-app=kube-dns
       │
4. 检查 Service/Endpoints
   kubectl get svc,endpoints -n kube-system kube-dns
       │
5. 检查 Pod DNS 配置
   kubectl exec <pod> -- cat /etc/resolv.conf
```

## 性能优化

### NodeLocal DNSCache

```yaml
# 部署 NodeLocal DNSCache 减少 CoreDNS 压力
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.28
        args: ["-localip", "169.254.20.10", "-conf", "/etc/Corefile"]
```

### 优化建议

1. **启用缓存** - `cache 30` 减少上游查询
2. **部署 NodeLocal DNSCache** - 本地缓存，减少跨节点查询
3. **调整 ndots** - 减少无效 search 查询
4. **自动扩缩** - HPA 基于 CPU/请求量
5. **监控告警** - 延迟、错误率、Pod 状态

## 检查清单

- [ ] 理解 CoreDNS 插件架构
- [ ] 掌握 Corefile 配置
- [ ] 能排查 DNS 解析问题
- [ ] 了解 NodeLocal DNSCache
- [ ] 掌握性能优化技巧
- [ ] 能配置监控告警

## 参考链接

- [[etcd]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]

## Related

- [[kuadrant]] — Kuadrant
- [[notary-project]] — Notary Project
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 14-coredns-configuration-corefile
- 28-coredns-troubleshooting-optimization
- 11-dns-service-discovery-coredns
- 15-coredns-plugins-reference
- 13-coredns-architecture-principles
- coredns
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- [[实体/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[实体/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[实体/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[实体/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[故障诊断/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[概念/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[概念/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[概念/kubernetes-version-evolution.md|Kubernetes 版本演进]] — Cross-reference
- [[概念/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[技能/网络/dns/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/工作负载/statefulset/skill-21-statefulset-failure.md|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[技能/网络/cni/最佳实践/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[技能/网络/service/培训/learn-04-service-basics.md|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-analogy-dictionary.md|K8S 概念类比词典]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/kubernetes-changelog.md|Kubernetes 变更日志索引]] — Cross-reference
- [[实体/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/dns-index.md|DNS 知识图谱索引]]


<!-- risk-assessed -->

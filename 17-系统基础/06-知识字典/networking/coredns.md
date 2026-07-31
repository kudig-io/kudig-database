---
title: CoreDNS
description: CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的
  DN...
summary: CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的
  DN...
category: dictionary
tags:
- k8s
- glossary
- coredns
- dns
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- CoreDNS 详解
trigger_keywords:
- CoreDNS
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CoreDNS

> **英文名**: CoreDNS

## 概述

CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的 DNS 解析服务，是集群内服务发现的基础设施。

## 核心概念/原理

### 架构

CoreDNS 以 Deployment 形式运行在 kube-system 命名空间，通过 ConfigMap（`coredns`）配置插件链。

### 核心插件

| 插件 | 功能 |
|------|------|
| kubernetes | 解析集群内 Service/Pod DNS |
| forward | 转发外部 DNS 查询 |
| cache | DNS 响应缓存 |
| loop | 检测 DNS 转发循环 |
| errors | 错误日志 |
| health | 健康检查端点 |
| prometheus | 指标暴露 |

## 关键机制或特性

- 插件链按 Corefile 中的顺序执行。
- 支持 DNS-over-TLS 和 DNS-over-gRPC。
- 通过 `hosts` 插件可添加自定义 DNS 记录。
- `rewrite` 插件支持 DNS 记录重写。
- 指标通过 `/metrics` 端点暴露给 Prometheus。

## 使用场景与最佳实践

- 大集群启用 NodeLocal DNSCache 减少 CoreDNS 压力。
- 使用 `cache` 插件合理设置 TTL 减少查询量。
- 排查 DNS 问题时检查 CoreDNS Pod 日志和资源使用。
- 配置 `forward` 插件的上游 DNS 服务器。
- 使用 `rewrite` 插件处理内部域名映射。

## 参考链接

- [CoreDNS Official](https://coredns.io/)

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              CoreDNS (K8s DNS)                      │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ kubernetes  │  │ cache        │  │ forward   │  │
│  │ plugin      │  │ plugin       │  │ plugin    │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │         Plugin Chain (Corefile)             │  │
│  │  errors → health → kubernetes → cache →     │  │
│  │  loop → forward → reload                    │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（coredns/coredns）

| 模块 | 路径 | 职责 |
|------|------|------|
| 主入口 | `coremain/` | 服务启动、插件加载 |
| kubernetes 插件 | `plugin/kubernetes/` | K8s Service/Endpoint DNS 记录 |
| cache 插件 | `plugin/cache/` | DNS 响应缓存 |
| forward 插件 | `plugin/forward/` | 上游 DNS 转发 |
| Corefile 解析 | `core/dnsserver/` | 配置文件解析与服务器编排 |

### DNS 解析流程（K8s 内部）

1. Pod 发起 DNS 查询（如 `svc.default.svc.cluster.local`）
2. 请求到达 CoreDNS Service（ClusterIP）
3. kubernetes 插件查询 API Server 获取 Service/Endpoint
4. 生成 A/AAAA/SRV 记录响应
5. cache 插件缓存结果（默认 30s TTL）
6. 返回响应给 Pod

## 生产案例

### 案例 1：CoreDNS Pod 资源不足导致 DNS 延迟

| 时间 | 事件 |
|------|------|
| 12:00 | 多个服务报告 DNS 解析超时 |
| 12:05 | 检查 CoreDNS Pod：CPU 使用率 95%，内存接近限制 |
| 12:10 | 确认：集群扩容到 500 节点，CoreDNS 副本数未相应增加 |
| 12:20 | 修复：扩容 CoreDNS 到 5 副本，增加资源限制，启用 NodeLocal DNSCache |

**修复命令**：
```bash
# 检查 CoreDNS 负载 🟢 只读
kubectl top pods -n kube-system -l k8s-app=kube-dns
# 查看 DNS 查询延迟 🟢 只读
kubectl exec -it test-pod -- nslookup kubernetes.default.svc.cluster.local
# 扩容 CoreDNS 🟡 中风险
kubectl scale deploy/coredns -n kube-system --replicas=5
```

### 案例 2：ndots 配置导致外部域名解析慢

**现象**：Pod 访问外部域名（如 `api.github.com`）延迟 5-10s。

**诊断**：默认 `ndots:5` 导致先尝试 `api.github.com.default.svc.cluster.local` 等后缀，多次失败后才查询真实域名。

**修复**：在 Pod spec 中设置 `dnsConfig.options: [{name: ndots, value: "2"}]`，或使用完全限定域名（FQDN）加点号结尾。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有 CoreDNS Pod 不可用 | 紧急恢复，检查节点状态 |
| P1 | DNS 解析延迟 > 1s | 扩容 CoreDNS，启用 NodeLocal DNSCache |
| P2 | 缓存命中率低 | 调整 cache TTL，检查查询模式 |

## 面试要点

1. **Q：CoreDNS 与 kube-dns 的区别？为什么 K8s 选择 CoreDNS？**
   A：kube-dns 基于 SkyDNS，使用 3 个容器（kubedns/dnsmasq/sidecar），配置复杂；CoreDNS 是单进程插件化架构，通过 Corefile 配置，资源占用更低。K8s 1.13+ 默认使用 CoreDNS，因为：① 插件化架构易扩展；② 支持自定义 DNS 记录；③ 更好的性能和稳定性；④ 单一二进制维护简单。

2. **Q：NodeLocal DNSCache 解决了什么问题？**
   A：大规模集群中，所有 Pod 的 DNS 查询都经过 CoreDNS Service（iptables/IPVS 转发），造成：① CoreDNS 负载高；② 网络跳数多；③ conntrack 表压力大。NodeLocal DNSCache 在每个节点运行本地 DNS 缓存（node-local-dns DaemonSet），Pod 直接查询本地缓存，大幅降低延迟和 CoreDNS 压力。

3. **Q：如何排查 K8s DNS 解析问题？**
   A：① `kubectl exec -it pod -- nslookup svc.ns.svc.cluster.local` 测试解析；② `kubectl logs -n kube-system -l k8s-app=kube-dns` 检查 CoreDNS 日志；③ `kubectl get endpoints kube-dns -n kube-system` 确认 Service 端点；④ 使用 `dig @coredns-pod-ip` 直接查询；⑤ 检查 Corefile 配置和插件链。

## Related

- [[17-系统基础/06-知识字典/networking/dns-resolution.md|DNS Resolution]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]
- [[17-系统基础/06-知识字典/networking/endpoint.md|Endpoints]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->

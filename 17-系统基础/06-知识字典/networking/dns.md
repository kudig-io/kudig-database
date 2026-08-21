---
title: 域名服务
description: Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes
  的默...
summary: Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes
  的默...
category: dictionary
tags:
- k8s
- glossary
- dns
- coredns
- networking
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 域名服务 是什么
- DNS 详解
trigger_keywords:
- 域名服务
- DNS
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 域名服务

> **英文名**: DNS

## 概述

Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes 的默认 DNS 实现。

## 核心概念/原理

### DNS 记录格式

- **Service**：`<service-name>.<namespace>.svc.cluster.local`
- **Headless Service**：返回所有后端 Pod 的 IP 地址。
- **Pod**：`<pod-ip-dashed>.<namespace>.pod.cluster.local`
- **StatefulSet Pod**：`<pod-name>.<headless-service>.<namespace>.svc.cluster.local`

### CoreDNS

CoreDNS 是 CNCF 毕业项目，通过 Kubernetes 插件机制部署。它支持丰富的插件生态，包括缓存、转发、日志等。

## 关键机制或特性

- `ndots` 配置影响 DNS 查询行为（默认 5，可能导致额外查询）。
- DNS 缓存（NodeLocal DNSCache）可显著减少 CoreDNS 负载。
- CoreDNS 的 `forward` 插件可将外部域名转发到上游 DNS。
- `dnsConfig` 字段允许自定义 Pod 的 DNS 配置。

## 使用场景与最佳实践

- 生产环境部署 NodeLocal DNSCache 减少 CoreDNS 压力。
- 调整 `ndots: 2` 减少不必要的 DNS 查询。
- 监控 CoreDNS 的 QPS、延迟和缓存命中率。
- 为外部服务配置 ExternalName Service 或 CoreDNS rewrite 规则。

## 架构深度解析

### 集群 DNS 架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用 Pod                              │
│  DNS 查询: my-svc.ns.svc.cluster.local                  │
│          │                                              │
│          ▼                                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │  本地 DNS 缓存（NodeLocal DNSCache，可选）         │   │
│  │  - 每节点 DaemonSet，监听 169.254.20.10          │   │
│  │  - 缓存集群域名，降低 CoreDNS 负载                │   │
│  └──────────────────────────────────────────────────┘   │
│          │  iptables/IPVS 重定向                         │
│          ▼                                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │  CoreDNS（集群 DNS 服务）                          │   │
│  │  - cluster.local 区域：Service/Pod 记录           │   │
│  │  - kubernetes 插件：headless A/SRV                │   │
│  │  - forward 插件：外部域名转发上游                  │   │
│  │  - cache 插件：TTL 缓存                            │   │
│  └──────────────────────────────────────────────────┘   │
│          │                                              │
│          ▼                                              │
│  上游 DNS（外部域名） / 外部服务（ExternalName/rewrite）  │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（coredns/coredns）

| 模块 | 路径 | 职责 |
|------|------|------|
| Core | `core/` | 插件注册与请求流水线 |
| kubernetes 插件 | `plugin/kubernetes/` | Service/Pod 记录生成（A/SRV/PTR） |
| forward 插件 | `plugin/forward/` | 外部域名转发（健康探测/重试） |
| cache 插件 | `plugin/cache/` | 响应缓存（TTL 管理） |
| NodeLocal 缓存 | `cluster/addons/dns/nodelocaldns` | 节点级缓存配置生成 |

### DNS 解析链路（以 Service 查询为例）

1. 应用发起查询，解析器按 `ndots:5` 策略尝试多个搜索域（`my-svc.ns.svc`、`my-svc.ns`、`my-svc`）
2. 集群内域名请求被重定向至 CoreDNS（或 NodeLocal 缓存）
3. CoreDNS 的 kubernetes 插件查 Service 列表：ClusterIP 服务返回 VIP 记录，headless 返回全部 Endpoint A 记录
4. 外部域名经 forward 插件转发到上游（`/etc/resolv.conf` 的 nameserver）
5. 响应按 TTL 缓存，TTL 到期后重新查询

## 生产案例

### 案例 1：ndots=5 导致外部域名解析延迟爆炸

| 时间 | 事件 |
|------|------|
| 09:30 | 应用访问外部 API 频繁超时，内部服务正常 |
| 09:40 | CoreDNS 日志显示大量 `NXDOMAIN` 查询外部域名 |
| 09:50 | 定位为 ndots=5：查询 `api.example.com` 先尝试 4 个集群内搜索域全部 NXDOMAIN 后才转发 |
| 10:10 | 调整 ndots=2，外部域名直接转发，延迟恢复 |

**根因**：`ndots:5` 意味着域名含 5 个点以下时先走搜索域列表，每次解析产生 4-5 次 NXDOMAIN 查询；外部域名场景放大延迟与 CoreDNS 负载。

**修复命令**：
```bash
# 查看当前 DNS 配置 🟢 只读
kubectl exec -it <pod> -- cat /etc/resolv.conf
# 修改 Deployment 的 dnsConfig（YAML）🟡 中风险
# dnsConfig:
#   options:
#     - name: ndots
#       value: "2"
# 或全局调整 kubelet 的 cluster-dns 参数
```

### 案例 2：NodeLocal DNSCache 部署后服务发现失效

**现象**：部署 NodeLocal DNSCache 后，部分 Pod 无法解析集群域名。

**诊断**：NodeLocal 缓存依赖 iptables 重定向规则（169.254.20.10）；节点重启后规则未重建，或 cache 与 CoreDNS 版本不匹配导致 `SERVFAIL`。

**修复**：验证规则 `iptables -t nat -L | grep 169.254.20.10`；重建 nodelocaldns DaemonSet 并确认 CoreDNS 上游配置；检查缓存 Pod 的 `--upstream` 指向正确。

## 对比评测

| 维度 | 默认 CoreDNS | NodeLocal 缓存 | 外部 DNS（ExternalDNS） |
|------|-------------|----------------|------------------------|
| 集群内解析 | ✅ | ✅（加速） | ❌ |
| 外部域名 | forward 转发 | 同上 | 同步到 DNS 服务商 |
| 延迟 | 中（跨节点） | 低（节点内） | - |
| 负载 | 集中 | 分散 | - |
| 适用场景 | 默认 | 大规模集群 | 服务对外暴露 |

**选型建议**：默认集群用 CoreDNS；规模大/延迟敏感加 NodeLocal；对外域名自动管理配 ExternalDNS。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 解析失败 | `dig my-svc.ns.svc @10.96.0.10` | CoreDNS 异常或转发失败 |
| NXDOMAIN 风暴 | CoreDNS 日志查查询统计 | ndots 配置不合理 |
| 延迟高 | `kubectl top pod -n kube-system` | CoreDNS 过载或缓存缺失 |
| SERVFAIL | `kubectl logs deploy/coredns` | 上游 DNS 不可达 |

## 生产部署清单

- [ ] 外部域名场景设置 ndots=2（业务侧 dnsConfig）
- [ ] 规模 >100 节点部署 NodeLocal DNSCache
- [ ] CoreDNS 多副本 + HPA + 反亲和部署
- [ ] 监控 CoreDNS QPS/延迟/缓存命中率
- [ ] 定期巡检 resolv.conf 与搜索域配置

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | CoreDNS 故障导致全集群解析失败 | 立即扩容副本并检查上游 DNS 连通性 |
| P1 | CoreDNS 版本升级（插件行为变化） | 预发验证 kubernetes/forward 插件兼容性 |
| P2 | 集群规模增长 | 部署 NodeLocal DNSCache 分散压力 |

## 面试要点

> 以下 Q&A 覆盖集群 DNS 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：ndots 参数如何影响 DNS 查询行为？**
   A：`ndots:5` 表示域名中点数 ≤5 时先依次尝试搜索域列表（默认含 `ns.svc.cluster.local`、`svc.cluster.local`、`cluster.local` 等），全部 NXDOMAIN 后才按绝对域名查询；`ndots:2` 让含 ≥2 个点的域名直接查询，减少外部域名解析的无效查询次数与延迟。

2. **Q：CoreDNS 如何生成 Service 的 DNS 记录？**
   A：kubernetes 插件监听 Service 与 Endpoints：ClusterIP 服务生成 1 条 A 记录（域名 → VIP）；headless 服务为每个 Ready Endpoint 生成独立 A 记录；同时生成 SRV 记录（端口 + 优先级）与 PTR 记录（反解），并支持 `publishNotReadyAddresses` 控制发布范围。

3. **Q：NodeLocal DNSCache 的工作原理与收益？**
   A：每节点运行 nodelocaldns（监听 169.254.20.10），通过 iptables 规则将 Pod 的 DNS 流量重定向到本节点缓存；缓存代理集群域名（转发 CoreDNS）并缓存外部域名（forward 上游），避免跨节点查询，降低 CoreDNS 负载与解析延迟。注意节点重启后需验证规则重建。

## 参考链接

- [DNS - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

## Related

[[23-实体/02-K8s核心组件/coredns.md|CoreDNS]]


<!-- risk-assessed -->

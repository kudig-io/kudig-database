---
title: DNS 解析
description: DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP
  地址的过程。集群内部...
summary: DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP 地址的过程。集群内部...
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
- DNS 解析 是什么
- DNS Resolution 详解
trigger_keywords:
- DNS 解析
- DNS Resolution
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DNS 解析

> **英文名**: DNS Resolution

## 概述

DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP 地址的过程。集群内部 DNS 由 CoreDNS 提供，遵循 `<service>.<namespace>.svc.cluster.local` 的命名格式。

## 核心概念/原理

### DNS 记录格式

| 资源类型 | DNS 格式 | 示例 |
|----------|----------|------|
| Service (ClusterIP) | `<svc>.<ns>.svc.cluster.local` | `nginx.default.svc.cluster.local` |
| Headless Service | 返回所有 Pod IP | `db.default.svc.cluster.local` → 多个 A 记录 |
| Pod | `<pod-ip-dashed>.<ns>.pod.cluster.local` | `10-244-0-5.default.pod.cluster.local` |
| SRV 记录 | `_<port>._<proto>.<svc>.<ns>.svc.cluster.local` | 用于发现命名端口 |

### 解析流程

Pod 内的 DNS 查询 → Pod DNS Config → CoreDNS → 上游 DNS（如需要）

## 关键机制或特性

- CoreDNS 以 Deployment 形式运行在 kube-system 命名空间。
- Pod 的 `/etc/resolv.conf` 由 kubelet 自动配置指向 CoreDNS。
- `dnsPolicy` 控制 Pod 的 DNS 行为：`ClusterFirst`（默认）、`Default`、`None`。
- NodeLocal DNSCache 减少 CoreDNS 压力，提升解析性能。

## 使用场景与最佳实践

- 排查 DNS 问题时使用 `nslookup` 或 `dig` 测试解析。
- 大集群启用 NodeLocal DNSCache 避免 CoreDNS 成为瓶颈。
- 使用 `ndots:2` 减少不必要的域名后缀搜索。
- 外部 DNS 查询使用 ExternalDNS 管理云 DNS 记录。

## 架构深度解析

### DNS 解析链路

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 内应用（gethostbyname / getaddrinfo）                    │
│   │                                                            │
│   ▼                                                            │
│  /etc/resolv.conf（kubelet 注入）                              │
│  nameserver 169.254.20.10（NodeLocal DNSCache）或 CoreDNS     │
│  search  default.svc.cluster.local svc.cluster.local cluster  │
│  ndots: 5                                                      │
│   │                                                            │
│   ▼                                                            │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ NodeLocal DNSCache（DaemonSet，监听 169.254.20.10）      │  │
│  │  ├─ 集群域请求 → 转发 CoreDNS                           │  │
│  │  ├─ 外部域名 → 直连上游（可配置）                       │  │
│  │  └─ 缓存 TTL 内结果                                    │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │                                                            │
│   ▼                                                            │
│  CoreDNS（Deployment 副本，kube-system）                       │
│  ├─ kubernetes 插件：解析 Service/Pod 记录（etcd watch）       │
│  ├─ cache 插件：节点级缓存                                    │
│  └─ forward 插件：外部域名 → 上游 DNS（/etc/resolv.conf）     │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（coredns/coredns）

| 模块 | 路径 | 职责 |
|------|------|------|
| kubernetes 插件 | `plugin/kubernetes/` | 监听 Service/EndpointSlice 事件，按命名规则应答 A/AAAA/SRV 记录 |
| cache 插件 | `plugin/cache/` | 基于 TTL 的缓存，支持否定缓存（NXDOMAIN） |
| forward 插件 | `plugin/forward/` | 异步转发外部域名，支持多个上游与健康检查 |
| host-local 配置 | kubelet `pkg/kubelet/network/dns` | 生成 Pod `/etc/resolv.conf`（search/ndots/options） |

### 流程步骤

1. kubelet 根据 `dnsPolicy`（ClusterFirst）注入 resolv.conf，`nameserver` 指向 CoreDNS Service ClusterIP（或 NodeLocal DNSCache 的链路本地 IP）。
2. 应用发起查询，glibc 按 `ndots:5` 规则先尝试带 search 后缀的完整名（`svc.ns.svc.cluster.local`）。
3. CoreDNS kubernetes 插件命中本地缓存/etcd watch 数据，直接应答。
4. 非集群域名经 forward 插件转发上游，结果经 cache 插件缓存。
5. 若 Pod 使用 `dnsPolicy: None`，则完全由 `dnsConfig` 字段控制解析行为。

## 生产案例

### 案例 1：大促期间 CoreDNS 成为 DNS 解析瓶颈

| 时间 | 事件 |
|------|------|
| 09:58 | 业务压测开始，P99 延迟从 30ms 飙升到 2s |
| 10:00 | `kubectl top pod -n kube-system` 显示 CoreDNS 3 副本 CPU 均 95%+ |
| 10:03 | dmesg 出现 `nf_conntrack: table full, dropping packet` |
| 10:05 | 检查 conntrack 条目：`conntrack -L | wc -l` 达 80 万（上限 65 万） |
| 10:20 | 扩容 CoreDNS 至 6 副本 + 接入 NodeLocal DNSCache |
| 10:35 | P99 恢复至 45ms |

**根因**：所有 Pod 直连 CoreDNS ClusterIP，conntrack 条目随 DNS 连接暴涨；并发查询超时后应用无限重试形成雪崩。
**修复命令**：
```bash
# 检查 CoreDNS 副本数与资源使用 🟢 只读
kubectl top pods -n kube-system -l k8s-app=kube-dns
# 安装 NodeLocal DNSCache 🟡 中风险
kubectl apply -f https://raw.githubusercontent.com/kubernetes/dnsutils/master/NodeLocalDNSCache.yaml
# 验证 Pod 是否走本地缓存 🟢 只读
kubectl exec <pod> -- cat /etc/resolv.conf
```

### 案例 2：应用偶发 DNS 超时（5s timeout）

**现象**：服务注册中心（Consul/Nacos）频繁报 DNS 解析超时，但集群内 `nslookup` 正常。
**诊断**：抓包发现请求未到 CoreDNS 即被 conntrack 丢弃；`sysctl net.netfilter.nf_conntrack_max` 已耗尽（集群 Pod 数 > 3 万，DNS 连接 5s 超时导致 conntrack 槽位短时间被占满）。
**修复**：调大 conntrack 表（`sysctl -w net.netfilter.nf_conntrack_max=1000000` 并写入 `/etc/sysctl.d/`）；同时为 Pod 配置 `options single-request-reopen` 规避 glibc 双栈查询 bug；最终方案是部署 NodeLocal DNSCache 将 DNS 连接本地化。

## 对比评测

| 维度 | CoreDNS 直连 | NodeLocal DNSCache | 外部 DNS（ExternalDNS） |
|------|-------------|-------------------|----------------------|
| 部署位置 | 集群内 Deployment | 每节点 DaemonSet | 集群内 + 云 DNS API |
| 主要瓶颈 | conntrack 表、CPU | 内存（缓存条目） | 上游延迟 |
| 适用规模 | < 500 节点 | > 500 节点或高 QPS | 与云 LB/DNS 联动 |
| 故障模式 | 单点（无缓存时） | 缓存陈旧（TTL 内） | API 限流 |

**选型建议**：500 节点以上或 DNS QPS > 5k 时强制接入 NodeLocal DNSCache；跨云多集群用 ExternalDNS 统一管理记录。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| `server misbehaving` | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | CoreDNS 副本不足/驱逐 |
| 部分域名解析慢 | `kubectl exec <pod> -- dig @<coredns-ip> <domain> +stats` | forward 上游抖动、cache 命中率低 |
| Pod 内 curl 域名失败但 IP 通 | `cat /etc/resolv.conf`；`nslookup` | search 域缺失、ndots 配置、dnsPolicy 错误 |
| 服务名解析到旧 IP | `kubectl get endpoints <svc>`；`dig <svc> A` | EndpointSlice 更新延迟、缓存 TTL 未过期 |
| NodeLocal DNSCache 不生效 | `kubectl get ds -n kube-system node-local-dns`；`iptables -t nat -L` | DaemonSet 未就绪、iptables 规则未注入 |

## 生产部署清单

- [ ] CoreDNS 副本数 ≥ 2 且配置 PDB；开启 `prefer_udp` 与负缓存
- [ ] 大集群部署 NodeLocal DNSCache 并验证 Pod resolv.conf 指向 169.254.20.10
- [ ] conntrack 上限已按节点 Pod 数估算并调整（`nf_conntrack_max`）
- [ ] forward 插件上游配置为多供应商（云 DNS + 自建），开启 health_check
- [ ] 对 CoreDNS 配置 Prometheus 指标采集（`prometheus` 插件 + `kubedns` 指标）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | CoreDNS 版本存在已知 CVE（如 CVE-2024-xxxx） | 立即升级，先验证 configmap 兼容性 |
| P0 | DNS QPS 触顶、conntrack 丢包 | 先加 NodeLocal DNSCache，再评估扩容 |
| P1 | 需要自定义域名（如 `internal.example.com`） | 配置 CoreDNS rewrite/hosts 插件，灰度验证 |
| P1 | 多集群需要统一 DNS 出口 | 部署 ExternalDNS + 云 DNS 策略 |
| P2 | 版本满足需求且无告警 | 跟随 K8s minor 版本同步升级 CoreDNS |

## 面试要点

> 以下 Q&A 覆盖 DNS 解析面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Kubernetes 中 Pod 的 DNS 解析流程是怎样的？**
   A：Pod 的 resolv.conf 由 kubelet 注入，nameserver 指向 CoreDNS Service ClusterIP（或 NodeLocal DNSCache 的 169.254.20.10），search 域为 `<ns>.svc.cluster.local svc.cluster.local cluster.local`。查询时 glibc 按 ndots（默认 5）决定是否先带 search 域解析，最终命中 CoreDNS kubernetes 插件（Service/Pod 记录）或 forward 插件（外部域名）。dnsPolicy 有 ClusterFirst/Default/None 三种。

2. **Q：为什么大规模集群要使用 NodeLocal DNSCache？**
   A：所有 Pod 的 DNS 查询都经过 iptables DNAT 到 CoreDNS ClusterIP，每个查询占用一条 conntrack 条目；集群规模大时 conntrack 表耗尽导致丢包、DNS 超时。NodeLocal DNSCache 在每节点运行一个本地 DNS 代理（监听链路本地地址 169.254.20.10），查询不出节点，conntrack 压力骤减，同时缓存提升响应速度。本质是把"中心化 DNS"变为"节点级 DNS"。

3. **Q：遇到"服务间歇性 DNS 解析失败"如何排查？**
   A：按四层递进排查：① 确认 CoreDNS 副本健康与资源水位（`kubectl top`）；② 检查 conntrack 表容量与丢包统计（`dmesg | grep nf_conntrack`）；③ 在故障 Pod 内用 `dig +stats +time` 复现并确认是 UDP 丢包还是超时；④ 检查 forward 上游健康与 cache 命中率（CoreDNS Prometheus 指标 `coredns_dns_request_duration_seconds`）。常见根因依次为 conntrack 耗尽、上游抖动、iptables 规则异常。

## 参考链接

- [DNS for Services and Pods - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

## Related

- [[17-系统基础/06-知识字典/networking/coredns.md|CoreDNS]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]
- [[17-系统基础/06-知识字典/networking/endpoint.md|Endpoints]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->

---
title: 网络最佳实践
description: 大规模 Kubernetes 集群网络的 CNI 选型、IP 地址规划、NetworkPolicy、DNS 性能、kube-proxy 模式、Ingress 与负载均衡的生产级最佳实践
summary: 覆盖 CNI 选型矩阵、大规模 IP 规划、默认拒绝 NetworkPolicy、NodeLocal DNS、IPVS 模式、Ingress/LB 高可用与网络性能调优
category: references
tags:
- k8s
- best-practices
- networking
- cni
- networkpolicy
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 网络工程师
- 平台工程师
estimated_read_time: 25min
---

# 网络最佳实践

> 网络是大规模集群中最容易出现"隐性退化"的层：规则膨胀、DNS 超时、conntrack 溢出往往在几千节点后才暴露。本文覆盖 CNI 选型、IP 规划、策略、DNS、kube-proxy、南北向入口与性能调优。

## 1. CNI 选型

### 1.1 主流方案对比

| 方案 | 数据面 | 优势 | 适用规模 |
|---|---|---|---|
| Calico | iptables/eBPF、VXLAN/BGP | 成熟、BGP 模式性能好、策略强 | 大、超大型 |
| Cilium | eBPF | 无 kube-proxy 依赖、L7 策略、可观测性（Hubble） | 大、超大型（eBPF 在大规模下规则复杂度最优） |
| Flannel | VXLAN | 简单 | 中小型，大规模不推荐 |
| 云厂商 VPC CNI（Terway/AWS VPC CNI） | 原生 VPC 路由 | Pod 直连 VPC、性能最优、可被 VPC 安全组管控 | 云上大规模首选之一 |

### 1.2 选型原则

- 云上生产优先 **VPC 原生 CNI**（Pod IP 即 VPC IP，省去 overlay 开销，利于安全组/NAC 管控）
- 大规模 + 强策略需求 → **Cilium（eBPF）** 或 **Calico eBPF 模式**，避免 iptables 规则爆炸
- 确认内核版本支持（eBPF 建议 ≥ 5.4）

## 2. IP 地址规划（大规模命门）

- Pod CIDR 与 Service CIDR **必须不重叠**、不与 VPC/IDC 路由冲突
- 按最终规模 ×2 规划：`/12` 级 Pod 网段在大集群中并不浪费
- 每节点 IP 数 = `maxPods + buffer`，与 kubelet `maxPods` 严格对齐
- VPC CNI 场景注意：
  - 单节点可挂 ENI 数与每 ENI IP 数决定单节点 Pod 密度上限
  - 预热 IP 池（`minimum-ip-target` / 弹性网卡辅助 IP 池），避免冷启动分配延迟
- 建立 **IPAM 使用率监控**，水位 > 70% 预警扩容

## 3. NetworkPolicy

### 3.1 基线策略：默认拒绝 + 白名单放行

```yaml
# 每个命名空间的兜底策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
---
# 放行 DNS（没有这条 Pod 无法解析）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - port: 53
      protocol: UDP
    - port: 53
      protocol: TCP
```

### 3.2 大规模策略治理

- 策略数量与命中规则会线性增加 CNI 数据面负担：定期清理无效策略、合并冗余规则
- 用 Cilium/Calico 的策略审计/观测能力（Hubble、`calicoctl`）验证策略生效情况
- 东西向零信任：跨命名空间访问必须显式授权；管理面组件（监控采集）统一放行标签

## 4. 集群 DNS（大规模性能关键）

1. **NodeLocal DNSCache 必装**（DaemonSet 本地缓存），收益：
   - 消除跨节点 DNS 流量的 conntrack 表压力
   - 降低 CoreDNS 负载 50%+
   - 解析延迟从毫秒级降到微秒级
2. CoreDNS 用 `cluster-proportional-autoscaler` 按集群规模自动扩缩
3. Corefile 调优：

```text
.:53 {
    errors
    health { lameduck 15s }        # 优雅下线，避免滚动更新丢包
    ready
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
    }
    prometheus :9153
    forward . /etc/resolv.conf { max_concurrent 1000 }
    cache 30
    loop
    reload
    loadbalance
}
```

4. 应用侧优化：对主要访问集群内服务的 Pod 设置 `dnsConfig.options.ndots: "2"`，减少 search 域遍历（可降低 CoreDNS QPS 数倍）
5. 监控：`coredns_dns_request_duration_seconds`、NodeLocal 的 `node_dns_cache_hit_rate`

## 5. kube-proxy 与 Service 数据面

| 模式 | 规则复杂度 | 大规模表现 |
|---|---|---|
| iptables | O(N) 规则顺序匹配 | >5,000 Service 后更新延迟秒级，CPU 抖动明显 |
| IPVS | 哈希表 O(1) | 大规模推荐，规则同步快、支持多种调度算法 |
| eBPF（Cilium kube-proxy replacement） | eBPF map | 最优，去掉 kube-proxy 组件本身 |

- iptables/IPVS 模式下大规模注意：
  - `--iptables-min-sync-period` 调大到 5s+，降低全量刷规则频率
  - `conntrack` 表：`net.netfilter.nf_conntrack_max` 调大（大节点 ≥ 1M），并监控使用率
- 大 Service（后端 >500 Pod）考虑用 `externalTrafficPolicy: Local` 减少跨节点转发与规则量
- Session 亲和（`sessionAffinity`）会显著放大规则量，非必要不使用

## 6. 南北向入口：Ingress 与负载均衡

### 6.1 Ingress Controller 高可用

- 至少 2 副本跨 AZ + PDB + HPA（按 QPS/连接数扩缩）
- 部署到**独立入口节点池**，taint 隔离，避免业务抢占
- 前置云 LB / 硬件 LB，健康检查指向 Ingress 的 `/healthz`
- `externalTrafficPolicy: Local` 保留源 IP（注意配合 Pod 反亲和保证每个节点都有入口 Pod）

### 6.2 大规模入口治理

- Ingress 对象数量膨胀（>5,000 条）会拖慢 controller 配置刷新：定期清理、合并域名
- Nginx Ingress 调优：`worker-processes: auto`、调大 `keepalive`、开启 `reuseport`
- 多 Ingress Controller 分片（按 `ingressClass` 隔离核心业务与普通业务）
- TLS 证书集中管理（cert-manager + 通配符证书），避免证书散落各处

### 6.3 Gateway API

新集群优先评估 Gateway API 替代 Ingress：角色分离（infra/app）、路由能力更强、多实现可选（Envoy Gateway / Cilium / Istio）。

## 7. 网络性能调优

| 项目 | 建议 |
|---|---|
| MTU | Overlay（VXLAN）场景 MTU = 物理 MTU - 50（如 1450）；VPC CNI 用物理 MTU；错误 MTU 是大规模集群 P99 抖动的常见根因 |
| conntrack | `nf_conntrack_max` 按内存调大（每连接 ~300B）；监控 `node_nf_conntrack_entries` 使用率 |
| 内核参数 | `net.core.somaxconn=32768`、`net.ipv4.tcp_max_syn_backlog`、`net.ipv4.ip_local_port_range` 扩大、`tcp_tw_reuse` |
| 中断与 RSS | 大流量节点开启网卡多队列 + IRQ 亲和；关注软中断均衡 |
| DNS | 见第 4 节 |
| SNAT 端口耗尽 | 出向访问集中（NAT 网关）场景监控端口耗尽；考虑多 EIP/多 NAT 实例 |

## 8. 多集群网络

- 跨集群服务发现：MCS API / Submariner / 云厂商多集群网络方案
- 多集群 IP 规划必须**全局唯一表**管理，防止集群间网段重叠导致无法互联
- 东西向跨集群流量必经统一网关/服务网格，便于审计与熔断

## 9. 常见反模式

| 反模式 | 后果 |
|---|---|
| 大规模集群用 iptables 模式硬扛 | Service 更新延迟、CPU 毛刺 |
| 无 NodeLocal DNS | CoreDNS 雪崩、全集群解析超时 |
| MTU 配置错误 | 大包丢包、TCP 重传、P99 抖动，极难排查 |
| NetworkPolicy 只配不配验证 | 策略形同虚设或误杀关键流量 |
| Ingress 与业务混部 | 流量高峰互相抢占，入口先崩 |
| Pod CIDR 规划过小 | 集群扩容到一半 IP 耗尽，被迫重建 |

## Related

- [[01-overview|大规模集群总览与规模基线]]
- [[02-cluster-configuration|集群配置最佳实践]]
- [[08-security-defense-checklist|护网检查项（网络隔离部分）]]
- [[20-最佳实践/07-scenarios/network-diagnosis|网络诊断场景]]

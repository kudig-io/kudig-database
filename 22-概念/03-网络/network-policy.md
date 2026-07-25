---
title: NetworkPolicy
summary: NetworkPolicy 是 Kubernetes 中用于定义 Pod 之间网络通信规则的 API 对象。
category: concepts
tags:
- core-concept
- k8s
- networking
- security
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# NetworkPolicy

## 概述

NetworkPolicy 是 Kubernetes 内置的网络流量控制 API 对象，用于限定 Pod 之间（东西向）以及 Pod 与外部（南北向）的入站（ingress）和出站（egress）通信。它是实现**零信任网络**和**微隔离（micro-segmentation）**的核心手段：默认情况下，集群中所有 Pod 网络互通；一旦对某个 Pod 应用了一条 NetworkPolicy，未被显式允许的流量即被拒绝。NetworkPolicy 工作在 L3/L4（IP/端口），不解析应用层协议。

## 架构与工作原理

```
┌────────────────────── Namespace: payment ─────────────────────┐
│                                                                │
│   Pod: checkout    ◄──── 仅允许 from app=web on :8080  ────    │  Ingress 规则
│                                                                │
│   Pod: checkout    ──── 仅允许 to app=db on :5432 ───────►    │  Egress 规则
│                                                                │
└────────────────────────────────────────────────────────────────┘
                              │ CNI 实现（Calico / Cilium / Weave）
                              ▼
            iptables / eBPF 规则下发到每个节点
```

**工作流**：
1. 用户 apply NetworkPolicy 资源（networking.k8s.io/v1）。
2. **CNI 插件**（必须支持 NetworkPolicy，如 Calico、Cilium、Weave；原生 Flannel 不支持）监听这些资源，将其翻译为底层规则（iptables 或 eBPF）下发到所有节点。
3. 规则作用域由 `podSelector` 限定（哪些 Pod 受约束），入口/出口由 `ingress` / `egress` 字段定义。
4. **默认拒绝语义**：某 Pod 一旦被任意一条策略选中，则只有策略显式允许的流量放行，其余全部拒绝。

**两种默认模式**：
- 默认全部允许（开箱即用）—— 直到出现 NetworkPolicy 才限制。
- 通过"默认拒绝"策略一次性把某 Namespace 全部 Pod 设为"全拒"，再逐条放行，是推荐的安全姿态。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `podSelector` | 策略作用于哪些 Pod（空 = Namespace 全部） |
| `policyTypes` | `[Ingress, Egress]`，声明该策略管的流量方向 |
| `ingress[].from` | 允许哪些源（podSelector / namespaceSelector / ipBlock） |
| `ingress[].ports` | 允许的目标端口与协议 |
| `egress[].to` | 允许去哪些目标 |
| `egress[].ports` | 允许的目标端口 |
| 命名空间隔离 | `namespaceSelector` 跨命名空间匹配 |

## 配置示例

```yaml
---
# 1. 默认拒绝：Namespace 内全部 Pod 进出全拒
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: payment
spec:
  podSelector: {}              # 所有 Pod
  policyTypes: [Ingress, Egress]
---
# 2. 只允许 web → checkout:8080
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-web-to-checkout
  namespace: payment
spec:
  podSelector:
    matchLabels:
      app: checkout
  policyTypes: [Ingress]
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: web
    ports:
    - protocol: TCP
      port: 8080
---
# 3. checkout → db:5432 + DNS（kube-system coredns）出站放行
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: checkout-egress
  namespace: payment
spec:
  podSelector:
    matchLabels:
      app: checkout
  policyTypes: [Egress]
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: db
    ports:
    - protocol: TCP
      port: 5432
  - to:                       # 允许 DNS
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
```

## 常用操作与命令

```bash
# 查看策略
kubectl get networkpolicy -n payment
kubectl describe networkpolicy default-deny-all -n payment

# 没有原生"测试连通性"命令，常用临时 Pod 探测
kubectl run probe --image=nicolaka/netshoot -it --rm --restart=Never -- \
  curl -m 2 http://checkout.payment:8080

# 查看策略影响范围（Cilium）
cilium policy get            # 在启用 hubble 时可用
hubble observe --from-pod payment/web --to-pod payment/checkout

# Calico 全局默认拒绝
kubectl apply -f - <<EOF
apiVersion: crd.projectcalico.org/v1
kind: GlobalNetworkPolicy
metadata: {name: default-deny}
spec:
  selector: all()
  types: [Ingress, Egress]
EOF
```

## 最佳实践

1. **白名单模型**：先"默认拒绝全部"，再按业务逐条放行，最小权限。
2. **不要忘记 DNS**：一旦启用 Egress 限制，kube-dns/CoreDNS 的 53 端口也要放行，否则解析失败。
3. **分 Namespace 隔离**：为 Namespace 打 `kubernetes.io/metadata.name` 标签，结合 namespaceSelector 做租户隔离。
4. **使用 Cilium/Calico 增强策略**：原生 NetworkPolicy 不支持 L7、日志和 deny 规则，Cilium eBPF 可补齐。
5. **CI 校验**：在 GitOps 流水线用 `kube-linter` 或 `kyverno` 校验每个 Namespace 都有"默认拒绝"。
6. **保留运维通道**：放行来自监控/日志/堡垒 Namespace 的流量，避免一刀切导致可观测性丢失。

## 常见陷阱

- **策略不生效**：CNI 不支持（如 Flannel 裸装）或装了多 CNI 互相覆盖，需确认实际生效的插件。
- **误用 namespaceSelector**：1.21 前需手动给 Namespace 打标签；1.21+ 自动有 `kubernetes.io/metadata.name`。
- **egress 限制后 DNS 断**：只放行了业务端口，忘了 UDP/TCP 53，导致连 Service 名都解析不出。
- **ipBlock 排除节点本地段**：放行 `0.0.0.0/0` 时记得 `except: [169.254.0.0/16, ...]`。
- **策略被空 podSelector 误伤**：`podSelector: {}` 表示选中所有 Pod，撰写时要小心。
- **CNI 升级策略丢失**：部分 CNI 的扩展 CRD（GlobalNetworkPolicy）在迁移到原生 NetworkPolicy 时需同步。

## 源码实现分析

### Cilium NetworkPolicy 实现（eBPF）

```go
// github.com/cilium/cilium/pkg/policy/repository.go
// Cilium 策略编译流程
func (repo *PolicyRepository) regeneratePolicy(ctx *policyCtx) {
    // 1. 解析 NetworkPolicy → 内部 EndpointPolicy
    for _, np := range repo.networkPolicies {
        // 将 podSelector/namespaceSelector 转换为 endpoint 集合
        endpoints := resolveSelector(np.Spec.PodSelector, np.Namespace)
        
        // 2. 编译为 eBPF map 条目
        for _, rule := range np.Spec.Ingress {
            for _, from := range rule.From {
                // 生成 BPF_MAP_TYPE_HASH 条目
                // key: {src_ip, src_port, protocol}
                // value: {action: ALLOW/DENY}
                policyMap.Update(key, allowEntry)
            }
        }
    }
    // 3. 通过 BPF syscall 加载到内核
    // 每个 endpoint 有独立的 policy map
}
```

```
┌─────────────────────────────────────────────────────────┐
│     NetworkPolicy 实现架构对比                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Cilium (eBPF):                                        │
│    Pod veth ─▶ tc hook ─▶ eBPF policy map ─▶ ALLOW/DROP│
│    优势: 内核态执行，无 iptables 规则膨胀              │
│                                                         │
│  Calico (iptables/nftables):                           │
│    Pod veth ─▶ iptables chain ─▶ per-policy rule       │
│    优势: 无需 eBPF 支持，兼容老内核                  │
│                                                         │
│  默认行为: 无策略 = 全放行 (allow-all)              │
│  一旦有策略选中 Pod → 未明确允许 = 拒绝 (deny-all)  │
└─────────────────────────────────────────────────────────┘
```

### 生产配置：微服务网络隔离

```yaml
# 默认拒绝所有入站 + 允许特定服务访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-deny-all-allow-frontend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes: [Ingress]
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 8080
---
# 允许 DNS 解析（常被遗漏！）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}  # 所有 Pod
  policyTypes: [Egress]
  egress:
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

### 生产运维：NetworkPolicy 故障诊断

```bash
# 🟢 检查策略是否被 CNI 支持
kubectl get networkpolicy -A
kubectl get pods -n kube-system -l k8s-app=cilium  # 确认 CNI 运行

# 🟢 Cilium 策略调试
kubectl exec -n kube-system <cilium-pod> -- cilium policy trace --src-labels app=frontend --dst-labels app=api --dport 8080

# 🟢 Calico 策略调试
calicoctl get networkpolicy -A -o wide
calicoctl node status

# 🟡 临时禁用策略排查连通性问题
kubectl delete networkpolicy <name> -n <ns>
# 🔴 生产环境删除策略前必须确认影响范围
```

## 面试要点

1. **NetworkPolicy 的默认行为是什么？**
   - 无任何策略时：所有 Pod 间流量全放行（allow-all）
   - 一旦有策略选中某 Pod：该 Pod 变为默认拒绝，只允许策略明确放行的流量
   - 这是白名单模型，生产建议每个 namespace 都有 default-deny 策略

2. **哪些 CNI 支持 NetworkPolicy？**
   - 支持：Cilium（eBPF）、Calico（iptables/eBPF）、Weave、Antrea
   - 不支持：Flannel（裸装）、AWS VPC CNI（需 Calico 插件）
   - 判断方法：创建策略后测试连通性，或查看 CNI 文档

3. **为什么限制 Egress 后 DNS 会断？**
   - CoreDNS 运行在 kube-system namespace 的 Pod 中
   - 限制 Egress 后必须显式放行 UDP/TCP 53 到 kube-system
   - 这是生产中最常见的 NetworkPolicy 故障

4. **Cilium 和 Calico 实现 NetworkPolicy 的区别？**
   - Cilium：eBPF map 内核态匹配，O(1) 复杂度，无规则膨胀
   - Calico：iptables chain 线性匹配，策略多时性能下降
   - Cilium 额外支持 L7 策略（HTTP method/path）和 FQDN 策略

## 相关概念

- [[22-概念/01-核心架构/kubernetes.md|Kubernetes]]
- [[22-概念/03-网络/service.md|Service]]
- [[22-概念/02-工作负载/pods.md|Pod]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF 网络]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub

---
title: NetworkPolicy
summary: NetworkPolicy 是 Kubernetes 中用于控制 Pod 级别网络流量的安全资源。它通过标签选择器（Label Selector）定义一组
  Pod，并显式声明哪些流量可以进入（ingress）或离开（egress）这组 Pod。NetworkPolicy 是构建零信任网络的基础构件。
category: concepts
tags:
- core-concept
- domain-03
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# NetworkPolicy

NetworkPolicy 是 Kubernetes 中用于控制 Pod 级别网络流量的安全资源。它通过标签选择器（Label Selector）定义一组 Pod，并显式声明哪些流量可以进入（ingress）或离开（egress）这组 Pod。NetworkPolicy 是构建零信任网络的基础构件。

## 资源模型

一个 NetworkPolicy 包含以下核心字段：

- **`podSelector`**：选择当前策略作用于哪些 Pod。空选择器（`{}`）表示作用于当前命名空间下的所有 Pod。这是策略的核心锚点，决定了哪些 Pod 的流量被管控。
- **`policyTypes`**：声明策略生效的方向，可选 `Ingress`、`Egress` 或两者兼具。若未指定且同时存在 `ingress` 和 `egress` 字段，则二者均生效。
- **`ingress`**：定义允许进入选定 Pod 的流量规则，可基于来源 Pod 标签（`podSelector`）、命名空间标签（`namespaceSelector`）或 IP 段（`ipBlock`）进行匹配。
- **`egress`**：定义允许从选定 Pod 发出的流量规则，可基于目标 Pod 标签、命名空间标签或 IP 段进行匹配。

每条规则内部为**逻辑或**关系，规则之间也为**逻辑或**关系。只有至少匹配一条规则的流量才被允许通过。

典型 deny-all 策略示例：

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
spec:
  podSelector: {}
  policyTypes:
  - Ingress
```

## 默认拒绝与显式允许

Kubernetes 遵循**默认放行**原则：没有 NetworkPolicy 的 Pod 可以被任何来源访问。要实现零信任网络，通常需要先创建一条 **deny-all** 策略：

- deny-all ingress：将 `podSelector` 设为空（`{}`），`policyTypes` 设为 `["Ingress"]`，不填写任何 `ingress` 规则，即拒绝所有入站流量。
- 随后创建具体的允许策略（allow-list），逐条开放必要的端口与来源。

这种"先全拒、再放行"的模式是生产环境使用 NetworkPolicy 的标准实践。

## 支持的 CNI

NetworkPolicy 的生效完全依赖 CNI 插件的实现。若 CNI 不支持，策略存在但不会产生任何实际效果：

| CNI | NetworkPolicy 支持 | 备注 |
|---|---|---|
| Calico | ✅ 完整支持 | 扩展支持全局网络策略（GlobalNetworkPolicy）与七层规则 |
| Cilium | ✅ 完整支持 | 基于 eBPF 实现，性能高，支持 L3-L7 细粒度策略 |
| Weave | ✅ 支持 | 功能相对基础，社区活跃度下降 |
| Flannel | ❌ 不支持 | 仅提供网络连通性，无策略能力 |
| Terway（阿里云） | ✅ 支持 | 依赖底层 VPC 路由与 eBPF 实现策略 |
| Kube-router | ✅ 支持 | 基于 IPVS 与 iptables 实现 |

若集群使用 Flannel 且需要网络隔离，必须迁移至支持 NetworkPolicy 的 CNI，否则所有策略均为无效声明。

## 远程顾问诊断要点

在远程顾问模式下，NetworkPolicy 问题的排查关键在于确认策略声明与底层实现之间的匹配，以及规则语义的正确性。由于网络策略的拒绝通常为静默丢弃（无 RST 包），应用层往往只能看到"连接超时"，增加了定位难度：

1. **CNI 不支持 NetworkPolicy**：首先确认集群使用的 CNI 是否支持策略。如果是 Flannel，所有 NetworkPolicy 都不会生效，表现为策略存在但流量依然畅通。指导用户执行 `kubectl get pods -n kube-system` 查看 CNI 组件。
2. **namespaceSelector 误用**：`ingress.from.namespaceSelector` 选择的是**来源命名空间的标签**，而非目标命名空间。用户常误以为填写目标命名空间名称即可放行跨命名空间流量，导致访问被拒绝。应核对来源命名空间的实际标签。
3. **端口与协议不匹配**：NetworkPolicy 中的端口必须明确指定协议（TCP/UDP/SCTP），默认 TCP。若应用使用 UDP（如 DNS、QUIC）但未在策略中声明，流量将被静默丢弃。注意 WebSocket、gRPC 等基于 TCP 的应用不受此影响。
4. **策略方向混淆**：`policyTypes` 未包含 `Egress` 时，出站流量不受任何限制。若用户反馈"无法访问外部服务"，需确认是否定义了 `egress` 规则并将 `Egress` 加入 `policyTypes`。
5. **规则顺序与叠加**：NetworkPolicy 为白名单机制，多条策略叠加时取并集。若 deny-all 策略与 allow 策略同时存在但标签选择器范围不一致，可能导致预期外的放行或拒绝。
6. **DNS 出站被阻断**：若 deny-all egress 策略生效后 Pod 无法解析域名，需显式放行 CoreDNS 的 53/UDP 与 53/TCP 流量。这是应用 deny-all egress 后最常见的遗漏点。

更多排查细节可参考 [[故障诊断/高级排障/03-networking/04-networkpolicy-troubleshooting.md|network-policy-troubleshooting]] 与 [[实体/cilium.md|cilium-network-policy]]。远程顾问应指导用户先确认 CNI 类型，再逐条核对策略规则，避免在错误的假设上消耗排查时间。

## 源码实现分析

### Calico Felix 策略下发

Calico 的 Felix 组件负责将 NetworkPolicy 转换为 iptables/eBPF 规则：

```go
// calico/felix/policy.go 简化逻辑
func (d *Dataplane) ApplyPolicy(profileID string, rules []Rule) {
    // 1. 将 K8s NetworkPolicy 转换为 Calico Profile
    // 2. 生成 iptables chain: cali-pro-<profileID>
    // 3. 每条规则映射为一条 iptables 匹配+动作
    for _, rule := range rules {
        match := iptables.Match{}
        if rule.SrcSelector != "" {
            match.SetSrcIPSet(rule.SrcSelector)  // ipset 匹配源 Pod
        }
        if rule.DstPorts != nil {
            match.SetDstPorts(rule.DstPorts)     // 端口匹配
        }
        chain.AppendRule(match, rule.Action)     // ACCEPT/DROP/LOG
    }
}
```

### Cilium eBPF 策略执行

Cilium 使用 eBPF 在内核层面执行策略，避免 iptables 性能退化：

```
┌─────────────────────────────────────────────────┐
│  Pod A (endpoint 1234)                          │
│  ┌───────────────────────────────────────────┐  │
│  │ cilium_from_container (eBPF prog)         │  │
│  │  → PolicyMap lookup: dst=Pod B, port=80   │  │
│  │  → Identity-based: allow if identity=5678 │  │
│  │  → Drop + metric increment if denied      │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
         │ (eBPF tail call)
         ▼
┌─────────────────────────────────────────────────┐
│  Pod B (endpoint 5678)                          │
│  ┌───────────────────────────────────────────┐  │
│  │ cilium_to_container (eBPF prog)           │  │
│  │  → Verify ingress policy for identity 1234│  │
│  │  → Accept → deliver to Pod B veth         │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：微服务间最小权限通信

```yaml
# 仅允许 frontend 访问 backend:8080
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-ingress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

### 场景二：限制 Pod 出站（防止数据外泄）

```yaml
# 仅允许访问集群内 DNS 和指定外部 API
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restricted-egress
spec:
  podSelector:
    matchLabels:
      app: data-processor
  policyTypes:
  - Egress
  egress:
  - to: []           # 允许集群内所有 Pod
    ports:
    - protocol: UDP
      port: 53       # DNS
    - protocol: TCP
      port: 53
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8    # 内网 API 网关
    ports:
    - protocol: TCP
      port: 443
```

### 场景三：命名空间隔离（多租户）

```yaml
# 租户 A 的 Pod 只能与同命名空间 Pod 通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
  - ports:           # 放行 DNS
    - protocol: UDP
      port: 53
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 创建 NetworkPolicy 即生效 | 必须 CNI 支持（Calico/Cilium），Flannel 下策略无效 |
| deny-all 后 Pod 仍能访问外网 | 需同时声明 Egress policyType 并配置 egress 规则 |
| 多条策略有优先级顺序 | NetworkPolicy 无优先级，多条策略取并集（白名单叠加） |
| namespaceSelector 用名称匹配 | 必须用标签匹配，需提前给 namespace 打标签 |
| 策略拒绝会返回错误 | 拒绝为静默丢包（无 RST），应用层只看到连接超时 |
| ipBlock 可以匹配 Pod IP | ipBlock 匹配的是节点/外部网段，Pod 间应用 podSelector |

## 面试要点

1. **NetworkPolicy 如何生效？** — NetworkPolicy 本身只是 API 对象，实际执行依赖 CNI 插件。Calico 通过 Felix 将策略转为 iptables/ipset 规则；Cilium 编译为 eBPF 程序挂载到 veth/TC 层面，性能更优。

2. **如何实现零信任网络？** — 先创建 deny-all（Ingress+Egress），再逐条添加 allow 策略。关键注意点：必须放行 DNS（53/UDP+TCP）到 kube-dns/CoreDNS Pod，否则所有域名解析失败。

3. **NetworkPolicy 与 Service Mesh 的关系？** — NetworkPolicy 工作在 L3/L4（IP+端口），Service Mesh（Istio/Linkerd）提供 L7 策略（HTTP method/path/header）。生产环境建议两者结合：NetworkPolicy 做粗粒度隔离，Mesh 做细粒度授权。

4. **大规模集群 NetworkPolicy 性能问题？** — iptables 规则数随策略线性增长（O(n)匹配），1000+策略时延迟显著。Cilium eBPF 使用 PolicyMap（hash lookup O(1)）避免此问题；Calico eBPF 模式同理。

## 相关概念

- [[cni-networking-model]] — CNI 网络模型与插件对比
- [[security-defense-depth]] — 云原生纵深防御体系
- [[cloud-native-defense-in-depth]] — 云原生安全纵深防御
- [[multi-tenancy-isolation]] — 多租户隔离机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->

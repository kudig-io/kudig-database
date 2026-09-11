---
title: K8s 网络快问快答与面试模拟对话
summary: 56 道 Kubernetes 网络口述自测题（Service/Ingress/CNI/DNS/NetworkPolicy/排障）、数据面与 conntrack 及 12 道高级课题（CNI/DNS/策略/Gateway API/eBPF/mTLS/Egress/多集群/双栈）高难度问答，及四轮面试官追问模拟脚本，配套理论面经使用。
category: interview
tags:
- kubernetes
- k8s
- networking
- interview
- quick-qa
tier: core
created: '2026-08-31'
last_updated: 2026-09
difficulty: intermediate
audience:
- 后端工程师
- SRE
- 平台工程师
- 云原生面试准备者
estimated_read_time: 18min
intent_queries:
- Kubernetes 网络快问快答
- K8s Service Ingress 口述自测
- K8s 网络面试模拟对话
trigger_keywords:
- 快问快答
- 面试模拟
- ClusterIP
- NodePort
- Ingress
- kube-proxy
- conntrack
prerequisites:
- kubernetes-service-ingress-interview
---

# K8s 网络快问快答与面试模拟对话

理论体系见 [[05-网络/01-K8s网络核心/57-kubernetes-service-ingress-interview.md|Kubernetes Service 与 Ingress 网络面经]]。建议先读理论，再用本文口述自测；两者配合使用，先理解再刷题。

---

## K8s 网络快问快答

建议先遮住答案口述，再对照关键词补全。

### 1. Service 解决什么问题？

Service 为一组动态变化的 Pod 提供稳定的访问入口。Pod 重建、扩缩容时 IP 会变化，但 Service 名称和 ClusterIP 通常保持稳定。

### 2. ClusterIP 是什么？

ClusterIP 是 Service 的集群内虚拟 IP。它通常不属于某块真实网卡，访问它时，节点内核根据 kube-proxy 写入的规则把流量 DNAT 到后端 Pod。

### 3. NodePort 是什么？

NodePort 在 ClusterIP 基础上，在每个 Node 开放一个端口。外部客户端可通过 `NodeIP:NodePort` 访问 Service 后端。

### 4. LoadBalancer 是什么？

LoadBalancer 类型通常让云厂商创建外部负载均衡器，并把外部流量转发到集群节点。传统实现中通常还会分配 NodePort。

### 5. 三种 Service 类型是什么关系？

传统实现可记为：

```text
LoadBalancer
  └── NodePort
        └── ClusterIP
              └── Pod IP
```

### 6. Service 如何找到 Pod？

Service 通过 selector 匹配 Pod label，EndpointSlice Controller 再生成对应的后端地址列表。

### 7. EndpointSlice 是什么？

EndpointSlice 保存 Service 后端的 Pod IP、端口和就绪状态，是传统 Endpoints API 的可扩展替代方案。

### 8. kube-proxy 是流量代理进程吗？

通常不是逐包代理。它负责监听 Service 和 EndpointSlice 变化并维护 iptables/IPVS 规则，真正的数据包转发由 Linux 内核完成。

### 9. iptables 模式如何转发 Service 流量？

请求命中 ClusterIP 和 Service port 后，iptables 规则选择一个后端，并把目的地址 DNAT 为 `PodIP:targetPort`。

### 10. IPVS 相比 iptables 有什么优势？

IPVS 使用内核负载均衡框架和哈希查找，支持更多调度算法，在大量 Service 场景下通常比庞大的 iptables 规则链更高效。

### 11. `port`、`targetPort`、`nodePort` 有什么区别？

- `port`：Service 暴露的端口；
- `targetPort`：后端 Pod 实际监听的端口；
- `nodePort`：每个 Node 对外开放的端口。

```text
ClusterIP:80 / NodeIP:30080 → PodIP:8080
```

### 12. Ingress 是 Service 类型吗？

不是。Ingress 是声明 HTTP/HTTPS 七层路由规则的 Kubernetes API 资源。

### 13. Ingress 和 Ingress Controller 有什么区别？

Ingress 是规则，Ingress Controller 是执行规则的控制器和反向代理。没有匹配的 Controller，Ingress 资源不会自动处理流量。

### 14. Ingress Controller 如何暴露？

它本身通常是一组 Pod，可通过 LoadBalancer、NodePort、hostNetwork、MetalLB 或外部硬件负载均衡器暴露。

### 15. Ingress 的完整流量路径是什么？

```text
Client → DNS → LoadBalancer/NodePort
       → Ingress Controller
       → 匹配 Host/Path
       → Service/EndpointSlice
       → Pod
```

### 16. Ingress 为什么还要引用 Service？

Pod 生命周期短且 IP 不稳定。Service 提供稳定后端抽象，EndpointSlice 则维护实时 Pod 地址。部分 Controller 最终会直接把请求发给 EndpointSlice 中的 Pod IP。

### 17. DNS 和 Service 有什么关系？

CoreDNS 把 Service 名称解析为 ClusterIP。客户端访问 ClusterIP 后，再由 Service 数据面规则把请求转发到后端 Pod。

### 18. Service 的完整 DNS 名是什么？

```text
<service>.<namespace>.svc.cluster.local
```

同一 Namespace 内通常可以只使用 Service 名称。

### 19. Headless Service 是什么？

Headless Service 设置 `clusterIP: None`，不提供 ClusterIP。DNS 通常直接返回后端地址，常用于 StatefulSet 和需要自主服务发现的应用。

### 20. ExternalName Service 是什么？

ExternalName 把集群内 Service 名称映射为外部 DNS 名称，本质上依赖 DNS CNAME，不创建 ClusterIP，也不进行数据面负载均衡。

### 21. Kubernetes 对 Pod 网络有什么基本要求？

每个 Pod 拥有独立 IP；在标准网络模型中，Pod 之间应能直接通信，不需要应用感知的 NAT。具体连通由 CNI 实现。

### 22. CNI 负责什么？

CNI 插件负责创建 Pod 网络接口、分配 IP、配置路由或隧道，并实现跨节点 Pod 网络。常见实现包括 Calico、Cilium、Flannel 和 Terway。

### 23. Service 和 CNI 如何分工？

```text
Service/kube-proxy：选择目标 Pod，并实现服务转发
CNI：负责把数据包真正送到目标 Pod
```

### 24. NetworkPolicy 解决什么问题？

NetworkPolicy 声明允许哪些入站和出站通信，相当于面向 Pod 的网络访问控制策略。

### 25. 创建 NetworkPolicy 就一定会生效吗？

不一定。底层 CNI 必须实现 NetworkPolicy，否则资源可能被 API Server 接受，但不会真正执行策略。

### 26. 没有 NetworkPolicy 时默认允许还是拒绝？

默认通常是允许。Pod 被相应方向的 NetworkPolicy 选中后，该方向才进入白名单式隔离状态。

### 27. `externalTrafficPolicy: Local` 有什么作用？

它只把外部流量转发给入口节点上的本地 Endpoint，通常可以保留客户端源 IP；代价是流量分布可能不均，没有本地 Endpoint 的节点也无法承接请求。

### 28. Service 如何实现会话保持？

可设置：

```yaml
spec:
  sessionAffinity: ClientIP
```

同一客户端 IP 会在一定时间内尽量命中同一后端。

### 29. Ingress 返回 404 怎么排查？

优先检查 Host、Path、`pathType`、IngressClass 是否匹配，以及请求是否携带正确 Host 头。

```bash
kubectl describe ingress <name>
curl -H 'Host: web.example.com' http://<ingress-address>/
```

### 30. Ingress 返回 502/503 怎么排查？

检查 Service 是否有可用 EndpointSlice、Pod 是否 Ready、端口是否匹配，以及 Controller 到后端的网络是否正常。

### 31. ClusterIP 不通时按什么顺序排查？

```text
Pod Running/Ready
  → Service selector
  → EndpointSlice
  → port/targetPort
  → NetworkPolicy
  → kube-proxy/数据面
```

### 32. NodePort 不通时检查什么？

检查 Node IP 可达性、NodePort、安全组和主机防火墙、EndpointSlice、`externalTrafficPolicy` 以及 kube-proxy 规则。

### 33. 为什么 Pod 能通过 Service 名访问服务？

Pod 的 DNS 查询由 CoreDNS 解析，Service 名被解析为 ClusterIP，再由节点 Service 数据面转发到后端 Pod。

### 34. kube-proxy 挂掉后 Service 会立刻全部中断吗？

不一定。已有内核规则可能继续工作，但新的 Service、Endpoint 变化和扩缩容无法及时同步，转发状态会逐渐过期。

### 35. Service selector 写错有什么现象？

EndpointSlice 没有目标后端，Service 请求失败，Ingress 访问后端时常表现为 503。

```bash
kubectl get endpointslice -l kubernetes.io/service-name=<svc>
kubectl get pod --show-labels
```

### 36. readinessProbe 和 Service 有什么关系？

Pod 即使处于 Running，只要未 Ready，通常也不会作为可服务 Endpoint 接收正常 Service 流量。

### 37. `hostNetwork: true` 有什么影响？

Pod 与宿主机共享网络命名空间和 Node IP，网络路径更直接，但会降低隔离性，并带来端口冲突风险。

### 38. hostPort 和 NodePort 有什么区别？

hostPort 把某个 Pod 的端口绑定到它所在节点；NodePort 属于 Service，在每个节点开放端口，并可负载均衡到多个后端 Pod。

### 39. Service 能根据 HTTP Path 路由吗？

普通 Service 是四层抽象，主要依据 IP 和端口，不能根据 Host 或 Path 路由。七层路由应使用 Ingress、Gateway API 或服务网格。

### 40. Gateway API 和 Ingress 有什么关系？

Gateway API 是更具扩展性和角色分工能力的新一代流量管理 API，可表达更丰富的四层和七层路由；Ingress 模型更简单，扩展通常依赖注解。

### 41. 如何完整描述外部请求访问 Pod 的过程？

外部用户通过 DNS 访问负载均衡器，流量进入 Ingress Controller 的 Service 和 Controller Pod。Controller 根据 Ingress 的 Host/Path 规则选择后端 Service，再根据 EndpointSlice 把请求发送到某个 Pod。kube-proxy 负责维护 Service 数据面规则，CNI 负责实际 Pod 网络连通。

### 42. 如何一句话说明 Service、Ingress、Ingress Controller 的关系？

Service 是稳定的四层服务入口，Ingress 是七层路由规则，Ingress Controller 是接收外部请求并执行这些规则的代理。

---

## 进阶快问快答：数据面与 conntrack（高难度）

高难度追问，答案为参考口径。先遮住答案口述，再对照补全；答不全属正常，重点是补齐关键词。

### 43. 发往 ClusterIP 的数据包，在 iptables 模式下究竟在哪台机器的哪个链上被 DNAT 成 Pod IP？kube-proxy 的规则数量与 Service/Endpoint 数量是什么关系，为什么大规模集群有性能问题而 IPVS 能缓解？

**DNAT 发生的位置**：

- 发生在**发起访问的那台节点**（客户端 Pod 所在的 Node），而不是某个"服务端"——ClusterIP 本来就没有实体。
- 链路：数据包从 Pod 发出，经 veth pair 进入 Node 的 root netns；kube-proxy 在 nat 表挂了入口链 KUBE-SERVICES，流量在 nat 表 PREROUTING 链（本机进程发起则走 OUTPUT 链）进入 KUBE-SERVICES；匹配目的地址 = 某 ClusterIP + 端口后，跳转到 KUBE-SVC-XXX 链，该链通过 statistic 模块（random 概率）做负载均衡，最终以 DNAT 把目的地址改写为选中的 PodIP:Port。后续经 POSTROUTING 时若目标是跨节点的 Pod，再做 SNAT（masquerade）；回程包由 conntrack 自动反向转换。

```text
Pod → veth → root netns
  → nat PREROUTING（本机进程发起则 OUTPUT）
  → KUBE-SERVICES（匹配 ClusterIP+port）
  → KUBE-SVC-XXX（statistic random 概率选后端）
  → KUBE-SEP-XXX（DNAT → PodIP:targetPort）
  → POSTROUTING（跨节点时 SNAT/masquerade）
```

**规则数量与性能问题**：

- iptables 模式下规则数是 O(Service 数 × 平均 Endpoint 数) 量级：每个 Service 一条 KUBE-SVC-* 链，每个 Endpoint 一条 KUBE-SEP-* 链及对应 DNAT 规则。几千个 Service 就是几万条规则。
- 数据面：iptables 匹配是线性遍历，包要一条条比对规则，转发路径随规则数变长（kube-proxy 用 probability 跳转做了折半优化，本质仍是 O(n)）。
- 控制面：Endpoint 变化时 kube-proxy 全量/大批量刷新 nat 表，更新几万条规则的耗时会达到秒级，期间规则处于不一致状态。
- IPVS 的缓解方式：IPVS 是内核里专门为负载均衡设计的模块，Service 查找基于哈希表，复杂度 O(1)，与规则数量无关；且更新是增量的（逐条 add/del server），不需要整表重写。代价是排错工具从 iptables-save 换成 ipvsadm，且 IPVS 仍依赖少量 iptables/nftables 规则做 SNAT 和过滤。

思考题：为什么"IPVS 快"主要在大集群才体现？——几十个 Service 的小集群两种模式几乎无感。

### 44. conntrack 表满（`nf_conntrack: table full, dropping packet`）时丢的是什么包，为什么表现为新建连接失败但已有连接正常？UDP 服务（如 CoreDNS）高并发下的 conntrack 竞态发生在哪一步，常见的两种缓解手段是什么？

**表满丢什么包**：

- 丢的是**需要新建 conntrack 表项的包**——查表未命中、要以 NEW 状态插入新表项的包，最典型是新建连接的第一个包（TCP SYN、UDP 首包、DNS 查询）。内核插入失败时直接丢包并打日志。
- 已有连接的包只做**查表匹配**（命中后取 NAT 映射、刷新超时），不产生新表项。根因是**插入会增长表、查找不会**，所以"新建失败、存量正常"。
- K8s 场景下 conntrack 还存着 DNAT 前后的映射，kube-proxy 反向回包靠它 un-DNAT，所以表满直接影响 Service 新建流。高 churn 的短连接（DNS、不复用连接的 HTTP）最容易打满表，排查用 `conntrack -S` 对比 count 与 `nf_conntrack_max`。

**UDP 竞态（DNS 5 秒延迟）**：

- 触发前提：musl libc（Alpine 镜像）并行发送 A/AAAA 查询时**复用同一个源端口**——两个逻辑上不同的查询共用同一个五元组；重传同理。UDP 无握手，conntrack 无法区分"新查询"和"同流量的下一个包"，只能按五元组视为同一条流。
- 竞态位置：在节点 conntrack 处理**同五元组的应答包与下一个请求包**时。第一个应答到达会把流表项从 UNREPLIED 迁移到 REPLIED 并刷新超时；此时下一个同五元组的新请求（或第二个应答）几乎同时到达，两个包的处理与表项状态迁移发生交错，不幸时序下后到的包不再匹配表项期望的方向状态，被判为 **INVALID**，随后被 netfilter 里 drop INVALID 的规则丢弃。
- 表现：客户端 resolver 收不到应答又不能立刻判定失败，只能等完整超时——musl 并行查询策略总超时约 5s，这就是经典的"DNS 5 秒延迟"。

**两种常见缓解手段**：

1. 改客户端解析行为，避免五元组复用：`/etc/resolv.conf` 加 `single-request-reopen`（A/AAAA 串行发送且每次重开 socket，换新源端口 = 新五元组），或 `single-request` / `use-vc`（强制 TCP）。注意 musl **不读这些选项**，Alpine 镜像需换 glibc 或走下一条。
2. NodeLocal DNSCache：每节点起缓存 DaemonSet（挂 dummy 网卡 169.254.20.10），Pod 直连本机缓存，命中不出节点，大幅减少经过 kube-proxy DNAT + conntrack 的短命 UDP 流。

**边界提醒**：调大 `nf_conntrack_max` 只能缓解表满，解决不了竞态；raw 表 `--notrack` 让 DNS 流量绕过 conntrack 更彻底，但会连带绕过 DNAT、必须直指 DNS endpoint，一般不推荐。另外 kube-proxy 对 UDP endpoint 摘除会主动删 conntrack 表项，那是另一个"陈旧 DNAT 指向已死 CoreDNS Pod"的问题，不要与这个竞态混淆。

---

## 高级课题快问快答：CNI/DNS/策略/网关/加密/多集群/双栈（高难度）

本节 12 题（第 45-56 题）覆盖完整高级课题。每题都是一次深入的机会：先口述作答，再对照参考答案补全关键词，最后沿"深挖"链接回语料源文件精读对应章节。

### 话题覆盖清单

| 题号 | 话题域 | 核心考点 |
|------|--------|---------|
| 45 | CNI / Flannel | VXLAN 与 host-gw 转发路径、封装开销、Directrouting |
| 46 | CNI / Terway | 四种网络模式、ENIIP 容量计算、VPC 直通本质 |
| 47 | Service 进阶 | externalTrafficPolicy、sessionAffinity、Topology Aware Hints |
| 48 | Ingress 生产 | 413/504/502 三大故障、平滑发布、canary、配置基线 |
| 49 | DNS / CoreDNS | ndots:5 查询放大、NodeLocal DNSCache、Corefile 顺序 |
| 50 | NetworkPolicy | 白名单模型、default-deny、AND/OR 语法、DNS 例外 |
| 51 | Gateway API | 角色分工、ReferenceGrant、GAMMA |
| 52 | eBPF / Cilium | 替代 kube-proxy 的原理、L3-L7 策略、收益与代价 |
| 53 | 加密 / mTLS | WireGuard vs Istio mTLS、PeerAuthentication、证书体系 |
| 54 | Egress 管理 | 分层控制、固定出口 IP、方案选型口诀 |
| 55 | 多集群 | 方案阶梯、ServiceExport/MCS、CIDR 规划第一坑 |
| 56 | IPv6 双栈 / 性能 | ipFamilyPolicy、sysctl 前置、conntrack/MTU/内核基线 |

### 45. Flannel 的 VXLAN 与 host-gw 后端转发路径有什么本质区别？封装开销各是多少？Directrouting 混合模式解决了什么？

**VXLAN 路径**：Pod → cni0 → 路由指向 flannel.1 设备 → FDB 表学到对端 VTEP MAC → 外层封装（外层以太网 + 外层 IP + UDP 8472 + VNI）→ 对端解封装。三层网络可达即可互通，但封装带来 **50 字节开销**，MTU 必须降到 **1450**。

**host-gw 路径**：cni0 直接按宿主机路由表把包发往对端节点的物理网关，零封装，MTU 1500 满速，但要求**所有节点在同一二层网络**。

| 后端 | 封装开销 | MTU | 前提 |
|------|---------|-----|------|
| host-gw | 0 | 1500 | 同二层 |
| IPIP | 20 | 1480 | 三层可达 |
| VXLAN | 50 | 1450 | 三层可达 |
| WireGuard | 80 | 1420 | 加密需求 |

**Directrouting**：同一子网走 host-gw 纯路由，跨子网自动回落 VXLAN，兼顾性能与跨网段能力。选型：性能敏感且同二层选 host-gw；云环境跨网段选 VXLAN；要加密选 WireGuard；UDP 后端是用户态转发，性能最差，仅作兜底。

深挖：[[05-网络/01-K8s网络核心/05-flannel-complete-guide.md|Flannel 完整指南]]

### 46. Terway 有哪几种网络模式？ENIIP 模式单节点 Pod 容量怎么算？相比 Flannel 隧道方案的本质提升是什么？

**四种模式**：VPC 路由（Pod 走共享节点 ENI + VPC 路由表直通）、ENI 独占（Pod 独占一块 ENI，性能最优但单节点数量少）、ENIIP 共享（一块 ENI 挂多个辅助 IP 分给 Pod，主流默认）、ENIIP-Trunking（综合分支入口）。

**容量公式**：`（节点可挂载 ENI 数 − 1）× 单 ENI 辅助 IP 数`。以 g7.4xlarge 为例：(8−1)×20 = **140 个 Pod IP**。

**本质提升**：Terway 的 Pod IP 是**真实 VPC IP**——无封装、无 NAT；安全组可以直接绑定到 Pod IP 做安全策略；NetworkPolicy 通过 iptables 或 Cilium eBPF 实现。对比 Flannel 的 VXLAN 隧道，排障视角从"双层网络"简化为"一层 VPC 网络"。

StatefulSet 可用 `k8s.aliyun.com/pod-ip-fixed` 固定 Pod IP；GC 机制回收泄漏的 IP 防止耗尽配额。

深挖：[[05-网络/01-K8s网络核心/06-terway-advanced-guide.md|Terway 高级指南]]

### 47. externalTrafficPolicy 的 Cluster 和 Local 分别牺牲什么换取什么？sessionAffinity ClientIP 为什么不能当应用会话保持的替代品？

**Cluster**：流量可在任意节点二跳，负载均匀，但跨节点转发内核做 SNAT，**丢失真实客户端源 IP**。

**Local**：只交给本节点上的本地 Pod，保留源 IP；代价是负载可能不均，且**没有本地端点的节点会直接丢包**——必须配合 `healthCheckNodePort` 让云 LB 健康检查自动摘除这些节点。`internalTrafficPolicy` 把同样语义搬进了集群内访问。

**sessionAffinity 的三个坑**：默认 ClientIP 亲和 10800s；① 它基于五元组，Cluster 模式下 SNAT 之后所有客户端源 IP 都变成节点 IP，全体流量亲和到同一个 Pod，反而放大不均；② 后端缩容时绑定失效；③ 它解决的是"请求去哪"，应用会话（登录态）应放在粘性 Cookie 或分布式存储里，两者是不同层的问题。

**Topology Aware Hints**：EndpointSlice 写入 `hints.forZones`，客户端优先访问同 Zone 端点，降低跨 AZ 流量与延迟；某 Zone 端点不足时自动回落到全局分发。

深挖：[[05-网络/01-K8s网络核心/11-service-advanced-features.md|Service 高级特性与应用案例]]

### 48. Ingress Nginx 的三大经典故障 413、504、间歇性 502，根因与修复各是什么？canary 灰度注解的优先级顺序？

- **413 Request Entity Too Large**：`proxy-body-size` 默认 **1m**，上传场景按需调大。
- **504 Gateway Timeout**：`proxy-read-timeout` / `proxy-send-timeout` 默认 **60s**，长连接/长轮询场景按业务调整。
- **间歇性 502（no live upstreams）**：upstream keepalive 竞态——nginx 从 keepalive 连接池复用了已被后端关闭的连接。修法：让**后端的 keepalive 超时大于 nginx 侧超时**，并调 `upstream-keepalive-requests` / `upstream-keepalive-timeout`。
- **平滑发布**：Pod 加 preStop 先 sleep 数秒再优雅退出（给 endpoint 摘除留窗口），`terminationGracePeriodSeconds` 给足 300，配合 PDB 控制驱逐节奏。
- **canary 优先级**：header（含 value/正则 pattern）→ cookie → weight 按比例。
- **rewrite**：`rewrite-target` 配 `$1`/`$2` 正则捕获组实现路径重写。

**生产基线**：Controller ≥3 副本跨 AZ 反亲和、配 HPA 与 PDB；`worker-processes: auto`、`max-worker-connections: 65535`、`upstream-keepalive-connections: 500`；TLS 1.2+ 证书走 cert-manager 自动续期；关闭 `allow-snippet-annotations` 防 annotation 注入；监控 `config_last_reload_successful`、`ssl_expire_time_seconds`、5xx 率、P99 延迟。

深挖：[[05-网络/01-K8s网络核心/27-ingress-production-best-practices.md|Ingress 生产最佳实践]]

### 49. `options ndots:5` 为什么会放大 DNS 查询量？NodeLocal DNSCache 一箭双雕解决哪两个问题？Corefile 的关键顺序是什么？

**ndots:5**：域名点数少于 5 就被当作短名，先按 search 列表逐个补全——`www.example.com`（2 个点）会先查 `www.example.com.default.svc.cluster.local`、`www.example.com.svc.cluster.local`、`www.example.com.cluster.local`，乘以 A/AAAA 两个查询，**最多 6 次查询**之后才轮到 FQDN 本身。生产建议：外部域名一律写**末尾带点**的 FQDN。

**NodeLocal DNSCache**（DaemonSet，占 dummy 网卡 169.254.20.10）：① Pod 直连本机缓存，流量不再经过 kube-proxy DNAT + conntrack，**绕开第 44 题的 UDP 竞态**；② 缓存命中不出节点，**降低 CoreDNS 压力与尾延迟**。

**Corefile 关键顺序**：`cache 30` 必须放在 `kubernetes` 插件**之前**（先查缓存再权威解析）；`kubernetes cluster.local in-addr.arpa ip6.arpa` 配 `fallthrough in-addr.arpa ip6.arpa`（反向解析未命中落给下游）；最后 `forward . /etc/resolv.conf` 兜底转发。注意：把 ndots 调低能减少查询，但会让集群内短名（`svc-name`）解析失效，不能一刀切。

深挖：[[05-网络/01-K8s网络核心/14-coredns-architecture-principles.md|CoreDNS 架构与核心原理]]、[[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns.md|DNS 服务发现与 CoreDNS 调优]]

### 50. NetworkPolicy 的白名单模型怎么运作？default-deny 怎么写？AND 和 OR 语法差在哪？最容易漏配的例外是什么？

**模型**：策略之间是**叠加（additive）**关系，选中的 Pod 默认拒绝一切未被允许的流量——只讲"允许什么"，没有"禁止"动作。

**default-deny-all**：`podSelector: {}`（选中本 ns 全部 Pod）+ `policyTypes: [Ingress, Egress]`，不写 ingress/egress 规则。

**AND vs OR**：`namespaceSelector` 与 `podSelector` 写在**同一个** from/to 元素里 = AND（同 ns 且同标签）；分成**两个**独立元素 = OR（满足任一即可）。这是最高频的笔试语法陷阱。

**最易漏的例外——DNS**：一旦上了 egress 默认拒绝，必须显式放行到 kube-dns 的 **UDP 和 TCP 53**（`namespaceSelector: {}` + `podSelector: matchLabels: {k8s-app: kube-dns}`），否则全 ns 的服务发现静默失败。`ipBlock` + `except` 可排除特定子网。注意 Flannel 没有策略引擎，NetworkPolicy 需要 Calico/Cilium 等实现支持。

深挖：[[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice.md|NetworkPolicy 深度实践指南]]

### 51. Gateway API 针对Ingress 的哪三个痛点重新设计？四个核心角色分别归属谁？GAMMA 解决了什么？

**三个痛点**：① 能力扩展靠注解碎片化，各家 Controller 一套 annotation；② 只表达七层，四层要绕 TLSRoute 之外的注解；③ 没有角色分工，平台配置和业务配置混在同一个对象里。

**角色分工**：

| 资源 | 级别 | 归属 | 关键能力 |
|------|------|------|---------|
| GatewayClass | 集群 | 基础设施管理员 | controllerName（如 `istio.io/gateway-controller`） |
| Gateway | 命名空间 | 平台管理员 | listeners、TLS、allowedRoutes |
| HTTPRoute | 命名空间 | 开发者 | Host/Path/Header 路由 |
| ReferenceGrant | 命名空间 | 被引用资源所有者 | 显式授权跨 ns 引用 |

跨命名空间引用**默认拒绝**，由被引用方写 ReferenceGrant 放行——把"谁能引用我"的决定权交还给资源所有者。

**GAMMA**（v1.1 起）：HTTPRoute 的 parentRef 可以指向 **Service** 而非 Gateway，把同一套路由语义带进服务网格的东西向流量，实现南北向与东西向一份 API。

深挖：[[05-网络/01-K8s网络核心/37-gateway-api-overview.md|Gateway API配置]]

### 52. Cilium 的 eBPF 数据面在哪些层替代了 iptables/kube-proxy？换来什么收益，付出什么代价？

**替代层次**：内核 tc、XDP、socket 层直接挂载 eBPF 程序，Service 查找用**哈希表 O(1)** 替代 iptables 规则线性遍历——这正是第 43 题 iptables 规模化问题的终极解法。

**收益**：① 性能：转发路径更短，无整表刷新；② 策略粒度：L3-L7，可以直接按 HTTP path/method 写策略；③ socket-LB：本机访问本机 Service 在 socket 层直接改写，连 DNAT 都不进；④ Hubble 全链路可观测；⑤ bandwidthManager + BBR 原生限速拥塞控制。原生 WireGuard/IPsec 加密 CPU 仅 2-5%、延迟 0.1-0.5ms，无 sidecar。

**代价**：内核版本要求高（老内核升级本身是工程）；排错工具链换新——iptables-save 换成 cilium CLI 与 hubble，团队要重新建立运维肌肉记忆。

深挖：[[05-网络/01-K8s网络核心/04-cni-plugins-comparison.md|CNI 插件深度对比]]

### 53. Istio mTLS 与 Cilium WireGuard 加密怎么选？PeerAuthentication 的迁移路径？证书体系怎么运作？

**方案对照**：

| 方案 | 层级 | CPU 开销 | 延迟 |
|------|------|---------|------|
| WireGuard（Calico/Cilium） | L3 | 2-5% | ~0.1-0.5ms |
| IPsec | L3 | 5-10% | 中 |
| Istio mTLS | L7 | 5-15% | 1-5ms（sidecar） |
| Cilium L3/L4 加密 | L3/L4 | 低 | 低 |

选择逻辑：只要加密 → WireGuard；要 L7 身份与授权 → mTLS。

**PeerAuthentication**：STRICT / PERMISSIVE / DISABLE，可设在 namespace、workload 级，`portLevelMtls` 支持按端口例外（如 9090 监控端口保持明文、8080 强制 STRICT）。迁移路径：**先 PERMISSIVE 双收 → 观测明文流量归零 → 切 STRICT**。

**证书体系**：workload 证书 24h TTL，istiod 自动签发轮换（SECRET_TTL / SECRET_GRACE_PERIOD_RATIO 可调）；自定义 CA 把根证书放 istio-system 的 cacerts Secret。身份是 SPIFFE ID：`cluster.local/ns/<ns>/sa/<sa>`。零信任收口：AuthorizationPolicy + 空 `spec: {}` 的 deny-all。验证用 `istioctl proxy-config secret` 与 `openssl s_client`；指标看 `istio_tcp_sent_bytes_total` / `cilium_encrypt_packets_total`。

深挖：[[05-网络/01-K8s网络核心/19-network-encryption-mtls.md|网络加密与 mTLS]]

### 54. 出口流量为什么必须分层控制？每层解决什么问题？方案选型的口诀是什么？

**分层**：NetworkPolicy（L3/L4 准入——"能不能出"）→ Egress Gateway（统一出口 IP——"从哪个 IP 出"）/ CNI Egress IP / SNAT → 云 NAT Gateway（VPC 级固定公网出口）→ 服务网格（L7 审计——"出了什么内容"）。

| 方案 | 控制粒度 | 典型场景 |
|------|---------|---------|
| NetworkPolicy | IP/端口 | 遏制横向移动 |
| Egress Gateway | 统一出口 IP | 白名单对端防火墙 |
| CNI Egress IP/SNAT | 节点/网段级 | 云厂商原生 |
| NAT Gateway | VPC 级 | 固定公网出口 |
| 网格 Egress | URL/Header（L7） | 出站审计合规 |

**口诀**：固定出口 IP → NAT Gateway/EIP 或 mesh Egress Gateway；要 L7 审计 → 网格；要遏制失陷面 → NetworkPolicy。各层**可叠加**：Policy 决定"能不能出"，Gateway 决定"从哪个 IP 出"，网格决定"出的是否合规"。

深挖：[[05-网络/01-K8s网络核心/30-egress-traffic-management.md|Egress 流量管理]]

### 55. 多集群互联方案怎么选？跨集群服务发现的标准协议是什么？第一坑是什么？

**方案阶梯**：

| 方案 | 架构 | 复杂度 | 适用 |
|------|------|-------|------|
| Submariner | 隧道 | 中 | 混合云 |
| Cilium ClusterMesh | eBPF 隧道 | 中 | 同质集群、低延迟 |
| Istio 多集群 | 服务网格 | 高 | 服务治理 |
| Skupper | 应用层 | 低 | 简单互联 |
| VPN/专线 | 网络层 | 高 | 企业网络 |
| ACK One | 托管 | 低 | 阿里云 |

**标准协议（MCS）**：导出方集群创建 `ServiceExport`（`multicluster.x-k8s.io/v1alpha1`），对端即可用 `<svc>.<ns>.svc.clusterset.local` 解析。Cilium 走注解：`service.cilium.io/global: "true"` + `shared: "true"`，DNS 透明无感；`service.cilium.io/affinity: "local"` 实现本地优先、跨集群故障转移。

**第一坑：CIDR 重叠**——重叠后必须引入 NAT，复杂度爆炸，唯一解法是**建集群时就规划互不重叠的 Pod/Service 网段**。跨公网互联必须启用 IPSec/WireGuard。排障先查 `kubectl get serviceexport -A`，再看 `subctl show connections`；指标盯 `submariner_connections` 与 `cilium_clustermesh_remote_clusters`。

深挖：[[05-网络/01-K8s网络核心/34-multi-cluster-networking.md|多集群网络互联]]

### 56. IPv6 双栈怎么落地？跨节点 IPv6 不通按什么顺序排查？规模化之后的性能调优基线有哪些？

**Service 三策略**：`SingleStack` 仅主栈；`PreferDualStack` 尽量双栈、不支持则回落（最安全）；`RequireDualStack` 强制双栈否则报错——Headless Service 配 StatefulSet 用它保证每 Pod 的 A + AAAA 记录。

**落地三件套**：① 初始化参数双 CIDR（`podSubnet: "10.244.0.0/16,fd00:10:244::/56"`，IPv6 用 fd00::/8 ULA）；② 节点 sysctl：`disable_ipv6=0`、`net.ipv6.conf.all.forwarding=1`、`accept_ra=0`；③ CNI 矩阵：Calico/Cilium(1.12+ GA)/Terway/Antrea 完整支持，Flannel 仅部分 backend，Weave 不建议生产。

**跨节点 IPv6 不通排查顺序**：forwarding 是否开启 → CNI backend 是否支持 IPv6 → ip6tables FORWARD 链是否放行。 Pod 无 IPv6 查 CNI/sysctl/cluster-cidr，Service 无 IPv6 查 ipFamilyPolicy/service-range，AAAA 失败查 CoreDNS 与 Service ipFamilies。

**性能调优基线**：conntrack `nf_conntrack_max` 1048576 起，kube-proxy `maxPerCore: 65536, min: 524288`，利用率 >80% 告警；MTU 按封装对齐（VXLAN 1450 / WireGuard 1420），开 `tcp_mtu_probing=1`；内核 `somaxconn`/backlog 65536、`ip_local_port_range 1024-65535` 防端口耗尽（`ss -s` 观察）；压测用 iperf3（带宽）、mtr（时延丢包）、`ethtool -S`（队列丢包）。

深挖：[[05-网络/01-K8s网络核心/50-ipv6-dual-stack-production.md|IPv6 双栈生产实践]]、[[05-网络/01-K8s网络核心/36-network-performance-tuning.md|网络性能调优]]

---

## 面试模拟对话脚本

用法：自己分饰两角，先遮住"候选人"部分口述作答，再对照参考回答和点评复盘。每题按真实面试节奏控制在 1-2 分钟内，追问链尽量不跳步。

### 场景一：开场热身——Service 基础

**面试官**：先聊聊 Kubernetes 的 Service 吧，你在项目里用过哪些类型？

**候选人（参考）**：用得最多的是 ClusterIP，做服务间的内部调用，比如订单服务访问支付服务。对外暴露 HTTP 服务我们走 Ingress，由统一入口按域名和路径分流。NodePort 一般用于临时调试，或者作为自建环境下外部负载均衡器的后端；云上需要直接暴露 TCP 服务时会用 LoadBalancer。

**点评**：
- ✅ 亮点：把每种类型和真实使用场景绑定，证明是"用过"而不是"背过"。
- ⚠️ 风险：如果只报类型名词，面试官会立刻转入原理追问，要准备好场景二。
- 🎯 面试官意图：快速定位实际经验层次，决定后面问概念还是问原理。

**面试官（追问）**：那 NodePort 和 ClusterIP 是两种独立的 Service 吗？

**候选人（参考）**：不是独立的，是叠加关系。创建 NodePort 类型的 Service，它同时会有 ClusterIP；LoadBalancer 通常还会带 NodePort。可以理解成套娃：LoadBalancer 包着 NodePort，NodePort 包着 ClusterIP，ClusterIP 后面才是真实的 Pod。

**点评**：
- ✅ 亮点：主动否定"并列关系"这个常见误区，给出清晰的层次结构。
- 🎯 面试官意图：高频送分题，回答犹豫会直接给整场面试定基调。

---

### 场景二：数据面深挖——kube-proxy 与转发原理

**面试官**：你说访问 ClusterIP 会转发到 Pod，那这个 ClusterIP 到底是谁持有的？在哪个网卡上？

**候选人（参考）**：这是个最容易答错的点。其实没有任何网卡持有 ClusterIP，ping 它通常也是不通的，这是正常现象。它只存在于每个节点上 kube-proxy 写入的转发规则里。数据包到达节点内核后，命中规则被 DNAT 改写成某个后端 Pod 的 IP。真正"存在"的是这条 NAT 规则，而不是一个 IP 地址。

**点评**：
- ✅ 亮点：敢于否定直觉认知，并主动解释"ping 不通是正常的"，这种表述非常加分。
- 🎯 面试官意图：鉴别候选人是背了定义，还是真的理解 Service 是内核 NAT 规则这一本质。

**面试官（追问）**：那 kube-proxy 在数据路径上吗？每个包都经过它吗？

**候选人（参考）**：不在。kube-proxy 是控制面角色，它 watch Service 和 EndpointSlice 的变化，把规则写进本节点的 iptables 或 IPVS，然后就退场了。之后每一个包的转发都是 Linux 内核在数据面完成的。kube-proxy 挂了，已有规则短期内甚至还能继续工作，只是新的 Service 和 Endpoint 变化无法同步。

**点评**：
- ✅ 亮点：区分了控制面（写规则）和数据面（转发），并顺带回答了"kube-proxy 挂了会怎样"。
- ⚠️ 风险：不要说成"流量经过 kube-proxy 代理转发"，这是经典错误答案。

**面试官（追问）**：iptables 模式下，一个请求进来之后具体发生了什么？

**候选人（参考）**：请求的目的地址是 ClusterIP 加 Service 端口。内核 netfilter 里先命中 KUBE-SERVICES 相关规则，然后进入这个 Service 对应的链做后端选择——多个 Pod 时用概率模块做随机负载均衡，最后在 SEP 链里执行 DNAT，目的地址被改写成 PodIP 加 targetPort，之后数据包按普通 Pod 间通信走 CNI 网络送出去。回程包由 conntrack 自动做反向转换，客户端完全无感知。

**点评**：
- ✅ 亮点：说出"概率负载均衡"和 conntrack 回程两个细节，属于超出预期的深度。
- 🎯 面试官意图：检验是否真的看过 iptables 规则，还是只知道"DNAT"一个词。

**面试官（追问）**：集群里 Service 有几千个的时候，iptables 模式会怎么样？

**候选人（参考）**：iptables 规则是线性匹配的，规则数量上去之后，每个包的匹配开销会明显增加，规则同步也会变慢。所以大规模集群一般用 IPVS 模式，它基于哈希表查找，还支持 rr、最小连接数这些成熟算法。再往前走一步，Cilium 这类 eBPF 数据面甚至可以完全替代 kube-proxy。

**点评**：
- ✅ 亮点：性能问题答到"线性匹配对哈希查找"就够面试用了，带上 eBPF 是锦上添花。
- 🎯 面试官意图：考察规模意识，判断有没有维护过大集群。

---

### 场景三：实战排障——Ingress 返回 503

**面试官**：线上反馈你们的一个网站突然 503 了，入口是 Ingress，说说你怎么排查。

**候选人（参考）**：我会从后端往前逐层验证。第一步看 Pod：是不是 Running、readinessProbe 有没有过、容器实际监听端口对不对。第二步看 Service 有没有选中 Pod：查 EndpointSlice，如果为空，大概率是 selector 和 Pod label 不匹配，或者 Pod 没 Ready。第三步在集群内直接 curl Service 名和 ClusterIP 验证数据面：名称不通查 CoreDNS，都不通查 NetworkPolicy 和 kube-proxy。第四步后端全部正常时，才回头看 Ingress 这层：describe 看 IngressClass 对不对、规则引用的 Service 名和端口对不对，再看 Controller 日志。

**点评**：
- ✅ 亮点：展现"每一层独立可验证"的排查心法，而不是漫无目的地乱翻日志。
- 🎯 面试官意图：503 题考的不是命令背诵，而是排查顺序和方法论。

**面试官（追问）**：怎么区分这是 404 问题还是 503 问题？

**候选人（参考）**：404 是规则层的问题：请求没匹配到任何 Ingress 规则，典型原因是 Host 头没带对、path 或 pathType 不匹配，或者规则压根没被 Controller 生效。503 是后端层的问题：规则匹配上了，但后面没有健康的 Endpoint 可以接。简单说，404 找路由，503 找后端。

**点评**：
- ✅ 亮点：一句话给出"404 找路由，503 找后端"的判断口诀，简洁且准确。

**面试官（追问）**：`kubectl get ingress` 的 ADDRESS 列是空的，说明什么？

**候选人（参考）**：说明没有任何 Ingress Controller 认领这条 Ingress。常见原因有三个：Controller 没装；`spec.ingressClassName` 写错或与 Controller 声明的 IngressClass 不匹配；Controller 自己的 LoadBalancer 还没创建成功。只有 Ingress 资源、没有 Controller，是什么都不会发生的——Ingress 是菜谱，Controller 才是厨师。

**点评**：
- ✅ 亮点：给出三个可操作的检查方向，并用菜谱/厨师类比收尾，表达有记忆点。

---

### 场景四：压力追问——反直觉细节

**面试官**：Pod 里 ping ClusterIP 能通吗？

**候选人（参考）**：iptables 模式下通常不通。kube-proxy 写的规则只针对 TCP/UDP 这类协议按端口匹配做 DNAT，ICMP 请求没有端口概念，命不中规则，也就没人应答。这不是故障，是正常现象。验证 Service 连通性应该用 curl 或 wget 这类基于端口的方式。

**点评**：
- ✅ 亮点：不仅答出结论，还解释了为什么，并给出替代验证手段。
- ⚠️ 注意：IPVS 模式下 VIP 行为有差异，不确定细节就明确限定"iptables 模式下"，不要把话说死。

**面试官**：外部流量经过 NodePort 转到另一个节点的 Pod 后，Pod 看到的源 IP 是谁？

**候选人（参考）**：默认情况下看到的是节点 IP，不是真实客户端 IP。因为跨节点转发时内核做了一次 SNAT，把源地址改成入口节点的地址，以保证回程包能原路回来。想保留客户端源 IP，要把 Service 的 `externalTrafficPolicy` 设成 Local，只把流量交给入口节点上的本地 Pod。代价是负载可能不均，没有本地 Pod 的节点接不了流量，云 LB 的健康检查也要跟着配合。

**点评**：
- ✅ 亮点：先答默认行为，再解释 SNAT 的"为什么"，最后给方案及其代价，结构完整。
- 🎯 面试官意图：检验生产经验的经典题，纯背文档的人往往答不出代价那一半。

**面试官（收尾）**：最后，把从浏览器输入网址到 Pod 收到请求的完整链路讲一遍。

**候选人（参考）**：浏览器发起请求，DNS 把域名解析到云负载均衡器的公网 IP。LB 把流量转到后端节点，打的是 Ingress Controller 的 Service 端口。请求进入 Controller Pod，它根据 Ingress 资源里的 Host 和 Path 规则选中后端 Service，然后从 EndpointSlice 拿到健康的 Pod 列表，把请求发给其中一个 Pod 的 targetPort。这一路上，kube-proxy 负责把 Service 的转发规则维护在每个节点的内核里，CNI 负责让包真正到达目标 Pod。整条链路里，Ingress 只是一份声明式规则，真正搬包的是 Controller、内核和 CNI。

**点评**：
- ✅ 亮点：一镜到底、层次清楚，最后一句点出"规则与执行分离"的本质，是很好的收束。
- 🎯 面试官意图：收尾题看全局串联能力，答好基本可以锁定网络这轮。

---

### 自练检查清单

- 每个回答能否在 60 秒内说完不卡壳？
- 是否至少有一处"踩过坑"式的细节（如 ping 不通 ClusterIP、源 IP 变节点 IP）？
- 追问被问倒时，能否说清"我知道边界在哪，回去会怎么查"？
- 收尾题能否不看稿一镜到底？

---

## 相关链接

- [[05-网络/01-K8s网络核心/57-kubernetes-service-ingress-interview.md|Kubernetes Service 与 Ingress 网络面经（理论体系）]]
- [[05-网络/01-K8s网络核心/10-kube-proxy-modes-performance.md|Kube-proxy 实现模式与性能优化]]
- [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization.md|CoreDNS 故障排查与性能优化]]
- [[05-网络/01-K8s网络核心/35-network-troubleshooting.md|网络故障诊断与链路排查]]
- [[05-网络/01-K8s网络核心/00-network-in-nutshell.md|Kubernetes 网络速览]]
- [[05-网络/01-K8s网络核心/20-ingress-fundamentals.md|Ingress 基础]]
- [[05-网络/01-K8s网络核心/05-flannel-complete-guide.md|Flannel 完整指南]]
- [[05-网络/01-K8s网络核心/06-terway-advanced-guide.md|Terway 高级指南]]
- [[05-网络/01-K8s网络核心/11-service-advanced-features.md|Service 高级特性与应用案例]]
- [[05-网络/01-K8s网络核心/27-ingress-production-best-practices.md|Ingress 生产最佳实践]]
- [[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns.md|DNS 服务发现与 CoreDNS 调优]]
- [[05-网络/01-K8s网络核心/14-coredns-architecture-principles.md|CoreDNS 架构与核心原理]]
- [[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice.md|NetworkPolicy 深度实践指南]]
- [[05-网络/01-K8s网络核心/37-gateway-api-overview.md|Gateway API配置]]
- [[05-网络/01-K8s网络核心/19-network-encryption-mtls.md|网络加密与 mTLS]]
- [[05-网络/01-K8s网络核心/30-egress-traffic-management.md|Egress 流量管理]]
- [[05-网络/01-K8s网络核心/34-multi-cluster-networking.md|多集群网络互联]]
- [[05-网络/01-K8s网络核心/36-network-performance-tuning.md|网络性能调优]]
- [[05-网络/01-K8s网络核心/50-ipv6-dual-stack-production.md|IPv6 双栈生产实践]]
